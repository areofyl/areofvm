#pragma once
#include "../gates/gates.h"
#include <array>

// Half Adder — adds two single bits.
// Outputs: sum and carry.

struct HalfAdder {
    bool sum   = false;
    bool carry = false;

    void add(bool a, bool b) {
        sum   = gate::XOR(a, b);
        carry = gate::AND(a, b);
    }
};

// Full Adder — adds two bits plus a carry-in.
// Built from two half adders (just like the book shows).

struct FullAdder {
    bool sum   = false;
    bool carry = false;

    void add(bool a, bool b, bool carry_in) {
        HalfAdder ha1, ha2;

        ha1.add(a, b);              // First half: add a + b
        ha2.add(ha1.sum, carry_in); // Second half: add that sum + carry_in

        sum   = ha2.sum;
        carry = gate::OR(ha1.carry, ha2.carry);  // Carry if either half adder carried
    }
};

// Ripple-Carry Adder — chains N full adders to add two N-bit numbers.
// The carry "ripples" from bit 0 up to bit N-1.

template <int N>
class RippleCarryAdder {
public:
    std::array<bool, N> sum = {};
    bool carry_out = false;

    void add(const std::array<bool, N>& a, const std::array<bool, N>& b, bool carry_in = false) {
        bool carry = carry_in;

        for (int i = 0; i < N; i++) {
            FullAdder fa;
            fa.add(a[i], b[i], carry);
            sum[i] = fa.sum;
            carry = fa.carry;
        }

        carry_out = carry;
    }

    // Helper: convert result to integer
    int to_int() const {
        int result = 0;
        for (int i = 0; i < N; i++) {
            if (sum[i]) result |= (1 << i);
        }
        return result;
    }
};
