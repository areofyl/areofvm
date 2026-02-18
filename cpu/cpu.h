#pragma once
#include "register_file.h"
#include "program_counter.h"
#include "instruction_register.h"
#include "flags.h"
#include "control_unit.h"
#include "../arithmetic/alu.h"
#include "../arithmetic/mux.h"
#include "../memory/bus.h"
#include <array>
#include <cstdint>

inline std::array<bool, 8> to_bits8(uint8_t val) {
    std::array<bool, 8> bits = {};
    for (int i = 0; i < 8; i++) bits[i] = (val >> i) & 1;
    return bits;
}

inline uint8_t from_bits8(const std::array<bool, 8>& bits) {
    uint8_t val = 0;
    for (int i = 0; i < 8; i++) if (bits[i]) val |= (1 << i);
    return val;
}

inline std::array<bool, 16> to_bits16(uint16_t val) {
    std::array<bool, 16> bits = {};
    for (int i = 0; i < 16; i++) bits[i] = (val >> i) & 1;
    return bits;
}

inline uint16_t from_bits16(const std::array<bool, 16>& bits) {
    uint16_t val = 0;
    for (int i = 0; i < 16; i++) if (bits[i]) val |= (1 << i);
    return val;
}

template <size_t N>
inline uint16_t bits_to_int(const std::array<bool, N>& bits) {
    uint16_t val = 0;
    for (size_t i = 0; i < N; i++) if (bits[i]) val |= (1 << i);
    return val;
}

// 8-bit CPU with 16-bit address space.
// 24-bit instructions (3 bytes): [opcode 4][rd 2][rs 2][imm16]
// 64KB addressable memory. SP and PC are 16-bit.

class CPU {
public:
    CPU(Bus& bus) : bus(bus) {}

    void reset() {
        pc.reset();
        sp = 0xFFFF;
        halted = false;
    }

    bool is_halted() const { return halted; }

    void step() {
        if (halted) return;
        fetch();
        auto ctrl = decode();
        execute(ctrl);
    }

    uint8_t get_reg(int i) const { return reg_file.get_reg(i); }
    uint16_t get_pc() const { return pc.to_int(); }
    bool get_zero() const { return flags.zero; }
    bool get_carry() const { return flags.carry; }
    uint16_t get_sp() const { return sp; }

private:
    Bus& bus;
    ProgramCounter pc;
    InstructionRegister ir;
    RegisterFile reg_file;
    ALU<8> alu;
    Flags flags;
    ControlUnit control;
    bool halted = false;
    uint16_t sp = 0xFFFF;

    void jump_to(const std::array<bool, 16>& addr) {
        pc.clock(false, true, addr);
        pc.clock(true, true, addr);
    }

    void write_reg(const std::array<bool, 2>& sel, const std::array<bool, 8>& data) {
        reg_file.write(false, sel, true, data);
        reg_file.write(true, sel, true, data);
    }

    void push_byte(uint8_t val) {
        sp--;
        bus.write_byte(sp, val);
    }

    uint8_t pop_byte() {
        uint8_t val = bus.read_byte(sp);
        sp++;
        return val;
    }

    // Push 16-bit value (high byte first so low byte is at lower address)
    void push16(uint16_t val) {
        push_byte((val >> 8) & 0xFF);
        push_byte(val & 0xFF);
    }

    uint16_t pop16() {
        uint16_t lo = pop_byte();
        uint16_t hi = pop_byte();
        return (hi << 8) | lo;
    }

    void fetch() {
        uint16_t addr = pc.to_int();
        auto b0 = to_bits8(bus.read_byte(addr));
        auto b1 = to_bits8(bus.read_byte(addr + 1));
        auto b2 = to_bits8(bus.read_byte(addr + 2));

        ir.load_byte0(false, true, b0);
        ir.load_byte0(true, true, b0);
        ir.load_byte1(false, true, b1);
        ir.load_byte1(true, true, b1);
        ir.load_byte2(false, true, b2);
        ir.load_byte2(true, true, b2);

        std::array<bool, 16> unused = {};
        pc.clock(false, false, unused);
        pc.clock(true, false, unused);
    }

    ControlSignals decode() {
        control.decode(ir.opcode(), flags.zero);
        reg_file.read(ir.rd(), ir.rs());
        return control.signals;
    }

    void execute(const ControlSignals& s) {
        uint8_t op = bits_to_int(ir.opcode());
        uint8_t rs_field = bits_to_int(ir.rs());

        if (op == 0x0) {
            switch (rs_field) {
                case 1: push_byte(from_bits8(reg_file.rd_out)); return;
                case 2: write_reg(ir.rd(), to_bits8(pop_byte())); return;
                case 3: jump_to(to_bits16(pop16())); return;
                default: return;
            }
        }
        if (op == 0xE) {
            push16(pc.to_int());
            jump_to(ir.imm16());
            return;
        }

        Mux2<8> alu_b_mux;
        alu_b_mux.select(s.alu_src_imm, reg_file.rs_out, ir.imm8());
        alu.compute(reg_file.rd_out, alu_b_mux.output, s.alu_op0, s.alu_op1);

        // LD/ST now use imm16 as address
        std::array<bool, 8> mem_data = {};
        if (s.mem_read)  mem_data = to_bits8(bus.read_byte(from_bits16(ir.imm16())));
        if (s.mem_write) bus.write_byte(from_bits16(ir.imm16()), from_bits8(reg_file.rd_out));

        std::array<bool, 8> write_data = alu.result;
        if (s.reg_src_mem) write_data = mem_data;
        else if (s.reg_src_imm) write_data = ir.imm8();
        else if (s.is_mov) write_data = reg_file.rs_out;

        if (s.reg_write) write_reg(ir.rd(), write_data);
        if (s.flags_write) {
            flags.update(false, true, alu.carry, alu.zero);
            flags.update(true, true, alu.carry, alu.zero);
        }
        if (s.pc_jump) jump_to(ir.imm16());
        if (s.halt) halted = true;
    }
};
