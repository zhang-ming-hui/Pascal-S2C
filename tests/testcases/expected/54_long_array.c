#include <stdio.h>

int long_array(int k)
{
  int _;
  int a1[150];
  int a2[150];
  int a3[150];
  int i, j, ans;
  for (i = 0; i <= 149; i++)
    a1[i] = (i * i) % 10;

  for (i = 0; i <= 149; i++)
    a2[i] = (a1[i] * a1[i]) % 10;

  for (i = 0; i <= 149; i++)
    a3[i] = (a2[i] * a2[i]) % 100 + a1[i];

  ans = 0;

  for (i = 0; i <= 149; i++)
  {
    if (i < 10)
    {
      ans = (ans + a3[i]) % 1333;
      printf("%d", ans);
    }

    if (i < 20)
    {
      for (j = (150 / 2); j <= 149; j++)
        ans = ans + a3[i] - a1[j];
      printf("%d", ans);
    }

    if (i < 30)
    {
      for (j = 150 / 2; j <= 149; j++)
      {
        if (j > 2233)
        {
          ans = ans + a2[i] - a1[j];
        }
        else
        {
          ans = (ans + a1[i] + a3[j]) % 13333;
        }
      }
      printf("%d", ans);
    }
    else
    {
      ans = (ans + a3[i] * k) % 99988;
    }
  }

  _ = ans;
  return _;
}

int main()
{
  printf("%d", long_array(9));
  return 0;
}