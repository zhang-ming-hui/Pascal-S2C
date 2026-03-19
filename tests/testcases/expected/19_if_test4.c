#include <stdio.h>

int if_if_Else()
{
  int _;
  int a, b;
  a = 5;
  b = 10;
  if (a == 5)
  {
    if (b == 10)
      a = 25;
  }
  else
    a = a + 15;
  _ = a;
  return _;
}

int main()
{
  printf("%d", if_if_Else());
  return 0;
}