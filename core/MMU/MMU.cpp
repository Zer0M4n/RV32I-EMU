#include "MMU.h"

explicit MMU::MMU(size_t size) : RAM(size, 0) {}
uint8_t MMU::loadByte(uint32_t addres)
{
}
uint16_t MMU::loadHalfWolf(uint32_t addres)
{
}
uint32_t MMU::loadWord(uint32_t addres)
{
}

void MMU::storeByte(uint32_t, uint8_t val)
{
}
void MMU::storeHalfWolf(uint32_t, uint16_t val)
{
}
void MMU::storeWord(uint32_t, uint32_t val)
{
}

void MMU::loadBinary(const std::vector<uint8_t> &bin, uint32_t base)
{
    std::copy(bin.begin(), bin.end(), RAM.begin() + base);
}
