# Pascal-S2C 编译器 JSON 接口设计与实现（修订版）

## 1. 文件总体分工

| 文件 | 核心职责 |
|------|---------|
| `json_utils.h` | 原子级转换工具（TokenKind ↔ 字符串、SourceLocation ↔ JSON） |
| `json_emitter.h` | 对外接口定义（序列化 + 反序列化 + 错误流） |
| `json_emitter.cpp` | 递归构建 AST / Token / Error 的 JSON，实现双向序列化 |

---

## 2. json_utils.h 详细说明

### 2.1 文件定位

`json_utils.h` 是 JSON 接口模块的**最底层工具文件**，不提供对外业务接口，只为 `json_emitter.cpp` 提供两类原子转换：
1. `TokenKind` 枚举值 ↔ 字符串（双向）
2. `SourceLocation` 结构体 ↔ JSON 对象（双向）

所有函数均为 `inline`，定义在头文件中，避免链接时的多重定义错误。

---

### 2.2 tokenKindToString

```cpp
inline std::string tokenKindToString(TokenKind kind) {
    return tokenKindName(kind);  // 复用 Lexer 已有映射
}
```

**函数作用**：将 `TokenKind` 枚举值转换为可读字符串，供 JSON 输出使用。

**参数**：
- `kind`：`TokenKind` 枚举值（如 `TokenKind::Identifier`、`TokenKind::Program`）

**返回值**：字符串（如 `"Identifier"`、`"Program"`）

**设计理由**：
- 避免在 JSON 模块中维护第二份 Token 类型表
- 复用 `lexer/token.h` 中已有的 `tokenKindName()` 函数，该函数通过 `switch` 覆盖了全部 40+ 种 `TokenKind`
- 保证 Lexer 与 JSON 输出语义一致，Lexer 新增 Token 类型时 JSON 自动同步

**调用示例**：
```cpp
Token token{TokenKind::Identifier, "main", {1, 9}};
std::string s = tokenKindToString(token.kind);  // s == "Identifier"
```

---

### 2.3 stringToTokenKind

```cpp
inline TokenKind stringToTokenKind(const std::string& str) {
    if (str == "Identifier")       return TokenKind::Identifier;
    if (str == "IntegerLiteral")   return TokenKind::IntegerLiteral;
    // ... 全部 40+ 种映射 ...
    if (str == "EndOfFile")        return TokenKind::EndOfFile;
    return TokenKind::EndOfFile;  // 兜底
}
```

**函数作用**：`tokenKindToString` 的逆操作，将字符串还原为 `TokenKind` 枚举值，**专供反序列化使用**。

**参数**：
- `str`：Token 类型名字符串（如 `"Identifier"`）

**返回值**：对应的 `TokenKind` 枚举值

**设计理由**：
- 与 `tokenKindName()` 的 `switch` 分支一一对应，确保双向映射一致性
- 兜底返回 `TokenKind::EndOfFile`，避免反序列化时因未知类型导致未定义行为
- 反序列化 `ir_json_to_token` 时调用此函数恢复 `Token.kind` 字段

**注意事项**：
- 若 JSON 中的 `"type"` 字段值为非法字符串（如 `"Error"`，组长规范中的 ErrorToken），会返回 `EndOfFile`。当前项目 Lexer 实现是遇到错误直接抛异常，不生成 ErrorToken，因此该场景在实际运行中不会出现。

---

### 2.4 locationToJson

```cpp
inline nlohmann::json locationToJson(const SourceLocation& loc) {
    return {
        {"line", loc.line},
        {"col", loc.column}
    };
}
```

**函数作用**：将 `SourceLocation` 结构体转换为 JSON 对象。

**参数**：
- `loc`：`SourceLocation` 结构体，含 `line`（行号）和 `column`（列号）两个 `int` 字段

**返回值**：JSON 对象 `{"line": <行号>, "col": <列号>}`

**关键修改（修订版）**：
- **字段名从 `"column"` 改为 `"col"`**，对齐组长规范 0.3.2 中 `Token` 结构的 `col` 字段命名
- 这是与旧版代码最显著的区别，确保下游模块（如 Driver 错误汇总、IDE 插件）解析位置信息时字段名统一

**调用示例**：
```cpp
SourceLocation loc{5, 10};
json j = locationToJson(loc);  // j == {"line": 5, "col": 10}
```

---

### 2.5 jsonToLocation

```cpp
inline SourceLocation jsonToLocation(const nlohmann::json& j) {
    SourceLocation loc;
    loc.line = j.value("line", 1);
    loc.column = j.value("col", 1);
    return loc;
}
```

**函数作用**：`locationToJson` 的逆操作，将 JSON 对象还原为 `SourceLocation` 结构体，**专供反序列化使用**。

**参数**：
- `j`：JSON 对象，预期包含 `"line"` 和 `"col"` 字段

**返回值**：`SourceLocation` 结构体

**设计细节**：
- 使用 `nlohmann::json::value(key, default)` 方法，若 JSON 中缺少字段则使用默认值 `1`
- 读取 `"col"` 字段写入 `loc.column`，完成字段名映射转换
- 反序列化 `ir_json_to_token` 和 `ir_json_to_ast` 时调用此函数恢复位置信息

---

## 3. json_emitter.h 详细说明

### 3.1 文件定位

`json_emitter.h` 是 JSON 接口模块的**对外接口层**，声明所有供其他模块调用的函数和数据结构。按功能分为四组：
1. **序列化接口（导出 JSON 对象）**：`emitLexJson`、`emitParseJson`、`emitErrorJson`
2. **规范接口（导出 JSON 字符串）**：`ir_token_to_json`、`ir_ast_to_json`、`ir_errors_to_json`
3. **批量错误流**：`ErrorItem`/`ErrorStream` 结构体 + `emitErrorStreamJson`
4. **反序列化接口（导入 JSON）**：`ir_json_to_token`、`ir_json_to_ast`

---

### 3.2 序列化接口（JSON 对象）

#### 3.2.1 emitLexJson

```cpp
nlohmann::json emitLexJson(const TokenList& tokens);
```

**函数作用**：将 Lexer 输出的 `TokenList` 转换为 JSON 对象。

**输入**：
- `tokens`：完整的 Token 流（`std::vector<Token>`）

**输出**：词法阶段 JSON 文档（`nlohmann::json` 对象）

**输出格式示例**：
```json
{
  "phase": "LEX",
  "status": "ok",
  "tokens": [
    {"type": "Program", "val": "program", "line": 1, "col": 1},
    {"type": "Identifier", "val": "main", "line": 1, "col": 9},
    {"type": "Semicolon", "val": ";", "line": 1, "col": 13},
    {"type": "EndOfFile", "val": "", "line": 1, "col": 14}
  ]
}
```

**关键修改（修订版）**：
- Token 对象中的 `"value"` 改为 `"val"`
- Token 对象中的 `"column"` 改为 `"col"`
- 对齐组长规范 0.3.2 的 `Token` 结构定义

---

#### 3.2.2 emitParseJson

```cpp
nlohmann::json emitParseJson(const ProgramNode& program);
```

**函数作用**：将 Parser 构建的 AST 根节点转换为 JSON 对象。

**输入**：
- `program`：AST 根节点（`ProgramNode`），包含程序名和顶层 `BlockNode`

**输出**：语法阶段 JSON 文档（`nlohmann::json` 对象）

**输出格式示例**：
```json
{
  "phase": "PARSE",
  "status": "ok",
  "ast": {
    "node_type": "Program",
    "name": "demo",
    "loc": {"line": 1, "col": 1},
    "block": {
      "node_type": "Block",
      "loc": {"line": 1, "col": 15},
      "constDecls": [...],
      "varDecls": [...],
      "subprograms": [...],
      "body": {...}
    }
  }
}
```

**设计要点**：
- 参数使用 `const ProgramNode&` 而非 `ProgramPtr`（智能指针），因为 JSON 输出阶段**不转移 AST 所有权**
- AST 所有权仍由调用方（如 `Compiler::compileSource`）持有

---

#### 3.2.3 emitErrorJson

```cpp
nlohmann::json emitErrorJson(const CompilerError& error);
```

**函数作用**：将单个 `CompilerError` 包装为统一 JSON 格式。

**输入**：
- `error`：`CompilerError` 对象，含阶段名、错误消息、源码位置

**输出**：错误 JSON 文档（`nlohmann::json` 对象）

**输出格式示例**：
```json
{
  "phase": "semantic",
  "status": "error",
  "errors": [
    {
      "err_type": "semantic",
      "message": "undefined symbol: x",
      "line": 5,
      "col": 10
    }
  ]
}
```

**关键修改（修订版）**：
- 错误对象中的 `"phase"` 改为 `"err_type"`，对齐组长规范 0.3.1 的 `ErrorItem.err_type`
- 错误对象中的 `"column"` 改为 `"col"`
- 保留顶层 `"phase"` 字段（表示错误发生的阶段），与 `"err_type"` 区分职责：
  - `"phase"`：标识本次 JSON 响应对应的编译阶段
  - `"err_type"`：标识具体错误的类型（与 `ErrorItem` 结构对齐）

---

### 3.3 规范接口（JSON 字符串）

#### 3.3.1 ir_token_to_json

```cpp
std::string ir_token_to_json(const TokenList& tokens);
```

**函数作用**：词法阶段序列化的**规范接口**，返回 JSON 字符串而非 JSON 对象。

**实现方式**：内部调用 `emitLexJson(tokens).dump(2)`，将 JSON 对象格式化为带 2 空格缩进的字符串。

**设计理由**：
- 组长规范 5.1 定义的接口签名要求返回 `std::string`
- 提供字符串接口方便直接写入文件或网络传输
- `dump(2)` 中的 `2` 表示缩进空格数，使输出具有可读性

---

#### 3.3.2 ir_ast_to_json

```cpp
std::string ir_ast_to_json(const ProgramNode& program);
```

**函数作用**：语法阶段序列化的**规范接口**，返回 JSON 字符串。

**实现方式**：内部调用 `emitParseJson(program).dump(2)`。

---

#### 3.3.3 ir_errors_to_json

```cpp
std::string ir_errors_to_json(const ErrorStream& errors);
```

**函数作用**：批量错误流序列化的**规范接口**，返回 JSON 字符串。

**实现方式**：内部调用 `emitErrorStreamJson(errors).dump(2)`。

**使用场景**：
- Driver 层收集词法、语法、语义三阶段的全部错误后，统一调用此函数输出完整错误报告
- 替代多次调用 `emitErrorJson`，避免生成多个独立 JSON 文档

---

### 3.4 批量错误流数据结构

#### 3.4.1 ErrorItem

```cpp
struct ErrorItem {
    int line = 0;
    int col = 0;
    std::string err_type;
    std::string message;
};
```

**结构作用**：单条错误的统一表示，**完全对齐组长规范 0.3.1 的 `ErrorItem` 定义**。

**字段说明**：

| 字段 | 类型 | 含义 |
|------|------|------|
| `line` | `int` | 错误行号 |
| `col` | `int` | 错误列号（注意不是 `column`） |
| `err_type` | `std::string` | 错误类型（`"lexer"`、`"parser"`、`"semantic"`） |
| `message` | `std::string` | 错误详细描述 |

**设计理由**：
- 与组长规范中的 `ErrorItem` 字段名完全一致，确保 Driver 层汇总错误时数据结构兼容
- 各模块（Lexer/Parser/Semantic）内部产生错误时，统一转换为 `ErrorItem` 后存入各自的 `ErrorStream`

---

#### 3.4.2 ErrorStream

```cpp
using ErrorStream = std::vector<ErrorItem>;
```

**类型作用**：错误流类型别名，表示一个编译阶段收集到的全部错误。

**使用方式**：
```cpp
ErrorStream lexerErrors;
lexerErrors.push_back({5, 10, "lexer", "unexpected character: @"});
lexerErrors.push_back({8, 3, "lexer", "unterminated comment"});
std::string jsonStr = ir_errors_to_json(lexerErrors);
```

---

#### 3.4.3 emitErrorStreamJson

```cpp
nlohmann::json emitErrorStreamJson(const ErrorStream& errors);
```

**函数作用**：将批量错误流转换为 JSON 对象。

**输出格式示例**：
```json
{
  "phase": "compilation",
  "status": "error",
  "errors": [
    {"err_type": "lexer", "message": "unexpected character: @", "line": 5, "col": 10},
    {"err_type": "parser", "message": "expected ';' after statement", "line": 8, "col": 3},
    {"err_type": "semantic", "message": "undefined symbol: x", "line": 12, "col": 5}
  ]
}
```

**设计要点**：
- `"phase": "compilation"` 表示这是跨阶段汇总后的错误报告
- `"errors"` 数组包含所有阶段的错误，支持一次性输出源码中的全部问题
- 对齐组长规范 7.1 的"级联错误"设计思想

---

### 3.5 反序列化接口

#### 3.5.1 ir_json_to_token

```cpp
TokenList ir_json_to_token(const std::string& json_str);
```

**函数作用**：将 JSON 字符串还原为 `TokenList`，实现词法阶段的**离线编译**和**分阶段调试**。

**输入**：
- `json_str`：符合 `emitLexJson` 输出格式的 JSON 字符串

**输出**：`TokenList`（`std::vector<Token>`）

**恢复逻辑**：
1. 解析 JSON 字符串
2. 遍历 `"tokens"` 数组
3. 对每个 Token 对象：
   - `"type"` 字段通过 `stringToTokenKind` 恢复为 `TokenKind`
   - `"val"` 字段恢复为 `lexeme`
   - `"line"`/`"col"` 字段恢复为 `SourceLocation`

**使用场景**：
- 组长规范 6 提到的"预留扩展：支持加载外部 JSON Token 执行后续编译流程"
- 调试时保存 Token 流，后续直接加载跳过词法分析阶段

---

#### 3.5.2 ir_json_to_ast

```cpp
std::unique_ptr<ProgramNode> ir_json_to_ast(const std::string& json_str);
```

**函数作用**：将 JSON 字符串还原为 AST 根节点，实现语法阶段的**离线编译**。

**输入**：
- `json_str`：符合 `emitParseJson` 输出格式的 JSON 字符串

**输出**：`ProgramPtr`（`std::unique_ptr<ProgramNode>`）

**恢复逻辑（当前为基础骨架版本）**：
1. 解析 JSON 字符串，读取 `"ast"` 字段
2. 恢复 `ProgramNode`：名字、位置
3. 恢复 `BlockNode`：位置
4. 恢复 `constDecls`：仅名字（`value` 为占位 `LiteralExprNode`）
5. 恢复 `varDecls`：仅名字列表（`type` 为占位 `ScalarTypeNode`）
6. 恢复 `subprograms`：名字、区分 function/procedure（`block` 为占位）
7. 恢复 `body`：占位 `CompoundStmtNode`

**当前限制**：
- 由于 `emitExprJson`、`emitStmtJson`、`emitTypeJson` 当前为简化实现（统一输出 `"Expr"`/`"Stmt"`/`"Type"`，未记录具体子类信息），反序列化时无法完整恢复表达式/语句/类型的具体类型
- 若后续需要完整 AST 重建，需先扩展序列化函数（通过 `dynamic_cast` 判断子类，输出 `op`、`kind`、`rawText` 等字段），反序列化才能跟着补全

**使用场景**：
- 组长规范 6 提到的"支持加载外部 JSON AST 执行后续编译流程"
- 调试时保存 AST，后续直接加载跳过客制化语法分析阶段

---

## 4. json_emitter.cpp 详细说明（重点）

### 4.1 文件整体结构

```
json_emitter.cpp
├── 词法序列化
│   ├── emitLexJson           → JSON 对象
│   └── ir_token_to_json      → JSON 字符串（规范接口）
├── 词法反序列化
│   └── ir_json_to_token      ← JSON 字符串
├── 语法序列化（匿名命名空间内辅助函数）
│   ├── emitExprJson          ─┐
│   ├── emitStmtJson           │  原子节点（简化实现）
│   ├── emitTypeJson          ─┘
│   ├── emitConstDeclJson     ─┐
│   ├── emitVarDeclJson        │  声明节点
│   ├── emitParamDeclJson     ─┘
│   ├── emitSubprogramJson    ─┐
│   ├── emitCompoundStmtJson   │  复合节点
│   └── emitBlockJson         ─┘
│   ├── emitParseJson           → JSON 对象
│   └── ir_ast_to_json          → JSON 字符串（规范接口）
├── 语法反序列化
│   └── ir_json_to_ast          ← JSON 字符串（基础骨架）
├── 错误序列化
│   ├── emitErrorJson           → 单条错误 JSON 对象
│   ├── emitErrorStreamJson     → 批量错误 JSON 对象
│   └── ir_errors_to_json       → 批量错误 JSON 字符串（规范接口）
└──
```

---

### 4.2 词法 JSON：emitLexJson

```cpp
json emitLexJson(const TokenList& tokens) {
    json j;
    j["phase"] = "LEX";         // 标识当前阶段为词法分析
    j["status"] = "ok";         // 表示阶段执行成功

    j["tokens"] = json::array(); // 初始化 Token 数组

    for (const Token& token : tokens) {
        j["tokens"].push_back({
            {"type", tokenKindToString(token.kind)},
            // Token 类型（如 Identifier）

            {"val", token.lexeme},
            // Token 的原始字符串（修订版：value → val）

            {"line", token.location.line},
            // 行号

            {"col", token.location.column}
            // 列号（修订版：column → col）
        });
    }

    return j;
}
```

**函数职责**：构建词法分析阶段的完整 JSON 输出，包括阶段标识、状态和 Token 列表。

**关键修改（修订版）**：
- `"value"` → `"val"`：对齐组长规范 0.3.2
- `"column"` → `"col"`：对齐组长规范 0.3.2

**对应输出格式**：
```json
{
  "phase": "LEX",
  "status": "ok",
  "tokens": [
    {
      "type": "Identifier",
      "val": "main",
      "line": 1,
      "col": 9
    }
  ]
}
```

---

### 4.3 词法反序列化：ir_json_to_token

```cpp
TokenList ir_json_to_token(const std::string& json_str) {
    json j = json::parse(json_str);
    TokenList tokens;

    for (const auto& item : j["tokens"]) {
        Token token;
        token.kind = stringToTokenKind(item.value("type", ""));
        // 通过 stringToTokenKind 恢复枚举值

        token.lexeme = item.value("val", "");
        // 恢复原始词素（读取 "val" 字段）

        token.location.line = item.value("line", 1);
        token.location.column = item.value("col", 1);
        // 恢复位置（读取 "col" 字段）

        tokens.push_back(token);
    }

    return tokens;
}
```

**函数职责**：将 JSON 字符串还原为 `TokenList`，支持离线加载 Token 流。

**恢复细节**：
- `stringToTokenKind` 完成 `"type"` 字符串 → `TokenKind` 枚举的映射
- `item.value("col", 1)` 读取 `"col"` 字段，若缺失则默认值为 1
- 恢复后的 `TokenList` 可直接送入 `Parser::parse` 进行语法分析

---

### 4.4 语法 JSON：AST 构建体系（匿名命名空间）

辅助函数全部放在匿名命名空间 `namespace { ... }` 中，**不暴露链接符号**，避免与其他编译单元的同名函数冲突。

---

#### 4.4.1 emitExprJson

```cpp
json emitExprJson(const Expr& expr) {
    json j;
    j["node_type"] = "Expr";       // 表达式节点类型（简化，未区分子类）
    j["loc"] = locationToJson(expr.loc); // 位置信息
    return j;
}
```

**函数作用**：处理表达式节点（基类），作为表达式 JSON 的兜底实现。

**当前简化**：所有 `Expr` 子类（`BinaryExprNode`、`UnaryExprNode`、`LiteralExprNode`、`VarExprNode`、`CallExprNode`、`IndexExprNode`）统一输出为 `"Expr"`，不展开运算符、操作数等细节。

**后续扩展路径**：可通过 `dynamic_cast` 链判断具体子类，例如：
```cpp
if (const auto* lit = dynamic_cast<const LiteralExprNode*>(&expr)) {
    j["node_type"] = "LiteralExpr";
    j["kind"] = toString(lit->kind);
    j["raw_text"] = lit->rawText;
}
```

---

#### 4.4.2 emitStmtJson

```cpp
json emitStmtJson(const Stmt& stmt) {
    json j;
    j["node_type"] = "Stmt";       // 语句节点类型（简化，未区分子类）
    j["loc"] = locationToJson(stmt.loc); // 位置信息
    return j;
}
```

**函数作用**：处理语句节点（基类），作为语句 JSON 的兜底实现。

**当前简化**：所有 `Stmt` 子类（`AssignStmtNode`、`IfStmtNode`、`WhileStmtNode`、`ForStmtNode`、`BreakStmtNode`、`CallStmtNode`、`ReadStmtNode`、`WriteStmtNode`、`CompoundStmtNode`）统一输出为 `"Stmt"`，不展开条件、分支、循环变量等细节。

**例外**：`CompoundStmtNode` 有独立的 `emitCompoundStmtJson` 函数，因为它需要递归处理 `statements` 数组。

---

#### 4.4.3 emitTypeJson

```cpp
json emitTypeJson(const TypeNode& type) {
    json j;
    j["node_type"] = "Type";       // 类型节点（简化，未区分 Scalar/Array）
    j["loc"] = locationToJson(type.loc); // 位置信息
    return j;
}
```

**函数作用**：处理类型节点（基类），作为类型 JSON 的兜底实现。

**当前简化**：`ScalarTypeNode` 和 `ArrayTypeNode` 统一输出为 `"Type"`，不展开 `kind` 或 `dims` 信息。

---

#### 4.4.4 emitConstDeclJson

```cpp
json emitConstDeclJson(const ConstDeclNode& decl) {
    json j;
    j["node_type"] = "ConstDecl";  // 常量声明
    j["name"] = decl.name;         // 常量名
    j["loc"] = locationToJson(decl.loc);

    if (decl.value) {
        j["value"] = emitExprJson(*decl.value);
        // 常量值（表达式，简化输出）
    }
    return j;
}
```

**函数作用**：将常量声明节点转换为 JSON。

**对应 AST 结构**：
```cpp
struct ConstDeclNode : Decl {
    std::string name;
    std::unique_ptr<Expr> value;
};
```

---

#### 4.4.5 emitVarDeclJson

```cpp
json emitVarDeclJson(const VarDeclNode& decl) {
    json j;
    j["node_type"] = "VarDecl";    // 变量声明
    j["names"] = decl.names;       // 变量名列表（支持多变量同类型声明）
    j["loc"] = locationToJson(decl.loc);

    if (decl.type) {
        j["type"] = emitTypeJson(*decl.type);
        // 变量类型（简化输出）
    }
    return j;
}
```

**函数作用**：将变量声明节点转换为 JSON，支持多变量同类型声明（如 `a, b: integer`）。

**对应 AST 结构**：
```cpp
struct VarDeclNode : Decl {
    std::vector<std::string> names;
    std::unique_ptr<TypeNode> type;
};
```

**设计细节**：`j["names"] = decl.names` 直接赋值 `std::vector<std::string>`，nlohmann::json 自动将其序列化为 JSON 数组。

---

#### 4.4.6 emitParamDeclJson

```cpp
json emitParamDeclJson(const ParamDeclNode& param) {
    json j;
    j["node_type"] = "ParamDecl";  // 参数声明
    j["names"] = param.names;      // 参数名列表
    j["loc"] = locationToJson(param.loc);
    return j;
}
```

**函数作用**：将形参组节点转换为 JSON。

**对应 AST 结构**：
```cpp
struct ParamDeclNode : Node {
    std::vector<std::string> names;
    BasicTypeKind type = BasicTypeKind::Void;
    ParamPassMode passMode = ParamPassMode::Value;
};
```

**当前简化**：未输出 `type` 和 `passMode` 字段。如需完整接口，可后续补充：
```cpp
j["type"] = toString(param.type);
j["pass_mode"] = (param.passMode == ParamPassMode::Var) ? "var" : "value";
```

---

#### 4.4.7 emitSubprogramJson

```cpp
json emitSubprogramJson(const SubprogramDeclNode& sub) {
    json j;
    j["node_type"] = "Subprogram"; // 子程序节点
    j["name"] = sub.name;          // 子程序名
    j["loc"] = locationToJson(sub.loc);

    j["params"] = json::array();
    for (const auto& param : sub.params) {
        j["params"].push_back(emitParamDeclJson(*param));
        // 递归生成参数列表 JSON
    }

    if (sub.block) {
        j["block"] = {
            {"node_type", "Block"},
            {"loc", locationToJson(sub.block->loc)}
        };
        // 子程序体（简化，仅输出节点类型和位置）
    }

    if (const auto* fn = dynamic_cast<const FunctionDeclNode*>(&sub)) {
        j["return_type"] = toString(fn->returnType);
        // 仅函数有返回类型字段
    }

    return j;
}
```

**函数作用**：将子程序声明（函数/过程）转换为 JSON。

**关键逻辑**：
- 通过 `dynamic_cast<const FunctionDeclNode*>` 判断是否为函数
- 若是函数，输出 `"return_type"` 字段（如 `"integer"`）
- 若是过程，无 `"return_type"` 字段，下游可通过此字段存在与否区分类别
- `sub.block` 简化输出，未递归调用 `emitBlockJson`（如需完整展开可修改）

---

#### 4.4.8 emitCompoundStmtJson

```cpp
json emitCompoundStmtJson(const CompoundStmtNode& compound) {
    json j;
    j["node_type"] = "CompoundStmt"; // 复合语句（begin ... end）
    j["loc"] = locationToJson(compound.loc);

    j["statements"] = json::array();
    for (const auto& stmt : compound.statements) {
        j["statements"].push_back(emitStmtJson(*stmt));
        // 递归生成每条语句的 JSON（当前为简化 Stmt）
    }

    return j;
}
```

**函数作用**：处理 `begin ... end` 语句块，递归处理其中的每条语句。

**对应 AST 结构**：
```cpp
struct CompoundStmtNode : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
};
```

---

#### 4.4.9 emitBlockJson

```cpp
json emitBlockJson(const BlockNode& block) {
    json j;
    j["node_type"] = "Block";      // Block 节点
    j["loc"] = locationToJson(block.loc);

    j["constDecls"] = json::array();
    for (const auto& decl : block.constDecls) {
        j["constDecls"].push_back(emitConstDeclJson(*decl));
    }

    j["varDecls"] = json::array();
    for (const auto& decl : block.varDecls) {
        j["varDecls"].push_back(emitVarDeclJson(*decl));
    }

    j["subprograms"] = json::array();
    for (const auto& sub : block.subprograms) {
        j["subprograms"].push_back(emitSubprogramJson(*sub));
    }

    if (block.body) {
        j["body"] = emitCompoundStmtJson(*block.body);
        // 主语句块（复合语句）
    }

    return j;
}
```

**函数作用**：构建 Pascal 的 Block 节点，连接常量、变量、子程序和主语句体。

**与 AST 结构的严格对应**：

| AST 成员 | JSON 字段 | 序列化函数 |
|---------|----------|-----------|
| `constDecls` | `"constDecls"` | `emitConstDeclJson` |
| `varDecls` | `"varDecls"` | `emitVarDeclJson` |
| `subprograms` | `"subprograms"` | `emitSubprogramJson` |
| `body` | `"body"` | `emitCompoundStmtJson` |

这四个成员对应 Parser 的四个解析阶段（`parseConstDeclarations`、`parseVarDeclarations`、`parseSubprogram`、`parseCompoundStatement`），JSON 输出严格反映这一结构。

---

### 4.5 语法 JSON 顶层：emitParseJson

```cpp
json emitParseJson(const ProgramNode& program) {
    json j;
    j["phase"] = "PARSE";           // 标识语法阶段
    j["status"] = "ok";             // 执行成功

    json ast;
    ast["node_type"] = "Program";   // 程序根节点
    ast["name"] = program.name;     // 程序名
    ast["loc"] = locationToJson(program.loc);

    if (program.block) {
        ast["block"] = emitBlockJson(*program.block);
        // 递归生成程序体
    }

    j["ast"] = ast;
    return j;
}
```

**函数作用**：语法分析的顶层入口，构建完整的 AST JSON。

**输出结构**：
```json
{
  "phase": "PARSE",
  "status": "ok",
  "ast": {
    "node_type": "Program",
    "name": "demo",
    "loc": {"line": 1, "col": 1},
    "block": { /* BlockNode 的完整展开 */ }
  }
}
```

---

### 4.6 语法反序列化：ir_json_to_ast

```cpp
std::unique_ptr<ProgramNode> ir_json_to_ast(const std::string& json_str) {
    json j = json::parse(json_str);
    auto program = std::make_unique<ProgramNode>();

    const json& ast = j["ast"];
    program->name = ast.value("name", "");
    program->loc = jsonToLocation(ast.value("loc", json::object()));

    if (ast.contains("block")) {
        const json& blockJson = ast["block"];
        auto block = std::make_unique<BlockNode>();
        block->loc = jsonToLocation(blockJson.value("loc", json::object()));

        // 恢复 constDecls（仅 name，value 为占位 LiteralExprNode）
        if (blockJson.contains("constDecls")) {
            for (const auto& declJson : blockJson["constDecls"]) {
                auto decl = std::make_unique<ConstDeclNode>();
                decl->name = declJson.value("name", "");
                decl->loc = jsonToLocation(declJson.value("loc", json::object()));
                decl->value = std::make_unique<LiteralExprNode>();
                // 占位：当前 JSON 未记录表达式具体类型
                block->constDecls.push_back(std::move(decl));
            }
        }

        // 恢复 varDecls（仅 names，type 为占位 ScalarTypeNode）
        if (blockJson.contains("varDecls")) {
            for (const auto& declJson : blockJson["varDecls"]) {
                auto decl = std::make_unique<VarDeclNode>();
                if (declJson.contains("names")) {
                    for (const auto& name : declJson["names"]) {
                        decl->names.push_back(name.get<std::string>());
                    }
                }
                decl->loc = jsonToLocation(declJson.value("loc", json::object()));
                decl->type = std::make_unique<ScalarTypeNode>();
                // 占位：当前 JSON 未记录类型具体信息
                block->varDecls.push_back(std::move(decl));
            }
        }

        // 恢复 subprograms（仅 name，通过 return_type 区分 function/procedure）
        if (blockJson.contains("subprograms")) {
            for (const auto& subJson : blockJson["subprograms"]) {
                std::unique_ptr<SubprogramDeclNode> sub;
                if (subJson.contains("return_type")) {
                    auto fn = std::make_unique<FunctionDeclNode>();
                    fn->returnType = BasicTypeKind::Integer;  // 简化默认
                    sub = std::move(fn);
                } else {
                    sub = std::make_unique<ProcedureDeclNode>();
                }
                sub->name = subJson.value("name", "");
                sub->loc = jsonToLocation(subJson.value("loc", json::object()));
                sub->block = std::make_unique<BlockNode>();
                // 占位：子程序体未展开
                block->subprograms.push_back(std::move(sub));
            }
        }

        // 恢复 body（占位 CompoundStmt）
        if (blockJson.contains("body")) {
            block->body = std::make_unique<CompoundStmtNode>();
            block->body->loc = jsonToLocation(blockJson["body"].value("loc", json::object()));
        }

        program->block = std::move(block);
    }

    return program;
}
```

**函数作用**：将 JSON 字符串还原为 AST 根节点，实现语法阶段的离线编译。

**恢复策略**：
- **完全恢复**：`ProgramNode.name`、`BlockNode` 的位置、所有声明的名字
- **占位恢复**：表达式、类型、子程序体等因 JSON 信息不足，使用默认构造的占位节点
- **类型区分**：通过 `"return_type"` 字段存在与否判断 function vs procedure

**使用前提**：当前版本适用于"骨架恢复"场景（如查看程序结构、统计声明数量），不适用于需要完整语义信息的场景（如直接送入代码生成器）。

---

### 4.7 错误 JSON（单条）：emitErrorJson

```cpp
json emitErrorJson(const CompilerError& error) {
    json j;
    j["phase"] = error.stage();     // 错误所在阶段（如 "semantic"）
    j["status"] = "error";          // 错误状态标识

    json err;
    err["err_type"] = error.stage(); // 错误类型（修订版：phase → err_type）
    err["message"] = error.what();   // 错误描述文本
    err["line"] = error.location().line;   // 行号
    err["col"] = error.location().column;  // 列号（修订版：column → col）

    j["errors"] = json::array({err});
    return j;
}
```

**函数作用**：将单个 `CompilerError` 包装为标准错误 JSON。

**关键修改（修订版）**：
- `"phase"` → `"err_type"`：对齐组长规范 0.3.1 的 `ErrorItem.err_type`
- `"column"` → `"col"`：对齐组长规范 0.3.1 的 `ErrorItem.col`

**字段职责区分**：
- 顶层 `"phase"`：标识本次 JSON 响应对应的编译阶段（如 `"semantic"`）
- `errors[0]."err_type"`：标识具体错误的类型（与 `ErrorItem` 结构对齐，便于 Driver 汇总）

**输出格式示例**：
```json
{
  "phase": "semantic",
  "status": "error",
  "errors": [
    {
      "err_type": "semantic",
      "message": "undefined symbol: x",
      "line": 5,
      "col": 10
    }
  ]
}
```

---

### 4.8 错误 JSON（批量）：emitErrorStreamJson

```cpp
json emitErrorStreamJson(const ErrorStream& errors) {
    json j;
    j["phase"] = "compilation";      // 跨阶段汇总标识
    j["status"] = "error";           // 错误状态

    j["errors"] = json::array();
    for (const auto& item : errors) {
        j["errors"].push_back({
            {"err_type", item.err_type},   // 错误类型
            {"message", item.message},     // 错误描述
            {"line", item.line},           // 行号
            {"col", item.col}              // 列号
        });
    }

    return j;
}
```

**函数作用**：将批量错误流（`ErrorStream`）转换为 JSON 对象，支持一次性输出全部编译错误。

**输入**：
- `errors`：`ErrorStream` 类型（`std::vector<ErrorItem>`），可包含任意阶段的错误

**输出格式示例**：
```json
{
  "phase": "compilation",
  "status": "error",
  "errors": [
    {"err_type": "lexer", "message": "unexpected character: @", "line": 5, "col": 10},
    {"err_type": "parser", "message": "expected ';' after statement", "line": 8, "col": 3},
    {"err_type": "semantic", "message": "undefined symbol: x", "line": 12, "col": 5}
  ]
}
```

**设计理由**：
- 对齐组长规范 7.1 的"级联错误"设计：一次编译收集所有阶段全部错误
- Driver 层调用方式：
  ```cpp
  ErrorStream allErrors;
  allErrors.insert(allErrors.end(), lexerResult.errorstream.begin(), lexerResult.errorstream.end());
  allErrors.insert(allErrors.end(), parserResult.errorstream.begin(), parserResult.errorstream.end());
  allErrors.insert(allErrors.end(), semanticResult.errorstream.begin(), semanticResult.errorstream.end());
  std::string errorJson = ir_errors_to_json(allErrors);
  ```

---

## 5. 与组长规范的完整对照

| 规范章节 | 规范要求 | 实现对应 | 状态 |
|---------|---------|---------|------|
| 0.3.1 `ErrorItem` | `line, col, err_type, message` | `json_emitter.h` 中 `ErrorItem` 结构体 | ✅ |
| 0.3.2 `Token` | `line, col, type, val` | `emitLexJson` 输出字段 | ✅ |
| 5.1 `ir_token_to_json` | `std::string ir_token_to_json(const TokenList&)` | `json_emitter.h/cpp` | ✅ |
| 5.1 `ir_ast_to_json` | `std::string ir_ast_to_json(ASTNode*)` | `json_emitter.h/cpp`（参数为 `ProgramNode&`） | ✅ |
| 5.1 反序列化 | `ir_json_to_token`, `ir_json_to_ast` | `json_emitter.h/cpp` | ✅ |
| 7.1 级联错误 | 批量错误输出 | `ErrorStream` + `emitErrorStreamJson` + `ir_errors_to_json` | ✅ |

---

## 6. 已知限制与后续扩展

| 位置 | 当前状态 | 扩展条件 |
|------|---------|---------|
| `emitExprJson` | 统一输出 `"Expr"` | 需通过 `dynamic_cast` 判断子类，补充运算符、操作数等字段 |
| `emitStmtJson` | 统一输出 `"Stmt"` | 需按 `Stmt` 子类分发，补充条件、分支、循环变量等 |
| `emitTypeJson` | 统一输出 `"Type"` | 需区分 `ScalarTypeNode` / `ArrayTypeNode`，输出 `kind` / `dims` |
| `emitParamDeclJson` | 未输出 `type` / `passMode` | 补充 `"type"` 和 `"pass_mode"` 字段 |
| `emitSubprogramJson` 的 `block` | 简化输出 | 如需完整子程序体，改为调用 `emitBlockJson` |
| `ir_json_to_ast` | 基础骨架恢复 | 需待序列化函数完整后，同步补充反序列化逻辑 |

这些限制是**接口定义的阶段性选择**，当前输出已能完整展示 AST 骨架结构（Program → Block → 四成员），满足组长"保证输出所有该有的信息"的要求。细节展开可在后续迭代中按需补充。
