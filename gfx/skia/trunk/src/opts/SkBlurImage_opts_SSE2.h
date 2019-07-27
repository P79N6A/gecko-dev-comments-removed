






#ifndef SkBlurImage_opts_SSE2_DEFINED
#define SkBlurImage_opts_SSE2_DEFINED

#include "SkBlurImage_opts.h"

bool SkBoxBlurGetPlatformProcs_SSE2(SkBoxBlurProc* boxBlurX,
                                    SkBoxBlurProc* boxBlurY,
                                    SkBoxBlurProc* boxBlurXY,
                                    SkBoxBlurProc* boxBlurYX);

#endif
