#pragma once

#include <cstdint>
#include <array>
#include <iostream>
#include <iomanip>

// Forward declaration para evitar dependencia circular
class MMU;

// Tipo del puntero — se define aquí porque InstructionSet es quien
// toma la dirección de sus propios métodos protegidos
class InstructionSet;
using OpcodeHandler = void (InstructionSet::*)(uint32_t);

class InstructionSet {
protected:
    std::array<uint32_t, 32>& regs;
    MMU& memory;
    uint32_t& PC;
    bool& halted;

    // Constructor: recibe referencias de los miembros de CPU
    InstructionSet(std::array<uint32_t, 32>& r, MMU& m, uint32_t& pc, bool& h)
        : regs(r), memory(m), PC(pc), halted(h) {}

    // ── Helpers de decodificación ─────────────────────────────────────────
    static uint8_t opcode(uint32_t i);
    static uint8_t rd    (uint32_t i);
    static uint8_t funct3(uint32_t i);
    static uint8_t rs1   (uint32_t i);
    static uint8_t rs2   (uint32_t i);
    static uint8_t funct7(uint32_t i);

    // ── Registro de la tabla de despacho ──────────────────────────────────
    void registerOpcodes(std::array<OpcodeHandler, 128>& table);

    // ── Handlers de instrucciones ─────────────────────────────────────────
    void LUI   (uint32_t instr);
    void AUIPC (uint32_t instr);

    void OP_IMM(uint32_t instr);
    void OP    (uint32_t instr);

    void LOAD  (uint32_t instr);
    void STORE (uint32_t instr);

    void BRANCH(uint32_t instr);
    void JAL   (uint32_t instr);
    void JALR  (uint32_t instr);

    void SYSTEM(uint32_t instr);
    void FENCE (uint32_t instr);

    // ── CSRs mínimos para riscv-tests ─────────────────────────────────────
    // Los tests -p- usan estos CSRs durante el reset_vector para configurar
    // el entorno antes de correr las pruebas reales.

    uint32_t csr_mstatus  = 0;  // 0x300 — estado de la máquina
    uint32_t csr_medeleg  = 0;  // 0x302 — delegación de excepciones
    uint32_t csr_mideleg  = 0;  // 0x303 — delegación de interrupciones
    uint32_t csr_mie      = 0;  // 0x304 — habilitación de interrupciones
    uint32_t csr_mtvec    = 0;  // 0x305 — vector de traps (trap_vector)
    uint32_t csr_mscratch = 0;  // 0x340 — scratch register
    uint32_t csr_mepc     = 0;  // 0x341 — PC de retorno de excepción (usado por mret)
    uint32_t csr_mcause   = 0;  // 0x342 — causa de la excepción
    uint32_t csr_satp     = 0;  // 0x180 — control de paginación (siempre 0 en -p-)
    uint32_t csr_pmpcfg0  = 0;  // 0x3a0 — config de PMP (Physical Memory Protection)
    uint32_t csr_pmpaddr0 = 0;  // 0x3b0 — dirección de PMP
    // mhartid (0xf14) es de solo lectura y siempre vale 0 (hart único)
    // misa    (0x301) es de solo lectura, se devuelve 0x40001100 (RV32I)
};