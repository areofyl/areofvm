#pragma once

#include "not.h"
#include "and.h"

namespace gate {

inline bool NAND(bool a, bool b) { return NOT(AND(a, b)); }

} // namespace gate
