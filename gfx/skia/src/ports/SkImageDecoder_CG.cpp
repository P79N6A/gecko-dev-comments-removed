







#include "SkColorPriv.h"

#include "SkImageDecoder.h"
#include "SkImageEncoder.h"
#include "SkMovie.h"
#include "SkStream.h"
#include "SkTemplates.h"
#include "SkCGUtils.h"

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
    
    size_t len = stream->getLength();
    void* data = sk_malloc_throw(len);
    stream->read(data, len);

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

    const int width = CGImageGetWidth(image);
    const int height = CGImageGetHeight(image);
    bm->setConfig(SkBitmap::kARGB_8888_Config, width, height);
    if (SkImageDecoder::kDecodeBounds_Mode == mode) {
        return true;
    }

    if (!this->allocPixelRef(bm, NULL)) {
        return false;
    }

    bm->lockPixels();
    bm->eraseColor(SK_ColorTRANSPARENT);

    
    CGColorSpaceRef cs = CGImageGetColorSpace(image);
    CGContextRef cg = CGBitmapContextCreate(bm->getPixels(), width, height, 8, bm->rowBytes(), cs, BITMAP_INFO);
    if (NULL == cg) {
        
        cs = CGColorSpaceCreateDeviceRGB();
        cg = CGBitmapContextCreate(bm->getPixels(), width, height, 8, bm->rowBytes(), cs, BITMAP_INFO);
        CFRelease(cs);
    }
    CGContextDrawImage(cg, CGRectMake(0, 0, width, height), image);
    CGContextRelease(cg);

    CGImageAlphaInfo info = CGImageGetAlphaInfo(image);
    switch (info) {
        case kCGImageAlphaNone:
        case kCGImageAlphaNoneSkipLast:
        case kCGImageAlphaNoneSkipFirst:
            SkASSERT(SkBitmap::ComputeIsOpaque(*bm));
            bm->setIsOpaque(true);
            break;
        default:
            
            bm->computeAndSetOpaquePredicate();
    }
    bm->unlockPixels();
    return true;
}



SkImageDecoder* SkImageDecoder::Factory(SkStream* stream) {
    return SkNEW(SkImageDecoder_CG);
}



SkMovie* SkMovie::DecodeStream(SkStream* stream) {
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
        case kJPEG_Type:
            type = kUTTypeJPEG;
            break;
        case kPNG_Type:
            
            
            
            
            
            if (bm.getConfig() == SkBitmap::kARGB_4444_Config) {
                bm.copyTo(&bitmap8888, SkBitmap::kARGB_8888_Config);
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

SkImageEncoder* SkImageEncoder::Create(Type t) {
    switch (t) {
        case kJPEG_Type:
        case kPNG_Type:
            break;
        default:
            return NULL;
    }
    return SkNEW_ARGS(SkImageEncoder_CG, (t));
}
