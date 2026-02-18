#pragma once
#include "../sequential/register.h"
#include <array>
#include <cstdint>

// Instruction register: holds the current 16-bit instruction.
// Loaded in two 8-bit halves from memory.

class InstructionRegister {
public:
    void load_lo(bool clk, bool en, const std::array<bool, 8>& data) {
        lo.clock(clk, en, data);
    }
    void load_hi(bool clk, bool en, const std::array<bool, 8>& data) {
        hi.clock(clk, en, data);
    }

    // bits 15-12
    std::array<bool, 4> opcode() const {
        return {hi.data_out[4], hi.data_out[5], hi.data_out[6], hi.data_out[7]};
    }
    // bits 11-10
    std::array<bool, 2> rd() const {
        return {hi.data_out[2], hi.data_out[3]};
    }
    // bits 9-8
    std::array<bool, 2> rs() const {
        return {hi.data_out[0], hi.data_out[1]};
    }
    // bits 7-0
    std::array<bool, 8> imm8() const {
        return lo.data_out;
    }

    uint16_t to_int() const {
        uint16_t v = 0;
        for (int i = 0; i < 8; i++) {
            if (lo.data_out[i]) v |= (1 << i);
            if (hi.data_out[i]) v |= (1 << (i + 8));
        }
        return v;
    }

private:
    Register<8> lo;
    Register<8> hi;
};
