#include "Instructionset.h"
#include "MMU.h"

// ── Registro de tabla de despacho ────────────────────────────────────────────

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

// ── R-Type (RV32I + RV32M) ────────────────────────────────────────────────────

void InstructionSet::OP(uint32_t instr) {
    uint8_t dest = rd(instr);
    uint8_t s1   = rs1(instr);
    uint8_t s2   = rs2(instr);
    uint8_t f3   = funct3(instr);
    uint8_t f7   = funct7(instr);

    if (dest == 0) return;

    // ── RV32M: funct7 == 0x01 ─────────────────────────────────────────────
    if (f7 == 0x01) {
        int32_t  a = static_cast<int32_t>(regs[s1]);
        int32_t  b = static_cast<int32_t>(regs[s2]);
        uint32_t u = regs[s1];
        uint32_t v = regs[s2];

        switch (f3) {
            case 0x0: // MUL — bits [31:0] del producto con signo
                regs[dest] = static_cast<uint32_t>(
                    static_cast<int64_t>(a) * static_cast<int64_t>(b));
                break;
            case 0x1: // MULH — bits [63:32] signed × signed
                regs[dest] = static_cast<uint32_t>(
                    (static_cast<int64_t>(a) * static_cast<int64_t>(b)) >> 32);
                break;
            case 0x2: // MULHSU — bits [63:32] signed × unsigned
                regs[dest] = static_cast<uint32_t>(
                    (static_cast<int64_t>(a) * static_cast<uint64_t>(v)) >> 32);
                break;
            case 0x3: // MULHU — bits [63:32] unsigned × unsigned
                regs[dest] = static_cast<uint32_t>(
                    (static_cast<uint64_t>(u) * static_cast<uint64_t>(v)) >> 32);
                break;
            case 0x4: // DIV — división entera con signo
                if (b == 0) {
                    regs[dest] = 0xFFFFFFFF;                        // división por cero → -1
                } else if (a == INT32_MIN && b == -1) {
                    regs[dest] = static_cast<uint32_t>(INT32_MIN);  // overflow
                } else {
                    regs[dest] = static_cast<uint32_t>(a / b);
                }
                break;
            case 0x5: // DIVU — división entera sin signo
                if (v == 0) {
                    regs[dest] = 0xFFFFFFFF;                        // división por cero → 2^32-1
                } else {
                    regs[dest] = u / v;
                }
                break;
            case 0x6: // REM — resto con signo
                if (b == 0) {
                    regs[dest] = static_cast<uint32_t>(a);          // división por cero → dividendo
                } else if (a == INT32_MIN && b == -1) {
                    regs[dest] = 0;                                  // overflow → 0
                } else {
                    regs[dest] = static_cast<uint32_t>(a % b);
                }
                break;
            case 0x7: // REMU — resto sin signo
                if (v == 0) {
                    regs[dest] = u;                                  // división por cero → dividendo
                } else {
                    regs[dest] = u % v;
                }
                break;
            default: break;
        }
        return;
    }

    // ── RV32I base ────────────────────────────────────────────────────────
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

    int32_t imm = ((static_cast<int32_t>(instr) >> 31) << 12)
                | (((instr >>  7) & 0x1)  << 11)
                | (((instr >> 25) & 0x3F) <<  5)
                | (((instr >>  8) & 0xF)  <<  1);

    uint32_t base_pc = PC - 4;
    bool take = false;

    switch (f3) {
        case 0x0: take = (regs[s1] == regs[s2]);                                             break; // BEQ
        case 0x1: take = (regs[s1] != regs[s2]);                                             break; // BNE
        case 0x4: take = (static_cast<int32_t>(regs[s1]) <  static_cast<int32_t>(regs[s2])); break; // BLT
        case 0x5: take = (static_cast<int32_t>(regs[s1]) >= static_cast<int32_t>(regs[s2])); break; // BGE
        case 0x6: take = (regs[s1] <  regs[s2]);                                             break; // BLTU
        case 0x7: take = (regs[s1] >= regs[s2]);                                             break; // BGEU
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

    int32_t imm = ((static_cast<int32_t>(instr) >> 31) << 20)
                | (((instr >> 12) & 0xFF)  << 12)
                | (((instr >> 20) & 0x1)   << 11)
                | (((instr >> 21) & 0x3FF) <<  1);

    uint32_t base_pc = PC - 4;

    if (dest != 0)
        regs[dest] = base_pc + 4;

    PC = base_pc + imm;
}

void InstructionSet::JALR(uint32_t instr) {
    uint8_t dest = rd(instr);
    uint8_t s1   = rs1(instr);
    int32_t imm  = static_cast<int32_t>(instr) >> 20;

    uint32_t base_pc   = PC - 4;
    uint32_t target_pc = (regs[s1] + imm) & ~1u;

    if (dest != 0)
        regs[dest] = base_pc + 4;

    PC = target_pc;
}

// ── Sistema / privilegiado ────────────────────────────────────────────────────
//
// El reset_vector de riscv-tests hace lo siguiente antes de correr los tests:
//
//   1. csrr  a0, mhartid      → leer hart ID (debe ser 0)
//   2. csrw  mtvec, t0        → setear vector de traps varias veces
//   3. csrw  pmpaddr0 / pmpcfg0 → configurar PMP
//   4. csrw  mepc, t0         → guardar PC de retorno = dirección de test_2
//   5. mret                   → saltar a mepc (test_2) para correr los tests
//
// Sin mret el PC nunca llega a test_2 y el test nunca escribe tohost.
// Sin los CSRs correctos el test cae en trap_vector y reporta FAIL.

void InstructionSet::SYSTEM(uint32_t instr) {
    uint8_t  f3      = funct3(instr);
    uint32_t csr_num = instr >> 20;
    uint8_t  dest    = rd(instr);
    uint8_t  s1      = rs1(instr);

    // ── Instrucciones especiales (f3 == 0) ───────────────────────────────
    if (f3 == 0x0) {
        switch (csr_num) {
            case 0x000:  // ECALL
                if (syscalls_) {
                    // Modo Doom/programa: delegar al manejador de syscalls
                    bool cont = syscalls_->handle();
                    if (!cont) halted = true;
                } else {
                    // Modo riscv-tests: fallback con tohost
                    if (regs[17] == 93) {
                        if (memory.tohost_addr != 0)
                            memory.storeWord(memory.tohost_addr,
                                regs[10] == 0 ? 1 : (regs[10] << 1 | 1));
                    }
                    halted = true;
                }
                break;

            case 0x001:  // EBREAK
                halted = true;
                break;

            case 0x302:  // MRET
                PC = csr_mepc;
                break;

            default:
                break;
        }
        return;
    }

    // ── Tabla de lectura de CSRs ──────────────────────────────────────────
    auto csr_read = [&](uint32_t addr) -> uint32_t {
        switch (addr) {
            case 0x300: return csr_mstatus;
            case 0x301: return 0x40001100;
            case 0x302: return csr_medeleg;
            case 0x303: return csr_mideleg;
            case 0x304: return csr_mie;
            case 0x305: return csr_mtvec;
            case 0x340: return csr_mscratch;
            case 0x341: return csr_mepc;
            case 0x342: return csr_mcause;
            case 0x180: return csr_satp;
            case 0x3a0: return csr_pmpcfg0;
            case 0x3b0: return csr_pmpaddr0;
            case 0xf14: return 0;
            default:    return 0;
        }
    };

    // ── Tabla de escritura de CSRs ────────────────────────────────────────
    auto csr_write = [&](uint32_t addr, uint32_t val) {
        switch (addr) {
            case 0x300: csr_mstatus  = val; break;
            case 0x302: csr_medeleg  = val; break;
            case 0x303: csr_mideleg  = val; break;
            case 0x304: csr_mie      = val; break;
            case 0x305: csr_mtvec    = val; break;
            case 0x340: csr_mscratch = val; break;
            case 0x341: csr_mepc     = val; break;
            case 0x342: csr_mcause   = val; break;
            case 0x180: csr_satp     = val; break;
            case 0x3a0: csr_pmpcfg0  = val; break;
            case 0x3b0: csr_pmpaddr0 = val; break;
            default: break;
        }
    };

    uint32_t old_val = csr_read(csr_num);
    uint32_t src     = (f3 & 0x4) ? (uint32_t)s1 : regs[s1];

    switch (f3 & 0x3) {
        case 0x1: csr_write(csr_num, src);             break;
        case 0x2: csr_write(csr_num, old_val |  src);  break;
        case 0x3: csr_write(csr_num, old_val & ~src);  break;
    }

    if (dest != 0) regs[dest] = old_val;
}

// ── FENCE ─────────────────────────────────────────────────────────────────────
// No-op: emulador simple sin caché ni reordenamiento de memoria.
// El test rv32ui-p-fence_i usa esta instrucción — debe existir pero no hacer nada.

void InstructionSet::FENCE(uint32_t /*instr*/) {
    // no-op intencional
}