






#include "SkStippleMaskFilter.h"
#include "SkString.h"

bool SkStippleMaskFilter::filterMask(SkMask* dst,
                                     const SkMask& src,
                                     const SkMatrix& matrix,
                                     SkIPoint* margin) const {

    if (src.fFormat != SkMask::kA8_Format) {
        return false;
    }

    dst->fBounds = src.fBounds;
    dst->fRowBytes = dst->fBounds.width();
    dst->fFormat = SkMask::kA8_Format;
    dst->fImage = NULL;

    if (NULL != src.fImage) {
        size_t dstSize = dst->computeImageSize();
        if (0 == dstSize) {
            return false;   
        }

        dst->fImage = SkMask::AllocImage(dstSize);

        uint8_t* srcScanLine = src.fImage;
        uint8_t* scanline = dst->fImage;

        for (int y = 0; y < src.fBounds.height(); ++y) {
            for (int x = 0; x < src.fBounds.width(); ++x) {
                scanline[x] = srcScanLine[x] && ((x+y) & 0x1) ? 0xFF : 0x00;
            }
            scanline += dst->fRowBytes;
            srcScanLine += src.fRowBytes;
        }
    }

    return true;
}

#ifdef SK_DEVELOPER
void SkStippleMaskFilter::toString(SkString* str) const {
    str->append("SkStippleMaskFilter: ()");
}
#endif
