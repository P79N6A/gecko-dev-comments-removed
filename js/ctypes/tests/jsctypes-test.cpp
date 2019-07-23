






































#include "jsctypes-test.h"
#include "nsStringAPI.h"
#include "nsCRT.h"
#include <math.h>

void
test_v()
{
  
  return;
}

short
test_s()
{
  return 12345;
}

short
test_s_s(short number)
{
  return number;
}

short
test_s_ss(short number1, short number2)
{
  return number1 + number2;
}

int
test_i()
{
  return 123456789;
}

int
test_i_i(int number)
{
  return number;
}

int
test_i_ii(int number1, int number2)
{
  return number1 + number2;
}

float
test_f()
{
  return 123456.5f;
}

float
test_f_f(float number)
{
  return number;
}

float
test_f_ff(float number1, float number2)
{
  return (number1 + number2);
}

double
test_d()
{
  return 123456789.5;
}

double
test_d_d(double number)
{
  return number;
}

double
test_d_dd(double number1, double number2)
{
  return (number1 + number2);
}

int
test_ansi_len(const char* string)
{
  return strlen(string);
}

int
test_wide_len(const PRUnichar* string)
{
  return nsCRT::strlen(string);
}

const char *
test_ansi_ret()
{
  return "success";
}

const PRUnichar *
test_wide_ret()
{
  static const PRUnichar kSuccess[] = {'s', 'u', 'c', 'c', 'e', 's', 's', '\0'};
  return kSuccess;
}

char *
test_ansi_echo(const char* string)
{
  return (char*)string;
}

int
test_i_if_floor(int number1, float number2)
{
  return floor(float(number1) + number2);
}

int
test_pt_in_rect(RECT rc, POINT pt)
{
  if (pt.x < rc.left || pt.x > rc.right)
    return 0;
  if (pt.y < rc.bottom || pt.y > rc.top)
    return 0;
  return 1;
}

int test_nested_struct(NESTED n) {
  return n.n1 + n.n2 + n.inner.i1 + n.inner.i2 + n.inner.i3 + n.n3 + n.n4;
}

POINT test_struct_return(RECT r) {
  POINT p;
  p.x = r.left; p.y = r.top;
  return p;
}

