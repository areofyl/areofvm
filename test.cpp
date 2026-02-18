#include "cpu/computer.h"
#include <iostream>
#include <cstdint>
#include <vector>
#include <string>

// Encode a 24-bit instruction into three bytes
// Layout: byte0=imm_lo, byte1=imm_hi, byte2=[opcode:4][rd:2][rs:2]
void emit(std::vector<uint8_t>& prog, uint8_t opcode, uint8_t rd, uint8_t rs, uint16_t imm) {
    uint8_t top = (opcode << 4) | ((rd & 3) << 2) | (rs & 3);
    prog.push_back(imm & 0xFF);
    prog.push_back((imm >> 8) & 0xFF);
    prog.push_back(top);
}

bool test_add() {
    std::vector<uint8_t> prog;
    emit(prog, 0x1, 0, 0, 3);   // addr 0: LDI R0, 3
    emit(prog, 0x1, 1, 0, 5);   // addr 3: LDI R1, 5
    emit(prog, 0x4, 0, 1, 0);   // addr 6: ADD R0, R1
    emit(prog, 0xF, 0, 0, 0);   // addr 9: HLT

    Computer c;
    c.load_program(prog.data(), prog.size());
    c.run();

    bool pass = c.get_cpu().get_reg(0) == 8;
    std::cout << "test_add:  R0=" << (int)c.get_cpu().get_reg(0) << " (expect 8) " << (pass ? "PASS" : "FAIL") << "\n";
    return pass;
}

bool test_sub() {
    std::vector<uint8_t> prog;
    emit(prog, 0x1, 0, 0, 20);  // addr 0
    emit(prog, 0x1, 1, 0, 7);   // addr 3
    emit(prog, 0x5, 0, 1, 0);   // addr 6
    emit(prog, 0xF, 0, 0, 0);   // addr 9

    Computer c;
    c.load_program(prog.data(), prog.size());
    c.run();

    bool pass = c.get_cpu().get_reg(0) == 13;
    std::cout << "test_sub:  R0=" << (int)c.get_cpu().get_reg(0) << " (expect 13) " << (pass ? "PASS" : "FAIL") << "\n";
    return pass;
}

bool test_ldi_and_mov() {
    std::vector<uint8_t> prog;
    emit(prog, 0x1, 2, 0, 42);  // addr 0
    emit(prog, 0x8, 3, 2, 0);   // addr 3
    emit(prog, 0xF, 0, 0, 0);   // addr 6

    Computer c;
    c.load_program(prog.data(), prog.size());
    c.run();

    bool pass = c.get_cpu().get_reg(3) == 42;
    std::cout << "test_mov:  R3=" << (int)c.get_cpu().get_reg(3) << " (expect 42) " << (pass ? "PASS" : "FAIL") << "\n";
    return pass;
}

bool test_jump() {
    // JMP should skip one instruction
    std::vector<uint8_t> prog;
    emit(prog, 0x1, 0, 0, 1);   // addr 0: LDI R0, 1
    emit(prog, 0xA, 0, 0, 9);   // addr 3: JMP 9 (skip addr 6)
    emit(prog, 0x1, 0, 0, 99);  // addr 6: LDI R0, 99 (skipped)
    emit(prog, 0xF, 0, 0, 0);   // addr 9: HLT

    Computer c;
    c.load_program(prog.data(), prog.size());
    c.run();

    bool pass = c.get_cpu().get_reg(0) == 1;
    std::cout << "test_jmp:  R0=" << (int)c.get_cpu().get_reg(0) << " (expect 1) " << (pass ? "PASS" : "FAIL") << "\n";
    return pass;
}

bool test_conditional_jump() {
    std::vector<uint8_t> prog;
    emit(prog, 0x1, 0, 0, 5);   // addr 0: LDI R0, 5
    emit(prog, 0x1, 1, 0, 5);   // addr 3: LDI R1, 5
    emit(prog, 0x9, 0, 1, 0);   // addr 6: CMP R0, R1
    emit(prog, 0xB, 0, 0, 15);  // addr 9: JZ 15
    emit(prog, 0x1, 2, 0, 99);  // addr 12: LDI R2, 99 (skipped)
    emit(prog, 0x1, 2, 0, 1);   // addr 15: LDI R2, 1
    emit(prog, 0xF, 0, 0, 0);   // addr 18: HLT

    Computer c;
    c.load_program(prog.data(), prog.size());
    c.run();

    bool pass = c.get_cpu().get_reg(2) == 1;
    std::cout << "test_jz:   R2=" << (int)c.get_cpu().get_reg(2) << " (expect 1) " << (pass ? "PASS" : "FAIL") << "\n";
    return pass;
}

bool test_memory() {
    // LD/ST now use imm16 as address
    std::vector<uint8_t> prog;
    emit(prog, 0x1, 0, 0, 77);      // addr 0: LDI R0, 77
    emit(prog, 0x3, 0, 0, 0x1000);  // addr 3: ST R0, [0x1000]
    emit(prog, 0x1, 0, 0, 0);       // addr 6: LDI R0, 0 (clobber)
    emit(prog, 0x2, 1, 0, 0x1000);  // addr 9: LD R1, [0x1000]
    emit(prog, 0xF, 0, 0, 0);       // addr 12: HLT

    Computer c;
    c.load_program(prog.data(), prog.size());
    c.run();

    bool pass = c.get_cpu().get_reg(1) == 77;
    std::cout << "test_mem:  R1=" << (int)c.get_cpu().get_reg(1) << " (expect 77) " << (pass ? "PASS" : "FAIL") << "\n";
    return pass;
}

bool test_loop() {
    std::vector<uint8_t> prog;
    emit(prog, 0x1, 0, 0, 0);   // addr 0: LDI R0, 0
    emit(prog, 0x1, 1, 0, 5);   // addr 3: LDI R1, 5
    emit(prog, 0x1, 2, 0, 1);   // addr 6: LDI R2, 1
    emit(prog, 0x4, 0, 2, 0);   // addr 9: ADD R0, R2
    emit(prog, 0x9, 0, 1, 0);   // addr 12: CMP R0, R1
    emit(prog, 0xC, 0, 0, 9);   // addr 15: JNZ 9
    emit(prog, 0xF, 0, 0, 0);   // addr 18: HLT

    Computer c;
    c.load_program(prog.data(), prog.size());
    c.run();

    bool pass = c.get_cpu().get_reg(0) == 5;
    std::cout << "test_loop: R0=" << (int)c.get_cpu().get_reg(0) << " (expect 5) " << (pass ? "PASS" : "FAIL") << "\n";
    return pass;
}

bool test_push_pop() {
    std::vector<uint8_t> prog;
    emit(prog, 0x1, 0, 0, 42);  // LDI R0, 42
    emit(prog, 0x0, 0, 1, 0);   // PUSH R0
    emit(prog, 0x1, 0, 0, 0);   // LDI R0, 0
    emit(prog, 0x0, 1, 2, 0);   // POP R1
    emit(prog, 0xF, 0, 0, 0);   // HLT

    Computer c;
    c.load_program(prog.data(), prog.size());
    c.run();

    bool pass = c.get_cpu().get_reg(1) == 42;
    std::cout << "test_push: R1=" << (int)c.get_cpu().get_reg(1) << " (expect 42) " << (pass ? "PASS" : "FAIL") << "\n";
    return pass;
}

bool test_call_ret() {
    std::vector<uint8_t> prog;
    emit(prog, 0x1, 0, 0, 10);  // addr 0: LDI R0, 10
    emit(prog, 0xE, 0, 0, 9);   // addr 3: CALL 9
    emit(prog, 0xF, 0, 0, 0);   // addr 6: HLT (return lands here)
    emit(prog, 0xD, 0, 0, 10);  // addr 9: ADDI R0, 10
    emit(prog, 0x0, 0, 3, 0);   // addr 12: RET

    Computer c;
    c.load_program(prog.data(), prog.size());
    c.run();

    bool pass = c.get_cpu().get_reg(0) == 20;
    std::cout << "test_call: R0=" << (int)c.get_cpu().get_reg(0) << " (expect 20) " << (pass ? "PASS" : "FAIL") << "\n";
    return pass;
}

bool test_16bit_address() {
    // Test that we can jump to and execute code beyond 256 bytes
    // Put HLT at address 0x200 (512), jump to it
    std::vector<uint8_t> prog;
    emit(prog, 0x1, 0, 0, 42);     // addr 0: LDI R0, 42
    emit(prog, 0xA, 0, 0, 0x200);  // addr 3: JMP 0x200

    Computer c;
    c.load_program(prog.data(), prog.size());
    // Place a HLT at 0x200
    uint8_t hlt_instr[3];
    hlt_instr[0] = 0x00;  // imm lo
    hlt_instr[1] = 0x00;  // imm hi
    hlt_instr[2] = 0xF0;  // opcode=0xF, rd=0, rs=0
    c.load_program(hlt_instr, 3, 0x200);
    c.run();

    bool pass = c.get_cpu().get_reg(0) == 42 && c.get_cpu().is_halted();
    std::cout << "test_16b:  R0=" << (int)c.get_cpu().get_reg(0) << " halted=" << c.get_cpu().is_halted() << " (expect 42, halted) " << (pass ? "PASS" : "FAIL") << "\n";
    return pass;
}

bool test_software_interrupt() {
    // Set up IVT entry 2 (software interrupt) pointing to handler at 0x100
    // Handler adds 100 to R0, then RTI
    // Main program: LDI R0, 5 → STI → SWI 2 → HLT
    // After SWI, R0 should be 105

    Computer c;

    // Write IVT entry 2 at 0xEFF4 (IVT_BASE + 2*2)
    // Handler address = 0x0100, little-endian
    c.get_bus().write_byte(0xEFF4, 0x00);  // lo byte of 0x0100
    c.get_bus().write_byte(0xEFF5, 0x01);  // hi byte of 0x0100

    // Main program at 0x0000
    std::vector<uint8_t> prog;
    emit(prog, 0x1, 0, 0, 5);   // addr 0: LDI R0, 5
    emit(prog, 0x0, 2, 0, 0);   // addr 3: STI (enable interrupts)
    emit(prog, 0x0, 1, 3, 2);   // addr 6: SWI 2 (software interrupt 2)
    emit(prog, 0xF, 0, 0, 0);   // addr 9: HLT
    c.load_program(prog.data(), prog.size());

    // Handler at 0x0100: ADDI R0, 100 then RTI
    std::vector<uint8_t> handler;
    emit(handler, 0xD, 0, 0, 100);  // addr 0x100: ADDI R0, 100
    emit(handler, 0x0, 3, 0, 0);    // addr 0x103: RTI
    c.load_program(handler.data(), handler.size(), 0x0100);

    c.run();

    bool pass = c.get_cpu().get_reg(0) == 105;
    std::cout << "test_swi:  R0=" << (int)c.get_cpu().get_reg(0) << " (expect 105) " << (pass ? "PASS" : "FAIL") << "\n";
    return pass;
}

bool test_hardware_interrupt() {
    // Timer fires after a few cycles, handler sets R1=99
    Computer c;

    // IVT entry 1 (timer) at 0xEFF2 → handler at 0x0100
    c.get_bus().write_byte(0xEFF2, 0x00);
    c.get_bus().write_byte(0xEFF3, 0x01);

    // Main program: enable interrupts, loop until R1 != 0
    std::vector<uint8_t> prog;
    emit(prog, 0x0, 2, 0, 0);   // addr 0: STI
    emit(prog, 0x1, 0, 0, 42);  // addr 3: LDI R0, 42
    emit(prog, 0xF, 0, 0, 0);   // addr 6: HLT
    c.load_program(prog.data(), prog.size());

    // Handler: LDI R1, 99, RTI
    std::vector<uint8_t> handler;
    emit(handler, 0x1, 1, 0, 99);
    emit(handler, 0x0, 3, 0, 0);   // RTI
    c.load_program(handler.data(), handler.size(), 0x0100);

    // Step once to execute STI
    c.step();
    // Raise timer interrupt
    c.get_cpu().raise_interrupt(1);
    // Run the rest
    c.run();

    bool pass = c.get_cpu().get_reg(0) == 42 && c.get_cpu().get_reg(1) == 99;
    std::cout << "test_hwi:  R0=" << (int)c.get_cpu().get_reg(0) << " R1=" << (int)c.get_cpu().get_reg(1)
              << " (expect 42, 99) " << (pass ? "PASS" : "FAIL") << "\n";
    return pass;
}

bool test_timer_device() {
    // Program enables interrupts, arms timer via I/O, then loops.
    // Timer fires after 5 ticks, handler sets R1=77 and halts.
    Computer c;

    // IVT entry 1 (timer) → handler at 0x0100
    c.get_bus().write_byte(0xEFF2, 0x00);
    c.get_bus().write_byte(0xEFF3, 0x01);

    std::vector<uint8_t> prog;
    emit(prog, 0x0, 2, 0, 0);          // addr 0: STI
    emit(prog, 0x1, 0, 0, 5);          // addr 3: LDI R0, 5
    emit(prog, 0x3, 0, 0, 0xF000);     // addr 6: ST R0, [0xF000] (timer reload)
    emit(prog, 0x1, 0, 0, 2);          // addr 9: LDI R0, 2 (enable bit)
    emit(prog, 0x3, 0, 0, 0xF001);     // addr 12: ST R0, [0xF001] (timer ctrl)
    emit(prog, 0xA, 0, 0, 15);         // addr 15: JMP 15 (spin)
    c.load_program(prog.data(), prog.size());

    // Handler: LDI R1, 77 then HLT
    std::vector<uint8_t> handler;
    emit(handler, 0x1, 1, 0, 77);
    emit(handler, 0xF, 0, 0, 0);
    c.load_program(handler.data(), handler.size(), 0x0100);

    c.run(100);

    bool pass = c.get_cpu().get_reg(1) == 77;
    std::cout << "test_tmr:  R1=" << (int)c.get_cpu().get_reg(1) << " (expect 77) " << (pass ? "PASS" : "FAIL") << "\n";
    return pass;
}

bool test_uart() {
    // CPU writes 'H' and 'i' to UART TX (0xF002), then reads a char
    // from RX that the host pushed. Verifies both directions work.
    Computer c;

    // IVT entry 2 (UART interrupt) → handler at 0x0100
    c.get_bus().write_byte(0xEFF4, 0x00);
    c.get_bus().write_byte(0xEFF5, 0x01);

    // Main program: write 'H' and 'i' to UART data register
    std::vector<uint8_t> prog;
    emit(prog, 0x1, 0, 0, 'H');         // LDI R0, 'H'
    emit(prog, 0x3, 0, 0, 0xF002);      // ST R0, [0xF002] (UART data)
    emit(prog, 0x1, 0, 0, 'i');         // LDI R0, 'i'
    emit(prog, 0x3, 0, 0, 0xF002);      // ST R0, [0xF002]
    // Now read a char from RX (we'll push one from host side first)
    emit(prog, 0x2, 1, 0, 0xF002);      // LD R1, [0xF002] (UART data = RX)
    emit(prog, 0xF, 0, 0, 0);           // HLT
    c.load_program(prog.data(), prog.size());

    // Push a character into RX before running
    c.get_uart().send_char('Z');

    c.run();

    // Check TX output
    bool tx_ok = false;
    if (c.get_uart().has_output()) {
        uint8_t ch1 = c.get_uart().recv_char();
        uint8_t ch2 = c.get_uart().recv_char();
        tx_ok = (ch1 == 'H' && ch2 == 'i');
    }
    // Check RX read
    bool rx_ok = c.get_cpu().get_reg(1) == 'Z';
    bool pass = tx_ok && rx_ok;
    std::cout << "test_uart: TX=" << (tx_ok ? "Hi" : "??") << " RX=R1=" << (int)c.get_cpu().get_reg(1)
              << " (expect Hi, 90) " << (pass ? "PASS" : "FAIL") << "\n";
    return pass;
}

bool test_jc_jnc() {
    // Test JC (jump if carry) and JNC (jump if no carry).
    // CMP/SUB sets carry when A >= B (no borrow).
    //
    // Test: R0=10, R1=5. CMP R0, R1 → carry set (10 >= 5).
    //   JC should jump. JNC should not.
    // Then: R0=3, R1=8. CMP R0, R1 → carry clear (3 < 8).
    //   JNC should jump. JC should not.

    Computer c;
    std::vector<uint8_t> prog;

    // Part 1: 10 >= 5 → carry set
    emit(prog, 0x1, 0, 0, 10);     // LDI R0, 10
    emit(prog, 0x1, 1, 0, 5);      // LDI R1, 5
    emit(prog, 0x9, 0, 1, 0);      // CMP R0, R1  → carry=1 (10 >= 5)
    emit(prog, 0x0, 2, 3, 15);     // JC 15  (Rd=2, Rs=3 → JC)
    emit(prog, 0x1, 2, 0, 99);     // LDI R2, 99 (should be skipped)
    // addr 15:
    emit(prog, 0x1, 2, 0, 1);      // LDI R2, 1 (JC landed here)

    // Part 2: 3 < 8 → carry clear
    emit(prog, 0x1, 0, 0, 3);      // addr 18: LDI R0, 3
    emit(prog, 0x1, 1, 0, 8);      // addr 21: LDI R1, 8
    emit(prog, 0x9, 0, 1, 0);      // addr 24: CMP R0, R1  → carry=0 (3 < 8)
    emit(prog, 0x0, 3, 3, 33);     // addr 27: JNC 33 (Rd=3, Rs=3 → JNC)
    emit(prog, 0x1, 3, 0, 99);     // addr 30: LDI R3, 99 (should be skipped)
    // addr 33:
    emit(prog, 0x1, 3, 0, 2);      // LDI R3, 2 (JNC landed here)
    emit(prog, 0xF, 0, 0, 0);      // HLT

    c.load_program(prog.data(), prog.size());
    c.run();

    bool pass = c.get_cpu().get_reg(2) == 1 && c.get_cpu().get_reg(3) == 2;
    std::cout << "test_jc:   R2=" << (int)c.get_cpu().get_reg(2)
              << " R3=" << (int)c.get_cpu().get_reg(3)
              << " (expect 1, 2) " << (pass ? "PASS" : "FAIL") << "\n";
    return pass;
}

int main() {
    std::cout << "=== seedisa CPU tests ===\n\n";

    int passed = 0, total = 0;
    auto run = [&](bool (*fn)()) { total++; if (fn()) passed++; };

    run(test_add);
    run(test_sub);
    run(test_ldi_and_mov);
    run(test_jump);
    run(test_conditional_jump);
    run(test_memory);
    run(test_loop);
    run(test_push_pop);
    run(test_call_ret);
    run(test_16bit_address);
    run(test_software_interrupt);
    run(test_hardware_interrupt);
    run(test_timer_device);
    run(test_uart);
    run(test_jc_jnc);

    std::cout << "\n" << passed << "/" << total << " tests passed\n";
    return (passed == total) ? 0 : 1;
}
