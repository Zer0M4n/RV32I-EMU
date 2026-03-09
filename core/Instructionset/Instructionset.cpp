#include "Instructionset.h"
#include "MMU.h"

// ── Registro de tabla de despacho ────────────────────────────────────────────
// Al estar dentro de InstructionSet, puede tomar &InstructionSet::X sin problema

void InstructionSet::registerOpcodes(std::array<OpcodeHandler, 128>& table) {
    table.fill(nullptr);

    table[0x37] = &InstructionSet::LUI;
    table[0x17] = &InstructionSet::AUIPC;

    table[0x13] = &InstructionSet::OP_IMM;
    table[0x33] = &InstructionSet::OP;

    table[0x03] = &InstructionSet::LOAD;
    table[0x23] = &InstructionSet::STORE;

    table[0x63] = &InstructionSet::BRANCH;
    table[0x6F] = &InstructionSet::JAL;
    table[0x67] = &InstructionSet::JALR;

    table[0x73] = &InstructionSet::SYSTEM;
    table[0x0F] = &InstructionSet::FENCE;
}

// ── Helpers de decodificación ─────────────────────────────────────────────────
uint8_t InstructionSet::opcode(uint32_t i) { return  i        & 0x7F; }
uint8_t InstructionSet::rd    (uint32_t i) { return (i >>  7) & 0x1F; }
uint8_t InstructionSet::funct3(uint32_t i) { return (i >> 12) & 0x07; }
uint8_t InstructionSet::rs1   (uint32_t i) { return (i >> 15) & 0x1F; }
uint8_t InstructionSet::rs2   (uint32_t i) { return (i >> 20) & 0x1F; }
uint8_t InstructionSet::funct7(uint32_t i) { return (i >> 25) & 0x7F; }

// ── U-Type ────────────────────────────────────────────────────────────────────

void InstructionSet::LUI(uint32_t instr) {
    uint8_t dest = rd(instr);
    if (dest != 0)
        regs[dest] = instr & 0xFFFFF000;
}

void InstructionSet::AUIPC(uint32_t instr) {
    uint8_t dest = rd(instr);
    if (dest != 0)
        regs[dest] = (PC - 4) + (instr & 0xFFFFF000); // PC ya avanzó 4 en fetch()
}

// ── I-Type aritmético ─────────────────────────────────────────────────────────

void InstructionSet::OP_IMM(uint32_t instr) {
    uint8_t  dest  = rd(instr);
    uint8_t  s1    = rs1(instr);
    uint8_t  f3    = funct3(instr);
    int32_t  imm   = static_cast<int32_t>(instr) >> 20;
    uint8_t  shamt = (instr >> 20) & 0x1F;
    uint8_t  f7    = funct7(instr);

    if (dest == 0) return;

    switch (f3) {
        case 0x0: regs[dest] = regs[s1] + imm;                          break; // ADDI
        case 0x1: regs[dest] = regs[s1] << shamt;                       break; // SLLI
        case 0x2: regs[dest] = (int32_t)regs[s1] < imm;                 break; // SLTI
        case 0x3: regs[dest] = regs[s1] < (uint32_t)imm;                break; // SLTIU
        case 0x4: regs[dest] = regs[s1] ^ imm;                          break; // XORI
        case 0x5: regs[dest] = (f7 == 0x20)
                    ? (int32_t)regs[s1] >> shamt   // SRAI
                    :           regs[s1] >> shamt;  // SRLI
                  break;
        case 0x6: regs[dest] = regs[s1] | imm;                          break; // ORI
        case 0x7: regs[dest] = regs[s1] & imm;                          break; // ANDI
        default:  break;
    }
}

// ── R-Type ────────────────────────────────────────────────────────────────────

void InstructionSet::OP(uint32_t instr) {
    uint8_t dest = rd(instr);
    uint8_t s1   = rs1(instr);
    uint8_t s2   = rs2(instr);
    uint8_t f3   = funct3(instr);
    uint8_t f7   = funct7(instr);

    if (dest == 0) return;

    switch (f3) {
        case 0x0: regs[dest] = (f7 == 0x20)
                    ? regs[s1] - regs[s2]           // SUB
                    : regs[s1] + regs[s2];           // ADD
                  break;
        case 0x1: regs[dest] = regs[s1] << (regs[s2] & 0x1F);          break; // SLL
        case 0x2: regs[dest] = (int32_t)regs[s1] < (int32_t)regs[s2];  break; // SLT
        case 0x3: regs[dest] = regs[s1] < regs[s2];                     break; // SLTU
        case 0x4: regs[dest] = regs[s1] ^ regs[s2];                     break; // XOR
        case 0x5: regs[dest] = (f7 == 0x20)
                    ? (int32_t)regs[s1] >> (regs[s2] & 0x1F)  // SRA
                    :           regs[s1] >> (regs[s2] & 0x1F); // SRL
                  break;
        case 0x6: regs[dest] = regs[s1] | regs[s2];                     break; // OR
        case 0x7: regs[dest] = regs[s1] & regs[s2];                     break; // AND
        default:  break;
    }
}

// ── Memoria ───────────────────────────────────────────────────────────────────

void InstructionSet::LOAD(uint32_t instr) {
    uint8_t dest = rd(instr);
    uint8_t s1   = rs1(instr);
    uint8_t f3   = funct3(instr);
    int32_t imm  = static_cast<int32_t>(instr) >> 20;

    if (dest == 0) return;

    uint32_t addr = regs[s1] + imm;

    switch (f3) {
        case 0x0: regs[dest] = static_cast<int8_t> (memory.loadByte    (addr)); break; // LB
        case 0x1: regs[dest] = static_cast<int16_t>(memory.loadHalfWolf(addr)); break; // LH
        case 0x2: regs[dest] = memory.loadWord(addr);                            break; // LW
        case 0x4: regs[dest] = memory.loadByte    (addr);                        break; // LBU
        case 0x5: regs[dest] = memory.loadHalfWolf(addr);                        break; // LHU
        default:
            std::cout << "[ERROR] funct3 no implementado en LOAD: 0x"
                      << std::hex << (int)f3 << "\n";
            break;
    }
}

void InstructionSet::STORE(uint32_t instr) {
    uint8_t s1  = rs1(instr);
    uint8_t s2  = rs2(instr);
    uint8_t f3  = funct3(instr);

    int32_t imm = ((static_cast<int32_t>(instr) >> 25) << 5)
                | ((instr >> 7) & 0x1F);

    uint32_t addr = regs[s1] + imm;

    switch (f3) {
        case 0x0: memory.storeByte    (addr, regs[s2] & 0xFF);   break; // SB
        case 0x1: memory.storeHalfWolf(addr, regs[s2] & 0xFFFF); break; // SH
        case 0x2: memory.storeWord    (addr, regs[s2]);           break; // SW
        default:
            std::cout << "[ERROR] funct3 no implementado en STORE: 0x"
                      << std::hex << (int)f3 << "\n";
            break;
    }
}

// ── Control de flujo ──────────────────────────────────────────────────────────

void InstructionSet::BRANCH(uint32_t instr) {
    uint8_t s1 = rs1(instr);
    uint8_t s2 = rs2(instr);
    uint8_t f3 = funct3(instr);

    // Inmediato tipo-B con extensión de signo
    int32_t imm = ((static_cast<int32_t>(instr) >> 31) << 12)
                | (((instr >>  7) & 0x1)  << 11)
                | (((instr >> 25) & 0x3F) <<  5)
                | (((instr >>  8) & 0xF)  <<  1);

    uint32_t base_pc = PC - 4; // dirección de esta instrucción
    bool take = false;

    switch (f3) {
        case 0x0: take = (regs[s1] == regs[s2]);                                          break; // BEQ
        case 0x1: take = (regs[s1] != regs[s2]);                                          break; // BNE
        case 0x4: take = (static_cast<int32_t>(regs[s1]) <  static_cast<int32_t>(regs[s2])); break; // BLT
        case 0x5: take = (static_cast<int32_t>(regs[s1]) >= static_cast<int32_t>(regs[s2])); break; // BGE
        case 0x6: take = (regs[s1] <  regs[s2]);                                          break; // BLTU
        case 0x7: take = (regs[s1] >= regs[s2]);                                          break; // BGEU
        default:
            std::cout << "[ERROR] funct3 no implementado en BRANCH: 0x"
                      << std::hex << (int)f3 << "\n";
            break;
    }

    if (take)
        PC = base_pc + imm;
}

void InstructionSet::JAL(uint32_t instr) {
    uint8_t dest = rd(instr);

    // Inmediato tipo-J con extensión de signo
    int32_t imm = ((static_cast<int32_t>(instr) >> 31) << 20)
                | (((instr >> 12) & 0xFF)  << 12)
                | (((instr >> 20) & 0x1)   << 11)
                | (((instr >> 21) & 0x3FF) <<  1);

    uint32_t base_pc = PC - 4;

    if (dest != 0)
        regs[dest] = base_pc + 4; // dirección de retorno

    PC = base_pc + imm;
}

void InstructionSet::JALR(uint32_t instr) {
    uint8_t dest = rd(instr);
    uint8_t s1   = rs1(instr);
    int32_t imm  = static_cast<int32_t>(instr) >> 20;

    uint32_t base_pc   = PC - 4;
    uint32_t target_pc = (regs[s1] + imm) & ~1u; // bit 0 siempre 0

    if (dest != 0)
        regs[dest] = base_pc + 4;

    PC = target_pc;
}

// ── Sistema / privilegiado ────────────────────────────────────────────────────

void InstructionSet::SYSTEM(uint32_t instr) {
    uint8_t  f3      = funct3(instr);
    uint32_t funct12 = instr >> 20;

    if (f3 == 0x0) {
        if (funct12 == 0x000) { // ECALL
            if (regs[3] == 1)
                std::cout << "\n✓ TEST PASSED\n";
            else
                std::cout << "\n✗ TEST FAILED en caso: "
                          << std::dec << (regs[3] >> 1) << "\n";
            halted = true;
        } else if (funct12 == 0x001) { // EBREAK
            halted = true;
        }
    } else {
        // CSR — ignorar silenciosamente; devolver 0 si hay registro destino
        uint8_t dest = rd(instr);
        if (dest != 0) regs[dest] = 0;
    }
}

void InstructionSet::FENCE(uint32_t /*instr*/) {
    // No-op: emulador simple sin caché que sincronizar
    std::cout << "[FENCE] no-op\n";
}