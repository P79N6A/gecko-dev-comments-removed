






#include "SkBlitRow.h"
#include "SkBlitMask.h"



SkBlitRow::Proc SkBlitRow::PlatformProcs4444(unsigned flags) {
    return NULL;
}

SkBlitRow::Proc SkBlitRow::PlatformProcs565(unsigned flags) {
    return NULL;
}

SkBlitRow::Proc32 SkBlitRow::PlatformProcs32(unsigned flags) {
    return NULL;
}

SkBlitRow::ColorProc SkBlitRow::PlatformColorProc() {
    return NULL;
}



SkBlitMask::ColorProc SkBlitMask::PlatformColorProcs(SkBitmap::Config dstConfig,
                                                     SkMask::Format maskFormat,
                                                     SkColor color) {
   return NULL;
}

SkBlitMask::RowProc SkBlitMask::PlatformRowProcs(SkBitmap::Config dstConfig,
                                                 SkMask::Format maskFormat,
                                                 RowFlags flags) {
    return NULL;
}

