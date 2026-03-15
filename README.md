# RV32I-EMU

A RISC-V (RV32IM) emulator written in C++ from scratch, built for personal learning. The goal is to understand how a CPU works at the instruction cycle level: fetch, decode, and execute — and eventually run real programs and a small OS.

---

## Current Status

The emulator loads ELF binaries, executes them in memory, and reports pass/fail using the `riscv-tests` convention via `tohost`. It includes an interactive TUI debugger (FTXUI) with register view, instruction trace, memory dump, and real-time test result panel.

### Implemented Instructions

| Group | Instructions |
|---|---|
| **LUI / AUIPC** | `LUI`, `AUIPC` |
| **OP-IMM** | `ADDI`, `SLTI`, `SLTIU`, `XORI`, `ORI`, `ANDI`, `SLLI`, `SRLI`, `SRAI` |
| **OP (RV32I)** | `ADD`, `SUB`, `SLL`, `SLT`, `SLTU`, `XOR`, `SRL`, `SRA`, `OR`, `AND` |
| **OP (RV32M)** | `MUL`, `MULH`, `MULHSU`, `MULHU`, `DIV`, `DIVU`, `REM`, `REMU` |
| **LOAD** | `LB`, `LH`, `LW`, `LBU`, `LHU` |
| **STORE** | `SB`, `SH`, `SW` |
| **BRANCH** | `BEQ`, `BNE`, `BLT`, `BGE`, `BLTU`, `BGEU` |
| **JAL / JALR** | `JAL`, `JALR` |
| **SYSTEM** | `ECALL`, `EBREAK`, `MRET`, `CSRRW`, `CSRRS`, `CSRRC` (+ `I` variants) |
| **MISC-MEM** | `FENCE` (no-op) |

### CSRs Implemented

`mstatus`, `misa` (read-only), `medeleg`, `mideleg`, `mie`, `mtvec`, `mscratch`, `mepc`, `mcause`, `satp`, `pmpcfg0`, `pmpaddr0`, `mhartid` (read-only)

### Passing Tests

#### RV32UI — Base Integer
| Test | Status |
|---|---|
| `rv32ui-p-simple` | ✅ PASS |
| `rv32ui-p-add` | ✅ PASS |
| `rv32ui-p-addi` | ✅ PASS |
| `rv32ui-p-and` | ✅ PASS |
| `rv32ui-p-andi` | ✅ PASS |
| `rv32ui-p-auipc` | ✅ PASS |
| `rv32ui-p-beq` | ✅ PASS |
| `rv32ui-p-bge` | ✅ PASS |
| `rv32ui-p-bgeu` | ✅ PASS |
| `rv32ui-p-blt` | ✅ PASS |
| `rv32ui-p-bltu` | ✅ PASS |
| `rv32ui-p-bne` | ✅ PASS |
| `rv32ui-p-fence_i` | ✅ PASS |
| `rv32ui-p-jal` | ✅ PASS |
| `rv32ui-p-jalr` | ✅ PASS |
| `rv32ui-p-lb` | ✅ PASS |
| `rv32ui-p-lbu` | ✅ PASS |
| `rv32ui-p-lh` | ✅ PASS |
| `rv32ui-p-lhu` | ✅ PASS |
| `rv32ui-p-lui` | ✅ PASS |
| `rv32ui-p-lw` | ✅ PASS |
| `rv32ui-p-or` | ✅ PASS |
| `rv32ui-p-ori` | ✅ PASS |
| `rv32ui-p-sb` | ✅ PASS |
| `rv32ui-p-sh` | ✅ PASS |
| `rv32ui-p-sll` | ✅ PASS |
| `rv32ui-p-slli` | ✅ PASS |
| `rv32ui-p-slt` | ✅ PASS |
| `rv32ui-p-slti` | ✅ PASS |
| `rv32ui-p-sltiu` | ✅ PASS |
| `rv32ui-p-sltu` | ✅ PASS |
| `rv32ui-p-sra` | ✅ PASS |
| `rv32ui-p-srai` | ✅ PASS |
| `rv32ui-p-srl` | ✅ PASS |
| `rv32ui-p-srli` | ✅ PASS |
| `rv32ui-p-sub` | ✅ PASS |
| `rv32ui-p-sw` | ✅ PASS |
| `rv32ui-p-xor` | ✅ PASS |
| `rv32ui-p-xori` | ✅ PASS |

#### RV32UM — Multiply/Divide Extension
| Test | Status |
|---|---|
| `rv32um-p-mul` | ✅ PASS |
| `rv32um-p-mulh` | ✅ PASS |
| `rv32um-p-mulhsu` | ✅ PASS |
| `rv32um-p-mulhu` | ✅ PASS |
| `rv32um-p-div` | ✅ PASS |
| `rv32um-p-divu` | ✅ PASS |
| `rv32um-p-rem` | ✅ PASS |
| `rv32um-p-remu` | ✅ PASS |

**47 / 47 tests passing.**

---

## Project Structure

```
RV32I-EMU/
├── core/
│   ├── CPU/
│   │   ├── CPU.h              # CPU class definition
│   │   └── CPU.cpp            # Fetch / decode / execute loop
│   ├── MMU/
│   │   ├── MMU.h              # MMU class definition
│   │   └── MMU.cpp            # Flat memory, tohost detection
│   ├── InstructionSet/
│   │   ├── Instructionset.h   # Base class with opcode dispatch table
│   │   └── Instructionset.cpp # All instruction handlers (RV32I + RV32M)
│   ├── Logger.h               # Thread-safe snapshot logger
│   ├── DebugUI.h / .cpp       # FTXUI interactive debugger
│   └── test/
│       ├── test-ui/           # rv32ui-p-* binaries
│       └── test-m/            # rv32um-p-* binaries
├── main.cpp                   # Entry point: ELF loader, batch mode, TUI
└── CMakeLists.txt
```

---

## How to Build

### Requirements

- `cmake` >= 3.10
- `g++` with C++17 support
- `ftxui` (for the debug TUI)
- `riscv64-unknown-elf-gcc` (only to regenerate test binaries)

### Build

```bash
mkdir build && cd build
cmake ..
make
```

### Run a single test (TUI mode)

```bash
./build/rv32i-emu core/test/test-ui/rv32ui-p-add
```

### Run all tests (batch mode)

```bash
./build/rv32i-emu core/test/test-ui --batch
./build/rv32i-emu core/test/test-m  --batch
```

### Generate test binaries

```bash
git clone --recurse-submodules https://github.com/riscv-software-src/riscv-tests
cd riscv-tests
autoconf && ./configure --prefix=$PWD/build
make isa XLEN=32
```

---

## How It Works

### Instruction Cycle

Each `cpu.step()` runs a full fetch–decode–execute cycle:

1. **Fetch** — reads 4 bytes from memory at PC, advances PC by 4.
2. **Decode** — extracts opcode, rd, rs1, rs2, funct3, funct7, immediates.
3. **Execute** — dispatches via a 128-entry function pointer table indexed by opcode.

### RV32M Dispatch

The M extension shares opcode `0x33` with base R-type instructions. Dispatch is done inside `OP()` by checking `funct7 == 0x01` before the RV32I switch, with correct handling of all edge cases: division by zero, signed overflow (`INT_MIN / -1`).

### Memory (MMU)

Flat byte vector with base address `0x80000000`. Supports 1, 2, and 4-byte little-endian accesses. Monitors a `tohost` address to detect test completion.

### Test Result Detection

Tests write to the `tohost` symbol at the end of execution:
- `tohost == 1` → **PASS**
- `tohost == (N << 1) | 1` → **FAIL**, test case N

### Debug TUI

Interactive terminal UI (FTXUI):
- **Left panel** — all 32 registers with ABI names, highlighted on change
- **Center panel** — instruction trace (last 20 instructions)
- **Right panel** — memory hex dump around PC + test result
- **Controls**: `Space` step, `R` run/pause, `Q` quit

---

## Roadmap

### Next — RV32C (Compressed Instructions)

The C extension adds 16-bit encodings for the most common instructions. Required to run binaries compiled without `-mno-rvc`.

- [ ] Implement 16-bit instruction fetch and decode
- [ ] Map all C.* instructions to their 32-bit equivalents
- [ ] Pass `rv32uc-p-*` tests

### Medium Term

- [ ] Run real C programs compiled with `riscv32-unknown-elf-gcc` + newlib
- [ ] Implement basic syscalls: `write`, `exit`, `brk`
- [ ] GDB stub (RSP protocol) for connecting a real debugger
- [ ] Exception/trap dispatch through `mtvec`

### Long Term — Goal: Boot a Small OS

The final goal is to boot [xv6-riscv](https://github.com/mit-pdos/xv6-riscv) or [FreeRTOS](https://www.freertos.org/) on the emulator. This requires:

- [ ] Supervisor mode + Sv32 paging (MMU with page table walker)
- [ ] UART, CLINT, PLIC as MMIO peripherals
- [ ] RV32A atomic instructions
- [ ] Timer interrupts

---

## References

- [Official RISC-V Specification](https://riscv.org/technical/specifications/)
- [riscv-tests on GitHub](https://github.com/riscv-software-src/riscv-tests)
- [RISC-V ISA Reference Card](https://github.com/jameslzhu/riscv-card)
- [RV32I Unprivileged Spec (PDF)](https://github.com/riscv/riscv-isa-manual/releases)
- [FTXUI — Terminal UI library](https://github.com/ArthurSonzogni/FTXUI)

---

## License

See `LICENSE`.