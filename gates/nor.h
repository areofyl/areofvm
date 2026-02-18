#pragma once

#include "not.h"
#include "or.h"

namespace gate {

inline bool NOR(bool a, bool b) { return NOT(OR(a, b)); }

} // namespace gate
