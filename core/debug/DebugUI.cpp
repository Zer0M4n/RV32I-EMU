#include "DebugUI.h"

#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"

#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>

using namespace ftxui;

// ── Nombres ABI de los registros ─────────────────────────────────────────────
const char* DebugUI::abiName(int r) {
    static const char* names[32] = {
        "zero","ra","sp","gp","tp",
        "t0","t1","t2",
        "s0","s1",
        "a0","a1","a2","a3","a4","a5","a6","a7",
        "s2","s3","s4","s5","s6","s7","s8","s9","s10","s11",
        "t3","t4","t5","t6"
    };
    return (r >= 0 && r < 32) ? names[r] : "??";
}

// ── Constructor ───────────────────────────────────────────────────────────────
DebugUI::DebugUI(std::function<bool()> stepFn, Logger& logger, MMU& mmu)
    : stepFn_(std::move(stepFn)), logger_(logger), mmu_(mmu) {}

// ── run() — bucle principal de la TUI ────────────────────────────────────────
void DebugUI::run() {
    auto screen = ScreenInteractive::Fullscreen();

    // ── Renderer ──────────────────────────────────────────────────────────
    auto renderer = Renderer([&] {
        CpuSnapshot snap = logger_.latest();

        // ── Panel: Registros ──────────────────────────────────────────────
        Elements reg_rows;
        for (int i = 0; i < 32; i++) {
            std::ostringstream val;
            val << "0x" << std::hex << std::setw(8) << std::setfill('0') << snap.regs[i];

            bool changed = (snap.regs[i] != 0);
            auto row = hbox({
                text(std::string("x") + std::to_string(i))
                    | color(Color::GrayDark) | size(WIDTH, EQUAL, 4),
                text(std::string(" ") + abiName(i))
                    | color(Color::Cyan) | size(WIDTH, EQUAL, 6),
                text(" " + val.str())
                    | (changed ? color(Color::GreenLight) : color(Color::GrayDark))
            });
            reg_rows.push_back(row);
        }

        auto reg_panel = window(
            hbox({ text(" ") , text("REGISTERS") | bold | color(Color::Yellow), text(" ") }),
            vbox(reg_rows) | frame
        ) | size(WIDTH, EQUAL, 28);

        // ── Panel: Historial de instrucciones ─────────────────────────────
        auto history = logger_.history();
        Elements hist_rows;
        int start = (history.size() > 20) ? (int)history.size() - 20 : 0;
        for (int i = start; i < (int)history.size(); i++) {
            const auto& h = history[i];
            bool is_last = (i == (int)history.size() - 1);

            std::ostringstream pc_str;
            pc_str << "0x" << std::hex << std::setw(8) << std::setfill('0') << h.pc;

            std::ostringstream inst_str;
            inst_str << "0x" << std::hex << std::setw(8) << std::setfill('0') << h.inst;

            auto row = hbox({
                text(pc_str.str())  | color(is_last ? Color::YellowLight : Color::GrayDark),
                text("  "),
                text(inst_str.str()) | color(Color::GrayLight),
                text("  "),
                text(h.mnemonic)    | (is_last ? (bold | color(Color::GreenLight))
                                               : color(Color::White))
            });
            hist_rows.push_back(row);
        }
        if (hist_rows.empty())
            hist_rows.push_back(text("(sin instrucciones aún)") | color(Color::GrayDark));

        auto hist_panel = window(
            hbox({ text(" "), text("INSTRUCTION TRACE") | bold | color(Color::Yellow), text(" ") }),
            vbox(hist_rows) | frame
        ) | flex;

        // ── Panel: Volcado de memoria alrededor del PC ────────────────────
        Elements mem_rows;
        uint32_t base = snap.pc & ~0xFu; // alinear a 16 bytes
        if (base >= 0x10) base -= 0x10;  // mostrar 1 fila antes

        for (int row = 0; row < 8; row++) {
            uint32_t addr = base + row * 16;
            std::ostringstream addr_str;
            addr_str << "0x" << std::hex << std::setw(8) << std::setfill('0') << addr;

            Elements bytes_elem;
            bytes_elem.push_back(text(addr_str.str()) | color(Color::Cyan));
            bytes_elem.push_back(text("  "));

            for (int b = 0; b < 16; b++) {
                uint8_t byte_val = 0;
                try { byte_val = mmu_.loadByte(addr + b); } catch (...) {}

                std::ostringstream bstr;
                bstr << std::hex << std::setw(2) << std::setfill('0') << (int)byte_val;

                bool is_pc_byte = ((addr + b) >= snap.pc && (addr + b) < snap.pc + 4);
                bytes_elem.push_back(
                    text(bstr.str() + " ")
                    | (is_pc_byte ? (bold | color(Color::YellowLight)) : color(Color::GrayLight))
                );
            }
            mem_rows.push_back(hbox(bytes_elem));
        }

        auto mem_panel = window(
            hbox({ text(" "), text("MEMORY @ PC") | bold | color(Color::Yellow), text(" ") }),
            vbox(mem_rows) | frame
        );

        // ── Barra de estado ───────────────────────────────────────────────
        std::ostringstream pc_disp;
        pc_disp << " PC: 0x" << std::hex << std::setw(8) << std::setfill('0') << snap.pc;

        auto status = hbox({
            text(pc_disp.str())        | bold | color(Color::YellowLight),
            text("  │  "),
            text(snap.mnemonic.empty() ? "---" : snap.mnemonic) | color(Color::GreenLight),
            text("  │  "),
            text(running_ ? "▶  RUNNING" : "⏸  PAUSED")
                | bold | color(running_ ? Color::Green : Color::RedLight),
            text(halted_  ? "  │  ■ HALTED" : "")
                | bold | color(Color::Red),
            text("  │  [Space] Step  [R] Run/Pause  [Q] Quit")
                | color(Color::GrayDark),
        });

        // ── Layout final ──────────────────────────────────────────────────
        return vbox({
            hbox({ reg_panel, hist_panel, vbox({ mem_panel }) | flex }) | flex,
            separator(),
            status | bgcolor(Color::Black),
        });
    });

    // ── Manejo de teclado ─────────────────────────────────────────────────
    auto component = CatchEvent(renderer, [&](Event event) {
        if (event == Event::Character('q') || event == Event::Character('Q')
            || event == Event::Escape) {
            running_ = false;
            screen.ExitLoopClosure()();
            return true;
        }
        if (event == Event::Character(' ')) {
            running_ = false;
            if (!halted_) {
                bool ok = stepFn_();
                if (!ok) halted_ = true;
            }
            return true;
        }
        if (event == Event::Character('r') || event == Event::Character('R')) {
            if (!halted_) running_ = !running_;
            return true;
        }
        return false;
    });

    // ── Hilo de ejecución continua ────────────────────────────────────────
    std::thread run_thread([&] {
        while (!halted_) {
            if (running_) {
                bool ok = stepFn_();
                if (!ok) {
                    halted_  = true;
                    running_ = false;
                }
                screen.PostEvent(Event::Custom);  // fuerza redibujado
                std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60fps
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
        }
    });

    screen.Loop(component);
    running_ = false;
    run_thread.join();
}