#include "CPU.h"

uint8_t CPU::opcode(uint32_t i) { return i & 0x7F; }
uint8_t CPU::rd(uint32_t i) { return (i >> 7) & 0x1F; }
uint8_t CPU::funct3(uint32_t i) { return (i >> 12) & 0x07; }
uint8_t CPU::rs1(uint32_t i) { return (i >> 15) & 0x1F; }
uint8_t CPU::rs2(uint32_t i) { return (i >> 20) & 0x1F; }
uint8_t CPU::funct7(uint32_t i) { return (i >> 25) & 0x7F; }

uint32_t CPU::fetch()
{
    uint32_t INST = memory.loadWord(regs[2]);
    regs[2] = regs[2] + 4;

    return INST;
}

void CPU::step()
{
    uint32_t inst = fetch();
    uint8_t op = opcode(inst);

    if (opcode_pointer[inst] != nullptr)
    {
        (this->*opcode_pointer[op])(op);
    }
    else
    {
        std::cout << "opcode not implement " << opcode << "\n";
    }
}
