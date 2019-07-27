






#include "SkCGUtils.h"
#include "SkColorPriv.h"
#include "SkImageDecoder.h"
#include "SkImageEncoder.h"
#include "SkMovie.h"
#include "SkStream.h"
#include "SkStreamPriv.h"
#include "SkTemplates.h"
#include "SkUnPreMultiply.h"

#ifdef SK_BUILD_FOR_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

#ifdef SK_BUILD_FOR_IOS
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#include <MobileCoreServices/MobileCoreServices.h>
#endif

static void malloc_release_proc(void* info, const void* data, size_t size) {
    sk_free(info);
}

static CGDataProviderRef SkStreamToDataProvider(SkStream* stream) {
    
    SkAutoMalloc storage;
    const size_t len = SkCopyStreamToStorage(&storage, stream);
    void* data = storage.detach();

    return CGDataProviderCreateWithData(data, data, len, malloc_release_proc);
}

static CGImageSourceRef SkStreamToCGImageSource(SkStream* stream) {
    CGDataProviderRef data = SkStreamToDataProvider(stream);
    CGImageSourceRef imageSrc = CGImageSourceCreateWithDataProvider(data, 0);
    CGDataProviderRelease(data);
    return imageSrc;
}

class SkImageDecoder_CG : public SkImageDecoder {
protected:
    virtual bool onDecode(SkStream* stream, SkBitmap* bm, Mode);
};

static void argb_4444_force_opaque(void* row, int count) {
    uint16_t* row16 = (uint16_t*)row;
    for (int i = 0; i < count; ++i) {
        row16[i] |= 0xF000;
    }
}

static void argb_8888_force_opaque(void* row, int count) {
    
    const uint32_t alphaMask = 0xFF << SK_RGBA_A32_SHIFT;
    uint32_t* row32 = (uint32_t*)row;
    for (int i = 0; i < count; ++i) {
        row32[i] |= alphaMask;
    }
}

static void alpha_8_force_opaque(void* row, int count) {
    memset(row, 0xFF, count);
}

static void force_opaque(SkBitmap* bm) {
    SkAutoLockPixels alp(*bm);
    if (!bm->getPixels()) {
        return;
    }

    void (*proc)(void*, int);
    switch (bm->colorType()) {
        case kARGB_4444_SkColorType:
            proc = argb_4444_force_opaque;
            break;
        case kRGBA_8888_SkColorType:
        case kBGRA_8888_SkColorType:
            proc = argb_8888_force_opaque;
            break;
        case kAlpha_8_SkColorType:
            proc = alpha_8_force_opaque;
            break;
        default:
            return;
    }

    char* row = (char*)bm->getPixels();
    for (int y = 0; y < bm->height(); ++y) {
        proc(row, bm->width());
        row += bm->rowBytes();
    }
    bm->setAlphaType(kOpaque_SkAlphaType);
}

#define BITMAP_INFO (kCGBitmapByteOrder32Big | kCGImageAlphaPremultipliedLast)

bool SkImageDecoder_CG::onDecode(SkStream* stream, SkBitmap* bm, Mode mode) {
    CGImageSourceRef imageSrc = SkStreamToCGImageSource(stream);

    if (NULL == imageSrc) {
        return false;
    }
    SkAutoTCallVProc<const void, CFRelease> arsrc(imageSrc);

    CGImageRef image = CGImageSourceCreateImageAtIndex(imageSrc, 0, NULL);
    if (NULL == image) {
        return false;
    }
    SkAutoTCallVProc<CGImage, CGImageRelease> arimage(image);

    const int width = SkToInt(CGImageGetWidth(image));
    const int height = SkToInt(CGImageGetHeight(image));

    bm->setInfo(SkImageInfo::MakeN32Premul(width, height));
    if (SkImageDecoder::kDecodeBounds_Mode == mode) {
        return true;
    }

    if (!this->allocPixelRef(bm, NULL)) {
        return false;
    }

    SkAutoLockPixels alp(*bm);

    if (!SkCopyPixelsFromCGImage(bm->info(), bm->rowBytes(), bm->getPixels(), image)) {
        return false;
    }

    CGImageAlphaInfo info = CGImageGetAlphaInfo(image);
    switch (info) {
        case kCGImageAlphaNone:
        case kCGImageAlphaNoneSkipLast:
        case kCGImageAlphaNoneSkipFirst:
            
            
            
            force_opaque(bm);
            break;
        default:
            
            if (SkBitmap::ComputeIsOpaque(*bm)) {
                bm->setAlphaType(kOpaque_SkAlphaType);
            }
    }
    if (!bm->isOpaque() && this->getRequireUnpremultipliedColors()) {
        
        
        for (int i = 0; i < width; ++i) {
            for (int j = 0; j < height; ++j) {
                uint32_t* addr = bm->getAddr32(i, j);
                *addr = SkUnPreMultiply::UnPreMultiplyPreservingByteOrder(*addr);
            }
        }
        bm->setAlphaType(kUnpremul_SkAlphaType);
    }
    return true;
}



extern SkImageDecoder* image_decoder_from_stream(SkStreamRewindable*);

SkImageDecoder* SkImageDecoder::Factory(SkStreamRewindable* stream) {
    SkImageDecoder* decoder = image_decoder_from_stream(stream);
    if (NULL == decoder) {
        
        return SkNEW(SkImageDecoder_CG);
    } else {
        return decoder;
    }
}



SkMovie* SkMovie::DecodeStream(SkStreamRewindable* stream) {
    return NULL;
}



static size_t consumer_put(void* info, const void* buffer, size_t count) {
    SkWStream* stream = reinterpret_cast<SkWStream*>(info);
    return stream->write(buffer, count) ? count : 0;
}

static void consumer_release(void* info) {
    
}

static CGDataConsumerRef SkStreamToCGDataConsumer(SkWStream* stream) {
    CGDataConsumerCallbacks procs;
    procs.putBytes = consumer_put;
    procs.releaseConsumer = consumer_release;
    
    
    return CGDataConsumerCreate(stream, &procs);
}

static CGImageDestinationRef SkStreamToImageDestination(SkWStream* stream,
                                                        CFStringRef type) {
    CGDataConsumerRef consumer = SkStreamToCGDataConsumer(stream);
    if (NULL == consumer) {
        return NULL;
    }
    SkAutoTCallVProc<const void, CFRelease> arconsumer(consumer);

    return CGImageDestinationCreateWithDataConsumer(consumer, type, 1, NULL);
}

class SkImageEncoder_CG : public SkImageEncoder {
public:
    SkImageEncoder_CG(Type t) : fType(t) {}

protected:
    virtual bool onEncode(SkWStream* stream, const SkBitmap& bm, int quality);

private:
    Type fType;
};





bool SkImageEncoder_CG::onEncode(SkWStream* stream, const SkBitmap& bm,
                                 int quality) {
    
    const SkBitmap* bmPtr = &bm;
    SkBitmap bitmap8888;

    CFStringRef type;
    switch (fType) {
        case kICO_Type:
            type = kUTTypeICO;
            break;
        case kBMP_Type:
            type = kUTTypeBMP;
            break;
        case kGIF_Type:
            type = kUTTypeGIF;
            break;
        case kJPEG_Type:
            type = kUTTypeJPEG;
            break;
        case kPNG_Type:
            
            
            
            
            
            if (bm.colorType() == kARGB_4444_SkColorType) {
                bm.copyTo(&bitmap8888, kN32_SkColorType);
                bmPtr = &bitmap8888;
            }
            type = kUTTypePNG;
            break;
        default:
            return false;
    }

    CGImageDestinationRef dst = SkStreamToImageDestination(stream, type);
    if (NULL == dst) {
        return false;
    }
    SkAutoTCallVProc<const void, CFRelease> ardst(dst);

    CGImageRef image = SkCreateCGImageRef(*bmPtr);
    if (NULL == image) {
        return false;
    }
    SkAutoTCallVProc<CGImage, CGImageRelease> agimage(image);

    CGImageDestinationAddImage(dst, image, NULL);
    return CGImageDestinationFinalize(dst);
}



static SkImageEncoder* sk_imageencoder_cg_factory(SkImageEncoder::Type t) {
    switch (t) {
        case SkImageEncoder::kICO_Type:
        case SkImageEncoder::kBMP_Type:
        case SkImageEncoder::kGIF_Type:
        case SkImageEncoder::kJPEG_Type:
        case SkImageEncoder::kPNG_Type:
            break;
        default:
            return NULL;
    }
    return SkNEW_ARGS(SkImageEncoder_CG, (t));
}

static SkImageEncoder_EncodeReg gEReg(sk_imageencoder_cg_factory);

#ifdef SK_BUILD_FOR_IOS
class SkPNGImageEncoder_IOS : public SkImageEncoder_CG {
public:
    SkPNGImageEncoder_IOS()
        : SkImageEncoder_CG(kPNG_Type) {
    }
};

DEFINE_ENCODER_CREATOR(PNGImageEncoder_IOS);
#endif

struct FormatConversion {
    CFStringRef             fUTType;
    SkImageDecoder::Format  fFormat;
};


static const FormatConversion gFormatConversions[] = {
    { kUTTypeBMP, SkImageDecoder::kBMP_Format },
    { kUTTypeGIF, SkImageDecoder::kGIF_Format },
    { kUTTypeICO, SkImageDecoder::kICO_Format },
    { kUTTypeJPEG, SkImageDecoder::kJPEG_Format },
    
    { kUTTypeJPEG2000, SkImageDecoder::kJPEG_Format },
    { kUTTypePNG, SkImageDecoder::kPNG_Format },
};

static SkImageDecoder::Format UTType_to_Format(const CFStringRef uttype) {
    for (size_t i = 0; i < SK_ARRAY_COUNT(gFormatConversions); i++) {
        if (CFStringCompare(uttype, gFormatConversions[i].fUTType, 0) == kCFCompareEqualTo) {
            return gFormatConversions[i].fFormat;
        }
    }
    return SkImageDecoder::kUnknown_Format;
}

static SkImageDecoder::Format get_format_cg(SkStreamRewindable* stream) {
    CGImageSourceRef imageSrc = SkStreamToCGImageSource(stream);

    if (NULL == imageSrc) {
        return SkImageDecoder::kUnknown_Format;
    }

    SkAutoTCallVProc<const void, CFRelease> arsrc(imageSrc);
    const CFStringRef name = CGImageSourceGetType(imageSrc);
    if (NULL == name) {
        return SkImageDecoder::kUnknown_Format;
    }
    return UTType_to_Format(name);
}

static SkImageDecoder_FormatReg gFormatReg(get_format_cg);
