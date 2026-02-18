#pragma once
#include "../sequential/register.h"
#include <array>
#include <cstdint>

// 24-bit instruction register. Three bytes:
//   byte0 = imm_lo   (bits 7-0)    — low byte of immediate
//   byte1 = imm_hi   (bits 15-8)   — high byte of immediate
//   byte2 = opcode+regs (bits 23-16) — opcode[7:4] rd[3:2] rs[1:0]

class InstructionRegister {
public:
    void load_byte0(bool clk, bool en, const std::array<bool, 8>& data) {
        b0.clock(clk, en, data);
    }
    void load_byte1(bool clk, bool en, const std::array<bool, 8>& data) {
        b1.clock(clk, en, data);
    }
    void load_byte2(bool clk, bool en, const std::array<bool, 8>& data) {
        b2.clock(clk, en, data);
    }

    std::array<bool, 4> opcode() const {
        return {b2.data_out[4], b2.data_out[5], b2.data_out[6], b2.data_out[7]};
    }

    std::array<bool, 2> rd() const {
        return {b2.data_out[2], b2.data_out[3]};
    }

    std::array<bool, 2> rs() const {
        return {b2.data_out[0], b2.data_out[1]};
    }

    // 8-bit immediate (low byte only, for LDI etc)
    std::array<bool, 8> imm8() const {
        return b0.data_out;
    }

    // 16-bit immediate (for JMP, CALL, LD/ST addresses)
    std::array<bool, 16> imm16() const {
        std::array<bool, 16> val = {};
        for (int i = 0; i < 8; i++) {
            val[i] = b0.data_out[i];
            val[i + 8] = b1.data_out[i];
        }
        return val;
    }

private:
    Register<8> b0;  // imm_lo
    Register<8> b1;  // imm_hi
    Register<8> b2;  // opcode + rd + rs
};
