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

        std::cout << "\033[1;36m[instr]\033[0m " << op << "\n";

        if (op == "i32.const") {
            std::string val; iss >> val;
            stack.push(WasmValue(static_cast<int32_t>(std::stoi(val))));
        } else if (op == "i64.const") {
            std::string val; iss >> val;
            stack.push(WasmValue(static_cast<int64_t>(std::stoll(val))));
        } else if (op == "f32.const") {
            std::string val; iss >> val;
            stack.push(WasmValue(static_cast<float>(std::stof(val))));
        } else if (op == "f64.const") {
            std::string val; iss >> val;
            stack.push(WasmValue(static_cast<double>(std::stod(val))));
        } else if (op == "i32.store8") {
            WasmValue value = stack.pop();   // i32
            WasmValue addr  = stack.pop();   // i32 (indirizzo)
            std::cout << "  \033[1;34m[mem]\033[0m i32.store8  → mem[" << addr.i32 << "] = (u8)" << (value.i32 & 0xFF) << "\n";
            memory.store8(addr.i32, static_cast<uint8_t>(value.i32 & 0xFF));
        }
        else if (op == "i32.store16") {
            WasmValue value = stack.pop();   // i32
            WasmValue addr  = stack.pop();   // i32
            std::cout << "  \033[1;34m[mem]\033[0m i32.store16 → mem[" << addr.i32 << "] = (u16)" << (value.i32 & 0xFFFF) << "\n";
            memory.store16(addr.i32, static_cast<uint16_t>(value.i32 & 0xFFFF));
        }
        else if (op == "i32.store") {
            WasmValue value = stack.pop();   // i32
            WasmValue addr  = stack.pop();   // i32
            std::cout << "  \033[1;34m[mem]\033[0m i32.store   → mem[" << addr.i32 << "] = " << value.i32 << "\n";
            memory.store32(addr.i32, value.i32);
        }

        else if (op == "i64.store8") {
            WasmValue value = stack.pop();   // i64
            WasmValue addr  = stack.pop();   // i32
            std::cout << "  \033[1;34m[mem]\033[0m i64.store8  → mem[" << addr.i32 << "] = (u8)" << (value.i64 & 0xFF) << "\n";
            memory.store8(addr.i32, static_cast<uint8_t>(value.i64 & 0xFF));
        }
        else if (op == "i64.store16") {
            WasmValue value = stack.pop();   // i64
            WasmValue addr  = stack.pop();   // i32
            std::cout << "  \033[1;34m[mem]\033[0m i64.store16 → mem[" << addr.i32 << "] = (u16)" << (value.i64 & 0xFFFF) << "\n";
            memory.store16(addr.i32, static_cast<uint16_t>(value.i64 & 0xFFFF));
        }
        else if (op == "i64.store32") {
            WasmValue value = stack.pop();   // i64
            WasmValue addr  = stack.pop();   // i32
            std::cout << "  \033[1;34m[mem]\033[0m i64.store32 → mem[" << addr.i32 << "] = (u32)" << (static_cast<uint32_t>(value.i64)) << "\n";
            memory.store32(addr.i32, static_cast<int32_t>(static_cast<uint32_t>(value.i64)));
        }
        else if (op == "i64.store") {
            WasmValue value = stack.pop();   // i64
            WasmValue addr  = stack.pop();   // i32
            std::cout << "  \033[1;34m[mem]\033[0m i64.store   → mem[" << addr.i32 << "] = " << value.i64 << "\n";
            memory.store64(addr.i32, value.i64);
        }

        else if (op == "f32.store") {
            WasmValue value = stack.pop();   // f32
            WasmValue addr  = stack.pop();   // i32
            std::cout << "  \033[1;34m[mem]\033[0m f32.store   → mem[" << addr.i32 << "] = " << value.f32 << "\n";
            memory.storeF32(addr.i32, value.f32);
        }
        else if (op == "f64.store") {
            WasmValue value = stack.pop();   // f64
            WasmValue addr  = stack.pop();   // i32
            std::cout << "  \033[1;34m[mem]\033[0m f64.store   → mem[" << addr.i32 << "] = " << value.f64 << "\n";
            memory.storeF64(addr.i32, value.f64);
        } else {
            std::cout << "\033[38;5;208m[warn]\033[0m Unrecognized opcode: " << op << "\n";
        }
    }

    std::cout << "\033[1;36m[executor:execute]\033[0m Function completed.\n";
}
