# seedisa

An 8-bit CPU and instruction set built from logic gates up in C++.

Inspired by *Code: The Hidden Language of Computer Hardware and Software* by Charles Petzold. Everything is header-only, using `std::array<bool, N>` as bit vectors. Gates compose into flip-flops, flip-flops into registers, registers into a full CPU with fetch-decode-execute.

Just for a learning experience, DO NOT USE THIS ISA!

## Architecture

- **8-bit** data width, **16-bit** address space (64 KB)
- **4 registers**: R0-R3
- **24-bit** fixed-width instructions: `[imm16:16][opcode:4][rd:2][rs:2]`
- **16-bit** program counter and stack pointer
- Hardware interrupts with 8-entry IVT at `0xEFF0`

## Instruction set

| Op | Mnemonic | Description |
|----|----------|-------------|
| 0x0 | misc | NOP, CLI, STI, RTI, PUSH, POP, RET, SWI |
| 0x1 | LDI Rd, imm | Rd = imm8 |
| 0x2 | LD Rd, [imm16] | Rd = mem[imm16] |
| 0x3 | ST Rd, [imm16] | mem[imm16] = Rd |
| 0x4 | ADD Rd, Rs | Rd = Rd + Rs |
| 0x5 | SUB Rd, Rs | Rd = Rd - Rs |
| 0x6 | AND Rd, Rs | Rd = Rd & Rs |
| 0x7 | OR Rd, Rs | Rd = Rd \| Rs |
| 0x8 | MOV Rd, Rs | Rd = Rs |
| 0x9 | CMP Rd, Rs | flags = Rd - Rs |
| 0xA | JMP imm16 | PC = imm16 |
| 0xB | JZ imm16 | if zero: PC = imm16 |
| 0xC | JNZ imm16 | if !zero: PC = imm16 |
| 0xD | ADDI Rd, imm | Rd = Rd + imm8 |
| 0xE | CALL imm16 | push PC, PC = imm16 |
| 0xF | HLT | halt |

## Memory map

| Range | Size | Purpose |
|-------|------|---------|
| `0x0000-0xEFEF` | 60 KB | RAM |
| `0xEFF0-0xEFFF` | 16 B | Interrupt Vector Table |
| `0xF000-0xFFFF` | 4 KB | Memory-mapped I/O |

## Devices

- **Timer** (I/O offset 0x00-0x01): countdown timer, fires interrupt 1

## Building

```
g++ -std=c++17 -o test_runner test.cpp && ./test_runner
```

## Project structure

```
seedisa/
  gates/        NAND, NOT, AND, OR, XOR, MUX
  sequential/   SR latch, D flip-flop, register
  arithmetic/   Adder, ALU, decoder, multiplexer
  memory/       RAM, system bus
  cpu/          Register file, PC, IR, flags, control unit, CPU
  devices/      Timer
```

Part of the [seedsys](https://github.com/seedsys) project. The OS is [seedos](https://github.com/seedsys/seedos).
