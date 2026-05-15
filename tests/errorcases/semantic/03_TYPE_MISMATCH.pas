program TypeMismatchTest;

var
    i: integer;
    r: real;
    b: boolean;

begin
    i := 3.14;              { 整数赋值实数 }
    r := true;              { 实数赋值布尔 }
    b := 10;                { 布尔赋值整数 }
    i := i + true;          { 运算类型不匹配 }
    r := b and 5;           { 布尔与整数运算 }
end.