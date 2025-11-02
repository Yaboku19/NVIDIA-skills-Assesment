#pragma once
#include <vector>
#include <cstdint>
#include <iostream>

class WasmMemory {
public:
    static constexpr size_t PAGE_SIZE = 65536;

    WasmMemory(size_t minPages = 1)
        : minPages(minPages), data(minPages * PAGE_SIZE, 0) {}


    void store32(uint32_t addr, int32_t value);
    int32_t load32(uint32_t addr) const;

    void store8(uint32_t addr, uint8_t value);
    uint8_t load8(uint32_t addr) const;

    void grow(size_t additionalPages);
    size_t sizeInPages() const { return data.size() / PAGE_SIZE; }

    void debugPrint(uint32_t start = 0, uint32_t count = 32) const;

    int getIndex() const { return index; }
    void setIndex(int i) { index = i; }

private:
    size_t minPages;
    std::vector<uint8_t> data;
    int index = 0;
};
