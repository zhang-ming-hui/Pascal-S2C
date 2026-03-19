#include <stdio.h>

int M, L, N;
int a0[3], a1[3], a2[3], b0[3], b1[3], b2[3], c0[3], c1[3], c2[3];
int i, x;

int tran()
{
  int _;
  int i;
  for (i = 0; i <= M - 1; i++)
  {
    c1[2] = a2[1];
    c2[1] = a1[2];
    c0[1] = a1[0];
    c0[2] = a2[0];
    c1[0] = a0[1];
    c2[0] = a0[2];
    c1[1] = a1[1];
    c2[2] = a2[2];
    c0[0] = a0[0];
  }

  _ = 0;
  return _;
}

int main()
{
  N = 3;
  M = 3;
  L = 3;

  for (i = 0; i <= M - 1; i++)
  {
    a0[i] = i;
    a1[i] = i;
    a2[i] = i;
    b0[i] = i;
    b1[i] = i;
    b2[i] = i;
  }

  tran();

  for (i = 0; i <= N - 1; i++)
  {
    printf("%d", c0[i]);
  }

  for (i = 0; i <= N - 1; i++)
  {
    printf("%d", c1[i]);
  }

  for (i = 0; i <= N - 1; i++)
  {
    printf("%d", c2[i]);
  }
  return 0;
}