{ Comprehensive error test file - contains multiple errors }
program ErrorDemo;
var
  a, b: integer;
  c: real;
begin

  // Error 7: Multiple issues in one line
  x := @ # $ % ^ & * ();
  
  // Error 8: Unterminated brace comment starts here
  { This comment never closes
  write('After unclosed comment');
  
  write('End of program');
end.