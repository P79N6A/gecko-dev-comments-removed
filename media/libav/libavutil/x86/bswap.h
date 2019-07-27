






















#ifndef AVUTIL_X86_BSWAP_H
#define AVUTIL_X86_BSWAP_H

#include <stdint.h>
#include "config.h"
#include "libavutil/attributes.h"

#if HAVE_INLINE_ASM

#if !AV_GCC_VERSION_AT_LEAST(4,1)
#define av_bswap16 av_bswap16
static av_always_inline av_const unsigned av_bswap16(unsigned x)
{
    __asm__("rorw $8, %w0" : "+r"(x));
    return x;
}
#endif 

#if !AV_GCC_VERSION_AT_LEAST(4,5)
#define av_bswap32 av_bswap32
static av_always_inline av_const uint32_t av_bswap32(uint32_t x)
{
    __asm__("bswap   %0" : "+r" (x));
    return x;
}

#if ARCH_X86_64
#define av_bswap64 av_bswap64
static inline uint64_t av_const av_bswap64(uint64_t x)
{
    __asm__("bswap  %0": "=r" (x) : "0" (x));
    return x;
}
#endif
#endif 

#endif 
#endif 
