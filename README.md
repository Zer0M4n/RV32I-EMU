# RV32I-EMU

A RISC-V (RV32I) emulator written in C++ from scratch, built for personal learning. The goal is to understand how a CPU works at the instruction cycle level: fetch, decode, and execute.

---

## Current Status

The emulator can load ELF binaries extracted as `.bin` files, execute them in memory, and report whether a test passed or failed using the `riscv-tests` convention.

### Implemented Instructions

| Group | Instructions |
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

### Passing Tests

| Test | Status |
|---|---|
| `rv32ui-p-simple` | ✅ PASS |

---

## Project Structure

```
RV32I-EMU/
├── core/
│   ├── CPU/
│   │   ├── CPU.h       # CPU class definition
│   │   └── CPU.cpp     # Fetch / decode / execute implementation
│   └── MMU/
│       ├── MMU.h       # MMU class definition
│       └── MMU.cpp     # Flat memory with configurable base address
├── main.cpp            # Entry point: loads the binary and runs the emulator
├── CMakeLists.txt
└── *.bin               # riscv-tests binaries
```

---

## How to Build

### Requirements

- `cmake` >= 3.10
- `g++` with C++17 support
- `riscv64-unknown-elf-gcc` (only needed to generate test binaries, not to build the emulator)

### Build the emulator

```bash
mkdir build && cd build
cmake ..
make
```

The binary will be at `build/rv32i-emu`.

### Run a test

Change the binary path in `main.cpp` and recompile:

```cpp
std::ifstream file("/path/to/rv32ui-p-add.bin", std::ios::binary);
```

Then:

```bash
cd build
make
./rv32i-emu
```

### Generate test binaries

```bash
# Clone riscv-tests with its submodules
git clone --recurse-submodules https://github.com/riscv-software-src/riscv-tests
cd riscv-tests
autoconf && ./configure --prefix=$PWD/build
make isa XLEN=32

# Extract all rv32ui-p tests as flat binaries
cd isa
for f in rv32ui-p-*; do
    [[ "$f" == *.dump ]] && continue
    riscv64-unknown-elf-objcopy -O binary "$f" "../../${f}.bin"
done
```

---

## How It Works

### Instruction Cycle

Each call to `cpu.step()` executes a full cycle:

1. **Fetch**: Reads 4 bytes from memory at the PC address and advances the PC by 4.
2. **Decode**: Extracts the opcode and instruction fields (rd, rs1, rs2, funct3, funct7, immediates).
3. **Execute**: Calls the corresponding handler using a function pointer table indexed by opcode.

### Memory (MMU)

The MMU is a flat byte vector with a configurable base address (`0x80000000` by default). All virtual addresses are translated to physical indices by subtracting the base. Supports 1, 2, and 4-byte little-endian accesses.

### Test Result Detection

The `riscv-tests` tests do not use ECALL to signal pass/fail in a standard way. Instead, they store the result in the `gp` register (x3):

- `gp == 1` → all test cases passed.
- `gp == N` (N even) → test case `N/2` failed.

The emulator reads this register on the final ECALL and prints the result.

---

## Roadmap

### Short Term — Complete RV32I

The immediate goal is to pass all 40 `rv32ui-p-*` tests.

- [ ] Implement `OP` (opcode `0x33`): full R-type instructions (`ADD`, `SUB`, `AND`, `OR`, `XOR`, `SLL`, `SRL`, `SRA`, `SLT`, `SLTU`)
- [ ] Complete `OP-IMM` (opcode `0x13`): `SLTI`, `SLTIU`, `XORI`, `ORI`, `ANDI`, `SLLI`, `SRLI`, `SRAI`
- [ ] Pass all `rv32ui-p-*` tests

### Medium Term — Emulator Quality

Once RV32I is complete:

- [ ] Accept the binary to run as a command-line argument instead of hardcoding the path
- [ ] Add a silent mode (no per-instruction debug output) for faster test runs
- [ ] Script to run all tests automatically and print a pass/fail summary
- [ ] Basic exception handling (illegal instruction, invalid memory access)

### Long Term — Optional Extensions

For those who want to go beyond the base RV32I:

- [ ] **M extension** (multiply/divide): `MUL`, `DIV`, `REM` and their variants
- [ ] Real CSRs: implement basic control registers (`mstatus`, `mepc`, `mcause`, `mtvec`)
- [ ] Proper trap and exception handling
- [ ] **RV64I** support (64-bit registers, new `W`-suffix instructions)

---

## References

- [Official RISC-V Specification](https://riscv.org/technical/specifications/)
- [riscv-tests on GitHub](https://github.com/riscv-software-src/riscv-tests)
- [RISC-V ISA Reference Card](https://github.com/jameslzhu/riscv-card)
- [RV32I Unprivileged Spec (PDF)](https://github.com/riscv/riscv-isa-manual/releases)

---

## License

See `LICENSE`.