#pragma once

#include <string>
#include "lexer/token.h"
#include "common/location.h"

namespace pascal_s2c {
namespace json_utils {

// TokenKind → 字符串（复用 lexer 已有实现）
inline std::string tokenKindToString(TokenKind kind) {
    return tokenKindName(kind);
}

// SourceLocation → JSON
inline nlohmann::json locationToJson(const SourceLocation& loc) {
    return {
        {"line", loc.line},
        {"column", loc.column}
    };
}

} // namespace json_utils
} // namespace pascal_s2c
