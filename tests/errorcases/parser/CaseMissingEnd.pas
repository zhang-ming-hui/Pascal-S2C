program CaseMissingEnd(output);
var
   n : integer;
begin
   n := 1;
   case n of
     1 : writeln('one');
     2 : writeln('two')
   { 缺少 end 结束 case }
end.