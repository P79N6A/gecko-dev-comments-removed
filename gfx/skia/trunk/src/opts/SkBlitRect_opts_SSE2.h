






#ifndef SkBlitRect_opts_SSE2_DEFINED
#define SkBlitRect_opts_SSE2_DEFINED

#include "SkColor.h"




void ColorRect32_SSE2(SkPMColor* SK_RESTRICT dst,
                      int width, int height,
                      size_t rowBytes, uint32_t color);


#endif
