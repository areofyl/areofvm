#pragma once
#include "../gates/gates.h"
#include <array>

// Mux2<N>: 2-to-1 multiplexer, N bits wide.
//
// A selector switch — picks one of two inputs based on a single bit.
//   sel=0 → output = a
//   sel=1 → output = b
//
// Each output bit is computed with pure gate logic:
//   output[i] = OR(AND(NOT(sel), a[i]), AND(sel, b[i]))

template <int N>
class Mux2 {
public:
    std::array<bool, N> output = {};

    void select(bool sel,
                const std::array<bool, N>& a,
                const std::array<bool, N>& b) {
        for (int i = 0; i < N; i++) {
            output[i] = gate::OR(
                gate::AND(gate::NOT(sel), a[i]),
                gate::AND(sel, b[i])
            );
        }
    }
};

// Mux4<N>: 4-to-1 multiplexer, N bits wide.
//
// Two select bits choose one of four inputs:
//   s1=0 s0=0 → a
//   s1=0 s0=1 → b
//   s1=1 s0=0 → c
//   s1=1 s0=1 → d
//
// Built as a tree of three Mux2s:
//   mux_lo  picks between a,b using s0
//   mux_hi  picks between c,d using s0
//   mux_out picks between those two results using s1
//
// Used in the CPU to select which of the 4 registers to read.

template <int N>
class Mux4 {
public:
    std::array<bool, N> output = {};

    void select(bool s0, bool s1,
                const std::array<bool, N>& a,
                const std::array<bool, N>& b,
                const std::array<bool, N>& c,
                const std::array<bool, N>& d) {
        mux_lo.select(s0, a, b);
        mux_hi.select(s0, c, d);
        mux_out.select(s1, mux_lo.output, mux_hi.output);
        output = mux_out.output;
    }

private:
    Mux2<N> mux_lo;
    Mux2<N> mux_hi;
    Mux2<N> mux_out;
};
