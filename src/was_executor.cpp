#include "wasm_executor.hpp"
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <bit>
#include "struct.h"
#include "wasm_memory.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>

static inline int countl_zero32(uint32_t x) { return x == 0 ? 32 : __builtin_clz(x); }
static inline int countr_zero32(uint32_t x) { return x == 0 ? 32 : __builtin_ctz(x); }
static inline int popcount32(uint32_t x) { return __builtin_popcount(x); }
static inline int countl_zero64(uint64_t x) { return x == 0 ? 64 : __builtin_clzll(x); }
static inline int countr_zero64(uint64_t x) { return x == 0 ? 64 : __builtin_ctzll(x); }
static inline int popcount64(uint64_t x) { return __builtin_popcountll(x); }

void WasmExecutor::execute(
    const FuncDef& func,
    std::unordered_map<int, FuncDef>& functionsByID,
    std::unordered_map<std::string, FuncDef>& functionByName,
    WasmMemory& memory,
    std::unordered_map<std::string, WasmGlobal>& globals
) {
    WasmStack stack;
    stack.clear();
    std::unordered_map<std::string, WasmValue> locals;
    for (const auto& [pname, pval] : func.params) {
        locals[pname] = pval;
    }
    std::cout << "\033[1;36m[executor:execute]\033[0m Executing function '" << func.name << "' (index " << func.index << ").\n";
    std::vector<bool> skipStack;
    std::vector<BlockInfo> blockStack;
    auto printValue = [](const WasmValue& v) {
        switch (v.type) {
            case ValueType::I32: std::cout << v.i32; break;
            case ValueType::I64: std::cout << v.i64; break;
            case ValueType::F32: std::cout << v.f32; break;
            case ValueType::F64: std::cout << v.f64; break;
        }
    };

    auto binaryOp = [&](auto fn, const std::string& tag, ValueType t) {
        WasmValue b = stack.pop(), a = stack.pop(), r;
        if (t == ValueType::I32) r = WasmValue(static_cast<int32_t>(fn(a.i32, b.i32)));
        else if (t == ValueType::I64) r = WasmValue(static_cast<int64_t>(fn(a.i64, b.i64)));
        else if (t == ValueType::F32) r = WasmValue(static_cast<float>(fn(a.f32, b.f32)));
        else r = WasmValue(static_cast<double>(fn(a.f64, b.f64)));
        std::cout << "\033[1;36m[executor:" << tag << "]\033[0m ";
        printValue(a);
        std::cout << ", ";
        printValue(b);
        std::cout << " -> ";
        printValue(r);
        std::cout << "\n";
        stack.push(r);
    };

    auto cmpOp = [&](auto fn, const std::string& tag, ValueType t) {
        WasmValue b = stack.pop(), a = stack.pop();
        int32_t res = 0;
        if (t == ValueType::I32) res = fn(a.i32, b.i32);
        else if (t == ValueType::I64) res = fn(a.i64, b.i64);
        else if (t == ValueType::F32) res = fn(a.f32, b.f32);
        else res = fn(a.f64, b.f64);
        stack.push(WasmValue(static_cast<int32_t>(res ? 1 : 0)));
        std::cout << "\033[1;36m[executor:" << tag << "]\033[0m = " << res << "\n";
    };

    auto unaryOp = [&](auto fn, const std::string& tag, ValueType t) {
        WasmValue a = stack.pop(), r;
        if (t == ValueType::I32) r = WasmValue(static_cast<int32_t>(fn(a.i32)));
        else if (t == ValueType::I64) r = WasmValue(static_cast<int64_t>(fn(a.i64)));
        else if (t == ValueType::F32) r = WasmValue(static_cast<float>(fn(a.f32)));
        else r = WasmValue(static_cast<double>(fn(a.f64)));
        std::cout << "\033[1;36m[executor:" << tag << "]\033[0m -> ";
        printValue(r);
        std::cout << "\n";
        stack.push(r);
    };
    
    auto doStore = [&](auto fn, const std::string& tag) {
        WasmValue v = stack.pop(), addr = stack.pop();
        fn(addr.i32, v);
        std::cout << "\033[1;36m[executor:" << tag << "]\033[0m mem[" << addr.i32 << "] = ";
        printValue(v);
        std::cout << "\n";
    };

    auto doLoad = [&](auto fn, auto castFn, const std::string& tag, ValueType t) {
        WasmValue addr = stack.pop();
        auto val = castFn(fn(addr.i32));
        if (t == ValueType::I32) stack.push(WasmValue(static_cast<int32_t>(val)));
        else if (t == ValueType::I64) stack.push(WasmValue(static_cast<int64_t>(val)));
        else if (t == ValueType::F32) stack.push(WasmValue(static_cast<float>(val)));
        else stack.push(WasmValue(static_cast<double>(val)));
        std::cout << "\033[1;36m[executor:" << tag << "]\033[0m mem[" << addr.i32 << "] → " << static_cast<double>(val) << "\n";
    };
    
    auto resolveDepth = [&](const std::string& tok) -> int {
        if (!tok.empty() && tok[0] == '$') {
            for (int i = (int)blockStack.size()-1, d = 0; i >= 0; --i, ++d) {
                if (blockStack[i].label == tok) return d;
            }
            std::cerr << "\033[1;31m[executor]\033[0m unknown label " << tok << " -> depth=0\n";
            return 0;
        }
        try { return std::stoi(tok); }
        catch (...) { return 0; }
    };

    auto firstTok = [](const std::string& s) {
        std::istringstream is(s);
        std::string t; 
        is >> t;
        return t;
    };


    auto safeDiv32 = [](int32_t a, int32_t b) { return b == 0 ? 0 : a / b; };
    auto safeDiv64 = [](int64_t a, int64_t b) { return b == 0 ? 0 : a / b; };
    auto safeRem32 = [](int32_t a, int32_t b) { return b == 0 ? 0 : a % b; };
    auto safeRem64 = [](int64_t a, int64_t b) { return b == 0 ? 0 : a % b; };

    for (size_t pc = 0; pc < func.body.size(); ++pc) {
        const std::string& line = func.body[pc];
        std::istringstream iss(line);
        std::string op; iss >> op;
        if (op.empty()) continue;
        std::cout << "\033[1;36m[executor:instr]\033[0m " << op << "\n";

        if (op == "i32.const" || op == "i64.const" || op == "f32.const" || op == "f64.const") {
            std::string v; iss >> v;
            if (op == "i32.const") {
                int32_t val = 0;
                if (v.rfind("0x", 0) == 0 || v.rfind("0X", 0) == 0) {
                    val = static_cast<int32_t>(std::stoul(v, nullptr, 16));
                } else {
                    val = static_cast<int32_t>(std::stol(v));
                }
                stack.push(WasmValue(val));
            } else if (op == "i64.const") {
                int64_t val = 0;
                if (v.rfind("0x", 0) == 0 || v.rfind("0X", 0) == 0) {
                    val = static_cast<int64_t>(std::stoull(v, nullptr, 16));
                } else {
                    val = static_cast<int64_t>(std::stoll(v));
                }
                stack.push(WasmValue(val));
            } else if (op == "f32.const") {
                stack.push(WasmValue(static_cast<float>(std::stof(v))));
            } else if (op == "f64.const") {
                stack.push(WasmValue(static_cast<double>(std::stod(v))));
            }
            continue;
        } else if (op == "select") {
            WasmValue cond = stack.pop();
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            WasmValue result = (cond.i32 != 0) ? a : b;
            std::cout << "\033[1;36m[select]\033[0m cond=" << cond.i32
                    << " → selected=";
            switch (result.type) {
                case ValueType::I32: std::cout << result.i32; break;
                case ValueType::I64: std::cout << result.i64; break;
                case ValueType::F32: std::cout << result.f32; break;
                case ValueType::F64: std::cout << result.f64; break;
            }
            std::cout << "\n";
            stack.push(result);
            continue;
        } else if (op == "return") {
            std::cout << "\033[1;36m[executor:return]\033[0m returning from function "
                    << (func.name.empty() ? "[anon]" : func.name) << "\n";
            return;
        }else if (op == "call") {
            std::string target;
            iss >> target;

            bool isNumeric = !target.empty() && std::all_of(target.begin(), target.end(), ::isdigit);
            FuncDef* callee = nullptr;

            if (isNumeric) {
                int funcIndex = std::stoi(target);
                auto it = functionsByID.find(funcIndex);
                if (it == functionsByID.end()) {
                    std::cerr << "\033[1;31m[executor:call]\033[0m Error: function index "
                            << funcIndex << " not found!\n";
                    continue;
                }
                callee = &it->second;
                std::cout << "\033[1;36m[executor:call]\033[0m Calling function index "
                        << funcIndex << " (" << (callee->name.empty() ? "[anon]" : callee->name) << ")\n";
            } else {
                auto it = functionByName.find(target);
                if (it == functionByName.end()) {
                    std::cerr << "\033[1;31m[executor:call]\033[0m Error: function name '"
                            << target << "' not found!\n";
                    continue;
                }
                callee = &it->second;
                std::cout << "\033[1;36m[executor:call]\033[0m Calling function name '"
                        << target << "' (index " << callee->index << ")\n";
            }

            size_t paramCount = callee->params.size();
            std::vector<WasmValue> args;
            args.reserve(paramCount);

            for (size_t i = 0; i < paramCount; ++i) {
                if (stack.empty()) {
                    std::cerr << "\033[1;31m[executor:call]\033[0m Error: stack underflow while reading args!\n";
                    break;
                }
                args.push_back(stack.pop());
            }
            std::reverse(args.begin(), args.end());

            size_t i = 0;
            for (auto& [paramName, paramVal] : callee->params) {
                if (i < args.size()) {
                    paramVal = args[i];
                    std::cout << "\033[1;36m[executor:call]\033[0m arg "
                            << (paramName.empty() ? "_" : paramName) << " = ";
                    switch (args[i].type) {
                        case ValueType::I32: std::cout << args[i].i32; break;
                        case ValueType::I64: std::cout << args[i].i64; break;
                        case ValueType::F32: std::cout << args[i].f32; break;
                        case ValueType::F64: std::cout << args[i].f64; break;
                    }
                    std::cout << "\n";
                }
                ++i;
            }
            
            std::cout << "\033[1;34m[debug:parseFunction]\033[0m Params detected: ";
            for (auto& [pname, _] : callee->params) std::cout << pname << " ";
            std::cout << "\n";


            WasmExecutor nestedExec;
            nestedExec.execute(*callee, functionsByID, functionByName, memory, globals);

            if (!nestedExec.lastStack.empty()) {
                WasmValue retVal = nestedExec.lastStack.top();
                stack.push(retVal);
                std::cout << "\033[1;36m[executor:call]\033[0m returned value pushed to caller stack: ";
                switch (retVal.type) {
                    case ValueType::I32: std::cout << retVal.i32; break;
                    case ValueType::I64: std::cout << retVal.i64; break;
                    case ValueType::F32: std::cout << retVal.f32; break;
                    case ValueType::F64: std::cout << retVal.f64; break;
                    default: std::cout << "(none)"; break;
                }
                std::cout << "\n";
            } else {
                std::cerr << "\033[1;31m[executor:call]\033[0m Warning: callee returned no value!\n";
            }


            continue;
        } else if (op == "memory.size") {
            int32_t pages = memory.size();
            stack.push(WasmValue(pages));
            std::cout << "\033[1;36m[executor:memory.size]\033[0m → pages=" << pages << "\n";
            continue;
        }
        else if (op == "memory.grow") {
            if (stack.empty()) {
                std::cerr << "\033[1;31m[executor:memory.grow]\033[0m Error: stack underflow\n";
                continue;
            }
            WasmValue pages = stack.pop();
            if (pages.type != ValueType::I32) {
                std::cerr << "\033[1;31m[executor:memory.grow]\033[0m Error: expected i32 argument\n";
                continue;
            }
            int32_t oldPages = memory.grow(pages.i32);
            stack.push(WasmValue(oldPages));
            continue;
        } else if (op.find("store") != std::string::npos) {
            if (op == "i32.store8") doStore([&](int32_t a, WasmValue v){ memory.store8(a, static_cast<uint8_t>(v.i32)); }, "executor:" + op);
            else if (op == "i32.store16") doStore([&](int32_t a, WasmValue v){ memory.store16(a, static_cast<uint16_t>(v.i32)); }, "executor:" + op);
            else if (op == "i32.store") doStore([&](int32_t a, WasmValue v){ memory.store32(a, v.i32); }, "executor:" + op);
            else if (op == "i64.store8") doStore([&](int32_t a, WasmValue v){ memory.store8(a, static_cast<uint8_t>(v.i64)); }, "executor:" + op);
            else if (op == "i64.store16") doStore([&](int32_t a, WasmValue v){ memory.store16(a, static_cast<uint16_t>(v.i64)); }, "executor:" + op);
            else if (op == "i64.store32") doStore([&](int32_t a, WasmValue v){ memory.store32(a, static_cast<int32_t>(v.i64)); }, "executor:" + op);
            else if (op == "i64.store") doStore([&](int32_t a, WasmValue v){ memory.store64(a, v.i64); }, "executor:" + op);
            else if (op == "f32.store") doStore([&](int32_t a, WasmValue v){ memory.storeF32(a, v.f32); }, "executor:" + op);
            else if (op == "f64.store") doStore([&](int32_t a, WasmValue v){ memory.storeF64(a, v.f64); }, "executor:" + op);
            continue;
        } else if (op.find("load") != std::string::npos) {
            if (op == "i32.load8_s") doLoad([&](int32_t a){ return static_cast<int8_t>(memory.load8(a)); }, [](int8_t x){return static_cast<int32_t>(x);}, "executor:" + op, ValueType::I32);
            else if (op == "i32.load8_u") doLoad([&](int32_t a){ return memory.load8(a); }, [](uint8_t x){return static_cast<int32_t>(x);}, "executor:" + op, ValueType::I32);
            else if (op == "i32.load16_s") doLoad([&](int32_t a){ return static_cast<int16_t>(memory.load16(a)); }, [](int16_t x){return static_cast<int32_t>(x);}, "executor:" + op, ValueType::I32);
            else if (op == "i32.load16_u") doLoad([&](int32_t a){ return memory.load16(a); }, [](uint16_t x){return static_cast<int32_t>(x);}, "executor:" + op, ValueType::I32);
            else if (op == "i32.load") doLoad([&](int32_t a){ return memory.load32(a); }, [](int32_t x){return x;}, "executor:" + op, ValueType::I32);
            else if (op == "i64.load8_s") doLoad([&](int32_t a){ return static_cast<int8_t>(memory.load8(a)); }, [](int8_t x){return static_cast<int64_t>(x);}, "executor:" + op, ValueType::I64);
            else if (op == "i64.load8_u") doLoad([&](int32_t a){ return memory.load8(a); }, [](uint8_t x){return static_cast<int64_t>(x);}, "executor:" + op, ValueType::I64);
            else if (op == "i64.load16_s") doLoad([&](int32_t a){ return static_cast<int16_t>(memory.load16(a)); }, [](int16_t x){return static_cast<int64_t>(x);}, "executor:" + op, ValueType::I64);
            else if (op == "i64.load16_u") doLoad([&](int32_t a){ return memory.load16(a); }, [](uint16_t x){return static_cast<int64_t>(x);}, "executor:" + op, ValueType::I64);
            else if (op == "i64.load32_s") doLoad([&](int32_t a){ return memory.load32(a); }, [](int32_t x){return static_cast<int64_t>(x);}, "executor:" + op, ValueType::I64);
            else if (op == "i64.load32_u") doLoad([&](int32_t a){ return static_cast<uint32_t>(memory.load32(a)); }, [](uint32_t x){return static_cast<int64_t>(x);}, "executor:" + op, ValueType::I64);
            else if (op == "i64.load") doLoad([&](int32_t a){ return memory.load64(a); }, [](int64_t x){return x;}, "executor:" + op, ValueType::I64);
            else if (op == "f32.load") doLoad([&](int32_t a){ return memory.loadF32(a); }, [](float x){return x;}, "executor:" + op, ValueType::F32);
            else if (op == "f64.load") doLoad([&](int32_t a){ return memory.loadF64(a); }, [](double x){return x;}, op, ValueType::F64);
            continue;
        } else if (op == "(local") {
            std::string name, type; iss >> name >> type;
            if (!type.empty() && type.back() == ')') type.pop_back();
            locals[name] = {};
            std::cout << "\033[1;36m[local]\033[0m Declared " << name << " (" << type << ")\n";
            continue;
        } else if (op == "local.set" || op == "local.get" || op == "local.tee") {
            std::string name; iss >> name;
            if (op == "local.set") locals[name] = stack.pop();
            else if (op == "local.get") stack.push(locals[name]);
            else locals[name] = stack.top();
            continue;
        } else if (op == "global.get" || op == "global.set") {
            std::string name; iss >> name;
            WasmGlobal& g = globals[name];
            if (op == "global.get") stack.push(g.value);
            else g.value = stack.pop();
            continue;
        } else if (op == "if" || op.rfind("if", 0) == 0) {
            WasmValue cond = stack.pop();
            bool condition = (cond.i32 != 0);
            std::cout << "\033[1;36m[executor:if]\033[0m condition=" << cond.i32
                    << " (" << (condition ? "true" : "false") << ")\n";
            if (!condition) {
                int depth = 0;
                for (++pc; pc < func.body.size(); ++pc) {
                    const std::string& next = func.body[pc];
                    if (next.rfind("if", 0) == 0) depth++;
                    else if (next == "else" && depth == 0) break;
                    else if (next == "end") {
                        if (depth == 0) break;
                        depth--;
                    }
                }
                skipStack.push_back(true);
            } else {
                skipStack.push_back(false);
            }
            continue;
        } else if (op == "else") {
            if (skipStack.empty()) {
                std::cerr << "[executor:else] Error: else without matching if!\n";
                continue;
            }

            bool parentSkipped = skipStack.back();
            skipStack.pop_back();

            if (!parentSkipped) {
                int depth = 0;
                for (++pc; pc < func.body.size(); ++pc) {
                    const std::string& next = func.body[pc];
                    if (next.rfind("if", 0) == 0) depth++;
                    else if (next == "end") {
                        if (depth == 0) break;
                        depth--;
                    }
                }
                std::cout << "\033[1;36m[executor:else]\033[0m skipped\n";
            } else {
                std::cout << "\033[1;36m[executor:else]\033[0m executing\n";
            }
            continue;
        } else if (op == "block") {
            std::string lbl;
            if (iss >> lbl && !lbl.empty() && lbl[0] == '$') {
                std::cout << "\033[1;36m[executor:block]\033[0m begin block " << lbl << " (pc=" << pc << ")\n";
                blockStack.push_back({pc, false, lbl});
            } else {
                std::cout << "\033[1;36m[executor:block]\033[0m begin block (pc=" << pc << ")\n";
                blockStack.push_back({pc, false, ""});
            }
            continue;
        } else if (op == "br_if") {
            std::string tok; iss >> tok; 
            int32_t depth = tok.empty() ? 0 : resolveDepth(tok);
            WasmValue cond = stack.pop();
            bool condition = (cond.i32 != 0);

            std::cout << "\033[1;36m[executor:br_if]\033[0m depth=" << depth
                    << " condition=" << cond.i32
                    << " (" << (condition ? "true" : "false") << ")\n";

            if (!condition) continue;

            if (depth < 0 || depth >= static_cast<int32_t>(blockStack.size())) {
                std::cerr << "\033[1;31m[executor:br_if]\033[0m invalid depth!\n";
                continue;
            }

            BlockInfo target = blockStack[blockStack.size() - 1 - depth];
            std::cout << "\033[1;36m[executor:br_if]\033[0m → target "
                    << (target.isLoop ? "loop" : "block")
                    << " (pc=" << target.startPC << ")\n";

            if (target.isLoop) {
                std::cout << "\033[1;36m[executor:br_if]\033[0m → continue loop\n";
                pc = target.startPC;
            } else {
                int open = 0;
                int toClose = depth + 1;

                for (++pc; pc < func.body.size(); ++pc) {
                    std::string t = firstTok(func.body[pc]);
                    if (t == "block" || t == "loop") {
                        ++open;
                    } else if (t == "end") {
                        if (open == 0) {
                            --toClose;
                            if (toClose == 0) break;
                        } else {
                            --open;
                        }
                    }
                }
                int pops = depth + 1;
                while (pops-- > 0 && !blockStack.empty())
                    blockStack.pop_back();
                continue;
            }
            continue;
        } else if (op == "br") {
            std::string tok; iss >> tok; 
            int32_t depth = tok.empty() ? 0 : resolveDepth(tok);
            std::cout << "\033[1;36m[executor:br]\033[0m depth=" << depth << "\n";

            if (depth < 0 || depth >= static_cast<int32_t>(blockStack.size())) {
                std::cerr << "\033[1;31m[executor:br]\033[0m invalid depth!\n";
                continue;
            }

            BlockInfo target = blockStack[blockStack.size() - 1 - depth];
            std::cout << "\033[1;36m[executor:br]\033[0m → target "
                    << (target.isLoop ? "loop" : "block")
                    << " (pc=" << target.startPC << ")\n";

            if (target.isLoop) {
                std::cout << "\033[1;36m[executor:br]\033[0m → continue loop\n";
                pc = target.startPC;
            } else {
                int open = 0;
                int toClose = depth + 1;

                for (++pc; pc < func.body.size(); ++pc) {
                    std::string t = firstTok(func.body[pc]);
                    if (t == "block" || t == "loop") {
                        ++open;
                    } else if (t == "end") {
                        if (open == 0) {
                            --toClose;
                            if (toClose == 0) break;
                        } else {
                            --open;
                        }
                    }
                }
                std::cout << "\033[1;36m[executor:br]\033[0m → break to end of block\n";
                int pops = depth + 1;
                while (pops-- > 0 && !blockStack.empty())
                    blockStack.pop_back();
                continue;
            }
            continue;
        } else if (op == "end") {
            if (!blockStack.empty()) blockStack.pop_back();
            if (!skipStack.empty()) skipStack.pop_back();
            std::cout << "\033[1;36m[executor:end]\033[0m block end\n";
            continue;
        } else if (op == "drop") {
            if (!stack.empty()) {
                WasmValue dropped = stack.pop();
                std::cout << "\033[1;36m[executor:drop]\033[0m dropped value \n";
            } else {
                std::cerr << "\033[1;31m[executor:drop]\033[0m Error: stack underflow!\n";
            }
            continue;
        } else if (op == "nop") {
            std::cout << "\033[1;36m[executor:nop]\033[0m (no operation)\n";
            continue;
        } else if (op == "loop") {
            std::string lbl;
            if (iss >> lbl && !lbl.empty() && lbl[0] == '$') {
                std::cout << "\033[1;36m[executor:loop]\033[0m begin loop " << lbl << " (pc=" << pc << ")\n";
                blockStack.push_back({pc, true, lbl});
            } else {
                std::cout << "\033[1;36m[executor:loop]\033[0m begin loop (pc=" << pc << ")\n";
                blockStack.push_back({pc, true, ""});
            }
            continue;
        } else if (op == "br_table") {
            std::vector<std::string> labels;
            std::string tok;
            while (iss >> tok)
                labels.push_back(tok);
            if (labels.empty()) {
                std::cerr << "\033[1;31m[executor:br_table]\033[0m no labels found!\n";
                continue;
            }
            WasmValue indexVal = stack.pop();
            uint32_t index = static_cast<uint32_t>(indexVal.i32);
            std::cout << "\033[1;36m[executor:br_table]\033[0m index=" << index << " → ";
            std::string targetLabel = (index < labels.size() - 1) 
                ? labels[index] 
                : labels.back();
            std::cout << "target=" << targetLabel << "\n";
            int depth = resolveDepth(targetLabel);
            if (depth < 0 || depth >= static_cast<int32_t>(blockStack.size())) {
                std::cerr << "\033[1;31m[executor:br_table]\033[0m invalid depth for label "
                          << targetLabel << "!\n";
                continue;
            }
            BlockInfo target = blockStack[blockStack.size() - 1 - depth];
            std::cout << "\033[1;36m[executor:br_table]\033[0m → target "
                      << (target.isLoop ? "loop" : "block")
                      << " (pc=" << target.startPC << ")\n";

            if (target.isLoop) {
                std::cout << "\033[1;36m[executor:br_table]\033[0m → continue loop\n";
                pc = target.startPC;
            } else {
                int open = 0;
                int toClose = depth + 1;
                for (++pc; pc < func.body.size(); ++pc) {
                    std::string t = firstTok(func.body[pc]);
                    if (t == "block" || t == "loop") {
                        ++open;
                    } else if (t == "end") {
                        if (open == 0) {
                            --toClose;
                            if (toClose == 0) break;
                        } else {
                            --open;
                        }
                    }
                }
                std::cout << "\033[1;36m[executor:br_table]\033[0m → break to end of block\n";
                int pops = depth + 1;
                while (pops-- > 0 && !blockStack.empty())
                    blockStack.pop_back();

                continue;
            }
            continue;
        } else if (op == "i32.reinterpret_f32") {
            if (stack.empty()) {
                std::cerr << "\033[1;31m[executor:i32.reinterpret_f32]\033[0m Error: stack underflow\n";
                continue;
            }
            WasmValue val = stack.pop();
            if (val.type != ValueType::F32) {
                std::cerr << "\033[1;31m[executor:i32.reinterpret_f32]\033[0m Error: expected f32 on stack, got different type\n";
                continue;
            }
            int32_t bits;
            std::memcpy(&bits, &val.f32, sizeof(bits));
            WasmValue reinterpreted(bits);
            stack.push(reinterpreted);
            union {
                int32_t i;
                float f;
            } u;
            u.i = bits;
            std::cout << "\033[1;36m[executor:i32.reinterpret_f32]\033[0m "
                    << "f32=" << val.f32
                    << " → bit pattern 0x" << std::hex << bits
                    << std::dec << " (interpreted f32=" << u.f << ")\n";
        } else if (op == "f32.convert_i32_s") {
            if (stack.empty()) {
                std::cerr << "\033[1;31m[executor:f32.convert_i32_s]\033[0m Error: stack underflow\n";
                continue;
            }
            WasmValue val = stack.pop();
            if (val.type != ValueType::I32) {
                std::cerr << "\033[1;31m[executor:f32.convert_i32_s]\033[0m Error: expected i32 on stack, got different type\n";
                continue;
            }
            float f = static_cast<float>(val.i32);
            WasmValue result(f);
            stack.push(result);
            std::cout << "\033[1;36m[executor:f32.convert_i32_s]\033[0m "
                    << "i32=" << val.i32 << " → f32=" << f << "\n";
        } else if (op == "f32.convert_i32_u") {
            if (stack.empty()) {
                std::cerr << "\033[1;31m[executor:f32.convert_i32_u]\033[0m Error: stack underflow\n";
                continue;
            }
            WasmValue val = stack.pop();
            if (val.type != ValueType::I32) {
                std::cerr << "\033[1;31m[executor:f32.convert_i32_u]\033[0m Error: expected i32\n";
                continue;
            }
            float f = static_cast<float>(static_cast<uint32_t>(val.i32));
            stack.push(WasmValue(f));
            std::cout << "\033[1;36m[executor:f32.convert_i32_u]\033[0m i32=" << val.i32 << " → f32=" << f << "\n";
        } else if (op == "i32.trunc_f32_s") {
            if (stack.empty()) {
                std::cerr << "\033[1;31m[executor:i32.trunc_f32_s]\033[0m Error: stack underflow\n";
                continue;
            }
            WasmValue val = stack.pop();
            if (val.type != ValueType::F32) {
                std::cerr << "\033[1;31m[executor:i32.trunc_f32_s]\033[0m Error: expected f32\n";
                continue;
            }
            int32_t i = static_cast<int32_t>(std::trunc(val.f32));
            stack.push(WasmValue(i));
            std::cout << "\033[1;36m[executor:i32.trunc_f32_s]\033[0m f32=" << val.f32 << " → i32=" << i << "\n";
        } else if (op == "i32.trunc_f32_u") {
            if (stack.empty()) {
                std::cerr << "\033[1;31m[executor:i32.trunc_f32_u]\033[0m Error: stack underflow\n";
                continue;
            }
            WasmValue val = stack.pop();
            if (val.type != ValueType::F32) {
                std::cerr << "\033[1;31m[executor:i32.trunc_f32_u]\033[0m Error: expected f32\n";
                continue;
            }
            uint32_t i = static_cast<uint32_t>(std::trunc(val.f32));
            stack.push(WasmValue(static_cast<int32_t>(i)));
            std::cout << "\033[1;36m[executor:i32.trunc_f32_u]\033[0m f32=" << val.f32 << " → u32=" << i << "\n";
        } else if (op == "f64.convert_i32_s") {
            if (stack.empty()) {
                std::cerr << "\033[1;31m[executor:f64.convert_i32_s]\033[0m Error: stack underflow\n";
                continue;
            }
            WasmValue val = stack.pop();
            if (val.type != ValueType::I32) {
                std::cerr << "\033[1;31m[executor:f64.convert_i32_s]\033[0m Error: expected i32\n";
                continue;
            }
            double d = static_cast<double>(val.i32);
            stack.push(WasmValue(d));
            std::cout << "\033[1;36m[executor:f64.convert_i32_s]\033[0m i32=" << val.i32 << " → f64=" << d << "\n";
        } else if (op == "i32.trunc_f64_s") {
            if (stack.empty()) {
                std::cerr << "\033[1;31m[executor:i32.trunc_f64_s]\033[0m Error: stack underflow\n";
                continue;
            }
            WasmValue val = stack.pop();
            if (val.type != ValueType::F64) {
                std::cerr << "\033[1;31m[executor:i32.trunc_f64_s]\033[0m Error: expected f64\n";
                continue;
            }
            int32_t i = static_cast<int32_t>(std::trunc(val.f64));
            stack.push(WasmValue(i));
            std::cout << "\033[1;36m[executor:i32.trunc_f64_s]\033[0m f64=" << val.f64 << " → i32=" << i << "\n";
        } else if (op == "f64.promote_f32") {
            if (stack.empty()) {
                std::cerr << "\033[1;31m[executor:f64.promote_f32]\033[0m Error: stack underflow\n";
                continue;
            }
            WasmValue val = stack.pop();
            if (val.type != ValueType::F32) {
                std::cerr << "\033[1;31m[executor:f64.promote_f32]\033[0m Error: expected f32\n";
                continue;
            }
            double d = static_cast<double>(val.f32);
            stack.push(WasmValue(d));
            std::cout << "\033[1;36m[executor:f64.promote_f32]\033[0m f32=" << val.f32 << " → f64=" << d << "\n";
        } else if (op == "f32.demote_f64") {
            if (stack.empty()) {
                std::cerr << "\033[1;31m[executor:f32.demote_f64]\033[0m Error: stack underflow\n";
                continue;
            }
            WasmValue val = stack.pop();
            if (val.type != ValueType::F64) {
                std::cerr << "\033[1;31m[executor:f32.demote_f64]\033[0m Error: expected f64\n";
                continue;
            }
            float f = static_cast<float>(val.f64);
            stack.push(WasmValue(f));
            std::cout << "\033[1;36m[executor:f32.demote_f64]\033[0m f64=" << val.f64 << " → f32=" << f << "\n";
        } else if (op == "f32.reinterpret_i32") {
            if (stack.empty()) {
                std::cerr << "\033[1;31m[executor:f32.reinterpret_i32]\033[0m Error: stack underflow\n";
                continue;
            }
            WasmValue val = stack.pop();
            if (val.type != ValueType::I32) {
                std::cerr << "\033[1;31m[executor:f32.reinterpret_i32]\033[0m Error: expected i32\n";
                continue;
            }
            float f;
            std::memcpy(&f, &val.i32, sizeof(f));
            stack.push(WasmValue(f));
            std::cout << "\033[1;36m[executor:f32.reinterpret_i32]\033[0m i32=0x" << std::hex << val.i32
                    << std::dec << " → f32=" << f << "\n";
        } else if (op == "i64.reinterpret_f64") {
            if (stack.empty()) {
                std::cerr << "\033[1;31m[executor:i64.reinterpret_f64]\033[0m Error: stack underflow\n";
                continue;
            }
            WasmValue val = stack.pop();
            if (val.type != ValueType::F64) {
                std::cerr << "\033[1;31m[executor:i64.reinterpret_f64]\033[0m Error: expected f64\n";
                continue;
            }
            int64_t bits;
            std::memcpy(&bits, &val.f64, sizeof(bits));
            stack.push(WasmValue(bits));
            std::cout << "\033[1;36m[executor:i64.reinterpret_f64]\033[0m f64=" << val.f64
                    << " → bits=0x" << std::hex << bits << std::dec << "\n";
        } else if (op == "i32.wrap_i64") {
            if (stack.empty()) {
                std::cerr << "\033[1;31m[executor:i32.wrap_i64]\033[0m Error: stack underflow\n";
                continue;
            }
            WasmValue val = stack.pop();
            if (val.type != ValueType::I64) {
                std::cerr << "\033[1;31m[executor:i32.wrap_i64]\033[0m Error: expected i64\n";
                continue;
            }
            int32_t truncated = static_cast<int32_t>(val.i64);
            stack.push(WasmValue(truncated));
            std::cout << "\033[1;36m[executor:i32.wrap_i64]\033[0m i64=" << val.i64
                    << " → i32=" << truncated << "\n";
        } else if (op.rfind("i32.", 0) == 0) {
            if (op == "i32.add") binaryOp([](int32_t a, int32_t b){ return a + b; }, op, ValueType::I32);
            else if (op == "i32.sub") binaryOp([](int32_t a, int32_t b){ return a - b; }, op, ValueType::I32);
            else if (op == "i32.mul") binaryOp([](int32_t a, int32_t b){ return a * b; }, op, ValueType::I32);
            else if (op == "i32.and") binaryOp([](int32_t a, int32_t b){ return a & b; }, op, ValueType::I32);
            else if (op == "i32.or") binaryOp([](int32_t a, int32_t b){ return a | b; }, op, ValueType::I32);
            else if (op == "i32.xor") binaryOp([](int32_t a, int32_t b){ return a ^ b; }, op, ValueType::I32);
            else if (op == "i32.min") binaryOp([](int32_t a, int32_t b){ return a < b ? a : b; }, op, ValueType::I32);
            else if (op == "i32.max") binaryOp([](int32_t a, int32_t b){ return a > b ? a : b; }, op, ValueType::I32);
            else if (op == "i32.abs") unaryOp([](int32_t a){ return a < 0 ? -a : a; }, op, ValueType::I32);
            else if (op == "i32.neg") unaryOp([](int32_t a){ return -a; }, op, ValueType::I32);
            else if (op == "i32.shl") binaryOp([](int32_t a, int32_t b){ return a << (b & 31); }, op, ValueType::I32);
            else if (op == "i32.shr_s") binaryOp([](int32_t a, int32_t b){ return a >> (b & 31); }, op, ValueType::I32);
            else if (op == "i32.shr_u") binaryOp([](int32_t a, int32_t b){ return static_cast<int32_t>(static_cast<uint32_t>(a) >> (b & 31)); }, op, ValueType::I32);
            else if (op == "i32.rotl") binaryOp([](uint32_t a, uint32_t b){ return (a << (b & 31)) | (a >> ((32 - b) & 31)); }, op, ValueType::I32);
            else if (op == "i32.rotr") binaryOp([](uint32_t a, uint32_t b){ return (a >> (b & 31)) | (a << ((32 - b) & 31)); }, op, ValueType::I32);
            else if (op == "i32.div_s") binaryOp(safeDiv32, op, ValueType::I32);
            else if (op == "i32.div_u") binaryOp([](int32_t a, int32_t b){ return b==0?0:static_cast<int32_t>(static_cast<uint32_t>(a)/static_cast<uint32_t>(b)); }, op, ValueType::I32);
            else if (op == "i32.rem_s") binaryOp(safeRem32, op, ValueType::I32);
            else if (op == "i32.rem_u") binaryOp([](int32_t a, int32_t b){ return b==0?0:static_cast<int32_t>(static_cast<uint32_t>(a)%static_cast<uint32_t>(b)); }, op, ValueType::I32);
            else if (op == "i32.eq") cmpOp([](int32_t a, int32_t b){return a==b;}, op, ValueType::I32);
            else if (op == "i32.ne") cmpOp([](int32_t a, int32_t b){return a!=b;}, op, ValueType::I32);
            else if (op == "i32.lt_s") cmpOp([](int32_t a, int32_t b){return a<b;}, op, ValueType::I32);
            else if (op == "i32.lt_u") cmpOp([](uint32_t a, uint32_t b){return a<b;}, op, ValueType::I32);
            else if (op == "i32.gt_s") cmpOp([](int32_t a, int32_t b){return a>b;}, op, ValueType::I32);
            else if (op == "i32.gt_u") cmpOp([](uint32_t a, uint32_t b){return a>b;}, op, ValueType::I32);
            else if (op == "i32.le_s") cmpOp([](int32_t a, int32_t b){return a<=b;}, op, ValueType::I32);
            else if (op == "i32.le_u") cmpOp([](uint32_t a, uint32_t b){return a<=b;}, op, ValueType::I32);
            else if (op == "i32.ge_s") cmpOp([](int32_t a, int32_t b){return a>=b;}, op, ValueType::I32);
            else if (op == "i32.ge_u") cmpOp([](uint32_t a, uint32_t b){return a>=b;}, op, ValueType::I32);
            else if (op == "i32.eqz") unaryOp([](int32_t a){return a==0?1:0;}, op, ValueType::I32);
            else if (op == "i32.clz") unaryOp([](uint32_t a){return a==0?32:countl_zero32(a);}, op, ValueType::I32);
            else if (op == "i32.ctz") unaryOp([](uint32_t a){return a==0?32:countr_zero32(a);}, op, ValueType::I32);
            else if (op == "i32.popcnt") unaryOp([](uint32_t a){return popcount32(a);}, op, ValueType::I32);
            continue;
        } else if (op.rfind("i64.", 0) == 0) {
            if (op == "i64.add") binaryOp([](int64_t a, int64_t b){ return a + b; }, op, ValueType::I64);
            else if (op == "i64.sub") binaryOp([](int64_t a, int64_t b){ return a - b; }, op, ValueType::I64);
            else if (op == "i64.mul") binaryOp([](int64_t a, int64_t b){ return a * b; }, op, ValueType::I64);
            else if (op == "i64.and") binaryOp([](int64_t a, int64_t b){ return a & b; }, op, ValueType::I64);
            else if (op == "i64.or") binaryOp([](int64_t a, int64_t b){ return a | b; }, op, ValueType::I64);
            else if (op == "i64.xor") binaryOp([](int64_t a, int64_t b){ return a ^ b; }, op, ValueType::I64);
            else if (op == "i64.min") binaryOp([](int64_t a, int64_t b){ return a < b ? a : b; }, op, ValueType::I64);
            else if (op == "i64.max") binaryOp([](int64_t a, int64_t b){ return a > b ? a : b; }, op, ValueType::I64);
            else if (op == "i64.abs") unaryOp([](int64_t a){ return a < 0 ? -a : a; }, op, ValueType::I64);
            else if (op == "i64.neg") unaryOp([](int64_t a){ return -a; }, op, ValueType::I64);
            else if (op == "i64.shl") binaryOp([](int64_t a, int64_t b){ return a << (b & 63); }, op, ValueType::I64);
            else if (op == "i64.shr_s") binaryOp([](int64_t a, int64_t b){ return a >> (b & 63); }, op, ValueType::I64);
            else if (op == "i64.shr_u") binaryOp([](int64_t a, int64_t b){ return static_cast<int64_t>(static_cast<uint64_t>(a) >> (b & 63)); }, op, ValueType::I64);
            else if (op == "i64.rotl") binaryOp([](uint64_t a, uint64_t b){ return (a << (b & 63)) | (a >> ((64 - b) & 63)); }, op, ValueType::I64);
            else if (op == "i64.rotr") binaryOp([](uint64_t a, uint64_t b){ return (a >> (b & 63)) | (a << ((64 - b) & 63)); }, op, ValueType::I64);
            else if (op == "i64.div_s") binaryOp(safeDiv64, op, ValueType::I64);
            else if (op == "i64.div_u") binaryOp([](int64_t a, int64_t b){ return b==0?0:static_cast<int64_t>(static_cast<uint64_t>(a)/static_cast<uint64_t>(b)); }, op, ValueType::I64);
            else if (op == "i64.rem_s") binaryOp(safeRem64, op, ValueType::I64);
            else if (op == "i64.rem_u") binaryOp([](int64_t a, int64_t b){ return b==0?0:static_cast<int64_t>(static_cast<uint64_t>(a)%static_cast<uint64_t>(b)); }, op, ValueType::I64);
            else if (op == "i64.eq") cmpOp([](int64_t a, int64_t b){return a==b;}, op, ValueType::I64);
            else if (op == "i64.ne") cmpOp([](int64_t a, int64_t b){return a!=b;}, op, ValueType::I64);
            else if (op == "i64.lt_s") cmpOp([](int64_t a, int64_t b){return a<b;}, op, ValueType::I64);
            else if (op == "i64.lt_u") cmpOp([](uint64_t a, uint64_t b){return a<b;}, op, ValueType::I64);
            else if (op == "i64.gt_s") cmpOp([](int64_t a, int64_t b){return a>b;}, op, ValueType::I64);
            else if (op == "i64.gt_u") cmpOp([](uint64_t a, uint64_t b){return a>b;}, op, ValueType::I64);
            else if (op == "i64.le_s") cmpOp([](int64_t a, int64_t b){return a<=b;}, op, ValueType::I64);
            else if (op == "i64.le_u") cmpOp([](uint64_t a, uint64_t b){return a<=b;}, op, ValueType::I64);
            else if (op == "i64.ge_s") cmpOp([](int64_t a, int64_t b){return a>=b;}, op, ValueType::I64);
            else if (op == "i64.ge_u") cmpOp([](uint64_t a, uint64_t b){return a>=b;}, op, ValueType::I64);
            else if (op == "i64.eqz") unaryOp([](int64_t a){return a==0?1:0;}, op, ValueType::I64);
            else if (op == "i64.clz") unaryOp([](uint64_t a){return a==0?64:countl_zero64(a);}, op, ValueType::I64);
            else if (op == "i64.ctz") unaryOp([](uint64_t a){return a==0?64:countr_zero64(a);}, op, ValueType::I64);
            else if (op == "i64.popcnt") unaryOp([](uint64_t a){return popcount64(a);}, op, ValueType::I64);
            continue;
        } else if (op.rfind("f32.", 0) == 0) {
            if (op == "f32.add") binaryOp([](float a, float b){ return a + b; }, op, ValueType::F32);
            else if (op == "f32.sub") binaryOp([](float a, float b){ return a - b; }, op, ValueType::F32);
            else if (op == "f32.mul") binaryOp([](float a, float b){ return a * b; }, op, ValueType::F32);
            else if (op == "f32.div") binaryOp([](float a, float b){ return a / b; }, op, ValueType::F32);
            else if (op == "f32.min") binaryOp([](float a, float b){ return a < b ? a : b; }, op, ValueType::F32);
            else if (op == "f32.max") binaryOp([](float a, float b){ return a > b ? a : b; }, op, ValueType::F32);
            else if (op == "f32.abs") unaryOp([](float a){ return a < 0 ? -a : a; }, op, ValueType::F32);
            else if (op == "f32.neg") unaryOp([](float a){ return -a; }, op, ValueType::F32);
            else if (op == "f32.sqrt") unaryOp([](float a){ return std::sqrt(a); }, op, ValueType::F32);
            else if (op == "f32.ceil") unaryOp([](float a){ return std::ceil(a); }, op, ValueType::F32);
            else if (op == "f32.floor") unaryOp([](float a){ return std::floor(a); }, op, ValueType::F32);
            else if (op == "f32.trunc") unaryOp([](float a){ return std::trunc(a); }, op, ValueType::F32);
            else if (op == "f32.nearest") unaryOp([](float a){ return std::nearbyint(a); }, op, ValueType::F32);
            else if (op == "f32.eq") cmpOp([](float a, float b){return a==b;}, op, ValueType::F32);
            else if (op == "f32.ne") cmpOp([](float a, float b){return a!=b;}, op, ValueType::F32);
            else if (op == "f32.lt") cmpOp([](float a, float b){return a<b;}, op, ValueType::F32);
            else if (op == "f32.gt") cmpOp([](float a, float b){return a>b;}, op, ValueType::F32);
            else if (op == "f32.le") cmpOp([](float a, float b){return a<=b;}, op, ValueType::F32);
            else if (op == "f32.ge") cmpOp([](float a, float b){return a>=b;}, op, ValueType::F32);
            continue;
        } else if (op.rfind("f64.", 0) == 0) {
            if (op == "f64.add") binaryOp([](double a, double b){ return a + b; }, op, ValueType::F64);
            else if (op == "f64.sub") binaryOp([](double a, double b){ return a - b; }, op, ValueType::F64);
            else if (op == "f64.mul") binaryOp([](double a, double b){ return a * b; }, op, ValueType::F64);
            else if (op == "f64.div") binaryOp([](double a, double b){ return a / b; }, op, ValueType::F64);
            else if (op == "f64.min") binaryOp([](double a, double b){ return a < b ? a : b; }, op, ValueType::F64);
            else if (op == "f64.max") binaryOp([](double a, double b){ return a > b ? a : b; }, op, ValueType::F64);
            else if (op == "f64.abs") unaryOp([](double a){ return a < 0 ? -a : a; }, op, ValueType::F64);
            else if (op == "f64.neg") unaryOp([](double a){ return -a; }, op, ValueType::F64);
            else if (op == "f64.sqrt") unaryOp([](double a){ return std::sqrt(a); }, op, ValueType::F64);
            else if (op == "f64.ceil") unaryOp([](double a){ return std::ceil(a); }, op, ValueType::F64);
            else if (op == "f64.floor") unaryOp([](double a){ return std::floor(a); }, op, ValueType::F64);
            else if (op == "f64.trunc") unaryOp([](double a){ return std::trunc(a); }, op, ValueType::F64);
            else if (op == "f64.nearest") unaryOp([](double a){ return std::nearbyint(a); }, op, ValueType::F64);
            else if (op == "f64.eq") cmpOp([](double a, double b){return a==b;}, op, ValueType::F64);
            else if (op == "f64.ne") cmpOp([](double a, double b){return a!=b;}, op, ValueType::F64);
            else if (op == "f64.lt") cmpOp([](double a, double b){return a<b;}, op, ValueType::F64);
            else if (op == "f64.gt") cmpOp([](double a, double b){return a>b;}, op, ValueType::F64);
            else if (op == "f64.le") cmpOp([](double a, double b){return a<=b;}, op, ValueType::F64);
            else if (op == "f64.ge") cmpOp([](double a, double b){return a>=b;}, op, ValueType::F64);
            continue;
        } else {
            std::cout << "\033[1;31m[executor:execute]\033[0m Error: Unknown instruction: " << op << "\n";
        }
    }

    std::cout << "\033[1;36m[executor:execute]\033[0m Function completed.\n";
    this->lastStack = stack;

}
