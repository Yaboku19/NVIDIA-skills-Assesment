#pragma once
#include "wasm_stack.hpp"
#include <unordered_map>
#include <string>
#include <cstdint>
#include "struct.h"

class WasmParser {
public:
    void parseModule(WasmStack& stack, std::unordered_map<std::string, int32_t>& globals);
    void parseGlobal(const std::string& line, std::unordered_map<std::string, int32_t>& globals);
    void parseType(const std::string& line, std::unordered_map<int, FuncType>& funcTypes);
    FuncDef parseFunction(const std::string& line, std::unordered_map<int, FuncDef>& functionsByID,
                       std::unordered_map<std::string, FuncDef>& functionByName,
                       const std::unordered_map<int, FuncType>& funcTypes);
    void parseBody(const std::string& line, FuncDef *func, bool toRemove);
    void print_globals(const std::unordered_map<std::string, int32_t>& globals) const;
    void print_functions(const std::unordered_map<std::string, FuncDef>& functionByName, const std::unordered_map<int, FuncDef>& functionsByID) const;
};
