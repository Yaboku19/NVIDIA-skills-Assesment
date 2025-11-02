#pragma once

#include <vector>
#include <string>

struct FuncType {
    std::vector<std::string> params = {};
    std::string resultType = "";
};

struct Value {
    std::string name = "";
    std::string type = "";
};

struct FuncDef {
    int index = -1;
    std::vector<Value> paramNames = {};
    Value result = {};
    std::string name = "";
    std::vector<std::string> body = {};
};

struct WasmExport {
    std::string name;
    std::string kind;
    int index; 
};