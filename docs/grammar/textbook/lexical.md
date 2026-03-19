# 词法规则

[TOC]

## 关键字

Pascal-S语言设计的关键字有

```pascal
and, array, begin, boolean, case, const, div, do, downto, else, end,, for, function, if, integer, mod, not, of, or, procedure, program, real, record, repeat, then, to, type, until, var, while
```

程序中的关键字（除开头的`program`和末尾的`end`之外）的前后必须有空格符或换行符，其他单词符号间的空格符是可选的

关键字作为保留字

## 专用符号

Pascal-S语言中用到的符号有

| 类型       | 值                                          |
| ---------- | ------------------------------------------- |
| 算数算符   | `+`,`-`,`*`,`/`,`mod`,`div`                 |
| 逻辑算符   | `<`,`<=`,`>`,`>=`,`=`,`<>`,`and`,`or`,`not` |
| 赋值号     | `:=`                                        |
| 子界符     | `..`                                        |
| 分界符     | `,`,`;`,`.`,`(`,`)`,`[`,`]`,`{`,`}`         |
| 注释起止符 | `(*`,`*)`,`{`,`}`                           |

关系运算符`RELOP`代表`=`,`<>`,`<`,`<=`,`>`,`>=`

`ADDOP`代表运算符`+`,`-`和`or`

`MULOP`代表运算符`*`,`/`,`div`,`mod`和`and`

`ASSIGNOP`代表赋值号`:=`

## 注释

程序中的注释可以出现在任何单词符号之后，用分解符`{`和`}`或`(*`和`*)`括起来

```pascal
{ 注释 }
(* 注释 *)
```

## 操作符

```
ASSIGNOP :=
RELOP =  <>  <  >  <=  >=
ADDOP +
MULOP *
UMINUS -
```

## 标识符

标识符记号`id`匹配以字母开头的字母数字串，其最大长度规定为8个字符

```
LETTER : [a-zA-Z]
DIGIT : [0-9]
ID : LETTER (LETTER | DIGIT)*
```

## 数字
```pascal
DIGITS : DIGIT DIGIT*
OPTIONAL_FRACTION : %empty
                  | "." DIGIT
OPTIONAL_EXPONENT : %empty
                  | ( E ( "+" | "-" | %empty ) DIGITS
NUM = DIGITS OPTIONAL_FRACTION OPTIONAL_EXPONENT
```

