# 00_main 编译全过程说明

本文针对示例输入 `00_main.pas`，按项目当前实现说明从命令行入口到 C 代码输出的完整调用链、接口签名和关键中间参数。

## 1. 输入样例

`00_main.pas` 内容：

```pascal
program main;
var
  a: integer;
begin
  a := 3;
  write(a);
end.
```

## 2. 入口与参数解析

### 2.1 入口函数

入口在 `src/main.cpp`:

- `int main(int argc, char** argv)`

典型调用：

```text
pascal_s2c.exe 00_main.pas
```

此时：

- `argc = 2`
- `argv[0] = "pascal_s2c.exe"`
- `argv[1] = "00_main.pas"`

### 2.2 命令行解析

调用：

- `parseArgs(int argc, char** argv) -> std::optional<CliOptions>`

输出结构体 `CliOptions`：

- `inputPath = "00_main.pas"`
- `outputPath = std::nullopt`

随后调用：

- `inferOutputPath("00_main.pas") -> "00_main.c"`

## 3. 编译总调度

在 `src/main.cpp` 中创建编译器并调用：

- `pascal_s2c::Compiler compiler;`
- `compiler.compileFile(options->inputPath)`

接口定义在 `src/driver/compiler.h`:

- `std::string compileFile(const std::string& path) const;`
- `std::string compileSource(const std::string& source) const;`

调用关系：

1. `compileFile("00_main.pas")`
2. `readTextFile("00_main.pas")` 得到源码字符串
3. `compileSource(source)` 执行完整流水线

## 4. 流水线分阶段细节

`compileSource` 位于 `src/driver/compiler.cpp`，按固定顺序执行：

1. `Lexer lexer;`
2. `Parser parser;`
3. `SemanticAnalyzer analyzer;`
4. `LoweringPass lowering;`
5. `CCodeGenerator codegen;`
6. `TokenList tokens = lexer.tokenize(source);`
7. `ProgramPtr program = parser.parse(tokens);`
8. `SemanticContext semantic = analyzer.analyze(*program);`
9. `LoweredProgramView lowered = lowering.lower(*program, semantic);`
10. `return codegen.generate(lowered);`

下面分别展开每一步的输入、输出和关键参数。

---

## 5. 词法分析 Lexer

接口：

- `TokenList Lexer::tokenize(const std::string& source) const`

输入参数：

- `source`：整个 `00_main.pas` 文件文本

输出参数：

- `TokenList`（`std::vector<Token>`）

### 5.1 关键 token（按顺序）

对 `00_main.pas`，核心 token 序列是：

1. `Program("program")`
2. `Identifier("main")`
3. `Semicolon(";")`
4. `Var("var")`
5. `Identifier("a")`
6. `Colon(":")`
7. `Integer("integer")`
8. `Semicolon(";")`
9. `Begin("begin")`
10. `Identifier("a")`
11. `Assign(":=")`
12. `IntegerLiteral("3")`
13. `Semicolon(";")`
14. `Write("write")`
15. `LParen("(")`
16. `Identifier("a")`
17. `RParen(")")`
18. `Semicolon(";")`
19. `End("end")`
20. `Dot(".")`
21. `EndOfFile("")`

说明：

- 关键字识别来自 `kKeywords` 映射。
- 标识符会统一转为小写后存入 `lexeme`。

---

## 6. 语法分析 Parser

接口：

- `ProgramPtr Parser::parse(const TokenList& tokens) const`

输入参数：

- `tokens`：Lexer 输出的 token 列表

输出参数：

- `ProgramPtr`（`std::unique_ptr<ProgramNode>`）

### 6.1 调用链（00_main 对应）

1. `parseProgram()`
2. `parseBlock()`
3. `parseConstDeclarations()`（本例无 const，直接返回）
4. `parseVarDeclarations()`
5. `parseVarDeclaration()` 解析 `a: integer`
6. `parseCompoundStatement()` 解析 `begin ... end`
7. `parseStatement()` 解析赋值 `a := 3`
8. `parseIdentifierLedStatement()`
9. `parseExpression()` 解析右值 `3`
10. `parseStatement()` 解析 `write(a)`
11. `parseWriteStatement()`
12. `parseExpressionList()` -> `parseExpression()` -> `parseIdentifierExpression()` 解析参数 `a`

### 6.2 产出的关键 AST 结构

- `ProgramNode`
  - `name = "main"`
  - `block = BlockNode`
    - `constDecls = []`
    - `varDecls = [ VarDeclNode(names=["a"], type=ScalarType(Integer)) ]`
    - `subprograms = []`
    - `body = CompoundStmtNode`
      - `statements[0] = AssignStmtNode(target=VarExpr("a"), value=LiteralExpr("3"))`
      - `statements[1] = WriteStmtNode(values=[VarExpr("a")])`

---

## 7. 语义分析 SemanticAnalyzer

接口：

- `SemanticContext SemanticAnalyzer::analyze(const ProgramNode& program) const`

输入参数：

- `program`：Parser 生成的 AST 根节点

输出参数：

- `SemanticContext`

### 7.1 本例关键语义结果

- `entryProgramName = "main"`
- 全局符号表包含：
  - 符号 `a`
  - `kind = Variable`
  - `type = integer`
  - `isGlobal = true`
- `expressionTypes` 中至少包含：
  - 字面量 `3` 的类型是 `integer`
  - 表达式 `a` 的类型是 `integer`
- `variableBindings` 将 AST 中的 `VarExpr("a")` 绑定到符号表中的变量 `a`

说明：

- `write(a)` 的格式串类型依据来自表达式类型推断结果。

---

## 8. Lower 阶段

接口：

- `LoweredProgramView LoweringPass::lower(const ProgramNode& program, const SemanticContext& semantic) const`

输入参数：

- `program`：AST
- `semantic`：语义上下文

输出参数：

- `LoweredProgramView`
  - `program` 指向 AST
  - `semantic` 指向语义信息

当前实现中该阶段不改写 AST，仅组织统一视图供后续代码生成使用。

---

## 9. C 代码生成

接口：

- `std::string CCodeGenerator::generate(const LoweredProgramView& program) const`

输入参数：

- `program.program`：AST 根
- `program.semantic`：类型和符号绑定信息

输出参数：

- 目标 C 源码字符串

### 9.1 本例的关键生成点

1. 生成头文件 `#include <stdio.h>`
2. 根据全局变量声明生成 `int a;`
3. 生成 `int main()` 主函数
4. 将 `a := 3` 生成为 `a = 3;`
5. 将 `write(a)` 生成为 `printf("%d", a);`
   - `%d` 来自 `a` 的语义类型 `integer`
6. 生成 `return 0;`

最终输出与 `tests/testcases/expected/00_main.c` 对齐。

---

## 10. 端到端时序（简化版）

```text
main(argc, argv)
  -> parseArgs(argc, argv)
  -> Compiler::compileFile(inputPath)
      -> readTextFile(path)
      -> Compiler::compileSource(source)
          -> Lexer::tokenize(source)
          -> Parser::parse(tokens)
              -> parseProgram -> parseBlock -> ...
          -> SemanticAnalyzer::analyze(program)
          -> LoweringPass::lower(program, semantic)
          -> CCodeGenerator::generate(lowered)
  -> 写入 outputPath
```

## 11. 对调试最有用的观测点

如果要逐步跟踪 `00_main`，建议断点顺序：

1. `src/main.cpp` 中 `compiler.compileFile(...)`
2. `src/driver/compiler.cpp` 中 `compileSource(...)` 每一步赋值行
3. `src/lexer/lexer.cpp` 中 `scanIdentifierOrKeyword` 与 `scanSymbol`
4. `src/parser/parser.cpp` 中 `parseProgram`、`parseVarDeclaration`、`parseWriteStatement`
5. `src/semantic/analyzer.cpp` 中变量声明与表达式类型推断位置
6. `src/codegen/c_codegen.cpp` 中 `emitVarDecl`、`emitStatement(WriteStmtNode)`
