#pragma once

#include "and.h"
#include "or.h"
#include "nand.h"

namespace gate {

inline bool XOR(bool a, bool b) { return AND(OR(a, b), NAND(a, b)); }

} // namespace gate
