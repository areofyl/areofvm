#pragma once
#include "../sequential/register.h"
#include <array>
#include <cstdint>

// InstructionRegister — holds the 16-bit instruction being executed.
//
// Memory is 8 bits wide, so instructions are fetched in two halves:
//   lo = memory[PC]     (bits 7-0, the immediate value)
//   hi = memory[PC+1]   (bits 15-8, opcode + register selects)
//
// Bit layout of a full instruction (LSB at index 0):
//
//   hi byte:                    lo byte:
//   [7  6  5  4  3  2  1  0]   [7  6  5  4  3  2  1  0]
//    op op op op Rd Rd Rs Rs     im im im im im im im im
//    15 14 13 12 11 10  9  8      7  6  5  4  3  2  1  0
//
// The extraction methods pull out each field as a bool array (LSB first).

class InstructionRegister {
public:
    void load_lo(bool clk, bool en, const std::array<bool, 8>& data) {
        lo.clock(clk, en, data);
    }

    void load_hi(bool clk, bool en, const std::array<bool, 8>& data) {
        hi.clock(clk, en, data);
    }

    // Opcode: bits 15-12 (hi byte bits 7-4)
    std::array<bool, 4> opcode() const {
        return {hi.data_out[4], hi.data_out[5], hi.data_out[6], hi.data_out[7]};
    }

    // Destination register: bits 11-10 (hi byte bits 3-2)
    std::array<bool, 2> rd() const {
        return {hi.data_out[2], hi.data_out[3]};
    }

    // Source register: bits 9-8 (hi byte bits 1-0)
    std::array<bool, 2> rs() const {
        return {hi.data_out[0], hi.data_out[1]};
    }

    // Immediate value: bits 7-0 (entire lo byte)
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
