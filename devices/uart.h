#pragma once
#include "../cpu/cpu.h"
#include <cstdint>
#include <queue>
#include <string>

// UART — simple serial character I/O device.
//
// Registers (I/O offsets from UART base):
//   0: data — write to transmit, read to receive
//   1: status — bit 0: RX data available, bit 1: TX ready (always 1)
//
// When a character is pushed into the RX buffer from the outside,
// the device raises interrupt 2 so the CPU knows to read it.
// Reading the data register when RX has data implicitly consumes
// one character; if the buffer becomes empty, bit 0 clears.

class UART {
public:
    UART(CPU& cpu) : cpu(cpu) {}

    void write_reg(uint8_t reg, uint8_t val) {
        if (reg == 0) {
            tx_buf.push(val);
        }
    }

    uint8_t read_reg(uint8_t reg) {
        if (reg == 0) {
            if (rx_buf.empty()) return 0;
            uint8_t ch = rx_buf.front();
            rx_buf.pop();
            return ch;
        }
        if (reg == 1) {
            uint8_t status = 0;
            if (!rx_buf.empty()) status |= 1;  // bit 0: RX ready
            status |= 2;                        // bit 1: TX ready (always)
            return status;
        }
        return 0;
    }

    // --- Host-side API (used by test harness / emulator) ---

    // Push a single character into RX (raises interrupt 2)
    void send_char(uint8_t ch) {
        rx_buf.push(ch);
        cpu.raise_interrupt(2);
    }

    // Push a whole string into RX (one interrupt per char)
    void send_string(const std::string& s) {
        for (uint8_t ch : s) send_char(ch);
    }

    // Pull one character from TX output
    bool has_output() const { return !tx_buf.empty(); }

    uint8_t recv_char() {
        if (tx_buf.empty()) return 0;
        uint8_t ch = tx_buf.front();
        tx_buf.pop();
        return ch;
    }

    // Drain the entire TX buffer as a string
    std::string recv_string() {
        std::string out;
        while (!tx_buf.empty()) {
            out += static_cast<char>(tx_buf.front());
            tx_buf.pop();
        }
        return out;
    }

private:
    CPU& cpu;
    std::queue<uint8_t> rx_buf;
    std::queue<uint8_t> tx_buf;
};
