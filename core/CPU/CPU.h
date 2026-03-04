#ifndef CPU_H
#define CPU_H

#include <iostream>
#include <array>
#include <cstdint> // Es buena práctica incluir esto para usar uint32_t
#include "MMU.h"

class CPU {
private:

    MMU& memory;
    std::array<uint32_t, 32> regs; // 🗄️ Ajustado a 32 para que los índices 0-31 coincidan con x0-x31
    
    using Instruction = int(CPU::*)(uint8_t);
    std::array<Instruction,40> opcode_pointer; //pointer fuction the cpu
    
    static uint8_t  opcode(uint32_t i) { return i & 0x7F; }
    static uint8_t  rd    (uint32_t i) { return (i >> 7)  & 0x1F; }
    static uint8_t  funct3(uint32_t i) { return (i >> 12) & 0x07; }
    static uint8_t  rs1   (uint32_t i) { return (i >> 15) & 0x1F; }
    static uint8_t  rs2   (uint32_t i) { return (i >> 20) & 0x1F; }
    static uint8_t  funct7(uint32_t i) { return (i >> 25) & 0x7F; }
    
    uint32_t fetch();
    void step();

public:
    CPU();
    ~CPU();
};

#endif