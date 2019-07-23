






































#include "nscore.h"

NS_EXTERN_C
{
  NS_EXPORT void test_v();

  NS_EXPORT short test_s();
  NS_EXPORT short test_s_s(short);
  NS_EXPORT short test_s_ss(short, short);

  NS_EXPORT int test_i();
  NS_EXPORT int test_i_i(int);
  NS_EXPORT int test_i_ii(int, int);

  NS_EXPORT float test_f();
  NS_EXPORT float test_f_f(float);
  NS_EXPORT float test_f_ff(float, float);

  NS_EXPORT double test_d();
  NS_EXPORT double test_d_d(double);
  NS_EXPORT double test_d_dd(double, double);

  NS_EXPORT int test_ansi_len(const char*);
  NS_EXPORT int test_wide_len(const PRUnichar*);
  NS_EXPORT const char* test_ansi_ret();
  NS_EXPORT const PRUnichar* test_wide_ret();
  NS_EXPORT char* test_ansi_echo(const char*);

  NS_EXPORT int test_i_if_floor(int, float);

  struct POINT {
    int x;
    int y;
  };

  struct RECT {
    int top;
    int left;
    int bottom;
    int right;
  };

  struct INNER {
    unsigned char i1;
    long long int i2;
    char i3;
  };

  struct NESTED {
    int n1;
    short n2;
    INNER inner;
    long long int n3;
    int n4;
  };

  NS_EXPORT int test_pt_in_rect(RECT, POINT);
  NS_EXPORT int test_nested_struct(NESTED);
  NS_EXPORT POINT test_struct_return(RECT);
}
