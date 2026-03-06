#ifndef CPU_H
#define CPU_H

#include <iostream>
#include <array>
#include <cstdint>
#include<iomanip>
#include "../MMU/MMU.h"

class CPU {
private:
    MMU& memory;
    std::array<uint32_t, 32> regs;
    uint32_t PC;

    using Instruction = void(CPU::*)(uint32_t); 
    std::array<Instruction, 128> opcode_pointer; 

    static uint8_t opcode(uint32_t i);
    static uint8_t rd(uint32_t i);
    static uint8_t funct3(uint32_t i);
    static uint8_t rs1(uint32_t i);
    static uint8_t rs2(uint32_t i);
    static uint8_t funct7(uint32_t i);

    uint32_t fetch();

public:
    bool halted;
    CPU(MMU& mmu_instance);
    ~CPU() {} 
    void step();
    //opcodes
    void LUI(uint32_t instr); 
    void OP_IMM(uint32_t instr);
    void STORE(uint32_t instr);
    void LOAD(uint32_t instr);
    void BRANCH(uint32_t instr); // Saltos condicionales (BEQ, BNE, etc)
    void JAL(uint32_t instr);    // Jump and Link
    void JALR(uint32_t instr);   // Jump and Link Register
    void SYSTEM(uint32_t instr);

    void AUIPC(uint32_t instr);   // 0x17
    void OP(uint32_t instr);      // 0x33  (ADD, SUB, AND, OR, XOR, SLL, SRL, SRA, SLT, SLTU)
    void FENCE(uint32_t instr);
};


#endif