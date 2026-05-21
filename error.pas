program ErrorDemo;
var
  score,sum:integer;

{ 过程：日常手写词法错误（合法字符写错、命名失误） }
procedure LexErrPro;
var
  stu_num_2#:integer;    { 错误：标识符混入#，合法命名写错 }
begin
  tot=99;
end;

{ 函数：程序员高频语法错误，token全部合法 }
function SynErrFun:integer;
var
  i:integer;
begin
  i:=1;
  while i<10 do          { 错误：while循环漏写循环体begin }
  i:=i+2;
  SynErrFun:=i;
  a := 0;
end;

{ 主程序：无词法语法错，纯业务语义错误 }
begin
  sum:=score+age;        { 语义错：age未定义，漏声明变量 }
  score:=15.6+20;        { 语义错：整数赋值浮点数值，类型不匹配 }
end.

