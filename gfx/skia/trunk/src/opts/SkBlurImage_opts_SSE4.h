






#ifndef SkBlurImage_opts_SSE4_DEFINED
#define SkBlurImage_opts_SSE4_DEFINED

#include "SkBlurImage_opts.h"

bool SkBoxBlurGetPlatformProcs_SSE4(SkBoxBlurProc* boxBlurX,
                                    SkBoxBlurProc* boxBlurY,
                                    SkBoxBlurProc* boxBlurXY,
                                    SkBoxBlurProc* boxBlurYX);

#endif
