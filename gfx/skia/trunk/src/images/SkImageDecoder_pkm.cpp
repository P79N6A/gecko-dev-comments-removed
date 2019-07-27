






#include "SkColorPriv.h"
#include "SkImageDecoder.h"
#include "SkScaledBitmapSampler.h"
#include "SkStream.h"
#include "SkStreamPriv.h"
#include "SkTypes.h"

#include "etc1.h"

class SkPKMImageDecoder : public SkImageDecoder {
public:
    SkPKMImageDecoder() { }

    virtual Format getFormat() const SK_OVERRIDE {
        return kPKM_Format;
    }

protected:
    virtual bool onDecode(SkStream* stream, SkBitmap* bm, Mode) SK_OVERRIDE;

private:
    typedef SkImageDecoder INHERITED;
};



bool SkPKMImageDecoder::onDecode(SkStream* stream, SkBitmap* bm, Mode mode) {
    SkAutoMalloc autoMal;
    const size_t length = SkCopyStreamToStorage(&autoMal, stream);
    if (0 == length) {
        return false;
    }

    unsigned char* buf = (unsigned char*)autoMal.get();

    
    SkASSERT(etc1_pkm_is_valid(buf));

    const unsigned short width = etc1_pkm_get_width(buf);
    const unsigned short height = etc1_pkm_get_height(buf);

#ifdef SK_SUPPORT_LEGACY_IMAGEDECODER_CHOOSER
    
    if (!this->chooseFromOneChoice(kN32_SkColorType, width, height)) {
        return false;
    }
#endif

    
    SkScaledBitmapSampler sampler(width, height, this->getSampleSize());

    
    bm->setInfo(SkImageInfo::MakeN32(sampler.scaledWidth(), sampler.scaledHeight(),
                                     kOpaque_SkAlphaType));
    if (SkImageDecoder::kDecodeBounds_Mode == mode) {
        return true;
    }

    if (!this->allocPixelRef(bm, NULL)) {
        return false;
    }

    
    SkAutoLockPixels alp(*bm);

    if (!sampler.begin(bm, SkScaledBitmapSampler::kRGB, *this)) {
        return false;
    }

    
    buf += ETC_PKM_HEADER_SIZE;

    
    int nPixels = width * height;
    SkAutoMalloc outRGBData(nPixels * 3);
    etc1_byte *outRGBDataPtr = reinterpret_cast<etc1_byte *>(outRGBData.get());

    
    if (etc1_decode_image(buf, outRGBDataPtr, width, height, 3, width*3)) {
        return false;
    }

    
    const int srcRowBytes = width * 3;
    const int dstHeight = sampler.scaledHeight();
    const uint8_t *srcRow = reinterpret_cast<uint8_t *>(outRGBDataPtr);
    srcRow += sampler.srcY0() * srcRowBytes;
    for (int y = 0; y < dstHeight; ++y) {
        sampler.next(srcRow);
        srcRow += sampler.srcDY() * srcRowBytes;
    }

    return true;
}


DEFINE_DECODER_CREATOR(PKMImageDecoder);


static bool is_pkm(SkStreamRewindable* stream) {
    
    unsigned char buf[ETC_PKM_HEADER_SIZE];
    if (stream->read((void*)buf, ETC_PKM_HEADER_SIZE) != ETC_PKM_HEADER_SIZE) {
        return false;
    }

    return SkToBool(etc1_pkm_is_valid(buf));
}

static SkImageDecoder* sk_libpkm_dfactory(SkStreamRewindable* stream) {
    if (is_pkm(stream)) {
        return SkNEW(SkPKMImageDecoder);
    }
    return NULL;
}

static SkImageDecoder_DecodeReg gReg(sk_libpkm_dfactory);

static SkImageDecoder::Format get_format_pkm(SkStreamRewindable* stream) {
    if (is_pkm(stream)) {
        return SkImageDecoder::kPKM_Format;
    }
    return SkImageDecoder::kUnknown_Format;
}

static SkImageDecoder_FormatReg gFormatReg(get_format_pkm);
