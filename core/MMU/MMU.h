#ifndef MMU_H
#define MMU_H

#include <iostream>
#include <vector>
class MMU
{
    private:
        std::vector<uint8_t> RAM;

    public:
        uint8_t loadByte(uint32_t addres);
        uint16_t loadHalfWolf(uint32_t addres);
        uint32_t loadWord(uint32_t addres);

        void storeByte(uint32_t, uint8_t val);
        void storeHalfWolf(uint32_t, uint16_t val);
        void storeWord(uint32_t, uint32_t val);
        
        void loadBinary(const std::vector<uint8_t>& bin , uint32_t base);
        explicit MMU(size_t size);
        ~MMU();
};
#endif