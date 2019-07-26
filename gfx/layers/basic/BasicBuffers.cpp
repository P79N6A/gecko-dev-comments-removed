




#include "BasicThebesLayer.h"
#include "BasicBuffers.h"
#include "gfxUtils.h"

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {


static bool
IsClippingCheap(gfxContext* aTarget, const nsIntRegion& aRegion)
{
  
  
  return !aTarget->CurrentMatrix().HasNonIntegerTranslation() &&
         aRegion.GetNumRects() <= 1; 
}

void
BasicThebesLayerBuffer::DrawTo(ThebesLayer* aLayer,
                               gfxContext* aTarget,
                               float aOpacity,
                               Layer* aMaskLayer)
{
  aTarget->Save();
  
  
  
  
  if (!aLayer->GetValidRegion().Contains(BufferRect()) ||
      (ToData(aLayer)->GetClipToVisibleRegion() &&
       !aLayer->GetVisibleRegion().Contains(BufferRect())) ||
      IsClippingCheap(aTarget, aLayer->GetEffectiveVisibleRegion())) {
    
    
    
    
    
    gfxUtils::ClipToRegionSnapped(aTarget, aLayer->GetEffectiveVisibleRegion());
  }

  
  
  AutoMaskData mask;
  if (GetMaskData(aMaskLayer, &mask)) {
    DrawBufferWithRotation(aTarget, aOpacity,
                           mask.GetSurface(), &mask.GetTransform());
  } else {
    DrawBufferWithRotation(aTarget, aOpacity);
  }
  aTarget->Restore();
}

already_AddRefed<gfxASurface>
BasicThebesLayerBuffer::CreateBuffer(ContentType aType, 
                                     const nsIntSize& aSize, uint32_t aFlags)
{
  return mLayer->CreateBuffer(aType, aSize);
}

TemporaryRef<DrawTarget>
BasicThebesLayerBuffer::CreateDrawTarget(const IntSize& aSize, SurfaceFormat aFormat)
{
  return mLayer->CreateDrawTarget(aSize, aFormat);
}

void
BasicThebesLayerBuffer::SetBackingBufferAndUpdateFrom(
  gfxASurface* aBuffer,
  gfxASurface* aSource, const nsIntRect& aRect, const nsIntPoint& aRotation,
  const nsIntRegion& aUpdateRegion)
{
  SetBackingBuffer(aBuffer, aRect, aRotation);
  nsRefPtr<gfxContext> destCtx =
    GetContextForQuadrantUpdate(aUpdateRegion.GetBounds());
  if (!destCtx) {
    return;
  }
  destCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
  if (IsClippingCheap(destCtx, aUpdateRegion)) {
    gfxUtils::ClipToRegion(destCtx, aUpdateRegion);
  }

  BasicThebesLayerBuffer srcBuffer(aSource, aRect, aRotation);
  srcBuffer.DrawBufferWithRotation(destCtx, 1.0);
}

void
BasicThebesLayerBuffer::SetBackingBufferAndUpdateFrom(
  DrawTarget* aBuffer,
  DrawTarget* aSource, const nsIntRect& aRect, const nsIntPoint& aRotation,
  const nsIntRegion& aUpdateRegion)
{
  SetBackingBuffer(aBuffer, aRect, aRotation);
  nsRefPtr<gfxContext> destCtx =
    GetContextForQuadrantUpdate(aUpdateRegion.GetBounds());
  destCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
  if (IsClippingCheap(destCtx, aUpdateRegion)) {
    gfxUtils::ClipToRegion(destCtx, aUpdateRegion);
  }

  BasicThebesLayerBuffer srcBuffer(aSource, aRect, aRotation);
  srcBuffer.DrawBufferWithRotation(destCtx, 1.0);
}

}
}
