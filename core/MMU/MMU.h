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
    // Seteados por el loader ELF al encontrar los símbolos
    uint32_t tohost_addr   = 0;
    uint32_t fromhost_addr = 0;

    uint8_t  loadByte(uint32_t addr);
    uint16_t loadHalfWolf(uint32_t addr);
    uint32_t loadWord(uint32_t addr);

    void storeByte(uint32_t addr, uint8_t val);
    void storeHalfWolf(uint32_t addr, uint16_t val);
    void storeWord(uint32_t addr, uint32_t val);

    void loadBinary(const std::vector<uint8_t>& bin, uint32_t base);

    void setTohostAddr(uint32_t addr)  { tohost_addr   = addr; }
    void setFromhostAddr(uint32_t addr){ fromhost_addr = addr; }

    // Devuelve 0 si tohost no fue seteado
    uint32_t getTohost() {
        if (tohost_addr == 0) return 0;
        return loadWord(tohost_addr);
    }

    explicit MMU(size_t size, uint32_t base = 0x80000000);
    ~MMU();
};
#endif