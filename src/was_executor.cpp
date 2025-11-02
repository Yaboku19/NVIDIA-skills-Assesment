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
    std::unordered_map<std::string, int32_t>& globals,
    WasmStack& stack
) {
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
        }
        else if (op == "f64.sub") {
            WasmValue b = stack.pop();
            WasmValue a = stack.pop();
            double result = a.f64 - b.f64;
            std::cout << "\033[1;36m[executor:f64.sub]\033[0m " 
                    << a.f64 << " - " << b.f64 << " = " << result << "\n";
            stack.push(WasmValue(result));
        } else {
            std::cout << "\033[38;5;208m[warn]\033[0m Unrecognized opcode: " << op << "\n";
        }
    }

    std::cout << "\033[1;36m[executor:execute]\033[0m Function completed.\n";
}
