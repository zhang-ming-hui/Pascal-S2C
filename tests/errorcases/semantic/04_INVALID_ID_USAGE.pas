program InvalidIDUsageTest;

var
    x: integer;
    arr: array[1..10] of integer;

begin
    x[5] := 10;             { 非数组类型当作数组 }
    arr := 5;               { 数组当作普通变量 }
end.