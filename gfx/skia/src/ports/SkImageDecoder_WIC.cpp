








#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wincodec.h>
#include "SkAutoCoInitialize.h"
#include "SkImageDecoder.h"
#include "SkImageEncoder.h"
#include "SkIStream.h"
#include "SkMovie.h"
#include "SkStream.h"
#include "SkTScopedComPtr.h"

class SkImageDecoder_WIC : public SkImageDecoder {
protected:
    virtual bool onDecode(SkStream* stream, SkBitmap* bm, Mode mode);
};

bool SkImageDecoder_WIC::onDecode(SkStream* stream, SkBitmap* bm, Mode mode) {
    
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
        bm->setConfig(SkBitmap::kARGB_8888_Config, width, height);
        if (SkImageDecoder::kDecodeBounds_Mode == mode) {
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
    
    if (SUCCEEDED(hr)) {
        hr = piFormatConverter->Initialize(
            piBitmapSourceOriginal.get()      
            , GUID_WICPixelFormat32bppPBGRA   
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
        bm->eraseColor(0);
        const int stride = bm->rowBytes();
        hr = piBitmapSourceConverted->CopyPixels(
            NULL,                             
            stride,
            stride * height,
            reinterpret_cast<BYTE *>(bm->getPixels())
        );
    }
    
    return SUCCEEDED(hr);
}



SkImageDecoder* SkImageDecoder::Factory(SkStream* stream) {
    return SkNEW(SkImageDecoder_WIC);
}



SkMovie* SkMovie::DecodeStream(SkStream* stream) {
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
    if (SkBitmap::kARGB_8888_Config == bitmapOrig.config()) {
        bitmap = &bitmapOrig;
    } else {
        if (!bitmapOrig.copyTo(&bitmapCopy, SkBitmap::kARGB_8888_Config)) {
            return false;
        }
        bitmap = &bitmapCopy;
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
        hr = piBitmapFrameEncode->WritePixels(
            height
            , bitmap->rowBytes()
            , bitmap->rowBytes()*height
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

SkImageEncoder* SkImageEncoder::Create(Type t) {
    switch (t) {
        case kJPEG_Type:
        case kPNG_Type:
            break;
        default:
            return NULL;
    }
    return SkNEW_ARGS(SkImageEncoder_WIC, (t));
}

