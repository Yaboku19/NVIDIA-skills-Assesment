#include "wasm_interpreter.hpp"
#include <iostream>
#include <climits>
#include <algorithm>
#include <vector>
#include <string>
#include "struct.h"

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
        std::vector<std::pair<std::string, WasmExport>> funcExports;

        for (const auto& [exportName, exp] : interpreter.getExports()) {
            if (exp.kind == "func") {
                funcExports.emplace_back(exportName, exp);
            }
        }

        std::sort(funcExports.begin(), funcExports.end(),
            [](const auto& a, const auto& b) {
                int indexA = (a.second.index >= 0 ? a.second.index : INT_MIN);
                int indexB = (b.second.index >= 0 ? b.second.index : INT_MIN);
                return indexA < indexB;
            });

        for (const auto& [exportName, exp] : funcExports) {
            std::cout << "\033[1;36m[sort]\033[0m calling " 
                    << exportName << " (index=" << exp.index << ")\n";
            interpreter.callFunctionByExportName(exportName);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
