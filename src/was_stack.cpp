#include "wasm_stack.hpp"
#include <iostream>

void WasmStack::clear() {
    data.clear();
}

void WasmStack::push(int32_t value) {
    data.push_back(value);
}

void WasmStack::dump() const {
    for (auto v : data) {
        std::cout << v << " ";
    }
    std::cout << "\n";
}

int32_t WasmStack::pop() {
    if (data.empty()) {
        throw std::runtime_error("Stack underflow");
    }
    int32_t value = data.back();
    data.pop_back();
    return value;
}