




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

void
AutoMaskData::Construct(const gfx::Matrix& aTransform,
                        const SurfaceDescriptor& aSurface)
{
  MOZ_ASSERT(!IsConstructed());
  mTransform = aTransform;
  mSurfaceOpener.construct(OPEN_READ_ONLY, aSurface);
}

gfxASurface*
AutoMaskData::GetSurface()
{
  MOZ_ASSERT(IsConstructed());
  if (mSurface) {
    return mSurface.get();
  }
  return mSurfaceOpener.ref().Get();
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
  return !!mSurface || !mSurfaceOpener.empty();
}


bool
GetMaskData(Layer* aMaskLayer, AutoMaskData* aMaskData)
{
  if (aMaskLayer) {
    nsRefPtr<gfxASurface> surface;
    SurfaceDescriptor descriptor;
    if (static_cast<BasicImplData*>(aMaskLayer->ImplData())
        ->GetAsSurface(getter_AddRefs(surface), &descriptor) &&
        (surface || IsSurfaceDescriptorValid(descriptor))) {
      Matrix transform;
      Matrix4x4 effectiveTransform = aMaskLayer->GetEffectiveTransform();
      DebugOnly<bool> maskIs2D = effectiveTransform.CanDraw2D(&transform);
      NS_ASSERTION(maskIs2D, "How did we end up with a 3D transform here?!");
      if (surface) {
        aMaskData->Construct(transform, surface);
      } else {
        aMaskData->Construct(transform, descriptor);
      }
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
FillWithMask(gfxContext* aContext, float aOpacity, Layer* aMaskLayer)
{
  AutoMaskData mask;
  if (GetMaskData(aMaskLayer, &mask)) {
    if (aOpacity < 1.0) {
      aContext->PushGroup(gfxContentType::COLOR_ALPHA);
      aContext->FillWithOpacity(aOpacity);
      aContext->PopGroupToSource();
      aContext->SetMatrix(ThebesMatrix(mask.GetTransform()));
      aContext->Mask(mask.GetSurface());
    } else {
      aContext->Save();
      aContext->Clip();
      aContext->SetMatrix(ThebesMatrix(mask.GetTransform()));
      aContext->Mask(mask.GetSurface());
      aContext->NewPath();
      aContext->Restore();
    }
    return;
  }

  
  aContext->FillWithOpacity(aOpacity);
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
