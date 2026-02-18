#pragma once
#include "latch.h"

// D-type edge-triggered flip-flop.
//
// Unlike a latch (which is transparent while enable is high),
// this only captures the D input at the RISING EDGE of the clock —
// the exact moment CLK goes from 0 to 1.
//
// Internally it's built from two D latches in a master-slave config:
//   - Master latch: enabled when CLK is LOW (captures D)
//   - Slave latch:  enabled when CLK is HIGH (outputs master's value)
// The result: output only changes on the 0→1 clock transition.

class DFlipFlop {
public:
    bool q  = false;
    bool qn = true;

    void clock(bool clk, bool d) {
        // Master is transparent when clock is LOW
        master.update(gate::NOT(clk), d);

        // Slave is transparent when clock is HIGH
        // It takes the master's output as input
        slave.update(clk, master.q);

        q  = slave.q;
        qn = slave.qn;
    }

private:
    DLatch master;
    DLatch slave;
};
