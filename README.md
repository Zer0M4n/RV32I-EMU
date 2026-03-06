# RV32I-EMU

Emulador de la arquitectura RISC-V (RV32I) escrito en C++ desde cero, con fines de aprendizaje personal. El objetivo es entender cómo funciona una CPU a nivel de ciclo de instrucción: fetch, decode y execute.

---

## Estado actual

El emulador es capaz de cargar binarios ELF extraídos como `.bin`, ejecutarlos en memoria y reportar si el test pasó o falló usando la convención de `riscv-tests`.

### Instrucciones implementadas

| Grupo | Instrucciones |
|---|---|
| **LUI** | `LUI` |
| **AUIPC** | `AUIPC` |
| **OP-IMM** | `ADDI` |
| **LOAD** | `LB`, `LH`, `LW`, `LBU`, `LHU` |
| **STORE** | `SB`, `SH`, `SW` |
| **BRANCH** | `BEQ`, `BNE`, `BLT`, `BGE`, `BLTU`, `BGEU` |
| **JAL** | `JAL` |
| **JALR** | `JALR` |
| **SYSTEM** | `ECALL`, `EBREAK` |
| **MISC-MEM** | `FENCE` (no-op) |

### Tests pasados

| Test | Estado |
|---|---|
| `rv32ui-p-simple` | ✅ PASS |

---

## Estructura del proyecto

```
RV32I-EMU/
├── core/
│   ├── CPU/
│   │   ├── CPU.h       # Definición de la CPU
│   │   └── CPU.cpp     # Implementación del fetch/decode/execute
│   └── MMU/
│       ├── MMU.h       # Definición de la MMU
│       └── MMU.cpp     # Memoria plana con dirección base configurable
├── main.cpp            # Entry point: carga el binario y corre el emulador
├── CMakeLists.txt
└── *.bin               # Binarios de los tests de riscv-tests
```

---

## Cómo compilar

### Requisitos

- `cmake` >= 3.10
- `g++` con soporte C++17
- `riscv64-unknown-elf-gcc` (solo para generar los tests, no para compilar el emulador)

### Compilar el emulador

```bash
mkdir build && cd build
cmake ..
make
```

El binario queda en `build/rv32i-emu`.

### Ejecutar un test

Cambia la ruta del binario en `main.cpp` y recompila:

```cpp
std::ifstream file("/ruta/a/rv32ui-p-add.bin", std::ios::binary);
```

Luego:

```bash
cd build
make
./rv32i-emu
```

### Generar los binarios de los tests

```bash
# Clonar riscv-tests con sus submodulos
git clone --recurse-submodules https://github.com/riscv-software-src/riscv-tests
cd riscv-tests
autoconf && ./configure --prefix=$PWD/build
make isa XLEN=32

# Extraer todos los tests rv32ui-p como binarios planos
cd isa
for f in rv32ui-p-*; do
    [[ "$f" == *.dump ]] && continue
    riscv64-unknown-elf-objcopy -O binary "$f" "../../${f}.bin"
done
```

---

## Cómo funciona

### Ciclo de instrucción

Cada llamada a `cpu.step()` ejecuta un ciclo completo:

1. **Fetch**: Lee 4 bytes de memoria en la dirección del PC y avanza el PC en 4.
2. **Decode**: Extrae el opcode y los campos de la instrucción (rd, rs1, rs2, funct3, funct7, inmediatos).
3. **Execute**: Llama al handler correspondiente usando una tabla de punteros a funciones indexada por opcode.

### Memoria (MMU)

La MMU es un vector plano de bytes con una dirección base configurable (`0x80000000` por defecto). Todas las direcciones virtuales se traducen a índices físicos restando la base. Soporta accesos de 1, 2 y 4 bytes en formato little-endian.

### Detección de resultado en los tests

Los tests de `riscv-tests` no usan ECALL para indicar éxito o fallo de forma estándar. En cambio, almacenan el resultado en el registro `gp` (x3):

- `gp == 1` → todos los casos del test pasaron.
- `gp == N` (N par) → el caso `N/2` falló.

El emulador lee este registro al recibir el ECALL final e imprime el resultado.

---

## Próximos pasos

### Corto plazo — completar RV32I

El objetivo inmediato es pasar los 40 tests de `rv32ui-p-*`.

- [ ] Implementar `OP` (opcode `0x33`): instrucciones R-type completas (`ADD`, `SUB`, `AND`, `OR`, `XOR`, `SLL`, `SRL`, `SRA`, `SLT`, `SLTU`)
- [ ] Completar `OP-IMM` (opcode `0x13`): `SLTI`, `SLTIU`, `XORI`, `ORI`, `ANDI`, `SLLI`, `SRLI`, `SRAI`
- [ ] Pasar todos los tests `rv32ui-p-*`

### Mediano plazo — calidad del emulador

Una vez que RV32I esté completo:

- [ ] Aceptar el binario a ejecutar como argumento de línea de comandos en lugar de tenerlo hardcodeado
- [ ] Agregar un modo silencioso (sin debug por instrucción) para correr los tests más rápido
- [ ] Script que corra todos los tests automáticamente e imprima un resumen de cuántos pasan
- [ ] Manejo de excepciones básico (instrucción ilegal, acceso a memoria inválido)

### Largo plazo — extensiones opcionales

Para quienes quieran ir más allá de RV32I base:

- [ ] Extensión **M** (multiplicación y división): `MUL`, `DIV`, `REM` y sus variantes
- [ ] CSRs reales: implementar los registros de control básicos (`mstatus`, `mepc`, `mcause`, `mtvec`)
- [ ] Manejo real de traps y excepciones
- [ ] Soporte para **RV64I** (registros de 64 bits, nuevas instrucciones `W`)

---

## Referencias

- [Especificación oficial RISC-V](https://riscv.org/technical/specifications/)
- [riscv-tests en GitHub](https://github.com/riscv-software-src/riscv-tests)
- [RISC-V ISA Reference Card](https://github.com/jameslzhu/riscv-card)
- [rv32i unprivileged spec (PDF)](https://github.com/riscv/riscv-isa-manual/releases)

---

## Licencia

Ver `LICENSE`.