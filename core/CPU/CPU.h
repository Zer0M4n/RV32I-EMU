#pragma once

#include "Instructionset.h"
#include "Logger.h"
#include "MMU.h"

#include <array>
#include <cstdint>

// OpcodeHandler ya está definido en InstructionSet.h

class CPU : public InstructionSet {
public:
    explicit CPU(MMU& mmu);

    // Ejecuta un ciclo. Devuelve false si el CPU quedó detenido (halted).
    bool step();

    bool    isHalted() const { return halted; }
    Logger& getLogger()      { return logger_; }

private:
    // ── Estado del procesador ─────────────────────────────────────────────
    std::array<uint32_t, 32> regs   {};
    uint32_t                 PC     { 0x80000000 };
    bool                     halted { false };
    MMU&                     memory;

    // ── Logger interno ────────────────────────────────────────────────────
    Logger logger_;

    // ── Tabla de despacho ─────────────────────────────────────────────────
    std::array<OpcodeHandler, 128> opcode_table {};

    // ── Ciclo fetch / decode ──────────────────────────────────────────────
    uint32_t fetch();
};