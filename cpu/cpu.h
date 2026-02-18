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

// Conversion helpers between integer and bool-array representations.
// The CPU components work with std::array<bool, N> (gate-level),
// but the Bus uses uint8_t (byte-level).

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

// CPU — the central processing unit.
//
// Connects all components together and runs the fetch-decode-execute cycle.
//
// Datapath overview:
//
//   [Memory] --bus--> [IR] --opcode--> [Control Unit] --signals--> everything
//                      |                                             |
//                      +--rd/rs--> [Register File] --values--> [ALU]
//                      |                  ^                      |
//                      +--imm8------------|---mux--> ALU input B |
//                                         |                      |
//                                         +---mux--- result ----+
//                                         |
//                                    [Memory data]
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

    // Debug accessors
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
    uint8_t sp = 0xFF;  // stack pointer, starts at top of memory, grows down

    // FETCH: Read the 16-bit instruction at PC from memory.
    // Memory is 8-bit, so we read two bytes: lo at PC, hi at PC+1.
    // Then increment PC by 2 to point at the next instruction.
    void fetch() {
        uint8_t addr = pc.to_int();
        auto lo_byte = to_bits8(bus.read_byte(addr));
        auto hi_byte = to_bits8(bus.read_byte(addr + 1));

        // Clock the IR registers (low then high = rising edge captures)
        ir.load_lo(false, true, lo_byte);
        ir.load_lo(true, true, lo_byte);
        ir.load_hi(false, true, hi_byte);
        ir.load_hi(true, true, hi_byte);

        // PC += 2 (jump=false, so it increments normally)
        std::array<bool, 8> unused = {};
        pc.clock(false, false, unused);
        pc.clock(true, false, unused);
    }

    // DECODE: Extract fields from the instruction and generate control signals.
    // Also reads the register values we'll need in the execute phase.
    ControlSignals decode() {
        control.decode(ir.opcode(), flags.zero);
        reg_file.read(ir.rd(), ir.rs());
        return control.signals;
    }

    // EXECUTE: Carry out the instruction based on control signals.
    void execute(const ControlSignals& s) {
        // Opcode 0x0 is overloaded: Rs field selects sub-operation
        //   Rs=0: NOP, Rs=1: PUSH Rd, Rs=2: POP Rd, Rs=3: RET
        // Opcode 0xE is now CALL imm8 (was OUT)
        uint8_t op = ir.opcode()[0] | (ir.opcode()[1] << 1) | (ir.opcode()[2] << 2) | (ir.opcode()[3] << 3);
        uint8_t rs_field = ir.rs()[0] | (ir.rs()[1] << 1);

        if (op == 0x0) {
            if (rs_field == 1) {
                // PUSH Rd
                sp--;
                bus.write_byte(sp, from_bits8(reg_file.rd_out));
                return;
            }
            if (rs_field == 2) {
                // POP Rd
                auto val = to_bits8(bus.read_byte(sp));
                sp++;
                reg_file.write(false, ir.rd(), true, val);
                reg_file.write(true, ir.rd(), true, val);
                return;
            }
            if (rs_field == 3) {
                // RET: pop return address, jump to it
                uint8_t ret_addr = bus.read_byte(sp);
                sp++;
                auto addr_bits = to_bits8(ret_addr);
                pc.clock(false, true, addr_bits);
                pc.clock(true, true, addr_bits);
                return;
            }
            return; // NOP
        }
        if (op == 0xE) {
            // CALL imm8: push return address (current PC), jump to imm8
            sp--;
            bus.write_byte(sp, pc.to_int());
            pc.clock(false, true, ir.imm8());
            pc.clock(true, true, ir.imm8());
            return;
        }

        // ALU input B: either Rs value (register-register) or imm8 (ADDI)
        Mux2<8> alu_b_mux;
        alu_b_mux.select(s.alu_src_imm, reg_file.rs_out, ir.imm8());

        // Always run the ALU — unused results are simply ignored
        alu.compute(reg_file.rd_out, alu_b_mux.output, s.alu_op0, s.alu_op1);

        // Memory read: LD Rd, [Rs] — use Rs value as memory address
        std::array<bool, 8> mem_data = {};
        if (s.mem_read) {
            mem_data = to_bits8(bus.read_byte(from_bits8(reg_file.rs_out)));
        }

        // Memory write: ST [Rd], Rs — use Rd as address, Rs as data
        if (s.mem_write) {
            bus.write_byte(from_bits8(reg_file.rd_out), from_bits8(reg_file.rs_out));
        }

        // Select what data gets written back to the register file.
        // Default is ALU result. Overridden by specific instructions:
        //   LD  → data from memory
        //   LDI → immediate value
        //   MOV → value from Rs register
        std::array<bool, 8> write_data = alu.result;
        if (s.reg_src_mem) write_data = mem_data;
        else if (s.reg_src_imm) write_data = ir.imm8();
        else if (s.is_mov) write_data = reg_file.rs_out;

        // Write result to destination register (Rd)
        if (s.reg_write) {
            reg_file.write(false, ir.rd(), true, write_data);
            reg_file.write(true, ir.rd(), true, write_data);
        }

        // Update zero/carry flags from ALU output
        if (s.flags_write) {
            flags.update(false, true, alu.carry, alu.zero);
            flags.update(true, true, alu.carry, alu.zero);
        }

        // Jump: override PC with immediate address
        if (s.pc_jump) {
            pc.clock(false, true, ir.imm8());
            pc.clock(true, true, ir.imm8());
        }

        if (s.halt) {
            halted = true;
        }
    }
};
