#pragma once
#include "../sequential/register.h"
#include "../arithmetic/adder.h"
#include "../arithmetic/mux.h"
#include <array>
#include <cstdint>

// ProgramCounter — tracks which instruction the CPU is executing.
//
// Each cycle it either:
//   - Increments by 2 (our instructions are 16-bit = 2 bytes)
//   - Loads an absolute address (for JMP/JZ/JNZ)
//
// Internally: an adder computes PC+2, a mux picks between
// that and the jump target, and a register stores the result.
//
// 8-bit PC means 256-byte address space (128 instructions max).

class ProgramCounter {
public:
    std::array<bool, 8> value = {};

    // jump=false: PC = PC + 2 (next instruction)
    // jump=true:  PC = jump_addr (branch taken)
    void clock(bool clk, bool jump, const std::array<bool, 8>& jump_addr) {
        adder.add(value, two);

        // Mux2: sel=0 gives first arg, sel=1 gives second arg.
        // So jump=0 → adder.sum (normal flow), jump=1 → jump_addr.
        mux.select(jump, adder.sum, jump_addr);

        reg.clock(clk, true, mux.output);
        value = reg.data_out;
    }

    void reset() {
        value = {};
        std::array<bool, 8> zero = {};
        reg.clock(false, true, zero);
        reg.clock(true, true, zero);
        value = reg.data_out;
    }

    uint8_t to_int() const {
        uint8_t v = 0;
        for (int i = 0; i < 8; i++)
            if (value[i]) v |= (1 << i);
        return v;
    }

private:
    Register<8> reg;
    RippleCarryAdder<8> adder;
    Mux2<8> mux;
    static constexpr std::array<bool, 8> two = {false, true, false, false, false, false, false, false};
};
