#pragma once
#include "memory.h"
#include <cstdint>
#include <functional>

// System Bus — routes CPU reads/writes to RAM or I/O devices.
//
// Memory map (1 MB address space):
//   0x00000 – 0xEFFFF  (960 KB)  General-purpose RAM
//   0xF0000 – 0xFFFFF  (64 KB)   Memory-mapped I/O
//
// The CPU never talks to RAM directly — it goes through the bus.
// This is how real hardware works: the bus decodes the address and
// decides who handles the request (RAM, ROM, I/O controller, etc.).
//
// I/O callbacks:
//   Devices register read/write handlers. When the CPU accesses an
//   address in the I/O region, the bus calls the appropriate handler.
//   This is how you'll plug in a console, timer, disk, etc.

class Bus {
public:
    static constexpr uint32_t IO_BASE = 0xF0000;
    static constexpr uint32_t IO_SIZE = 0x10000;  // 64 KB
    static constexpr uint32_t RAM_SIZE = IO_BASE;  // 960 KB

    using IoReadFn  = std::function<uint8_t(uint32_t addr)>;
    using IoWriteFn = std::function<void(uint32_t addr, uint8_t value)>;

    Bus() = default;

    // Register an I/O device handler for a region within the I/O space.
    // addr is relative to IO_BASE.
    void attach_io(IoReadFn read_fn, IoWriteFn write_fn) {
        io_read  = read_fn;
        io_write = write_fn;
    }

    // --- Byte access ---

    uint8_t read_byte(uint32_t addr) const {
        addr &= (Memory::SIZE - 1);  // wrap to 20-bit address space

        if (addr >= IO_BASE) {
            if (io_read) return io_read(addr - IO_BASE);
            return 0;
        }
        return ram.read_byte(addr);
    }

    void write_byte(uint32_t addr, uint8_t value) {
        addr &= (Memory::SIZE - 1);

        if (addr >= IO_BASE) {
            if (io_write) io_write(addr - IO_BASE, value);
            return;
        }
        ram.write_byte(addr, value);
    }

    // --- Word access (16-bit, little-endian) ---

    uint16_t read_word(uint32_t addr) const {
        uint8_t lo = read_byte(addr);
        uint8_t hi = read_byte(addr + 1);
        return (hi << 8) | lo;
    }

    void write_word(uint32_t addr, uint16_t value) {
        write_byte(addr, value & 0xFF);
        write_byte(addr + 1, (value >> 8) & 0xFF);
    }

    // --- Bulk load (for loading programs) ---

    void load(uint32_t start_addr, const uint8_t* data, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            write_byte(start_addr + i, data[i]);
        }
    }

    // Direct RAM access (for testing/debugging)
    Memory& get_ram() { return ram; }

private:
    Memory ram;
    IoReadFn  io_read;
    IoWriteFn io_write;
};
