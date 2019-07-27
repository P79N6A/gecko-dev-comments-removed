







#include "SkTypes.h"







#undef INT8_MIN
#undef INT16_MIN
#undef INT32_MIN
#undef INT64_MIN
#undef INT8_MAX
#undef UINT8_MAX
#undef INT16_MAX
#undef UINT16_MAX
#undef INT32_MAX
#undef UINT32_MAX
#undef INT64_MAX
#undef UINT64_MAX

#include <wincodec.h>
#include "SkAutoCoInitialize.h"
#include "SkImageDecoder.h"
#include "SkImageEncoder.h"
#include "SkIStream.h"
#include "SkMovie.h"
#include "SkStream.h"
#include "SkTScopedComPtr.h"
#include "SkUnPreMultiply.h"






#if defined(CLSID_WICImagingFactory)
#undef CLSID_WICImagingFactory
#endif

class SkImageDecoder_WIC : public SkImageDecoder {
public:
    
    
    enum WICModes {
        kDecodeFormat_WICMode,
        kDecodeBounds_WICMode,
        kDecodePixels_WICMode,
    };

    







    bool decodeStream(SkStream* stream, SkBitmap* bm, WICModes wicMode, Format* format) const;

protected:
    virtual bool onDecode(SkStream* stream, SkBitmap* bm, Mode mode) SK_OVERRIDE;
};

struct FormatConversion {
    GUID                    fGuidFormat;
    SkImageDecoder::Format  fFormat;
};

static const FormatConversion gFormatConversions[] = {
    { GUID_ContainerFormatBmp, SkImageDecoder::kBMP_Format },
    { GUID_ContainerFormatGif, SkImageDecoder::kGIF_Format },
    { GUID_ContainerFormatIco, SkImageDecoder::kICO_Format },
    { GUID_ContainerFormatJpeg, SkImageDecoder::kJPEG_Format },
    { GUID_ContainerFormatPng, SkImageDecoder::kPNG_Format },
};

static SkImageDecoder::Format GuidContainerFormat_to_Format(REFGUID guid) {
    for (size_t i = 0; i < SK_ARRAY_COUNT(gFormatConversions); i++) {
        if (IsEqualGUID(guid, gFormatConversions[i].fGuidFormat)) {
            return gFormatConversions[i].fFormat;
        }
    }
    return SkImageDecoder::kUnknown_Format;
}

bool SkImageDecoder_WIC::onDecode(SkStream* stream, SkBitmap* bm, Mode mode) {
    WICModes wicMode;
    switch (mode) {
        case SkImageDecoder::kDecodeBounds_Mode:
            wicMode = kDecodeBounds_WICMode;
            break;
        case SkImageDecoder::kDecodePixels_Mode:
            wicMode = kDecodePixels_WICMode;
            break;
    }
    return this->decodeStream(stream, bm, wicMode, NULL);
}

bool SkImageDecoder_WIC::decodeStream(SkStream* stream, SkBitmap* bm, WICModes wicMode,
                                      Format* format) const {
    
    SkAutoCoInitialize scopedCo;
    if (!scopedCo.succeeded()) {
        return false;
    }

    HRESULT hr = S_OK;

    
    SkTScopedComPtr<IWICImagingFactory> piImagingFactory;
    if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(
            CLSID_WICImagingFactory
            , NULL
            , CLSCTX_INPROC_SERVER
            , IID_PPV_ARGS(&piImagingFactory)
        );
    }

    
    SkTScopedComPtr<IStream> piStream;
    if (SUCCEEDED(hr)) {
        hr = SkIStream::CreateFromSkStream(stream, false, &piStream);
    }

    
    if (SUCCEEDED(hr)) {
        LARGE_INTEGER liBeginning = { 0 };
        hr = piStream->Seek(liBeginning, STREAM_SEEK_SET, NULL);
    }

    
    SkTScopedComPtr<IWICBitmapDecoder> piBitmapDecoder;
    if (SUCCEEDED(hr)) {
        hr = piImagingFactory->CreateDecoderFromStream(
            piStream.get()                    
            , NULL                            
            , WICDecodeMetadataCacheOnDemand  
            , &piBitmapDecoder                
        );
    }

    if (kDecodeFormat_WICMode == wicMode) {
        SkASSERT(format != NULL);
        
        if (SUCCEEDED(hr)) {
            GUID guidFormat;
            hr = piBitmapDecoder->GetContainerFormat(&guidFormat);
            if (SUCCEEDED(hr)) {
                *format = GuidContainerFormat_to_Format(guidFormat);
                return true;
            }
        }
        return false;
    }

    
    SkTScopedComPtr<IWICBitmapFrameDecode> piBitmapFrameDecode;
    if (SUCCEEDED(hr)) {
        hr = piBitmapDecoder->GetFrame(0, &piBitmapFrameDecode);
    }

    
    SkTScopedComPtr<IWICBitmapSource> piBitmapSourceOriginal;
    if (SUCCEEDED(hr)) {
        hr = piBitmapFrameDecode->QueryInterface(
            IID_PPV_ARGS(&piBitmapSourceOriginal)
        );
    }

    
    UINT width;
    UINT height;
    if (SUCCEEDED(hr)) {
        hr = piBitmapSourceOriginal->GetSize(&width, &height);
    }

    
    if (SUCCEEDED(hr)) {
        bm->setInfo(SkImageInfo::MakeN32Premul(width, height));
        if (kDecodeBounds_WICMode == wicMode) {
            return true;
        }
        if (!this->allocPixelRef(bm, NULL)) {
            return false;
        }
    }

    
    SkTScopedComPtr<IWICFormatConverter> piFormatConverter;
    if (SUCCEEDED(hr)) {
        hr = piImagingFactory->CreateFormatConverter(&piFormatConverter);
    }

    GUID destinationPixelFormat;
    if (this->getRequireUnpremultipliedColors()) {
        destinationPixelFormat = GUID_WICPixelFormat32bppBGRA;
    } else {
        destinationPixelFormat = GUID_WICPixelFormat32bppPBGRA;
    }

    if (SUCCEEDED(hr)) {
        hr = piFormatConverter->Initialize(
            piBitmapSourceOriginal.get()      
            , destinationPixelFormat          
            , WICBitmapDitherTypeNone         
            , NULL                            
            , 0.f                             
            , WICBitmapPaletteTypeCustom      
        );
    }

    
    SkTScopedComPtr<IWICBitmapSource> piBitmapSourceConverted;
    if (SUCCEEDED(hr)) {
        hr = piFormatConverter->QueryInterface(
            IID_PPV_ARGS(&piBitmapSourceConverted)
        );
    }

    
    if (SUCCEEDED(hr)) {
        SkAutoLockPixels alp(*bm);
        bm->eraseColor(SK_ColorTRANSPARENT);
        const UINT stride = (UINT) bm->rowBytes();
        hr = piBitmapSourceConverted->CopyPixels(
            NULL,                             
            stride,
            stride * height,
            reinterpret_cast<BYTE *>(bm->getPixels())
        );

        
        if (SkBitmap::ComputeIsOpaque(*bm)) {
            bm->setAlphaType(kOpaque_SkAlphaType);
        }
    }

    return SUCCEEDED(hr);
}



extern SkImageDecoder* image_decoder_from_stream(SkStreamRewindable*);

SkImageDecoder* SkImageDecoder::Factory(SkStreamRewindable* stream) {
    SkImageDecoder* decoder = image_decoder_from_stream(stream);
    if (NULL == decoder) {
        
        return SkNEW(SkImageDecoder_WIC);
    } else {
        return decoder;
    }
}



SkMovie* SkMovie::DecodeStream(SkStreamRewindable* stream) {
    return NULL;
}



class SkImageEncoder_WIC : public SkImageEncoder {
public:
    SkImageEncoder_WIC(Type t) : fType(t) {}

protected:
    virtual bool onEncode(SkWStream* stream, const SkBitmap& bm, int quality);

private:
    Type fType;
};

bool SkImageEncoder_WIC::onEncode(SkWStream* stream
                                , const SkBitmap& bitmapOrig
                                , int quality)
{
    GUID type;
    switch (fType) {
        case kBMP_Type:
            type = GUID_ContainerFormatBmp;
            break;
        case kICO_Type:
            type = GUID_ContainerFormatIco;
            break;
        case kJPEG_Type:
            type = GUID_ContainerFormatJpeg;
            break;
        case kPNG_Type:
            type = GUID_ContainerFormatPng;
            break;
        default:
            return false;
    }

    
    const SkBitmap* bitmap;
    SkBitmap bitmapCopy;
    if (kN32_SkColorType == bitmapOrig.colorType() && bitmapOrig.isOpaque()) {
        bitmap = &bitmapOrig;
    } else {
        if (!bitmapOrig.copyTo(&bitmapCopy, kN32_SkColorType)) {
            return false;
        }
        bitmap = &bitmapCopy;
    }

    
    if (!bitmap->isOpaque()) {
        SkAutoLockPixels alp(*bitmap);

        uint8_t* pixels = reinterpret_cast<uint8_t*>(bitmap->getPixels());
        for (int y = 0; y < bitmap->height(); ++y) {
            for (int x = 0; x < bitmap->width(); ++x) {
                uint8_t* bytes = pixels + y * bitmap->rowBytes() + x * bitmap->bytesPerPixel();

                SkPMColor* src = reinterpret_cast<SkPMColor*>(bytes);
                SkColor* dst = reinterpret_cast<SkColor*>(bytes);

                *dst = SkUnPreMultiply::PMColorToColor(*src);
            }
        }
    }

    
    SkAutoCoInitialize scopedCo;
    if (!scopedCo.succeeded()) {
        return false;
    }

    HRESULT hr = S_OK;

    
    SkTScopedComPtr<IWICImagingFactory> piImagingFactory;
    if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(
            CLSID_WICImagingFactory
            , NULL
            , CLSCTX_INPROC_SERVER
            , IID_PPV_ARGS(&piImagingFactory)
        );
    }

    
    SkTScopedComPtr<IStream> piStream;
    if (SUCCEEDED(hr)) {
        hr = SkWIStream::CreateFromSkWStream(stream, &piStream);
    }

    
    SkTScopedComPtr<IWICBitmapEncoder> piEncoder;
    if (SUCCEEDED(hr)) {
        hr = piImagingFactory->CreateEncoder(type, NULL, &piEncoder);
    }

    if (SUCCEEDED(hr)) {
        hr = piEncoder->Initialize(piStream.get(), WICBitmapEncoderNoCache);
    }

    
    SkTScopedComPtr<IWICBitmapFrameEncode> piBitmapFrameEncode;
    SkTScopedComPtr<IPropertyBag2> piPropertybag;
    if (SUCCEEDED(hr)) {
        hr = piEncoder->CreateNewFrame(&piBitmapFrameEncode, &piPropertybag);
    }

    if (SUCCEEDED(hr)) {
        PROPBAG2 name = { 0 };
        name.dwType = PROPBAG2_TYPE_DATA;
        name.vt = VT_R4;
        name.pstrName = L"ImageQuality";

        VARIANT value;
        VariantInit(&value);
        value.vt = VT_R4;
        value.fltVal = (FLOAT)(quality / 100.0);

        
        
        
        
        piPropertybag->Write(1, &name, &value);
    }
    if (SUCCEEDED(hr)) {
        hr = piBitmapFrameEncode->Initialize(piPropertybag.get());
    }

    
    const UINT width = bitmap->width();
    const UINT height = bitmap->height();
    if (SUCCEEDED(hr)) {
        hr = piBitmapFrameEncode->SetSize(width, height);
    }

    
    const WICPixelFormatGUID formatDesired = GUID_WICPixelFormat32bppBGRA;
    WICPixelFormatGUID formatGUID = formatDesired;
    if (SUCCEEDED(hr)) {
        hr = piBitmapFrameEncode->SetPixelFormat(&formatGUID);
    }
    if (SUCCEEDED(hr)) {
        
        hr = IsEqualGUID(formatGUID, formatDesired) ? S_OK : E_FAIL;
    }

    
    if (SUCCEEDED(hr)) {
        SkAutoLockPixels alp(*bitmap);
        const UINT stride = (UINT) bitmap->rowBytes();
        hr = piBitmapFrameEncode->WritePixels(
            height
            , stride
            , stride * height
            , reinterpret_cast<BYTE*>(bitmap->getPixels()));
    }

    if (SUCCEEDED(hr)) {
        hr = piBitmapFrameEncode->Commit();
    }

    if (SUCCEEDED(hr)) {
        hr = piEncoder->Commit();
    }

    return SUCCEEDED(hr);
}



static SkImageEncoder* sk_imageencoder_wic_factory(SkImageEncoder::Type t) {
    switch (t) {
        case SkImageEncoder::kBMP_Type:
        case SkImageEncoder::kICO_Type:
        case SkImageEncoder::kJPEG_Type:
        case SkImageEncoder::kPNG_Type:
            break;
        default:
            return NULL;
    }
    return SkNEW_ARGS(SkImageEncoder_WIC, (t));
}

static SkImageEncoder_EncodeReg gEReg(sk_imageencoder_wic_factory);

static SkImageDecoder::Format get_format_wic(SkStreamRewindable* stream) {
    SkImageDecoder::Format format;
    SkImageDecoder_WIC codec;
    if (!codec.decodeStream(stream, NULL, SkImageDecoder_WIC::kDecodeFormat_WICMode, &format)) {
        format = SkImageDecoder::kUnknown_Format;
    }
    return format;
}

static SkImageDecoder_FormatReg gFormatReg(get_format_wic);
