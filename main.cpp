#include <iostream>
#include <fstream>
#include <vector>
#include "MMU.h"
#include "CPU.h"
#include "DebugUI.h"

int main() {
    MMU memoria(2 * 1024 * 1024, 0x80000000);
    CPU cpu(memoria);

    std::ifstream file("/home/developer/projects/RV32I-EMU/test.bin", std::ios::binary);
    if (!file) { std::cerr << "Error abriendo binario\n"; return 1; }

    std::vector<uint8_t> buf(std::istreambuf_iterator<char>(file), {});
    memoria.loadBinary(buf, 0x80000000);

    // ── TUI interactiva ───────────────────────────────────────────────────
    DebugUI ui(
        [&]() -> bool {
            if (cpu.isHalted()) return false;

            bool ok = cpu.step();

            // Chequeo tohost (riscv-tests)
            uint32_t tohost = memoria.loadWord(0x80001000);
            if (tohost != 0) {
                if (tohost == 1)
                    std::cout << "PASS ✓\n";
                else
                    std::cout << "FAIL ✗ caso: " << (tohost >> 1) << "\n";
                return false; // detener
            }

            return ok;
        },
        cpu.getLogger(),
        memoria
    );

    ui.run();

    return 0;
}