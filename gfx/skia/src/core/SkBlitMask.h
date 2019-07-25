






#ifndef SkBlitMask_DEFINED
#define SkBlitMask_DEFINED

#include "SkBitmap.h"
#include "SkMask.h"

class SkBlitMask {
public:
    



    static bool BlitColor(const SkBitmap& device, const SkMask& mask,
                          const SkIRect& clip, SkColor color);

    




    typedef void (*Proc)(void* dst, size_t dstRB,
                         const void* mask, size_t maskRB,
                         SkColor color, int width, int height);

    



    static Proc Factory(SkBitmap::Config dstConfig, SkMask::Format, SkColor);

    



    static Proc PlatformProcs(SkBitmap::Config dstConfig, SkMask::Format, SkColor);
};

#endif
