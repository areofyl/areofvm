#pragma once
#include "../sequential/register.h"
#include "../arithmetic/decoder.h"
#include "../arithmetic/mux.h"
#include <array>
#include <cstdint>

// RegisterFile — the CPU's fast scratch storage.
//
// Contains 4 general-purpose 8-bit registers (R0-R3).
// Two read ports (Rd and Rs can be read simultaneously)
// and one write port.
//
// Reading uses Mux4 — the 2-bit select lines pick which
// register's output gets routed to the output.
//
// Writing uses a Decoder — the 2-bit select lines activate
// exactly one register's load enable, so only that register
// captures the new data on the clock edge.

class RegisterFile {
public:
    std::array<bool, 8> rd_out = {};
    std::array<bool, 8> rs_out = {};

    // Read two registers simultaneously.
    // rd_sel picks which register appears on rd_out.
    // rs_sel picks which register appears on rs_out.
    void read(const std::array<bool, 2>& rd_sel,
              const std::array<bool, 2>& rs_sel) {
        rd_mux.select(rd_sel[0], rd_sel[1],
                      regs[0].data_out, regs[1].data_out,
                      regs[2].data_out, regs[3].data_out);
        rs_mux.select(rs_sel[0], rs_sel[1],
                      regs[0].data_out, regs[1].data_out,
                      regs[2].data_out, regs[3].data_out);
        rd_out = rd_mux.output;
        rs_out = rs_mux.output;
    }

    // Write data into one register on the rising clock edge.
    // The decoder converts the 2-bit sel into a one-hot signal,
    // so only the selected register's load enable goes high.
    // write_en gates the whole thing — if false, nothing writes.
    void write(bool clk, const std::array<bool, 2>& sel,
               bool write_en, const std::array<bool, 8>& data) {
        dec.decode(sel, write_en);
        for (int i = 0; i < 4; i++) {
            regs[i].clock(clk, dec.outputs[i], data);
        }
    }

    uint8_t get_reg(int i) const {
        uint8_t val = 0;
        for (int b = 0; b < 8; b++)
            if (regs[i].data_out[b]) val |= (1 << b);
        return val;
    }

private:
    Register<8> regs[4];
    Decoder<2> dec;
    Mux4<8> rd_mux;
    Mux4<8> rs_mux;
};
