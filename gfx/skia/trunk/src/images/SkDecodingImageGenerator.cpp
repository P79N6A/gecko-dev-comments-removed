






#include "SkData.h"
#include "SkDecodingImageGenerator.h"
#include "SkImageDecoder.h"
#include "SkImageInfo.h"
#include "SkImageGenerator.h"
#include "SkImagePriv.h"
#include "SkStream.h"
#include "SkUtils.h"

namespace {




class TargetAllocator : public SkBitmap::Allocator {
public:
    TargetAllocator(void* target,
                    size_t rowBytes,
                    int width,
                    int height,
                    SkBitmap::Config config)
        : fTarget(target)
        , fRowBytes(rowBytes)
        , fWidth(width)
        , fHeight(height)
        , fConfig(config) { }

    bool isReady() { return (fTarget != NULL); }

    virtual bool allocPixelRef(SkBitmap* bm, SkColorTable* ct) {
        if ((NULL == fTarget)
            || (fConfig != bm->config())
            || (fWidth != bm->width())
            || (fHeight != bm->height())
            || (ct != NULL)) {
            
            return bm->allocPixels(NULL, ct);
        }
        
        bm->setConfig(fConfig, fWidth, fHeight, fRowBytes, bm->alphaType());
        
        
        
        bm->setPixels(fTarget, NULL);
        fTarget = NULL;  
        return true;
    }

private:
    void* fTarget;  
                    
                    
                    
    size_t fRowBytes;  
    int fWidth;   
    int fHeight;  
                  
                  
                  
                  
                  
                  
    SkBitmap::Config fConfig;
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
        bool ditherImage,
        SkBitmap::Config requestedConfig)
    : fData(data)
    , fStream(stream)
    , fInfo(info)
    , fSampleSize(sampleSize)
    , fDitherImage(ditherImage)
    , fRequestedConfig(requestedConfig) {
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
    int bpp = SkBitmap::ComputeBytesPerPixel(fRequestedConfig);
    if (static_cast<size_t>(bpp * info.fWidth) > rowBytes) {
        
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
    TargetAllocator allocator(pixels, rowBytes, info.fWidth,
                              info.fHeight, fRequestedConfig);
    decoder->setAllocator(&allocator);
    bool success = decoder->decode(fStream, &bitmap, fRequestedConfig,
                                   SkImageDecoder::kDecodePixels_Mode);
    decoder->setAllocator(NULL);
    if (!success) {
        return false;
    }
    if (allocator.isReady()) {  
        SkBitmap bm;
        SkASSERT(bitmap.canCopyTo(fRequestedConfig));
        if (!bitmap.copyTo(&bm, fRequestedConfig, &allocator)
            || allocator.isReady()) {
            SkDEBUGFAIL("bitmap.copyTo(requestedConfig) failed.");
            
            return false;
        }
        SkASSERT(check_alpha(fInfo.fAlphaType, bm.alphaType()));
    } else {
        SkASSERT(check_alpha(fInfo.fAlphaType, bitmap.alphaType()));
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

    SkImageInfo info;
    SkBitmap::Config config;

    if (!opts.fUseRequestedColorType) {
        
        if (SkBitmap::kIndex8_Config == bitmap.config()) {
            
            
            config = SkBitmap::kARGB_8888_Config;
            info.fWidth = bitmap.width();
            info.fHeight = bitmap.height();
            info.fColorType = kPMColor_SkColorType;
            info.fAlphaType = bitmap.alphaType();
        } else {
            config = bitmap.config();  
            if (!bitmap.asImageInfo(&info)) {
                SkDEBUGFAIL("Getting SkImageInfo from bitmap failed.");
                return NULL;
            }
        }
    } else {
        config = SkColorTypeToBitmapConfig(opts.fRequestedColorType);
        if (!bitmap.canCopyTo(config)) {
            SkASSERT(bitmap.config() != config);
            return NULL;  
        }
        info.fWidth = bitmap.width();
        info.fHeight = bitmap.height();
        info.fColorType = opts.fRequestedColorType;
        info.fAlphaType = bitmap.alphaType();
    }
    return SkNEW_ARGS(SkDecodingImageGenerator,
                      (data, autoStream.detach(), info,
                       opts.fSampleSize, opts.fDitherImage, config));
}
