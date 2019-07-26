






#include "SkData.h"
#include "SkDecodingImageGenerator.h"
#include "SkImageDecoder.h"
#include "SkImageInfo.h"
#include "SkImageGenerator.h"
#include "SkImagePriv.h"
#include "SkStream.h"
#include "SkUtils.h"

static bool equal_modulo_alpha(const SkImageInfo& a, const SkImageInfo& b) {
    return a.width() == b.width() && a.height() == b.height() &&
           a.colorType() == b.colorType();
}

namespace {




class TargetAllocator : public SkBitmap::Allocator {
public:
    TargetAllocator(const SkImageInfo& info,
                    void* target,
                    size_t rowBytes)
        : fInfo(info)
        , fTarget(target)
        , fRowBytes(rowBytes)
    {}

    bool isReady() { return (fTarget != NULL); }

    virtual bool allocPixelRef(SkBitmap* bm, SkColorTable* ct) {
        if (NULL == fTarget || !equal_modulo_alpha(fInfo, bm->info())) {
            
            return bm->allocPixels(NULL, ct);
        }

        
        
        
        bm->installPixels(fInfo, fTarget, fRowBytes, NULL, NULL);

        fTarget = NULL;  
        return true;
    }

private:
    const SkImageInfo fInfo;
    void* fTarget;  
                    
                    
    const size_t fRowBytes;  

    typedef SkBitmap::Allocator INHERITED;
};


#ifdef SK_DEBUG
    #define SkCheckResult(expr, value)  SkASSERT((value) == (expr))
#else
    #define SkCheckResult(expr, value)  (void)(expr)
#endif

#ifdef SK_DEBUG
inline bool check_alpha(SkAlphaType reported, SkAlphaType actual) {
    return ((reported == actual)
            || ((reported == kPremul_SkAlphaType)
                && (actual == kOpaque_SkAlphaType)));
}
#endif  

}  


SkDecodingImageGenerator::SkDecodingImageGenerator(
        SkData* data,
        SkStreamRewindable* stream,
        const SkImageInfo& info,
        int sampleSize,
        bool ditherImage)
    : fData(data)
    , fStream(stream)
    , fInfo(info)
    , fSampleSize(sampleSize)
    , fDitherImage(ditherImage)
{
    SkASSERT(stream != NULL);
    SkSafeRef(fData);  
}

SkDecodingImageGenerator::~SkDecodingImageGenerator() {
    SkSafeUnref(fData);
    fStream->unref();
}

bool SkDecodingImageGenerator::getInfo(SkImageInfo* info) {
    if (info != NULL) {
        *info = fInfo;
    }
    return true;
}

SkData* SkDecodingImageGenerator::refEncodedData() {
    
    
    if (fData != NULL) {
        return SkSafeRef(fData);
    }
    
    
    if (!fStream->rewind()) {
        return NULL;
    }
    size_t length = fStream->getLength();
    if (0 == length) {
        return NULL;
    }
    void* buffer = sk_malloc_flags(length, 0);
    SkCheckResult(fStream->read(buffer, length), length);
    fData = SkData::NewFromMalloc(buffer, length);
    return SkSafeRef(fData);
}

bool SkDecodingImageGenerator::getPixels(const SkImageInfo& info,
                                         void* pixels,
                                         size_t rowBytes) {
    if (NULL == pixels) {
        return false;
    }
    if (fInfo != info) {
        
        
        
        return false;
    }
    if (info.minRowBytes() > rowBytes) {
        
        return false;
    }

    SkAssertResult(fStream->rewind());
    SkAutoTDelete<SkImageDecoder> decoder(SkImageDecoder::Factory(fStream));
    if (NULL == decoder.get()) {
        return false;
    }
    decoder->setDitherImage(fDitherImage);
    decoder->setSampleSize(fSampleSize);

    SkBitmap bitmap;
    TargetAllocator allocator(fInfo, pixels, rowBytes);
    decoder->setAllocator(&allocator);
    
    SkBitmap::Config legacyConfig = SkColorTypeToBitmapConfig(info.colorType());
    bool success = decoder->decode(fStream, &bitmap, legacyConfig,
                                   SkImageDecoder::kDecodePixels_Mode);
    decoder->setAllocator(NULL);
    if (!success) {
        return false;
    }
    if (allocator.isReady()) {  
        SkBitmap bm;
        SkASSERT(bitmap.canCopyTo(info.colorType()));
        bool copySuccess = bitmap.copyTo(&bm, info.colorType(), &allocator);
        if (!copySuccess || allocator.isReady()) {
            SkDEBUGFAIL("bitmap.copyTo(requestedConfig) failed.");
            
            return false;
        }
        SkASSERT(check_alpha(info.alphaType(), bm.alphaType()));
    } else {
        SkASSERT(check_alpha(info.alphaType(), bitmap.alphaType()));
    }
    return true;
}

SkImageGenerator* SkDecodingImageGenerator::Create(
        SkData* data,
        const SkDecodingImageGenerator::Options& opts) {
    SkASSERT(data != NULL);
    if (NULL == data) {
        return NULL;
    }
    SkStreamRewindable* stream = SkNEW_ARGS(SkMemoryStream, (data));
    SkASSERT(stream != NULL);
    SkASSERT(stream->unique());
    return SkDecodingImageGenerator::Create(data, stream, opts);
}

SkImageGenerator* SkDecodingImageGenerator::Create(
        SkStreamRewindable* stream,
        const SkDecodingImageGenerator::Options& opts) {
    SkASSERT(stream != NULL);
    SkASSERT(stream->unique());
    if ((stream == NULL) || !stream->unique()) {
        SkSafeUnref(stream);
        return NULL;
    }
    return SkDecodingImageGenerator::Create(NULL, stream, opts);
}




SkImageGenerator* SkDecodingImageGenerator::Create(
        SkData* data,
        SkStreamRewindable* stream,
        const SkDecodingImageGenerator::Options& opts) {
    SkASSERT(stream);
    SkAutoTUnref<SkStreamRewindable> autoStream(stream);  
    if (opts.fUseRequestedColorType &&
        (kIndex_8_SkColorType == opts.fRequestedColorType)) {
        
        return NULL;
    }
    SkAssertResult(autoStream->rewind());
    SkAutoTDelete<SkImageDecoder> decoder(SkImageDecoder::Factory(autoStream));
    if (NULL == decoder.get()) {
        return NULL;
    }
    SkBitmap bitmap;
    decoder->setSampleSize(opts.fSampleSize);
    if (!decoder->decode(stream, &bitmap,
                         SkImageDecoder::kDecodeBounds_Mode)) {
        return NULL;
    }
    if (bitmap.config() == SkBitmap::kNo_Config) {
        return NULL;
    }

    SkImageInfo info = bitmap.info();

    if (!opts.fUseRequestedColorType) {
        
        if (kIndex_8_SkColorType == bitmap.colorType()) {
            
            
            info.fColorType = kPMColor_SkColorType;
        }
    } else {
        if (!bitmap.canCopyTo(opts.fRequestedColorType)) {
            SkASSERT(bitmap.colorType() != opts.fRequestedColorType);
            return NULL;  
        }
        info.fColorType = opts.fRequestedColorType;
    }
    return SkNEW_ARGS(SkDecodingImageGenerator,
                      (data, autoStream.detach(), info,
                       opts.fSampleSize, opts.fDitherImage));
}
