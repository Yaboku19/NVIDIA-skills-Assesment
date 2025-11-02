#include "wasm_executor.hpp"
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <iostream>
#include "struct.h"
#include "wasm_memory.hpp"
#include <sstream>


void WasmExecutor::execute(
    const FuncDef& func,
    std::unordered_map<int, FuncDef>& functionsByID,
    WasmMemory& memory,
    std::unordered_map<std::string, WasmGlobal>& globals,
    WasmStack& stack
) {
    stack.clear();
    std::unordered_map<std::string, WasmValue> locals;
    std::cout << "\033[1;36m[executor:execute]\033[0m Executing function '" 
              << func.name << "' (index " << func.index << ").\n";

    for (const std::string& line : func.body) {
        std::istringstream iss(line);
        std::string op;
        iss >> op;

        if (op.empty()) continue;

        std::cout << "\033[1;36m[executor:instr]\033[0m " << op << "\n";

        if (op == "i32.const") {
            std::cout << "\033[1;36m[executor:i32.const]\033[0m\n";
            std::string val; iss >> val;
            stack.push(WasmValue(static_cast<int32_t>(std::stoi(val))));
        } else if (op == "i64.const") {
            std::cout << "\033[1;36m[executor:i64.const]\033[0m \n";
            std::string val; iss >> val;
            stack.push(WasmValue(static_cast<int64_t>(std::stoll(val))));
        } else if (op == "f32.const") {
            std::cout << "\033[1;36m[executor:f32.const]\033[0m\n";
            std::string val; iss >> val;
            stack.push(WasmValue(static_cast<float>(std::stof(val))));
        } else if (op == "f64.const") {
            std::cout << "\033[1;36m[executor:f64.const]\033[0m\n";
            std::string val; iss >> val;
            stack.push(WasmValue(static_cast<double>(std::stod(val))));
        } else if (op == "i32.store8") {
            WasmValue value = stack.pop();
            WasmValue addr  = stack.pop();
            std::cout << "\033[1;36m[executor:i32.store8]\033[0m mem[" << addr.i32 
                      << "] = (u8)" << (value.i32 & 0xFF) << "\n";
            memory.store8(addr.i32, static_cast<uint8_t>(value.i32 & 0xFF));
        } else if (op == "i32.store16") {
            WasmValue value = stack.pop();
            WasmValue addr  = stack.pop();
            std::cout << "\033[1;36m[executor:i32.store16]\033[0m mem[" << addr.i32 
                      << "] = (u16)" << (value.i32 & 0xFFFF) << "\n";
            memory.store16(addr.i32, static_cast<uint16_t>(value.i32 & 0xFFFF));
        } else if (op == "i32.store") {
            WasmValue value = stack.pop();
            WasmValue addr  = stack.pop();
            std::cout << "\033[1;36m[executor:i32.store]\033[0m mem[" << addr.i32 
                      << "] = " << value.i32 << "\n";
            memory.store32(addr.i32, value.i32);
        } else if (op == "i64.store8") {
            WasmValue value = stack.pop();
            WasmValue addr  = stack.pop();
            std::cout << "\033[1;36m[executor:i64.store8]\033[0m mem[" << addr.i32 
                      << "] = (u8)" << (value.i64 & 0xFF) << "\n";
            memory.store8(addr.i32, static_cast<uint8_t>(value.i64 & 0xFF));
        } else if (op == "i64.store16") {
            WasmValue value = stack.pop();
            WasmValue addr  = stack.pop();
            std::cout << "\033[1;36m[executor:i64.store16]\033[0m mem[" << addr.i32 
                      << "] = (u16)" << (value.i64 & 0xFFFF) << "\n";
            memory.store16(addr.i32, static_cast<uint16_t>(value.i64 & 0xFFFF));
        } else if (op == "i64.store32") {
            WasmValue value = stack.pop();
            WasmValue addr  = stack.pop();
            std::cout << "\033[1;36m[executor:i64.store32]\033[0m mem[" << addr.i32 
                      << "] = (u32)" << static_cast<uint32_t>(value.i64) << "\n";
            memory.store32(addr.i32, static_cast<int32_t>(static_cast<uint32_t>(value.i64)));
        } else if (op == "i64.store") {
            WasmValue value = stack.pop();
            WasmValue addr  = stack.pop();
            std::cout << "\033[1;36m[executor:i64.store]\033[0m mem[" << addr.i32 
                      << "] = " << value.i64 << "\n";
            memory.store64(addr.i32, value.i64);
        } else if (op == "f32.store") {
            WasmValue value = stack.pop();
            WasmValue addr  = stack.pop();
            std::cout << "\033[1;36m[executor:f32.store]\033[0m mem[" << addr.i32 
                      << "] = " << value.f32 << "\n";
            memory.storeF32(addr.i32, value.f32);
        } else if (op == "f64.store") {
            WasmValue value = stack.pop();
            WasmValue addr  = stack.pop();
            std::cout << "\033[1;36m[executor:f64.store]\033[0m mem[" << addr.i32 
                      << "] = " << value.f64 << "\n";
            memory.storeF64(addr.i32, value.f64);
        } else if (op == "i32.load8_s") {
            WasmValue addr = stack.pop();
            int8_t value = static_cast<int8_t>(memory.load8(addr.i32));
            std::cout << "\033[1;36m[executor:i32.load8_s]\033[0m mem[" << addr.i32
                    << "] → (i32)" << static_cast<int32_t>(value) << "\n";
            stack.push(WasmValue(static_cast<int32_t>(value)));
        } else if (op == "i32.load8_u") {
            WasmValue addr = stack.pop();
            uint8_t value = memory.load8(addr.i32);
            std::cout << "\033[1;36m[executor:i32.load8_u]\033[0m mem[" << addr.i32
                    << "] → (u32)" << static_cast<uint32_t>(value) << "\n";
            stack.push(WasmValue(static_cast<int32_t>(value)));
        } else if (op == "i32.load16_s") {
            WasmValue addr = stack.pop();
            int16_t value = static_cast<int16_t>(memory.load16(addr.i32));
            std::cout << "\033[1;36m[executor:i32.load16_s]\033[0m mem[" << addr.i32
                    << "] → (i32)" << static_cast<int32_t>(value) << "\n";
            stack.push(WasmValue(static_cast<int32_t>(value)));
        } else if (op == "i32.load16_u") {
            WasmValue addr = stack.pop();
            uint16_t value = memory.load16(addr.i32);
            std::cout << "\033[1;36m[executor:i32.load16_u]\033[0m mem[" << addr.i32
                    << "] → (u32)" << static_cast<uint32_t>(value) << "\n";
            stack.push(WasmValue(static_cast<int32_t>(value)));
        } else if (op == "i32.load") {
            WasmValue addr = stack.pop();
            int32_t value = memory.load32(addr.i32);
            std::cout << "\033[1;36m[executor:i32.load]\033[0m mem[" << addr.i32
                    << "] → " << value << "\n";
            stack.push(WasmValue(value));
        } else if (op == "i64.load8_s") {
            WasmValue addr = stack.pop();
            int8_t value = static_cast<int8_t>(memory.load8(addr.i32));
            std::cout << "\033[1;36m[executor:i64.load8_s]\033[0m mem[" << addr.i32
                    << "] → (i64)" << static_cast<int64_t>(value) << "\n";
            stack.push(WasmValue(static_cast<int64_t>(value)));
        } else if (op == "i64.load8_u") {
            WasmValue addr = stack.pop();
            uint8_t value = memory.load8(addr.i32);
            std::cout << "\033[1;36m[executor:i64.load8_u]\033[0m mem[" << addr.i32
                    << "] → (u64)" << static_cast<uint64_t>(value) << "\n";
            stack.push(WasmValue(static_cast<int64_t>(value)));
        } else if (op == "i64.load16_s") {
            WasmValue addr = stack.pop();
            int16_t value = static_cast<int16_t>(memory.load16(addr.i32));
            std::cout << "\033[1;36m[executor:i64.load16_s]\033[0m mem[" << addr.i32
                    << "] → (i64)" << static_cast<int64_t>(value) << "\n";
            stack.push(WasmValue(static_cast<int64_t>(value)));
        } else if (op == "i64.load16_u") {
            WasmValue addr = stack.pop();
            uint16_t value = memory.load16(addr.i32);
            std::cout << "\033[1;36m[executor:i64.load16_u]\033[0m mem[" << addr.i32
                    << "] → (u64)" << static_cast<uint64_t>(value) << "\n";
            stack.push(WasmValue(static_cast<int64_t>(value)));
        } else if (op == "i64.load32_s") {
            WasmValue addr = stack.pop();
            int32_t value = memory.load32(addr.i32);
            std::cout << "\033[1;36m[executor:i64.load32_s]\033[0m mem[" << addr.i32
                    << "] → (i64)" << static_cast<int64_t>(value) << "\n";
            stack.push(WasmValue(static_cast<int64_t>(value)));
        } else if (op == "i64.load32_u") {
            WasmValue addr = stack.pop();
            uint32_t value = static_cast<uint32_t>(memory.load32(addr.i32));
            std::cout << "\033[1;36m[executor:i64.load32_u]\033[0m mem[" << addr.i32
                    << "] → (u64)" << static_cast<uint64_t>(value) << "\n";
            stack.push(WasmValue(static_cast<int64_t>(value)));
        } else if (op == "i64.load") {
            WasmValue addr = stack.pop();
            int64_t value = memory.load64(addr.i32);
            std::cout << "\033[1;36m[executor:i64.load]\033[0m mem[" << addr.i32
                    << "] → " << value << "\n";
            stack.push(WasmValue(value));
        } else if (op == "f32.load") {
            WasmValue addr = stack.pop();
            float value = memory.loadF32(addr.i32);
            std::cout << "\033[1;36m[executor:f32.load]\033[0m mem[" << addr.i32
                    << "] → " << value << "\n";
            stack.push(WasmValue(value));
        } else if (op == "f64.load") {
            WasmValue addr = stack.pop();
            double value = memory.loadF64(addr.i32);
            std::cout << "\033[1;36m[executor:f64.load]\033[0m mem[" << addr.i32
                    << "] → " << value << "\n";
            stack.push(WasmValue(value));
        } else if (op == "(local") {
            std::string name, typeStr;
            iss >> name >> typeStr;

            // rimuovi eventuale parentesi finale ")"
            if (!typeStr.empty() && typeStr.back() == ')')
                typeStr.pop_back();

            ValueType t;
            if (typeStr == "i32") t = ValueType::I32;
            else if (typeStr == "i64") t = ValueType::I64;
            else if (typeStr == "f32") t = ValueType::F32;
            else if (typeStr == "f64") t = ValueType::F64;
            else {
                std::cerr << "\033[1;31m[executor:local]\033[0m Unknown type '" << typeStr << "'\n";
                continue;
            }

            locals[name] = {};
            std::cout << "\033[1;36m[executor:local]\033[0m Declared local "
                    << name << " of type " << typeStr << " (uninitialized)\n";
        } else if (op == "local.set") {
            std::string name;
            iss >> name;
            WasmValue value = stack.pop();

            if (locals.find(name) == locals.end()) {
                std::cerr << "\033[1;31m[executor:local.set]\033[0m Error: local " 
                        << name << " not declared.\n";
                continue;
            }

            locals[name] = value;
            std::cout << "\033[1;36m[executor:local.set]\033[0m " 
                    << name << " = ";

            switch (value.type) {
                case ValueType::I32: std::cout << value.i32; break;
                case ValueType::I64: std::cout << value.i64; break;
                case ValueType::F32: std::cout << value.f32; break;
                case ValueType::F64: std::cout << value.f64; break;
            }
            std::cout << "\n";
        } else if (op == "local.get") {
            std::string name;
            iss >> name;

            if (locals.find(name) == locals.end()) {
                std::cerr << "\033[1;31m[executor:local.get]\033[0m Error: local "
                        << name << " not declared.\n";
                stack.push(WasmValue(int32_t(0)));
                continue;
            }

            WasmValue value = locals[name];
            std::cout << "\033[1;36m[executor:local.get]\033[0m push " << name << " = ";
            switch (value.type) {
                case ValueType::I32: std::cout << value.i32; break;
                case ValueType::I64: std::cout << value.i64; break;
                case ValueType::F32: std::cout << value.f32; break;
                case ValueType::F64: std::cout << value.f64; break;
            }
            std::cout << "\n";

            stack.push(value);
        } else if (op == "local.tee") {
            std::string name;
            iss >> name;

            if (locals.find(name) == locals.end()) {
                std::cerr << "\033[1;31m[executor:local.tee]\033[0m Error: local "
                        << name << " not declared.\n";
                continue;
            }

            WasmValue value = stack.top(); // non pop, tee lo lascia sullo stack
            locals[name] = value;
            std::cout << "\033[1;36m[executor:local.tee]\033[0m " 
                    << name << " = (value remains on stack)\n";
        } else if (op == "i32.add") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            int32_t result = a.i32 + b.i32;
            std::cout << "\033[1;36m[executor:i32.add]\033[0m " 
                      << a.i32 << " + " << b.i32 << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "i64.add") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            int64_t result = a.i64 + b.i64;
            std::cout << "\033[1;36m[executor:i64.add]\033[0m " 
                      << a.i64 << " + " << b.i64 << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "f32.add") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            float result = a.f32 + b.f32;
            std::cout << "\033[1;36m[executor:f32.add]\033[0m " 
                      << a.f32 << " + " << b.f32 << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "f64.add") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            double result = a.f64 + b.f64;
            std::cout << "\033[1;36m[executor:f64.add]\033[0m " 
                      << a.f64 << " + " << b.f64 << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "i32.sub") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            int32_t result = a.i32 - b.i32;
            std::cout << "\033[1;36m[executor:i32.sub]\033[0m " 
                    << a.i32 << " - " << b.i32 << " = " << result << "\n";
            stack.push(WasmValue(result));
        }
        else if (op == "i64.sub") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            int64_t result = a.i64 - b.i64;
            std::cout << "\033[1;36m[executor:i64.sub]\033[0m " 
                    << a.i64 << " - " << b.i64 << " = " << result << "\n";
            stack.push(WasmValue(result));
        }
        else if (op == "f32.sub") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            float result = a.f32 - b.f32;
            std::cout << "\033[1;36m[executor:f32.sub]\033[0m " 
                    << a.f32 << " - " << b.f32 << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "f64.sub") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            double result = a.f64 - b.f64;
            std::cout << "\033[1;36m[executor:f64.sub]\033[0m " 
                    << a.f64 << " - " << b.f64 << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "i32.mul") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            int32_t result = a.i32 * b.i32;
            std::cout << "\033[1;36m[executor:i32.mul]\033[0m " 
                    << a.i32 << " * " << b.i32 << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "i64.mul") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            int64_t result = a.i64 * b.i64;
            std::cout << "\033[1;36m[executor:i64.mul]\033[0m " 
                    << a.i64 << " * " << b.i64 << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "f32.mul") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            float result = a.f32 * b.f32;
            std::cout << "\033[1;36m[executor:f32.mul]\033[0m " 
                    << a.f32 << " * " << b.f32 << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "f64.mul") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            double result = a.f64 * b.f64;
            std::cout << "\033[1;36m[executor:f64.mul]\033[0m " 
                    << a.f64 << " * " << b.f64 << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "i32.div_s") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            if (b.i32 == 0) {
                std::cout << "\033[1;31m[executor:i32.div_s]\033[0m Division by zero\n";
                stack.push(WasmValue(0));
            } else {
                int32_t result = a.i32 / b.i32;
                std::cout << "\033[1;36m[executor:i32.div_s]\033[0m " 
                        << a.i32 << " / " << b.i32 << " = " << result << "\n";
                stack.push(WasmValue(result));
            }
        } else if (op == "i32.div_u") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            if (b.i32 == 0) {
                std::cout << "\033[1;31m[executor:i32.div_u]\033[0m Division by zero\n";
                stack.push(WasmValue(0));
            } else {
                uint32_t result = static_cast<uint32_t>(a.i32) / static_cast<uint32_t>(b.i32);
                std::cout << "\033[1;36m[executor:i32.div_u]\033[0m " 
                        << static_cast<uint32_t>(a.i32) << " / " 
                        << static_cast<uint32_t>(b.i32) << " = " << result << "\n";
                stack.push(WasmValue(static_cast<int32_t>(result)));
            }
        } else if (op == "i64.div_s") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            if (b.i64 == 0) {
                std::cout << "\033[1;31m[executor:i64.div_s]\033[0m Division by zero\n";
                stack.push(WasmValue(int64_t(0)));
            } else {
                int64_t result = a.i64 / b.i64;
                std::cout << "\033[1;36m[executor:i64.div_s]\033[0m " 
                        << a.i64 << " / " << b.i64 << " = " << result << "\n";
                stack.push(WasmValue(result));
            }
        } else if (op == "i64.div_u") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            if (b.i64 == 0) {
                std::cout << "\033[1;31m[executor:i64.div_u]\033[0m Division by zero\n";
                stack.push(WasmValue(int64_t(0)));
            } else {
                uint64_t result = static_cast<uint64_t>(a.i64) / static_cast<uint64_t>(b.i64);
                std::cout << "\033[1;36m[executor:i64.div_u]\033[0m " 
                        << static_cast<uint64_t>(a.i64) << " / " 
                        << static_cast<uint64_t>(b.i64) << " = " << result << "\n";
                stack.push(WasmValue(static_cast<int64_t>(result)));
            }
        } else if (op == "f32.div") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            float result = a.f32 / b.f32;
            std::cout << "\033[1;36m[executor:f32.div]\033[0m " 
                    << a.f32 << " / " << b.f32 << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "f64.div") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            double result = a.f64 / b.f64;
            std::cout << "\033[1;36m[executor:f64.div]\033[0m " 
                    << a.f64 << " / " << b.f64 << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "i32.rem_s") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            if (b.i32 == 0) {
                std::cout << "\033[1;31m[executor:i32.rem_s]\033[0m Division by zero\n";
                stack.push(WasmValue(0));
            } else {
                int32_t result = a.i32 % b.i32;
                std::cout << "\033[1;36m[executor:i32.rem_s]\033[0m "
                        << a.i32 << " % " << b.i32 << " = " << result << "\n";
                stack.push(WasmValue(result));
            }
        } else if (op == "i32.rem_u") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            if (b.i32 == 0) {
                std::cout << "\033[1;31m[executor:i32.rem_u]\033[0m Division by zero\n";
                stack.push(WasmValue(0));
            } else {
                uint32_t result = static_cast<uint32_t>(a.i32) % static_cast<uint32_t>(b.i32);
                std::cout << "\033[1;36m[executor:i32.rem_u]\033[0m "
                        << static_cast<uint32_t>(a.i32) << " % "
                        << static_cast<uint32_t>(b.i32) << " = " << result << "\n";
                stack.push(WasmValue(static_cast<int32_t>(result)));
            }
        } else if (op == "i64.rem_s") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            if (b.i64 == 0) {
                std::cout << "\033[1;31m[executor:i64.rem_s]\033[0m Division by zero\n";
                stack.push(WasmValue(int64_t(0)));
            } else {
                int64_t result = a.i64 % b.i64;
                std::cout << "\033[1;36m[executor:i64.rem_s]\033[0m "
                        << a.i64 << " % " << b.i64 << " = " << result << "\n";
                stack.push(WasmValue(result));
            }
        } else if (op == "i64.rem_u") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            if (b.i64 == 0) {
                std::cout << "\033[1;31m[executor:i64.rem_u]\033[0m Division by zero\n";
                stack.push(WasmValue(int64_t(0)));
            } else {
                uint64_t result = static_cast<uint64_t>(a.i64) % static_cast<uint64_t>(b.i64);
                std::cout << "\033[1;36m[executor:i64.rem_u]\033[0m "
                        << static_cast<uint64_t>(a.i64) << " % "
                        << static_cast<uint64_t>(b.i64) << " = " << result << "\n";
                stack.push(WasmValue(static_cast<int64_t>(result)));
            }
        } else if (op == "i32.and") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            int32_t result = a.i32 & b.i32;
            std::cout << "\033[1;36m[executor:i32.and]\033[0m "
                    << a.i32 << " & " << b.i32 << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "i64.and") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            int64_t result = a.i64 & b.i64;
            std::cout << "\033[1;36m[executor:i64.and]\033[0m "
                    << a.i64 << " & " << b.i64 << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "i32.or") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            int32_t result = a.i32 | b.i32;
            std::cout << "\033[1;36m[executor:i32.or]\033[0m "
                    << a.i32 << " | " << b.i32 << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "i64.or") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            int64_t result = a.i64 | b.i64;
            std::cout << "\033[1;36m[executor:i64.or]\033[0m "
                    << a.i64 << " | " << b.i64 << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "i32.xor") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            int32_t result = a.i32 ^ b.i32;
            std::cout << "\033[1;36m[executor:i32.xor]\033[0m "
                    << a.i32 << " ^ " << b.i32 << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "i64.xor") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            int64_t result = a.i64 ^ b.i64;
            std::cout << "\033[1;36m[executor:i64.xor]\033[0m "
                    << a.i64 << " ^ " << b.i64 << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "i32.shl") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            int32_t result = a.i32 << (b.i32 & 31);
            std::cout << "\033[1;36m[executor:i32.shl]\033[0m "
                    << a.i32 << " << " << (b.i32 & 31) << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "i64.shl") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            int64_t result = a.i64 << (b.i64 & 63);
            std::cout << "\033[1;36m[executor:i64.shl]\033[0m "
                    << a.i64 << " << " << (b.i64 & 63) << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "i32.shr_s") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            int32_t result = a.i32 >> (b.i32 & 31);
            std::cout << "\033[1;36m[executor:i32.shr_s]\033[0m "
                    << a.i32 << " >>s " << (b.i32 & 31) << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "i32.shr_u") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            uint32_t result = static_cast<uint32_t>(a.i32) >> (b.i32 & 31);
            std::cout << "\033[1;36m[executor:i32.shr_u]\033[0m "
                    << static_cast<uint32_t>(a.i32) << " >>u "
                    << (b.i32 & 31) << " = " << result << "\n";
            stack.push(WasmValue(static_cast<int32_t>(result)));
        } else if (op == "i64.shr_s") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            int64_t result = a.i64 >> (b.i64 & 63);
            std::cout << "\033[1;36m[executor:i64.shr_s]\033[0m "
                    << a.i64 << " >>s " << (b.i64 & 63) << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else if (op == "i64.shr_u") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            uint64_t result = static_cast<uint64_t>(a.i64) >> (b.i64 & 63);
            std::cout << "\033[1;36m[executor:i64.shr_u]\033[0m "
                    << static_cast<uint64_t>(a.i64) << " >>u "
                    << (b.i64 & 63) << " = " << result << "\n";
            stack.push(WasmValue(static_cast<int64_t>(result)));
        } else {
                std::cout << "\033[38;5;208m[warn]\033[0m Unrecognized opcode: " << op << "\n";
        }
    }

    std::cout << "\033[1;36m[executor:execute]\033[0m Function completed.\n";
}
