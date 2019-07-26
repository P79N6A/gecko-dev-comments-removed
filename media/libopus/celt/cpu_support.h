


























#ifndef CPU_SUPPORT_H
#define CPU_SUPPORT_H

#if defined(OPUS_HAVE_RTCD) && defined(ARMv4_ASM)
#include "arm/armcpu.h"







#define OPUS_ARCHMASK 3

#else
#define OPUS_ARCHMASK 0

static inline int opus_select_arch(void)
{
  return 0;
}
#endif

#endif
