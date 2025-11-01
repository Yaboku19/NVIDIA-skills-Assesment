#pragma once
#include <vector>
#include <cstdint>

class WasmStack {
public:
    void push(int32_t value);
    int32_t pop();
    void dump() const;
    void clear();
private:
    std::vector<int32_t> data;
};
