# Parser 结束后 BlockNode 的形态说明

本文说明 Parser 完成后，`BlockNode` 在 AST 中应当具备的结构与信息，并给出一个覆盖全部字段类型的完整示例。

目标结构如下：

```cpp
struct BlockNode : Node {
    std::vector<std::unique_ptr<ConstDeclNode>> constDecls;
    std::vector<std::unique_ptr<VarDeclNode>> varDecls;
    std::vector<std::unique_ptr<SubprogramDeclNode>> subprograms;
    std::unique_ptr<CompoundStmtNode> body;
};
```

上面四个成员对应 Parser 的四个阶段：

1. `constDecls` 由 `parseConstDeclarations` 填充。
2. `varDecls` 由 `parseVarDeclarations` 填充。
3. `subprograms` 由 `parseSubprogram` 循环填充。
4. `body` 由 `parseCompoundStatement` 填充。

## 1. 覆盖全部类型的示例输入

下面这个 Pascal 示例能被当前项目 parser 支持，并且会覆盖 `BlockNode` 的四个成员。

```pascal
program demo;
const
  ci = 10;
  ch = 'x';
var
  a, b: integer;
  r: real;
  flag: boolean;
  c: char;
  arr: array[1..3, -1..1] of integer;

function inc1(x: integer): integer;
begin
  inc1 := x + 1;
end;

procedure touch(var p: integer; q: integer);
begin
  p := p + q;
end;

begin
  a := ci;
  b := inc1(a);
  touch(b, 2);
  arr[1, 0] := b;
  if b > 0 then
    write(b)
  else
    write(ci);
end.
```

## 2. Parser 之后的顶层 Program 形态

Parser 成功后，顶层大致是：

- `ProgramNode.name = "demo"`
- `ProgramNode.block = <BlockNode>`

本文重点展开 `ProgramNode.block`。

## 3. BlockNode 四个成员的详细展开

### 3.1 constDecls: vector of ConstDeclNode

本例中会得到两个常量声明节点。

1. `ConstDeclNode`
- `name = "ci"`
- `value = LiteralExprNode(kind=Int, rawText="10")`

2. `ConstDeclNode`
- `name = "ch"`
- `value = LiteralExprNode(kind=Char, rawText="'x'")`

编译过程里它们携带的信息：

1. 常量名。
2. 常量初始化表达式语法树。
3. 源码位置 `loc`（用于报错定位）。

语义分析阶段会基于 `value` 推断类型并写入符号表，代码生成阶段会输出 `const` 声明。

### 3.2 varDecls: vector of VarDeclNode

本例会得到多条变量声明，每条 `VarDeclNode` 有两部分：`names + type`。

1. `a, b: integer`
- `names = ["a", "b"]`
- `type = ScalarTypeNode(kind=Integer)`

2. `r: real`
- `names = ["r"]`
- `type = ScalarTypeNode(kind=Real)`

3. `flag: boolean`
- `names = ["flag"]`
- `type = ScalarTypeNode(kind=Boolean)`

4. `c: char`
- `names = ["c"]`
- `type = ScalarTypeNode(kind=Char)`

5. `arr: array[1..3, -1..1] of integer`
- `names = ["arr"]`
- `type = ArrayTypeNode`
- `type.dims = [{lower=1, upper=3}, {lower=-1, upper=1}]`
- `type.elementType.kind = Integer`

编译过程里它们携带的信息：

1. 一组变量名。
2. 变量类型语法树（标量或数组）。
3. 数组维度上下界（用于代码生成时长度和下标修正）。

### 3.3 subprograms: vector of SubprogramDeclNode

本例有一个 `function` 和一个 `procedure`，都会进入 `subprograms`。

1. `FunctionDeclNode(name="inc1")`
- `params`: 一个参数组
- 参数组内容：`names=["x"], type=Integer, passMode=Value`
- `returnType = Integer`
- `block`: 函数自己的 `BlockNode`
  - `constDecls = []`
  - `varDecls = []`
  - `subprograms = []`
  - `body` 里有赋值语句 `inc1 := x + 1`

2. `ProcedureDeclNode(name="touch")`
- `params`: 两个参数组
- 第一个参数组：`names=["p"], type=Integer, passMode=Var`
- 第二个参数组：`names=["q"], type=Integer, passMode=Value`
- `block`: 过程自己的 `BlockNode`
  - `constDecls = []`
  - `varDecls = []`
  - `subprograms = []`
  - `body` 里有赋值语句 `p := p + q`

编译过程里它们携带的信息：

1. 子程序名称与参数签名。
2. 函数返回类型（仅 function）。
3. 子程序局部 block（后续用于建立子作用域和生成独立 C 函数）。
4. `var` 参数标记（后续转 C 指针与按地址传参）。

### 3.4 body: CompoundStmtNode

`body` 是 block 的执行主体，对应最外层 `begin ... end`。

本例中 `body.statements` 依次包含：

1. `AssignStmtNode`: `a := ci`
2. `AssignStmtNode`: `b := inc1(a)`
3. `CallStmtNode`: `touch(b, 2)`
4. `AssignStmtNode`: `arr[1, 0] := b`
5. `IfStmtNode`:
- `condition`: `b > 0`
- `thenBranch`: `write(b)`
- `elseBranch`: `write(ci)`

编译过程里它携带的信息：

1. 可执行语句序列。
2. 控制流结构（if/while/for）。
3. 读写、调用、赋值中涉及的表达式树。

## 4. 一个接近真实对象图的简化视图

```text
ProgramNode("demo")
└── BlockNode
    ├── constDecls
    │   ├── ConstDeclNode("ci", LiteralExprNode(Int,"10"))
    │   └── ConstDeclNode("ch", LiteralExprNode(Char,"'x'"))
    ├── varDecls
    │   ├── VarDeclNode(["a","b"], ScalarType(Integer))
    │   ├── VarDeclNode(["r"], ScalarType(Real))
    │   ├── VarDeclNode(["flag"], ScalarType(Boolean))
    │   ├── VarDeclNode(["c"], ScalarType(Char))
    │   └── VarDeclNode(["arr"], ArrayType(dims=[1..3,-1..1], element=Integer))
    ├── subprograms
    │   ├── FunctionDeclNode("inc1", params=[x:integer], return=integer, block=...)
    │   └── ProcedureDeclNode("touch", params=[var p:integer, q:integer], block=...)
    └── body
        └── CompoundStmtNode
            ├── Assign(a := ci)
            ├── Assign(b := inc1(a))
            ├── Call(touch(b,2))
            ├── Assign(arr[1,0] := b)
            └── If(b > 0, then=write(b), else=write(ci))
```

## 5. 这个 BlockNode 在后续阶段如何被消费

1. 语义分析阶段
- 遍历 `constDecls` 建常量符号并推断类型。
- 遍历 `varDecls` 建变量符号并解析数组维度。
- 遍历 `subprograms` 先登记签名，再进入子 block 分析。
- 遍历 `body` 进行表达式类型检查与调用参数检查。

2. 代码生成阶段
- `constDecls` 生成 C 的 `const` 定义。
- `varDecls` 生成 C 变量/数组定义。
- `subprograms` 生成 C 函数定义。
- `body` 生成 `main` 或子函数中的语句代码。

## 6. 结论

Parser 之后，一个完整且可用于后续编译阶段的 `BlockNode` 至少应满足：

1. `constDecls` 正确承载所有常量定义和其表达式。
2. `varDecls` 正确承载变量列表与类型（含数组维度）。
3. `subprograms` 正确承载函数/过程签名与各自子 block。
4. `body` 是非空 `CompoundStmtNode`，保存可执行语句序列。

这四部分共同构成后续语义分析与代码生成的核心输入。
