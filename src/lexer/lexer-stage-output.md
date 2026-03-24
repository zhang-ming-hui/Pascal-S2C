# Lexer 阶段产物说明

本文说明词法分析（Lexer）运行结束后，程序在编译流程中的数据形态。

## 1. 阶段接口

- 输入类型：`std::string source`
- 输出类型：`TokenList`
- 接口：`TokenList Lexer::tokenize(const std::string& source) const`

`TokenList` 定义为：

```cpp
using TokenList = std::vector<Token>;
```

`Token` 结构为：

```cpp
struct Token {
    TokenKind kind;
    std::string lexeme;
    SourceLocation location;
};
```

## 2. 示例输入

```pascal
program main;
var
  a: integer;
begin
  a := 3;
  write(a);
end.
```

## 3. Lexer 结束后的结果形态

下表是简化后的 `TokenList`（按顺序）：

1. `{kind=Program, lexeme="program", location=(1,1)}`
2. `{kind=Identifier, lexeme="main", location=(1,9)}`
3. `{kind=Semicolon, lexeme=";", ...}`
4. `{kind=Var, lexeme="var", ...}`
5. `{kind=Identifier, lexeme="a", ...}`
6. `{kind=Colon, lexeme=":", ...}`
7. `{kind=Integer, lexeme="integer", ...}`
8. `{kind=Semicolon, lexeme=";", ...}`
9. `{kind=Begin, lexeme="begin", ...}`
10. `{kind=Identifier, lexeme="a", ...}`
11. `{kind=Assign, lexeme=":=", ...}`
12. `{kind=IntegerLiteral, lexeme="3", ...}`
13. `{kind=Semicolon, lexeme=";", ...}`
14. `{kind=Write, lexeme="write", ...}`
15. `{kind=LParen, lexeme="(", ...}`
16. `{kind=Identifier, lexeme="a", ...}`
17. `{kind=RParen, lexeme=")", ...}`
18. `{kind=Semicolon, lexeme=";", ...}`
19. `{kind=End, lexeme="end", ...}`
20. `{kind=Dot, lexeme=".", ...}`
21. `{kind=EndOfFile, lexeme="", ...}`

## 4. 阶段产物要点

1. Lexer 不做语义判断，只做切分和分类。
2. 标识符会被标准化为小写后存入 `lexeme`。
3. 关键字、运算符、分隔符都以 `TokenKind` 明确区分。
4. 最后必有 `EndOfFile` 作为 Parser 的停止条件。
