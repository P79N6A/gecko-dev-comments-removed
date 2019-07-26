




#include "RotatedBuffer.h"
#include <sys/types.h>                  
#include <algorithm>                    
#include "BasicImplData.h"              
#include "BasicLayersImpl.h"            
#include "BufferUnrotate.h"             
#include "GeckoProfiler.h"              
#include "Layers.h"                     
#include "gfxContext.h"                 
#include "gfxMatrix.h"                  
#include "gfxPlatform.h"                
#include "gfxPoint.h"                   
#include "gfxUtils.h"                   
#include "mozilla/Util.h"               
#include "mozilla/gfx/BasePoint.h"      
#include "mozilla/gfx/BaseRect.h"       
#include "mozilla/gfx/BaseSize.h"       
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/ShadowLayers.h"  
#include "mozilla/layers/TextureClient.h"  
#include "nsSize.h"                     
#include "gfx2DGlue.h"

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

Rect
RotatedBuffer::GetSourceRectangle(XSide aXSide, YSide aYSide) const
{
  Rect result;
  if (aXSide == LEFT) {
    result.x = 0;
    result.width = mBufferRotation.x;
  } else {
    result.x = mBufferRotation.x;
    result.width = mBufferRect.width - mBufferRotation.x;
  }
  if (aYSide == TOP) {
    result.y = 0;
    result.height = mBufferRotation.y;
  } else {
    result.y = mBufferRotation.y;
    result.height = mBufferRect.height - mBufferRotation.y;
  }
  return result;
}











void
RotatedBuffer::DrawBufferQuadrant(gfx::DrawTarget* aTarget,
                                  XSide aXSide, YSide aYSide,
                                  ContextSource aSource,
                                  float aOpacity,
                                  gfx::CompositionOp aOperator,
                                  gfx::SourceSurface* aMask,
                                  const gfx::Matrix* aMaskTransform) const
{
  
  
  
  
  nsIntRect quadrantRect = GetQuadrantRectangle(aXSide, aYSide);
  nsIntRect fillRect;
  if (!fillRect.IntersectRect(mBufferRect, quadrantRect))
    return;

  gfx::Point quadrantTranslation(quadrantRect.x, quadrantRect.y);

  MOZ_ASSERT(aOperator == OP_OVER || aOperator == OP_SOURCE);
  
  
  
  
  
  if (aTarget->GetType() == BACKEND_DIRECT2D && aOperator == OP_SOURCE) {
    aOperator = OP_OVER;
    if (mDTBuffer->GetFormat() == FORMAT_B8G8R8A8) {
      aTarget->ClearRect(ToRect(fillRect));
    }
  }

  RefPtr<gfx::SourceSurface> snapshot;
  if (aSource == BUFFER_BLACK) {
    snapshot = mDTBuffer->Snapshot();
  } else {
    MOZ_ASSERT(aSource == BUFFER_WHITE);
    snapshot = mDTBufferOnWhite->Snapshot();
  }

  if (aOperator == OP_SOURCE) {
    
    
    
    aTarget->PushClipRect(gfx::Rect(fillRect.x, fillRect.y,
                                    fillRect.width, fillRect.height));
  }

  if (aMask) {
    
    Matrix transform;
    transform.Translate(quadrantTranslation.x, quadrantTranslation.y);

#ifdef MOZ_GFX_OPTIMIZE_MOBILE
    SurfacePattern source(snapshot, EXTEND_CLAMP, transform, FILTER_POINT);
#else
    SurfacePattern source(snapshot, EXTEND_CLAMP, transform);
#endif

    Matrix oldTransform = aTarget->GetTransform();
    aTarget->SetTransform(*aMaskTransform);
    aTarget->MaskSurface(source, aMask, Point(0, 0), DrawOptions(aOpacity, aOperator));
    aTarget->SetTransform(oldTransform);
  } else {
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
    DrawSurfaceOptions options(FILTER_POINT);
#else
    DrawSurfaceOptions options;
#endif
    aTarget->DrawSurface(snapshot, ToRect(fillRect),
                         GetSourceRectangle(aXSide, aYSide),
                         options,
                         DrawOptions(aOpacity, aOperator));
  }

  if (aOperator == OP_SOURCE) {
    aTarget->PopClip();
  }

  aTarget->Flush();
}

void
RotatedBuffer::DrawBufferWithRotation(gfx::DrawTarget *aTarget, ContextSource aSource,
                                      float aOpacity,
                                      gfx::CompositionOp aOperator,
                                      gfx::SourceSurface* aMask,
                                      const gfx::Matrix* aMaskTransform) const
{
  PROFILER_LABEL("RotatedBuffer", "DrawBufferWithRotation");
  
  
  
  DrawBufferQuadrant(aTarget, LEFT, TOP, aSource, aOpacity, aOperator, aMask, aMaskTransform);
  DrawBufferQuadrant(aTarget, RIGHT, TOP, aSource, aOpacity, aOperator, aMask, aMaskTransform);
  DrawBufferQuadrant(aTarget, LEFT, BOTTOM, aSource, aOpacity, aOperator, aMask, aMaskTransform);
  DrawBufferQuadrant(aTarget, RIGHT, BOTTOM, aSource, aOpacity, aOperator,aMask, aMaskTransform);
}

 bool
RotatedContentBuffer::IsClippingCheap(gfxContext* aTarget, const nsIntRegion& aRegion)
{
  
  
  return !aTarget->CurrentMatrix().HasNonIntegerTranslation() &&
         aRegion.GetNumRects() <= 1;
}

void
RotatedContentBuffer::DrawTo(ThebesLayer* aLayer,
                             gfxContext* aTarget,
                             float aOpacity,
                             gfxASurface* aMask,
                             const gfxMatrix* aMaskTransform)
{
  if (!EnsureBuffer()) {
    return;
  }

  RefPtr<DrawTarget> dt = aTarget->GetDrawTarget();
  MOZ_ASSERT(dt, "Did you pass a non-Azure gfxContext?");
  bool clipped = false;

  
  
  
  
  if (!aLayer->GetValidRegion().Contains(BufferRect()) ||
      (ToData(aLayer)->GetClipToVisibleRegion() &&
       !aLayer->GetVisibleRegion().Contains(BufferRect())) ||
      IsClippingCheap(aTarget, aLayer->GetEffectiveVisibleRegion())) {
    
    
    
    
    
    gfxUtils::ClipToRegionSnapped(dt, aLayer->GetEffectiveVisibleRegion());
    clipped = true;
  }

  RefPtr<gfx::SourceSurface> mask;
  if (aMask) {
    mask = gfxPlatform::GetPlatform()->GetSourceSurfaceForSurface(dt, aMask);
  }

  Matrix maskTransform;
  if (aMaskTransform) {
    maskTransform = ToMatrix(*aMaskTransform);
  }

  CompositionOp op = CompositionOpForOp(aTarget->CurrentOperator());
  DrawBufferWithRotation(dt, BUFFER_BLACK, aOpacity, op, mask, &maskTransform);
  if (clipped) {
    dt->PopClip();
  }
}

already_AddRefed<gfxContext>
RotatedContentBuffer::GetContextForQuadrantUpdate(const nsIntRect& aBounds,
                                                  ContextSource aSource,
                                                  nsIntPoint *aTopLeft)
{
  if (!EnsureBuffer()) {
    return nullptr;
  }

  nsRefPtr<gfxContext> ctx;
  if (aSource == BUFFER_BOTH && HaveBufferOnWhite()) {
    if (!EnsureBufferOnWhite()) {
      return nullptr;
    }
    MOZ_ASSERT(mDTBuffer && mDTBufferOnWhite);
    RefPtr<DrawTarget> dualDT = Factory::CreateDualDrawTarget(mDTBuffer, mDTBufferOnWhite);
    ctx = new gfxContext(dualDT);
  } else if (aSource == BUFFER_WHITE) {
    if (!EnsureBufferOnWhite()) {
      return nullptr;
    }
    ctx = new gfxContext(mDTBufferOnWhite);
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

  if (aTopLeft) {
    *aTopLeft = nsIntPoint(quadrantRect.x, quadrantRect.y);
  }

  return ctx.forget();
}

gfxContentType
RotatedContentBuffer::BufferContentType()
{
  if (mDeprecatedBufferProvider) {
    return mDeprecatedBufferProvider->GetContentType();
  }
  if (mBufferProvider || mDTBuffer) {
    SurfaceFormat format;

    if (mBufferProvider) {
      format = mBufferProvider->AsTextureClientDrawTarget()->GetFormat();
    } else if (mDTBuffer) {
      format = mDTBuffer->GetFormat();
    }

    return ContentForFormat(format);
  }
  return GFX_CONTENT_SENTINEL;
}

bool
RotatedContentBuffer::BufferSizeOkFor(const nsIntSize& aSize)
{
  return (aSize == mBufferRect.Size() ||
          (SizedToVisibleBounds != mBufferSizePolicy &&
           aSize < mBufferRect.Size()));
}

bool
RotatedContentBuffer::EnsureBuffer()
{
  if (!mDTBuffer) {
    if (mDeprecatedBufferProvider) {
      mDTBuffer = mDeprecatedBufferProvider->LockDrawTarget();
    } else if (mBufferProvider) {
      mDTBuffer = mBufferProvider->AsTextureClientDrawTarget()->GetAsDrawTarget();
    }
  }

  NS_WARN_IF_FALSE(mDTBuffer, "no buffer");
  return !!mDTBuffer;
}

bool
RotatedContentBuffer::EnsureBufferOnWhite()
{
  if (!mDTBufferOnWhite) {
    if (mDeprecatedBufferProviderOnWhite) {
      mDTBufferOnWhite = mDeprecatedBufferProviderOnWhite->LockDrawTarget();
    } else if (mBufferProviderOnWhite) {
      mDTBufferOnWhite =
        mBufferProviderOnWhite->AsTextureClientDrawTarget()->GetAsDrawTarget();
    }
  }

  NS_WARN_IF_FALSE(mDTBufferOnWhite, "no buffer");
  return mDTBufferOnWhite;
}

bool
RotatedContentBuffer::HaveBuffer() const
{
  return mDTBuffer || mDeprecatedBufferProvider || mBufferProvider;
}

bool
RotatedContentBuffer::HaveBufferOnWhite() const
{
  return mDTBufferOnWhite || mDeprecatedBufferProviderOnWhite || mBufferProviderOnWhite;
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
#ifdef MOZ_WIDGET_GONK
  
  
  
  
  
  
  
  
  
  if (rect.height > 0) {
    rect.height = std::max(aRequestedRect.height, 32);
  }
#endif
  return rect;
}

RotatedContentBuffer::PaintState
RotatedContentBuffer::BeginPaint(ThebesLayer* aLayer, ContentType aContentType,
                                 uint32_t aFlags)
{
  PaintState result;
  
  
  bool canHaveRotation = gfxPlatform::BufferRotationEnabled() &&
                         !(aFlags & (PAINT_WILL_RESAMPLE | PAINT_NO_ROTATION));

  nsIntRegion validRegion = aLayer->GetValidRegion();

  Layer::SurfaceMode mode;
  ContentType contentType;
  nsIntRegion neededRegion;
  bool canReuseBuffer;
  nsIntRect destBufferRect;

  while (true) {
    mode = aLayer->GetSurfaceMode();
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

    if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
#if defined(MOZ_GFX_OPTIMIZE_MOBILE) || defined(MOZ_WIDGET_GONK)
      mode = Layer::SURFACE_SINGLE_CHANNEL_ALPHA;
#else
      if (!aLayer->GetParent() ||
          !aLayer->GetParent()->SupportsComponentAlphaChildren() ||
          !aLayer->Manager()->IsCompositingCheap() ||
          !aLayer->AsShadowableLayer() ||
          !aLayer->AsShadowableLayer()->HasShadow() ||
          !gfxPlatform::ComponentAlphaEnabled()) {
        mode = Layer::SURFACE_SINGLE_CHANNEL_ALPHA;
      } else {
        contentType = GFX_CONTENT_COLOR;
      }
#endif
    }

    if ((aFlags & PAINT_WILL_RESAMPLE) &&
        (!neededRegion.GetBounds().IsEqualInterior(destBufferRect) ||
         neededRegion.GetNumRects() > 1)) {
      
      if (mode == Layer::SURFACE_OPAQUE) {
        contentType = GFX_CONTENT_COLOR_ALPHA;
        mode = Layer::SURFACE_SINGLE_CHANNEL_ALPHA;
      }

      
      
      neededRegion = destBufferRect;
    }

    
    
    if (HaveBuffer() &&
        (contentType != BufferContentType() ||
        (mode == Layer::SURFACE_COMPONENT_ALPHA) != HaveBufferOnWhite())) {

      
      
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
  RefPtr<DrawTarget> destDTBuffer;
  RefPtr<DrawTarget> destDTBufferOnWhite;
  uint32_t bufferFlags = canHaveRotation ? ALLOW_REPEAT : 0;
  if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
    bufferFlags |= BUFFER_COMPONENT_ALPHA;
  }
  if (canReuseBuffer) {
    if (!EnsureBuffer()) {
      return result;
    }
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
          MOZ_ASSERT(mDTBuffer);
          mDTBuffer->CopyRect(IntRect(srcRect.x, srcRect.y, srcRect.width, srcRect.height),
                              IntPoint(dest.x, dest.y));
          if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
            if (!EnsureBufferOnWhite()) {
              return result;
            }
            MOZ_ASSERT(mDTBufferOnWhite);
            mDTBufferOnWhite->CopyRect(IntRect(srcRect.x, srcRect.y, srcRect.width, srcRect.height),
                                       IntPoint(dest.x, dest.y));
          }
          result.mDidSelfCopy = true;
          mDidSelfCopy = true;
          
          
          mBufferRect = destBufferRect;
        } else {
          
          
          unsigned char* data;
          IntSize size;
          int32_t stride;
          SurfaceFormat format;

          if (mDTBuffer->LockBits(&data, &size, &stride, &format)) {
            uint8_t bytesPerPixel = BytesPerPixel(format);
            BufferUnrotate(data,
                           size.width * bytesPerPixel,
                           size.height, stride,
                           newRotation.x * bytesPerPixel, newRotation.y);
            mDTBuffer->ReleaseBits(data);

            if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
              if (!EnsureBufferOnWhite()) {
                return result;
              }
              MOZ_ASSERT(mDTBufferOnWhite);
              mDTBufferOnWhite->LockBits(&data, &size, &stride, &format);
              uint8_t bytesPerPixel = BytesPerPixel(format);
              BufferUnrotate(data,
                             size.width * bytesPerPixel,
                             size.height, stride,
                             newRotation.x * bytesPerPixel, newRotation.y);
              mDTBufferOnWhite->ReleaseBits(data);
            }

            
            
            result.mDidSelfCopy = true;
            mDidSelfCopy = true;
            mBufferRect = destBufferRect;
            mBufferRotation = nsIntPoint(0, 0);
          }

          if (!result.mDidSelfCopy) {
            destBufferRect = ComputeBufferRect(neededRegion.GetBounds());
            CreateBuffer(contentType, destBufferRect, bufferFlags,
                         &destDTBuffer, &destDTBufferOnWhite);
            if (!destDTBuffer) {
              return result;
            }
          }
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
    
    CreateBuffer(contentType, destBufferRect, bufferFlags,
                 &destDTBuffer, &destDTBufferOnWhite);
    if (!destDTBuffer) {
      return result;
    }
  }

  NS_ASSERTION(!(aFlags & PAINT_WILL_RESAMPLE) || destBufferRect == neededRegion.GetBounds(),
               "If we're resampling, we need to validate the entire buffer");

  
  
  bool isClear = !HaveBuffer();

  if (destDTBuffer) {
    if (!isClear && (mode != Layer::SURFACE_COMPONENT_ALPHA || HaveBufferOnWhite())) {
      
      nsIntPoint offset = -destBufferRect.TopLeft();
      Matrix mat;
      mat.Translate(offset.x, offset.y);
      destDTBuffer->SetTransform(mat);
      if (!EnsureBuffer()) {
        return result;
      }
       MOZ_ASSERT(mDTBuffer, "Have we got a Thebes buffer for some reason?");
      DrawBufferWithRotation(destDTBuffer, BUFFER_BLACK, 1.0, OP_SOURCE);
      destDTBuffer->SetTransform(Matrix());

      if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
        NS_ASSERTION(destDTBufferOnWhite, "Must have a white buffer!");
        destDTBufferOnWhite->SetTransform(mat);
        if (!EnsureBufferOnWhite()) {
          return result;
        }
        MOZ_ASSERT(mDTBufferOnWhite, "Have we got a Thebes buffer for some reason?");
        DrawBufferWithRotation(destDTBufferOnWhite, BUFFER_WHITE, 1.0, OP_SOURCE);
        destDTBufferOnWhite->SetTransform(Matrix());
      }
    }

    mDTBuffer = destDTBuffer.forget();
    mDTBufferOnWhite = destDTBufferOnWhite.forget();
    mBufferRect = destBufferRect;
    mBufferRotation = nsIntPoint(0,0);
  }
  NS_ASSERTION(canHaveRotation || mBufferRotation == nsIntPoint(0,0),
               "Rotation disabled, but we have nonzero rotation?");

  nsIntRegion invalidate;
  invalidate.Sub(aLayer->GetValidRegion(), destBufferRect);
  result.mRegionToInvalidate.Or(result.mRegionToInvalidate, invalidate);

  nsIntPoint topLeft;
  result.mContext = GetContextForQuadrantUpdate(drawBounds, BUFFER_BOTH, &topLeft);
  result.mClip = CLIP_DRAW_SNAPPED;

  if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
    MOZ_ASSERT(mDTBuffer && mDTBufferOnWhite);
    nsIntRegionRectIterator iter(result.mRegionToDraw);
    const nsIntRect *iterRect;
    while ((iterRect = iter.Next())) {
      mDTBuffer->FillRect(Rect(iterRect->x, iterRect->y, iterRect->width, iterRect->height),
                          ColorPattern(Color(0.0, 0.0, 0.0, 1.0)));
      mDTBufferOnWhite->FillRect(Rect(iterRect->x, iterRect->y, iterRect->width, iterRect->height),
                                 ColorPattern(Color(1.0, 1.0, 1.0, 1.0)));
    }
  } else if (contentType == GFX_CONTENT_COLOR_ALPHA && !isClear) {
    nsIntRegionRectIterator iter(result.mRegionToDraw);
    const nsIntRect *iterRect;
    while ((iterRect = iter.Next())) {
      result.mContext->GetDrawTarget()->ClearRect(Rect(iterRect->x, iterRect->y, iterRect->width, iterRect->height));
    }
  }

  return result;
}

}
}

