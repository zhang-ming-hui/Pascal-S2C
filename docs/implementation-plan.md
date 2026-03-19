# Pascal-S2C 实现方案

## 目标

构建一个最小可用的 Pascal-S 到 C 的翻译器，使其能够通过当前仓库中的
`tests/testcases/` 黄金用例。

本项目的实现目标应当以现有测试集覆盖到的语言子集为准，而不是一次性覆盖
教材版 Pascal-S 的全部语法。文档中出现、但当前测试集中没有出现的功能
`record`、`type`、`case`、`repeat until`、`downto`，应延后实现。

## 实现语言与总体方案

首版实现使用 C++ 完成，推荐标准为 C++17 或 C++20。

选择 C++ 的原因：

- 更符合编译原理课程项目和后续扩展的实际场景。
- 词法、语法、语义分析与代码生成都适合用较强类型系统组织。
- 标准库足够完成首版，不依赖额外第三方库。
- 后续如果需要增加中间表示、优化器、寄存器分配或目标代码后端，迁移成本更低。

首版不建议直接使用 C 实现全部模块。原因不是做不到，而是：

- AST、符号表、作用域、类型系统在 C 中组织成本显著更高。
- 错误恢复、字符串处理、容器管理、测试驱动会变得更重。
- 当前仓库的重点是尽快对齐黄金输出，而不是验证底层内存管理能力。

因此建议：

- 编译器前端与 C 代码生成器使用 C++ 实现。
- 生成目标仍然是 C 代码，与当前 `expected/*.c` 保持一致。

## 推荐目录结构

```text
.
|-- README.md
|-- docs/
|   |-- implementation-plan.md
|   |-- rule.md
|   `-- grammar/
|-- src/
|   |-- main.cpp
|   |-- common/
|   |   |-- error.h
|   |   |-- error.cpp
|   |   |-- location.h
|   |   `-- util.h
|   |-- lexer/
|   |   |-- token.h
|   |   |-- lexer.h
|   |   `-- lexer.cpp
|   |-- ast/
|   |   |-- ast.h
|   |   `-- ast.cpp
|   |-- parser/
|   |   |-- parser.h
|   |   `-- parser.cpp
|   |-- semantic/
|   |   |-- type.h
|   |   |-- symbol.h
|   |   |-- scope.h
|   |   |-- analyzer.h
|   |   `-- analyzer.cpp
|   |-- lower/
|   |   |-- lower.h
|   |   `-- lower.cpp
|   |-- codegen/
|   |   |-- c_writer.h
|   |   |-- c_writer.cpp
|   |   |-- c_codegen.h
|   |   `-- c_codegen.cpp
|   `-- driver/
|       |-- compiler.h
|       `-- compiler.cpp
|-- scripts/
|   `-- run_golden.py
|-- tests/
|   |-- testcases/
|   |   |-- pascal/
|   |   |-- expected/
|   |   `-- input/
|   `-- golden/
|       `-- test_golden.py
|-- CMakeLists.txt
`-- .gitignore
```

## 各模块职责

### `src/main.cpp`

- 命令行入口。
- 读取 Pascal 源文件。
- 调用编译驱动完成词法、语法、语义和代码生成。
- 将生成的 C 代码输出到标准输出或指定文件。

### `src/common/`

- 存放公共基础设施。
- `location.h`：记录 token 和 AST 节点的源码位置。
- `error.*`：统一错误类型、格式化输出和诊断信息。
- `util.h`：字符串、文件读取等小型工具函数。

### `src/lexer/`

- 将 Pascal 源码切分为 token 序列。
- 处理关键字、标识符、数字、字符常量、运算符和分隔符。
- 跳过空白字符和 Pascal 注释 `{ ... }`。
- 支持长标识符、混乱空白和紧凑排版。

### `src/ast/`

- 定义 AST 节点。
- AST 只负责承载语法结构，不直接承担语义分析或代码生成逻辑。
- 保持节点种类尽量少，避免在首版中过度设计。

### `src/parser/`

- 使用递归下降完成语法分析。
- 生成 AST。
- 首版不做复杂错误恢复，但错误信息必须足够定位问题。

### `src/semantic/`

- 建立作用域与符号表。
- 完成名字解析、类型推断和类型检查。
- 识别函数、过程、变量、常量、参数的语义属性。
- 判断 `var` 参数的传递方式。
- 为 `write(...)` 推导输出格式串。
- 为数组访问记录每一维边界信息。

### `src/lower/`

- 将 Pascal 特有语义转换为更容易生成 C 的中间形式。
- 将函数返回值赋值统一映射到伪返回变量 `_`。
- 统一处理数组下界偏移。
- 为 `var` 参数调用标记“按地址传递”。

### `src/codegen/`

- 负责把语义完成后的结构输出为 C 源码。
- 控制缩进、空行、语句格式和 include。
- 保持生成风格尽量贴近 `tests/testcases/expected/*.c`。

### `src/driver/`

- 封装编译流程。
- 对外暴露单一入口，例如 `compileFile()` 或 `compileSource()`。
- 便于命令行程序和测试脚本统一调用。

## 构建系统

推荐使用 CMake。

原因：

- 平台兼容性更好。
- 适合后续逐步增加模块和测试目标。
- 可以方便地生成调试构建和发布构建。

建议最小目标：

- 可执行文件：`pascal_s2c`

建议命令约定：

```text
cmake -S . -B build
cmake --build build
build/pascal_s2c path/to/input.pas
```

在 Windows 下可对应为：

```text
cmake -S . -B build
cmake --build build --config Debug
build\\Debug\\pascal_s2c.exe path\\to\\input.pas
```

## AST 设计

首版 AST 只需要覆盖测试集中真实出现的语法结构。

### 顶层节点

- `Program`
- `ConstDecl`
- `VarDecl`
- `FunctionDecl`
- `ProcedureDecl`
- `Block`

### 类型节点

- `ScalarType`
- `ArrayType`
- `ArrayBound`

### 语句节点

- `CompoundStmt`
- `AssignStmt`
- `CallStmt`
- `IfStmt`
- `WhileStmt`
- `ForStmt`
- `ReadStmt`
- `WriteStmt`

### 表达式节点

- `BinaryExpr`
- `UnaryExpr`
- `CallExpr`
- `VarExpr`
- `IndexExpr`
- `LiteralExpr`

## AST 推荐结构

下面给出一套适合首版的 C++ 组织方式，重点是简单、清晰、可扩展。

```cpp
enum class BasicTypeKind {
    Integer,
    Real,
    Boolean,
    Char,
    Void
};

struct SourceLocation {
    int line;
    int column;
};

struct Node {
    SourceLocation loc;
    virtual ~Node() = default;
};
```

### 类型节点

```cpp
struct TypeNode : Node {
    virtual ~TypeNode() = default;
};

struct ScalarTypeNode : TypeNode {
    BasicTypeKind kind;
};

struct ArrayBound {
    int lower;
    int upper;
};

struct ArrayTypeNode : TypeNode {
    std::vector<ArrayBound> dims;
    std::unique_ptr<ScalarTypeNode> elementType;
};
```

### 声明节点

```cpp
struct Decl : Node {
    virtual ~Decl() = default;
};

struct ConstDeclNode : Decl {
    std::string name;
    std::unique_ptr<class Expr> value;
};

struct VarDeclNode : Decl {
    std::vector<std::string> names;
    std::unique_ptr<TypeNode> type;
};

enum class ParamPassMode {
    Value,
    Var
};

struct ParamDeclNode : Node {
    std::vector<std::string> names;
    BasicTypeKind type;
    ParamPassMode passMode;
};
```

### 语句节点

```cpp
struct Stmt : Node {
    virtual ~Stmt() = default;
};

struct CompoundStmtNode : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
};

struct AssignStmtNode : Stmt {
    std::unique_ptr<class Expr> target;
    std::unique_ptr<class Expr> value;
};

struct CallStmtNode : Stmt {
    std::string name;
    std::vector<std::unique_ptr<class Expr>> args;
};

struct IfStmtNode : Stmt {
    std::unique_ptr<class Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;
};

struct WhileStmtNode : Stmt {
    std::unique_ptr<class Expr> condition;
    std::unique_ptr<Stmt> body;
};

struct ForStmtNode : Stmt {
    std::string varName;
    std::unique_ptr<class Expr> start;
    std::unique_ptr<class Expr> stop;
    std::unique_ptr<Stmt> body;
};

struct ReadStmtNode : Stmt {
    std::vector<std::unique_ptr<class Expr>> targets;
};

struct WriteStmtNode : Stmt {
    std::vector<std::unique_ptr<class Expr>> values;
};
```

### 表达式节点

```cpp
enum class BinaryOp {
    Add,
    Sub,
    Mul,
    RealDiv,
    IntDiv,
    Mod,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    And,
    Or
};

enum class UnaryOp {
    Plus,
    Minus,
    Not
};

struct Expr : Node {
    virtual ~Expr() = default;
};

struct BinaryExprNode : Expr {
    BinaryOp op;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

struct UnaryExprNode : Expr {
    UnaryOp op;
    std::unique_ptr<Expr> operand;
};

struct CallExprNode : Expr {
    std::string name;
    std::vector<std::unique_ptr<Expr>> args;
};

struct VarExprNode : Expr {
    std::string name;
};

struct IndexExprNode : Expr {
    std::string baseName;
    std::vector<std::unique_ptr<Expr>> indices;
};

enum class LiteralKind {
    Int,
    Real,
    Bool,
    Char
};

struct LiteralExprNode : Expr {
    LiteralKind kind;
    std::string rawText;
};
```

### 子程序与程序节点

```cpp
struct BlockNode : Node {
    std::vector<std::unique_ptr<ConstDeclNode>> constDecls;
    std::vector<std::unique_ptr<VarDeclNode>> varDecls;
    std::vector<std::unique_ptr<class SubprogramDeclNode>> subprograms;
    std::unique_ptr<CompoundStmtNode> body;
};

struct SubprogramDeclNode : Decl {
    std::string name;
    std::vector<std::unique_ptr<ParamDeclNode>> params;
    std::unique_ptr<BlockNode> block;
    virtual ~SubprogramDeclNode() = default;
};

struct FunctionDeclNode : SubprogramDeclNode {
    BasicTypeKind returnType;
};

struct ProcedureDeclNode : SubprogramDeclNode {
};

struct ProgramNode : Node {
    std::string name;
    std::unique_ptr<BlockNode> block;
};
```

## 语义分析设计

代码生成器不应该自行猜测语义。语义分析阶段至少要建立以下信息：

- 每个标识符引用最终绑定到哪个声明。
- 每个表达式的类型。
- `not` 到底应翻译成逻辑非还是按位非。
- `/` 是否需要执行浮点提升。
- 每个数组声明的每一维上下界。
- 某个函数调用的某个参数是否需要按地址传递。
- 每个 `write(...)` 应生成什么 `printf` 格式串。

建议语义分析器维护以下数据结构：

- `Scope`
  - 父作用域指针
  - 名字到 `Symbol` 的映射
- `Symbol`
  - 名字
  - 符号种类：常量、变量、参数、函数、过程
  - 类型信息
  - 是否全局
  - 如果是参数，记录是否为 `var`
  - 如果是数组，记录维度边界

建议不要把过多语义信息直接塞回 AST 节点中，优先使用“AST 节点指针到语义结果”的侧表。

## 首版支持的语法子集

首版应严格围绕当前测试集实现以下内容：

- `program`
- `const`
- `var`
- 基本类型：`integer`、`real`、`boolean`、`char`
- 多维数组
- `function`
- `procedure`
- 值参数
- `var` 参数
- 赋值语句
- 过程调用
- 函数调用
- `begin ... end`
- `if ... then ... else`
- `for ... to ... do`
- `while ... do`
- `read(...)`
- `write(...)`
- 运算符
  - `+`
  - `-`
  - `*`
  - `/`
  - `div`
  - `mod`
  - 比较运算
  - `and`
  - `or`
  - `not`
- 字面量
  - 整数
  - 实数
  - 字符
  - `true`
  - `false`

首版明确延后：

- `type`
- `record`
- `case`
- `repeat until`
- `downto`

## 代码生成规则

代码生成应尽量贴近现有黄金输出。

首版必须满足的规则：

- 程序级变量翻译为 C 全局变量。
- 函数和过程内部变量翻译为局部变量。
- Pascal 函数返回值统一映射到局部变量 `_`，最后 `return _;`。
- `procedure` 翻译为 `void` 函数。
- `var` 参数翻译为指针参数。
- 数组访问在需要时按下界做偏移修正。
- `for i := a to b do` 翻译为 `for (i = a; i <= b; i++)`。
- `write(x, y, z)` 合并为一次 `printf(...)`。
- `read(x)` 翻译为 `scanf(...)`。
- 只有在出现布尔类型或布尔字面量时才引入 `#include <stdbool.h>`。

## 首版测试驱动方案

### 测试目标

首版测试目标不是先执行生成出的程序，而是先保证“生成的 C 文本结构正确”，
即与 `tests/testcases/expected/*.c` 对齐。

### 输入

- Pascal 输入：`tests/testcases/pascal/*.pas`
- 期望输出：`tests/testcases/expected/*.c`
- 运行输入：`tests/testcases/input/*.in`

### 首版测试流程

对每个 `NN_name.pas`：

1. 调用编译器生成 C 代码。
2. 统一换行符为 `\n`。
3. 去掉每行行尾空白。
4. 与对应的 `expected/NN_name.c` 做文本比较。
5. 若失败，输出测试名和差异信息。

### 为什么首版测试脚本仍建议用 Python

即便编译器本体改成 C++，测试驱动仍建议先用 Python 编写：

- 遍历目录、调用编译器、比较文本这类工作 Python 更省时。
- 当前仓库已经是“文档 + fixture”结构，先做出能跑的外部测试脚本更重要。
- 测试脚本不影响“实际实现必须用 C/C++”这一要求，因为编译器本体仍然是 C++。

如果必须把测试驱动也写成 C++，可以后续再补一个原生测试二进制。但首版不建议把时间耗在这里。

## 首版测试脚本约定

建议保留如下接口：

```text
cmake -S . -B build
cmake --build build
python scripts/run_golden.py
```

其中 `scripts/run_golden.py` 的职责：

- 自动调用 `build/.../pascal_s2c`
- 读取 `tests/testcases/pascal/*.pas`
- 收集标准输出中的 C 代码
- 与 `tests/testcases/expected/*.c` 对比

建议的伪代码如下：

```python
for pas_path in sorted(pascal_dir.glob("*.pas")):
    expected_path = expected_dir / pas_path.with_suffix(".c").name
    generated = run_compiler(binary_path, pas_path)
    if normalize(generated) != normalize(expected_path.read_text()):
        report_failure(pas_path.name, generated, expected_path.read_text())
```

### 文本归一化策略

归一化应尽量轻量，避免把输出格式问题掩盖掉：

- 统一 CRLF/LF
- 去掉每行尾部空格和制表符
- 保留空行
- 保留缩进差异

这样既能避免平台换行带来的误报，也能迫使代码生成器尽快贴近黄金格式。

## 第二阶段再增加的测试

当黄金文本比对稳定后，再增加运行级测试：

1. 编译生成的 C 代码。
2. 对存在 `.in` 输入文件的用例，将输入重定向到生成程序。
3. 捕获程序输出。
4. 与期望输出进行比对。

这个阶段需要额外确定：

- 使用 `gcc` 还是 `clang`
- 如何在 Windows 环境下统一执行编译
- 运行输出的期望值是来自“执行 expected C”还是维护单独的 stdout 基准

在首版里，这些都不是优先级最高的事情。

## 分阶段交付计划

### 阶段一：工程骨架

- 新建 `CMakeLists.txt`
- 新建 `src/` 目录结构
- 实现最小 `main.cpp`
- 实现编译驱动外壳
- 加入 `scripts/run_golden.py` 骨架

阶段完成标准：

- 工程可成功编译
- 命令行程序可读取输入文件并输出占位结果
- 测试脚本可遍历全部 Pascal 用例

### 阶段二：词法与语法

- 实现 lexer
- 实现 parser
- 能成功解析全部测试用例

阶段完成标准：

- 所有 `70` 个 Pascal 文件均可完成词法分析
- 所有 `70` 个 Pascal 文件均可构建 AST

### 阶段三：语义分析

- 实现符号表与作用域
- 实现常量类型推断
- 实现表达式类型检查
- 实现函数和过程调用检查
- 实现数组访问检查

阶段完成标准：

- AST 上所有标识符引用都能完成绑定
- `write(...)` 已可确定格式串
- `var` 参数已经能区分值传递和地址传递

### 阶段四：代码生成

- 先覆盖简单声明、表达式、赋值和控制流
- 先通过 `00` 到 `33`
- 再处理短路、副作用、复杂调用，覆盖 `34` 到 `53`
- 最后处理长数组、浮点、多参数、深层循环，覆盖 `54` 到 `69`

阶段完成标准：

- 黄金测试全部通过

## MVP 完成标准

以下条件全部满足时，视为首版可交付：

- 所有 `70` 个 Pascal 用例均能被成功解析
- 所有 `70` 个 Pascal 用例均能生成 C 代码
- 首版黄金测试全部通过
- 报错信息足够快速定位失败用例和失败阶段

## 当前不做的事情

- 不做完整 Pascal 兼容
- 不做优化器
- 不做复杂中间表示设计
- 不做寄存器分配
- 不做目标机器代码生成
- 不优先实现测试集中未覆盖的教材语法

## 下一步建议

这份方案落地后的直接下一步应当是：

1. 建立 `CMakeLists.txt` 和 `src/` 工程骨架。
2. 先实现 token 定义与 lexer。
3. 先让全部用例“可词法分析”，再进入 parser。
4. 同时补上 `scripts/run_golden.py`，保证每增加一个模块都能立即回归。

不要一开始就写完整编译器。先把“能跑通全部文件的词法与语法骨架”搭起来，
再逐步把语义和代码生成补齐，这样风险最低。
