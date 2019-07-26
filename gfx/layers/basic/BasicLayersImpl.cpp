




#include "BasicLayersImpl.h"
#include <new>                          
#include "Layers.h"                     
#include "basic/BasicImplData.h"        
#include "mozilla/Assertions.h"         
#include "mozilla/DebugOnly.h"          
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/ISurfaceAllocator.h"
#include "AutoMaskData.h"

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

void
AutoMaskData::Construct(const gfx::Matrix& aTransform,
                        gfxASurface* aSurface)
{
  MOZ_ASSERT(!IsConstructed());
  mTransform = aTransform;
  mSurface = aSurface;
}

gfxASurface*
AutoMaskData::GetSurface()
{
  MOZ_ASSERT(IsConstructed());
  return mSurface.get();
}

const gfx::Matrix&
AutoMaskData::GetTransform()
{
  MOZ_ASSERT(IsConstructed());
  return mTransform;
}

bool
AutoMaskData::IsConstructed()
{
  return !!mSurface;
}

bool
GetMaskData(Layer* aMaskLayer, AutoMaskData* aMaskData)
{
  if (aMaskLayer) {
    nsRefPtr<gfxASurface> surface;
    if (static_cast<BasicImplData*>(aMaskLayer->ImplData())
        ->GetAsSurface(getter_AddRefs(surface)) &&
        surface) {
      Matrix transform;
      Matrix4x4 effectiveTransform = aMaskLayer->GetEffectiveTransform();
      DebugOnly<bool> maskIs2D = effectiveTransform.CanDraw2D(&transform);
      NS_ASSERTION(maskIs2D, "How did we end up with a 3D transform here?!");
      aMaskData->Construct(transform, surface);
      return true;
    }
  }
  return false;
}

bool
GetMaskData(Layer* aMaskLayer, AutoMoz2DMaskData* aMaskData)
{
  if (aMaskLayer) {
    RefPtr<SourceSurface> surface =
      static_cast<BasicImplData*>(aMaskLayer->ImplData())->GetAsSourceSurface();
    if (surface) {
      Matrix transform;
      Matrix4x4 effectiveTransform = aMaskLayer->GetEffectiveTransform();
      DebugOnly<bool> maskIs2D = effectiveTransform.CanDraw2D(&transform);
      NS_ASSERTION(maskIs2D, "How did we end up with a 3D transform here?!");
      aMaskData->Construct(transform, surface);
      return true;
    }
  }
  return false;
}

void
PaintWithMask(gfxContext* aContext, float aOpacity, Layer* aMaskLayer)
{
  AutoMaskData mask;
  if (GetMaskData(aMaskLayer, &mask)) {
    if (aOpacity < 1.0) {
      aContext->PushGroup(gfxContentType::COLOR_ALPHA);
      aContext->Paint(aOpacity);
      aContext->PopGroupToSource();
    }
    aContext->SetMatrix(ThebesMatrix(mask.GetTransform()));
    aContext->Mask(mask.GetSurface());
    return;
  }

  
  aContext->Paint(aOpacity);
}

void
FillRectWithMask(DrawTarget* aDT,
                 const Rect& aRect,
                 const Color& aColor,
                 const DrawOptions& aOptions,
                 Layer* aMaskLayer)
{
  AutoMoz2DMaskData mask;
  if (GetMaskData(aMaskLayer, &mask)) {
    aDT->PushClipRect(aRect);
    Matrix oldTransform = aDT->GetTransform();

    aDT->SetTransform(mask.GetTransform());
    aDT->MaskSurface(ColorPattern(aColor), mask.GetSurface(),
                     Point(0, 0), aOptions);
    aDT->SetTransform(oldTransform);
    aDT->PopClip();
    return;
  }

  aDT->FillRect(aRect, ColorPattern(aColor), aOptions);
}

void
FillRectWithMask(DrawTarget* aDT,
                 const Rect& aRect,
                 SourceSurface* aSurface,
                 Filter aFilter,
                 const DrawOptions& aOptions,
                 Layer* aMaskLayer)
{
  AutoMoz2DMaskData mask;
  if (GetMaskData(aMaskLayer, &mask)) {
    aDT->PushClipRect(aRect);
    Matrix oldTransform = aDT->GetTransform();
    Matrix transform = oldTransform;

    Matrix inverseMask = mask.GetTransform();
    inverseMask.Invert();

    transform *= inverseMask;

    SurfacePattern source(aSurface, ExtendMode::CLAMP, transform, aFilter);

    aDT->SetTransform(mask.GetTransform());
    aDT->MaskSurface(source, mask.GetSurface(), Point(0, 0), aOptions);
    aDT->SetTransform(oldTransform);
    aDT->PopClip();
    return;
  }

  aDT->FillRect(aRect, SurfacePattern(aSurface, ExtendMode::CLAMP, Matrix(), aFilter), aOptions);
}

BasicImplData*
ToData(Layer* aLayer)
{
  return static_cast<BasicImplData*>(aLayer->ImplData());
}

gfx::CompositionOp
GetEffectiveOperator(Layer* aLayer)
{
  CompositionOp op = aLayer->GetEffectiveMixBlendMode();

  if (op != CompositionOp::OP_OVER) {
    return op;
  }

  return ToData(aLayer)->GetOperator();
}

ShadowableLayer*
ToShadowable(Layer* aLayer)
{
  return aLayer->AsShadowableLayer();
}

bool
ShouldShadow(Layer* aLayer)
{
  if (!ToShadowable(aLayer)) {
    NS_ABORT_IF_FALSE(aLayer->GetType() == Layer::TYPE_READBACK,
                      "Only expect not to shadow ReadbackLayers");
    return false;
  }
  return true;
}


}
}
