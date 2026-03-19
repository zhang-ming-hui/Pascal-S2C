# 词法规则

[TOC]

## 关键字

```pascal
program, begin, end, var, char, integer, real, boolean
if, then, else, while, do, for, to, downto
read, write
```

## 操作符
```pascal
ASSIGNOP :=
RELOP =  <>  <  >  <=  >=
ADDOP +
MULOP *
UMINUS -
```

## 分隔符
```pascal
;  :  ,  .  (  )
```

## 标识符
```pascal
LETTER = [a-zA-Z]
DIGIT = [0-9]
ID = LETTER (LETTER | DIGIT)*
```

## 数字
```pascal
DIGITS = DIGIT*
NUM = DIGITS | DIGITS '.' DIGITS
```

## 注释
```pascal
{ 注释内容 }
```