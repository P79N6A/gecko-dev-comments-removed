




































#include "opus_types.h"
#include <math.h>
#include <limits.h>
#include "arch.h"
#if !defined(_ecintrin_H)
# define _ecintrin_H (1)











# define EC_MINI(_a,_b)      ((_a)+(((_b)-(_a))&-((_b)<(_a))))




#if defined(_MSC_VER)
# include <intrin.h>

# pragma intrinsic(_BitScanReverse)

static __inline int ec_bsr(unsigned long _x){
  unsigned long ret;
  _BitScanReverse(&ret,_x);
  return (int)ret;
}
# define EC_CLZ0    (1)
# define EC_CLZ(_x) (-ec_bsr(_x))
#elif defined(ENABLE_TI_DSPLIB)
# include "dsplib.h"
# define EC_CLZ0    (31)
# define EC_CLZ(_x) (_lnorm(_x))
#elif __GNUC_PREREQ(3,4)
# if INT_MAX>=2147483647
#  define EC_CLZ0    ((int)sizeof(unsigned)*CHAR_BIT)
#  define EC_CLZ(_x) (__builtin_clz(_x))
# elif LONG_MAX>=2147483647L
#  define EC_CLZ0    ((int)sizeof(unsigned long)*CHAR_BIT)
#  define EC_CLZ(_x) (__builtin_clzl(_x))
# endif
#endif

#if defined(EC_CLZ)




# define EC_ILOG(_x) (EC_CLZ0-EC_CLZ(_x))
#else
int ec_ilog(opus_uint32 _v);
# define EC_ILOG(_x) (ec_ilog(_x))
#endif
#endif
