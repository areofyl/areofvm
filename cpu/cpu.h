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

// Helper: integer to 8-bit bool array (LSB at index 0)
inline std::array<bool, 8> to_bits8(uint8_t val) {
    std::array<bool, 8> bits = {};
    for (int i = 0; i < 8; i++) bits[i] = (val >> i) & 1;
    return bits;
}

// Helper: 8-bit bool array to integer
inline uint8_t from_bits8(const std::array<bool, 8>& bits) {
    uint8_t val = 0;
    for (int i = 0; i < 8; i++) if (bits[i]) val |= (1 << i);
    return val;
}

class CPU {
public:
    CPU(Bus& bus) : bus(bus) {}

    void reset() {
        pc.reset();
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
    uint8_t get_pc() const { return pc.to_int(); }
    bool get_zero() const { return flags.zero; }
    bool get_carry() const { return flags.carry; }

private:
    Bus& bus;
    ProgramCounter pc;
    InstructionRegister ir;
    RegisterFile reg_file;
    ALU<8> alu;
    Flags flags;
    ControlUnit control;
    bool halted = false;

    void fetch() {
        uint8_t addr = pc.to_int();
        auto lo_byte = to_bits8(bus.read_byte(addr));
        auto hi_byte = to_bits8(bus.read_byte(addr + 1));

        // Load into instruction register (clock low then high)
        ir.load_lo(false, true, lo_byte);
        ir.load_lo(true, true, lo_byte);
        ir.load_hi(false, true, hi_byte);
        ir.load_hi(true, true, hi_byte);

        // Increment PC by 2
        std::array<bool, 8> dummy = {};
        pc.clock(false, false, dummy);
        pc.clock(true, false, dummy);
    }

    ControlSignals decode() {
        control.decode(ir.opcode(), flags.zero);
        reg_file.read(ir.rd(), ir.rs());
        return control.signals;
    }

    void execute(const ControlSignals& s) {
        // Determine ALU's second operand
        Mux2<8> alu_b_mux;
        alu_b_mux.select(s.alu_src_imm, reg_file.rs_out, ir.imm8());

        // Run the ALU
        alu.compute(reg_file.rd_out, alu_b_mux.output, s.alu_op0, s.alu_op1);

        // Memory read (for LD instruction)
        std::array<bool, 8> mem_data = {};
        if (s.mem_read) {
            mem_data = to_bits8(bus.read_byte(from_bits8(reg_file.rs_out)));
        }

        // Memory write (for ST instruction)
        if (s.mem_write) {
            bus.write_byte(from_bits8(reg_file.rd_out), from_bits8(reg_file.rs_out));
        }

        // Determine what gets written back to register file
        // Priority: mem > imm > mov > alu
        std::array<bool, 8> write_data = alu.result;
        if (s.reg_src_mem) write_data = mem_data;
        else if (s.reg_src_imm) write_data = ir.imm8();
        else if (s.is_mov) write_data = reg_file.rs_out;

        // Write to register file
        if (s.reg_write) {
            reg_file.write(false, ir.rd(), true, write_data);
            reg_file.write(true, ir.rd(), true, write_data);
        }

        // Update flags
        if (s.flags_write) {
            flags.update(false, true, alu.carry, alu.zero);
            flags.update(true, true, alu.carry, alu.zero);
        }

        // Jump
        if (s.pc_jump) {
            pc.clock(false, true, ir.imm8());
            pc.clock(true, true, ir.imm8());
        }

        // I/O output
        if (s.io_write) {
            bus.write_byte(Bus::IO_BASE, from_bits8(reg_file.rd_out));
        }

        // Halt
        if (s.halt) {
            halted = true;
        }
    }
};
