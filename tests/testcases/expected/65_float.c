#include <stdio.h>
#include <stdbool.h>

const float RADIUS = 5.5;
const float PI = 03.141595653589793;
const float EPS = 0.000001;
const float EVAL1 = 95.033188;
const int CONV1 = 233;
const int MAX = 1000000000;
const float TWO = 2.9;
const int THREE = 3;
const int FIVE = 5;
const char e = 'e';
const char o = 'o';

int p;
float arr[10];
float input, area, area_trunc;

float float_abs(float x)
{
  float _;
  if (x < 0)
    _ = -x;
  else
    _ = x;
  return _;
}

float circle_area(int radius)
{
  float _;
  _ = (PI * radius * radius + (radius * radius) * PI) / 2;
  return _;
}

int float_eq(float a, float b)
{
  int _;
  if (float_abs(a - b) < EPS)
    _ = 1;
  else
    _ = 0;
  return _;
}

void error()
{
  printf("%c", e);
}

void ok()
{
  printf("%c", o);
}

void assert(int cond)
{
  if (cond == 0)
    error();
  else
    ok();
}

int main()
{
  assert(float_eq(circle_area(5), circle_area(FIVE)));
  if (1.5 != 0.0)
    ok();
  if (!(3.3 == 0.0))
    ok();
  if ((0.0 != 0.0) && (3 != 0.0))
    error();
  if ((0 != 0.0) || (0.3 != 0.0))
    ok();

  p = 0;
  arr[0] = 1.0;
  arr[1] = 2.0;
  input = 0.520;
  area = PI * input * input;
  area_trunc = circle_area(0);
  arr[p] = arr[p] + input;

  printf("%f", area);
  printf("%f", area_trunc);
  printf("%f", arr[0]);
  return 0;
}