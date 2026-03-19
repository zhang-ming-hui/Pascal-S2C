# 语法规则 (YACC格式)

[TOC]

## 程序结构

```
program : program_head program_body

program_head : PROGRAM ID "(" identifier_list ")"
             | PROGRAM ID

program_body : const_declarations type_declarations var_declarations subprogram_declarations compound_statement

identifier_list | identifier_list "," ID
                : ID
```

```pascal
program sort(input, output)
program gcd(arg[2])
program example
```



## 常量声明

```
const_declarations : CONST const_declaration ";"
                   | %empty

const_declaration : const_declaration ";" ID "=" const_variable
                  | ID "=" const_variable

const_variable : "+" ID
               | "-" ID
               | ID
               | "+" NUM
               | "-" NUM
               | NUM
               | "'" LETTER "'"
```

```pascal
const a = 1; b = -2;
  ch = 'c';
```



## 类型声明

```
type_declarations : TYPE type_declaration ";"
                  | %empty

type_declaration : type_declaration ";" ID "=" type
                 | ID "=" type

type : standard_type
     | RECORD record_body END
     | ARRAY "[" periods "]" OF type

standard_type : INTEGER
              | REAL
              | BOOLEAN
              | CHAR

record_body : var_declaration
            | %empty

periods : periods "," period
        | period

period : const_variable ".." const_variable
```



## 变量声明

```
var_declarations : VAR var_declaration ";"
                 | %empty

var_declaration : var_declaration ";" identifier_list ":" type
                | identifier_list ":" type
```



```pascal
var a, b: integer; x, y: real;
  ch: char;
  mx: array [3..9] of integer;
```



## 子程序声明

```
subprogram_declarations : subprogram_declarations subprogram_declaration ";"
                        | %empty

subprogram_declaration : subprogram_head program_body

subprogram_head : FUNCTION ID formal_parameter ":" standard_type ";"
                | PROCEDURE ID formal_parameter ";"

formal_parameter : "(" parameter_list ")"
                 | %empty

parameter_lists : parameter_lists ";" parameter
                | parameter_list

parameter_list : var_parameter
               | value_parameter

var_parameter : VAR value_parameter

value_parameter : identifier_list ":" standard_type
```



```pascal
function f(a, b: integer): integer;
procedure p(x, y: real; var b, c: integer);
```



## 语句

```
compound_statement : BEGIN statement_list END

statement_list : statement_list ";" statement
               | statement

statement : variable ASSIGNOP expression
          | call_procedure_statement
          | compound_statement
          | IF expression THEN statement else_part
          | CASE expression OF case_body END
          | WHILE expression DO statement
          | REPEAT statement_list UNTIL expression
          | FOR ID ASSIGNOP expression updown expression DO statement
          | %empty

variable : ID id_varparts

id_varparts : id_varparts id_varpart
            | %empty

id_varpart : "[" expression_list "]"
           | "." id

else_part : ELSE statement
          | %empty

case_body : branch_list
          | %empty

branch_list : branch_list ";" branch
            | branch

branch : const_list ":" statement

const_list : const_list "," const_variable
           | const_variable

updown : TO
       | DOWNTO

call_procedure_statement : ID
                         | ID "(" expression_list ")"

expression_list : expression_list "," expression
                | expression

expression : simple_expression RELOP simple_expression
           | simple_expression

simple_expression : term
                  | "+" term
                  | "-" term
                  | simple_expression ADDOP term

term : term MULOP factor
     | factor

factor : unsign_const_variable
       | variable
       | ID "(" expression_list ")"
       | "(" expression ")"
       | "not" factor

unsign_const_variable : ID
                      | NUM
                      | "'" LETTER "'"
```



```pascal
function gcd(a, b: integer): integer;
begin
  if b = 0 then gcd := a
  else gcd := gcd(b, a mod b)
end;
```