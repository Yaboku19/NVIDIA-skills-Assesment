#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include "wasm_stack.hpp"
#include "wasm_parser.hpp"
#include "struct.h"

class WasmInterpreter {
public:
    void loadFile(const std::string& path);
    void run();
private:
    std::string sourceCode;
    WasmStack stack;
    WasmParser parser;
    bool inFunction = false;
    std::string functionName = "";
    int functionIndex = -1;
    int brakes = 0;
    std::unordered_map<std::string, int32_t> globals;

    std::unordered_map<int, FuncType> funcTypes; // type index → signature
    std::unordered_map<int, FuncDef> functionsByID;
    std::unordered_map<std::string, FuncDef> functionByName;

    void executeLine(const std::string& line);
};
