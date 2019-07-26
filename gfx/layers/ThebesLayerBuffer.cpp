




#include "base/basictypes.h"

#include "ThebesLayerBuffer.h"
#include "Layers.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxUtils.h"
#include "ipc/AutoOpenSurface.h"
#include "nsDeviceContext.h"
#include "sampler.h"
#include <algorithm>

namespace mozilla {
namespace layers {

nsIntRect
ThebesLayerBuffer::GetQuadrantRectangle(XSide aXSide, YSide aYSide)
{
  
  
  nsIntPoint quadrantTranslation = -mBufferRotation;
  quadrantTranslation.x += aXSide == LEFT ? mBufferRect.width : 0;
  quadrantTranslation.y += aYSide == TOP ? mBufferRect.height : 0;
  return mBufferRect + quadrantTranslation;
}











void
ThebesLayerBuffer::DrawBufferQuadrant(gfxContext* aTarget,
                                      XSide aXSide, YSide aYSide,
                                      float aOpacity,
                                      gfxASurface* aMask,
                                      const gfxMatrix* aMaskTransform)
{
  
  
  
  
  nsIntRect quadrantRect = GetQuadrantRectangle(aXSide, aYSide);
  nsIntRect fillRect;
  if (!fillRect.IntersectRect(mBufferRect, quadrantRect))
    return;

  aTarget->NewPath();
  aTarget->Rectangle(gfxRect(fillRect.x, fillRect.y,
                             fillRect.width, fillRect.height),
                     true);

  gfxPoint quadrantTranslation(quadrantRect.x, quadrantRect.y);
  nsRefPtr<gfxPattern> pattern = new gfxPattern(EnsureBuffer());

#ifdef MOZ_GFX_OPTIMIZE_MOBILE
  gfxPattern::GraphicsFilter filter = gfxPattern::FILTER_NEAREST;
  pattern->SetFilter(filter);
#endif

  gfxContextMatrixAutoSaveRestore saveMatrix(aTarget);

  
  gfxMatrix transform;
  transform.Translate(-quadrantTranslation);

  pattern->SetMatrix(transform);
  aTarget->SetPattern(pattern);

  if (aMask) {
    if (aOpacity == 1.0) {
      aTarget->SetMatrix(*aMaskTransform);
      aTarget->Mask(aMask);
    } else {
      aTarget->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
      aTarget->Paint(aOpacity);
      aTarget->PopGroupToSource();
      aTarget->SetMatrix(*aMaskTransform);
      aTarget->Mask(aMask);
    }
  } else {
    if (aOpacity == 1.0) {
      aTarget->Fill();
    } else {
      aTarget->Save();
      aTarget->Clip();
      aTarget->Paint(aOpacity);
      aTarget->Restore();
    }
  }
}

void
ThebesLayerBuffer::DrawBufferWithRotation(gfxContext* aTarget, float aOpacity,
                                          gfxASurface* aMask,
                                          const gfxMatrix* aMaskTransform)
{
  PROFILER_LABEL("ThebesLayerBuffer", "DrawBufferWithRotation");
  
  
  DrawBufferQuadrant(aTarget, LEFT, TOP, aOpacity, aMask, aMaskTransform);
  DrawBufferQuadrant(aTarget, RIGHT, TOP, aOpacity, aMask, aMaskTransform);
  DrawBufferQuadrant(aTarget, LEFT, BOTTOM, aOpacity, aMask, aMaskTransform);
  DrawBufferQuadrant(aTarget, RIGHT, BOTTOM, aOpacity, aMask, aMaskTransform);
}

already_AddRefed<gfxContext>
ThebesLayerBuffer::GetContextForQuadrantUpdate(const nsIntRect& aBounds)
{
  nsRefPtr<gfxContext> ctx = new gfxContext(EnsureBuffer());

  
  int32_t xBoundary = mBufferRect.XMost() - mBufferRotation.x;
  int32_t yBoundary = mBufferRect.YMost() - mBufferRotation.y;
  XSide sideX = aBounds.XMost() <= xBoundary ? RIGHT : LEFT;
  YSide sideY = aBounds.YMost() <= yBoundary ? BOTTOM : TOP;
  nsIntRect quadrantRect = GetQuadrantRectangle(sideX, sideY);
  NS_ASSERTION(quadrantRect.Contains(aBounds), "Messed up quadrants");
  ctx->Translate(-gfxPoint(quadrantRect.x, quadrantRect.y));

  return ctx.forget();
}

gfxASurface::gfxContentType
ThebesLayerBuffer::BufferContentType()
{
  return mBuffer ? mBuffer->GetContentType() : mBufferProvider->ContentType();
}

bool
ThebesLayerBuffer::BufferSizeOkFor(const nsIntSize& aSize)
{
  return (aSize == mBufferRect.Size() ||
          (SizedToVisibleBounds != mBufferSizePolicy &&
           aSize < mBufferRect.Size()));
}

gfxASurface*
ThebesLayerBuffer::EnsureBuffer()
{
  if (!mBuffer && mBufferProvider) {
    mBuffer = mBufferProvider->Get();
  }
  return mBuffer;
}

bool
ThebesLayerBuffer::HaveBuffer()
{
  return mBuffer || mBufferProvider;
}

static void
WrapRotationAxis(int32_t* aRotationPoint, int32_t aSize)
{
  if (*aRotationPoint < 0) {
    *aRotationPoint += aSize;
  } else if (*aRotationPoint >= aSize) {
    *aRotationPoint -= aSize;
  }
}

static nsIntRect
ComputeBufferRect(const nsIntRect& aRequestedRect)
{
  nsIntRect rect(aRequestedRect);
  
  
  
  
  
  rect.width = std::max(aRequestedRect.width, 64);
  return rect;
}

ThebesLayerBuffer::PaintState
ThebesLayerBuffer::BeginPaint(ThebesLayer* aLayer, ContentType aContentType,
                              uint32_t aFlags)
{
  PaintState result;
  
  
  bool canHaveRotation = !(aFlags & (PAINT_WILL_RESAMPLE | PAINT_NO_ROTATION));

  nsIntRegion validRegion = aLayer->GetValidRegion();

  ContentType contentType;
  nsIntRegion neededRegion;
  bool canReuseBuffer;
  nsIntRect destBufferRect;

  while (true) {
    contentType = aContentType;
    neededRegion = aLayer->GetVisibleRegion();
    canReuseBuffer = HaveBuffer() && BufferSizeOkFor(neededRegion.GetBounds().Size());

    if (canReuseBuffer) {
      if (mBufferRect.Contains(neededRegion.GetBounds())) {
        
        destBufferRect = mBufferRect;
      } else if (neededRegion.GetBounds().Size() <= mBufferRect.Size()) {
        
        
        destBufferRect = nsIntRect(neededRegion.GetBounds().TopLeft(), mBufferRect.Size());
      } else {
        destBufferRect = neededRegion.GetBounds();
      }
    } else {
      
      destBufferRect = ComputeBufferRect(neededRegion.GetBounds());
    }

    if ((aFlags & PAINT_WILL_RESAMPLE) &&
        (!neededRegion.GetBounds().IsEqualInterior(destBufferRect) ||
         neededRegion.GetNumRects() > 1)) {
      
      contentType = gfxASurface::CONTENT_COLOR_ALPHA;

      
      
      neededRegion = destBufferRect;
    }

    if (HaveBuffer() && contentType != BufferContentType()) {
      
      
      result.mRegionToInvalidate = aLayer->GetValidRegion();
      validRegion.SetEmpty();
      Clear();
      
      
      continue;
    }

    break;
  }

  NS_ASSERTION(destBufferRect.Contains(neededRegion.GetBounds()),
               "Destination rect doesn't contain what we need to paint");

  result.mRegionToDraw.Sub(neededRegion, validRegion);
  if (result.mRegionToDraw.IsEmpty())
    return result;

  nsIntRect drawBounds = result.mRegionToDraw.GetBounds();
  nsRefPtr<gfxASurface> destBuffer;
  uint32_t bufferFlags = canHaveRotation ? ALLOW_REPEAT : 0;
  if (canReuseBuffer) {
    nsIntRect keepArea;
    if (keepArea.IntersectRect(destBufferRect, mBufferRect)) {
      
      
      
      nsIntPoint newRotation = mBufferRotation +
        (destBufferRect.TopLeft() - mBufferRect.TopLeft());
      WrapRotationAxis(&newRotation.x, mBufferRect.width);
      WrapRotationAxis(&newRotation.y, mBufferRect.height);
      NS_ASSERTION(nsIntRect(nsIntPoint(0,0), mBufferRect.Size()).Contains(newRotation),
                   "newRotation out of bounds");
      int32_t xBoundary = destBufferRect.XMost() - newRotation.x;
      int32_t yBoundary = destBufferRect.YMost() - newRotation.y;
      if ((drawBounds.x < xBoundary && xBoundary < drawBounds.XMost()) ||
          (drawBounds.y < yBoundary && yBoundary < drawBounds.YMost()) ||
          (newRotation != nsIntPoint(0,0) && !canHaveRotation)) {
        
        
        
        if (mBufferRotation == nsIntPoint(0,0)) {
          nsIntRect srcRect(nsIntPoint(0, 0), mBufferRect.Size());
          nsIntPoint dest = mBufferRect.TopLeft() - destBufferRect.TopLeft();
          EnsureBuffer()->MovePixels(srcRect, dest);
          result.mDidSelfCopy = true;
          
          
          mBufferRect = destBufferRect;
        } else {
          
          
          destBufferRect = ComputeBufferRect(neededRegion.GetBounds());
          destBuffer = CreateBuffer(contentType, destBufferRect.Size(), bufferFlags);
          if (!destBuffer)
            return result;
        }
      } else {
        mBufferRect = destBufferRect;
        mBufferRotation = newRotation;
      }
    } else {
      
      
      
      mBufferRect = destBufferRect;
      mBufferRotation = nsIntPoint(0,0);
    }
  } else {
    
    destBuffer = CreateBuffer(contentType, destBufferRect.Size(), bufferFlags);
    if (!destBuffer)
      return result;
  }
  NS_ASSERTION(!(aFlags & PAINT_WILL_RESAMPLE) || destBufferRect == neededRegion.GetBounds(),
               "If we're resampling, we need to validate the entire buffer");

  
  
  bool isClear = !HaveBuffer();

  if (destBuffer) {
    if (HaveBuffer()) {
      
      nsRefPtr<gfxContext> tmpCtx = new gfxContext(destBuffer);
      nsIntPoint offset = -destBufferRect.TopLeft();
      tmpCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
      tmpCtx->Translate(gfxPoint(offset.x, offset.y));
      DrawBufferWithRotation(tmpCtx, 1.0);
    }

    mBuffer = destBuffer.forget();
    mBufferRect = destBufferRect;
    mBufferRotation = nsIntPoint(0,0);
  }
  NS_ASSERTION(canHaveRotation || mBufferRotation == nsIntPoint(0,0),
               "Rotation disabled, but we have nonzero rotation?");

  nsIntRegion invalidate;
  invalidate.Sub(aLayer->GetValidRegion(), destBufferRect);
  result.mRegionToInvalidate.Or(result.mRegionToInvalidate, invalidate);

  result.mContext = GetContextForQuadrantUpdate(drawBounds);

  gfxUtils::ClipToRegionSnapped(result.mContext, result.mRegionToDraw);
  if (contentType == gfxASurface::CONTENT_COLOR_ALPHA && !isClear) {
    result.mContext->SetOperator(gfxContext::OPERATOR_CLEAR);
    result.mContext->Paint();
    result.mContext->SetOperator(gfxContext::OPERATOR_OVER);
  }
  return result;
}

}
}

