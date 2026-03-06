#include <iostream>
#include <fstream>
#include <vector>
#include "MMU.h"
#include "CPU.h"

int main() {
    MMU memoria(2 * 1024 * 1024, 0x80000000); // <-- base addr
    CPU cpu(memoria);

    std::ifstream file("/home/developer/projects/RV32I-EMU/rv32ui-p-simple.bin", std::ios::binary);
    if (!file) { std::cerr << "Error\n"; return 1; }

    std::vector<uint8_t> buf(std::istreambuf_iterator<char>(file), {});
    
    memoria.loadBinary(buf, 0x80000000); // <-- carga en el lugar correcto

    while (!cpu.halted) {
        cpu.step();

        // Los tests escriben aquí para indicar éxito o fallo
        uint32_t tohost = memoria.loadWord(0x80001000);
        if (tohost != 0) {
            if (tohost == 1)
                std::cout << "PASS ✓\n";
            else
                std::cout << "FAIL ✗ caso: " << (tohost >> 1) << "\n";
            break;
        }
    }
}