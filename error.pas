program ErrorDemo;
var
  score,sum:integer;

procedure LexErrPro;
var
  $stu_num_2:integer;
begin
end;

function SynErrFun:integer;
var
  i:integer;
begin
  i:=1;
  while i<10 do
    i=i+2;
  SynErrFun:=i;
end;

begin
  sum:=score+age;
  score:=15.6+20;
end.

