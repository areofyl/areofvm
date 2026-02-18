#pragma once
#include "adder.h"
#include <array>

// Simple ALU (Arithmetic Logic Unit)
//
// Operations (selected by 2-bit opcode):
//   00 = ADD:  result = A + B
//   01 = SUB:  result = A - B  (using two's complement: A + NOT(B) + 1)
//   10 = AND:  result = A & B  (bitwise)
//   11 = OR:   result = A | B  (bitwise)
//
// Flags:
//   carry — carry/borrow out from addition/subtraction
//   zero  — true when result is all zeros

template <int N>
class ALU {
public:
    std::array<bool, N> result = {};
    bool carry = false;
    bool zero  = false;

    void compute(const std::array<bool, N>& a,
                 const std::array<bool, N>& b,
                 bool op0, bool op1)
    {
        // op1=0: arithmetic (ADD/SUB),  op1=1: logic (AND/OR)
        // op0=0: ADD or AND,            op0=1: SUB or OR

        // --- Arithmetic path ---
        // For SUB, invert B and set carry-in to 1 (two's complement negation)
        std::array<bool, N> b_modified = {};
        for (int i = 0; i < N; i++) {
            b_modified[i] = gate::XOR(b[i], op0);  // invert B if SUB
        }

        RippleCarryAdder<N> adder;
        adder.add(a, b_modified, op0);  // carry_in = 1 if SUB

        // --- Logic path ---
        std::array<bool, N> logic_result = {};
        for (int i = 0; i < N; i++) {
            bool and_result = gate::AND(a[i], b[i]);
            bool or_result  = gate::OR(a[i], b[i]);
            // Mux: op0 selects AND (0) or OR (1)
            logic_result[i] = gate::OR(
                gate::AND(gate::NOT(op0), and_result),
                gate::AND(op0, or_result)
            );
        }

        // --- Output mux: op1 selects arithmetic (0) or logic (1) ---
        zero = true;
        for (int i = 0; i < N; i++) {
            result[i] = gate::OR(
                gate::AND(gate::NOT(op1), adder.sum[i]),
                gate::AND(op1, logic_result[i])
            );
            if (result[i]) zero = false;
        }

        // Carry flag only meaningful for arithmetic ops
        carry = gate::AND(gate::NOT(op1), adder.carry_out);
    }

    // Helper: convert result to integer
    int to_int() const {
        int r = 0;
        for (int i = 0; i < N; i++) {
            if (result[i]) r |= (1 << i);
        }
        return r;
    }
};
