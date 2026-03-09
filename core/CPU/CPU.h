#pragma once

#include "Instructionset.h"
#include "MMU.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <iomanip>

// OpcodeHandler ya está definido en InstructionSet.h

class CPU : public InstructionSet {
public:
    explicit CPU(MMU& mmu);

    void step();
    bool isHalted() const { return halted; }

private:
    // ── Estado del procesador ─────────────────────────────────────────────
    std::array<uint32_t, 32> regs    {};
    uint32_t                 PC      { 0x80000000 };
    bool                     halted  { false };
    MMU&                     memory;

    // ── Tabla de despacho ─────────────────────────────────────────────────
    std::array<OpcodeHandler, 128> opcode_table {};

    // ── Ciclo fetch / decode ──────────────────────────────────────────────
    uint32_t fetch();
    void     printState(uint32_t pc, uint32_t inst, uint8_t op) const;
};