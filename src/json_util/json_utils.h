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

// 字符串 → TokenKind（反序列化用）
inline TokenKind stringToTokenKind(const std::string& str) {
    // 与 tokenKindName 的映射完全对应
    if (str == "Identifier")       return TokenKind::Identifier;
    if (str == "IntegerLiteral")   return TokenKind::IntegerLiteral;
    if (str == "RealLiteral")      return TokenKind::RealLiteral;
    if (str == "CharLiteral")      return TokenKind::CharLiteral;
    if (str == "StringLiteral")    return TokenKind::StringLiteral;
    if (str == "Program")          return TokenKind::Program;
    if (str == "Const")            return TokenKind::Const;
    if (str == "Var")              return TokenKind::Var;
    if (str == "Type")             return TokenKind::Type;
    if (str == "Record")           return TokenKind::Record;
    if (str == "Array")            return TokenKind::Array;
    if (str == "Of")               return TokenKind::Of;
    if (str == "Begin")            return TokenKind::Begin;
    if (str == "End")              return TokenKind::End;
    if (str == "Function")         return TokenKind::Function;
    if (str == "Procedure")        return TokenKind::Procedure;
    if (str == "Integer")          return TokenKind::Integer;
    if (str == "Real")             return TokenKind::Real;
    if (str == "Boolean")          return TokenKind::Boolean;
    if (str == "Char")             return TokenKind::Char;
    if (str == "If")               return TokenKind::If;
    if (str == "Then")             return TokenKind::Then;
    if (str == "Else")             return TokenKind::Else;
    if (str == "While")            return TokenKind::While;
    if (str == "Do")               return TokenKind::Do;
    if (str == "For")              return TokenKind::For;
    if (str == "To")               return TokenKind::To;
    if (str == "Downto")           return TokenKind::Downto;
    if (str == "Case")             return TokenKind::Case;
    if (str == "Repeat")           return TokenKind::Repeat;
    if (str == "Until")            return TokenKind::Until;
    if (str == "Break")            return TokenKind::Break;
    if (str == "Read")             return TokenKind::Read;
    if (str == "Write")            return TokenKind::Write;
    if (str == "Div")              return TokenKind::Div;
    if (str == "Mod")              return TokenKind::Mod;
    if (str == "And")              return TokenKind::And;
    if (str == "Or")               return TokenKind::Or;
    if (str == "Not")              return TokenKind::Not;
    if (str == "True")             return TokenKind::True;
    if (str == "False")            return TokenKind::False;
    if (str == "Assign")           return TokenKind::Assign;
    if (str == "Equal")            return TokenKind::Equal;
    if (str == "NotEqual")         return TokenKind::NotEqual;
    if (str == "Less")             return TokenKind::Less;
    if (str == "LessEqual")        return TokenKind::LessEqual;
    if (str == "Greater")          return TokenKind::Greater;
    if (str == "GreaterEqual")     return TokenKind::GreaterEqual;
    if (str == "Plus")             return TokenKind::Plus;
    if (str == "Minus")            return TokenKind::Minus;
    if (str == "Star")             return TokenKind::Star;
    if (str == "Slash")            return TokenKind::Slash;
    if (str == "Comma")            return TokenKind::Comma;
    if (str == "Semicolon")        return TokenKind::Semicolon;
    if (str == "Colon")            return TokenKind::Colon;
    if (str == "Dot")              return TokenKind::Dot;
    if (str == "Range")            return TokenKind::Range;
    if (str == "LParen")           return TokenKind::LParen;
    if (str == "RParen")           return TokenKind::RParen;
    if (str == "LBracket")         return TokenKind::LBracket;
    if (str == "RBracket")         return TokenKind::RBracket;
    if (str == "EndOfFile")        return TokenKind::EndOfFile;
    return TokenKind::EndOfFile;  // 兜底
}

// SourceLocation → JSON（字段名对齐规范：col 而非 column）
inline nlohmann::json locationToJson(const SourceLocation& loc) {
    return {
        {"line", loc.line},
        {"col", loc.column}
    };
}

// JSON → SourceLocation
inline SourceLocation jsonToLocation(const nlohmann::json& j) {
    SourceLocation loc;
    loc.line = j.value("line", 1);
    loc.column = j.value("col", 1);
    return loc;
}

} // namespace json_utils
} // namespace pascal_s2c
