#pragma once
#include "cpu.h"
#include "../memory/bus.h"
#include <cstdint>
#include <cstddef>

// Top level: owns the bus and CPU, provides program loading and execution.

class Computer {
public:
    Computer() : cpu(bus) {
        cpu.reset();
    }

    void load_program(const uint8_t* data, size_t length, uint32_t addr = 0) {
        bus.load(addr, data, length);
    }

    void run(int max_cycles = 10000) {
        while (!cpu.is_halted() && max_cycles-- > 0) {
            cpu.step();
        }
    }

    void step() { cpu.step(); }
    void reset() { cpu.reset(); }

    CPU& get_cpu() { return cpu; }
    Bus& get_bus() { return bus; }

private:
    Bus bus;
    CPU cpu;
};
