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

// IVT sits at 0xFF00. Each entry is 2 bytes (16-bit handler address).
// Interrupt 0 = reset, 1 = timer, 2 = software interrupt (syscall)
static constexpr uint16_t IVT_BASE = 0xFF00;
static constexpr int MAX_INTERRUPTS = 8;

class CPU {
public:
    CPU(Bus& bus) : bus(bus) {}

    void reset() {
        pc.reset();
        sp = 0xFFFF;
        halted = false;
        int_enabled = false;
        int_pending = 0;
    }

    bool is_halted() const { return halted; }

    // External devices call this to raise an interrupt
    void raise_interrupt(uint8_t num) {
        if (num < MAX_INTERRUPTS) int_pending |= (1 << num);
    }

    void step() {
        if (halted) return;

        // Check for pending interrupts before fetch
        if (int_enabled && int_pending) {
            for (int i = 0; i < MAX_INTERRUPTS; i++) {
                if (int_pending & (1 << i)) {
                    int_pending &= ~(1 << i);
                    handle_interrupt(i);
                    return;
                }
            }
        }

        fetch();
        auto ctrl = decode();
        execute(ctrl);
    }

    uint8_t get_reg(int i) const { return reg_file.get_reg(i); }
    uint16_t get_pc() const { return pc.to_int(); }
    bool get_zero() const { return flags.zero; }
    bool get_carry() const { return flags.carry; }
    uint16_t get_sp() const { return sp; }
    bool get_int_enabled() const { return int_enabled; }

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
    bool int_enabled = false;
    uint8_t int_pending = 0;

    void handle_interrupt(uint8_t num) {
        // Save state: push flags byte then PC
        uint8_t flags_byte = (flags.zero ? 1 : 0) | (flags.carry ? 2 : 0) | (int_enabled ? 4 : 0);
        push16(pc.to_int());
        push_byte(flags_byte);
        int_enabled = false;
        // Read handler address from IVT
        uint16_t handler = bus.read_byte(IVT_BASE + num * 2)
                         | (bus.read_byte(IVT_BASE + num * 2 + 1) << 8);
        jump_to(to_bits16(handler));
    }

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
        uint8_t rd_field = bits_to_int(ir.rd());

        if (op == 0x0) {
            switch (rs_field) {
                case 0:
                    // Sub-ops via Rd: 0=NOP, 1=CLI, 2=STI, 3=RTI
                    switch (rd_field) {
                        case 1: int_enabled = false; return;  // CLI
                        case 2: int_enabled = true; return;   // STI
                        case 3: {  // RTI
                            uint8_t flags_byte = pop_byte();
                            uint16_t ret_addr = pop16();
                            flags.zero = flags_byte & 1;
                            flags.carry = (flags_byte >> 1) & 1;
                            int_enabled = (flags_byte >> 2) & 1;
                            jump_to(to_bits16(ret_addr));
                            return;
                        }
                        default: return;  // NOP
                    }
                case 1: push_byte(from_bits8(reg_file.rd_out)); return;  // PUSH
                case 2: write_reg(ir.rd(), to_bits8(pop_byte())); return;  // POP
                case 3:
                    if (rd_field == 1) {
                        // SWI: software interrupt using imm8 as interrupt number
                        uint8_t int_num = from_bits8(ir.imm8());
                        handle_interrupt(int_num);
                        return;
                    }
                    // RET
                    jump_to(to_bits16(pop16()));
                    return;
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
