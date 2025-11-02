#include "wasm_memory.hpp"
#include <stdexcept>
#include <cstring>
#include <cctype>

void WasmMemory::store8(uint32_t addr, uint8_t value)     { writeBytes(addr, value); }
void WasmMemory::store16(uint32_t addr, uint16_t value)   { writeBytes(addr, value); }
void WasmMemory::store32(uint32_t addr, int32_t value)    { writeBytes(addr, value); }
void WasmMemory::store64(uint32_t addr, int64_t value)    { writeBytes(addr, value); }
void WasmMemory::storeF32(uint32_t addr, float value)     { writeBytes(addr, value); }
void WasmMemory::storeF64(uint32_t addr, double value)    { writeBytes(addr, value); }

uint8_t  WasmMemory::load8(uint32_t addr) const           { return readBytes<uint8_t>(addr); }
uint16_t WasmMemory::load16(uint32_t addr) const          { return readBytes<uint16_t>(addr); }
int32_t  WasmMemory::load32(uint32_t addr) const          { return readBytes<int32_t>(addr); }
int64_t  WasmMemory::load64(uint32_t addr) const          { return readBytes<int64_t>(addr); }
float    WasmMemory::loadF32(uint32_t addr) const         { return readBytes<float>(addr); }
double   WasmMemory::loadF64(uint32_t addr) const         { return readBytes<double>(addr); }

int32_t WasmMemory::grow(int32_t  additionalPages) {
    if (additionalPages < 0) return -1;

    size_t oldPages = sizeInPages();
    size_t newSize = data.size() + (static_cast<size_t>(additionalPages) * PAGE_SIZE);

    try {
        data.resize(newSize, 0);
        std::cout << "\033[1;36m[memory:grow]\033[0m from " << oldPages 
                  << " → " << sizeInPages() << " pages (+" << additionalPages << ")\n";
        return static_cast<int32_t>(oldPages);
    } catch (const std::bad_alloc&) {
        std::cerr << "\033[1;31m[memory:grow]\033[0m failed to allocate additional pages\n";
        return -1; // grow failed
    }
}

void WasmMemory::debugPrint(uint32_t start, uint32_t count) const {
    if (data.empty()) {
        std::cout << "\033[1;35m[memory]\033[0m (empty)\n";
        return;
    }
    if (count == 0 || start + count > data.size())
        count = data.size() - start;

    std::cout << "\n\033[1;35m================ MEMORY DUMP =================\033[0m\n";
    std::cout << "\033[1;35m[memory]\033[0m Pages: " << minPages
              << " (" << (minPages * PAGE_SIZE) << " bytes total)\n";
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
