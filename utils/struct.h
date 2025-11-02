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

enum class ValueType {
    I32,
    I64,
    F32,
    F64
};

struct WasmValue {
    ValueType type;
    union {
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
    };

    WasmValue() : type(ValueType::I32), i32(0) {}
    explicit WasmValue(int32_t v) : type(ValueType::I32), i32(v) {}
    explicit WasmValue(int64_t v) : type(ValueType::I64), i64(v) {}
    explicit WasmValue(float v)   : type(ValueType::F32), f32(v) {}
    explicit WasmValue(double v)  : type(ValueType::F64), f64(v) {}
};

struct WasmGlobal {
    std::string name;
    ValueType type;
    bool mutableFlag;
    WasmValue value;
};
