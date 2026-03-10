cmake_minimum_required(VERSION 3.10)
project(RV32I-EMU)

set(CMAKE_CXX_STANDARD 17)

# ── Buscar FTXUI en el sistema ────────────────────────────────────────────────
# Como ya lo instalaste, CMake lo encontrará automáticamente
find_package(ftxui REQUIRED)

# ── Fuentes del emulador ──────────────────────────────────────────────────────
add_executable(rv32i-emu
    main.cpp
    core/CPU/CPU.cpp
    core/MMU/MMU.cpp
    core/Instructionset/Instructionset.cpp
    core/debug/DebugUI.cpp
)

# ── Includes ──────────────────────────────────────────────────────────────────
target_include_directories(rv32i-emu PRIVATE
    core/CPU
    core/MMU
    core/Instructionset
    core/debug
)

# ── Linkear FTXUI ─────────────────────────────────────────────────────────────
# Al usar find_package, las librerías se mantienen con el mismo nombre
target_link_libraries(rv32i-emu PRIVATE
    ftxui::screen
    ftxui::dom
    ftxui::component
)