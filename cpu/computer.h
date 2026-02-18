#pragma once
#include "cpu.h"
#include "../memory/bus.h"
#include "../devices/timer.h"
#include <cstdint>
#include <cstddef>

// Computer — the top-level system.
// Owns Bus, CPU, and Timer. Wires them together.

class Computer {
public:
    Computer() : cpu(bus), timer(cpu) {
        cpu.reset();
        // Wire timer into I/O space (registers at offset 0x00-0x01)
        bus.attach_io(
            [this](uint32_t addr) -> uint8_t { return timer.read_reg(addr); },
            [this](uint32_t addr, uint8_t val) { timer.write_reg(addr, val); }
        );
    }

    void load_program(const uint8_t* data, size_t length, uint32_t addr = 0) {
        bus.load(addr, data, length);
    }

    void run(int max_cycles = 10000) {
        while (!cpu.is_halted() && max_cycles-- > 0) {
            timer.tick();
            cpu.step();
        }
    }

    void step() {
        timer.tick();
        cpu.step();
    }

    void reset() { cpu.reset(); }

    CPU& get_cpu() { return cpu; }
    Bus& get_bus() { return bus; }
    Timer& get_timer() { return timer; }

private:
    Bus bus;
    CPU cpu;
    Timer timer;
};
