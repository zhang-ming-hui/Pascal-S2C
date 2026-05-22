program case_stmt;
var
  n: integer;
  ch: char;
begin
  n := 2;
  ch := 'b';
  case n of
    1: write(n);
    2, 3: write(n)
  end;
  case ch of
    'a': write(ch);
    'b': write(ch)
  end
end.
