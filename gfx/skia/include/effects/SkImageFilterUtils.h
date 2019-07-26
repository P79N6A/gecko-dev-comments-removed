






#ifndef SkImageFilterUtils_DEFINED
#define SkImageFilterUtils_DEFINED

#if SK_SUPPORT_GPU

#include "SkImageFilter.h"

class SkBitmap;
class GrTexture;
class SkImageFilter;

class SK_API SkImageFilterUtils {
public:
    


    static bool WrapTexture(GrTexture* texture, int width, int height, SkBitmap* result);

    




    static bool GetInputResultGPU(SkImageFilter* filter, SkImageFilter::Proxy* proxy, const SkBitmap& src, SkBitmap* result);
};

#endif

#endif
