#pragma once

#include <string>
#include <vector>

#include "semantic/type.h"

namespace pascal_s2c {

// 符号种类。
// 使用位置：
// 1) SemanticAnalyzer 声明阶段区分常量/变量/参数/子程序。
// 2) 语义检查与代码生成按 kind 执行不同逻辑。
enum class SymbolKind {
    Constant,
    Variable,
    Parameter,
    Function,
    Procedure
};

// 调用签名中的单个参数信息。
// 使用位置：函数/过程符号的 parameters 列表。
// 存储信息：参数类型与是否 var 引用参数。
struct SymbolParameter {
    TypeInfo type;
    bool isVar = false;
};

// 符号表条目。
// 使用位置：Scope::symbols_ 的值类型。
// 存储信息：
// - name/kind/type: 基本语义属性
// - isGlobal: 是否在全局作用域声明
// - isVarParameter: 参数是否按引用传递（用于 * / & 代码生成）
// - parameters: 子程序的形参签名
struct Symbol {
    std::string name;
    SymbolKind kind = SymbolKind::Variable;
    TypeInfo type;
    bool isGlobal = false;
    bool isVarParameter = false;
    std::vector<SymbolParameter> parameters;
};

// 判断符号是否可调用（function/procedure）。
inline bool isCallableSymbol(const Symbol& symbol) {
    return symbol.kind == SymbolKind::Function || symbol.kind == SymbolKind::Procedure;
}

// 判断符号是否可作为赋值左值。
// 说明：函数名在函数体内可接收返回值赋值，因此也视为可赋值。
inline bool isAssignableSymbol(const Symbol& symbol) {
    return symbol.kind == SymbolKind::Variable || symbol.kind == SymbolKind::Parameter || symbol.kind == SymbolKind::Function;
}

}  // namespace pascal_s2c
