#include "wasm_executor.hpp"
#include <iostream>
#include <sstream>
#include <unordered_map>
#include "struct.h"
#include "wasm_memory.hpp"

void WasmExecutor::execute(
    const FuncDef& func,
    std::unordered_map<int, FuncDef>& functionsByID,
    WasmMemory& memory,
    std::unordered_map<std::string, WasmGlobal>& globals,
    WasmStack& stack
) {
    stack.clear();
    std::unordered_map<std::string, WasmValue> locals;
    std::cout << "\033[1;36m[executor:execute]\033[0m Executing function '" << func.name << "' (index " << func.index << ").\n";

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
        std::cout << "\033[1;36m[" << tag << "]\033[0m ";
        printValue(a);
        std::cout << ", ";
        printValue(b);
        std::cout << " -> ";
        printValue(r);
        std::cout << "\n";
        stack.push(r);
    };

    auto safeDiv32 = [](int32_t a, int32_t b) { return b == 0 ? 0 : a / b; };
    auto safeDiv64 = [](int64_t a, int64_t b) { return b == 0 ? 0 : a / b; };
    auto safeRem32 = [](int32_t a, int32_t b) { return b == 0 ? 0 : a % b; };
    auto safeRem64 = [](int64_t a, int64_t b) { return b == 0 ? 0 : a % b; };

    for (const std::string& line : func.body) {
        std::istringstream iss(line);
        std::string op; iss >> op;
        if (op.empty()) continue;
        std::cout << "\033[1;36m[executor:instr]\033[0m " << op << "\n";

        if (op == "i32.const" || op == "i64.const" || op == "f32.const" || op == "f64.const") {
            std::string v; iss >> v;
            if (op == "i32.const") stack.push(WasmValue(static_cast<int32_t>(std::stoi(v))));
            else if (op == "i64.const") stack.push(WasmValue(static_cast<int64_t>(std::stoll(v))));
            else if (op == "f32.const") stack.push(WasmValue(static_cast<float>(std::stof(v))));
            else stack.push(WasmValue(static_cast<double>(std::stod(v))));
            continue;
        }

        auto doStore = [&](auto fn, const std::string& tag) {
            WasmValue v = stack.pop(), addr = stack.pop();
            fn(addr.i32, v);
            std::cout << "\033[1;36m[" << tag << "]\033[0m mem[" << addr.i32 << "] = ";
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
            std::cout << "\033[1;36m[" << tag << "]\033[0m mem[" << addr.i32 << "] → " << val << "\n";
        };

        if (op.find("store") != std::string::npos) {
            if (op == "i32.store8") doStore([&](int32_t a, WasmValue v){ memory.store8(a, static_cast<uint8_t>(v.i32)); }, op);
            else if (op == "i32.store16") doStore([&](int32_t a, WasmValue v){ memory.store16(a, static_cast<uint16_t>(v.i32)); }, op);
            else if (op == "i32.store") doStore([&](int32_t a, WasmValue v){ memory.store32(a, v.i32); }, op);
            else if (op == "i64.store") doStore([&](int32_t a, WasmValue v){ memory.store64(a, v.i64); }, op);
            else if (op == "f32.store") doStore([&](int32_t a, WasmValue v){ memory.storeF32(a, v.f32); }, op);
            else if (op == "f64.store") doStore([&](int32_t a, WasmValue v){ memory.storeF64(a, v.f64); }, op);
            continue;
        }

        if (op.find("load") != std::string::npos) {
            if (op == "i32.load") doLoad([&](int32_t a){ return memory.load32(a); }, [](auto x){return x;}, op, ValueType::I32);
            else if (op == "i64.load") doLoad([&](int32_t a){ return memory.load64(a); }, [](auto x){return x;}, op, ValueType::I64);
            else if (op == "f32.load") doLoad([&](int32_t a){ return memory.loadF32(a); }, [](auto x){return x;}, op, ValueType::F32);
            else if (op == "f64.load") doLoad([&](int32_t a){ return memory.loadF64(a); }, [](auto x){return x;}, op, ValueType::F64);
            continue;
        }

        if (op == "(local") {
            std::string name, type; iss >> name >> type;
            if (!type.empty() && type.back() == ')') type.pop_back();
            locals[name] = {};
            std::cout << "\033[1;36m[local]\033[0m Declared " << name << " (" << type << ")\n";
            continue;
        }

        if (op == "local.set" || op == "local.get" || op == "local.tee") {
            std::string name; iss >> name;
            if (op == "local.set") locals[name] = stack.pop();
            else if (op == "local.get") stack.push(locals[name]);
            else locals[name] = stack.top();
            continue;
        }

        if (op == "global.get" || op == "global.set") {
            std::string name; iss >> name;
            WasmGlobal& g = globals[name];
            if (op == "global.get") stack.push(g.value);
            else g.value = stack.pop();
            continue;
        }

        if (op.rfind("i32.", 0) == 0) {
            if (op == "i32.add") binaryOp([](int32_t a, int32_t b){ return a + b; }, op, ValueType::I32);
            else if (op == "i32.sub") binaryOp([](int32_t a, int32_t b){ return a - b; }, op, ValueType::I32);
            else if (op == "i32.mul") binaryOp([](int32_t a, int32_t b){ return a * b; }, op, ValueType::I32);
            else if (op == "i32.and") binaryOp([](int32_t a, int32_t b){ return a & b; }, op, ValueType::I32);
            else if (op == "i32.or") binaryOp([](int32_t a, int32_t b){ return a | b; }, op, ValueType::I32);
            else if (op == "i32.xor") binaryOp([](int32_t a, int32_t b){ return a ^ b; }, op, ValueType::I32);
            else if (op == "i32.shl") binaryOp([](int32_t a, int32_t b){ return a << (b & 31); }, op, ValueType::I32);
            else if (op == "i32.shr_s") binaryOp([](int32_t a, int32_t b){ return a >> (b & 31); }, op, ValueType::I32);
            else if (op == "i32.shr_u") binaryOp([](int32_t a, int32_t b){ return static_cast<int32_t>(static_cast<uint32_t>(a) >> (b & 31)); }, op, ValueType::I32);
            else if (op == "i32.div_s") binaryOp(safeDiv32, op, ValueType::I32);
            else if (op == "i32.div_u") binaryOp([](int32_t a, int32_t b){ return b==0?0:static_cast<int32_t>(static_cast<uint32_t>(a)/static_cast<uint32_t>(b)); }, op, ValueType::I32);
            else if (op == "i32.rem_s") binaryOp(safeRem32, op, ValueType::I32);
            else if (op == "i32.rem_u") binaryOp([](int32_t a, int32_t b){ return b==0?0:static_cast<int32_t>(static_cast<uint32_t>(a)%static_cast<uint32_t>(b)); }, op, ValueType::I32);
            continue;
        }

        if (op.rfind("i64.", 0) == 0) {
            if (op == "i64.add") binaryOp([](int64_t a, int64_t b){ return a + b; }, op, ValueType::I64);
            else if (op == "i64.sub") binaryOp([](int64_t a, int64_t b){ return a - b; }, op, ValueType::I64);
            else if (op == "i64.mul") binaryOp([](int64_t a, int64_t b){ return a * b; }, op, ValueType::I64);
            else if (op == "i64.and") binaryOp([](int64_t a, int64_t b){ return a & b; }, op, ValueType::I64);
            else if (op == "i64.or") binaryOp([](int64_t a, int64_t b){ return a | b; }, op, ValueType::I64);
            else if (op == "i64.xor") binaryOp([](int64_t a, int64_t b){ return a ^ b; }, op, ValueType::I64);
            else if (op == "i64.shl") binaryOp([](int64_t a, int64_t b){ return a << (b & 63); }, op, ValueType::I64);
            else if (op == "i64.shr_s") binaryOp([](int64_t a, int64_t b){ return a >> (b & 63); }, op, ValueType::I64);
            else if (op == "i64.shr_u") binaryOp([](int64_t a, int64_t b){ return static_cast<int64_t>(static_cast<uint64_t>(a) >> (b & 63)); }, op, ValueType::I64);
            else if (op == "i64.div_s") binaryOp(safeDiv64, op, ValueType::I64);
            else if (op == "i64.div_u") binaryOp([](int64_t a, int64_t b){ return b==0?0:static_cast<int64_t>(static_cast<uint64_t>(a)/static_cast<uint64_t>(b)); }, op, ValueType::I64);
            else if (op == "i64.rem_s") binaryOp(safeRem64, op, ValueType::I64);
            else if (op == "i64.rem_u") binaryOp([](int64_t a, int64_t b){ return b==0?0:static_cast<int64_t>(static_cast<uint64_t>(a)%static_cast<uint64_t>(b)); }, op, ValueType::I64);
            continue;
        }

        if (op.rfind("f32.", 0) == 0) {
            if (op == "f32.add") binaryOp([](float a, float b){ return a + b; }, op, ValueType::F32);
            else if (op == "f32.sub") binaryOp([](float a, float b){ return a - b; }, op, ValueType::F32);
            else if (op == "f32.mul") binaryOp([](float a, float b){ return a * b; }, op, ValueType::F32);
            else if (op == "f32.div") binaryOp([](float a, float b){ return a / b; }, op, ValueType::F32);
            continue;
        }

        if (op.rfind("f64.", 0) == 0) {
            if (op == "f64.add") binaryOp([](double a, double b){ return a + b; }, op, ValueType::F64);
            else if (op == "f64.sub") binaryOp([](double a, double b){ return a - b; }, op, ValueType::F64);
            else if (op == "f64.mul") binaryOp([](double a, double b){ return a * b; }, op, ValueType::F64);
            else if (op == "f64.div") binaryOp([](double a, double b){ return a / b; }, op, ValueType::F64);
            continue;
        }
    }

    std::cout << "\033[1;36m[executor:execute]\033[0m Function completed.\n";
}
