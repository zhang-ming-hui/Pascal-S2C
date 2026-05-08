#pragma once

#include <vector>

#include "ast/ast.h"
#include "common/error.h"
#include "lexer/token.h"

namespace pascal_s2c {

class Parser {
public:
    ProgramPtr parse(const TokenList& tokens, std::vector<CompilerError>* diagnostics = nullptr) const;
};

}  // namespace pascal_s2c
