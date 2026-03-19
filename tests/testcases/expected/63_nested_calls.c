#include <stdio.h>

int i, i1, i2, i3, i4, a;
int arr[10];

int func1(int x, int y, int z)
{
  int _;
  if (z == 0)
    _ = x * y;
  else
    _ = func1(x, y - z, 0);
  return _;
}

int func2(int x, int y)
{
  int _;
  if (y != 0)
    _ = func2(x % y, 0);
  else
    _ = x;
  return _;
}

int func3(int x, int y)
{
  int _;
  if (y == 0)
    _ = x + 1;
  else
    _ = func3(x + y, 0);
  return _;
}

int func4(int x, int y, int z)
{
  int _;
  if (x != 0)
    _ = y;
  else
    _ = z;
  return _;
}

int func5(int x)
{
  int _;
  _ = -x;
  return _;
}

int func6(int x, int y)
{
  int _;
  if ((x != 0) && (y != 0))
    _ = 1;
  else
    _ = 0;
  return _;
}

int func7(int x)
{
  int _;
  if (x == 0)
    _ = 1;
  else
    _ = 0;
  return _;
}

int main()
{
  i1 = 1;
  i2 = 2;
  i3 = 3;
  i4 = 4;
  for (i = 0; i <= 9; i++)
    arr[i] = i + 1;
  a = func1(
      func2(
          func1(
              func3(func4(func5(func3(func2(func6(func7(i1), func5(i2)), i3),
                                      i4)),
                          arr[0],
                          func1(func2(func3(func4(func5(arr[1]),
                                                  func6(arr[2], func7(arr[3])),
                                                  func2(arr[4], func7(arr[5]))),
                                            arr[6]),
                                      arr[7]),
                                func3(arr[8], func7(arr[9])), i1)),
                    func2(i2, func3(func7(i3), i4))),
              arr[0], arr[1]),
          arr[2]),
      arr[3],
      func3(func2(func1(func2(func3(arr[4], func5(arr[5])), func5(arr[6])),
                        arr[7], func7(arr[8])),
                  func5(arr[9])),
            i1));
  printf("%d", a);
  return 0;
}