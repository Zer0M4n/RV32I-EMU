#include "CPU.h"

uint8_t CPU::opcode(uint32_t i) { return i & 0x7F; }
uint8_t CPU::rd(uint32_t i) { return (i >> 7) & 0x1F; }
uint8_t CPU::funct3(uint32_t i) { return (i >> 12) & 0x07; }
uint8_t CPU::rs1(uint32_t i) { return (i >> 15) & 0x1F; }
uint8_t CPU::rs2(uint32_t i) { return (i >> 20) & 0x1F; }
uint8_t CPU::funct7(uint32_t i) { return (i >> 25) & 0x7F; }

CPU::CPU(MMU& mmu_instance) : memory(mmu_instance), PC(0)
{  
    PC = 0x80000000;
    regs.fill(0);
    regs[2] = 0x00100000; // Stack Pointer
    opcode_pointer.fill(nullptr);
    opcode_pointer[0x37] = &CPU::LUI;
    opcode_pointer[0x13] = &CPU::OP_IMM;
    opcode_pointer[0X23] = &CPU::STORE;
    opcode_pointer[0x03] = &CPU::LOAD;

    // Nuevos Opcodes de control de flujo
    opcode_pointer[0x63] = &CPU::BRANCH; // Opcode 99
    opcode_pointer[0x6F] = &CPU::JAL;    // Opcode 111
    opcode_pointer[0x67] = &CPU::JALR;   // Opcode 103

    opcode_pointer[0x73] = &CPU::SYSTEM;

    opcode_pointer[0x17] = &CPU::AUIPC;
    opcode_pointer[0x33] = &CPU::OP;

    opcode_pointer[0x0F] = &CPU::FENCE;

}

uint32_t CPU::fetch()
{
    uint32_t INST = memory.loadWord(PC);
    PC = PC + 4;
    return INST;
}

void CPU::step()
{
    uint32_t current_pc = PC; 

    uint32_t inst = fetch();
    uint8_t op = opcode(inst);

    std::cout << "========================================\n";
    std::cout << "[DEBUG] PC: 0x" << std::hex << std::setw(8) << std::setfill('0') << current_pc 
              << " | Inst: 0x" << std::setw(8) << inst 
              << " | Opcode: 0x" << std::setw(2) << (int)op << "\n";
    
    // Ejecutamos la instrucción
    if (opcode_pointer[op] != nullptr)
    {
        (this->*opcode_pointer[op])(inst);
    }
    else
    {
        std::cout << "[ERROR] Opcode no implementado: 0x" << std::hex << (int)op << "\n";
    }

    std::cout << "Registros activos:\n";
    for (int i = 0; i < 32; i++) 
    {
        if (regs[i] != 0) 
        {
            std::cout << "  x" << std::dec << i << " : 0x" 
                      << std::hex << std::setw(8) << std::setfill('0') << regs[i] << "\n";
        }
    }
    std::cout << "========================================\n\n";
}

void CPU::LUI(uint32_t instr)
{
    uint8_t dest_reg = rd(instr);
    uint32_t imm = instr & 0xFFFFF000;

    if (dest_reg != 0)
    {
        regs[dest_reg] = imm;
    }
}
void CPU::OP_IMM(uint32_t instr) {
    uint8_t dest = rd(instr);
    uint8_t s1 = rs1(instr);
    uint8_t f3 = funct3(instr);
    int32_t imm = static_cast<int32_t>(instr) >> 20;
    uint8_t shamt = (instr >> 20) & 0x1F;
    uint8_t f7 = funct7(instr);

    if (dest == 0) return;

    switch (f3) {
        case 0x0: regs[dest] = regs[s1] + imm; break;          // ADDI
        case 0x1: regs[dest] = regs[s1] << shamt; break;       // SLLI
        case 0x2: regs[dest] = (int32_t)regs[s1] < imm; break; // SLTI
        case 0x3: regs[dest] = regs[s1] < (uint32_t)imm; break;// SLTIU
        case 0x4: regs[dest] = regs[s1] ^ imm; break;          // XORI
        case 0x5: regs[dest] = (f7 == 0x20)
                    ? (int32_t)regs[s1] >> shamt   // SRAI
                    : regs[s1] >> shamt; break;    // SRLI
        case 0x6: regs[dest] = regs[s1] | imm; break;  // ORI
        case 0x7: regs[dest] = regs[s1] & imm; break;  // ANDI
        default: break;
    }
}
void CPU::STORE(uint32_t instr)
{
    uint8_t src_reg1 = rs1(instr); 
    uint8_t src_reg2 = rs2(instr); 
    uint8_t f3 = funct3(instr);

    int32_t imm = ((static_cast<int32_t>(instr) >> 25) << 5) | ((instr >> 7) & 0x1F);

    uint32_t addr = regs[src_reg1] + imm;

    switch (f3)
    {
        case 0x0: // SB (Store Byte - 8 bits)
            memory.storeByte(addr, regs[src_reg2] & 0xFF);
            break;
            
        case 0x1: // SH (Store Halfword - 16 bits)
            memory.storeHalfWolf(addr, regs[src_reg2] & 0xFFFF);
            break;
            
        case 0x2: // SW (Store Word - 32 bits)
            memory.storeWord(addr, regs[src_reg2]);
            break;
            
        default:
            std::cout << "[ERROR] funct3 no implementado en STORE: 0x" << std::hex << (int)f3 << "\n";
            break;
    }
}
void CPU::LOAD(uint32_t instr)
{
    uint8_t dest_reg = rd(instr);
    uint8_t src_reg1 = rs1(instr);
    uint8_t f3 = funct3(instr);

    int32_t imm = static_cast<int32_t>(instr) >> 20;

    uint32_t addr = regs[src_reg1] + imm;

    if (dest_reg != 0)
    {
        switch (f3)
        {
            case 0x0: // LB (Load Byte - con extensión de signo)
                regs[dest_reg] = static_cast<int8_t>(memory.loadByte(addr));
                break;
                
            case 0x1: // LH (Load Halfword - con extensión de signo)
                regs[dest_reg] = static_cast<int16_t>(memory.loadHalfWolf(addr));
                break;
                
            case 0x2: // LW (Load Word - 32 bits, no requiere extensión)
                regs[dest_reg] = memory.loadWord(addr);
                break;
                
            case 0x4: // LBU (Load Byte Unsigned - relleno con ceros)
                regs[dest_reg] = memory.loadByte(addr);
                break;
                
            case 0x5: // LHU (Load Halfword Unsigned - relleno con ceros)
                regs[dest_reg] = memory.loadHalfWolf(addr);
                break;
                
            default:
                std::cout << "[ERROR] funct3 no implementado en LOAD: 0x" << std::hex << (int)f3 << "\n";
                break;
        }
    }
}

void CPU::BRANCH(uint32_t instr)
{
    uint8_t src_reg1 = rs1(instr);
    uint8_t src_reg2 = rs2(instr);
    uint8_t f3 = funct3(instr);

    int32_t imm = ((static_cast<int32_t>(instr) >> 31) << 12) | 
                  (((instr >> 7) & 0x1) << 11) | 
                  (((instr >> 25) & 0x3F) << 5) | 
                  (((instr >> 8) & 0xF) << 1);

    uint32_t current_pc = PC - 4; // La dirección de ESTA instrucción
    bool take_branch = false;

    // Evaluamos la condición
    switch (f3)
    {
        case 0x0: take_branch = (regs[src_reg1] == regs[src_reg2]); break; // BEQ
        case 0x1: take_branch = (regs[src_reg1] != regs[src_reg2]); break; // BNE
        case 0x4: take_branch = (static_cast<int32_t>(regs[src_reg1]) < static_cast<int32_t>(regs[src_reg2])); break; // BLT (Con signo)
        case 0x5: take_branch = (static_cast<int32_t>(regs[src_reg1]) >= static_cast<int32_t>(regs[src_reg2])); break; // BGE (Con signo)
        case 0x6: take_branch = (regs[src_reg1] < regs[src_reg2]); break; // BLTU (Sin signo)
        case 0x7: take_branch = (regs[src_reg1] >= regs[src_reg2]); break; // BGEU (Sin signo)
        default: std::cout << "[ERROR] funct3 no implementado en BRANCH: 0x" << std::hex << (int)f3 << "\n"; break;
    }

    if (take_branch) {
        PC = current_pc + imm; // Efectuamos el salto
    }
}

void CPU::JAL(uint32_t instr)
{
    uint8_t dest_reg = rd(instr);

    // Gimnasia de bits para el inmediato (Tipo-J) con extensión de signo
    int32_t imm = ((static_cast<int32_t>(instr) >> 31) << 20) | 
                  (((instr >> 12) & 0xFF) << 12) | 
                  (((instr >> 20) & 0x1) << 11) | 
                  (((instr >> 21) & 0x3FF) << 1);

    uint32_t current_pc = PC - 4;

    if (dest_reg != 0) {
        regs[dest_reg] = current_pc + 4; // Guardamos la dirección de retorno (Link)
    }

    PC = current_pc + imm; // Saltamos
}

void CPU::JALR(uint32_t instr)
{
    uint8_t dest_reg = rd(instr);
    uint8_t src_reg1 = rs1(instr);
    
    int32_t imm = static_cast<int32_t>(instr) >> 20;

    uint32_t current_pc = PC - 4;
    
    uint32_t target_pc = (regs[src_reg1] + imm) & ~1;

    if (dest_reg != 0) {
        regs[dest_reg] = current_pc + 4; // Guardamos la dirección de retorno
    }

    PC = target_pc; // Saltamos
}
void CPU::SYSTEM(uint32_t instr) {
    uint8_t f3 = funct3(instr);
    uint32_t funct12 = instr >> 20;
      if (f3 == 0x0 && funct12 == 0x000) { // ECALL
        if (regs[3] == 1)
            std::cout << "\n✓ TEST PASSED\n";
        else
            std::cout << "\n✗ TEST FAILED en caso: " << std::dec << (regs[3] >> 1) << "\n";
        halted = true;
    }

    if (f3 == 0x0) {
        if (funct12 == 0x000) { // ECALL
            halted = true;
        } else if (funct12 == 0x001) { // EBREAK
            halted = true;
        }
    } else {
        // CSR — ignorar silenciosamente (los tests lo usan para setup)
        uint8_t dest = rd(instr);
        if (dest != 0) regs[dest] = 0; // csrr devuelve 0
        // No crash, continuar
    }
}
void CPU::AUIPC(uint32_t instr) {
    uint8_t dest = rd(instr);
    uint32_t imm = instr & 0xFFFFF000;
    if (dest != 0)
        regs[dest] = (PC - 4) + imm; // PC ya avanzó 4 en fetch()
}
void CPU::OP(uint32_t instr) {
    uint8_t dest = rd(instr);
    uint8_t s1 = rs1(instr);
    uint8_t s2 = rs2(instr);
    uint8_t f3 = funct3(instr);
    uint8_t f7 = funct7(instr);

    if (dest == 0) return;

    switch (f3) {
        case 0x0: regs[dest] = (f7 == 0x20)
                    ? regs[s1] - regs[s2]           // SUB
                    : regs[s1] + regs[s2]; break;   // ADD
        case 0x1: regs[dest] = regs[s1] << (regs[s2] & 0x1F); break; // SLL
        case 0x2: regs[dest] = (int32_t)regs[s1] < (int32_t)regs[s2]; break; // SLT
        case 0x3: regs[dest] = regs[s1] < regs[s2]; break; // SLTU
        case 0x4: regs[dest] = regs[s1] ^ regs[s2]; break; // XOR
        case 0x5: regs[dest] = (f7 == 0x20)
                    ? (int32_t)regs[s1] >> (regs[s2] & 0x1F)  // SRA
                    : regs[s1] >> (regs[s2] & 0x1F); break;   // SRL
        case 0x6: regs[dest] = regs[s1] | regs[s2]; break; // OR
        case 0x7: regs[dest] = regs[s1] & regs[s2]; break; // AND
        default: break;
    }
}
void CPU::FENCE(uint32_t instr) {
    // No-op para emuladores simples — no tenemos caché que sincronizar
    std::cout << "se ejecuto FENCE" << "\n";
}