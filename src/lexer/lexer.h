#pragma once

#include <string>

#include "lexer/token.h"

namespace pascal_s2c {

class Lexer {
public:
    TokenList tokenize(const std::string& source) const;
};

}  // namespace pascal_s2c
