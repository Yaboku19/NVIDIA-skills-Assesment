#include "wasm_stack.hpp"

void WasmStack::clear() {
    data.clear();
}

void WasmStack::push(const WasmValue& val) {
    data.push_back(val);
    std::cout << "\033[1;33m[stack]\033[0m push ";
    printTop(val);
}

WasmValue WasmStack::pop() {
    if (data.empty()) throw std::runtime_error("Stack underflow");
    WasmValue val = data.back();
    data.pop_back();
    std::cout << "\033[1;33m[stack]\033[0m pop ";
    printTop(val);
    return val;
}

WasmValue WasmStack::top() {
    if (data.empty()) throw std::runtime_error("Stack underflow");
    WasmValue val = data.back();
    std::cout << "\033[1;33m[stack]\033[0m pop ";
    printTop(val);
    return val;
}

void WasmStack::dump() {
    std::cout << "\033[1;33m[stack dump]\033[0m ";
    for (auto& v : data)
        printTop(v, false);
    std::cout << "\n";
}

void WasmStack::printTop(const WasmValue& v, bool newline) {
    switch (v.type) {
        case ValueType::I32: std::cout << "i32=" << v.i32; break;
        case ValueType::I64: std::cout << "i64=" << v.i64; break;
        case ValueType::F32: std::cout << "f32=" << v.f32; break;
        case ValueType::F64: std::cout << "f64=" << v.f64; break;
    }
    if (newline) std::cout << "\n";
    else std::cout << " ";
}
