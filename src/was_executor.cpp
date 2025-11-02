#include "wasm_executor.hpp"
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <iostream>
#include "struct.h"
#include "wasm_memory.hpp"

void WasmExecutor::execute(const FuncDef& func,
        std::unordered_map<int, FuncDef>& functionsByID,
        std::unordered_map<int, WasmMemory>& memoriesByIndex,
        std::unordered_map<std::string, int32_t>& globals,
        WasmStack& stack
    ) {
    std::cout << "\033[1;34m[executor:execute]\033[0m Executing function '" 
              << func.name << "' (index " << func.index << ").\n";
}