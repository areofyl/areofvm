#pragma once
#include "../arithmetic/decoder.h"
#include "../gates/gates.h"
#include <array>

// Control signals — one wire per decision the CPU makes each cycle.
//
// reg_write:   write a result back to the register file
// mem_read:    read a byte from memory into a register
// mem_write:   write a register value to memory
// alu_op0/1:   which ALU operation (00=ADD, 01=SUB, 10=AND, 11=OR)
// alu_src_imm: ALU's second input comes from imm8 instead of Rs
// reg_src_mem: register write data comes from memory (not ALU)
// reg_src_imm: register write data comes from imm8 directly
// pc_jump:     load the PC with a new address (branch/jump)
// flags_write: update the zero/carry flags from ALU output
// halt:        stop the CPU
// io_write:    output a register value to the I/O bus
// is_mov:      register write data comes from Rs (register-to-register copy)

struct ControlSignals {
    bool reg_write    = false;
    bool mem_read     = false;
    bool mem_write    = false;
    bool alu_op0      = false;
    bool alu_op1      = false;
    bool alu_src_imm  = false;
    bool reg_src_mem  = false;
    bool reg_src_imm  = false;
    bool pc_jump      = false;
    bool flags_write  = false;
    bool halt         = false;
    bool is_mov       = false;
};

// ControlUnit — the CPU's "brain". Pure combinational logic.
//
// Takes the 4-bit opcode and current flags, produces all the
// control signals that tell every other component what to do.
//
// How it works:
//   1. Decoder converts 4-bit opcode into 16 one-hot lines
//      (exactly one line is HIGH for each opcode)
//   2. Each control signal is an OR of the opcode lines that need it
//      (e.g., reg_write is high for LDI, LD, ADD, SUB, AND, OR, MOV, ADDI)
//   3. Conditional jumps AND the opcode line with the relevant flag

class ControlUnit {
public:
    ControlSignals signals = {};

    void decode(const std::array<bool, 4>& opcode, bool zero_flag) {
        dec.decode(opcode);

        // Give each decoder output a readable name
        bool nop  = dec.outputs[0x0];
        bool ldi  = dec.outputs[0x1];
        bool ld   = dec.outputs[0x2];
        bool st   = dec.outputs[0x3];
        bool add  = dec.outputs[0x4];
        bool sub  = dec.outputs[0x5];
        bool and_ = dec.outputs[0x6];
        bool or_  = dec.outputs[0x7];
        bool mov  = dec.outputs[0x8];
        bool cmp  = dec.outputs[0x9];
        bool jmp  = dec.outputs[0xA];
        bool jz   = dec.outputs[0xB];
        bool jnz  = dec.outputs[0xC];
        bool addi = dec.outputs[0xD];
        bool call = dec.outputs[0xE];
        bool hlt  = dec.outputs[0xF];

        (void)nop;   // NOP (and PUSH/POP/RET) handled directly in CPU
        (void)call;  // CALL handled directly in CPU

        // Which instructions write back to a register?
        signals.reg_write = gate::OR(gate::OR(gate::OR(ldi, ld), gate::OR(add, sub)),
                                     gate::OR(gate::OR(and_, or_), gate::OR(mov, addi)));

        // Memory access
        signals.mem_read  = ld;
        signals.mem_write = st;

        // ALU operation select (maps to ALU's op0/op1 inputs)
        // ADD=00, SUB=01, AND=10, OR=11
        signals.alu_op0 = gate::OR(sub, gate::OR(or_, cmp));
        signals.alu_op1 = gate::OR(and_, or_);

        // ALU second operand source
        signals.alu_src_imm = addi;

        // What data gets written to the register file?
        signals.reg_src_mem = ld;   // from memory
        signals.reg_src_imm = ldi;  // from immediate
        signals.is_mov      = mov;  // from another register

        // Jump logic — unconditional or conditional on flags
        signals.pc_jump = gate::OR(jmp,
                          gate::OR(gate::AND(jz, zero_flag),
                                   gate::AND(jnz, gate::NOT(zero_flag))));

        // Which instructions update flags?
        signals.flags_write = gate::OR(gate::OR(add, sub),
                              gate::OR(gate::OR(and_, or_),
                                       gate::OR(cmp, addi)));

        signals.halt = hlt;
    }

private:
    Decoder<4> dec;
};
