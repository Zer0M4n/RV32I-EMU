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
    // Se llama desde el constructor de CPU pasando su propia tabla.
    // Al estar dentro de InstructionSet tiene acceso a sus miembros protegidos.
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
};