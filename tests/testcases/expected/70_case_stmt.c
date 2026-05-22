#include <stdio.h>

int n;
char ch;

int main()
{
  n = 2;
  ch = 'b';
  {
    int _case_selector0 = n;
    if (_case_selector0 == 1)
      printf("%d", n);
    else if (_case_selector0 == 2 || _case_selector0 == 3)
      printf("%d", n);
  }
  {
    char _case_selector1 = ch;
    if (_case_selector1 == 'a')
      printf("%c", ch);
    else if (_case_selector1 == 'b')
      printf("%c", ch);
  }
  return 0;
}
