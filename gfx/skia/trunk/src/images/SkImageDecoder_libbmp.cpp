








#include "bmpdecoderhelper.h"
#include "SkColorPriv.h"
#include "SkImageDecoder.h"
#include "SkScaledBitmapSampler.h"
#include "SkStream.h"
#include "SkStreamPriv.h"
#include "SkTDArray.h"

class SkBMPImageDecoder : public SkImageDecoder {
public:
    SkBMPImageDecoder() {}

    virtual Format getFormat() const SK_OVERRIDE {
        return kBMP_Format;
    }

protected:
    virtual bool onDecode(SkStream* stream, SkBitmap* bm, Mode mode) SK_OVERRIDE;

private:
    typedef SkImageDecoder INHERITED;
};


DEFINE_DECODER_CREATOR(BMPImageDecoder);


static bool is_bmp(SkStreamRewindable* stream) {
    static const char kBmpMagic[] = { 'B', 'M' };


    char buffer[sizeof(kBmpMagic)];

    return stream->read(buffer, sizeof(kBmpMagic)) == sizeof(kBmpMagic) &&
        !memcmp(buffer, kBmpMagic, sizeof(kBmpMagic));
}

static SkImageDecoder* sk_libbmp_dfactory(SkStreamRewindable* stream) {
    if (is_bmp(stream)) {
        return SkNEW(SkBMPImageDecoder);
    }
    return NULL;
}

static SkImageDecoder_DecodeReg gReg(sk_libbmp_dfactory);

static SkImageDecoder::Format get_format_bmp(SkStreamRewindable* stream) {
    if (is_bmp(stream)) {
        return SkImageDecoder::kBMP_Format;
    }
    return SkImageDecoder::kUnknown_Format;
}

static SkImageDecoder_FormatReg gFormatReg(get_format_bmp);



class SkBmpDecoderCallback : public image_codec::BmpDecoderCallback {
public:
    
    SkBmpDecoderCallback(bool justBounds) : fJustBounds(justBounds) {}

    
    virtual uint8* SetSize(int width, int height) {
        fWidth = width;
        fHeight = height;
        if (fJustBounds) {
            return NULL;
        }

        fRGB.setCount(width * height * 3);  
        return fRGB.begin();
    }

    int width() const { return fWidth; }
    int height() const { return fHeight; }
    const uint8_t* rgb() const { return fRGB.begin(); }

private:
    SkTDArray<uint8_t> fRGB;
    int fWidth;
    int fHeight;
    bool fJustBounds;
};

bool SkBMPImageDecoder::onDecode(SkStream* stream, SkBitmap* bm, Mode mode) {
    
    

    
    SkAutoMalloc storage;
    
    const size_t length = SkCopyStreamToStorage(&storage, stream);
    if (0 == length) {
        return 0;
    }

    const bool justBounds = SkImageDecoder::kDecodeBounds_Mode == mode;
    SkBmpDecoderCallback callback(justBounds);

    
    {
        image_codec::BmpDecoderHelper helper;
        const int max_pixels = 16383*16383; 
        if (!helper.DecodeImage((const char*)storage.get(), length,
                                max_pixels, &callback)) {
            return false;
        }
    }

    
    
    storage.free();

    int width = callback.width();
    int height = callback.height();
    SkColorType colorType = this->getPrefColorType(k32Bit_SrcDepth, false);

    
    if (kARGB_4444_SkColorType != colorType && kRGB_565_SkColorType != colorType) {
        colorType = kN32_SkColorType;
    }

    SkScaledBitmapSampler sampler(width, height, getSampleSize());

    bm->setInfo(SkImageInfo::Make(sampler.scaledWidth(), sampler.scaledHeight(),
                                  colorType, kOpaque_SkAlphaType));

    if (justBounds) {
        return true;
    }

    if (!this->allocPixelRef(bm, NULL)) {
        return false;
    }

    SkAutoLockPixels alp(*bm);

    if (!sampler.begin(bm, SkScaledBitmapSampler::kRGB, *this)) {
        return false;
    }

    const int srcRowBytes = width * 3;
    const int dstHeight = sampler.scaledHeight();
    const uint8_t* srcRow = callback.rgb();

    srcRow += sampler.srcY0() * srcRowBytes;
    for (int y = 0; y < dstHeight; y++) {
        sampler.next(srcRow);
        srcRow += sampler.srcDY() * srcRowBytes;
    }
    return true;
}
