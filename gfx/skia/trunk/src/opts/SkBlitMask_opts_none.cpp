






#include "SkBlitMask.h"

SkBlitMask::ColorProc SkBlitMask::PlatformColorProcs(SkColorType dstCT,
                                                     SkMask::Format maskFormat,
                                                     SkColor color) {
    return NULL;
}

SkBlitMask::BlitLCD16RowProc SkBlitMask::PlatformBlitRowProcs16(bool isOpaque) {
    return NULL;
}

SkBlitMask::RowProc SkBlitMask::PlatformRowProcs(SkColorType dstCT,
                                                 SkMask::Format maskFormat,
                                                 RowFlags flags) {
    return NULL;
}
