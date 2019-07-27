



















#ifndef AVUTIL_INTMATH_H
#define AVUTIL_INTMATH_H

#include <stdint.h>

#include "config.h"
#include "attributes.h"

#if ARCH_ARM
#   include "arm/intmath.h"
#endif






#if HAVE_FAST_CLZ && AV_GCC_VERSION_AT_LEAST(3,4)

#ifndef ff_log2
#   define ff_log2(x) (31 - __builtin_clz((x)|1))
#   ifndef ff_log2_16bit
#      define ff_log2_16bit av_log2
#   endif
#endif 

#endif 

extern const uint8_t ff_log2_tab[256];

#ifndef ff_log2
#define ff_log2 ff_log2_c
static av_always_inline av_const int ff_log2_c(unsigned int v)
{
    int n = 0;
    if (v & 0xffff0000) {
        v >>= 16;
        n += 16;
    }
    if (v & 0xff00) {
        v >>= 8;
        n += 8;
    }
    n += ff_log2_tab[v];

    return n;
}
#endif

#ifndef ff_log2_16bit
#define ff_log2_16bit ff_log2_16bit_c
static av_always_inline av_const int ff_log2_16bit_c(unsigned int v)
{
    int n = 0;
    if (v & 0xff00) {
        v >>= 8;
        n += 8;
    }
    n += ff_log2_tab[v];

    return n;
}
#endif

#define av_log2       ff_log2
#define av_log2_16bit ff_log2_16bit










#if HAVE_FAST_CLZ && AV_GCC_VERSION_AT_LEAST(3,4)
#ifndef ff_ctz
#define ff_ctz(v) __builtin_ctz(v)
#endif
#endif

#ifndef ff_ctz
#define ff_ctz ff_ctz_c
static av_always_inline av_const int ff_ctz_c(int v)
{
    int c;

    if (v & 0x1)
        return 0;

    c = 1;
    if (!(v & 0xffff)) {
        v >>= 16;
        c += 16;
    }
    if (!(v & 0xff)) {
        v >>= 8;
        c += 8;
    }
    if (!(v & 0xf)) {
        v >>= 4;
        c += 4;
    }
    if (!(v & 0x3)) {
        v >>= 2;
        c += 2;
    }
    c -= v & 0x1;

    return c;
}
#endif







int av_ctz(int v);




#endif 
