#pragma once
#include "flip_flop.h"
#include <array>

// N-bit Register — a row of D flip-flops sharing one clock.
//
// Inputs:
//   clk      — clock signal (data captured on rising edge)
//   load     — load enable: when LOW, the register ignores new data
//              and keeps feeding its own output back in
//   data_in  — N bits of input data
//
// Output:
//   data_out — the N stored bits

template <int N>
class Register {
public:
    std::array<bool, N> data_out = {};

    void clock(bool clk, bool load, const std::array<bool, N>& data_in) {
        for (int i = 0; i < N; i++) {
            // Mux: if load is high, feed in new data; otherwise feed back current output
            // This is just: selected = OR(AND(load, data_in), AND(NOT(load), data_out))
            bool selected = gate::OR(
                gate::AND(load, data_in[i]),
                gate::AND(gate::NOT(load), data_out[i])
            );

            bits[i].clock(clk, selected);
            data_out[i] = bits[i].q;
        }
    }

private:
    std::array<DFlipFlop, N> bits = {};
};
