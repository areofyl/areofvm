#pragma once
#include "memory.h"
#include <cstdint>
#include <functional>

// System Bus — routes CPU reads/writes to RAM or I/O devices.
//
// Memory map (64 KB, matching the CPU's 16-bit address space):
//   0x0000 – 0xEFFF  (60 KB)  General-purpose RAM
//   0xF000 – 0xFFFF  (4 KB)   Memory-mapped I/O
//
// When the CPU accesses an address in the I/O region,
// the bus calls the registered device handler instead of RAM.

class Bus {
public:
    static constexpr uint32_t IO_BASE = 0xF000;
    static constexpr uint32_t IO_SIZE = 0x1000;   // 4 KB
    static constexpr uint32_t RAM_SIZE = IO_BASE;  // 60 KB

    using IoReadFn  = std::function<uint8_t(uint32_t addr)>;
    using IoWriteFn = std::function<void(uint32_t addr, uint8_t value)>;

    Bus() = default;

    void attach_io(IoReadFn read_fn, IoWriteFn write_fn) {
        io_read  = read_fn;
        io_write = write_fn;
    }

    uint8_t read_byte(uint32_t addr) const {
        addr &= 0xFFFF;  // wrap to 16-bit
        if (addr >= IO_BASE) {
            if (io_read) return io_read(addr - IO_BASE);
            return 0;
        }
        return ram.read_byte(addr);
    }

    void write_byte(uint32_t addr, uint8_t value) {
        addr &= 0xFFFF;
        if (addr >= IO_BASE) {
            if (io_write) io_write(addr - IO_BASE, value);
            return;
        }
        ram.write_byte(addr, value);
    }

    uint16_t read_word(uint32_t addr) const {
        uint8_t lo = read_byte(addr);
        uint8_t hi = read_byte(addr + 1);
        return (hi << 8) | lo;
    }

    void write_word(uint32_t addr, uint16_t value) {
        write_byte(addr, value & 0xFF);
        write_byte(addr + 1, (value >> 8) & 0xFF);
    }

    void load(uint32_t start_addr, const uint8_t* data, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            write_byte(start_addr + i, data[i]);
        }
    }

    Memory& get_ram() { return ram; }

private:
    Memory ram;
    IoReadFn  io_read;
    IoWriteFn io_write;
};
