#include "CPU.h"

uint32_t CPU::fetch()
{
    uint32_t INST = memory.loadWord(regs[2]);
    regs[2] = regs[2] + 4;

    return INST;
}

void CPU::step()
{
    uint32_t inst = fetch() & 0x7F;
    uint32_t rd = (inst >> 7) & 0x1F;

    if (opcode_pointer[inst] != nullptr)
    {
        (this->*opcode_pointer[inst])(inst);    } else
    {
        std::cout << "opcode not implement " << opcode << "\n";
    }
    

}
