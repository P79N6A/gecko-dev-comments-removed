














#include "SkUtils.h"

#if defined(__ARM_HAVE_NEON) && defined(SK_CPU_LENDIAN)
extern "C" void memset16_neon(uint16_t dst[], uint16_t value, int count);
extern "C" void memset32_neon(uint32_t dst[], uint32_t value, int count);
#endif

#if defined(SK_CPU_LENDIAN)
extern "C" void arm_memset16(uint16_t* dst, uint16_t value, int count);
extern "C" void arm_memset32(uint32_t* dst, uint32_t value, int count);
#endif

SkMemset16Proc SkMemset16GetPlatformProc() {
#if defined(__ARM_HAVE_NEON) && defined(SK_CPU_LENDIAN)
    return memset16_neon;
#elif defined(SK_CPU_LENDIAN)
    return arm_memset16;
#else
    return NULL;
#endif
}

SkMemset32Proc SkMemset32GetPlatformProc() {
#if defined(__ARM_HAVE_NEON) && defined(SK_CPU_LENDIAN)
    return memset32_neon;
#elif defined(SK_CPU_LENDIAN)
    return arm_memset32;
#else
    return NULL;
#endif
}
