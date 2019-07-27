






#include "SkData.h"
#include "SkDecodingImageGenerator.h"
#include "SkImageDecoder.h"
#include "SkImageInfo.h"
#include "SkImageGenerator.h"
#include "SkImagePriv.h"
#include "SkStream.h"
#include "SkUtils.h"

namespace {
bool equal_modulo_alpha(const SkImageInfo& a, const SkImageInfo& b) {
    return a.width() == b.width() && a.height() == b.height() &&
           a.colorType() == b.colorType();
}

class DecodingImageGenerator : public SkImageGenerator {
public:
    virtual ~DecodingImageGenerator();

    SkData*                fData;
    SkStreamRewindable*    fStream;
    const SkImageInfo      fInfo;
    const int              fSampleSize;
    const bool             fDitherImage;

    DecodingImageGenerator(SkData* data,
                           SkStreamRewindable* stream,
                           const SkImageInfo& info,
                           int sampleSize,
                           bool ditherImage);

protected:
    virtual SkData* onRefEncodedData() SK_OVERRIDE;
    virtual bool onGetInfo(SkImageInfo* info) SK_OVERRIDE {
        *info = fInfo;
        return true;
    }
    virtual bool onGetPixels(const SkImageInfo& info,
                             void* pixels, size_t rowBytes,
                             SkPMColor ctable[], int* ctableCount) SK_OVERRIDE;

private:
    typedef SkImageGenerator INHERITED;
};





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

        
        
        
        bm->installPixels(fInfo, fTarget, fRowBytes, ct, NULL, NULL);

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



DecodingImageGenerator::DecodingImageGenerator(
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

DecodingImageGenerator::~DecodingImageGenerator() {
    SkSafeUnref(fData);
    fStream->unref();
}

SkData* DecodingImageGenerator::onRefEncodedData() {
    
    
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

bool DecodingImageGenerator::onGetPixels(const SkImageInfo& info,
                                         void* pixels, size_t rowBytes,
                                         SkPMColor ctableEntries[], int* ctableCount) {
    if (fInfo != info) {
        
        
        
        return false;
    }

    SkAssertResult(fStream->rewind());
    SkAutoTDelete<SkImageDecoder> decoder(SkImageDecoder::Factory(fStream));
    if (NULL == decoder.get()) {
        return false;
    }
    decoder->setDitherImage(fDitherImage);
    decoder->setSampleSize(fSampleSize);
    decoder->setRequireUnpremultipliedColors(
            info.fAlphaType == kUnpremul_SkAlphaType);

    SkBitmap bitmap;
    TargetAllocator allocator(fInfo, pixels, rowBytes);
    decoder->setAllocator(&allocator);
    bool success = decoder->decode(fStream, &bitmap, info.colorType(),
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

    if (kIndex_8_SkColorType == info.colorType()) {
        if (kIndex_8_SkColorType != bitmap.colorType()) {
            return false;   
        }
        SkColorTable* ctable = bitmap.getColorTable();
        if (NULL == ctable) {
            return false;
        }
        const int count = ctable->count();
        memcpy(ctableEntries, ctable->lockColors(), count * sizeof(SkPMColor));
        ctable->unlockColors();
        *ctableCount = count;
    }
    return true;
}




SkImageGenerator* CreateDecodingImageGenerator(
        SkData* data,
        SkStreamRewindable* stream,
        const SkDecodingImageGenerator::Options& opts) {
    SkASSERT(stream);
    SkAutoTUnref<SkStreamRewindable> autoStream(stream);  
    SkAssertResult(autoStream->rewind());
    SkAutoTDelete<SkImageDecoder> decoder(SkImageDecoder::Factory(autoStream));
    if (NULL == decoder.get()) {
        return NULL;
    }
    SkBitmap bitmap;
    decoder->setSampleSize(opts.fSampleSize);
    decoder->setRequireUnpremultipliedColors(opts.fRequireUnpremul);
    if (!decoder->decode(stream, &bitmap, SkImageDecoder::kDecodeBounds_Mode)) {
        return NULL;
    }
    if (kUnknown_SkColorType == bitmap.colorType()) {
        return NULL;
    }

    SkImageInfo info = bitmap.info();

    if (opts.fUseRequestedColorType && (opts.fRequestedColorType != info.colorType())) {
        if (!bitmap.canCopyTo(opts.fRequestedColorType)) {
            SkASSERT(bitmap.colorType() != opts.fRequestedColorType);
            return NULL;  
        }
        info.fColorType = opts.fRequestedColorType;
    }

    if (opts.fRequireUnpremul && info.fAlphaType != kOpaque_SkAlphaType) {
        info.fAlphaType = kUnpremul_SkAlphaType;
    }

    if (!SkColorTypeValidateAlphaType(info.fColorType, info.fAlphaType, &info.fAlphaType)) {
        return NULL;
    }

    return SkNEW_ARGS(DecodingImageGenerator,
                      (data, autoStream.detach(), info,
                       opts.fSampleSize, opts.fDitherImage));
}

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
    return CreateDecodingImageGenerator(data, stream, opts);
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
    return CreateDecodingImageGenerator(NULL, stream, opts);
}
