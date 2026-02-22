#ifndef CPU_H
#define CPU_H

#include <iostream>
#include <array>
#include <cstdint> // Es buena práctica incluir esto para usar uint32_t
#include "MMU.h"

class CPU {
private:

    MMU& memory;

    uint32_t pc; 
    std::array<uint32_t, 32> regs; // 🗄️ Ajustado a 32 para que los índices 0-31 coincidan con x0-x31

    uint32_t fetch();
    void step();

public:
    CPU();
    ~CPU();
};

#endif