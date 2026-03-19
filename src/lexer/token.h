#pragma once

#include <string>
#include <vector>

#include "common/location.h"

namespace pascal_s2c {

enum class TokenKind {
    Identifier,
    IntegerLiteral,
    RealLiteral,
    CharLiteral,

    Program,
    Const,
    Var,
    Type,
    Record,
    Array,
    Of,
    Begin,
    End,
    Function,
    Procedure,
    Integer,
    Real,
    Boolean,
    Char,
    If,
    Then,
    Else,
    While,
    Do,
    For,
    To,
    Downto,
    Case,
    Repeat,
    Until,
    Read,
    Write,
    Div,
    Mod,
    And,
    Or,
    Not,
    True,
    False,

    Assign,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Plus,
    Minus,
    Star,
    Slash,
    Comma,
    Semicolon,
    Colon,
    Dot,
    Range,
    LParen,
    RParen,
    LBracket,
    RBracket,

    EndOfFile
};

struct Token {
    TokenKind kind = TokenKind::EndOfFile;
    std::string lexeme;
    SourceLocation location;
};

using TokenList = std::vector<Token>;

inline const char* tokenKindName(TokenKind kind) {
    switch (kind) {
    case TokenKind::Identifier:
        return "Identifier";
    case TokenKind::IntegerLiteral:
        return "IntegerLiteral";
    case TokenKind::RealLiteral:
        return "RealLiteral";
    case TokenKind::CharLiteral:
        return "CharLiteral";
    case TokenKind::Program:
        return "Program";
    case TokenKind::Const:
        return "Const";
    case TokenKind::Var:
        return "Var";
    case TokenKind::Type:
        return "Type";
    case TokenKind::Record:
        return "Record";
    case TokenKind::Array:
        return "Array";
    case TokenKind::Of:
        return "Of";
    case TokenKind::Begin:
        return "Begin";
    case TokenKind::End:
        return "End";
    case TokenKind::Function:
        return "Function";
    case TokenKind::Procedure:
        return "Procedure";
    case TokenKind::Integer:
        return "Integer";
    case TokenKind::Real:
        return "Real";
    case TokenKind::Boolean:
        return "Boolean";
    case TokenKind::Char:
        return "Char";
    case TokenKind::If:
        return "If";
    case TokenKind::Then:
        return "Then";
    case TokenKind::Else:
        return "Else";
    case TokenKind::While:
        return "While";
    case TokenKind::Do:
        return "Do";
    case TokenKind::For:
        return "For";
    case TokenKind::To:
        return "To";
    case TokenKind::Downto:
        return "Downto";
    case TokenKind::Case:
        return "Case";
    case TokenKind::Repeat:
        return "Repeat";
    case TokenKind::Until:
        return "Until";
    case TokenKind::Read:
        return "Read";
    case TokenKind::Write:
        return "Write";
    case TokenKind::Div:
        return "Div";
    case TokenKind::Mod:
        return "Mod";
    case TokenKind::And:
        return "And";
    case TokenKind::Or:
        return "Or";
    case TokenKind::Not:
        return "Not";
    case TokenKind::True:
        return "True";
    case TokenKind::False:
        return "False";
    case TokenKind::Assign:
        return "Assign";
    case TokenKind::Equal:
        return "Equal";
    case TokenKind::NotEqual:
        return "NotEqual";
    case TokenKind::Less:
        return "Less";
    case TokenKind::LessEqual:
        return "LessEqual";
    case TokenKind::Greater:
        return "Greater";
    case TokenKind::GreaterEqual:
        return "GreaterEqual";
    case TokenKind::Plus:
        return "Plus";
    case TokenKind::Minus:
        return "Minus";
    case TokenKind::Star:
        return "Star";
    case TokenKind::Slash:
        return "Slash";
    case TokenKind::Comma:
        return "Comma";
    case TokenKind::Semicolon:
        return "Semicolon";
    case TokenKind::Colon:
        return "Colon";
    case TokenKind::Dot:
        return "Dot";
    case TokenKind::Range:
        return "Range";
    case TokenKind::LParen:
        return "LParen";
    case TokenKind::RParen:
        return "RParen";
    case TokenKind::LBracket:
        return "LBracket";
    case TokenKind::RBracket:
        return "RBracket";
    case TokenKind::EndOfFile:
        return "EndOfFile";
    default:
        return "Unknown";
    }
}

}  // namespace pascal_s2c
