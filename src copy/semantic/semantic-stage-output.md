# Semantic 阶段产物说明

本文说明语义分析（SemanticAnalyzer）运行结束后，程序在编译流程中的数据形态。

## 1. 阶段接口

- 输入类型：`const ProgramNode&`
- 输出类型：`SemanticContext`
- 接口：`SemanticContext SemanticAnalyzer::analyze(const ProgramNode& program) const`

`SemanticContext` 关键字段：

```cpp
struct SemanticContext {
    std::string entryProgramName;
    std::vector<std::unique_ptr<Scope>> ownedScopes;
    const Scope* globalScope;
    std::unordered_map<const Expr*, TypeInfo> expressionTypes;
    std::unordered_map<const VarExprNode*, const Symbol*> variableBindings;
    std::unordered_map<const IndexExprNode*, const Symbol*> indexBindings;
    std::unordered_map<const CallExprNode*, const Symbol*> callExprBindings;
    std::unordered_map<const CallStmtNode*, const Symbol*> callStmtBindings;
};
```

## 2. 示例输入

沿用 parser 文档中的 `demo` 程序。

## 3. Semantic 结束后的结果形态（简化）

1. 入口与作用域
- `entryProgramName = "demo"`
- `ownedScopes` 包含全局作用域 + 子程序作用域
- `globalScope` 指向全局作用域

2. 全局符号（globalScope）示例
- `ci`: `{kind=Constant, type=integer, isGlobal=true}`
- `a`: `{kind=Variable, type=integer, isGlobal=true}`
- `inc1`: `{kind=Function, type=integer, parameters=[{type=integer,isVar=false}], isGlobal=true}`

3. 表达式类型表 `expressionTypes` 示例
- `LiteralExpr("10") -> integer`
- `VarExpr("ci") -> integer`
- `CallExpr("inc1", [a]) -> integer`
- `BinaryExpr(x + 1) -> integer`

4. 绑定表示例
- `variableBindings[VarExpr("a")] -> Symbol("a")`
- `callExprBindings[CallExpr("inc1", ...)] -> Symbol("inc1")`
- `callStmtBindings` 在存在过程调用语句时会填充对应符号

## 4. 阶段产物要点

1. SemanticContext 是后续代码生成的重要输入，不再只靠 AST 结构。
2. `TypeInfo` 提供统一类型表示（标量/数组 + 维度边界）。
3. 绑定表将 AST 节点与符号表条目建立直接关系，减少代码生成时二次查找。
4. `var` 参数信息通过 `SymbolParameter.isVar` 与 `Symbol.isVarParameter` 传递到代码生成阶段。
