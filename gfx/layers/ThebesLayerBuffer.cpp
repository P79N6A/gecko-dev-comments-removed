




#include "ThebesLayerBuffer.h"
#include <sys/types.h>                  
#include <algorithm>                    
#include "BasicImplData.h"              
#include "BasicLayersImpl.h"            
#include "GeckoProfiler.h"              
#include "Layers.h"                     
#include "gfxColor.h"                   
#include "gfxContext.h"                 
#include "gfxMatrix.h"                  
#include "gfxPattern.h"                 
#include "gfxPlatform.h"                
#include "gfxPoint.h"                   
#include "gfxRect.h"                    
#include "gfxTeeSurface.h"              
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
                                  ContextSource aSource,
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

  if (aSource == BUFFER_BLACK) {
    if (mBuffer) {
      source = mBuffer;
    } else if (mDTBuffer) {
      source = gfxPlatform::GetPlatform()->GetThebesSurfaceForDrawTarget(mDTBuffer);
    } else {
      NS_RUNTIMEABORT("Can't draw a RotatedBuffer without any buffer!");
    }
  } else {
    MOZ_ASSERT(aSource == BUFFER_WHITE);
    if (mBufferOnWhite) {
      source = mBufferOnWhite;
    } else if (mDTBufferOnWhite) {
      source = gfxPlatform::GetPlatform()->GetThebesSurfaceForDrawTarget(mDTBufferOnWhite);
    } else {
      NS_RUNTIMEABORT("Can't draw a RotatedBuffer without any buffer!");
    }
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

  RefPtr<SourceSurface> snapshot;
  if (aSource == BUFFER_BLACK) {
    snapshot = mDTBuffer->Snapshot();
  } else {
    MOZ_ASSERT(aSource == BUFFER_WHITE);
    snapshot = mDTBufferOnWhite->Snapshot();
  }

  
  Matrix transform;
  transform.Translate(quadrantTranslation.x, quadrantTranslation.y);

#ifdef MOZ_GFX_OPTIMIZE_MOBILE
  SurfacePattern source(snapshot, EXTEND_CLAMP, transform, FILTER_POINT);
#else
  SurfacePattern source(snapshot, EXTEND_CLAMP, transform);
#endif

  if (aOperator == OP_SOURCE) {
    
    
    
    aTarget->PushClipRect(gfx::Rect(fillRect.x, fillRect.y,
                                    fillRect.width, fillRect.height));
  }

  if (aMask) {
    Matrix oldTransform = aTarget->GetTransform();
    aTarget->SetTransform(*aMaskTransform);
    aTarget->MaskSurface(source, aMask, Point(0, 0), DrawOptions(aOpacity, aOperator));
    aTarget->SetTransform(oldTransform);
  } else {
    aTarget->FillRect(gfx::Rect(fillRect.x, fillRect.y,
                                fillRect.width, fillRect.height),
                      source, DrawOptions(aOpacity, aOperator));
  }

  if (aOperator == OP_SOURCE) {
    aTarget->PopClip();
  }

  aTarget->Flush();
}

void
RotatedBuffer::DrawBufferWithRotation(gfxContext* aTarget, ContextSource aSource,
                                      float aOpacity,
                                      gfxASurface* aMask,
                                      const gfxMatrix* aMaskTransform) const
{
  PROFILER_LABEL("RotatedBuffer", "DrawBufferWithRotation");
  
  
  DrawBufferQuadrant(aTarget, LEFT, TOP, aSource, aOpacity, aMask, aMaskTransform);
  DrawBufferQuadrant(aTarget, RIGHT, TOP, aSource, aOpacity, aMask, aMaskTransform);
  DrawBufferQuadrant(aTarget, LEFT, BOTTOM, aSource, aOpacity, aMask, aMaskTransform);
  DrawBufferQuadrant(aTarget, RIGHT, BOTTOM, aSource, aOpacity, aMask, aMaskTransform);
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

    DrawBufferWithRotation(aTarget, BUFFER_BLACK, aOpacity, aMask, aMaskTransform);
    aTarget->Restore();
  } else {
    RefPtr<DrawTarget> dt = aTarget->GetDrawTarget();
    bool clipped = false;

    
    
    
    
    if (!aLayer->GetValidRegion().Contains(BufferRect()) ||
        (ToData(aLayer)->GetClipToVisibleRegion() &&
         !aLayer->GetVisibleRegion().Contains(BufferRect())) ||
        IsClippingCheap(aTarget, aLayer->GetEffectiveVisibleRegion())) {
      
      
      
      
      
      gfxUtils::ClipToRegionSnapped(dt, aLayer->GetEffectiveVisibleRegion());
      clipped = true;
    }

    RefPtr<SourceSurface> mask;
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
}

static void
FillSurface(gfxASurface* aSurface, const nsIntRegion& aRegion,
            const nsIntPoint& aOffset, const gfxRGBA& aColor)
{
  nsRefPtr<gfxContext> ctx = new gfxContext(aSurface);
  ctx->Translate(-gfxPoint(aOffset.x, aOffset.y));
  gfxUtils::ClipToRegion(ctx, aRegion);
  ctx->SetColor(aColor);
  ctx->Paint();
}

already_AddRefed<gfxContext>
ThebesLayerBuffer::GetContextForQuadrantUpdate(const nsIntRect& aBounds, ContextSource aSource, nsIntPoint *aTopLeft)
{
  EnsureBuffer();

  nsRefPtr<gfxContext> ctx;
  if (aSource == BUFFER_BOTH && HaveBufferOnWhite()) {
    EnsureBufferOnWhite();
    if (mBuffer) {
      MOZ_ASSERT(mBufferOnWhite);
      gfxASurface* surfaces[2] = { mBuffer, mBufferOnWhite };
      nsRefPtr<gfxTeeSurface> surf = new gfxTeeSurface(surfaces, ArrayLength(surfaces));

      
      
      gfxPoint deviceOffset = mBuffer->GetDeviceOffset();
      surfaces[0]->SetDeviceOffset(gfxPoint(0, 0));
      surfaces[1]->SetDeviceOffset(gfxPoint(0, 0));
      surf->SetDeviceOffset(deviceOffset);

      surf->SetAllowUseAsSource(false);
      ctx = new gfxContext(surf);
    } else {
      MOZ_ASSERT(mDTBuffer && mDTBufferOnWhite);
      RefPtr<DrawTarget> dualDT = Factory::CreateDualDrawTarget(mDTBuffer, mDTBufferOnWhite);
      ctx = new gfxContext(dualDT);
    }
  } else if (aSource == BUFFER_WHITE) {
    EnsureBufferOnWhite();
    if (mBufferOnWhite) {
      ctx = new gfxContext(mBufferOnWhite);
    } else {
      ctx = new gfxContext(mDTBufferOnWhite);
    }
  } else {
    
    if (mBuffer) {
      ctx = new gfxContext(mBuffer);
    } else {
      ctx = new gfxContext(mDTBuffer);
    }
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

gfxASurface::gfxContentType
ThebesLayerBuffer::BufferContentType()
{
  if (mBuffer) {
    return mBuffer->GetContentType();
  }
  if (mBufferProvider) {
    return mBufferProvider->GetContentType();
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

bool
ThebesLayerBuffer::IsAzureBuffer()
{
  MOZ_ASSERT(!(mDTBuffer && mBuffer), "Trying to use Azure and Thebes in the same buffer?");
  if (mDTBuffer) {
    return true;
  }
  if (mBuffer) {
    return false;
  }
  if (mBufferProvider) {
    return gfxPlatform::GetPlatform()->SupportsAzureContentForType(
      mBufferProvider->BackendType());
  }
  return SupportsAzureContent();
}

void
ThebesLayerBuffer::EnsureBuffer()
{
  if ((!mBuffer && !mDTBuffer) && mBufferProvider) {
    if (IsAzureBuffer()) {
      mDTBuffer = mBufferProvider->LockDrawTarget();
      mBuffer = nullptr;
    } else {
      mBuffer = mBufferProvider->LockSurface();
      mDTBuffer = nullptr;
    }
  }
}

void
ThebesLayerBuffer::EnsureBufferOnWhite()
{
  if ((!mBufferOnWhite && !mDTBufferOnWhite) && mBufferProviderOnWhite) {
    if (IsAzureBuffer()) {
      mDTBufferOnWhite = mBufferProviderOnWhite->LockDrawTarget();
      mBufferOnWhite = nullptr;
    } else {
      mBufferOnWhite = mBufferProviderOnWhite->LockSurface();
      mDTBufferOnWhite = nullptr;
    }
  }
}

bool
ThebesLayerBuffer::HaveBuffer() const
{
  return mDTBuffer || mBuffer || mBufferProvider;
}

bool
ThebesLayerBuffer::HaveBufferOnWhite() const
{
  return mDTBufferOnWhite || mBufferOnWhite || mBufferProviderOnWhite;
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

ThebesLayerBuffer::PaintState
ThebesLayerBuffer::BeginPaint(ThebesLayer* aLayer, ContentType aContentType,
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
        contentType = gfxASurface::CONTENT_COLOR;
      }
#endif
    }

    if ((aFlags & PAINT_WILL_RESAMPLE) &&
        (!neededRegion.GetBounds().IsEqualInterior(destBufferRect) ||
         neededRegion.GetNumRects() > 1)) {
      
      if (mode == Layer::SURFACE_OPAQUE) {
        contentType = gfxASurface::CONTENT_COLOR_ALPHA;
        mode = Layer::SURFACE_SINGLE_CHANNEL_ALPHA;
      }

      
      
      neededRegion = destBufferRect;
    }

    
    
    if (HaveBuffer() &&
        (contentType != BufferContentType() ||
         mode == Layer::SURFACE_COMPONENT_ALPHA) != (HaveBufferOnWhite())) {

      
      
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
  nsRefPtr<gfxASurface> destBufferOnWhite;
  RefPtr<DrawTarget> destDTBuffer;
  RefPtr<DrawTarget> destDTBufferOnWhite;
  uint32_t bufferFlags = canHaveRotation ? ALLOW_REPEAT : 0;
  if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
    bufferFlags |= BUFFER_COMPONENT_ALPHA;
  }
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
          if (IsAzureBuffer()) {
            MOZ_ASSERT(mDTBuffer);
            RefPtr<SourceSurface> source = mDTBuffer->Snapshot();
            mDTBuffer->CopySurface(source,
                                   IntRect(srcRect.x, srcRect.y, srcRect.width, srcRect.height),
                                   IntPoint(dest.x, dest.y));
            if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
              EnsureBufferOnWhite();
              MOZ_ASSERT(mDTBufferOnWhite);
              RefPtr<SourceSurface> sourceOnWhite = mDTBufferOnWhite->Snapshot();
              mDTBufferOnWhite->CopySurface(sourceOnWhite,
                                            IntRect(srcRect.x, srcRect.y, srcRect.width, srcRect.height),
                                            IntPoint(dest.x, dest.y));
            }
          } else {
            MOZ_ASSERT(mBuffer);
            mBuffer->MovePixels(srcRect, dest);
            if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
              EnsureBufferOnWhite();
              MOZ_ASSERT(mBufferOnWhite);
              mBufferOnWhite->MovePixels(srcRect, dest);
            }
          }
          result.mDidSelfCopy = true;
          
          
          mBufferRect = destBufferRect;
        } else {
          
          
          destBufferRect = ComputeBufferRect(neededRegion.GetBounds());
          CreateBuffer(contentType, destBufferRect, bufferFlags,
                       getter_AddRefs(destBuffer), getter_AddRefs(destBufferOnWhite),
                       &destDTBuffer, &destDTBufferOnWhite);
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
    
    CreateBuffer(contentType, destBufferRect, bufferFlags,
                 getter_AddRefs(destBuffer), getter_AddRefs(destBufferOnWhite),
                 &destDTBuffer, &destDTBufferOnWhite);
    if (!destBuffer && !destDTBuffer)
      return result;
  }

  NS_ASSERTION(!(aFlags & PAINT_WILL_RESAMPLE) || destBufferRect == neededRegion.GetBounds(),
               "If we're resampling, we need to validate the entire buffer");

  
  
  bool isClear = !HaveBuffer();

  if (destBuffer) {
    if (!isClear && (mode != Layer::SURFACE_COMPONENT_ALPHA || HaveBufferOnWhite())) {
      
      nsRefPtr<gfxContext> tmpCtx = new gfxContext(destBuffer);
      nsIntPoint offset = -destBufferRect.TopLeft();
      tmpCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
      tmpCtx->Translate(gfxPoint(offset.x, offset.y));
      EnsureBuffer();
      DrawBufferWithRotation(tmpCtx, BUFFER_BLACK);

      if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
        EnsureBufferOnWhite();
        NS_ASSERTION(destBufferOnWhite, "Must have a white buffer!");
        nsRefPtr<gfxContext> tmpCtx = new gfxContext(destBufferOnWhite);
        tmpCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
        tmpCtx->Translate(gfxPoint(offset.x, offset.y));
        DrawBufferWithRotation(tmpCtx, BUFFER_WHITE);
      }
    }

    mBuffer = destBuffer.forget();
    mDTBuffer = nullptr;
    mBufferRect = destBufferRect;
    mBufferOnWhite = destBufferOnWhite.forget();
    mDTBufferOnWhite = nullptr;
    mBufferRotation = nsIntPoint(0,0);
  } else if (destDTBuffer) {
    if (!isClear && (mode != Layer::SURFACE_COMPONENT_ALPHA || HaveBufferOnWhite())) {
      
      nsIntPoint offset = -destBufferRect.TopLeft();
      Matrix mat;
      mat.Translate(offset.x, offset.y);
      destDTBuffer->SetTransform(mat);
      EnsureBuffer();
      MOZ_ASSERT(mDTBuffer, "Have we got a Thebes buffer for some reason?");
      DrawBufferWithRotation(destDTBuffer, BUFFER_BLACK, 1.0, OP_SOURCE);
      destDTBuffer->SetTransform(Matrix());

      if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
        NS_ASSERTION(destDTBufferOnWhite, "Must have a white buffer!");
        destDTBufferOnWhite->SetTransform(mat);
        EnsureBufferOnWhite();
        MOZ_ASSERT(destDTBufferOnWhite, "Have we got a Thebes buffer for some reason?");
        DrawBufferWithRotation(destDTBufferOnWhite, BUFFER_WHITE, 1.0, OP_SOURCE);
        destDTBufferOnWhite->SetTransform(Matrix());
      }
    }

    mDTBuffer = destDTBuffer.forget();
    mBuffer = nullptr;
    mDTBufferOnWhite = destDTBufferOnWhite.forget();
    mBufferOnWhite = nullptr;
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

  if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
    if (IsAzureBuffer()) {
      MOZ_ASSERT(mDTBuffer && mDTBufferOnWhite);
      nsIntRegionRectIterator iter(result.mRegionToDraw);
      const nsIntRect *iterRect;
      while ((iterRect = iter.Next())) {
        mDTBuffer->FillRect(Rect(iterRect->x, iterRect->y, iterRect->width, iterRect->height),
                            ColorPattern(Color(0.0, 0.0, 0.0, 1.0)));
        mDTBufferOnWhite->FillRect(Rect(iterRect->x, iterRect->y, iterRect->width, iterRect->height),
                                   ColorPattern(Color(1.0, 1.0, 1.0, 1.0)));
      }
    } else {
      MOZ_ASSERT(mBuffer && mBufferOnWhite);
      FillSurface(mBuffer, result.mRegionToDraw, topLeft, gfxRGBA(0.0, 0.0, 0.0, 1.0));
      FillSurface(mBufferOnWhite, result.mRegionToDraw, topLeft, gfxRGBA(1.0, 1.0, 1.0, 1.0));
    }
    gfxUtils::ClipToRegionSnapped(result.mContext, result.mRegionToDraw);
  } else if (contentType == gfxASurface::CONTENT_COLOR_ALPHA && !isClear) {
    if (IsAzureBuffer()) {
      nsIntRegionRectIterator iter(result.mRegionToDraw);
      const nsIntRect *iterRect;
      while ((iterRect = iter.Next())) {
        result.mContext->GetDrawTarget()->ClearRect(Rect(iterRect->x, iterRect->y, iterRect->width, iterRect->height));
      }
      
      
      gfxUtils::ClipToRegionSnapped(result.mContext, result.mRegionToDraw);
    } else {
      MOZ_ASSERT(result.mContext->IsCairo());
      gfxUtils::ClipToRegionSnapped(result.mContext, result.mRegionToDraw);
      result.mContext->SetOperator(gfxContext::OPERATOR_CLEAR);
      result.mContext->Paint();
      result.mContext->SetOperator(gfxContext::OPERATOR_OVER);
    }
  } else {
    gfxUtils::ClipToRegionSnapped(result.mContext, result.mRegionToDraw);
  }

  return result;
}

}
}

