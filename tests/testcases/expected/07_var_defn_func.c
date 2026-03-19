#include <stdio.h>

int a;

int defn()
{
  int _;
  _ = 4;
  return _;
}

int main()
{
  a = defn();
  printf("%d", a);
  return 0;
}