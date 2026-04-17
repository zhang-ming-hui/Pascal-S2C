#pragma once

#include <string>

#include "lower/lower.h"

namespace pascal_s2c {

class CCodeGenerator {
public:
    std::string generate(const LoweredProgramView& program) const;
};

}  // namespace pascal_s2c
