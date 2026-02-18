#pragma once
#include "gates/gates.h"
#include <array>

// N-to-2^N Decoder — selects one of 2^N output lines based on N input bits.
//
// Example: a 2-to-4 decoder with input bits [1, 0] (= 2 in binary)
// produces outputs [0, 0, 1, 0] — only output line 2 is high.
//
// This is how a CPU selects which register to write to, or how
// memory chips select which address to access.

template <int N>
class Decoder {
public:
    static constexpr int NUM_OUTPUTS = 1 << N;  // 2^N

    std::array<bool, NUM_OUTPUTS> outputs = {};

    void decode(const std::array<bool, N>& address, bool enable = true) {
        for (int out = 0; out < NUM_OUTPUTS; out++) {
            // An output line is HIGH when the address bits match its index.
            // We AND together each address bit (or its complement):
            //   - If bit i of the output index is 1, use address[i]
            //   - If bit i of the output index is 0, use NOT(address[i])
            bool match = true;
            for (int bit = 0; bit < N; bit++) {
                bool need_high = (out >> bit) & 1;
                bool term = need_high ? address[bit] : gate::NOT(address[bit]);
                match = gate::AND(match, term);
            }
            outputs[out] = gate::AND(match, enable);
        }
    }
};
