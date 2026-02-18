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

### CPU block diagram

```
                            ┌─────────────────────────────────────────────┐
                            │                   CPU                       │
                            │                                             │
  ┌──────────┐   16-bit     │  ┌──────────────────────────────────┐       │
  │          │◄─────────────┼──┤ Program Counter (16-bit)         │       │
  │          │   address    │  │   +3 each cycle / load on jump   │       │
  │          │              │  └──────┬──────────────────▲────────┘       │
  │          │   8-bit      │         │ fetch addr       │ jump addr     │
  │          ├──────────────┼──►┌─────▼──────────────┐   │                │
  │          │   data       │   │ Instruction Reg    │   │                │
  │          │              │   │ (24 bits: 3 bytes) │   │                │
  │   BUS    │              │   └──┬───┬───┬───┬─────┘   │                │
  │          │              │      │   │   │   │         │                │
  │ addr bus │              │  opcode rd  rs  imm16──────┘                │
  │ data bus │              │   4b  2b  2b  16b                           │
  │          │              │      │   │   │   │                          │
  │          │              │  ┌───▼───┼───┼───┼──────────────────────┐   │
  │          │              │  │ Control Unit          zero flag      │   │
  │          │              │  │  Decoder(4) ──► one-hot ──► gates   │   │
  │          │              │  └──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬───┘   │
  │          │              │     │  │  │  │  │  │  │  │  │  │  │       │
  │          │              │     │ reg mem alu │ src│  │ flg│ halt     │
  │          │              │     │ wr  r/w op  │ sel│  │ wr │         │
  │          │              │     │  │  │  │   │  │ │  │  │  │         │
  │          │              │  ┌──┼──┼──┼──▼───┼──▼─┼──┼──▼──┼──┐      │
  │          │              │  │  │  │  │      │    │  │     │  │      │
  │          │   16-bit     │  │  ▼  │  │  ┌───▼──────▼──┐   │  │      │
  │          │◄─────────────┼──│ SP  │  │  │ Register    │   │  │      │
  │          │   stack ops  │  │     │  │  │ File (4x8)  │   │  │      │
  │          │              │  └──┬──┘  │  │ Rd_out Rs_out│   │  │      │
  │          │              │     │     │  └──┬───────┬──┘   │  │      │
  │          │              │     │     │     │  Rd   │ Rs   │  │      │
  │          │              │     │     │     │       │      │  │      │
  │          │              │     │     │     │  ┌────▼──┐   │  │      │
  │          │              │     │     │     │  │ MUX   │   │  │      │
  │          │              │     │     │     │  │Rs/imm8│   │  │      │
  │          │              │     │     │     │  └───┬───┘   │  │      │
  │          │              │     │     │     │      │       │  │      │
  │          │              │     │     │     ▼      ▼       │  │      │
  │          │              │     │     │   ┌──────────┐     │  │      │
  │          │              │     │     │   │  ALU     │     │  │      │
  │          │              │     │     │   │ ADD SUB  │     │  │      │
  │          │              │     │     │   │ AND OR   │     │  │      │
  │          │              │     │     │   └──┬────┬──┘     │  │      │
  │          │              │     │     │      │    │carry   │  │      │
  │          │              │     │     │   result  │zero    │  │      │
  │          │              │     │     │      │    │        │  │      │
  │          │              │     │     │      │  ┌─▼──────┐ │  │      │
  │          │              │     │     │      │  │ Flags  │ │  │      │
  │          │              │     │     │      │  │ Z  C   │─┘  │      │
  │          │              │     │     │      │  └────────┘    │      │
  │          │              │     │     │      │                │      │
  │          │              │     │  ┌──▼──────▼──────┐         │      │
  │          │   read data  │     │  │ Writeback MUX  │         │      │
  │          ├──────────────┼─────┼─►│ alu / mem / imm│         │      │
  │          │              │     │  │ / Rs (mov)     │         │      │
  │          │   write data │     │  └───────┬────────┘         │      │
  │          │◄─────────────┼─────┘          │                  │      │
  │          │              │           write data ──► Register File    │
  └──────────┘              │                                          │
       │                    └──────────────────────────────────────────┘
       │ I/O region (0xF000+)
  ┌────▼─────┐
  │  Timer   │──── interrupt 1 ──► CPU.raise_interrupt()
  │ reload   │
  │ control  │
  └──────────┘
```

### Instruction encoding (24-bit)

```
byte 0       byte 1       byte 2
┌───────────┬───────────┬───────────────────┐
│  imm_lo   │  imm_hi   │ opcode │ Rd │ Rs  │
│  [7:0]    │  [15:8]   │ [7:4]  │[3:2]│[1:0]│
└───────────┴───────────┴────────┴────┴─────┘
     8 bits      8 bits    4 bits  2b    2b
```

### Fetch-decode-execute cycle

```
┌───────┐     ┌────────┐     ┌─────────┐
│ FETCH │────►│ DECODE │────►│ EXECUTE │──┐
└───────┘     └────────┘     └─────────┘  │
    ▲                                     │
    └─────────────────────────────────────┘

FETCH:   PC ──► Bus ──► IR (3 bytes), PC += 3
DECODE:  IR.opcode ──► Control Unit ──► control signals
         IR.rd/rs  ──► Register File ──► Rd_out, Rs_out
EXECUTE: ALU computes, memory reads/writes,
         writeback mux selects result, flags update,
         PC jumps if needed
```

### Interrupt flow

```
Device raises interrupt
        │
        ▼
┌──────────────────┐     ┌──────────────────┐
│ check_interrupts │────►│ enter_interrupt   │
│ (each step)      │     │  push PC + flags  │
│ int_enabled &&   │     │  int_enabled = 0  │
│ int_pending?     │     │  PC = IVT[num]    │
└──────────────────┘     └──────────────────┘
                                  │
                     handler runs (interrupts off)
                                  │
                                  ▼
                         ┌──────────────────┐
                         │ RTI              │
                         │  pop flags + PC  │
                         │  restore int_en  │
                         └──────────────────┘
```

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

| Device | I/O Offset | Registers | Interrupt |
|--------|-----------|-----------|-----------|
| Timer  | 0x00-0x01 | 0: reload, 1: status/ctrl | 1 |
| UART   | 0x02-0x03 | 0: data (TX/RX), 1: status | 2 |

**Timer**: Countdown timer. Write reload value to reg 0, enable via reg 1 bit 1. Fires interrupt 1 when counter hits zero.

**UART**: Serial character I/O. Write a byte to reg 0 to transmit. Read reg 0 to receive. Status reg 1: bit 0 = RX data available, bit 1 = TX ready.

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
  devices/      Timer, UART
```

Part of the [seedsys](https://github.com/seedsys) project. The OS is [seedos](https://github.com/seedsys/seedos).
