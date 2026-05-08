#pragma once

#include <string>

#include "lexer/token.h"

namespace pascal_s2c {

// 词法分析器接口。
// 阶段职责：将 Pascal 源码字符串切分为 TokenList，供 Parser 使用。
// 阶段输入：源码文本 source。
// 阶段输出：TokenList（最后一个 token 必为 EndOfFile）。
class Lexer {
public:
    // 执行词法分析。
    // 示例输入："program main; begin end."
    // 示例输出（kind 序列）：Program, Identifier, Semicolon, Begin, End, Dot, EndOfFile
    LexerResult tokenize(const std::string& source) const;
};

}  // namespace pascal_s2c
