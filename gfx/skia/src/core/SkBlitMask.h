






#ifndef SkBlitMask_DEFINED
#define SkBlitMask_DEFINED

#include "SkBitmap.h"
#include "SkColor.h"
#include "SkMask.h"

class SkBlitMask {
public:
    



    static bool BlitColor(const SkBitmap& device, const SkMask& mask,
                          const SkIRect& clip, SkColor color);

    




    typedef void (*ColorProc)(void* dst, size_t dstRB,
                              const void* mask, size_t maskRB,
                              SkColor color, int width, int height);

    




    typedef void (*RowProc)(void* dst, const void* mask,
                            const SkPMColor* src, int width);
    
    



    static ColorProc ColorFactory(SkBitmap::Config, SkMask::Format, SkColor);
    
    



    static ColorProc PlatformColorProcs(SkBitmap::Config, SkMask::Format, SkColor);

    enum RowFlags {
        kSrcIsOpaque_RowFlag    = 1 << 0
    };

    



    static RowProc RowFactory(SkBitmap::Config, SkMask::Format, RowFlags);
    
    



    static RowProc PlatformRowProcs(SkBitmap::Config, SkMask::Format, RowFlags);
};

#endif
