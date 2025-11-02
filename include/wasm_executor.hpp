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
    WasmMemory& memory,
    std::unordered_map<std::string, WasmGlobal>& globals,
    WasmStack& stack);
private:
};
