#include "wasm_interpreter.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <thread>
#include <chrono>

void WasmInterpreter::loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("\033[1;31m[interpreter:loadFile]\033[0m Cannot open file: " + path);

    sourceCode.assign(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
}

void WasmInterpreter::run() {
    std::cout << "\033[1;34m[interpreter:run]\033[0m Executing simplified WebAssembly:\n";

    std::istringstream stream(sourceCode);
    std::string line;

    while (std::getline(stream, line)) {
        std::cout << "\033[1;34m[interpreter:run]\033[0m Executing line: " << line << "\n";
        executeLine(line);
    }

    // if (!stack.empty()) {
    //     std::cout << "Final stack state: ";
    //     for (auto v : stack) std::cout << v << " ";
    //     std::cout << "\n";
    // } else {
    //     std::cout << "Stack is empty.\n";
    // }
}

void WasmInterpreter::executeLine(const std::string& line) {
    std::string trimmed = line;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));

    if (trimmed.empty() || trimmed.rfind(";;", 0) == 0) {
        std::cout << "\033[1;34m[interpreter:executeLine]\033[0m skipped\n";
        return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::istringstream iss(trimmed);
    std::string token;
    iss >> token;

    std::cout << "\033[1;34m[interpreter:executeLine]\033[0m Executing: " << token << "\n";
    if (inFunction) {
        std::cout << "\033[1;34m[interpreter:executeLine]\033[0m Current function: "
                  << (functionName.empty() ? "[anon]" : functionName)
                  << " (index " << functionIndex << ")\n";
        if (functionName.empty()) {
            parser.parseBody(trimmed, &functionsByID[functionIndex]);
        } else {
            parser.parseBody(trimmed, &functionByName[functionName]);
        }
        
        //std::cout << "\033[1;34m[interpreter]\033[0m ---------function body " << functions.size()-1 << "\n";
        //parser.parseBody(trimmed, functions[functions.size()-1]);
        if (trimmed.size() && trimmed.back() == ')') {
            inFunction = false;
            // std::cout << "\033[1;34m[interpreter]\033[0m --------- end function body "
            //         << functions.size() - 1 << "\n";
        }
    } else if (token.find("module") != std::string::npos) {
        parser.parseModule(stack, globals);
    } else if (token.find("global") != std::string::npos) {
        parser.parseGlobal(trimmed, globals);
        parser.print_globals(globals);
    } else if (token.find("type") != std::string::npos) {
        parser.parseType(trimmed, funcTypes);
    } else if (token.find("func") != std::string::npos) {
        FuncDef fun = parser.parseFunction(trimmed, functionsByID, functionByName, funcTypes);
        functionName = fun.name;
        functionIndex = fun.index;
        parser.print_functions(functionByName, functionsByID);
        inFunction = true;
    }
}
