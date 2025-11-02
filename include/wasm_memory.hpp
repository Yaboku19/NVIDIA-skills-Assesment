#pragma once
#include <vector>
#include <cstdint>
#include <iostream>
#include <cstring>

class WasmMemory {
public:
    static constexpr size_t PAGE_SIZE = 65536;

    WasmMemory(size_t minPages = 1)
        : minPages(minPages), data(minPages * PAGE_SIZE, 0) {}

    // ---- STORE ----
    void store8(uint32_t addr, uint8_t value);
    void store16(uint32_t addr, uint16_t value);
    void store32(uint32_t addr, int32_t value);
    void store64(uint32_t addr, int64_t value);
    void storeF32(uint32_t addr, float value);
    void storeF64(uint32_t addr, double value);

    // ---- LOAD ----
    uint8_t  load8(uint32_t addr) const;
    uint16_t load16(uint32_t addr) const;
    int32_t  load32(uint32_t addr) const;
    int64_t  load64(uint32_t addr) const;
    float    loadF32(uint32_t addr) const;
    double   loadF64(uint32_t addr) const;

    // ---- MANAGEMENT ----
    void grow(size_t additionalPages);
    size_t sizeInPages() const { return data.size() / PAGE_SIZE; }

    void debugPrint(uint32_t start = 0, uint32_t count = 32) const;

private:
    size_t minPages;
    std::vector<uint8_t> data;

    template <typename T>
    void writeBytes(uint32_t addr, const T& value) {
        if (addr + sizeof(T) > data.size())
            throw std::out_of_range("[memory] store out of bounds");
        std::memcpy(&data[addr], &value, sizeof(T));
    }

    template <typename T>
    T readBytes(uint32_t addr) const {
        if (addr + sizeof(T) > data.size())
            throw std::out_of_range("[memory] load out of bounds");
        T value;
        std::memcpy(&value, &data[addr], sizeof(T));
        return value;
    }
};
