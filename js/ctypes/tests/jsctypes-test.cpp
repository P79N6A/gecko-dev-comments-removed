







































#include "jsctypes-test.h"
#include "nsCRTGlue.h"
#include <string.h>
#include <math.h>

void
test_v()
{
  
  return;
}

PRInt8
test_i8()
{
  return 123;
}

PRInt8
test_i8_i8(PRInt8 number)
{
  return number;
}

PRInt8
test_i8_i8_sum(PRInt8 number1, PRInt8 number2)
{
  return number1 + number2;
}

PRInt16
test_i16()
{
  return 12345;
}

PRInt16
test_i16_i16(PRInt16 number)
{
  return number;
}

PRInt16
test_i16_i16_sum(PRInt16 number1, PRInt16 number2)
{
  return number1 + number2;
}

PRInt32
test_i32()
{
  return 123456789;
}

PRInt32
test_i32_i32(PRInt32 number)
{
  return number;
}

PRInt32
test_i32_i32_sum(PRInt32 number1, PRInt32 number2)
{
  return number1 + number2;
}

PRInt64
test_i64()
{
#if defined(WIN32) && !defined(__GNUC__)
  return 0x28590a1c921de000i64;
#else
  return 0x28590a1c921de000LL;
#endif
}

PRInt64
test_i64_i64(PRInt64 number)
{
  return number;
}

PRInt64
test_i64_i64_sum(PRInt64 number1, PRInt64 number2)
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
test_f_f_sum(float number1, float number2)
{
  return (number1 + number2);
}

double
test_d()
{
  return 1234567890123456789.5;
}

double
test_d_d(double number)
{
  return number;
}

double
test_d_d_sum(double number1, double number2)
{
  return (number1 + number2);
}

PRInt32
test_ansi_len(const char* string)
{
  return PRInt32(strlen(string));
}

PRInt32
test_wide_len(const PRUnichar* string)
{
  return PRInt32(NS_strlen(string));
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

PRInt32
test_floor(PRInt32 number1, float number2)
{
  return PRInt32(floor(float(number1) + number2));
}

