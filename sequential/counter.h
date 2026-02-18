#pragma once
#include "flip_flop.h"
#include "../gates/gates.h"
#include <array>

// N-bit binary counter — counts up on each rising clock edge.
//
// Inputs:
//   clk    — clock signal
//   reset  — synchronous reset: clears count to 0 on next rising edge
//   enable — when LOW, counter holds its current value
//
// Output:
//   value  — the current N-bit count
//
// How it works: each bit toggles when all lower bits are 1.
// Bit 0 toggles every cycle. Bit 1 toggles when bit 0 is 1.
// Bit 2 toggles when bits 0 AND 1 are both 1. And so on.
// (This is a ripple-carry counter built from toggle logic.)

template <int N>
class Counter {
public:
    std::array<bool, N> value = {};

    void clock(bool clk, bool reset, bool enable) {
        // Compute the next value for each bit
        // A bit toggles when enable is high and all lower bits are 1
        bool all_lower_ones = true;

        for (int i = 0; i < N; i++) {
            // Should this bit toggle?
            bool toggle = gate::AND(enable, all_lower_ones);

            // XOR current value with toggle to get next value
            // But if reset, force to 0
            bool next = gate::AND(
                gate::NOT(reset),
                gate::XOR(value[i], toggle)
            );

            bits[i].clock(clk, next);
            value[i] = bits[i].q;

            // Update the carry chain: all bits up to here must be 1
            all_lower_ones = gate::AND(all_lower_ones, value[i]);
        }
    }

    // Helper: get count as an integer
    int to_int() const {
        int result = 0;
        for (int i = 0; i < N; i++) {
            if (value[i]) result |= (1 << i);
        }
        return result;
    }

private:
    std::array<DFlipFlop, N> bits = {};
};
