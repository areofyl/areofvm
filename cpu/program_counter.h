#pragma once
#include "../sequential/register.h"
#include "../arithmetic/adder.h"
#include "../arithmetic/mux.h"
#include <array>
#include <cstdint>

// 16-bit program counter. Increments by 3 (24-bit instructions) or loads a jump address.

class ProgramCounter {
public:
    std::array<bool, 16> value = {};

    void clock(bool clk, bool jump, const std::array<bool, 16>& jump_addr) {
        adder.add(value, three);
        mux.select(jump, adder.sum, jump_addr);
        reg.clock(clk, true, mux.output);
        value = reg.data_out;
    }

    void reset() {
        value = {};
        std::array<bool, 16> zero = {};
        reg.clock(false, true, zero);
        reg.clock(true, true, zero);
        value = reg.data_out;
    }

    uint16_t to_int() const {
        uint16_t v = 0;
        for (int i = 0; i < 16; i++)
            if (value[i]) v |= (1 << i);
        return v;
    }

private:
    Register<16> reg;
    RippleCarryAdder<16> adder;
    Mux2<16> mux;
    static constexpr std::array<bool, 16> three = {true, true, false, false, false, false, false, false,
                                                    false, false, false, false, false, false, false, false};
};
