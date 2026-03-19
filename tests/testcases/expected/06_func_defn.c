#include <stdio.h>

int a;
int b;

int func(int p)
{
  int _;
  p = p - 1;
  _ = p;
  return _;
}

int main()
{
  a = 10;
  b = func(a);
  printf("%d", b);
  return 0;
}