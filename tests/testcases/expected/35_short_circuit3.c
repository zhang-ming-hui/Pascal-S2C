#include <stdio.h>

const char AA = 'A';
const char BB = 'B';
const char CC = 'C';
const char DD = 'D';
const char E = 'E';
const char F = 'F';
const char G = 'G';
const char H = 'H';
const char I = 'I';
const char J = 'J';
const char K = 'K';
const int c = 1;
int a, b, d;
int i0, i1, i2, i3, i4;

int set_a(int val)
{
  int _;
  a = val;
  _ = val;
  return _;
}

int set_b(int val)
{
  int _;
  b = val;
  _ = val;
  return _;
}

int set_d(int val)
{
  int _;
  d = val;
  _ = val;
  return _;
}

int main()
{
  a = 2;
  b = 3;
  if ((set_a(0) != 0) && (set_b(1) != 0))
    ;
  printf("%d", a);
  printf("%d", b);

  a = 2;
  b = 3;
  if ((set_a(0) != 0) && (set_b(1) != 0))
    ;
  printf("%d", a);
  printf("%d", b);

  d = 2;
  if ((c >= 1) && (set_d(3) != 0))
    ;
  printf("%d", d);
  if ((c <= 1) || (set_d(4) != 0))
    ;
  printf("%d", d);

  if (16 >= (3 - (2 + 1)))
    printf("%c", AA);
  if ((25 - 7) != (36 - 6 * 3))
    printf("%c", BB);
  if (1 != (7 % 2))
    printf("%c", CC);
  if (3 <= 4)
    printf("%c", DD);
  if (0 != 0)
    printf("%c", E);
  if (1 != 0)
    printf("%c", F);

  i0 = 0;
  i1 = 1;
  i2 = 2;
  i3 = 3;
  i4 = 4;
  if ((i0 != 0) || (i1 != 0))
    printf("%c", G);
  if ((i0 >= i1) || (i1 <= i0))
    printf("%c", H);
  if ((i2 >= i1) && (i4 != i3))
    printf("%c", I);
  if ((i0 == 0) && (i3 < i3) || (i4 >= i4))
    printf("%c", J);
  if ((i0 == 0) || (i3 < i3) && (i4 >= i4))
    printf("%c", K);
  return 0;
}