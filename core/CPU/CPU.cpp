#include "CPU.h"

uint8_t CPU::opcode(uint32_t i) { return i & 0x7F; }
uint8_t CPU::rd(uint32_t i) { return (i >> 7) & 0x1F; }
uint8_t CPU::funct3(uint32_t i) { return (i >> 12) & 0x07; }
uint8_t CPU::rs1(uint32_t i) { return (i >> 15) & 0x1F; }
uint8_t CPU::rs2(uint32_t i) { return (i >> 20) & 0x1F; }
uint8_t CPU::funct7(uint32_t i) { return (i >> 25) & 0x7F; }

CPU::CPU(MMU& mmu_instance) : memory(mmu_instance), PC(0)
{  
    regs.fill(0);
    regs[2] = 0x00100000; // Stack Pointer
    opcode_pointer.fill(nullptr);
    opcode_pointer[0x37] = &CPU::LUI;
    opcode_pointer[0x13] = &CPU::OP_IMM;
    opcode_pointer[0X23] = &CPU::STORE;
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
void CPU::OP_IMM(uint32_t instr)
{
    uint8_t dest_reg = rd(instr);
    uint8_t src_reg1 = rs1(instr);
    uint8_t f3 = funct3(instr);

    int32_t imm = static_cast<int32_t>(instr) >> 20;

    if (dest_reg != 0) 
    {
        switch (f3)
        {
            case 0x0: // funct3 = 0x0 corresponde a ADDI
                regs[dest_reg] = regs[src_reg1] + imm;
                break;
            default:
                std::cout << "funct3 no implementado en OP-IMM: 0x" << std::hex << (int)f3 << "\n";
                break;
        }
    }
}
void CPU::STORE(uint32_t instr)
{
    uint8_t src_reg1 = rs1(instr); // Dirección base
    uint8_t src_reg2 = rs2(instr); // Dato a guardar
    uint8_t f3 = funct3(instr);

    // 🧠 Reconstrucción del inmediato dividido en instrucciones tipo S
    // Los bits superiores (31 a 25) forman la parte alta del inmediato.
    // Los bits inferiores (11 a 7) forman la parte baja.
    // Usamos static_cast a int32_t para mantener el signo negativo si lo hay.
    int32_t imm = ((static_cast<int32_t>(instr) >> 25) << 5) | ((instr >> 7) & 0x1F);

    // Calculamos la dirección final de memoria
    uint32_t addr = regs[src_reg1] + imm;

    // Ejecutamos según el tamaño (funct3)
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