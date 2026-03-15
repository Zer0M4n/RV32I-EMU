#pragma once

#include "Instructionset.h"
#include "Logger.h"
#include "MMU.h"

#include <array>
#include <cstdint>

class CPU : public InstructionSet {
public:
    explicit CPU(MMU& mmu);

    bool step();

    bool     isHalted() const { return halted; }
    Logger&  getLogger()      { return logger_; }

    // Permite al loader ELF setear el entry point real
    void setPC(uint32_t addr) { PC = addr; }
    uint32_t getPC() const    { return PC; }

private:
    std::array<uint32_t, 32> regs   {};
    uint32_t                 PC     { 0x80000000 };
    bool                     halted { false };
    MMU&                     memory;

    Logger logger_;

    std::array<OpcodeHandler, 128> opcode_table {};

    uint32_t fetch();
};