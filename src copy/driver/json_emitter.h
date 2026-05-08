#pragma once

#include <nlohmann/json.hpp>
#include "lexer/token.h"
#include "ast/ast.h"
#include "common/error.h"

namespace pascal_s2c {

// 词法阶段 JSON 输出
nlohmann::json emitLexJson(const TokenList& tokens);

// 语法阶段 JSON 输出
nlohmann::json emitParseJson(const ProgramNode& program);

// 错误 JSON 输出（只识别，不定义类型）
nlohmann::json emitErrorJson(const CompilerError& error);

} // namespace pascal_s2c
