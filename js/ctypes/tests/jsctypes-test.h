







































#include "nscore.h"
#include "prtypes.h"

NS_EXTERN_C
{
  NS_EXPORT void test_v();

  NS_EXPORT PRInt8 test_i8();
  NS_EXPORT PRInt8 test_i8_i8(PRInt8);
  NS_EXPORT PRInt8 test_i8_i8_sum(PRInt8, PRInt8);

  NS_EXPORT PRInt16 test_i16();
  NS_EXPORT PRInt16 test_i16_i16(PRInt16);
  NS_EXPORT PRInt16 test_i16_i16_sum(PRInt16, PRInt16);

  NS_EXPORT PRInt32 test_i32();
  NS_EXPORT PRInt32 test_i32_i32(PRInt32);
  NS_EXPORT PRInt32 test_i32_i32_sum(PRInt32, PRInt32);

  NS_EXPORT PRInt64 test_i64();
  NS_EXPORT PRInt64 test_i64_i64(PRInt64);
  NS_EXPORT PRInt64 test_i64_i64_sum(PRInt64, PRInt64);

  NS_EXPORT float test_f();
  NS_EXPORT float test_f_f(float);
  NS_EXPORT float test_f_f_sum(float, float);

  NS_EXPORT double test_d();
  NS_EXPORT double test_d_d(double);
  NS_EXPORT double test_d_d_sum(double, double);

  NS_EXPORT PRInt32 test_ansi_len(const char*);
  NS_EXPORT PRInt32 test_wide_len(const PRUnichar*);
  NS_EXPORT const char* test_ansi_ret();
  NS_EXPORT const PRUnichar* test_wide_ret();
  NS_EXPORT char* test_ansi_echo(const char*);

  NS_EXPORT PRInt32 test_floor(PRInt32, float);
}

