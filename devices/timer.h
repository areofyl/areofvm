#pragma once
#include "../cpu/cpu.h"
#include <cstdint>

// Timer device. Counts down each tick, fires interrupt 1 at zero.
//
// Registers (I/O offsets from timer base):
//   0: reload value — counter resets to this after firing
//   1: status/control — bit 0: fired (write 0 to clear), bit 1: enable

class Timer {
public:
    Timer(CPU& cpu) : cpu(cpu) {}

    void write_reg(uint8_t reg, uint8_t val) {
        if (reg == 0) {
            reload = val;
            counter = val;
        } else if (reg == 1) {
            if (!(val & 1)) fired = false;   // write bit 0 = 0 to ack
            enabled = (val >> 1) & 1;         // bit 1 = enable
        }
    }

    uint8_t read_reg(uint8_t reg) {
        if (reg == 0) return counter;
        if (reg == 1) return (fired ? 1 : 0) | (enabled ? 2 : 0);
        return 0;
    }

    void tick() {
        if (!enabled) return;
        if (counter > 0) counter--;
        if (counter == 0) {
            fired = true;
            cpu.raise_interrupt(1);
            counter = reload;
        }
    }

private:
    CPU& cpu;
    uint8_t reload = 0;
    uint8_t counter = 0;
    bool enabled = false;
    bool fired = false;
};
