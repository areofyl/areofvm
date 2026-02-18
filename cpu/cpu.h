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

static constexpr uint16_t IVT_BASE = 0xEFF0;
static constexpr int MAX_INTERRUPTS = 8;

// 8-bit CPU with 16-bit address space and interrupt support.
//
// Opcode 0x0 sub-instructions:
//   Rs=0: Rd=0 NOP, Rd=1 CLI, Rd=2 STI, Rd=3 RTI
//   Rs=1: PUSH Rd
//   Rs=2: POP Rd
//   Rs=3: Rd=0 RET, Rd=1 SWI (imm8 = interrupt number)

class CPU {
public:
    CPU(Bus& bus) : bus(bus) {}

    void reset() {
        pc.reset();
        sp = 0xEFFF;
        halted = false;
        int_enabled = false;
        int_pending = 0;
    }

    bool is_halted() const { return halted; }

    void raise_interrupt(uint8_t num) {
        if (num < MAX_INTERRUPTS) int_pending |= (1 << num);
    }

    void step() {
        if (halted) return;
        if (check_interrupts()) return;
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
    uint16_t sp = 0xEFFF;
    bool int_enabled = false;
    uint8_t int_pending = 0;

    // --- Interrupt handling ---

    bool check_interrupts() {
        if (!int_enabled || !int_pending) return false;
        for (int i = 0; i < MAX_INTERRUPTS; i++) {
            if (int_pending & (1 << i)) {
                int_pending &= ~(1 << i);
                enter_interrupt(i);
                return true;
            }
        }
        return false;
    }

    void enter_interrupt(uint8_t num) {
        // Pack interrupt-enable into flags byte (bit 2)
        uint8_t saved_flags = flags.pack() | (int_enabled ? 4 : 0);
        push16(pc.to_int());
        push_byte(saved_flags);
        int_enabled = false;
        uint16_t handler = bus.read_byte(IVT_BASE + num * 2)
                         | (bus.read_byte(IVT_BASE + num * 2 + 1) << 8);
        jump_to(to_bits16(handler));
    }

    void return_from_interrupt() {
        uint8_t saved_flags = pop_byte();
        uint16_t ret_addr = pop16();
        flags.unpack(saved_flags);
        int_enabled = (saved_flags >> 2) & 1;
        jump_to(to_bits16(ret_addr));
    }

    // --- Helpers ---

    void jump_to(const std::array<bool, 16>& addr) {
        pc.clock(false, true, addr);
        pc.clock(true, true, addr);
    }

    void write_reg(const std::array<bool, 2>& sel, const std::array<bool, 8>& data) {
        reg_file.write(false, sel, true, data);
        reg_file.write(true, sel, true, data);
    }

    void push_byte(uint8_t val) { sp--; bus.write_byte(sp, val); }
    uint8_t pop_byte() { uint8_t v = bus.read_byte(sp); sp++; return v; }

    void push16(uint16_t val) {
        push_byte((val >> 8) & 0xFF);
        push_byte(val & 0xFF);
    }

    uint16_t pop16() {
        uint16_t lo = pop_byte();
        uint16_t hi = pop_byte();
        return (hi << 8) | lo;
    }

    // --- Fetch / Decode / Execute ---

    void fetch() {
        uint16_t addr = pc.to_int();
        auto b0 = to_bits8(bus.read_byte(addr));
        auto b1 = to_bits8(bus.read_byte(addr + 1));
        auto b2 = to_bits8(bus.read_byte(addr + 2));

        ir.load_byte0(false, true, b0); ir.load_byte0(true, true, b0);
        ir.load_byte1(false, true, b1); ir.load_byte1(true, true, b1);
        ir.load_byte2(false, true, b2); ir.load_byte2(true, true, b2);

        std::array<bool, 16> unused = {};
        pc.clock(false, false, unused);
        pc.clock(true, false, unused);
    }

    ControlSignals decode() {
        control.decode(ir.opcode(), flags.zero);
        reg_file.read(ir.rd(), ir.rs());
        return control.signals;
    }

    // Opcode 0x0 sub-dispatch
    void execute_misc() {
        uint8_t rs = bits_to_int(ir.rs());
        uint8_t rd = bits_to_int(ir.rd());

        switch (rs) {
            case 0:
                switch (rd) {
                    case 1: int_enabled = false; return;       // CLI
                    case 2: int_enabled = true; return;        // STI
                    case 3: return_from_interrupt(); return;    // RTI
                    default: return;                            // NOP
                }
            case 1: push_byte(from_bits8(reg_file.rd_out)); return;       // PUSH
            case 2: write_reg(ir.rd(), to_bits8(pop_byte())); return;     // POP
            case 3:
                if (rd == 1) { enter_interrupt(from_bits8(ir.imm8())); return; }  // SWI
                jump_to(to_bits16(pop16())); return;                               // RET
        }
    }

    void execute(const ControlSignals& s) {
        uint8_t op = bits_to_int(ir.opcode());

        if (op == 0x0) { execute_misc(); return; }
        if (op == 0xE) { push16(pc.to_int()); jump_to(ir.imm16()); return; }  // CALL

        // ALU
        Mux2<8> alu_b_mux;
        alu_b_mux.select(s.alu_src_imm, reg_file.rs_out, ir.imm8());
        alu.compute(reg_file.rd_out, alu_b_mux.output, s.alu_op0, s.alu_op1);

        // Memory
        std::array<bool, 8> mem_data = {};
        if (s.mem_read)  mem_data = to_bits8(bus.read_byte(from_bits16(ir.imm16())));
        if (s.mem_write) bus.write_byte(from_bits16(ir.imm16()), from_bits8(reg_file.rd_out));

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
        if (s.pc_jump) jump_to(ir.imm16());
        if (s.halt) halted = true;
    }
};
