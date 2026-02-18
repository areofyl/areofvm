#pragma once
#include "../cpu/cpu.h"
#include <cstdint>
#include <queue>

// UART — simple serial character I/O device.
//
// Registers (I/O offsets from UART base):
//   0: data — write to transmit, read to receive
//   1: status — bit 0: RX data available, bit 1: TX ready (always 1)
//
// When a character is pushed into the RX buffer from the outside,
// the device raises interrupt 2 so the CPU knows to read it.

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

    // Host side: push a character into the RX buffer (simulates typing)
    void send_char(uint8_t ch) {
        rx_buf.push(ch);
        cpu.raise_interrupt(2);
    }

    // Host side: pull a character from the TX buffer (simulates screen)
    bool has_output() const { return !tx_buf.empty(); }

    uint8_t recv_char() {
        if (tx_buf.empty()) return 0;
        uint8_t ch = tx_buf.front();
        tx_buf.pop();
        return ch;
    }

private:
    CPU& cpu;
    std::queue<uint8_t> rx_buf;
    std::queue<uint8_t> tx_buf;
};
