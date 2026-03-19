#include <stdio.h>

int ififElse()
{
  int a;
  int b;
  int _;
  a = 5;
  b = 10;
  if (a == 5)
    if (b == 10)
      a = 25;
    else
      a = a + 15;
  _ = a;
  return _;
}

int main()
{
  printf("%d", ififElse());
  return 0;
}