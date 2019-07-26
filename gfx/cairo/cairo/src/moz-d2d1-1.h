






#pragma once

#ifndef _D2D1_1_H_
#ifndef _D2D1_H_
#include <d2d1.h>
#endif 










typedef enum D2D1_LAYER_OPTIONS1
{
        D2D1_LAYER_OPTIONS1_NONE = 0,
        D2D1_LAYER_OPTIONS1_INITIALIZE_FROM_BACKGROUND = 1,
        D2D1_LAYER_OPTIONS1_IGNORE_ALPHA = 2,
        D2D1_LAYER_OPTIONS1_FORCE_DWORD = 0xffffffff

} D2D1_LAYER_OPTIONS1;










typedef struct D2D1_LAYER_PARAMETERS1
{
    D2D1_RECT_F contentBounds;
    ID2D1Geometry *geometricMask;
    D2D1_ANTIALIAS_MODE maskAntialiasMode;
    D2D1_MATRIX_3X2_F maskTransform;
    FLOAT opacity;
    ID2D1Brush *opacityBrush;
    D2D1_LAYER_OPTIONS1 layerOptions;

} D2D1_LAYER_PARAMETERS1;

DEFINE_ENUM_FLAG_OPERATORS(D2D1_LAYER_OPTIONS1);

#ifndef DX_DECLARE_INTERFACE
#define DX_DECLARE_INTERFACE(x) DECLSPEC_UUID(x) DECLSPEC_NOVTABLE
#endif

namespace D2D1
{
    D2D1FORCEINLINE
    D2D1_LAYER_PARAMETERS1
    LayerParameters1(
        CONST D2D1_RECT_F &contentBounds = D2D1::InfiniteRect(),
        ID2D1Geometry *geometricMask = NULL,
        D2D1_ANTIALIAS_MODE maskAntialiasMode = D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
        D2D1_MATRIX_3X2_F maskTransform = D2D1::IdentityMatrix(),
        FLOAT opacity = 1.0,
        ID2D1Brush *opacityBrush = NULL,
        D2D1_LAYER_OPTIONS1 layerOptions = D2D1_LAYER_OPTIONS1_NONE
        )
    {
        D2D1_LAYER_PARAMETERS1 layerParameters = { 0 };

        layerParameters.contentBounds = contentBounds;
        layerParameters.geometricMask = geometricMask;
        layerParameters.maskAntialiasMode = maskAntialiasMode;
        layerParameters.maskTransform = maskTransform;
        layerParameters.opacity = opacity;
        layerParameters.opacityBrush = opacityBrush;
        layerParameters.layerOptions = layerOptions;

        return layerParameters;
    }
}

DEFINE_GUID(IID_ID2D1DeviceContext, 0xe8f7fe7a, 0x191c, 0x466d, 0xad, 0x95, 0x97, 0x56, 0x78, 0xbd, 0xa9, 0x98);











interface DX_DECLARE_INTERFACE("e8f7fe7a-191c-466d-ad95-975678bda998") ID2D1DeviceContext  : public ID2D1RenderTarget
{
    
    
    
    
    
    
    STDMETHOD(CreateBitmap)() PURE;
    
    
    
    
    STDMETHOD(CreateBitmapFromWicBitmap)() PURE;    
    
    
    
    
    
    
    
    STDMETHOD(CreateColorContext)() PURE;
    
    STDMETHOD(CreateColorContextFromFilename)() PURE;
    
    STDMETHOD(CreateColorContextFromWicColorContext)() PURE;
    
    
    
    
    
    STDMETHOD(CreateBitmapFromDxgiSurface)() PURE;
    
    
    
    
    
    
    
    STDMETHOD(CreateEffect)() PURE;
    
    
    
    
    
    
    STDMETHOD(CreateGradientStopCollection)() PURE;
    
    
    
    
    
    STDMETHOD(CreateImageBrush)() PURE;
    
    STDMETHOD(CreateBitmapBrush)() PURE;

    
    
    
    STDMETHOD(CreateCommandList)() PURE;
    
    
    
    
    
    STDMETHOD_(BOOL, IsDxgiFormatSupported)() CONST PURE;
    
    
    
    
    
    STDMETHOD_(BOOL, IsBufferPrecisionSupported)() CONST PURE;
    
    
    
    
    
    
    STDMETHOD(GetImageLocalBounds)() CONST PURE;
    
    
    
    
    
    
    STDMETHOD(GetImageWorldBounds)() CONST PURE;
    
    
    
    
    
    
    STDMETHOD(GetGlyphRunWorldBounds)() CONST PURE;
    
    
    
    
    
    STDMETHOD_(void, GetDevice)() CONST PURE;
    
    
    
    
    
    
    
    STDMETHOD_(void, SetTarget)() PURE;
    
    
    
    
    
    STDMETHOD_(void, GetTarget)() CONST PURE;
    
    
    
    
    
    STDMETHOD_(void, SetRenderingControls)() PURE;
    
    
    
    
    
    
    STDMETHOD_(void, GetRenderingControls)() CONST PURE;
    
    
    
    
    
    STDMETHOD_(void, SetPrimitiveBlend)() PURE;
    
    
    
    
    
    STDMETHOD_(void, GetPrimitiveBlend)(
        ) CONST PURE;
    
    
    
    
    
    STDMETHOD_(void, SetUnitMode)() PURE;
    
    
    
    
    
    STDMETHOD_(void, GetUnitMode)(
        ) CONST PURE;
    
    
    
    
    
    STDMETHOD_(void, DrawGlyphRun)() PURE;
    
    
    
    
    
    STDMETHOD_(void, DrawImage)() PURE;
    
    
    
    
    
    STDMETHOD_(void, DrawGdiMetafile)() PURE;
    
    STDMETHOD_(void, DrawBitmap)() PURE;
    
    
    
    
    
    STDMETHOD_(void, PushLayer)(
        _In_ CONST D2D1_LAYER_PARAMETERS1 *layerParameters,
        _In_opt_ ID2D1Layer *layer 
        ) PURE;
    
    using ID2D1RenderTarget::PushLayer;
    
    
    
    
    
    
    STDMETHOD(InvalidateEffectInputRectangle)() PURE;
    
    
    
    
    
    
    STDMETHOD(GetEffectInvalidRectangleCount)() PURE;
    
    
    
    
    
    STDMETHOD(GetEffectInvalidRectangles)() PURE;
    
    
    
    
    
    
    STDMETHOD(GetEffectRequiredInputRectangles)() PURE;
    
    
    
    
    
    
    
    STDMETHOD_(void, FillOpacityMask)() PURE;
    
 
    HRESULT CreateBitmap1() { return S_OK; }
    HRESULT CreateBitmap2() { return S_OK; }
    HRESULT CreateBitmap3() { return S_OK; }
    HRESULT CreateBitmap4() { return S_OK; }
    
    HRESULT CreateImageBrush1() { return S_OK; }
    HRESULT CreateImageBrush2() { return S_OK; }
    
    HRESULT CreateBitmapBrush1() { return S_OK; }
    HRESULT CreateBitmapBrush2() { return S_OK; }
    HRESULT CreateBitmapBrush3() { return S_OK; }

    
    
    
    void DrawImage1() {}
    void DrawImage2() {}
    void DrawImage3() {}
    void DrawImage4() {}
    void DrawImage5() {}
    void DrawImage6() {}
    void DrawImage7() {}
    
    void
    PushLayer(
        CONST D2D1_LAYER_PARAMETERS1 &layerParameters,
        _In_opt_ ID2D1Layer *layer 
        )  
    {
        PushLayer(&layerParameters, layer);
    }
    
    void DrawGdiMetafile1() {}
    
    void DrawBitmap1() {}
    void DrawBitmap2() {}
    void DrawBitmap3() {}
    
    void FillOpacityMask1() {}
    void FillOpacityMask2() {}   
    
    
    
    
    void SetRenderingControls1() {}
}; 

#endif 
