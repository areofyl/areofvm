#pragma once
#include "../arithmetic/decoder.h"
#include "../gates/gates.h"
#include <array>

struct ControlSignals {
    bool reg_write;
    bool mem_read;
    bool mem_write;
    bool alu_op0;
    bool alu_op1;
    bool alu_src_imm;
    bool reg_src_mem;
    bool reg_src_imm;
    bool pc_jump;
    bool flags_write;
    bool halt;
    bool io_write;
    bool is_mov;
};

// Takes opcode + flags, produces control signals. Pure combinational.

class ControlUnit {
public:
    ControlSignals signals = {};

    void decode(const std::array<bool, 4>& opcode, bool zero_flag) {
        dec.decode(opcode);

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
        bool out  = dec.outputs[0xE];
        bool hlt  = dec.outputs[0xF];

        signals.reg_write = gate::OR(gate::OR(gate::OR(ldi, ld), gate::OR(add, sub)),
                                     gate::OR(gate::OR(and_, or_), gate::OR(mov, addi)));
        signals.mem_read    = ld;
        signals.mem_write   = st;
        signals.alu_op0     = gate::OR(sub, gate::OR(or_, cmp));
        signals.alu_op1     = gate::OR(and_, or_);
        signals.alu_src_imm = addi;
        signals.reg_src_mem = ld;
        signals.reg_src_imm = ldi;
        signals.pc_jump     = gate::OR(jmp, gate::OR(gate::AND(jz, zero_flag),
                                                     gate::AND(jnz, gate::NOT(zero_flag))));
        signals.flags_write = gate::OR(gate::OR(add, sub), gate::OR(gate::OR(and_, or_), gate::OR(cmp, addi)));
        signals.halt        = hlt;
        signals.io_write    = out;
        signals.is_mov      = mov;
    }

private:
    Decoder<4> dec;
};
