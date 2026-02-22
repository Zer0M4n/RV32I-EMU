#ifndef MMU_H
#define MMU_H

#include <iostream>;
#include <vector>;
class MMU
{
    private:
        std::vector<uint8_t> RAM;

    public:
        uint32_t write();
        uint32_t read();
        MMU(/* args */);
        ~MMU();
};
#endif