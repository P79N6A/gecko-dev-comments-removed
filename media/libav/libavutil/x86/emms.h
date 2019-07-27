

















#ifndef AVUTIL_X86_EMMS_H
#define AVUTIL_X86_EMMS_H

#include "config.h"
#include "libavutil/attributes.h"

void avpriv_emms_yasm(void);

#if HAVE_MMX_INLINE
#   define emms_c emms_c





static av_always_inline void emms_c(void)
{
    __asm__ volatile ("emms" ::: "memory");
}
#elif HAVE_MMX && HAVE_MM_EMPTY
#   include <mmintrin.h>
#   define emms_c _mm_empty
#elif HAVE_MMX_EXTERNAL
#   define emms_c avpriv_emms_yasm
#endif 

#endif 
