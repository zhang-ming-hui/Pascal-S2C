#pragma once

#include "ast/ast.h"
#include "lexer/token.h"

namespace pascal_s2c {

class Parser {
public:
    ProgramPtr parse(const TokenList& tokens) const;
};

}  // namespace pascal_s2c
