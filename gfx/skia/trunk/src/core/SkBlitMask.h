






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

    




    typedef void (*BlitLCD16RowProc)(SkPMColor dst[], const uint16_t src[],
                                     SkColor color, int width,
                                     SkPMColor opaqueDst);

    




    typedef void (*RowProc)(void* dst, const void* mask,
                            const SkPMColor* src, int width);

    



    static ColorProc ColorFactory(SkColorType, SkMask::Format, SkColor);

    



    static ColorProc PlatformColorProcs(SkColorType, SkMask::Format, SkColor);

    


    static BlitLCD16RowProc BlitLCD16RowFactory(bool isOpaque);

    



    static BlitLCD16RowProc PlatformBlitRowProcs16(bool isOpaque);

    enum RowFlags {
        kSrcIsOpaque_RowFlag    = 1 << 0
    };

    



    static RowProc RowFactory(SkColorType, SkMask::Format, RowFlags);

    



    static RowProc PlatformRowProcs(SkColorType, SkMask::Format, RowFlags);
};

#endif
