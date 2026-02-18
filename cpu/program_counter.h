#pragma once
#include "../sequential/register.h"
#include "../arithmetic/adder.h"
#include "../arithmetic/mux.h"
#include <array>
#include <cstdint>

// Program counter: holds address of current instruction.
// Increments by 2 each cycle, or loads a jump address.

class ProgramCounter {
public:
    std::array<bool, 8> value = {};

    void clock(bool clk, bool jump, const std::array<bool, 8>& jump_addr) {
        // add 2 to current value
        adder.add(value, two);
        // pick: jump address or incremented value
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
    std::array<bool, 8> two = {false, true}; // constant 2
};
