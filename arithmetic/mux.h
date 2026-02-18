#pragma once
#include "../gates/gates.h"
#include <array>

// 2-to-1 mux: sel=0 gives a, sel=1 gives b
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

// 4-to-1 mux: two select bits pick from four inputs
template <int N>
class Mux4 {
public:
    std::array<bool, N> output = {};

    void select(bool s0, bool s1,
                const std::array<bool, N>& a,
                const std::array<bool, N>& b,
                const std::array<bool, N>& c,
                const std::array<bool, N>& d) {
        lo.select(s0, a, b);
        hi.select(s0, c, d);
        final.select(s1, lo.output, hi.output);
        output = final.output;
    }

private:
    Mux2<N> lo, hi, final;
};
