program UndefinedIDTest;

var
    x: integer;

begin
    y := 10;          { y 未定义 }
    z := x + 5;       { z 未定义 }
    result := add(x); { add 函数未定义 }
end.