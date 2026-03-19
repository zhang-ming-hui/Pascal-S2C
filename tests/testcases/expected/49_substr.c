#include <stdio.h>

int A[15];
int B[13];

int MAX(int a, int b)
{
  int _;
  if (a >= b)
    _ = a;
  else
    _ = b;
  return _;
}

int max_sum_nonadjacent(int n)
{
  int _;
  int i;
  int temp[16];
  temp[0] = A[0];
  temp[1] = MAX(A[0], A[1]);
  for (i = 2; i <= n - 1; i++)
    temp[i] = MAX(temp[i - 2] + A[i], temp[i - 1]);
  _ = temp[n - 1];
  return _;
}

int longest_common_subseq(int len1, int len2)
{
  int _;
  int i, j;
  int p[16][16];
  for (i = 0; i <= 15; i++)
    p[i][0] = 0;
  for (j = 0; j <= 15; j++)
    p[0][j] = 0;
  for (i = 1; i <= len1; i++)
  {
    for (j = 1; j <= len2; j++)
    {
      if (A[i - 1] == B[j - 1])
        p[i][j] = p[i - 1][j - 1] + 1;
      else
        p[i][j] = MAX(p[i - 1][j], p[i][j - 1]);
    }
  }
  _ = p[len1][len2];
  return _;
}

int main()
{
  A[0] = 8;
  A[1] = 7;
  A[2] = 4;
  A[3] = 1;
  A[4] = 2;
  A[5] = 7;
  A[6] = 0;
  A[7] = 1;
  A[8] = 9;
  A[9] = 3;
  A[10] = 4;
  A[11] = 8;
  A[12] = 3;
  A[13] = 7;
  A[14] = 0;
  B[0] = 3;
  B[1] = 9;
  B[2] = 7;
  B[3] = 1;
  B[4] = 4;
  B[5] = 2;
  B[6] = 4;
  B[7] = 3;
  B[8] = 6;
  B[9] = 8;
  B[10] = 0;
  B[11] = 1;
  B[12] = 5;

  printf("%d", max_sum_nonadjacent(15));
  printf("%d", longest_common_subseq(15, 13));
  return 0;
}