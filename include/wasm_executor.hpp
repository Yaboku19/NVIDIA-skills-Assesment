#pragma once
#include "wasm_stack.hpp"
#include <unordered_map>
#include <string>
#include <cstdint>

class WasmExecutor {
public:
    void parseModule(WasmStack& stack, std::unordered_map<std::string, int32_t>& globals);
    void parseGlobal(const std::string& line, std::unordered_map<std::string, int32_t>& globals);
    void print_globals(const std::unordered_map<std::string, int32_t>& globals) const;
};
