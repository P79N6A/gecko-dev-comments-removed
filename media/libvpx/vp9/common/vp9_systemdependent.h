









#ifndef VP9_COMMON_VP9_SYSTEMDEPENDENT_H_
#define VP9_COMMON_VP9_SYSTEMDEPENDENT_H_

#ifdef _MSC_VER
# if _MSC_VER > 1310 && (defined(_M_X64) || defined(_M_IX86))
#  include <intrin.h>
#  define USE_MSC_INTRIN
# endif
# include <math.h>
# if _MSC_VER < 1900
#  define snprintf _snprintf
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "./vpx_config.h"
#if ARCH_X86 || ARCH_X86_64
void vpx_reset_mmx_state(void);
#define vp9_clear_system_state() vpx_reset_mmx_state()
#else
#define vp9_clear_system_state()
#endif

#if defined(_MSC_VER) && _MSC_VER < 1800

static INLINE int round(double x) {
  if (x < 0)
    return (int)ceil(x - 0.5);
  else
    return (int)floor(x + 0.5);
}
#endif


#if defined(__GNUC__) && \
    ((__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || __GNUC__ >= 4)
static INLINE int get_msb(unsigned int n) {
  return 31 ^ __builtin_clz(n);
}
#elif defined(USE_MSC_INTRIN)
#pragma intrinsic(_BitScanReverse)

static INLINE int get_msb(unsigned int n) {
  unsigned long first_set_bit;
  _BitScanReverse(&first_set_bit, n);
  return first_set_bit;
}
#undef USE_MSC_INTRIN
#else

static INLINE int get_msb(unsigned int n) {
  int log = 0;
  unsigned int value = n;
  int i;

  for (i = 4; i >= 0; --i) {
    const int shift = (1 << i);
    const unsigned int x = value >> shift;
    if (x != 0) {
      value = x;
      log += shift;
    }
  }
  return log;
}
#endif

struct VP9Common;
void vp9_machine_specific_config(struct VP9Common *cm);

#ifdef __cplusplus
}  
#endif

#endif
