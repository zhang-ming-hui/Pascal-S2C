#include <stdio.h>

int n;
char ch;

int main()
{
  n = 2;
  ch = 'b';
  if (n == 1)
    printf("%d", n);
  else if (n == 2 || n == 3)
    printf("%d", n);
  if (ch == 'a')
    printf("%c", ch);
  else if (ch == 'b')
    printf("%c", ch);
  return 0;
}
