#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "ast/ast.h"
#include "semantic/scope.h"
#include "semantic/symbol.h"
#include "semantic/type.h"

namespace pascal_s2c {

// 语义分析阶段的核心产物。
// 阶段输入：Parser 产生的 Program AST。
// 阶段输出：SemanticContext。
// 该结构被 Lower/Codegen 直接消费，用于类型查询、符号绑定和调用签名检查。
struct SemanticContext {
    // 入口程序名（来自 ProgramNode.name）。
    std::string entryProgramName;

    // 所有作用域对象的所有权集合。
    // 设计目的：确保 globalScope 与其子作用域指针在编译流程中始终有效。
    std::vector<std::unique_ptr<Scope>> ownedScopes;

    // 全局作用域指针。
    // 代码生成阶段会用它做名称查询（例如循环变量/函数名解析）。
    const Scope* globalScope = nullptr;

    // 表达式类型表：Expr* -> TypeInfo。
    // 用途：赋值兼容性、运算合法性、printf/scanf 格式推导。
    std::unordered_map<const Expr*, TypeInfo> expressionTypes;

    // 变量表达式绑定：VarExprNode* -> Symbol*。
    // 用途：在生成代码时确定变量属性（是否 var 参数、是否函数名等）。
    std::unordered_map<const VarExprNode*, const Symbol*> variableBindings;

    // 数组索引表达式绑定：IndexExprNode* -> Symbol*（数组符号）。
    // 用途：校验维度后在代码生成时做下界修正。
    std::unordered_map<const IndexExprNode*, const Symbol*> indexBindings;

    // 调用表达式绑定：CallExprNode* -> Symbol*。
    // 用途：表达式语境下函数调用的签名检查与返回类型获取。
    std::unordered_map<const CallExprNode*, const Symbol*> callExprBindings;

    // 调用语句绑定：CallStmtNode* -> Symbol*。
    // 用途：语句语境下过程/函数调用的参数检查与 var 传参判定。
    std::unordered_map<const CallStmtNode*, const Symbol*> callStmtBindings;
};

// 语义分析器接口。
// 阶段职责：
// 1) 建立作用域与符号表。
// 2) 绑定名称引用。
// 3) 推断并检查表达式/语句类型。
// 4) 产出供后续阶段复用的 SemanticContext。
class SemanticAnalyzer {
public:
    // 对完整 Program AST 做语义分析。
    // 示例：analyze(*program) -> SemanticContext
    SemanticContext analyze(const ProgramNode& program) const;
};

}  // namespace pascal_s2c
