


























#ifndef CPU_SUPPORT_H
#define CPU_SUPPORT_H

#include "opus_types.h"
#include "opus_defines.h"

#if defined(OPUS_HAVE_RTCD) && defined(OPUS_ARM_ASM)
#include "arm/armcpu.h"







#define OPUS_ARCHMASK 3

#else
#define OPUS_ARCHMASK 0

static OPUS_INLINE int opus_select_arch(void)
{
  return 0;
}
#endif

#endif
