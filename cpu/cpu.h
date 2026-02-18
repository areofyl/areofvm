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

// Convert a 2-bit or 4-bit bool array to an integer
template <size_t N>
inline uint8_t bits_to_int(const std::array<bool, N>& bits) {
    uint8_t val = 0;
    for (size_t i = 0; i < N; i++) if (bits[i]) val |= (1 << i);
    return val;
}

// CPU — the central processing unit.
//
// Instruction set (updated):
//   0x0 Rs=0: NOP        0x0 Rs=1: PUSH Rd
//   0x0 Rs=2: POP Rd     0x0 Rs=3: RET
//   0x1: LDI Rd, imm     0x2: LD Rd, [Rs]
//   0x3: ST [Rd], Rs     0x4: ADD Rd, Rs
//   0x5: SUB Rd, Rs      0x6: AND Rd, Rs
//   0x7: OR Rd, Rs       0x8: MOV Rd, Rs
//   0x9: CMP Rd, Rs      0xA: JMP imm
//   0xB: JZ imm          0xC: JNZ imm
//   0xD: ADDI Rd, imm    0xE: CALL imm
//   0xF: HLT
//
// Each step() call executes exactly one instruction.

class CPU {
public:
    CPU(Bus& bus) : bus(bus) {}

    void reset() {
        pc.reset();
        sp = 0xFF;
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
    uint8_t get_sp() const { return sp; }

private:
    Bus& bus;
    ProgramCounter pc;
    InstructionRegister ir;
    RegisterFile reg_file;
    ALU<8> alu;
    Flags flags;
    ControlUnit control;
    bool halted = false;
    uint8_t sp = 0xFF;

    void jump_to(const std::array<bool, 8>& addr) {
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

    void fetch() {
        uint8_t addr = pc.to_int();
        auto lo_byte = to_bits8(bus.read_byte(addr));
        auto hi_byte = to_bits8(bus.read_byte(addr + 1));

        ir.load_lo(false, true, lo_byte);
        ir.load_lo(true, true, lo_byte);
        ir.load_hi(false, true, hi_byte);
        ir.load_hi(true, true, hi_byte);

        std::array<bool, 8> unused = {};
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

        // Stack and flow control instructions (handled directly)
        if (op == 0x0) {
            switch (rs_field) {
                case 1: push_byte(from_bits8(reg_file.rd_out)); return;  // PUSH
                case 2: write_reg(ir.rd(), to_bits8(pop_byte())); return;  // POP
                case 3: jump_to(to_bits8(pop_byte())); return;  // RET
                default: return;  // NOP
            }
        }
        if (op == 0xE) {  // CALL
            push_byte(pc.to_int());
            jump_to(ir.imm8());
            return;
        }

        // ALU path
        Mux2<8> alu_b_mux;
        alu_b_mux.select(s.alu_src_imm, reg_file.rs_out, ir.imm8());
        alu.compute(reg_file.rd_out, alu_b_mux.output, s.alu_op0, s.alu_op1);

        // Memory
        std::array<bool, 8> mem_data = {};
        if (s.mem_read)  mem_data = to_bits8(bus.read_byte(from_bits8(reg_file.rs_out)));
        if (s.mem_write) bus.write_byte(from_bits8(reg_file.rd_out), from_bits8(reg_file.rs_out));

        // Writeback mux
        std::array<bool, 8> write_data = alu.result;
        if (s.reg_src_mem) write_data = mem_data;
        else if (s.reg_src_imm) write_data = ir.imm8();
        else if (s.is_mov) write_data = reg_file.rs_out;

        if (s.reg_write) write_reg(ir.rd(), write_data);
        if (s.flags_write) {
            flags.update(false, true, alu.carry, alu.zero);
            flags.update(true, true, alu.carry, alu.zero);
        }
        if (s.pc_jump) jump_to(ir.imm8());
        if (s.halt) halted = true;
    }
};
