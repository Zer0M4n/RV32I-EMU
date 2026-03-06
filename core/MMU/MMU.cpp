#include "MMU.h"
#include <stdexcept>

MMU::MMU(size_t size, uint32_t base) : RAM(size, 0), base_addr(base) {}

static inline uint32_t phys(uint32_t addr, uint32_t base, size_t size) {
    uint32_t p = addr - base;
    if (p >= size) throw std::out_of_range("MMU: acceso fuera de rango");
    return p;
}

uint8_t MMU::loadByte(uint32_t addr) {
    return RAM[phys(addr, base_addr, RAM.size())];
}
uint16_t MMU::loadHalfWolf(uint32_t addr) {
    uint32_t p = phys(addr, base_addr, RAM.size());
    return RAM[p] | (RAM[p+1] << 8);
}
uint32_t MMU::loadWord(uint32_t addr) {
    uint32_t p = phys(addr, base_addr, RAM.size());
    return RAM[p] | (RAM[p+1]<<8) | (RAM[p+2]<<16) | (RAM[p+3]<<24);
}
void MMU::storeByte(uint32_t addr, uint8_t val) {
    RAM[phys(addr, base_addr, RAM.size())] = val;
}
void MMU::storeHalfWolf(uint32_t addr, uint16_t val) {
    uint32_t p = phys(addr, base_addr, RAM.size());
    RAM[p] = val & 0xFF; RAM[p+1] = val >> 8;
}
void MMU::storeWord(uint32_t addr, uint32_t val) {
    uint32_t p = phys(addr, base_addr, RAM.size());
    RAM[p]=val; RAM[p+1]=val>>8; RAM[p+2]=val>>16; RAM[p+3]=val>>24;
}
void MMU::loadBinary(const std::vector<uint8_t>& bin, uint32_t base) {
    uint32_t p = phys(base, base_addr, RAM.size());
    std::copy(bin.begin(), bin.end(), RAM.begin() + p);
}
MMU::~MMU() {}