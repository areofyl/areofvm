#pragma once
#include "../gates/gates.h"

// SR Latch — the simplest memory element.
// Built from two cross-coupled NOR gates (like in the book).
// Set makes Q=1, Reset makes Q=0. Both high is invalid.

class SRLatch {
public:
    bool q  = false;
    bool qn = true;  // Q-bar (complement of Q)

    void update(bool set, bool reset) {
        // Two cross-coupled NOR gates:
        //   Q  = NOR(R, Q̄)
        //   Q̄ = NOR(S, Q)
        // We iterate a few times to let it stabilize (simulates propagation)
        for (int i = 0; i < 3; i++) {
            q  = gate::NOR(reset, qn);
            qn = gate::NOR(set, q);
        }
    }
};

// D Latch — level-triggered. When enable is HIGH, output follows input.
// When enable goes LOW, output is latched (held).
// Built from an SR latch + gates that steer D into set/reset.

class DLatch {
public:
    bool q  = false;
    bool qn = true;

    void update(bool enable, bool d) {
        // D feeds into an SR latch like this:
        //   Set   = AND(enable, D)
        //   Reset = AND(enable, NOT(D))
        bool set   = gate::AND(enable, d);
        bool reset = gate::AND(enable, gate::NOT(d));

        sr.update(set, reset);
        q  = sr.q;
        qn = sr.qn;
    }

private:
    SRLatch sr;
};
