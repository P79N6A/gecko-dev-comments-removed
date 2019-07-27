






#include "SkImageDecoder.h"
#include "SkImageEncoder.h"
#include "SkColor.h"
#include "SkColorPriv.h"
#include "SkDither.h"
#include "SkMath.h"
#include "SkRTConf.h"
#include "SkScaledBitmapSampler.h"
#include "SkStream.h"
#include "SkTemplates.h"
#include "SkUtils.h"
#include "transform_scanline.h"
extern "C" {
#include "png.h"
}


#ifndef png_infopp_NULL
#define png_infopp_NULL NULL
#endif

#ifndef png_bytepp_NULL
#define png_bytepp_NULL NULL
#endif

#ifndef int_p_NULL
#define int_p_NULL NULL
#endif

#ifndef png_flush_ptr_NULL
#define png_flush_ptr_NULL NULL
#endif

#if defined(SK_DEBUG)
#define DEFAULT_FOR_SUPPRESS_PNG_IMAGE_DECODER_WARNINGS false
#else  
#define DEFAULT_FOR_SUPPRESS_PNG_IMAGE_DECODER_WARNINGS true
#endif  
SK_CONF_DECLARE(bool, c_suppressPNGImageDecoderWarnings,
                "images.png.suppressDecoderWarnings",
                DEFAULT_FOR_SUPPRESS_PNG_IMAGE_DECODER_WARNINGS,
                "Suppress most PNG warnings when calling image decode "
                "functions.");



class SkPNGImageIndex {
public:
    SkPNGImageIndex(SkStreamRewindable* stream, png_structp png_ptr, png_infop info_ptr)
        : fStream(stream)
        , fPng_ptr(png_ptr)
        , fInfo_ptr(info_ptr)
        , fColorType(kUnknown_SkColorType) {
        SkASSERT(stream != NULL);
        stream->ref();
    }
    ~SkPNGImageIndex() {
        if (NULL != fPng_ptr) {
            png_destroy_read_struct(&fPng_ptr, &fInfo_ptr, png_infopp_NULL);
        }
    }

    SkAutoTUnref<SkStreamRewindable>    fStream;
    png_structp                         fPng_ptr;
    png_infop                           fInfo_ptr;
    SkColorType                         fColorType;
};

class SkPNGImageDecoder : public SkImageDecoder {
public:
    SkPNGImageDecoder() {
        fImageIndex = NULL;
    }
    virtual Format getFormat() const SK_OVERRIDE {
        return kPNG_Format;
    }

    virtual ~SkPNGImageDecoder() {
        SkDELETE(fImageIndex);
    }

protected:
#ifdef SK_BUILD_FOR_ANDROID
    virtual bool onBuildTileIndex(SkStreamRewindable *stream, int *width, int *height) SK_OVERRIDE;
    virtual bool onDecodeSubset(SkBitmap* bitmap, const SkIRect& region) SK_OVERRIDE;
#endif
    virtual bool onDecode(SkStream* stream, SkBitmap* bm, Mode) SK_OVERRIDE;

private:
    SkPNGImageIndex* fImageIndex;

    bool onDecodeInit(SkStream* stream, png_structp *png_ptrp, png_infop *info_ptrp);
    bool decodePalette(png_structp png_ptr, png_infop info_ptr,
                       bool * SK_RESTRICT hasAlphap, bool *reallyHasAlphap,
                       SkColorTable **colorTablep);
    bool getBitmapColorType(png_structp, png_infop, SkColorType*, bool* hasAlpha,
                            SkPMColor* theTranspColor);

    typedef SkImageDecoder INHERITED;
};

#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

#define PNG_BYTES_TO_CHECK 4


struct PNGAutoClean {
    PNGAutoClean(png_structp p, png_infop i): png_ptr(p), info_ptr(i) {}
    ~PNGAutoClean() {
        png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
    }
private:
    png_structp png_ptr;
    png_infop info_ptr;
};

static void sk_read_fn(png_structp png_ptr, png_bytep data, png_size_t length) {
    SkStream* sk_stream = (SkStream*) png_get_io_ptr(png_ptr);
    size_t bytes = sk_stream->read(data, length);
    if (bytes != length) {
        png_error(png_ptr, "Read Error!");
    }
}

#ifdef SK_BUILD_FOR_ANDROID
static void sk_seek_fn(png_structp png_ptr, png_uint_32 offset) {
    SkStreamRewindable* sk_stream = (SkStreamRewindable*) png_get_io_ptr(png_ptr);
    if (!sk_stream->rewind()) {
        png_error(png_ptr, "Failed to rewind stream!");
    }
    (void)sk_stream->skip(offset);
}
#endif

static int sk_read_user_chunk(png_structp png_ptr, png_unknown_chunkp chunk) {
    SkImageDecoder::Peeker* peeker =
                    (SkImageDecoder::Peeker*)png_get_user_chunk_ptr(png_ptr);
    
    return peeker->peek((const char*)chunk->name, chunk->data, chunk->size) ?
            1 : -1;
}

static void sk_error_fn(png_structp png_ptr, png_const_charp msg) {
    SkDEBUGF(("------ png error %s\n", msg));
    longjmp(png_jmpbuf(png_ptr), 1);
}

static void skip_src_rows(png_structp png_ptr, uint8_t storage[], int count) {
    for (int i = 0; i < count; i++) {
        uint8_t* tmp = storage;
        png_read_rows(png_ptr, &tmp, png_bytepp_NULL, 1);
    }
}

static bool pos_le(int value, int max) {
    return value > 0 && value <= max;
}

static bool substituteTranspColor(SkBitmap* bm, SkPMColor match) {
    SkASSERT(bm->colorType() == kN32_SkColorType);

    bool reallyHasAlpha = false;

    for (int y = bm->height() - 1; y >= 0; --y) {
        SkPMColor* p = bm->getAddr32(0, y);
        for (int x = bm->width() - 1; x >= 0; --x) {
            if (match == *p) {
                *p = 0;
                reallyHasAlpha = true;
            }
            p += 1;
        }
    }
    return reallyHasAlpha;
}

static bool canUpscalePaletteToConfig(SkColorType dstColorType, bool srcHasAlpha) {
    switch (dstColorType) {
        case kN32_SkColorType:
        case kARGB_4444_SkColorType:
            return true;
        case kRGB_565_SkColorType:
            
            return !srcHasAlpha;
        default:
            return false;
    }
}


static bool hasTransparencyInPalette(png_structp png_ptr, png_infop info_ptr) {
    png_bytep trans;
    int num_trans;

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, NULL);
        return num_trans > 0;
    }
    return false;
}

void do_nothing_warning_fn(png_structp, png_const_charp) {
    
}

bool SkPNGImageDecoder::onDecodeInit(SkStream* sk_stream, png_structp *png_ptrp,
                                     png_infop *info_ptrp) {
    





    png_error_ptr user_warning_fn =
        (c_suppressPNGImageDecoderWarnings) ? (&do_nothing_warning_fn) : NULL;
    
    
    




    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
        NULL, sk_error_fn, user_warning_fn);
    
    if (png_ptr == NULL) {
        return false;
    }

    *png_ptrp = png_ptr;

    
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
        return false;
    }
    *info_ptrp = info_ptr;

    



    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
        return false;
    }

    


    png_set_read_fn(png_ptr, (void *)sk_stream, sk_read_fn);
#ifdef SK_BUILD_FOR_ANDROID
    png_set_seek_fn(png_ptr, sk_seek_fn);
#endif
    
    


    
    png_set_keep_unknown_chunks(png_ptr, PNG_HANDLE_CHUNK_ALWAYS, (png_byte*)"", 0);
    if (this->getPeeker()) {
        png_set_read_user_chunk_fn(png_ptr, (png_voidp)this->getPeeker(), sk_read_user_chunk);
    }

    

    png_read_info(png_ptr, info_ptr);
    png_uint_32 origWidth, origHeight;
    int bitDepth, colorType;
    png_get_IHDR(png_ptr, info_ptr, &origWidth, &origHeight, &bitDepth,
                 &colorType, int_p_NULL, int_p_NULL, int_p_NULL);

    
    if (bitDepth == 16) {
        png_set_strip_16(png_ptr);
    }
    

    if (bitDepth < 8) {
        png_set_packing(png_ptr);
    }
    
    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) {
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    }

    return true;
}

bool SkPNGImageDecoder::onDecode(SkStream* sk_stream, SkBitmap* decodedBitmap,
                                 Mode mode) {
    png_structp png_ptr;
    png_infop info_ptr;

    if (!onDecodeInit(sk_stream, &png_ptr, &info_ptr)) {
        return false;
    }

    PNGAutoClean autoClean(png_ptr, info_ptr);

    if (setjmp(png_jmpbuf(png_ptr))) {
        return false;
    }

    png_uint_32 origWidth, origHeight;
    int bitDepth, pngColorType, interlaceType;
    png_get_IHDR(png_ptr, info_ptr, &origWidth, &origHeight, &bitDepth,
                 &pngColorType, &interlaceType, int_p_NULL, int_p_NULL);

    SkColorType         colorType;
    bool                hasAlpha = false;
    SkPMColor           theTranspColor = 0; 

    if (!this->getBitmapColorType(png_ptr, info_ptr, &colorType, &hasAlpha, &theTranspColor)) {
        return false;
    }

    SkAlphaType alphaType = this->getRequireUnpremultipliedColors() ?
                                kUnpremul_SkAlphaType : kPremul_SkAlphaType;
    const int sampleSize = this->getSampleSize();
    SkScaledBitmapSampler sampler(origWidth, origHeight, sampleSize);
    decodedBitmap->setInfo(SkImageInfo::Make(sampler.scaledWidth(), sampler.scaledHeight(),
                                             colorType, alphaType));

    if (SkImageDecoder::kDecodeBounds_Mode == mode) {
        return true;
    }

    

    
    
    
    bool reallyHasAlpha = false;
    SkColorTable* colorTable = NULL;

    if (pngColorType == PNG_COLOR_TYPE_PALETTE) {
        decodePalette(png_ptr, info_ptr, &hasAlpha, &reallyHasAlpha, &colorTable);
    }

    SkAutoUnref aur(colorTable);

    if (!this->allocPixelRef(decodedBitmap,
                             kIndex_8_SkColorType == colorType ? colorTable : NULL)) {
        return false;
    }

    SkAutoLockPixels alp(*decodedBitmap);

    



    const int number_passes = (interlaceType != PNG_INTERLACE_NONE) ?
                              png_set_interlace_handling(png_ptr) : 1;

    



    png_read_update_info(png_ptr, info_ptr);

    if ((kAlpha_8_SkColorType == colorType || kIndex_8_SkColorType == colorType) &&
            1 == sampleSize) {
        if (kAlpha_8_SkColorType == colorType) {
            
            
            
            reallyHasAlpha = true;
            
            SkASSERT(PNG_COLOR_TYPE_GRAY == pngColorType);
        }
        for (int i = 0; i < number_passes; i++) {
            for (png_uint_32 y = 0; y < origHeight; y++) {
                uint8_t* bmRow = decodedBitmap->getAddr8(0, y);
                png_read_rows(png_ptr, &bmRow, png_bytepp_NULL, 1);
            }
        }
    } else {
        SkScaledBitmapSampler::SrcConfig sc;
        int srcBytesPerPixel = 4;

        if (colorTable != NULL) {
            sc = SkScaledBitmapSampler::kIndex;
            srcBytesPerPixel = 1;
        } else if (kAlpha_8_SkColorType == colorType) {
            
            SkASSERT(PNG_COLOR_TYPE_GRAY == pngColorType);
            sc = SkScaledBitmapSampler::kGray;
            srcBytesPerPixel = 1;
        } else if (hasAlpha) {
            sc = SkScaledBitmapSampler::kRGBA;
        } else {
            sc = SkScaledBitmapSampler::kRGBX;
        }

        



        SkAutoLockColors ctLock(colorTable);
        if (!sampler.begin(decodedBitmap, sc, *this, ctLock.colors())) {
            return false;
        }
        const int height = decodedBitmap->height();

        if (number_passes > 1) {
            SkAutoMalloc storage(origWidth * origHeight * srcBytesPerPixel);
            uint8_t* base = (uint8_t*)storage.get();
            size_t rowBytes = origWidth * srcBytesPerPixel;

            for (int i = 0; i < number_passes; i++) {
                uint8_t* row = base;
                for (png_uint_32 y = 0; y < origHeight; y++) {
                    uint8_t* bmRow = row;
                    png_read_rows(png_ptr, &bmRow, png_bytepp_NULL, 1);
                    row += rowBytes;
                }
            }
            
            base += sampler.srcY0() * rowBytes;
            for (int y = 0; y < height; y++) {
                reallyHasAlpha |= sampler.next(base);
                base += sampler.srcDY() * rowBytes;
            }
        } else {
            SkAutoMalloc storage(origWidth * srcBytesPerPixel);
            uint8_t* srcRow = (uint8_t*)storage.get();
            skip_src_rows(png_ptr, srcRow, sampler.srcY0());

            for (int y = 0; y < height; y++) {
                uint8_t* tmp = srcRow;
                png_read_rows(png_ptr, &tmp, png_bytepp_NULL, 1);
                reallyHasAlpha |= sampler.next(srcRow);
                if (y < height - 1) {
                    skip_src_rows(png_ptr, srcRow, sampler.srcDY() - 1);
                }
            }

            
            png_uint_32 read = (height - 1) * sampler.srcDY() +
                               sampler.srcY0() + 1;
            SkASSERT(read <= origHeight);
            skip_src_rows(png_ptr, srcRow, origHeight - read);
        }
    }

    
    png_read_end(png_ptr, info_ptr);

    if (0 != theTranspColor) {
        reallyHasAlpha |= substituteTranspColor(decodedBitmap, theTranspColor);
    }
    if (reallyHasAlpha && this->getRequireUnpremultipliedColors()) {
        switch (decodedBitmap->colorType()) {
            case kIndex_8_SkColorType:
                
            case kARGB_4444_SkColorType:
                
                return false;
            default: {
                
                
                
                
            }
        }
    }

    if (!reallyHasAlpha) {
        decodedBitmap->setAlphaType(kOpaque_SkAlphaType);
    }
    return true;
}



bool SkPNGImageDecoder::getBitmapColorType(png_structp png_ptr, png_infop info_ptr,
                                           SkColorType* colorTypep,
                                           bool* hasAlphap,
                                           SkPMColor* SK_RESTRICT theTranspColorp) {
    png_uint_32 origWidth, origHeight;
    int bitDepth, colorType;
    png_get_IHDR(png_ptr, info_ptr, &origWidth, &origHeight, &bitDepth,
                 &colorType, int_p_NULL, int_p_NULL, int_p_NULL);

    
    
    png_color_8p sig_bit;
    if (this->getDitherImage() && png_get_sBIT(png_ptr, info_ptr, &sig_bit)) {
#if 0
        SkDebugf("----- sBIT %d %d %d %d\n", sig_bit->red, sig_bit->green,
                 sig_bit->blue, sig_bit->alpha);
#endif
        
        if (pos_le(sig_bit->red, SK_R16_BITS) &&
            pos_le(sig_bit->green, SK_G16_BITS) &&
            pos_le(sig_bit->blue, SK_B16_BITS)) {
            this->setDitherImage(false);
        }
    }

    if (colorType == PNG_COLOR_TYPE_PALETTE) {
        bool paletteHasAlpha = hasTransparencyInPalette(png_ptr, info_ptr);
        *colorTypep = this->getPrefColorType(kIndex_SrcDepth, paletteHasAlpha);
        
        if (!canUpscalePaletteToConfig(*colorTypep, paletteHasAlpha)) {
            *colorTypep = kIndex_8_SkColorType;
        }
    } else {
        png_color_16p transpColor = NULL;
        int numTransp = 0;

        png_get_tRNS(png_ptr, info_ptr, NULL, &numTransp, &transpColor);

        bool valid = png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS);

        if (valid && numTransp == 1 && transpColor != NULL) {
            






            if (colorType & PNG_COLOR_MASK_COLOR) {
                if (16 == bitDepth) {
                    *theTranspColorp = SkPackARGB32(0xFF, transpColor->red >> 8,
                                                    transpColor->green >> 8,
                                                    transpColor->blue >> 8);
                } else {
                    


                    *theTranspColorp = SkPackARGB32(0xFF,
                                                    0xFF & (transpColor->red),
                                                    0xFF & (transpColor->green),
                                                    0xFF & (transpColor->blue));
                }
            } else {    
                if (16 == bitDepth) {
                    *theTranspColorp = SkPackARGB32(0xFF, transpColor->gray >> 8,
                                                    transpColor->gray >> 8,
                                                    transpColor->gray >> 8);
                } else {
                    




                    *theTranspColorp = SkPackARGB32(0xFF,
                                                    0xFF & (transpColor->gray),
                                                    0xFF & (transpColor->gray),
                                                    0xFF & (transpColor->gray));
                }
            }
        }

        if (valid ||
            PNG_COLOR_TYPE_RGB_ALPHA == colorType ||
            PNG_COLOR_TYPE_GRAY_ALPHA == colorType) {
            *hasAlphap = true;
        }

        SrcDepth srcDepth = k32Bit_SrcDepth;
        if (PNG_COLOR_TYPE_GRAY == colorType) {
            srcDepth = k8BitGray_SrcDepth;
            
            
        }

        *colorTypep = this->getPrefColorType(srcDepth, *hasAlphap);
        
        if (*hasAlphap) {
            if (*colorTypep != kARGB_4444_SkColorType) {
                *colorTypep = kN32_SkColorType;
            }
        } else {
            if (kAlpha_8_SkColorType == *colorTypep) {
                if (k8BitGray_SrcDepth != srcDepth) {
                    
                    *colorTypep = kN32_SkColorType;
                }
            } else if (*colorTypep != kRGB_565_SkColorType &&
                       *colorTypep != kARGB_4444_SkColorType) {
                *colorTypep = kN32_SkColorType;
            }
        }
    }

    
    {
        int64_t size = sk_64_mul(origWidth, origHeight);
        
        if (size < 0 || size > (0x7FFFFFFF >> 2)) {
            return false;
        }
    }

#ifdef SK_SUPPORT_LEGACY_IMAGEDECODER_CHOOSER
    if (!this->chooseFromOneChoice(*colorTypep, origWidth, origHeight)) {
        return false;
    }
#endif

    
    
    if (this->getRequireUnpremultipliedColors() && *hasAlphap) {
        *colorTypep = kN32_SkColorType;
    }

    if (fImageIndex != NULL) {
        if (kUnknown_SkColorType == fImageIndex->fColorType) {
            
            
            fImageIndex->fColorType = *colorTypep;
        } else if (fImageIndex->fColorType != *colorTypep) {
            
            
            return false;
        }
    }

    bool convertGrayToRGB = PNG_COLOR_TYPE_GRAY == colorType && *colorTypep != kAlpha_8_SkColorType;

    
    
    if (convertGrayToRGB || colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png_ptr);
    }

    
    if (colorType == PNG_COLOR_TYPE_RGB || convertGrayToRGB) {
        png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
    }

    return true;
}

typedef uint32_t (*PackColorProc)(U8CPU a, U8CPU r, U8CPU g, U8CPU b);

bool SkPNGImageDecoder::decodePalette(png_structp png_ptr, png_infop info_ptr,
                                      bool *hasAlphap, bool *reallyHasAlphap,
                                      SkColorTable **colorTablep) {
    int numPalette;
    png_colorp palette;
    png_bytep trans;
    int numTrans;

    png_get_PLTE(png_ptr, info_ptr, &palette, &numPalette);

    





    int colorCount = numPalette + (numPalette < 256);
    SkPMColor colorStorage[256];    
    SkPMColor* colorPtr = colorStorage;

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        png_get_tRNS(png_ptr, info_ptr, &trans, &numTrans, NULL);
        *hasAlphap = (numTrans > 0);
    } else {
        numTrans = 0;
    }

    
    if (numTrans > numPalette) {
        numTrans = numPalette;
    }

    int index = 0;
    int transLessThanFF = 0;

    
    
    PackColorProc proc;
    if (this->getRequireUnpremultipliedColors()) {
        proc = &SkPackARGB32NoCheck;
    } else {
        proc = &SkPreMultiplyARGB;
    }
    for (; index < numTrans; index++) {
        transLessThanFF |= (int)*trans - 0xFF;
        *colorPtr++ = proc(*trans++, palette->red, palette->green, palette->blue);
        palette++;
    }
    bool reallyHasAlpha = (transLessThanFF < 0);

    for (; index < numPalette; index++) {
        *colorPtr++ = SkPackARGB32(0xFF, palette->red, palette->green, palette->blue);
        palette++;
    }

    
    if (numPalette < 256) {
        *colorPtr = colorPtr[-1];
    }

    SkAlphaType alphaType = kOpaque_SkAlphaType;
    if (reallyHasAlpha) {
        if (this->getRequireUnpremultipliedColors()) {
            alphaType = kUnpremul_SkAlphaType;
        } else {
            alphaType = kPremul_SkAlphaType;
        }
    }

    *colorTablep = SkNEW_ARGS(SkColorTable,
                              (colorStorage, colorCount, alphaType));
    *reallyHasAlphap = reallyHasAlpha;
    return true;
}

#ifdef SK_BUILD_FOR_ANDROID

bool SkPNGImageDecoder::onBuildTileIndex(SkStreamRewindable* sk_stream, int *width, int *height) {
    png_structp png_ptr;
    png_infop   info_ptr;

    if (!onDecodeInit(sk_stream, &png_ptr, &info_ptr)) {
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr)) != 0) {
        png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
        return false;
    }

    png_uint_32 origWidth, origHeight;
    int bitDepth, colorType;
    png_get_IHDR(png_ptr, info_ptr, &origWidth, &origHeight, &bitDepth,
                 &colorType, int_p_NULL, int_p_NULL, int_p_NULL);

    *width = origWidth;
    *height = origHeight;

    png_build_index(png_ptr);

    if (fImageIndex) {
        SkDELETE(fImageIndex);
    }
    fImageIndex = SkNEW_ARGS(SkPNGImageIndex, (sk_stream, png_ptr, info_ptr));

    return true;
}

bool SkPNGImageDecoder::onDecodeSubset(SkBitmap* bm, const SkIRect& region) {
    if (NULL == fImageIndex) {
        return false;
    }

    png_structp png_ptr = fImageIndex->fPng_ptr;
    png_infop info_ptr = fImageIndex->fInfo_ptr;
    if (setjmp(png_jmpbuf(png_ptr))) {
        return false;
    }

    png_uint_32 origWidth, origHeight;
    int bitDepth, pngColorType, interlaceType;
    png_get_IHDR(png_ptr, info_ptr, &origWidth, &origHeight, &bitDepth,
                 &pngColorType, &interlaceType, int_p_NULL, int_p_NULL);

    SkIRect rect = SkIRect::MakeWH(origWidth, origHeight);

    if (!rect.intersect(region)) {
        
        
        return false;
    }

    SkColorType         colorType;
    bool                hasAlpha = false;
    SkPMColor           theTranspColor = 0; 

    if (!this->getBitmapColorType(png_ptr, info_ptr, &colorType, &hasAlpha, &theTranspColor)) {
        return false;
    }

    const int sampleSize = this->getSampleSize();
    SkScaledBitmapSampler sampler(origWidth, rect.height(), sampleSize);

    SkBitmap decodedBitmap;
    decodedBitmap.setInfo(SkImageInfo::Make(sampler.scaledWidth(), sampler.scaledHeight(),
                                            colorType, kPremul_SkAlphaType));

    

    
    
    
    bool reallyHasAlpha = false;
    SkColorTable* colorTable = NULL;

    if (pngColorType == PNG_COLOR_TYPE_PALETTE) {
        decodePalette(png_ptr, info_ptr, &hasAlpha, &reallyHasAlpha, &colorTable);
    }

    SkAutoUnref aur(colorTable);

    
    
    
    int w = rect.width() / sampleSize;
    int h = rect.height() / sampleSize;
    const bool swapOnly = (rect == region) && (w == decodedBitmap.width()) &&
                          (h == decodedBitmap.height()) && bm->isNull();
    const bool needColorTable = kIndex_8_SkColorType == colorType;
    if (swapOnly) {
        if (!this->allocPixelRef(&decodedBitmap, needColorTable ? colorTable : NULL)) {
            return false;
        }
    } else {
        if (!decodedBitmap.allocPixels(NULL, needColorTable ? colorTable : NULL)) {
            return false;
        }
    }
    SkAutoLockPixels alp(decodedBitmap);

    



    const int number_passes = (interlaceType != PNG_INTERLACE_NONE) ?
                              png_set_interlace_handling(png_ptr) : 1;

    




    
#if defined(PNG_1_0_X) || defined (PNG_1_2_X)
    png_ptr->pass = 0;
#else
    
    png_set_interlaced_pass(png_ptr, 0);
#endif
    png_read_update_info(png_ptr, info_ptr);

    int actualTop = rect.fTop;

    if ((kAlpha_8_SkColorType == colorType || kIndex_8_SkColorType == colorType)
        && 1 == sampleSize) {
        if (kAlpha_8_SkColorType == colorType) {
            
            
            
            reallyHasAlpha = true;
            
            SkASSERT(PNG_COLOR_TYPE_GRAY == pngColorType);
        }

        for (int i = 0; i < number_passes; i++) {
            png_configure_decoder(png_ptr, &actualTop, i);
            for (int j = 0; j < rect.fTop - actualTop; j++) {
                uint8_t* bmRow = decodedBitmap.getAddr8(0, 0);
                png_read_rows(png_ptr, &bmRow, png_bytepp_NULL, 1);
            }
            png_uint_32 bitmapHeight = (png_uint_32) decodedBitmap.height();
            for (png_uint_32 y = 0; y < bitmapHeight; y++) {
                uint8_t* bmRow = decodedBitmap.getAddr8(0, y);
                png_read_rows(png_ptr, &bmRow, png_bytepp_NULL, 1);
            }
        }
    } else {
        SkScaledBitmapSampler::SrcConfig sc;
        int srcBytesPerPixel = 4;

        if (colorTable != NULL) {
            sc = SkScaledBitmapSampler::kIndex;
            srcBytesPerPixel = 1;
        } else if (kAlpha_8_SkColorType == colorType) {
            
            SkASSERT(PNG_COLOR_TYPE_GRAY == pngColorType);
            sc = SkScaledBitmapSampler::kGray;
            srcBytesPerPixel = 1;
        } else if (hasAlpha) {
            sc = SkScaledBitmapSampler::kRGBA;
        } else {
            sc = SkScaledBitmapSampler::kRGBX;
        }

        



        SkAutoLockColors ctLock(colorTable);
        if (!sampler.begin(&decodedBitmap, sc, *this, ctLock.colors())) {
            return false;
        }
        const int height = decodedBitmap.height();

        if (number_passes > 1) {
            SkAutoMalloc storage(origWidth * origHeight * srcBytesPerPixel);
            uint8_t* base = (uint8_t*)storage.get();
            size_t rb = origWidth * srcBytesPerPixel;

            for (int i = 0; i < number_passes; i++) {
                png_configure_decoder(png_ptr, &actualTop, i);
                for (int j = 0; j < rect.fTop - actualTop; j++) {
                    png_read_rows(png_ptr, &base, png_bytepp_NULL, 1);
                }
                uint8_t* row = base;
                for (int32_t y = 0; y < rect.height(); y++) {
                    uint8_t* bmRow = row;
                    png_read_rows(png_ptr, &bmRow, png_bytepp_NULL, 1);
                    row += rb;
                }
            }
            
            base += sampler.srcY0() * rb;
            for (int y = 0; y < height; y++) {
                reallyHasAlpha |= sampler.next(base);
                base += sampler.srcDY() * rb;
            }
        } else {
            SkAutoMalloc storage(origWidth * srcBytesPerPixel);
            uint8_t* srcRow = (uint8_t*)storage.get();

            png_configure_decoder(png_ptr, &actualTop, 0);
            skip_src_rows(png_ptr, srcRow, sampler.srcY0());

            for (int i = 0; i < rect.fTop - actualTop; i++) {
                png_read_rows(png_ptr, &srcRow, png_bytepp_NULL, 1);
            }
            for (int y = 0; y < height; y++) {
                uint8_t* tmp = srcRow;
                png_read_rows(png_ptr, &tmp, png_bytepp_NULL, 1);
                reallyHasAlpha |= sampler.next(srcRow);
                if (y < height - 1) {
                    skip_src_rows(png_ptr, srcRow, sampler.srcDY() - 1);
                }
            }
        }
    }

    if (0 != theTranspColor) {
        reallyHasAlpha |= substituteTranspColor(&decodedBitmap, theTranspColor);
    }
    if (reallyHasAlpha && this->getRequireUnpremultipliedColors()) {
        switch (decodedBitmap.colorType()) {
            case kIndex_8_SkColorType:
                
            case kARGB_4444_SkColorType:
                
                return false;
            default: {
                
                
                
                
            }
        }
    }
    SkAlphaType alphaType = kOpaque_SkAlphaType;
    if (reallyHasAlpha) {
        if (this->getRequireUnpremultipliedColors()) {
            alphaType = kUnpremul_SkAlphaType;
        } else {
            alphaType = kPremul_SkAlphaType;
        }
    }
    decodedBitmap.setAlphaType(alphaType);

    if (swapOnly) {
        bm->swap(decodedBitmap);
        return true;
    }
    return this->cropBitmap(bm, &decodedBitmap, sampleSize, region.x(), region.y(),
                            region.width(), region.height(), 0, rect.y());
}
#endif



#include "SkColorPriv.h"
#include "SkUnPreMultiply.h"

static void sk_write_fn(png_structp png_ptr, png_bytep data, png_size_t len) {
    SkWStream* sk_stream = (SkWStream*)png_get_io_ptr(png_ptr);
    if (!sk_stream->write(data, len)) {
        png_error(png_ptr, "sk_write_fn Error!");
    }
}

static transform_scanline_proc choose_proc(SkColorType ct, bool hasAlpha) {
    
    
    if (kIndex_8_SkColorType == ct) {
        hasAlpha = false;   
    }

    static const struct {
        SkColorType             fColorType;
        bool                    fHasAlpha;
        transform_scanline_proc fProc;
    } gMap[] = {
        { kRGB_565_SkColorType,     false,  transform_scanline_565 },
        { kN32_SkColorType,         false,  transform_scanline_888 },
        { kN32_SkColorType,         true,   transform_scanline_8888 },
        { kARGB_4444_SkColorType,   false,  transform_scanline_444 },
        { kARGB_4444_SkColorType,   true,   transform_scanline_4444 },
        { kIndex_8_SkColorType,     false,  transform_scanline_memcpy },
    };

    for (int i = SK_ARRAY_COUNT(gMap) - 1; i >= 0; --i) {
        if (gMap[i].fColorType == ct && gMap[i].fHasAlpha == hasAlpha) {
            return gMap[i].fProc;
        }
    }
    sk_throw();
    return NULL;
}




static int computeBitDepth(int colorCount) {
#if 0
    int bits = SkNextLog2(colorCount);
    SkASSERT(bits >= 1 && bits <= 8);
    
    return SkNextPow2(bits);
#else
    
    return 8;
#endif
}








static inline int pack_palette(SkColorTable* ctable,
                               png_color* SK_RESTRICT palette,
                               png_byte* SK_RESTRICT trans, bool hasAlpha) {
    SkAutoLockColors alc(ctable);
    const SkPMColor* SK_RESTRICT colors = alc.colors();
    const int ctCount = ctable->count();
    int i, num_trans = 0;

    if (hasAlpha) {
        







        num_trans = ctCount;
        for (i = ctCount - 1; i >= 0; --i) {
            if (SkGetPackedA32(colors[i]) != 0xFF) {
                break;
            }
            num_trans -= 1;
        }

        const SkUnPreMultiply::Scale* SK_RESTRICT table =
                                            SkUnPreMultiply::GetScaleTable();

        for (i = 0; i < num_trans; i++) {
            const SkPMColor c = *colors++;
            const unsigned a = SkGetPackedA32(c);
            const SkUnPreMultiply::Scale s = table[a];
            trans[i] = a;
            palette[i].red = SkUnPreMultiply::ApplyScale(s, SkGetPackedR32(c));
            palette[i].green = SkUnPreMultiply::ApplyScale(s,SkGetPackedG32(c));
            palette[i].blue = SkUnPreMultiply::ApplyScale(s, SkGetPackedB32(c));
        }
        
        
    }

    
    for (i = num_trans; i < ctCount; i++) {
        SkPMColor c = *colors++;
        palette[i].red = SkGetPackedR32(c);
        palette[i].green = SkGetPackedG32(c);
        palette[i].blue = SkGetPackedB32(c);
    }
    return num_trans;
}

class SkPNGImageEncoder : public SkImageEncoder {
protected:
    virtual bool onEncode(SkWStream* stream, const SkBitmap& bm, int quality) SK_OVERRIDE;
private:
    bool doEncode(SkWStream* stream, const SkBitmap& bm,
                  const bool& hasAlpha, int colorType,
                  int bitDepth, SkColorType ct,
                  png_color_8& sig_bit);

    typedef SkImageEncoder INHERITED;
};

bool SkPNGImageEncoder::onEncode(SkWStream* stream, const SkBitmap& bitmap, int ) {
    SkColorType ct = bitmap.colorType();

    const bool hasAlpha = !bitmap.isOpaque();
    int colorType = PNG_COLOR_MASK_COLOR;
    int bitDepth = 8;   
    png_color_8 sig_bit;

    switch (ct) {
        case kIndex_8_SkColorType:
            colorType |= PNG_COLOR_MASK_PALETTE;
            
        case kN32_SkColorType:
            sig_bit.red = 8;
            sig_bit.green = 8;
            sig_bit.blue = 8;
            sig_bit.alpha = 8;
            break;
        case kARGB_4444_SkColorType:
            sig_bit.red = 4;
            sig_bit.green = 4;
            sig_bit.blue = 4;
            sig_bit.alpha = 4;
            break;
        case kRGB_565_SkColorType:
            sig_bit.red = 5;
            sig_bit.green = 6;
            sig_bit.blue = 5;
            sig_bit.alpha = 0;
            break;
        default:
            return false;
    }

    if (hasAlpha) {
        
        if (!(colorType & PNG_COLOR_MASK_PALETTE)) {
            colorType |= PNG_COLOR_MASK_ALPHA;
        }
    } else {
        sig_bit.alpha = 0;
    }

    SkAutoLockPixels alp(bitmap);
    
    if (!bitmap.readyToDraw()) {
        return false;
    }

    
    SkColorTable* ctable = bitmap.getColorTable();
    if (NULL != ctable) {
        if (ctable->count() == 0) {
            return false;
        }
        
        bitDepth = computeBitDepth(ctable->count());
    }

    return doEncode(stream, bitmap, hasAlpha, colorType, bitDepth, ct, sig_bit);
}

bool SkPNGImageEncoder::doEncode(SkWStream* stream, const SkBitmap& bitmap,
                  const bool& hasAlpha, int colorType,
                  int bitDepth, SkColorType ct,
                  png_color_8& sig_bit) {

    png_structp png_ptr;
    png_infop info_ptr;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, sk_error_fn,
                                      NULL);
    if (NULL == png_ptr) {
        return false;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (NULL == info_ptr) {
        png_destroy_write_struct(&png_ptr,  png_infopp_NULL);
        return false;
    }

    


    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return false;
    }

    png_set_write_fn(png_ptr, (void*)stream, sk_write_fn, png_flush_ptr_NULL);

    








    png_set_IHDR(png_ptr, info_ptr, bitmap.width(), bitmap.height(),
                 bitDepth, colorType,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
                 PNG_FILTER_TYPE_BASE);

    
    png_color paletteColors[256];
    png_byte trans[256];
    if (kIndex_8_SkColorType == ct) {
        SkColorTable* ct = bitmap.getColorTable();
        int numTrans = pack_palette(ct, paletteColors, trans, hasAlpha);
        png_set_PLTE(png_ptr, info_ptr, paletteColors, ct->count());
        if (numTrans > 0) {
            png_set_tRNS(png_ptr, info_ptr, trans, numTrans, NULL);
        }
    }

    png_set_sBIT(png_ptr, info_ptr, &sig_bit);
    png_write_info(png_ptr, info_ptr);

    const char* srcImage = (const char*)bitmap.getPixels();
    SkAutoSMalloc<1024> rowStorage(bitmap.width() << 2);
    char* storage = (char*)rowStorage.get();
    transform_scanline_proc proc = choose_proc(ct, hasAlpha);

    for (int y = 0; y < bitmap.height(); y++) {
        png_bytep row_ptr = (png_bytep)storage;
        proc(srcImage, bitmap.width(), storage);
        png_write_rows(png_ptr, &row_ptr, 1);
        srcImage += bitmap.rowBytes();
    }

    png_write_end(png_ptr, info_ptr);

    
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return true;
}


DEFINE_DECODER_CREATOR(PNGImageDecoder);
DEFINE_ENCODER_CREATOR(PNGImageEncoder);


static bool is_png(SkStreamRewindable* stream) {
    char buf[PNG_BYTES_TO_CHECK];
    if (stream->read(buf, PNG_BYTES_TO_CHECK) == PNG_BYTES_TO_CHECK &&
        !png_sig_cmp((png_bytep) buf, (png_size_t)0, PNG_BYTES_TO_CHECK)) {
        return true;
    }
    return false;
}

SkImageDecoder* sk_libpng_dfactory(SkStreamRewindable* stream) {
    if (is_png(stream)) {
        return SkNEW(SkPNGImageDecoder);
    }
    return NULL;
}

static SkImageDecoder::Format get_format_png(SkStreamRewindable* stream) {
    if (is_png(stream)) {
        return SkImageDecoder::kPNG_Format;
    }
    return SkImageDecoder::kUnknown_Format;
}

SkImageEncoder* sk_libpng_efactory(SkImageEncoder::Type t) {
    return (SkImageEncoder::kPNG_Type == t) ? SkNEW(SkPNGImageEncoder) : NULL;
}

static SkImageDecoder_DecodeReg gDReg(sk_libpng_dfactory);
static SkImageDecoder_FormatReg gFormatReg(get_format_png);
static SkImageEncoder_EncodeReg gEReg(sk_libpng_efactory);
