#pragma once

#include <string>

#include "ast/ast.h"

namespace pascal_s2c {

std::string emitParseJson(const ProgramNode& program);

}  // namespace pascal_s2c
