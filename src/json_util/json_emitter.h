#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "lexer/token.h"
#include "ast/ast.h"
#include "common/error.h"

namespace pascal_s2c {

// ========== 序列化接口（导出 JSON） ==========

// 词法阶段 JSON 对象
nlohmann::json emitLexJson(const TokenList& tokens);
// 词法阶段 JSON 字符串（规范接口）
std::string ir_token_to_json(const TokenList& tokens);

// 语法阶段 JSON 对象
nlohmann::json emitParseJson(const ProgramNode& program);
// 语法阶段 JSON 字符串（规范接口）
std::string ir_ast_to_json(const ProgramNode& program);

// 单条错误 JSON 对象
nlohmann::json emitErrorJson(const CompilerError& error);

// ========== 批量错误流序列化 ==========

// 错误流结构体（对齐组长规范 0.3.1）
struct ErrorItem {
    int line = 0;
    int col = 0;
    std::string err_type;
    std::string message;
};
using ErrorStream = std::vector<ErrorItem>;

// 批量错误流 → JSON 对象
nlohmann::json emitErrorStreamJson(const ErrorStream& errors);
// 批量错误流 → JSON 字符串（规范接口）
std::string ir_errors_to_json(const ErrorStream& errors);

// ========== 反序列化接口（导入 JSON） ==========

// JSON 字符串 → TokenList
TokenList ir_json_to_token(const std::string& json_str);
// JSON 字符串 → AST（基础骨架恢复）
std::unique_ptr<ProgramNode> ir_json_to_ast(const std::string& json_str);

} // namespace pascal_s2c
