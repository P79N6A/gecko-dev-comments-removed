







#include "SkImageDecoder.h"
#include "SkBitmap.h"
#include "SkImagePriv.h"
#include "SkPixelRef.h"
#include "SkStream.h"
#include "SkTemplates.h"
#include "SkCanvas.h"

SkImageDecoder::SkImageDecoder()
    : fPeeker(NULL)
#ifdef SK_SUPPORT_LEGACY_IMAGEDECODER_CHOOSER
    , fChooser(NULL)
#endif
    , fAllocator(NULL)
    , fSampleSize(1)
    , fDefaultPref(kUnknown_SkColorType)
    , fPreserveSrcDepth(false)
    , fDitherImage(true)
#ifdef SK_SUPPORT_LEGACY_BITMAP_CONFIG
    , fUsePrefTable(false)
#endif
    , fSkipWritingZeroes(false)
    , fPreferQualityOverSpeed(false)
    , fRequireUnpremultipliedColors(false) {
}

SkImageDecoder::~SkImageDecoder() {
    SkSafeUnref(fPeeker);
#ifdef SK_SUPPORT_LEGACY_IMAGEDECODER_CHOOSER
    SkSafeUnref(fChooser);
#endif
    SkSafeUnref(fAllocator);
}

void SkImageDecoder::copyFieldsToOther(SkImageDecoder* other) {
    if (NULL == other) {
        return;
    }
    other->setPeeker(fPeeker);
#ifdef SK_SUPPORT_LEGACY_IMAGEDECODER_CHOOSER
    other->setChooser(fChooser);
#endif
    other->setAllocator(fAllocator);
    other->setSampleSize(fSampleSize);
#ifdef SK_SUPPORT_LEGACY_BITMAP_CONFIG
    if (fUsePrefTable) {
        other->setPrefConfigTable(fPrefTable);
    } else {
        other->fDefaultPref = fDefaultPref;
    }
#endif
    other->setPreserveSrcDepth(fPreserveSrcDepth);
    other->setDitherImage(fDitherImage);
    other->setSkipWritingZeroes(fSkipWritingZeroes);
    other->setPreferQualityOverSpeed(fPreferQualityOverSpeed);
    other->setRequireUnpremultipliedColors(fRequireUnpremultipliedColors);
}

SkImageDecoder::Format SkImageDecoder::getFormat() const {
    return kUnknown_Format;
}

const char* SkImageDecoder::getFormatName() const {
    return GetFormatName(this->getFormat());
}

const char* SkImageDecoder::GetFormatName(Format format) {
    switch (format) {
        case kUnknown_Format:
            return "Unknown Format";
        case kBMP_Format:
            return "BMP";
        case kGIF_Format:
            return "GIF";
        case kICO_Format:
            return "ICO";
        case kPKM_Format:
            return "PKM";
        case kKTX_Format:
            return "KTX";
        case kJPEG_Format:
            return "JPEG";
        case kPNG_Format:
            return "PNG";
        case kWBMP_Format:
            return "WBMP";
        case kWEBP_Format:
            return "WEBP";
        default:
            SkDEBUGFAIL("Invalid format type!");
    }
    return "Unknown Format";
}

SkImageDecoder::Peeker* SkImageDecoder::setPeeker(Peeker* peeker) {
    SkRefCnt_SafeAssign(fPeeker, peeker);
    return peeker;
}

#ifdef SK_SUPPORT_LEGACY_IMAGEDECODER_CHOOSER
SkImageDecoder::Chooser* SkImageDecoder::setChooser(Chooser* chooser) {
    SkRefCnt_SafeAssign(fChooser, chooser);
    return chooser;
}
#endif

SkBitmap::Allocator* SkImageDecoder::setAllocator(SkBitmap::Allocator* alloc) {
    SkRefCnt_SafeAssign(fAllocator, alloc);
    return alloc;
}

void SkImageDecoder::setSampleSize(int size) {
    if (size < 1) {
        size = 1;
    }
    fSampleSize = size;
}

#ifdef SK_SUPPORT_LEGACY_IMAGEDECODER_CHOOSER


bool SkImageDecoder::chooseFromOneChoice(SkColorType colorType, int width, int height) const {
    Chooser* chooser = fChooser;
    
    if (NULL == chooser) {    
        return true;
    }
    chooser->begin(1);
    chooser->inspect(0, SkColorTypeToBitmapConfig(colorType), width, height);
    return chooser->choose() == 0;
}
#endif

bool SkImageDecoder::allocPixelRef(SkBitmap* bitmap,
                                   SkColorTable* ctable) const {
    return bitmap->allocPixels(fAllocator, ctable);
}



#ifdef SK_SUPPORT_LEGACY_BITMAP_CONFIG
void SkImageDecoder::setPrefConfigTable(const PrefConfigTable& prefTable) {
    fUsePrefTable = true;
    fPrefTable = prefTable;
}
#endif



SkColorType SkImageDecoder::getPrefColorType(SrcDepth srcDepth, bool srcHasAlpha) const {
    SkColorType ct = fDefaultPref;
#ifdef SK_SUPPORT_LEGACY_BITMAP_CONFIG
    if (fUsePrefTable) {
        
        SkBitmap::Config config = SkBitmap::kNo_Config;
        switch (srcDepth) {
            case kIndex_SrcDepth:
                config = srcHasAlpha ? fPrefTable.fPrefFor_8Index_YesAlpha_src
                                     : fPrefTable.fPrefFor_8Index_NoAlpha_src;
                break;
            case k8BitGray_SrcDepth:
                config = fPrefTable.fPrefFor_8Gray_src;
                break;
            case k32Bit_SrcDepth:
                config = srcHasAlpha ? fPrefTable.fPrefFor_8bpc_YesAlpha_src
                                     : fPrefTable.fPrefFor_8bpc_NoAlpha_src;
                break;
        }
        
        ct = SkBitmapConfigToColorType(config);
    }
#endif
    if (fPreserveSrcDepth) {
        switch (srcDepth) {
            case kIndex_SrcDepth:
                ct = kIndex_8_SkColorType;
                break;
            case k8BitGray_SrcDepth:
                ct = kN32_SkColorType;
                break;
            case k32Bit_SrcDepth:
                ct = kN32_SkColorType;
                break;
        }
    }
    return ct;
}

bool SkImageDecoder::decode(SkStream* stream, SkBitmap* bm, SkColorType pref, Mode mode) {
    
    fShouldCancelDecode = false;
    
    fDefaultPref = pref;

    
    
    SkBitmap    tmp;
    if (!this->onDecode(stream, &tmp, mode)) {
        return false;
    }
    bm->swap(tmp);
    return true;
}

bool SkImageDecoder::decodeSubset(SkBitmap* bm, const SkIRect& rect, SkColorType pref) {
    
    fShouldCancelDecode = false;
    
    fDefaultPref = pref;

    return this->onDecodeSubset(bm, rect);
}

bool SkImageDecoder::buildTileIndex(SkStreamRewindable* stream, int *width, int *height) {
    
    fShouldCancelDecode = false;

    return this->onBuildTileIndex(stream, width, height);
}

bool SkImageDecoder::cropBitmap(SkBitmap *dst, SkBitmap *src, int sampleSize,
                                int dstX, int dstY, int width, int height,
                                int srcX, int srcY) {
    int w = width / sampleSize;
    int h = height / sampleSize;
    if (src->colorType() == kIndex_8_SkColorType) {
        
        
        
        
        
        
        
        
        int x = (dstX - srcX) / sampleSize;
        int y = (dstY - srcY) / sampleSize;
        SkIRect subset = SkIRect::MakeXYWH(x, y, w, h);
        return src->extractSubset(dst, subset);
    }
    
    if (dst->isNull()) {
        dst->setInfo(src->info().makeWH(w, h));

        if (!this->allocPixelRef(dst, NULL)) {
            SkDEBUGF(("failed to allocate pixels needed to crop the bitmap"));
            return false;
        }
    }
    
    
    
    if (dst->width() < w || dst->height() < h) {
        SkDEBUGF(("SkImageDecoder::cropBitmap does not have a large enough bitmap.\n"));
    }

    
    
    SkPaint paint;
    paint.setXfermodeMode(SkXfermode::kSrc_Mode);

    SkCanvas canvas(*dst);
    canvas.drawSprite(*src, (srcX - dstX) / sampleSize,
                            (srcY - dstY) / sampleSize,
                            &paint);
    return true;
}



bool SkImageDecoder::DecodeFile(const char file[], SkBitmap* bm, SkColorType pref,  Mode mode,
                                Format* format) {
    SkASSERT(file);
    SkASSERT(bm);

    SkAutoTUnref<SkStreamRewindable> stream(SkStream::NewFromFile(file));
    if (stream.get()) {
        if (SkImageDecoder::DecodeStream(stream, bm, pref, mode, format)) {
            bm->pixelRef()->setURI(file);
            return true;
        }
    }
    return false;
}

bool SkImageDecoder::DecodeMemory(const void* buffer, size_t size, SkBitmap* bm, SkColorType pref,
                                  Mode mode, Format* format) {
    if (0 == size) {
        return false;
    }
    SkASSERT(buffer);

    SkMemoryStream  stream(buffer, size);
    return SkImageDecoder::DecodeStream(&stream, bm, pref, mode, format);
}

bool SkImageDecoder::DecodeStream(SkStreamRewindable* stream, SkBitmap* bm, SkColorType pref,
                                  Mode mode, Format* format) {
    SkASSERT(stream);
    SkASSERT(bm);

    bool success = false;
    SkImageDecoder* codec = SkImageDecoder::Factory(stream);

    if (NULL != codec) {
        success = codec->decode(stream, bm, pref, mode);
        if (success && format) {
            *format = codec->getFormat();
            if (kUnknown_Format == *format) {
                if (stream->rewind()) {
                    *format = GetStreamFormat(stream);
                }
            }
        }
        delete codec;
    }
    return success;
}
