#include <stdio.h>

int ret;

int ifWhile()
{
  int _;
  int a, b;
  a = 0;
  b = 1;
  if (a == 5)
  {
    for (b = 1; b <= 3; b++)
    {
    }
    b = b + 25;
    _ = b;
  }
  else
    for (a = 0; a <= 4; a++)
      b = b * 2;
  _ = b;
  return _;
}

int main()
{
  printf("%d", ifWhile());
  return 0;
}