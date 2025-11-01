#include "wasm_executor.hpp"
#include "wasm_stack.hpp"
#include <iostream>
#include <sstream>

void WasmExecutor::parseModule(WasmStack& stack, std::unordered_map<std::string, int32_t>& globals) {
    stack.clear();
    globals.clear();

    std::cout << "\033[1;32m[executor]\033[0m New module initialized. Stack and globals cleared.\n";
}

void WasmExecutor::parseGlobal(const std::string& line, std::unordered_map<std::string, int32_t>& globals) {
    std::cout << "\033[1;32m[executor]\033[0m Global definition found: " << line << "\n";

    std::istringstream iss(line);
    std::string temp, name, token;
    int32_t value = 0;

    iss >> temp;
    iss >> name;

    while (iss >> token) {
        if (token.find("i32.const") != std::string::npos) {
            iss >> value;
            break;
        }
    }

    globals[name] = value;
    std::cout << "\033[1;32m[executor]\033[0m Global init → " << name << " = " << value << "\n";
}

void WasmExecutor::print_globals(const std::unordered_map<std::string, int32_t>& globals) const {
    std::cout << "\033[1;32m[executor]\033[0m Global variables state:\n";
    if (globals.empty()) {
        std::cout << "  (empty)\n";
        return;
    }

    for (const auto& [name, value] : globals) {
        std::cout << "  " << name << " = " << value << "\n";
    }
}