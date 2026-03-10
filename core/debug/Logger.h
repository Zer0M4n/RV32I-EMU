#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <string>
#include <sstream>
#include <iomanip>
#include <mutex>

// ─────────────────────────────────────────────────────────────────────────────
// Snapshot de un ciclo de ejecución
// ─────────────────────────────────────────────────────────────────────────────
struct CpuSnapshot {
    uint32_t                 pc;
    uint32_t                 inst;
    uint8_t                  opcode;
    std::string              mnemonic;   // e.g. "ADDI x1, x0, 42"
    std::array<uint32_t, 32> regs;
};

// ─────────────────────────────────────────────────────────────────────────────
// Logger: hilo-seguro, mantiene historial de snapshots y último estado
// ─────────────────────────────────────────────────────────────────────────────
class Logger {
public:
    static constexpr size_t MAX_HISTORY = 256;

    // Registra un nuevo ciclo
    void push(const CpuSnapshot& snap) {
        std::lock_guard<std::mutex> lock(mtx_);
        history_.push_back(snap);
        if (history_.size() > MAX_HISTORY)
            history_.pop_front();
        latest_ = snap;
    }

    // Último snapshot (para la TUI en tiempo real)
    CpuSnapshot latest() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return latest_;
    }

    // Copia del historial completo
    std::deque<CpuSnapshot> history() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return history_;
    }

    // Utilidad: convierte opcode a nombre legible
    static std::string mnemonicOf(uint8_t op, uint32_t inst) {
        // funct3 para diferencias internas
        uint8_t f3 = (inst >> 12) & 0x07;
        uint8_t f7 = (inst >> 25) & 0x7F;
        switch (op) {
            case 0x37: return "LUI";
            case 0x17: return "AUIPC";
            case 0x6F: return "JAL";
            case 0x67: return "JALR";
            case 0x63: {
                const char* names[] = {"BEQ","BNE","?","?","BLT","BGE","BLTU","BGEU"};
                return names[f3 & 0x7];
            }
            case 0x03: {
                const char* names[] = {"LB","LH","LW","?","LBU","LHU","?","?"};
                return names[f3 & 0x7];
            }
            case 0x23: {
                const char* names[] = {"SB","SH","SW","?","?","?","?","?"};
                return names[f3 & 0x7];
            }
            case 0x13: {
                if (f3 == 0x5) return (f7 == 0x20) ? "SRAI" : "SRLI";
                const char* names[] = {"ADDI","SLLI","SLTI","SLTIU","XORI","SRLI","ORI","ANDI"};
                return names[f3 & 0x7];
            }
            case 0x33: {
                if (f3 == 0x0) return (f7 == 0x20) ? "SUB" : "ADD";
                if (f3 == 0x5) return (f7 == 0x20) ? "SRA" : "SRL";
                const char* names[] = {"ADD","SLL","SLT","SLTU","XOR","SRL","OR","AND"};
                return names[f3 & 0x7];
            }
            case 0x73: return "SYSTEM";
            case 0x0F: return "FENCE";
            default:   return "???";
        }
    }

private:
    mutable std::mutex      mtx_;
    std::deque<CpuSnapshot> history_;
    CpuSnapshot             latest_ {};
};