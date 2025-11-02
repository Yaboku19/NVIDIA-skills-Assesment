#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include "wasm_stack.hpp"
#include "wasm_parser.hpp"
#include "wasm_memory.hpp"
#include "wasm_executor.hpp"
#include "struct.h"

class WasmInterpreter {
public:
    void loadFile(const std::string& path);
    void parse();
    void callFunctionByExportName(const std::string& exportName);
    void showMemory(uint32_t start, uint32_t count);
private:
    std::string sourceCode;
    WasmStack stack;
    WasmParser parser;
    WasmExecutor executor;
    bool inFunction = false;
    std::string functionName = "";
    int functionIndex = -1;
    int brakes = 0;
    std::unordered_map<std::string, int32_t> globals;

    std::unordered_map<int, FuncType> funcTypes; // type index → signature
    std::unordered_map<int, FuncDef> functionsByID;
    std::unordered_map<std::string, FuncDef> functionByName;
    WasmMemory memory{1};
    std::unordered_map<std::string, WasmExport> exports;

    void executeLine(const std::string& line);
};
