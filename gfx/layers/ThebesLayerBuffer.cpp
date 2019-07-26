




#include "base/basictypes.h"

#include "BasicLayersImpl.h"
#include "ThebesLayerBuffer.h"
#include "Layers.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxUtils.h"
#include "ipc/AutoOpenSurface.h"
#include "nsDeviceContext.h"
#include "GeckoProfiler.h"
#include <algorithm>

namespace mozilla {

using namespace gfx;

namespace layers {

nsIntRect
RotatedBuffer::GetQuadrantRectangle(XSide aXSide, YSide aYSide) const
{
  
  
  nsIntPoint quadrantTranslation = -mBufferRotation;
  quadrantTranslation.x += aXSide == LEFT ? mBufferRect.width : 0;
  quadrantTranslation.y += aYSide == TOP ? mBufferRect.height : 0;
  return mBufferRect + quadrantTranslation;
}











void
RotatedBuffer::DrawBufferQuadrant(gfxContext* aTarget,
                                  XSide aXSide, YSide aYSide,
                                  float aOpacity,
                                  gfxASurface* aMask,
                                  const gfxMatrix* aMaskTransform) const
{
  
  
  
  
  nsIntRect quadrantRect = GetQuadrantRectangle(aXSide, aYSide);
  nsIntRect fillRect;
  if (!fillRect.IntersectRect(mBufferRect, quadrantRect)) {
    return;
  }

  nsRefPtr<gfxASurface> source;

  if (mBuffer) {
    source = mBuffer;
  } else if (mDTBuffer) {
    source = gfxPlatform::GetPlatform()->GetThebesSurfaceForDrawTarget(mDTBuffer);
  } else {
    NS_RUNTIMEABORT("Can't draw a RotatedBuffer without any buffer!");
  }


  aTarget->NewPath();
  aTarget->Rectangle(gfxRect(fillRect.x, fillRect.y,
                             fillRect.width, fillRect.height),
                     true);

  gfxPoint quadrantTranslation(quadrantRect.x, quadrantRect.y);
  nsRefPtr<gfxPattern> pattern = new gfxPattern(source);

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

  nsRefPtr<gfxASurface> surf = aTarget->CurrentSurface();
  surf->Flush();
}











void
RotatedBuffer::DrawBufferQuadrant(gfx::DrawTarget* aTarget,
                                  XSide aXSide, YSide aYSide,
                                  float aOpacity,
                                  gfx::SourceSurface* aMask,
                                  const gfx::Matrix* aMaskTransform) const
{
  
  
  
  
  nsIntRect quadrantRect = GetQuadrantRectangle(aXSide, aYSide);
  nsIntRect fillRect;
  if (!fillRect.IntersectRect(mBufferRect, quadrantRect))
    return;

  gfx::Point quadrantTranslation(quadrantRect.x, quadrantRect.y);

  RefPtr<SourceSurface> snapshot = mDTBuffer->Snapshot();

  
  Matrix transform;
  transform.Translate(quadrantTranslation.x, quadrantTranslation.y);

#ifdef MOZ_GFX_OPTIMIZE_MOBILE
  SurfacePattern source(snapshot, EXTEND_CLAMP, transform, FILTER_POINT);
#else
  SurfacePattern source(snapshot, EXTEND_CLAMP, transform);
#endif

  if (aMask) {
    SurfacePattern mask(aMask, EXTEND_CLAMP, *aMaskTransform);

    aTarget->Mask(source, mask, DrawOptions(aOpacity));
  } else {
    aTarget->FillRect(gfx::Rect(fillRect.x, fillRect.y,
                                fillRect.width, fillRect.height),
                      source, DrawOptions(aOpacity));
  }

  aTarget->Flush();
}

void
RotatedBuffer::DrawBufferWithRotation(gfxContext* aTarget, float aOpacity,
                                      gfxASurface* aMask,
                                      const gfxMatrix* aMaskTransform) const
{
  PROFILER_LABEL("RotatedBuffer", "DrawBufferWithRotation");
  
  
  DrawBufferQuadrant(aTarget, LEFT, TOP, aOpacity, aMask, aMaskTransform);
  DrawBufferQuadrant(aTarget, RIGHT, TOP, aOpacity, aMask, aMaskTransform);
  DrawBufferQuadrant(aTarget, LEFT, BOTTOM, aOpacity, aMask, aMaskTransform);
  DrawBufferQuadrant(aTarget, RIGHT, BOTTOM, aOpacity, aMask, aMaskTransform);
}

void
RotatedBuffer::DrawBufferWithRotation(gfx::DrawTarget *aTarget, float aOpacity,
                                      gfx::SourceSurface* aMask,
                                      const gfx::Matrix* aMaskTransform) const
{
  PROFILER_LABEL("RotatedBuffer", "DrawBufferWithRotation");
  
  
  
  DrawBufferQuadrant(aTarget, LEFT, TOP, aOpacity, aMask, aMaskTransform);
  DrawBufferQuadrant(aTarget, RIGHT, TOP, aOpacity, aMask, aMaskTransform);
  DrawBufferQuadrant(aTarget, LEFT, BOTTOM, aOpacity, aMask, aMaskTransform);
  DrawBufferQuadrant(aTarget, RIGHT, BOTTOM, aOpacity, aMask, aMaskTransform);
}

 bool
ThebesLayerBuffer::IsClippingCheap(gfxContext* aTarget, const nsIntRegion& aRegion)
{
  
  
  return !aTarget->CurrentMatrix().HasNonIntegerTranslation() &&
         aRegion.GetNumRects() <= 1;
}

void
ThebesLayerBuffer::DrawTo(ThebesLayer* aLayer,
                          gfxContext* aTarget,
                          float aOpacity,
                          gfxASurface* aMask,
                          const gfxMatrix* aMaskTransform)
{
  EnsureBuffer();

  if (aTarget->IsCairo()) {
    aTarget->Save();
    
    
    
    
    if (!aLayer->GetValidRegion().Contains(BufferRect()) ||
        (ToData(aLayer)->GetClipToVisibleRegion() &&
         !aLayer->GetVisibleRegion().Contains(BufferRect())) ||
        IsClippingCheap(aTarget, aLayer->GetEffectiveVisibleRegion())) {
      
      
      
      
      
      gfxUtils::ClipToRegionSnapped(aTarget, aLayer->GetEffectiveVisibleRegion());
    }

    DrawBufferWithRotation(aTarget, aOpacity, aMask, aMaskTransform);
    aTarget->Restore();
  } else {
    RefPtr<DrawTarget> dt = aTarget->GetDrawTarget();

    
    
    
    
    if (!aLayer->GetValidRegion().Contains(BufferRect()) ||
        (ToData(aLayer)->GetClipToVisibleRegion() &&
         !aLayer->GetVisibleRegion().Contains(BufferRect())) ||
        IsClippingCheap(aTarget, aLayer->GetEffectiveVisibleRegion())) {
      
      
      
      
      
      gfxUtils::ClipToRegionSnapped(dt, aLayer->GetEffectiveVisibleRegion());
    }

    DrawBufferWithRotation(aTarget, aOpacity, aMask, aMaskTransform);
    aTarget->Restore();
   }
}

already_AddRefed<gfxContext>
ThebesLayerBuffer::GetContextForQuadrantUpdate(const nsIntRect& aBounds)
{
  EnsureBuffer();

  nsRefPtr<gfxContext> ctx;
  if (mBuffer) {
    ctx = new gfxContext(mBuffer);
  } else {
    ctx = new gfxContext(mDTBuffer);
  }

  
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
  if (mBuffer) {
    return mBuffer->GetContentType();
  }
  if (mBufferProvider) {
    return mBufferProvider->ContentType();
  }
  if (mTextureClientForBuffer) {
    return mTextureClientForBuffer->GetContentType();
  }
  if (mDTBuffer) {
    switch (mDTBuffer->GetFormat()) {
    case FORMAT_A8:
      return gfxASurface::CONTENT_ALPHA;
    case FORMAT_B8G8R8A8:
    case FORMAT_R8G8B8A8:
      return gfxASurface::CONTENT_COLOR_ALPHA;
    default:
      return gfxASurface::CONTENT_COLOR;
    }
  }
  return gfxASurface::CONTENT_SENTINEL;
}

bool
ThebesLayerBuffer::BufferSizeOkFor(const nsIntSize& aSize)
{
  return (aSize == mBufferRect.Size() ||
          (SizedToVisibleBounds != mBufferSizePolicy &&
           aSize < mBufferRect.Size()));
}

void
ThebesLayerBuffer::EnsureBuffer()
{
  MOZ_ASSERT(!mBufferProvider || !mTextureClientForBuffer,
             "Can't have both kinds of buffer provider.");
  if (!mBuffer && mBufferProvider) {
    mBuffer = mBufferProvider->Get();
  }
  if ((!mBuffer && !mDTBuffer) && mTextureClientForBuffer) {
    if (SupportsAzureContent()) {
      mDTBuffer = mTextureClientForBuffer->LockDrawTarget();
    } else {
      mBuffer = mTextureClientForBuffer->LockSurface();
    }
  }
}

bool
ThebesLayerBuffer::HaveBuffer()
{
  return mDTBuffer || mBuffer || mBufferProvider || mTextureClientForBuffer;
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
  RefPtr<DrawTarget> destDTBuffer;
  uint32_t bufferFlags = canHaveRotation ? ALLOW_REPEAT : 0;
  if (canReuseBuffer) {
    EnsureBuffer();
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
          if (mBuffer) {
            mBuffer->MovePixels(srcRect, dest);
          } else {
            RefPtr<SourceSurface> source = mDTBuffer->Snapshot();
            mDTBuffer->CopySurface(source,
                                   IntRect(srcRect.x, srcRect.y, srcRect.width, srcRect.height),
                                   IntPoint(dest.x, dest.y));
          }
          result.mDidSelfCopy = true;
          
          
          mBufferRect = destBufferRect;
        } else {
          
          
          destBufferRect = ComputeBufferRect(neededRegion.GetBounds());
          if (SupportsAzureContent()) {
            destDTBuffer = CreateDTBuffer(contentType, destBufferRect, bufferFlags);
          } else {
            destBuffer = CreateBuffer(contentType, destBufferRect, bufferFlags);
          }
          if (!destBuffer && !destDTBuffer)
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
    
    if (SupportsAzureContent()) {
      destDTBuffer = CreateDTBuffer(contentType, destBufferRect, bufferFlags);
    } else {
      destBuffer = CreateBuffer(contentType, destBufferRect, bufferFlags);
    }
    if (!destBuffer && !destDTBuffer)
      return result;
  }

  NS_ASSERTION(!(aFlags & PAINT_WILL_RESAMPLE) || destBufferRect == neededRegion.GetBounds(),
               "If we're resampling, we need to validate the entire buffer");

  
  
  bool isClear = !HaveBuffer();

  if (destBuffer) {
    if (!isClear) {
      
      nsRefPtr<gfxContext> tmpCtx = new gfxContext(destBuffer);
      nsIntPoint offset = -destBufferRect.TopLeft();
      tmpCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
      tmpCtx->Translate(gfxPoint(offset.x, offset.y));
      EnsureBuffer();
      DrawBufferWithRotation(tmpCtx, 1.0, nullptr, nullptr);
    }

    mBuffer = destBuffer.forget();
    mBufferRect = destBufferRect;
    mBufferRotation = nsIntPoint(0,0);
  } else if (destDTBuffer) {
    if (!isClear) {
      
      nsIntPoint offset = -destBufferRect.TopLeft();
      Matrix mat;
      mat.Translate(offset.x, offset.y);
      destDTBuffer->SetTransform(mat);
      EnsureBuffer();
      DrawBufferWithRotation(destDTBuffer, 1.0, nullptr, nullptr);
      destDTBuffer->SetTransform(Matrix());
    }

    mDTBuffer = destDTBuffer.forget();
    mBufferRect = destBufferRect;
    mBufferRotation = nsIntPoint(0,0);
  }
  NS_ASSERTION(canHaveRotation || mBufferRotation == nsIntPoint(0,0),
               "Rotation disabled, but we have nonzero rotation?");

  nsIntRegion invalidate;
  invalidate.Sub(aLayer->GetValidRegion(), destBufferRect);
  result.mRegionToInvalidate.Or(result.mRegionToInvalidate, invalidate);

  result.mContext = GetContextForQuadrantUpdate(drawBounds);

  if (contentType == gfxASurface::CONTENT_COLOR_ALPHA && !isClear) {
    if (result.mContext->IsCairo()) {
      gfxUtils::ClipToRegionSnapped(result.mContext, result.mRegionToDraw);
      result.mContext->SetOperator(gfxContext::OPERATOR_CLEAR);
      result.mContext->Paint();
      result.mContext->SetOperator(gfxContext::OPERATOR_OVER);
    } else {
      nsIntRegionRectIterator iter(result.mRegionToDraw);
      const nsIntRect *iterRect;
      while ((iterRect = iter.Next())) {
        result.mContext->GetDrawTarget()->ClearRect(Rect(iterRect->x, iterRect->y, iterRect->width, iterRect->height));
      }
      
      
      gfxUtils::ClipToRegionSnapped(result.mContext, result.mRegionToDraw);
    }
  } else {
    gfxUtils::ClipToRegionSnapped(result.mContext, result.mRegionToDraw);
  }

  return result;
}

}
}

