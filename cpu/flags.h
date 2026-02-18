#pragma once
#include "../sequential/flip_flop.h"
#include "../gates/gates.h"

// Flags — remembers ALU status between instructions.
//
// Two single-bit flags stored in D flip-flops:
//   zero  — was the ALU result 0? (used by JZ/JNZ)
//   carry — did the addition overflow? (useful for multi-byte math)
//
// The load signal acts as a gate: when load=0, the flip-flop
// feeds back its own value (hold). When load=1, it captures
// the new flag value. This is the same mux-before-register
// pattern used in Register<N>.
//
// Only ALU instructions (ADD, SUB, AND, OR, CMP, ADDI) set load=1.
// Other instructions leave the flags unchanged.

class Flags {
public:
    bool carry = false;
    bool zero  = false;

    void update(bool clk, bool load, bool new_carry, bool new_zero) {
        // Mux: load=1 passes new value, load=0 feeds back current value
        bool c_in = gate::OR(gate::AND(load, new_carry),
                             gate::AND(gate::NOT(load), carry));
        bool z_in = gate::OR(gate::AND(load, new_zero),
                             gate::AND(gate::NOT(load), zero));

        carry_ff.clock(clk, c_in);
        zero_ff.clock(clk, z_in);

        carry = carry_ff.q;
        zero  = zero_ff.q;
    }

private:
    DFlipFlop carry_ff;
    DFlipFlop zero_ff;
};
