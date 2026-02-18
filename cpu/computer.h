#pragma once
#include "cpu.h"
#include "../memory/bus.h"
#include <cstdint>
#include <cstddef>
#include <functional>

// Computer — the top-level system.
//
// Owns the Bus (which owns RAM) and the CPU (which connects to the Bus).
// This is the "motherboard" — the thing you interact with from the outside.
//
// Usage:
//   Computer comp;
//   comp.load_program(bytes, length);  // load machine code into memory
//   comp.run();                        // execute until HLT or cycle limit
//
// For I/O, attach handlers via get_bus().attach_io() before running.
// The OUT instruction writes to the I/O address space, which triggers
// whatever callback you registered.

class Computer {
public:
    Computer() : cpu(bus) {
        cpu.reset();
    }

    // Load raw bytes into memory at the given address (default: 0).
    // Programs start executing from address 0.
    void load_program(const uint8_t* data, size_t length, uint32_t addr = 0) {
        bus.load(addr, data, length);
    }

    // Run until HLT instruction or max_cycles reached.
    void run(int max_cycles = 10000) {
        while (!cpu.is_halted() && max_cycles-- > 0) {
            cpu.step();
        }
    }

    // Execute a single instruction (for debugging/stepping).
    void step() { cpu.step(); }

    void reset() { cpu.reset(); }

    CPU& get_cpu() { return cpu; }
    Bus& get_bus() { return bus; }

private:
    Bus bus;
    CPU cpu;
};
