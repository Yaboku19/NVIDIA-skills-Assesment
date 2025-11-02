#include "wasm_interpreter.hpp"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: wasm_interpreter <file.wat>\n";
        return 1;
    }

    std::string filename = argv[1];
    try {
        WasmInterpreter interpreter;
        interpreter.loadFile(filename);
        interpreter.parse();
        interpreter.callFunctionByExportName("_start");
        interpreter.showMemory(0, 128);
        interpreter.callFunctionByExportName("_test_addition");
        interpreter.showMemory(0, 128);
        interpreter.callFunctionByExportName("_test_subtraction");
        interpreter.showMemory(0, 128);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
