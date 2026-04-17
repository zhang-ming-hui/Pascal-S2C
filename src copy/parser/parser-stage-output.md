# Parser 阶段产物说明

本文说明语法分析（Parser）运行结束后，程序在编译流程中的数据形态。

## 1. 阶段接口

- 输入类型：`TokenList`
- 输出类型：`ProgramPtr`
- 接口：`ProgramPtr Parser::parse(const TokenList& tokens) const`

`ProgramPtr` 定义为：

```cpp
using ProgramPtr = std::unique_ptr<ProgramNode>;
```

核心结构关系：

```text
ProgramNode
  └── BlockNode
      ├── constDecls      (ConstDeclNode[])
      ├── varDecls        (VarDeclNode[])
      ├── subprograms     (SubprogramDeclNode[])
      └── body            (CompoundStmtNode)
```

## 2. 示例输入（覆盖 BlockNode 四个成员）

```pascal
program demo;
const
  ci = 10;
var
  a: integer;

function inc1(x: integer): integer;
begin
  inc1 := x + 1;
end;

begin
  a := ci;
  a := inc1(a);
  write(a);
end.
```

## 3. Parser 结束后的结果形态（简化）

1. `ProgramNode`
- `name = "demo"`
- `block = <BlockNode>`

2. 顶层 `BlockNode`
- `constDecls`
  - `ConstDeclNode(name="ci", value=LiteralExprNode(Int,"10"))`
- `varDecls`
  - `VarDeclNode(names=["a"], type=ScalarTypeNode(Integer))`
- `subprograms`
  - `FunctionDeclNode(name="inc1", params=[x:integer], returnType=Integer, block=...)`
- `body`
  - `CompoundStmtNode(statements=[Assign(a:=ci), Assign(a:=inc1(a)), Write(a)])`

## 4. 阶段产物要点

1. Parser 只保证语法结构正确，不做完整类型校验。
2. AST 中每个节点都保留 `loc` 便于错误定位。
3. `subprograms` 中每个函数/过程都有自己的 `BlockNode`。
4. `body` 一定是 `CompoundStmtNode`，保存可执行语句序列。
