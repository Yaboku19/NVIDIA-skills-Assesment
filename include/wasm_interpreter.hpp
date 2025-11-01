#pragma once
#include <string>
#include <unordered_map>
#include "wasm_stack.hpp"
#include "wasm_executor.hpp"

class WasmInterpreter {
public:
    void loadFile(const std::string& path);
    void run();
private:
    std::string sourceCode;
    WasmStack stack;
    WasmExecutor executor;
    std::unordered_map<std::string, int32_t> globals;

    std::unordered_map<int, std::string> funcIndexToName;   // ID → name
    std::unordered_map<std::string, int> funcNameToIndex;   // Name → ID
    std::unordered_map<std::string, std::string> exports;   // Export name → function name
    std::unordered_map<std::string, std::function<void()>> funcImpl; // Name → executable function

    void executeLine(const std::string& line);
};
