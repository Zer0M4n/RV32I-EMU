#ifndef MMU_H
#define MMU_H

#include <iostream>
#include <vector>
#include <cstdint>

class MMU {
private:
    std::vector<uint8_t> RAM;
    uint32_t base_addr;

public:
    static const uint32_t TOHOST_ADDR = 0x80001000; // Dirección donde los tests escriben resultado

    uint8_t  loadByte(uint32_t addr);
    uint16_t loadHalfWolf(uint32_t addr);
    uint32_t loadWord(uint32_t addr);

    void storeByte(uint32_t addr, uint8_t val);
    void storeHalfWolf(uint32_t addr, uint16_t val);
    void storeWord(uint32_t addr, uint32_t val);

    void loadBinary(const std::vector<uint8_t>& bin, uint32_t base);

    uint32_t getTohost() { return loadWord(TOHOST_ADDR); }

    explicit MMU(size_t size, uint32_t base = 0x80000000);
    ~MMU();
};
#endif