#pragma once
#include "wasm_memory.hpp"
#include <unordered_map>
#include <string>
#include <cstdint>
#include "struct.h"

class WasmParser {
public:
    void parseModule( std::unordered_map<std::string, WasmGlobal>& globals);
    void parseGlobal(const std::string& line, std::unordered_map<std::string, WasmGlobal>& globals);
    void parseType(const std::string& line, std::unordered_map<int, FuncType>& funcTypes);
    FuncDef parseFunction(const std::string& line, std::unordered_map<int, FuncDef>& functionsByID,
                       std::unordered_map<std::string, FuncDef>& functionByName,
                       const std::unordered_map<int, FuncType>& funcTypes);
    void parseBody(const std::string& line, FuncDef *func, bool toRemove);
    void parseMemory(const std::string& line, WasmMemory& memory);
    void parseExport(const std::string& line, std::unordered_map<std::string, WasmExport>& exports);
    
    void print_exports(const std::unordered_map<std::string, WasmExport>& exports) const;
    void print_globals(const std::unordered_map<std::string, WasmGlobal>& globals) const;
    void print_functions(const std::unordered_map<std::string, FuncDef>& functionByName, const std::unordered_map<int, FuncDef>& functionsByID) const;
};
