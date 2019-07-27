










#ifndef VPX_PORTS_MEM_H_
#define VPX_PORTS_MEM_H_

#include "vpx_config.h"
#include "vpx/vpx_integer.h"

#if (defined(__GNUC__) && __GNUC__) || defined(__SUNPRO_C)
#define DECLARE_ALIGNED(n,typ,val)  typ val __attribute__ ((aligned (n)))
#elif defined(_MSC_VER)
#define DECLARE_ALIGNED(n,typ,val)  __declspec(align(n)) typ val
#else
#warning No alignment directives known for this compiler.
#define DECLARE_ALIGNED(n,typ,val)  typ val
#endif





#if defined(__GNUC__) && __GNUC__
#define UNINITIALIZED_IS_SAFE(x) x=x
#else
#define UNINITIALIZED_IS_SAFE(x) x
#endif

#if HAVE_NEON && defined(_MSC_VER)
#define __builtin_prefetch(x)
#endif


#define ROUND_POWER_OF_TWO(value, n) \
    (((value) + (1 << ((n) - 1))) >> (n))

#define ALIGN_POWER_OF_TWO(value, n) \
    (((value) + ((1 << (n)) - 1)) & ~((1 << (n)) - 1))

#if CONFIG_VP9_HIGHBITDEPTH
#define CONVERT_TO_SHORTPTR(x) ((uint16_t*)(((uintptr_t)x) << 1))
#define CONVERT_TO_BYTEPTR(x) ((uint8_t*)(((uintptr_t)x) >> 1))
#endif  

#endif  
