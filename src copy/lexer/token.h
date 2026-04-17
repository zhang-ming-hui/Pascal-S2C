#pragma once

#include <string>
#include <vector>

#include "common/location.h"

namespace pascal_s2c {

// 词法单元类别枚举。
// 使用位置：
// 1) Lexer 根据源码字符流生成对应 kind。
// 2) Parser 按 kind 进行语法分支匹配（expect/check/match）。
// 3) 报错与调试阶段通过 tokenKindName 输出可读名称。
// 存储信息：
// - 标识符/字面量
// - 关键字
// - 运算符与分隔符
// - 结束标记 EndOfFile
enum class TokenKind {
    Identifier,
    IntegerLiteral,
    RealLiteral,
    CharLiteral,
    StringLiteral,

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
    Break,
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

// 单个 token 结构。
// 使用位置：
// 1) Lexer 作为 tokenize 的产物元素。
// 2) Parser 消费 token 序列并构建 AST。
// 存储信息：
// - kind: token 类型
// - lexeme: 原词素文本（标识符会标准化为小写）
// - location: 起始源码位置（行/列）
struct Token {
    TokenKind kind = TokenKind::EndOfFile;
    std::string lexeme;
    SourceLocation location;
};

// 词法分析阶段的标准输出类型：Token 序列。
// 示例：program main; -> [Program, Identifier("main"), Semicolon, EndOfFile]
using TokenList = std::vector<Token>;

// 将 TokenKind 转成可读字符串，主要用于日志、调试和错误输出。
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
    case TokenKind::StringLiteral:
        return "StringLiteral";
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
    case TokenKind::Break:
        return "Break";
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
