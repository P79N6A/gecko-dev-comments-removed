






#include "SkColorPriv.h"
#include "SkImageDecoder.h"
#include "SkPixelRef.h"
#include "SkScaledBitmapSampler.h"
#include "SkStream.h"
#include "SkStreamPriv.h"
#include "SkTypes.h"

#include "ktx.h"
#include "etc1.h"

















class SkKTXImageDecoder : public SkImageDecoder {
public:
    SkKTXImageDecoder() { }

    virtual Format getFormat() const SK_OVERRIDE {
        return kKTX_Format;
    }

protected:
    virtual bool onDecode(SkStream* stream, SkBitmap* bm, Mode) SK_OVERRIDE;

private:
    typedef SkImageDecoder INHERITED;
};

bool SkKTXImageDecoder::onDecode(SkStream* stream, SkBitmap* bm, Mode mode) {
    
    SkAutoDataUnref data(SkCopyStreamToData(stream));
    if (NULL == data) {
        return false;
    }

    SkKTXFile ktxFile(data);
    if (!ktxFile.valid()) {
        return false;
    }

    const unsigned short width = ktxFile.width();
    const unsigned short height = ktxFile.height();

#ifdef SK_SUPPORT_LEGACY_IMAGEDECODER_CHOOSER
    
    if (!this->chooseFromOneChoice(kN32_SkColorType, width, height)) {
        return false;
    }
#endif

    
    const SkString premulKey("KTXPremultipliedAlpha");
    const bool bSrcIsPremul = ktxFile.getValueForKey(premulKey) == SkString("True");

    
    SkScaledBitmapSampler sampler(width, height, this->getSampleSize());

    
    SkAlphaType alphaType = kOpaque_SkAlphaType;
    if (ktxFile.isRGBA8()) {
        if (this->getRequireUnpremultipliedColors()) {
            alphaType = kUnpremul_SkAlphaType;
            
            
            if (bSrcIsPremul) {
                return false;
            }
        } else {
            alphaType = kPremul_SkAlphaType;
        }
    }

    
    bm->setInfo(SkImageInfo::MakeN32(sampler.scaledWidth(), sampler.scaledHeight(), alphaType));
    if (SkImageDecoder::kDecodeBounds_Mode == mode) {
        return true;
    }
    
    
    if (!this->allocPixelRef(bm, NULL)) {
        return false;
    }

    
    SkAutoLockPixels alp(*bm);

    if (ktxFile.isETC1()) {
        if (!sampler.begin(bm, SkScaledBitmapSampler::kRGB, *this)) {
            return false;
        }

        
        int nPixels = width * height;
        SkAutoMalloc outRGBData(nPixels * 3);
        etc1_byte *outRGBDataPtr = reinterpret_cast<etc1_byte *>(outRGBData.get());

        
        const etc1_byte *buf = reinterpret_cast<const etc1_byte *>(ktxFile.pixelData());
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

    } else if (ktxFile.isRGB8()) {

        
        if (!sampler.begin(bm, SkScaledBitmapSampler::kRGB, *this)) {
            return false;
        }

        
        const int srcRowBytes = width * 3;
        const int dstHeight = sampler.scaledHeight();
        const uint8_t *srcRow = reinterpret_cast<const uint8_t *>(ktxFile.pixelData());
        srcRow += sampler.srcY0() * srcRowBytes;
        for (int y = 0; y < dstHeight; ++y) {
            sampler.next(srcRow);
            srcRow += sampler.srcDY() * srcRowBytes;
        }

        return true;

    } else if (ktxFile.isRGBA8()) {

        

        
        
        SkScaledBitmapSampler::Options opts (*this);
        if (bSrcIsPremul) {
            SkASSERT(bm->alphaType() == kPremul_SkAlphaType);
            SkASSERT(!this->getRequireUnpremultipliedColors());

            opts.fPremultiplyAlpha = false;
        } 

        if (!sampler.begin(bm, SkScaledBitmapSampler::kRGBA, opts)) {
            return false;
        }

        
        const int srcRowBytes = width * 4;
        const int dstHeight = sampler.scaledHeight();
        const uint8_t *srcRow = reinterpret_cast<const uint8_t *>(ktxFile.pixelData());
        srcRow += sampler.srcY0() * srcRowBytes;
        for (int y = 0; y < dstHeight; ++y) {
            sampler.next(srcRow);
            srcRow += sampler.srcDY() * srcRowBytes;
        }

        return true;
    }

    return false;
}














class SkKTXImageEncoder : public SkImageEncoder {
protected:
    virtual bool onEncode(SkWStream* stream, const SkBitmap& bm, int quality) SK_OVERRIDE;

private:
    virtual bool encodePKM(SkWStream* stream, const SkData *data);
    typedef SkImageEncoder INHERITED;
};

bool SkKTXImageEncoder::onEncode(SkWStream* stream, const SkBitmap& bitmap, int) {
    if (!bitmap.pixelRef()) {
        return false;
    }
    SkAutoDataUnref data(bitmap.pixelRef()->refEncodedData());

    
    if (NULL != data) {
        const uint8_t *bytes = data->bytes();
        if (etc1_pkm_is_valid(bytes)) {
            return this->encodePKM(stream, data);
        }

        
        if (SkKTXFile::is_ktx(bytes)) {
            return stream->write(bytes, data->size());
        }
        
        
        
    }

    return SkKTXFile::WriteBitmapToKTX(stream, bitmap);
}

bool SkKTXImageEncoder::encodePKM(SkWStream* stream, const SkData *data) {
    const uint8_t* bytes = data->bytes();
    SkASSERT(etc1_pkm_is_valid(bytes));

    etc1_uint32 width = etc1_pkm_get_width(bytes);
    etc1_uint32 height = etc1_pkm_get_height(bytes);

    
    
    if (width == 0 || (width & 3) != 0 || height == 0 || (height & 3) != 0) {
        return false;
    }

    
    bytes += ETC_PKM_HEADER_SIZE;

    return SkKTXFile::WriteETC1ToKTX(stream, bytes, width, height);
}


DEFINE_DECODER_CREATOR(KTXImageDecoder);
DEFINE_ENCODER_CREATOR(KTXImageEncoder);


static SkImageDecoder* sk_libktx_dfactory(SkStreamRewindable* stream) {
    if (SkKTXFile::is_ktx(stream)) {
        return SkNEW(SkKTXImageDecoder);
    }
    return NULL;
}

static SkImageDecoder::Format get_format_ktx(SkStreamRewindable* stream) {
    if (SkKTXFile::is_ktx(stream)) {
        return SkImageDecoder::kKTX_Format;
    }
    return SkImageDecoder::kUnknown_Format;
}

SkImageEncoder* sk_libktx_efactory(SkImageEncoder::Type t) {
    return (SkImageEncoder::kKTX_Type == t) ? SkNEW(SkKTXImageEncoder) : NULL;
}

static SkImageDecoder_DecodeReg gReg(sk_libktx_dfactory);
static SkImageDecoder_FormatReg gFormatReg(get_format_ktx);
static SkImageEncoder_EncodeReg gEReg(sk_libktx_efactory);
