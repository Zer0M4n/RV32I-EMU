#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <iomanip>
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <elf.h>

#include "MMU.h"
#include "CPU.h"
#include "DebugUI.h"

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// loadELF — carga segmentos y encuentra tohost/fromhost
// ─────────────────────────────────────────────────────────────────────────────
uint32_t loadELF(const std::string& path, MMU& mmu, bool verbose = true) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("No se puede abrir: " + path);

    std::vector<uint8_t> buf(std::istreambuf_iterator<char>(f), {});

    if (buf.size() < sizeof(Elf32_Ehdr))
        throw std::runtime_error("Archivo demasiado pequeño");

    auto* ehdr = reinterpret_cast<Elf32_Ehdr*>(buf.data());
    if (memcmp(ehdr->e_ident, ELFMAG, 4) != 0)
        throw std::runtime_error("No es un ELF válido: " + path);
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32)
        throw std::runtime_error("Solo ELF32 soportado (RV32I)");

    // ── Cargar segmentos PT_LOAD ──────────────────────────────────────────
    auto* phdrs = reinterpret_cast<Elf32_Phdr*>(buf.data() + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type != PT_LOAD || phdrs[i].p_filesz == 0) continue;
        std::vector<uint8_t> seg(
            buf.data() + phdrs[i].p_offset,
            buf.data() + phdrs[i].p_offset + phdrs[i].p_filesz);
        mmu.loadBinary(seg, phdrs[i].p_paddr);
        if (verbose)
            std::cout << "[ELF] Segmento @ 0x" << std::hex << phdrs[i].p_paddr
                      << "  (" << std::dec << phdrs[i].p_filesz << " bytes)\n";
    }

    // ── Buscar tohost / fromhost en .symtab ───────────────────────────────
    auto* shdrs = reinterpret_cast<Elf32_Shdr*>(buf.data() + ehdr->e_shoff);
    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdrs[i].sh_type != SHT_SYMTAB) continue;
        int   nsyms  = shdrs[i].sh_size / sizeof(Elf32_Sym);
        auto* syms   = reinterpret_cast<Elf32_Sym*>(buf.data() + shdrs[i].sh_offset);
        const char* strtab = reinterpret_cast<const char*>(
            buf.data() + shdrs[shdrs[i].sh_link].sh_offset);
        for (int j = 0; j < nsyms; j++) {
            std::string name = strtab + syms[j].st_name;
            if (name == "tohost") {
                mmu.setTohostAddr(syms[j].st_value);
                if (verbose)
                    std::cout << "[ELF] tohost   @ 0x" << std::hex << syms[j].st_value << "\n";
            }
            if (name == "fromhost") {
                mmu.setFromhostAddr(syms[j].st_value);
                if (verbose)
                    std::cout << "[ELF] fromhost @ 0x" << std::hex << syms[j].st_value << "\n";
            }
        }
    }

    if (verbose)
        std::cout << "[ELF] Entry point: 0x" << std::hex << ehdr->e_entry << "\n\n";

    return ehdr->e_entry;
}

// ─────────────────────────────────────────────────────────────────────────────
// runTest — ejecuta un test y devuelve true si pasó
// Usado tanto por modo batch como por modo TUI
// ─────────────────────────────────────────────────────────────────────────────
bool runTest(const std::string& test_path, bool verbose) {
    std::string test_name = fs::path(test_path).filename().string();

    MMU memoria(64 * 1024 * 1024, 0x80000000);
    CPU cpu(memoria);

    uint32_t entry = loadELF(test_path, memoria, verbose);
    cpu.setPC(entry);

    bool test_done   = false;
    bool test_passed = false;

    auto stepFn = [&]() -> bool {
        if (cpu.isHalted()) return false;
        bool ok = cpu.step();
        if (!test_done && memoria.tohost_addr != 0) {
            uint32_t th = memoria.getTohost();
            if (th != 0) {
                test_done   = true;
                test_passed = (th == 1);
                if (memoria.fromhost_addr != 0)
                    memoria.storeWord(memoria.fromhost_addr, 1);
                return false;
            }
        }
        return ok;
    };

    while (stepFn()) {}  // loop simple sin TUI

    return test_passed;
}

// ─────────────────────────────────────────────────────────────────────────────
// collectTests — lista todos los ELFs del directorio ordenados
// ─────────────────────────────────────────────────────────────────────────────
std::vector<fs::path> collectTests(const std::string& tests_dir) {
    if (!fs::exists(tests_dir))
        throw std::runtime_error("Directorio no existe: " + tests_dir);

    std::vector<fs::path> tests;
    for (const auto& entry : fs::directory_iterator(tests_dir))
        if (entry.is_regular_file() && entry.path().extension().empty())
            tests.push_back(entry.path());

    if (tests.empty())
        throw std::runtime_error("No se encontraron tests en: " + tests_dir);

    std::sort(tests.begin(), tests.end());
    return tests;
}

// ─────────────────────────────────────────────────────────────────────────────
// selectTest — menú interactivo
// ─────────────────────────────────────────────────────────────────────────────
std::string selectTest(const std::string& tests_dir) {
    auto tests = collectTests(tests_dir);

    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════╗\n";
    std::cout << "║     RV32I Emulator — Selección de test   ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n\n";
    std::cout << "  Directorio: " << tests_dir << "\n\n";

    for (size_t i = 0; i < tests.size(); i++)
        std::cout << "  [" << std::setw(3) << (i + 1) << "]  "
                  << tests[i].filename().string() << "\n";

    int choice = 0;
    std::cout << "\nElige el número del test (1-" << tests.size() << "): ";
    while (!(std::cin >> choice) || choice < 1 || choice > (int)tests.size()) {
        std::cin.clear();
        std::cin.ignore(1024, '\n');
        std::cout << "Número inválido, intenta de nuevo: ";
    }

    return tests[choice - 1].string();
}

// ─────────────────────────────────────────────────────────────────────────────
// modoBatch — corre todos los tests sin TUI y muestra resumen
// ─────────────────────────────────────────────────────────────────────────────
int modoBatch(const std::string& tests_dir) {
    std::vector<fs::path> tests;
    try {
        tests = collectTests(tests_dir);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }

    int passed = 0, failed = 0;
    std::vector<std::string> failures;

    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════╗\n";
    std::cout << "║     RV32I Emulator — Batch Test Suite    ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n\n";

    for (const auto& t : tests) {
        std::string name = t.filename().string();
        std::cout << std::left << std::setw(30) << name << " ... ";
        std::cout.flush();

        bool ok = false;
        try {
            ok = runTest(t.string(), false);  // verbose=false: sin prints de ELF
        } catch (const std::exception& e) {
            std::cout << "ERROR (" << e.what() << ")\n";
            failures.push_back(name + " [excepción]");
            failed++;
            continue;
        }

        if (ok) {
            std::cout << "PASS ✓\n";
            passed++;
        } else {
            std::cout << "FAIL ✗\n";
            failures.push_back(name);
            failed++;
        }
    }

    // ── Resumen ───────────────────────────────────────────────────────────
    std::cout << "\n══════════════════════════════════════════\n";
    std::cout << "  Total:  " << (passed + failed) << "\n";
    std::cout << "  PASS:   " << passed << "\n";
    std::cout << "  FAIL:   " << failed << "\n";

    if (!failures.empty()) {
        std::cout << "\n  Tests fallidos:\n";
        for (const auto& f : failures)
            std::cout << "    ✗ " << f << "\n";
    }

    std::cout << "══════════════════════════════════════════\n\n";

    return failed == 0 ? 0 : 1;
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    std::string tests_dir = (argc >= 2) ? argv[1] : "./core/test";

    // ── Modo batch: ./rv32i-emu ./core/test --batch ───────────────────────
    if (argc >= 3 && std::string(argv[2]) == "--batch")
        return modoBatch(tests_dir);

    // ── Modo interactivo con TUI ──────────────────────────────────────────
    std::string test_path;
    try {
        test_path = selectTest(tests_dir);
    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] " << e.what() << "\n";
        std::cerr << "Uso: " << argv[0] << " [directorio]           — modo interactivo\n";
        std::cerr << "     " << argv[0] << " [directorio] --batch   — correr todos\n\n";
        return 1;
    }

    std::string test_name = fs::path(test_path).filename().string();
    std::cout << "\nCargando: " << test_path << "\n";

    MMU memoria(64 * 1024 * 1024, 0x80000000);
    CPU cpu(memoria);

    uint32_t entry = 0;
    try {
        entry = loadELF(test_path, memoria, true);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }
    cpu.setPC(entry);

    bool test_done   = false;
    bool test_passed = false;

    std::unique_ptr<DebugUI> ui;

    auto stepFn = [&]() -> bool {
        if (cpu.isHalted()) return false;

        bool ok = cpu.step();

        if (!test_done && memoria.tohost_addr != 0) {
            uint32_t th = memoria.getTohost();
            if (th != 0) {
                test_done = true;
                if (th == 1) {
                    test_passed = true;
                    if (ui) ui->setTestResult(TestResult::PASS);
                } else if (th & 1) {
                    test_passed = false;
                    if (ui) ui->setTestResult(TestResult::FAIL, th >> 1);
                }
                if (memoria.fromhost_addr != 0)
                    memoria.storeWord(memoria.fromhost_addr, 1);
                return false;
            }
        }

        return ok;
    };

    ui = std::make_unique<DebugUI>(stepFn, cpu.getLogger(), memoria, test_name);
    ui->run();

    std::cout << "\n══════════════════════════════════════\n";
    std::cout << "  Test:      " << test_name << "\n";
    if (test_done)
        std::cout << "  Resultado: " << (test_passed ? "PASS ✓" : "FAIL ✗") << "\n";
    else
        std::cout << "  Resultado: sin datos (tohost no escrito)\n";
    std::cout << "══════════════════════════════════════\n\n";

    return test_done ? (test_passed ? 0 : 1) : 2;
}