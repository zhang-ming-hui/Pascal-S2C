# 语法规则 (YACC格式)

[TOC]

## 程序结构

```
programstruct : program_head ":" program_body "."

program_head : program ID "(" idlist ")"
             | program ID

program_body : const_declarations
             | var_declarations
             | subprogram_declarations
             | compound_statement

idlist : ID
       | idlist "," ID
```

```pascal
program sort(input, output)
program gcd(arg[2])
program example
```



## 常量声明

```
const_declarations : %empty
                   | "const" const_declaration ";"

const_declaration : ID "=" const_value
                  | const_declaration ";" ID "=" const_value

const_value : "+" NUM
            | "-" NUM
            | NUM
            | "'" LETTER "'"
```



```pascal
const a = 1; b = -2;
  ch = 'c';
```



## 变量声明

```
var_declarations : %empty
                 | "var" var_declaration ";"

var_declaration : idlist ":" type
                | var_declaration ";" idlist ":" type

type : basic_type
     | "array" "[" period "]" "of" basic_type

basic_type : "integer"
           | "real"
           | "boolean"
           | "char"

period : DIGITS ".." DIGITS
       | period "," DIGITS ".." DIGITS
```



```pascal
var a, b: integer; x, y: real;
  ch: char;
  mx: array [3..9] of integer;
```



## 子程序声明

```
subprogram_declarations : %empty
                        | subprogram_declarations subprogram ";"

subprogram : subprogram_head ";" subprogram_body

subprogram_head : "procedure" ID formal_parameter
                | "function" ID formal_parameter ":" basic_type

formal_parameter : %empty
                 | "(" parameter_list ")"

parameter_list : parameter
               | parameter_list ";" parameter

parameter : var_parameter
          | value_parameter

var_parameter : "var" value_parameter

value_parameter : idlist ":" basic_type

subprogram_body : const_declarations
                | var_declarations
                | compound_statement
```



```pascal
function f(a, b: integer): integer;
procedure p(x, y: real; var b, c: integer);
```



## 语句

```
compound_statement : "begin" statement_list "end"
statement_list : statement
               | statement_list ";" statement

statement : %empty
          | variable ASSIGNOP expression
          | FUNC_ID ASSIGNOP expression
          | procedure_call
          | compound_statement
          | "if" expression "then" statement else_part
          | "for" ID ASSIGNOP expression "to" expression "do" statement
          | "read" "(" variable_list ")"
          | "write" "(" expression_list ")"

variable_list : variable
              | variable_list "," variable

variable : ID id_varpart

id_varpart : %empty
           | "[" expression_list "]"

procedure_call : ID
               | ID "(" expression_list ")"

else_part : %empty
          | "else" statement

expression_list : expression
                | expression_list "," expression

expression : simple_expression
           | simple_expression RELOP simple_expression

simple_expression : term
                  | simple_expression ADDOP term

term : factor
     | term MULOP factor

factor : NUM
       | variable
       | "(" expression ")"
       | ID "(" expression_list ")"
       | "not" factor
       | UMINUS factor
```



```pascal
function gcd(a, b: integer): integer;
begin
  if b = 0 then gcd := a
  else gcd := gcd(b, a mod b)
end;
```