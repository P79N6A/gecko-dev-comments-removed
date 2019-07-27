




#ifndef MOZILLA_GFX_EXTENDINPUTEFFECTD2D1_H_
#define MOZILLA_GFX_EXTENDINPUTEFFECTD2D1_H_

#include <d2d1_1.h>
#include <d2d1effectauthor.h>
#include <d2d1effecthelpers.h>

#include "2D.h"
#include "mozilla/Attributes.h"


DEFINE_GUID(CLSID_ExtendInputEffect,
0x5fb55c7c, 0xd795, 0x4ba3, 0xa9, 0x5c, 0x22, 0x82, 0x5d, 0x0c, 0x4d, 0xf7);

namespace mozilla {
namespace gfx {

enum {
  EXTENDINPUT_PROP_OUTPUT_RECT = 0
};










class ExtendInputEffectD2D1 MOZ_FINAL : public ID2D1EffectImpl
                                      , public ID2D1DrawTransform
{
public:
  
  IFACEMETHODIMP Initialize(ID2D1EffectContext* pContextInternal, ID2D1TransformGraph* pTransformGraph);
  IFACEMETHODIMP PrepareForRender(D2D1_CHANGE_TYPE changeType);
  IFACEMETHODIMP SetGraph(ID2D1TransformGraph* pGraph);

  
  IFACEMETHODIMP_(ULONG) AddRef();
  IFACEMETHODIMP_(ULONG) Release();
  IFACEMETHODIMP QueryInterface(REFIID riid, void** ppOutput);

  
  IFACEMETHODIMP MapInputRectsToOutputRect(const D2D1_RECT_L* pInputRects,
                                           const D2D1_RECT_L* pInputOpaqueSubRects,
                                           UINT32 inputRectCount,
                                           D2D1_RECT_L* pOutputRect,
                                           D2D1_RECT_L* pOutputOpaqueSubRect);
  IFACEMETHODIMP MapOutputRectToInputRects(const D2D1_RECT_L* pOutputRect,
                                           D2D1_RECT_L* pInputRects,
                                           UINT32 inputRectCount) const;
  IFACEMETHODIMP MapInvalidRect(UINT32 inputIndex,
                                D2D1_RECT_L invalidInputRect,
                                D2D1_RECT_L* pInvalidOutputRect) const;

  
  IFACEMETHODIMP_(UINT32) GetInputCount() const { return 1; }

  
  IFACEMETHODIMP SetDrawInfo(ID2D1DrawInfo *pDrawInfo) { return S_OK; }

  static HRESULT Register(ID2D1Factory1* aFactory);
  static void Unregister(ID2D1Factory1* aFactory);
  static HRESULT __stdcall CreateEffect(IUnknown** aEffectImpl);

  HRESULT SetOutputRect(D2D1_VECTOR_4F aOutputRect)
  { mOutputRect = aOutputRect; return S_OK; }
  D2D1_VECTOR_4F GetOutputRect() const { return mOutputRect; }

private:
  ExtendInputEffectD2D1();

  uint32_t mRefCount;
  D2D1_VECTOR_4F mOutputRect;
};

}
}
#undef SIMPLE_PROP

#endif
