#pragma once
#include <vector>
#include <cstdint>
#include <unordered_map>
#include "struct.h"
#include "wasm_stack.hpp"
#include "wasm_memory.hpp"

class WasmExecutor {
public:
    void execute(const FuncDef& func,
    std::unordered_map<int, FuncDef>& functionsByID,
    std::unordered_map<int, WasmMemory>& memoriesByIndex,
    std::unordered_map<std::string, int32_t>& globals,
    WasmStack& stack);
private:
};
