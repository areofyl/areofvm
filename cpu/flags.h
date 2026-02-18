#pragma once
#include "../sequential/flip_flop.h"
#include "../gates/gates.h"

// Flags — stores ALU status bits between instructions.
// Zero and carry flags, each a single D flip-flop.
// Only updated when load is high (ALU/CMP instructions).

class Flags {
public:
    bool carry = false;
    bool zero  = false;

    void update(bool clk, bool load, bool new_carry, bool new_zero) {
        bool c_in = gate::OR(gate::AND(load, new_carry), gate::AND(gate::NOT(load), carry));
        bool z_in = gate::OR(gate::AND(load, new_zero), gate::AND(gate::NOT(load), zero));
        carry_ff.clock(clk, c_in);
        zero_ff.clock(clk, z_in);
        carry = carry_ff.q;
        zero  = zero_ff.q;
    }

private:
    DFlipFlop carry_ff;
    DFlipFlop zero_ff;
};
