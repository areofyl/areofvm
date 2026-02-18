#pragma once
#include "cpu.h"
#include "../memory/bus.h"
#include "../devices/timer.h"
#include "../devices/uart.h"
#include <cstdint>
#include <cstddef>

// Computer — the top-level system.
// Owns Bus, CPU, Timer, and UART. Wires them together.
//
// I/O address map (offsets from 0xF000):
//   0x00-0x01  Timer (reload, control)
//   0x02-0x03  UART  (data, status)

class Computer {
public:
    Computer() : cpu(bus), timer(cpu), uart(cpu) {
        cpu.reset();
        bus.attach_io(
            [this](uint32_t addr) -> uint8_t {
                if (addr < 2) return timer.read_reg(addr);
                if (addr < 4) return uart.read_reg(addr - 2);
                return 0;
            },
            [this](uint32_t addr, uint8_t val) {
                if (addr < 2) timer.write_reg(addr, val);
                else if (addr < 4) uart.write_reg(addr - 2, val);
            }
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
    UART& get_uart() { return uart; }

private:
    Bus bus;
    CPU cpu;
    Timer timer;
    UART uart;
};
