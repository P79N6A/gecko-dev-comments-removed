



















#ifndef AVUTIL_X86_TIMER_H
#define AVUTIL_X86_TIMER_H

#include <stdint.h>

#if HAVE_INLINE_ASM

#define AV_READ_TIME read_time

static inline uint64_t read_time(void)
{
    uint32_t a, d;
    __asm__ volatile("rdtsc" : "=a" (a), "=d" (d));
    return ((uint64_t)d << 32) + a;
}

#elif HAVE_RDTSC

#include <intrin.h>
#define AV_READ_TIME __rdtsc

#endif 

#endif 
