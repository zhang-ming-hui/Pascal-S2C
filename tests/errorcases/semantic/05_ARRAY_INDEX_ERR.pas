program ArrayIndexErrTest;

var
    x: integer;

begin
    x[5] := 10;            { 非数组类型当作数组 }
end.