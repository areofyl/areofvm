#pragma once
#include "../gates/gates.h"
#include <array>
#include <vector>
#include <cstdint>

// Memory — 1 MB (1,048,576 bytes), 20-bit address bus, 8-bit data bus.
//
// In real hardware this would be a grid of flip-flops with a decoder selecting
// which row to read/write. Simulating 8 million flip-flops would eat ~4 GB of
// host RAM, so the storage is a plain byte array — but the interface matches
// what the gate-level version would look like.
//
// Two interfaces:
//   1. clock()      — gate-level: bool arrays for address/data, rising-edge writes
//   2. read/write   — direct byte/word access for the CPU to use at speed

class Memory {
public:
    static constexpr int ADDR_BITS = 20;
    static constexpr int DATA_BITS = 8;
    static constexpr int SIZE = 1 << ADDR_BITS;  // 1,048,576 bytes

    std::array<bool, DATA_BITS> data_out = {};

    Memory() : storage(SIZE, 0) {}

    // --- Gate-level interface ---

    void clock(bool clk,
               bool write_en,
               const std::array<bool, ADDR_BITS>& address,
               const std::array<bool, DATA_BITS>& data_in)
    {
        int addr = addr_to_int(address);

        // Read: always output the value at the address
        uint8_t byte = storage[addr];
        for (int i = 0; i < DATA_BITS; i++) {
            data_out[i] = (byte >> i) & 1;
        }

        // Write: on rising edge when write_enable is high
        bool rising_edge = gate::AND(clk, gate::NOT(prev_clk));
        if (gate::AND(rising_edge, write_en)) {
            storage[addr] = data_to_byte(data_in);
        }

        prev_clk = clk;
    }

    // --- Direct interface (used by CPU) ---

    uint8_t read_byte(uint32_t addr) const {
        return storage[addr & (SIZE - 1)];
    }

    void write_byte(uint32_t addr, uint8_t value) {
        storage[addr & (SIZE - 1)] = value;
    }

    // 16-bit word access, little-endian (low byte at lower address)
    uint16_t read_word(uint32_t addr) const {
        uint8_t lo = read_byte(addr);
        uint8_t hi = read_byte(addr + 1);
        return (hi << 8) | lo;
    }

    void write_word(uint32_t addr, uint16_t value) {
        write_byte(addr, value & 0xFF);
        write_byte(addr + 1, (value >> 8) & 0xFF);
    }

    // Bulk load — for loading programs into memory
    void load(uint32_t start_addr, const uint8_t* data, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            write_byte(start_addr + i, data[i]);
        }
    }

private:
    std::vector<uint8_t> storage;
    bool prev_clk = false;

    static int addr_to_int(const std::array<bool, ADDR_BITS>& address) {
        int addr = 0;
        for (int i = 0; i < ADDR_BITS; i++) {
            if (address[i]) addr |= (1 << i);
        }
        return addr;
    }

    static uint8_t data_to_byte(const std::array<bool, DATA_BITS>& data) {
        uint8_t byte = 0;
        for (int i = 0; i < DATA_BITS; i++) {
            if (data[i]) byte |= (1 << i);
        }
        return byte;
    }
};
