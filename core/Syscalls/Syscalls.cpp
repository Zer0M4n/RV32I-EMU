#include "Syscalls.h"

#include <iostream>
#include <cstring>
#include <ctime>
#include <sys/stat.h>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────
Syscalls::Syscalls(MMU& mmu, std::array<uint32_t, 32>& regs)
    : mmu_(mmu), regs_(regs)
{
    // Precargar stdin/stdout/stderr en la tabla
    fd_table_[0] = stdin;
    fd_table_[1] = stdout;
    fd_table_[2] = stderr;
}

// ─────────────────────────────────────────────────────────────────────────────
// handle() — despacho principal
// ─────────────────────────────────────────────────────────────────────────────
bool Syscalls::handle() {
    uint32_t syscall_num = regs_[17]; // a7

    switch (syscall_num) {
        case  56: return sys_openat();
        case  57: return sys_close();
        case  62: return sys_lseek();
        case  63: return sys_read();
        case  64: return sys_write();
        case  80: return sys_fstat();
        case  93: return sys_exit();
        case  94: return sys_exit_group();
        case 169: return sys_gettimeofday();
        case 214: return sys_brk();
        default:  return sys_unknown();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────
std::string Syscalls::readString(uint32_t addr) {
    std::string result;
    while (true) {
        uint8_t* ptr = mmu_.rawPtr(addr++);
        if (!ptr || *ptr == '\0') break;
        result += (char)*ptr;
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// exit / exit_group
// ─────────────────────────────────────────────────────────────────────────────
bool Syscalls::sys_exit() {
    int code = (int32_t)a(0);
    std::cout << "\n[SYSCALL] exit(" << code << ")\n";
    return false;  // señal para detener el emulador
}

bool Syscalls::sys_exit_group() {
    return sys_exit();
}

// ─────────────────────────────────────────────────────────────────────────────
// write — imprime en stdout/stderr del host
// ─────────────────────────────────────────────────────────────────────────────
bool Syscalls::sys_write() {
    int      fd    = (int32_t)a(0);
    uint32_t buf   = a(1);
    uint32_t count = a(2);

    // Solo manejamos stdout y stderr directamente
    if (fd == 1 || fd == 2) {
        for (uint32_t i = 0; i < count; i++) {
            uint8_t* ptr = mmu_.rawPtr(buf + i);
            if (ptr) fputc(*ptr, (fd == 2) ? stderr : stdout);
        }
        fflush((fd == 2) ? stderr : stdout);
        ret() = count;
        return true;
    }

    // Otros fds — escribir al archivo abierto
    auto it = fd_table_.find(fd);
    if (it == fd_table_.end()) { ret() = (uint32_t)-1; return true; }

    uint32_t written = 0;
    for (uint32_t i = 0; i < count; i++) {
        uint8_t* ptr = mmu_.rawPtr(buf + i);
        if (ptr) { fputc(*ptr, it->second); written++; }
    }
    ret() = written;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// read
// ─────────────────────────────────────────────────────────────────────────────
bool Syscalls::sys_read() {
    int      fd    = (int32_t)a(0);
    uint32_t buf   = a(1);
    uint32_t count = a(2);

    auto it = fd_table_.find(fd);
    if (it == fd_table_.end()) { ret() = (uint32_t)-1; return true; }

    uint32_t nread = 0;
    for (uint32_t i = 0; i < count; i++) {
        int c = fgetc(it->second);
        if (c == EOF) break;
        uint8_t* ptr = mmu_.rawPtr(buf + i);
        if (ptr) { *ptr = (uint8_t)c; nread++; }
    }
    ret() = nread;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// openat — abre un archivo del host
// a0=dirfd (ignorado), a1=path, a2=flags, a3=mode
// ─────────────────────────────────────────────────────────────────────────────
bool Syscalls::sys_openat() {
    // dirfd en a0, path en a1
    uint32_t    path_addr = a(1);
    int         flags     = (int32_t)a(2);
    std::string path      = readString(path_addr);

    // Convertir flags de Linux a modo de fopen
    const char* mode = "rb";
    if (flags & 0x1)       mode = "rb+";  // O_WRONLY o O_RDWR
    if (flags & 0x40)      mode = "wb";   // O_CREAT | O_WRONLY
    if ((flags & 0x241) == 0x241) mode = "ab"; // O_APPEND

    FILE* f = fopen(path.c_str(), mode);
    if (!f) {
        std::cerr << "[SYSCALL] openat(\"" << path << "\") → ENOENT\n";
        ret() = (uint32_t)-1;
        return true;
    }

    int fd = next_fd_++;
    fd_table_[fd] = f;
    std::cout << "[SYSCALL] openat(\"" << path << "\") → fd=" << fd << "\n";
    ret() = (uint32_t)fd;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// close
// ─────────────────────────────────────────────────────────────────────────────
bool Syscalls::sys_close() {
    int fd = (int32_t)a(0);

    // No cerrar stdin/stdout/stderr
    if (fd <= 2) { ret() = 0; return true; }

    auto it = fd_table_.find(fd);
    if (it == fd_table_.end()) { ret() = (uint32_t)-1; return true; }

    fclose(it->second);
    fd_table_.erase(it);
    ret() = 0;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// lseek
// ─────────────────────────────────────────────────────────────────────────────
bool Syscalls::sys_lseek() {
    int     fd     = (int32_t)a(0);
    int32_t offset = (int32_t)a(1);
    int     whence = (int32_t)a(2);

    auto it = fd_table_.find(fd);
    if (it == fd_table_.end()) { ret() = (uint32_t)-1; return true; }

    int result = fseek(it->second, offset, whence);
    ret() = result == 0 ? (uint32_t)ftell(it->second) : (uint32_t)-1;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// fstat — Doom lo usa para saber el tamaño del WAD
// Escribe una struct stat simplificada en memoria del emulador
// ─────────────────────────────────────────────────────────────────────────────
bool Syscalls::sys_fstat() {
    int      fd      = (int32_t)a(0);
    uint32_t statbuf = a(1);

    auto it = fd_table_.find(fd);
    if (it == fd_table_.end()) { ret() = (uint32_t)-1; return true; }

    // Obtener tamaño del archivo
    long pos = ftell(it->second);
    fseek(it->second, 0, SEEK_END);
    long size = ftell(it->second);
    fseek(it->second, pos, SEEK_SET);

    // Escribir struct stat mínima en memoria del emulador
    // Solo st_size (offset 44 en la struct stat de Linux RV32)
    // Llenamos todo con ceros primero
    for (int i = 0; i < 88; i++) {
        uint8_t* ptr = mmu_.rawPtr(statbuf + i);
        if (ptr) *ptr = 0;
    }
    // st_size está en offset 44 (RV32 Linux ABI)
    mmu_.storeWord(statbuf + 44, (uint32_t)size);

    ret() = 0;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// brk — gestión del heap (malloc usa esto)
//
// Si a0 == 0 → devolver heap_ptr actual
// Si a0 != 0 → intentar setear heap_ptr a a0, devolver nuevo heap_ptr
// ─────────────────────────────────────────────────────────────────────────────
bool Syscalls::sys_brk() {
    uint32_t new_brk = a(0);

    if (new_brk == 0) {
        // Consulta: devolver heap actual
        if (mmu_.heap_ptr == 0)
            mmu_.heap_ptr = mmu_.heap_start;
        ret() = mmu_.heap_ptr;
        return true;
    }

    // Verificar que el nuevo brk sea válido
    uint32_t limit = mmu_.baseAddr() + mmu_.ramSize();
    if (new_brk >= mmu_.heap_start && new_brk < limit) {
        mmu_.heap_ptr = new_brk;
        ret() = mmu_.heap_ptr;
    } else {
        // Fallo — devolver brk actual sin cambiar
        ret() = mmu_.heap_ptr;
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// gettimeofday — necesario para el game loop de Doom
// Escribe struct timeval en memoria: { tv_sec, tv_usec }
// ─────────────────────────────────────────────────────────────────────────────
bool Syscalls::sys_gettimeofday() {
    uint32_t tv_addr = a(0);
    // a1 = timezone, ignorado

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    if (tv_addr != 0) {
        mmu_.storeWord(tv_addr,     (uint32_t)ts.tv_sec);          // tv_sec
        mmu_.storeWord(tv_addr + 4, (uint32_t)(ts.tv_nsec / 1000)); // tv_usec
    }

    ret() = 0;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// unknown — syscall no implementada
// Devuelve -ENOSYS (= -38) y sigue ejecutando
// ─────────────────────────────────────────────────────────────────────────────
bool Syscalls::sys_unknown() {
    uint32_t num = regs_[17];
    std::cerr << "[SYSCALL] no implementada: " << std::dec << num
              << " (a0=0x" << std::hex << a(0) << ")\n";
    ret() = (uint32_t)-38;  // -ENOSYS
    return true;
}