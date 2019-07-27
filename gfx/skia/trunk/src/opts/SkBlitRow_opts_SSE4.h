






#ifndef SkBlitRow_opts_SSE4_DEFINED
#define SkBlitRow_opts_SSE4_DEFINED

#include "SkBlitRow.h"






#if  (defined(__clang__) || (defined(__GNUC__) && !defined(SK_BUILD_FOR_MAC))) \
     && !defined(SK_BUILD_FOR_WIN)                                             \
     && !defined(MEMORY_SANITIZER)
extern "C" void S32A_Opaque_BlitRow32_SSE4_asm(SkPMColor* SK_RESTRICT dst,
                                               const SkPMColor* SK_RESTRICT src,
                                               int count, U8CPU alpha);

#define SK_ATT_ASM_SUPPORTED
#endif

#endif

