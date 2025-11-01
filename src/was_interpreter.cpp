#include "wasm_interpreter.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

void WasmInterpreter::loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) throw std::runtime_error("Cannot open file: " + path);
    sourceCode.assign((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}

void WasmInterpreter::run() {
    std::cout << "Running WebAssembly code:\n";
    std::cout << sourceCode << "\n";
}
