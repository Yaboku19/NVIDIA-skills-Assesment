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
    std::istringstream iss(trimmed);
    std::string token;
    iss >> token;

    std::cout << "\033[1;34m[interpreter:executeLine]\033[0m Executing: " << token << "\n";
    if (inFunction) {
        std::cout << "\033[1;34m[interpreter:executeLine]\033[0m Current function: "
                  << (functionName.empty() ? "[anon]" : functionName)
                  << " (index " << functionIndex << ")\n";

        std::string codePart = trimmed;
        size_t commentPos = codePart.find(";;");
        if (commentPos != std::string::npos)
            codePart = codePart.substr(0, commentPos);

        for (char c : codePart) {
            if (c == '(') brakes++;
            else if (c == ')') brakes--;
        }

        std::cout << "\033[1;34m[interpreter:executeLine]\033[0m Braces count = " << brakes << "\n";
        bool toRemove = false;
        if (brakes <= 0) {
            inFunction = false;
            toRemove = true;
            brakes = 0;
            std::cout << "\033[1;34m[interpreter:executeLine]\033[0m -------- end function body --------\n";
        }

        if (functionName.empty())
            parser.parseBody(trimmed, &functionsByID[functionIndex], toRemove);
        else
            parser.parseBody(trimmed, &functionByName[functionName], toRemove);
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
        brakes = 1;
    }
}
