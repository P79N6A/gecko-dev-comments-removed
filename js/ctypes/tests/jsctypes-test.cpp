







































#include "jsctypes-test.h"
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
  return int(strlen(string));
}

int
test_wide_len(const PRUnichar* string)
{
  return int(nsCRT::strlen(string));
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
  return int(floor(float(number1) + number2));
}

