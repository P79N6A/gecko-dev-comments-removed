







#include "SkScaledBitmapSampler.h"
#include "SkBitmap.h"
#include "SkColorPriv.h"
#include "SkDither.h"
#include "SkTypes.h"



static bool Sample_Gray_D8888(void* SK_RESTRICT dstRow,
                              const uint8_t* SK_RESTRICT src,
                              int width, int deltaSrc, int, const SkPMColor[]) {
    SkPMColor* SK_RESTRICT dst = (SkPMColor*)dstRow;
    for (int x = 0; x < width; x++) {
        dst[x] = SkPackARGB32(0xFF, src[0], src[0], src[0]);
        src += deltaSrc;
    }
    return false;
}

static SkScaledBitmapSampler::RowProc
get_gray_to_8888_proc(const SkScaledBitmapSampler::Options& opts) {
    
    return Sample_Gray_D8888;
}

static bool Sample_RGBx_D8888(void* SK_RESTRICT dstRow,
                              const uint8_t* SK_RESTRICT src,
                              int width, int deltaSrc, int, const SkPMColor[]) {
    SkPMColor* SK_RESTRICT dst = (SkPMColor*)dstRow;
    for (int x = 0; x < width; x++) {
        dst[x] = SkPackARGB32(0xFF, src[0], src[1], src[2]);
        src += deltaSrc;
    }
    return false;
}

static SkScaledBitmapSampler::RowProc
get_RGBx_to_8888_proc(const SkScaledBitmapSampler::Options& opts) {
    
    return Sample_RGBx_D8888;
}

static bool Sample_RGBA_D8888(void* SK_RESTRICT dstRow,
                              const uint8_t* SK_RESTRICT src,
                              int width, int deltaSrc, int, const SkPMColor[]) {
    SkPMColor* SK_RESTRICT dst = (SkPMColor*)dstRow;
    unsigned alphaMask = 0xFF;
    for (int x = 0; x < width; x++) {
        unsigned alpha = src[3];
        dst[x] = SkPreMultiplyARGB(alpha, src[0], src[1], src[2]);
        src += deltaSrc;
        alphaMask &= alpha;
    }
    return alphaMask != 0xFF;
}

static bool Sample_RGBA_D8888_Unpremul(void* SK_RESTRICT dstRow,
                                       const uint8_t* SK_RESTRICT src,
                                       int width, int deltaSrc, int,
                                       const SkPMColor[]) {
    uint32_t* SK_RESTRICT dst = reinterpret_cast<uint32_t*>(dstRow);
    unsigned alphaMask = 0xFF;
    for (int x = 0; x < width; x++) {
        unsigned alpha = src[3];
        dst[x] = SkPackARGB32NoCheck(alpha, src[0], src[1], src[2]);
        src += deltaSrc;
        alphaMask &= alpha;
    }
    return alphaMask != 0xFF;
}

static bool Sample_RGBA_D8888_SkipZ(void* SK_RESTRICT dstRow,
                                    const uint8_t* SK_RESTRICT src,
                                    int width, int deltaSrc, int,
                                    const SkPMColor[]) {
    SkPMColor* SK_RESTRICT dst = (SkPMColor*)dstRow;
    unsigned alphaMask = 0xFF;
    for (int x = 0; x < width; x++) {
        unsigned alpha = src[3];
        if (0 != alpha) {
            dst[x] = SkPreMultiplyARGB(alpha, src[0], src[1], src[2]);
        }
        src += deltaSrc;
        alphaMask &= alpha;
    }
    return alphaMask != 0xFF;
}

static SkScaledBitmapSampler::RowProc
get_RGBA_to_8888_proc(const SkScaledBitmapSampler::Options& opts) {
    
    if (!opts.fPremultiplyAlpha) {
        
        
        return Sample_RGBA_D8888_Unpremul;
    }
    
    if (opts.fSkipZeros) {
        return Sample_RGBA_D8888_SkipZ;
    }
    return Sample_RGBA_D8888;
}



static bool Sample_Gray_D565(void* SK_RESTRICT dstRow,
                             const uint8_t* SK_RESTRICT src,
                             int width, int deltaSrc, int, const SkPMColor[]) {
    uint16_t* SK_RESTRICT dst = (uint16_t*)dstRow;
    for (int x = 0; x < width; x++) {
        dst[x] = SkPack888ToRGB16(src[0], src[0], src[0]);
        src += deltaSrc;
    }
    return false;
}

static bool Sample_Gray_D565_D(void* SK_RESTRICT dstRow,
                               const uint8_t* SK_RESTRICT src,
                           int width, int deltaSrc, int y, const SkPMColor[]) {
    uint16_t* SK_RESTRICT dst = (uint16_t*)dstRow;
    DITHER_565_SCAN(y);
    for (int x = 0; x < width; x++) {
        dst[x] = SkDitherRGBTo565(src[0], src[0], src[0], DITHER_VALUE(x));
        src += deltaSrc;
    }
    return false;
}

static SkScaledBitmapSampler::RowProc
get_gray_to_565_proc(const SkScaledBitmapSampler::Options& opts) {
    
    if (opts.fDither) {
        return Sample_Gray_D565_D;
    }
    return Sample_Gray_D565;
}

static bool Sample_RGBx_D565(void* SK_RESTRICT dstRow,
                             const uint8_t* SK_RESTRICT src,
                             int width, int deltaSrc, int, const SkPMColor[]) {
    uint16_t* SK_RESTRICT dst = (uint16_t*)dstRow;
    for (int x = 0; x < width; x++) {
        dst[x] = SkPack888ToRGB16(src[0], src[1], src[2]);
        src += deltaSrc;
    }
    return false;
}

static bool Sample_RGBx_D565_D(void* SK_RESTRICT dstRow,
                               const uint8_t* SK_RESTRICT src,
                               int width, int deltaSrc, int y,
                               const SkPMColor[]) {
    uint16_t* SK_RESTRICT dst = (uint16_t*)dstRow;
    DITHER_565_SCAN(y);
    for (int x = 0; x < width; x++) {
        dst[x] = SkDitherRGBTo565(src[0], src[1], src[2], DITHER_VALUE(x));
        src += deltaSrc;
    }
    return false;
}

static SkScaledBitmapSampler::RowProc
get_RGBx_to_565_proc(const SkScaledBitmapSampler::Options& opts) {
    
    if (opts.fDither) {
        return Sample_RGBx_D565_D;
    }
    return Sample_RGBx_D565;
}


static bool Sample_D565_D565(void* SK_RESTRICT dstRow,
                             const uint8_t* SK_RESTRICT src,
                             int width, int deltaSrc, int, const SkPMColor[]) {
    uint16_t* SK_RESTRICT dst = (uint16_t*)dstRow;
    uint16_t* SK_RESTRICT castedSrc = (uint16_t*) src;
    for (int x = 0; x < width; x++) {
        dst[x] = castedSrc[0];
        castedSrc += deltaSrc >> 1;
    }
    return false;
}

static SkScaledBitmapSampler::RowProc
get_565_to_565_proc(const SkScaledBitmapSampler::Options& opts) {
    
    return Sample_D565_D565;
}



static bool Sample_Gray_D4444(void* SK_RESTRICT dstRow,
                              const uint8_t* SK_RESTRICT src,
                              int width, int deltaSrc, int, const SkPMColor[]) {
    SkPMColor16* SK_RESTRICT dst = (SkPMColor16*)dstRow;
    for (int x = 0; x < width; x++) {
        unsigned gray = src[0] >> 4;
        dst[x] = SkPackARGB4444(0xF, gray, gray, gray);
        src += deltaSrc;
    }
    return false;
}

static bool Sample_Gray_D4444_D(void* SK_RESTRICT dstRow,
                                const uint8_t* SK_RESTRICT src,
                            int width, int deltaSrc, int y, const SkPMColor[]) {
    SkPMColor16* SK_RESTRICT dst = (SkPMColor16*)dstRow;
    DITHER_4444_SCAN(y);
    for (int x = 0; x < width; x++) {
        dst[x] = SkDitherARGB32To4444(0xFF, src[0], src[0], src[0],
                                      DITHER_VALUE(x));
        src += deltaSrc;
    }
    return false;
}

static SkScaledBitmapSampler::RowProc
get_gray_to_4444_proc(const SkScaledBitmapSampler::Options& opts) {
    
    if (opts.fDither) {
        return Sample_Gray_D4444_D;
    }
    return Sample_Gray_D4444;
}

static bool Sample_RGBx_D4444(void* SK_RESTRICT dstRow,
                              const uint8_t* SK_RESTRICT src,
                              int width, int deltaSrc, int, const SkPMColor[]) {
    SkPMColor16* SK_RESTRICT dst = (SkPMColor16*)dstRow;
    for (int x = 0; x < width; x++) {
        dst[x] = SkPackARGB4444(0xF, src[0] >> 4, src[1] >> 4, src[2] >> 4);
        src += deltaSrc;
    }
    return false;
}

static bool Sample_RGBx_D4444_D(void* SK_RESTRICT dstRow,
                                const uint8_t* SK_RESTRICT src,
                            int width, int deltaSrc, int y, const SkPMColor[]) {
    SkPMColor16* dst = (SkPMColor16*)dstRow;
    DITHER_4444_SCAN(y);

    for (int x = 0; x < width; x++) {
        dst[x] = SkDitherARGB32To4444(0xFF, src[0], src[1], src[2],
                                      DITHER_VALUE(x));
        src += deltaSrc;
    }
    return false;
}

static SkScaledBitmapSampler::RowProc
get_RGBx_to_4444_proc(const SkScaledBitmapSampler::Options& opts) {
    
    if (opts.fDither) {
        return Sample_RGBx_D4444_D;
    }
    return Sample_RGBx_D4444;
}

static bool Sample_RGBA_D4444(void* SK_RESTRICT dstRow,
                              const uint8_t* SK_RESTRICT src,
                              int width, int deltaSrc, int, const SkPMColor[]) {
    SkPMColor16* SK_RESTRICT dst = (SkPMColor16*)dstRow;
    unsigned alphaMask = 0xFF;

    for (int x = 0; x < width; x++) {
        unsigned alpha = src[3];
        SkPMColor c = SkPreMultiplyARGB(alpha, src[0], src[1], src[2]);
        dst[x] = SkPixel32ToPixel4444(c);
        src += deltaSrc;
        alphaMask &= alpha;
    }
    return alphaMask != 0xFF;
}

static bool Sample_RGBA_D4444_SkipZ(void* SK_RESTRICT dstRow,
                                    const uint8_t* SK_RESTRICT src,
                                    int width, int deltaSrc, int,
                                    const SkPMColor[]) {
    SkPMColor16* SK_RESTRICT dst = (SkPMColor16*)dstRow;
    unsigned alphaMask = 0xFF;

    for (int x = 0; x < width; x++) {
        unsigned alpha = src[3];
        if (alpha != 0) {
            SkPMColor c = SkPreMultiplyARGB(alpha, src[0], src[1], src[2]);
            dst[x] = SkPixel32ToPixel4444(c);
        }
        src += deltaSrc;
        alphaMask &= alpha;
    }
    return alphaMask != 0xFF;
}


static bool Sample_RGBA_D4444_D(void* SK_RESTRICT dstRow,
                                const uint8_t* SK_RESTRICT src,
                                int width, int deltaSrc, int y,
                                const SkPMColor[]) {
    SkPMColor16* SK_RESTRICT dst = (SkPMColor16*)dstRow;
    unsigned alphaMask = 0xFF;
    DITHER_4444_SCAN(y);

    for (int x = 0; x < width; x++) {
        unsigned alpha = src[3];
        SkPMColor c = SkPreMultiplyARGB(alpha, src[0], src[1], src[2]);
        dst[x] = SkDitherARGB32To4444(c, DITHER_VALUE(x));
        src += deltaSrc;
        alphaMask &= alpha;
    }
    return alphaMask != 0xFF;
}

static bool Sample_RGBA_D4444_D_SkipZ(void* SK_RESTRICT dstRow,
                                      const uint8_t* SK_RESTRICT src,
                                      int width, int deltaSrc, int y,
                                      const SkPMColor[]) {
    SkPMColor16* SK_RESTRICT dst = (SkPMColor16*)dstRow;
    unsigned alphaMask = 0xFF;
    DITHER_4444_SCAN(y);

    for (int x = 0; x < width; x++) {
        unsigned alpha = src[3];
        if (alpha != 0) {
            SkPMColor c = SkPreMultiplyARGB(alpha, src[0], src[1], src[2]);
            dst[x] = SkDitherARGB32To4444(c, DITHER_VALUE(x));
        }
        src += deltaSrc;
        alphaMask &= alpha;
    }
    return alphaMask != 0xFF;
}

static SkScaledBitmapSampler::RowProc
get_RGBA_to_4444_proc(const SkScaledBitmapSampler::Options& opts) {
    if (!opts.fPremultiplyAlpha) {
        
        return NULL;
    }
    if (opts.fSkipZeros) {
        if (opts.fDither) {
            return Sample_RGBA_D4444_D_SkipZ;
        }
        return Sample_RGBA_D4444_SkipZ;
    }
    if (opts.fDither) {
        return Sample_RGBA_D4444_D;
    }
    return Sample_RGBA_D4444;
}



#define A32_MASK_IN_PLACE   (SkPMColor)(SK_A32_MASK << SK_A32_SHIFT)

static bool Sample_Index_D8888(void* SK_RESTRICT dstRow,
                               const uint8_t* SK_RESTRICT src,
                       int width, int deltaSrc, int, const SkPMColor ctable[]) {

    SkPMColor* SK_RESTRICT dst = (SkPMColor*)dstRow;
    SkPMColor cc = A32_MASK_IN_PLACE;
    for (int x = 0; x < width; x++) {
        SkPMColor c = ctable[*src];
        cc &= c;
        dst[x] = c;
        src += deltaSrc;
    }
    return cc != A32_MASK_IN_PLACE;
}

static bool Sample_Index_D8888_SkipZ(void* SK_RESTRICT dstRow,
                                     const uint8_t* SK_RESTRICT src,
                                     int width, int deltaSrc, int,
                                     const SkPMColor ctable[]) {

    SkPMColor* SK_RESTRICT dst = (SkPMColor*)dstRow;
    SkPMColor cc = A32_MASK_IN_PLACE;
    for (int x = 0; x < width; x++) {
        SkPMColor c = ctable[*src];
        cc &= c;
        if (c != 0) {
            dst[x] = c;
        }
        src += deltaSrc;
    }
    return cc != A32_MASK_IN_PLACE;
}

static SkScaledBitmapSampler::RowProc
get_index_to_8888_proc(const SkScaledBitmapSampler::Options& opts) {
    if (!opts.fPremultiplyAlpha) {
        
        return NULL;
    }
    
    if (opts.fSkipZeros) {
        return Sample_Index_D8888_SkipZ;
    }
    return Sample_Index_D8888;
}

static bool Sample_Index_D565(void* SK_RESTRICT dstRow,
                               const uint8_t* SK_RESTRICT src,
                       int width, int deltaSrc, int, const SkPMColor ctable[]) {

    uint16_t* SK_RESTRICT dst = (uint16_t*)dstRow;
    for (int x = 0; x < width; x++) {
        dst[x] = SkPixel32ToPixel16(ctable[*src]);
        src += deltaSrc;
    }
    return false;
}

static bool Sample_Index_D565_D(void* SK_RESTRICT dstRow,
                                const uint8_t* SK_RESTRICT src, int width,
                                int deltaSrc, int y, const SkPMColor ctable[]) {

    uint16_t* SK_RESTRICT dst = (uint16_t*)dstRow;
    DITHER_565_SCAN(y);

    for (int x = 0; x < width; x++) {
        SkPMColor c = ctable[*src];
        dst[x] = SkDitherRGBTo565(SkGetPackedR32(c), SkGetPackedG32(c),
                                  SkGetPackedB32(c), DITHER_VALUE(x));
        src += deltaSrc;
    }
    return false;
}

static SkScaledBitmapSampler::RowProc
get_index_to_565_proc(const SkScaledBitmapSampler::Options& opts) {
    
    if (opts.fDither) {
        return Sample_Index_D565_D;
    }
    return Sample_Index_D565;
}

static bool Sample_Index_D4444(void* SK_RESTRICT dstRow,
                               const uint8_t* SK_RESTRICT src, int width,
                               int deltaSrc, int y, const SkPMColor ctable[]) {

    SkPMColor16* SK_RESTRICT dst = (SkPMColor16*)dstRow;
    SkPMColor cc = A32_MASK_IN_PLACE;
    for (int x = 0; x < width; x++) {
        SkPMColor c = ctable[*src];
        cc &= c;
        dst[x] = SkPixel32ToPixel4444(c);
        src += deltaSrc;
    }
    return cc != A32_MASK_IN_PLACE;
}

static bool Sample_Index_D4444_D(void* SK_RESTRICT dstRow,
                                 const uint8_t* SK_RESTRICT src, int width,
                                int deltaSrc, int y, const SkPMColor ctable[]) {

    SkPMColor16* SK_RESTRICT dst = (SkPMColor16*)dstRow;
    SkPMColor cc = A32_MASK_IN_PLACE;
    DITHER_4444_SCAN(y);

    for (int x = 0; x < width; x++) {
        SkPMColor c = ctable[*src];
        cc &= c;
        dst[x] = SkDitherARGB32To4444(c, DITHER_VALUE(x));
        src += deltaSrc;
    }
    return cc != A32_MASK_IN_PLACE;
}

static bool Sample_Index_D4444_SkipZ(void* SK_RESTRICT dstRow,
                                     const uint8_t* SK_RESTRICT src, int width,
                                     int deltaSrc, int y, const SkPMColor ctable[]) {

    SkPMColor16* SK_RESTRICT dst = (SkPMColor16*)dstRow;
    SkPMColor cc = A32_MASK_IN_PLACE;
    for (int x = 0; x < width; x++) {
        SkPMColor c = ctable[*src];
        cc &= c;
        if (c != 0) {
            dst[x] = SkPixel32ToPixel4444(c);
        }
        src += deltaSrc;
    }
    return cc != A32_MASK_IN_PLACE;
}

static bool Sample_Index_D4444_D_SkipZ(void* SK_RESTRICT dstRow,
                                       const uint8_t* SK_RESTRICT src, int width,
                                       int deltaSrc, int y, const SkPMColor ctable[]) {

    SkPMColor16* SK_RESTRICT dst = (SkPMColor16*)dstRow;
    SkPMColor cc = A32_MASK_IN_PLACE;
    DITHER_4444_SCAN(y);

    for (int x = 0; x < width; x++) {
        SkPMColor c = ctable[*src];
        cc &= c;
        if (c != 0) {
            dst[x] = SkDitherARGB32To4444(c, DITHER_VALUE(x));
        }
        src += deltaSrc;
    }
    return cc != A32_MASK_IN_PLACE;
}

static SkScaledBitmapSampler::RowProc
get_index_to_4444_proc(const SkScaledBitmapSampler::Options& opts) {
    
    if (!opts.fPremultiplyAlpha) {
        return NULL;
    }
    if (opts.fSkipZeros) {
        if (opts.fDither) {
            return Sample_Index_D4444_D_SkipZ;
        }
        return Sample_Index_D4444_SkipZ;
    }
    if (opts.fDither) {
        return Sample_Index_D4444_D;
    }
    return Sample_Index_D4444;
}

static bool Sample_Index_DI(void* SK_RESTRICT dstRow,
                            const uint8_t* SK_RESTRICT src,
                            int width, int deltaSrc, int, const SkPMColor[]) {
    if (1 == deltaSrc) {
        memcpy(dstRow, src, width);
    } else {
        uint8_t* SK_RESTRICT dst = (uint8_t*)dstRow;
        for (int x = 0; x < width; x++) {
            dst[x] = src[0];
            src += deltaSrc;
        }
    }
    return false;
}

static SkScaledBitmapSampler::RowProc
get_index_to_index_proc(const SkScaledBitmapSampler::Options& opts) {
    
    if (!opts.fPremultiplyAlpha) {
        return NULL;
    }
    
    return Sample_Index_DI;
}


static bool Sample_Gray_DA8(void* SK_RESTRICT dstRow,
                            const uint8_t* SK_RESTRICT src,
                            int width, int deltaSrc, int,
                            const SkPMColor[]) {
    
    
    
    (void) Sample_Index_DI(dstRow, src, width, deltaSrc,  0,
                            NULL);
    return true;
}

static SkScaledBitmapSampler::RowProc
get_gray_to_A8_proc(const SkScaledBitmapSampler::Options& opts) {
    if (!opts.fPremultiplyAlpha) {
        return NULL;
    }
    
    return Sample_Gray_DA8;
}

typedef SkScaledBitmapSampler::RowProc (*RowProcChooser)(const SkScaledBitmapSampler::Options&);


#include "SkScaledBitmapSampler.h"

SkScaledBitmapSampler::SkScaledBitmapSampler(int width, int height,
                                             int sampleSize) {
    fCTable = NULL;
    fDstRow = NULL;
    fRowProc = NULL;

    if (width <= 0 || height <= 0) {
        sk_throw();
    }

    SkDEBUGCODE(fSampleMode = kUninitialized_SampleMode);

    if (sampleSize <= 1) {
        fScaledWidth = width;
        fScaledHeight = height;
        fX0 = fY0 = 0;
        fDX = fDY = 1;
        return;
    }

    int dx = SkMin32(sampleSize, width);
    int dy = SkMin32(sampleSize, height);

    fScaledWidth = width / dx;
    fScaledHeight = height / dy;

    SkASSERT(fScaledWidth > 0);
    SkASSERT(fScaledHeight > 0);

    fX0 = dx >> 1;
    fY0 = dy >> 1;

    SkASSERT(fX0 >= 0 && fX0 < width);
    SkASSERT(fY0 >= 0 && fY0 < height);

    fDX = dx;
    fDY = dy;

    SkASSERT(fDX > 0 && (fX0 + fDX * (fScaledWidth - 1)) < width);
    SkASSERT(fDY > 0 && (fY0 + fDY * (fScaledHeight - 1)) < height);
}

bool SkScaledBitmapSampler::begin(SkBitmap* dst, SrcConfig sc,
                                  const Options& opts,
                                  const SkPMColor ctable[]) {
    static const RowProcChooser gProcChoosers[] = {
        get_gray_to_8888_proc,
        get_RGBx_to_8888_proc,
        get_RGBA_to_8888_proc,
        get_index_to_8888_proc,
        NULL, 

        get_gray_to_565_proc,
        get_RGBx_to_565_proc,
        get_RGBx_to_565_proc, 
        get_index_to_565_proc,
        get_565_to_565_proc,

        get_gray_to_4444_proc,
        get_RGBx_to_4444_proc,
        get_RGBA_to_4444_proc,
        get_index_to_4444_proc,
        NULL, 

        NULL, 
        NULL, 
        NULL, 
        get_index_to_index_proc,
        NULL, 

        get_gray_to_A8_proc,
        NULL, 
        NULL, 
        NULL, 
        NULL, 
    };

    
    static const int gProcDstConfigSpan = 5;
    SK_COMPILE_ASSERT(SK_ARRAY_COUNT(gProcChoosers) == 5 * gProcDstConfigSpan,
                      gProcs_has_the_wrong_number_of_entries);

    fCTable = ctable;

    int index = 0;
    switch (sc) {
        case SkScaledBitmapSampler::kGray:
            fSrcPixelSize = 1;
            index += 0;
            break;
        case SkScaledBitmapSampler::kRGB:
            fSrcPixelSize = 3;
            index += 1;
            break;
        case SkScaledBitmapSampler::kRGBX:
            fSrcPixelSize = 4;
            index += 1;
            break;
        case SkScaledBitmapSampler::kRGBA:
            fSrcPixelSize = 4;
            index += 2;
            break;
        case SkScaledBitmapSampler::kIndex:
            fSrcPixelSize = 1;
            index += 3;
            break;
        case SkScaledBitmapSampler::kRGB_565:
            fSrcPixelSize = 2;
            index += 4;
            break;
        default:
            return false;
    }

    switch (dst->colorType()) {
        case kN32_SkColorType:
            index += 0 * gProcDstConfigSpan;
            break;
        case kRGB_565_SkColorType:
            index += 1 * gProcDstConfigSpan;
            break;
        case kARGB_4444_SkColorType:
            index += 2 * gProcDstConfigSpan;
            break;
        case kIndex_8_SkColorType:
            index += 3 * gProcDstConfigSpan;
            break;
        case kAlpha_8_SkColorType:
            index += 4 * gProcDstConfigSpan;
            break;
        default:
            return false;
    }

    RowProcChooser chooser = gProcChoosers[index];
    if (NULL == chooser) {
        fRowProc = NULL;
    } else {
        fRowProc = chooser(opts);
    }
    fDstRow = (char*)dst->getPixels();
    fDstRowBytes = dst->rowBytes();
    fCurrY = 0;
    return fRowProc != NULL;
}

bool SkScaledBitmapSampler::begin(SkBitmap* dst, SrcConfig sc,
                                  const SkImageDecoder& decoder,
                                  const SkPMColor ctable[]) {
    return this->begin(dst, sc, Options(decoder), ctable);
}

bool SkScaledBitmapSampler::next(const uint8_t* SK_RESTRICT src) {
    SkASSERT(kInterlaced_SampleMode != fSampleMode);
    SkDEBUGCODE(fSampleMode = kConsecutive_SampleMode);
    SkASSERT((unsigned)fCurrY < (unsigned)fScaledHeight);

    bool hadAlpha = fRowProc(fDstRow, src + fX0 * fSrcPixelSize, fScaledWidth,
                             fDX * fSrcPixelSize, fCurrY, fCTable);
    fDstRow += fDstRowBytes;
    fCurrY += 1;
    return hadAlpha;
}

bool SkScaledBitmapSampler::sampleInterlaced(const uint8_t* SK_RESTRICT src, int srcY) {
    SkASSERT(kConsecutive_SampleMode != fSampleMode);
    SkDEBUGCODE(fSampleMode = kInterlaced_SampleMode);
    
    
    
    const int srcYMinusY0 = srcY - fY0;
    if (srcYMinusY0 % fDY != 0) {
        
        
        return false;
    }
    
    
    
    
    const int dstY = srcYMinusY0 / fDY;
    SkASSERT(dstY < fScaledHeight);
    char* dstRow = fDstRow + dstY * fDstRowBytes;
    return fRowProc(dstRow, src + fX0 * fSrcPixelSize, fScaledWidth,
                    fDX * fSrcPixelSize, dstY, fCTable);
}

#ifdef SK_DEBUG




class RowProcTester {
public:
    static SkScaledBitmapSampler::RowProc getRowProc(const SkScaledBitmapSampler& sampler) {
        return sampler.fRowProc;
    }
};











SkScaledBitmapSampler::RowProc gTestProcs[] = {
    
    Sample_Gray_DA8,    Sample_Gray_DA8,        NULL,                       NULL,                       
    NULL,               NULL,                   NULL,                       NULL,                       
    Sample_Gray_D565,   Sample_Gray_D565_D,     Sample_Gray_D565,           Sample_Gray_D565_D,         
    Sample_Gray_D4444,  Sample_Gray_D4444_D,    Sample_Gray_D4444,          Sample_Gray_D4444_D,        
    Sample_Gray_D8888,  Sample_Gray_D8888,      Sample_Gray_D8888,          Sample_Gray_D8888,          
    
    NULL,               NULL,                   NULL,                       NULL,                       
    Sample_Index_DI,    Sample_Index_DI,        NULL,                       NULL,                       
    Sample_Index_D565,  Sample_Index_D565_D,    Sample_Index_D565,          Sample_Index_D565_D,        
    Sample_Index_D4444, Sample_Index_D4444_D,   NULL,                       NULL,                       
    Sample_Index_D8888, Sample_Index_D8888,     NULL,                       NULL,                       
    
    NULL,               NULL,                   NULL,                       NULL,                       
    NULL,               NULL,                   NULL,                       NULL,                       
    Sample_RGBx_D565,   Sample_RGBx_D565_D,     Sample_RGBx_D565,           Sample_RGBx_D565_D,         
    Sample_RGBx_D4444,  Sample_RGBx_D4444_D,    Sample_RGBx_D4444,          Sample_RGBx_D4444_D,        
    Sample_RGBx_D8888,  Sample_RGBx_D8888,      Sample_RGBx_D8888,          Sample_RGBx_D8888,          
    
    NULL,               NULL,                   NULL,                       NULL,                       
    NULL,               NULL,                   NULL,                       NULL,                       
    Sample_RGBx_D565,   Sample_RGBx_D565_D,     Sample_RGBx_D565,           Sample_RGBx_D565_D,         
    Sample_RGBx_D4444,  Sample_RGBx_D4444_D,    Sample_RGBx_D4444,          Sample_RGBx_D4444_D,        
    Sample_RGBx_D8888,  Sample_RGBx_D8888,      Sample_RGBx_D8888,          Sample_RGBx_D8888,          
    
    NULL,               NULL,                   NULL,                       NULL,                       
    NULL,               NULL,                   NULL,                       NULL,                       
    Sample_RGBx_D565,   Sample_RGBx_D565_D,     Sample_RGBx_D565,           Sample_RGBx_D565_D,         
    Sample_RGBA_D4444,  Sample_RGBA_D4444_D,    NULL,                       NULL,                       
    Sample_RGBA_D8888,  Sample_RGBA_D8888,      Sample_RGBA_D8888_Unpremul, Sample_RGBA_D8888_Unpremul, 
    
    NULL,               NULL,                   NULL,                       NULL,                       
    NULL,               NULL,                   NULL,                       NULL,                       
    Sample_D565_D565,   Sample_D565_D565,       Sample_D565_D565,           Sample_D565_D565,           
    NULL,               NULL,                   NULL,                       NULL,                       
    NULL,               NULL,                   NULL,                       NULL,                       
};


class DummyDecoder : public SkImageDecoder {
public:
    DummyDecoder() {}
protected:
    virtual bool onDecode(SkStream*, SkBitmap*, SkImageDecoder::Mode) SK_OVERRIDE {
        return false;
    }
};

void test_row_proc_choice();
void test_row_proc_choice() {
    const SkColorType colorTypes[] = {
        kAlpha_8_SkColorType, kIndex_8_SkColorType, kRGB_565_SkColorType, kARGB_4444_SkColorType,
        kN32_SkColorType
    };

    SkBitmap dummyBitmap;
    DummyDecoder dummyDecoder;
    size_t procCounter = 0;
    for (int sc = SkScaledBitmapSampler::kGray; sc <= SkScaledBitmapSampler::kRGB_565; ++sc) {
        for (size_t c = 0; c < SK_ARRAY_COUNT(colorTypes); ++c) {
            for (int unpremul = 0; unpremul <= 1; ++unpremul) {
                for (int dither = 0; dither <= 1; ++dither) {
                    
                    
                    SkScaledBitmapSampler sampler(10, 10, 1);
                    dummyBitmap.setInfo(SkImageInfo::Make(10, 10,
                                                          colorTypes[c], kPremul_SkAlphaType));
                    dummyDecoder.setDitherImage(SkToBool(dither));
                    dummyDecoder.setRequireUnpremultipliedColors(SkToBool(unpremul));
                    sampler.begin(&dummyBitmap, (SkScaledBitmapSampler::SrcConfig) sc,
                                  dummyDecoder);
                    SkScaledBitmapSampler::RowProc expected = gTestProcs[procCounter];
                    SkScaledBitmapSampler::RowProc actual = RowProcTester::getRowProc(sampler);
                    SkASSERT(expected == actual);
                    procCounter++;
                }
            }
        }
    }
    SkASSERT(SK_ARRAY_COUNT(gTestProcs) == procCounter);
}
#endif 
