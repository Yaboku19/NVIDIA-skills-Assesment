#pragma once
#include <string>

class WasmInterpreter {
public:
    void loadFile(const std::string& path);
    void run();

private:
    std::string sourceCode;
};