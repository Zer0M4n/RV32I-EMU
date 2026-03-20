#pragma once

#include "MMU.h"
#include <cstdint>
#include <array>
#include <string>
#include <unordered_map>

// ─────────────────────────────────────────────────────────────────────────────
// Syscalls — intercepta ecall y emula las syscalls de Linux/newlib
//
// Doom (doomgeneric) necesita:
//   sbrk  (heap)
//   open / openat
//   read
//   write  (printf/puts van aquí)
//   close
//   lseek
//   fstat / stat
//   exit
//   gettimeofday  (game loop)
//
// Convención RV32:
//   a7 (x17) = número de syscall
//   a0 (x10) = arg0 / valor de retorno
//   a1 (x11) = arg1
//   a2 (x12) = arg2
//   a3 (x13) = arg3
// ─────────────────────────────────────────────────────────────────────────────

class Syscalls {
public:
    explicit Syscalls(MMU& mmu, std::array<uint32_t, 32>& regs);

    // Llamar desde InstructionSet::SYSTEM cuando es ECALL
    // Devuelve false si el programa debe terminar (exit)
    bool handle();

private:
    MMU&                       mmu_;
    std::array<uint32_t, 32>&  regs_;

    // Tabla de file descriptors abiertos
    // fd emulado → FILE* del host
    std::unordered_map<int, FILE*> fd_table_;
    int next_fd_ = 3;  // 0=stdin, 1=stdout, 2=stderr ya ocupados

    // ── Helpers ───────────────────────────────────────────────────────────
    uint32_t  a(int n) const { return regs_[10 + n]; }  // a0=x10, a1=x11...
    uint32_t& ret()          { return regs_[10]; }       // valor de retorno en a0

    // Leer string de la memoria del emulador
    std::string readString(uint32_t addr);

    // ── Handlers individuales ─────────────────────────────────────────────
    bool sys_exit();          // a7=93
    bool sys_exit_group();    // a7=94
    bool sys_write();         // a7=64
    bool sys_read();          // a7=63
    bool sys_openat();        // a7=56
    bool sys_close();         // a7=57
    bool sys_lseek();         // a7=62
    bool sys_fstat();         // a7=80
    bool sys_brk();           // a7=214  (sbrk/malloc)
    bool sys_gettimeofday();  // a7=169
    bool sys_unknown();       // cualquier otro
};