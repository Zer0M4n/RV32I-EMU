#ifndef MMU_H
#define MMU_H

#include <iostream>
#include <vector>
#include <cstdint>

class MMU {
private:
    std::vector<uint8_t> RAM;
    uint32_t base_addr;

public:
    // ── tohost / fromhost (riscv-tests) ──────────────────────────────────
    uint32_t tohost_addr   = 0;
    uint32_t fromhost_addr = 0;

    // ── Heap para sbrk / malloc ───────────────────────────────────────────
    // heap_start se fija en main después de cargar el ELF
    // heap_ptr avanza con cada sbrk()
    uint32_t heap_start = 0;
    uint32_t heap_ptr   = 0;

    // ── Acceso raw a la RAM (para framebuffer SDL2) ───────────────────────
    uint8_t* rawPtr(uint32_t addr) {
        uint32_t p = addr - base_addr;
        if (p >= RAM.size()) return nullptr;
        return RAM.data() + p;
    }

    uint32_t ramSize()  const { return (uint32_t)RAM.size(); }
    uint32_t baseAddr() const { return base_addr; }

    // ── Loads ─────────────────────────────────────────────────────────────
    uint8_t  loadByte    (uint32_t addr);
    uint16_t loadHalfWolf(uint32_t addr);
    uint32_t loadWord    (uint32_t addr);

    // ── Stores ────────────────────────────────────────────────────────────
    void storeByte    (uint32_t addr, uint8_t  val);
    void storeHalfWolf(uint32_t addr, uint16_t val);
    void storeWord    (uint32_t addr, uint32_t val);

    // ── Carga binario en memoria ──────────────────────────────────────────
    void loadBinary(const std::vector<uint8_t>& bin, uint32_t base);

    // ── tohost helpers ────────────────────────────────────────────────────
    void setTohostAddr  (uint32_t addr) { tohost_addr   = addr; }
    void setFromhostAddr(uint32_t addr) { fromhost_addr = addr; }

    uint32_t getTohost() {
        if (tohost_addr == 0) return 0;
        return loadWord(tohost_addr);
    }

    // ── sbrk ─────────────────────────────────────────────────────────────
    // Devuelve el heap_ptr anterior (comportamiento POSIX)
    // Devuelve 0xFFFFFFFF si se sale de rango (= -1)
    uint32_t sbrk(int32_t increment) {
        uint32_t old     = heap_ptr;
        uint32_t new_ptr = heap_ptr + (uint32_t)increment;
        if (new_ptr < base_addr || new_ptr >= base_addr + (uint32_t)RAM.size())
            return 0xFFFFFFFF;
        heap_ptr = new_ptr;
        return old;
    }

    explicit MMU(size_t size, uint32_t base = 0x80000000);
    ~MMU();
};

#endif