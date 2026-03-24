#pragma once

#include "ast/ast.h"
#include "lexer/token.h"

namespace pascal_s2c {

// 语法分析器接口。
// 阶段职责：消费 TokenList 并构建完整 Program AST。
// 阶段输入：Lexer 产生的 token 序列。
// 阶段输出：ProgramPtr（AST 根节点所有权）。
class Parser {
public:
    // 执行语法分析。
    // 成功：返回 ProgramNode 根节点。
    // 失败：抛出带源码位置的 CompilerError。
    ProgramPtr parse(const TokenList& tokens) const;
};

}  // namespace pascal_s2c
