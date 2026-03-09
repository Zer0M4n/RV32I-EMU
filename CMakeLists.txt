cmake_minimum_required(VERSION 3.10)
project(RV32I-EMU)

set(CMAKE_CXX_STANDARD 17)

# Agrega los archivos fuente
add_executable(rv32i-emu
    main.cpp
    core/CPU/CPU.cpp
    core/MMU/MMU.cpp
    core/Instructionset/Instructionset.cpp

)

# Incluye los directorios de encabezados
target_include_directories(rv32i-emu PRIVATE
    core/CPU
    core/MMU
    core/Instructionset
)