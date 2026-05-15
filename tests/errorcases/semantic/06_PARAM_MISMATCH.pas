program ParamMismatchTest;

procedure foo(a: integer; b: real);
begin
end;

function add(x: integer; y: integer): integer;
begin
    add := x + y;
end;

begin
    foo(5);                  { 参数数量不匹配 }
    foo(5, 10, 15);         { 参数数量不匹配 }
    foo(3.14, true);        { 参数类型不匹配 }
    add(5, 3.14);           { 函数参数类型不匹配 }
end.