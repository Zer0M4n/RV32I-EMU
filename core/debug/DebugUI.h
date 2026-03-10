#pragma once

#include "Logger.h"
#include "MMU.h"

#include <functional>
#include <string>
#include <atomic>

// ─────────────────────────────────────────────────────────────────────────────
// DebugUI
//
// TUI interactiva con FTXUI que muestra:
//   • Panel izquierdo  — 32 registros (x0–x31) con alias ABI
//   • Panel central    — historial de instrucciones ejecutadas
//   • Panel derecho    — volcado de memoria alrededor del PC/SP
//
// Controles:
//   Space  — Step (ejecuta un ciclo)
//   R      — Toggle Run/Pause (ejecución continua)
//   Q/Esc  — Salir
// ─────────────────────────────────────────────────────────────────────────────
class DebugUI {
public:
    // stepFn  : ejecuta UN ciclo del emulador y devuelve false si halted
    // logger  : referencia al Logger del CPU
    // mmu     : referencia a la MMU para el volcado de memoria
    DebugUI(std::function<bool()> stepFn, Logger& logger, MMU& mmu);

    // Bloquea hasta que el usuario cierre la TUI
    void run();

private:
    std::function<bool()> stepFn_;
    Logger&               logger_;
    MMU&                  mmu_;
    std::atomic<bool>     running_  { false };
    std::atomic<bool>     halted_   { false };

    // Helpers de renderizado
    static const char* abiName(int reg);
};