#pragma once

#include "Logger.h"
#include "MMU.h"

#include <functional>
#include <string>
#include <atomic>

// ─────────────────────────────────────────────────────────────────────────────
// TestResult — estado del test riscv-tests
// ─────────────────────────────────────────────────────────────────────────────
enum class TestResult {
    RUNNING,   // todavía ejecutando
    PASS,      // tohost == 1
    FAIL       // tohost impar != 1
};

// ─────────────────────────────────────────────────────────────────────────────
// DebugUI
//
// TUI interactiva con FTXUI que muestra:
//   • Panel izquierdo  — 32 registros (x0–x31) con alias ABI
//   • Panel central    — historial de instrucciones ejecutadas
//   • Panel derecho    — volcado de memoria alrededor del PC
//   • Barra inferior   — PC, mnemonic, estado, resultado del test
//
// Controles:
//   Space  — Step (ejecuta un ciclo)
//   R      — Toggle Run/Pause (ejecución continua)
//   Q/Esc  — Salir
// ─────────────────────────────────────────────────────────────────────────────
class DebugUI {
public:
    // stepFn    : ejecuta UN ciclo del emulador y devuelve false si halted
    // logger    : referencia al Logger del CPU
    // mmu       : referencia a la MMU para el volcado de memoria
    // test_name : nombre del archivo del test (para mostrarlo en la TUI)
    DebugUI(std::function<bool()> stepFn,
            Logger&               logger,
            MMU&                  mmu,
            const std::string&    test_name = "");

    // Bloquea hasta que el usuario cierre la TUI
    void run();

    // Llamado desde main cuando se detecta tohost != 0
    void setTestResult(TestResult result, uint32_t fail_case = 0);

private:
    std::function<bool()> stepFn_;
    Logger&               logger_;
    MMU&                  mmu_;
    std::string           test_name_;

    std::atomic<bool>       running_ { false };
    std::atomic<bool>       halted_  { false };

    // Resultado del test — protegido con mutex para acceso desde hilo
    mutable std::mutex  result_mtx_;
    TestResult          test_result_  { TestResult::RUNNING };
    uint32_t            fail_case_    { 0 };

    static const char* abiName(int reg);
};