#pragma once
#include <vector>
#include <iostream>
#include <stdexcept>
#include <cstdint>
#include <string>
#include "struct.h"

class WasmStack {
public:
    void clear();

    void push(const WasmValue& val);

    WasmValue pop();

    WasmValue top();

    void dump();

    bool empty() const {
        return data.empty();
    }

private:
    std::vector<WasmValue> data;

    static void printTop(const WasmValue& v, bool newline = true);
};
