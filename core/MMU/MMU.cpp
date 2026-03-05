#include "MMU.h"

MMU::MMU(size_t size) : RAM(size, 0) {}
uint8_t MMU::loadByte(uint32_t addres)
{
    if (addres >= RAM.size())
        return 0; // Evita crasheos si lees fuera de la RAM
    return RAM[addres];
}
uint16_t MMU::loadHalfWolf(uint32_t addres)
{
    if (addres + 1 >= RAM.size())
        return 0;

    // Unimos 2 bytes con formato Little-Endian
    uint16_t b0 = RAM[addres];
    uint16_t b1 = RAM[addres + 1] << 8;
    return b0 | b1;
}
uint32_t MMU::loadWord(uint32_t addres)
{
    // Verificamos que no nos salgamos de la memoria
    if (addres + 3 >= RAM.size())
        return 0;

    // Combinamos 4 bytes usando desplazamientos de bits (Little-Endian)
    uint32_t b0 = RAM[addres];
    uint32_t b1 = RAM[addres + 1] << 8;
    uint32_t b2 = RAM[addres + 2] << 16;
    uint32_t b3 = RAM[addres + 3] << 24;

    return b0 | b1 | b2 | b3;
}

void MMU::storeByte(uint32_t address, uint8_t val)
{
    RAM[address] = val;
}
void MMU::storeHalfWolf(uint32_t address, uint16_t val)
{
    RAM[address] = val & 0xFF;
    RAM[address + 1] = (val >> 8) & 0xFF;
}
void MMU::storeWord(uint32_t address, uint32_t val)
{
    RAM[address] = val & 0xFF;
    RAM[address + 1] = (val >> 8) & 0xFF;
    RAM[address + 2] = (val >> 16) & 0xFF;
    RAM[address + 3] = (val >> 24) & 0xFF;
}

void MMU::loadBinary(const std::vector<uint8_t> &bin, uint32_t base)
{
    std::copy(bin.begin(), bin.end(), RAM.begin() + base);
}
MMU::~MMU()
{
}