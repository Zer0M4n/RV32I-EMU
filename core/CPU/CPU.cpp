#include "CPU.h"
#include <iostream>
#include <iomanip>
#include <fstream>

// ── Constructor ───────────────────────────────────────────────────────────────

CPU::CPU(MMU& mmu)
    : InstructionSet(regs, mmu, PC, halted)
    , memory(mmu)
{
    PC = 0x80000000;
    regs.fill(0);
    regs[2] = 0x00100000; // Stack Pointer

    // InstructionSet registra sus propios handlers desde adentro —
    // sin problema de acceso a miembros protegidos
    registerOpcodes(opcode_table);
}

// ── Fetch ─────────────────────────────────────────────────────────────────────

uint32_t CPU::fetch() {
    uint32_t inst = memory.loadWord(PC);
    PC += 4;
    return inst;
}

// ── Step ─────────────────────────────────────────────────────────────────────
// Devuelve false cuando el CPU queda detenido (halted).

bool CPU::step() {
    uint32_t current_pc = PC;
    uint32_t inst       = fetch();
    uint8_t  op         = opcode(inst);  // viene de InstructionSet

        static std::ofstream dbg("debug.log");
    dbg << "PC=0x" << std::hex << current_pc
        << "  op=0x" << (int)op
        << "  inst=0x" << inst << "\n";
    dbg.flush();

    if (opcode_table[op] != nullptr) {
        (this->*opcode_table[op])(inst);
    } else {
        std::cout << "[ERROR] Opcode no implementado: 0x"
                  << std::hex << (int)op << "\n";
    }

    // ── Empujar snapshot al Logger ────────────────────────────────────────
    CpuSnapshot snap;
    snap.pc       = current_pc;
    snap.inst     = inst;
    snap.opcode   = op;
    snap.mnemonic = Logger::mnemonicOf(op, inst);
    snap.regs     = regs;
    logger_.push(snap);

    return !halted;
}