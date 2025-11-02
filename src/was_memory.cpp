#include "wasm_memory.hpp"
#include <stdexcept>
#include <cstring>

void WasmMemory::store32(uint32_t addr, int32_t value) {
    if (addr + 4 > data.size())
        throw std::out_of_range("[memory] store32 out of bounds");
    std::memcpy(&data[addr], &value, 4);
}

int32_t WasmMemory::load32(uint32_t addr) const {
    if (addr + 4 > data.size())
        throw std::out_of_range("[memory] load32 out of bounds");
    int32_t value;
    std::memcpy(&value, &data[addr], 4);
    return value;
}

void WasmMemory::store8(uint32_t addr, uint8_t value) {
    if (addr >= data.size())
        throw std::out_of_range("[memory] store8 out of bounds");
    data[addr] = value;
}

uint8_t WasmMemory::load8(uint32_t addr) const {
    if (addr >= data.size())
        throw std::out_of_range("[memory] load8 out of bounds");
    return data[addr];
}

void WasmMemory::grow(size_t additionalPages) {
    data.resize(data.size() + additionalPages * PAGE_SIZE, 0);
}

void WasmMemory::debugPrint(uint32_t start, uint32_t count) const {
    if (data.empty()) {
        std::cout << "\033[1;35m[memory]\033[0m (empty)\n";
        return;
    }
    if (count == 0 || start + count > data.size()) {
        count = data.size() - start;
    }
    std::cout << "\n\033[1;35m================ MEMORY DUMP =================\033[0m\n";
    std::cout << "\033[1;35m[memory]\033[0m Index: " << index << "\n";
    std::cout << "\033[1;35m[memory]\033[0m Pages: " << minPages << " ("
              << (minPages * PAGE_SIZE) << " bytes total)\n";
    std::cout << "\033[1;35m[memory]\033[0m Dump range: start=" << start
              << ", count=" << count << " bytes\n";
    std::cout << "-----------------------------------------------\n";
    for (uint32_t offset = 0; offset < count; offset += 16) {
        uint32_t addr = start + offset;
        printf("0x%08X  ", addr);
        for (uint32_t i = 0; i < 16; ++i) {
            if (addr + i < data.size())
                printf("%02X ", data[addr + i]);
            else
                printf("   ");
        }
        printf(" | ");
        for (uint32_t i = 0; i < 16; ++i) {
            if (addr + i < data.size()) {
                unsigned char c = data[addr + i];
                printf("%c", std::isprint(c) ? c : '.');
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
    std::cout << "-----------------------------------------------\n";
    std::cout << "\033[1;35m[memory]\033[0m Total bytes: " << data.size() << "\n";
    std::cout << "\033[1;35m===============================================\033[0m\n";
}


