#include "CPU.h"

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

// ── Debug helper ──────────────────────────────────────────────────────────────

void CPU::printState(uint32_t pc, uint32_t inst, uint8_t op) const {
    std::cout << "========================================\n"
              << "[DEBUG] PC: 0x" << std::hex << std::setw(8) << std::setfill('0') << pc
              << " | Inst: 0x"    << std::setw(8) << inst
              << " | Opcode: 0x"  << std::setw(2) << (int)op << "\n";

    std::cout << "Registros activos:\n";
    for (int i = 0; i < 32; ++i) {
        if (regs[i] != 0) {
            std::cout << "  x" << std::dec << i
                      << " : 0x" << std::hex << std::setw(8) << std::setfill('0')
                      << regs[i] << "\n";
        }
    }
    std::cout << "========================================\n\n";
}

// ── Step (un ciclo de reloj) ──────────────────────────────────────────────────

void CPU::step() {
    uint32_t current_pc = PC;
    uint32_t inst       = fetch();
    uint8_t  op         = opcode(inst);   // opcode() viene de InstructionSet

    if (opcode_table[op] != nullptr) {
        // Llamamos el handler a través del puntero de InstructionSet
        // 'this' es válido porque CPU hereda de InstructionSet
        (this->*opcode_table[op])(inst);
    } else {
        std::cout << "[ERROR] Opcode no implementado: 0x"
                  << std::hex << (int)op << "\n";
    }

    printState(current_pc, inst, op);
}