




#include "RotatedBuffer.h"
#include <sys/types.h>                  
#include <algorithm>                    
#include "BasicImplData.h"              
#include "BasicLayersImpl.h"            
#include "BufferUnrotate.h"             
#include "GeckoProfiler.h"              
#include "Layers.h"                     
#include "gfxPlatform.h"                
#include "gfxPrefs.h"                   
#include "gfxUtils.h"                   
#include "mozilla/ArrayUtils.h"         
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
#include "nsLayoutUtils.h"              

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

  MOZ_ASSERT(aSource != BUFFER_BOTH);
  RefPtr<SourceSurface> snapshot = GetSourceSurface(aSource);

  
  
  
  
  
  if ((aTarget->GetBackendType() == BackendType::DIRECT2D ||
       aTarget->GetBackendType() == BackendType::DIRECT2D1_1) &&
      aOperator == CompositionOp::OP_SOURCE) {
    aOperator = CompositionOp::OP_OVER;
    if (snapshot->GetFormat() == SurfaceFormat::B8G8R8A8) {
      aTarget->ClearRect(ToRect(fillRect));
    }
  }

  if (aOperator == CompositionOp::OP_SOURCE) {
    
    
    
    aTarget->PushClipRect(gfx::Rect(fillRect.x, fillRect.y,
                                    fillRect.width, fillRect.height));
  }

  if (aMask) {
    Matrix oldTransform = aTarget->GetTransform();

    
    Matrix transform =
      Matrix::Translation(quadrantTranslation.x, quadrantTranslation.y);

    Matrix inverseMask = *aMaskTransform;
    inverseMask.Invert();

    transform *= oldTransform;
    transform *= inverseMask;

#ifdef MOZ_GFX_OPTIMIZE_MOBILE
    SurfacePattern source(snapshot, ExtendMode::CLAMP, transform, Filter::POINT);
#else
    SurfacePattern source(snapshot, ExtendMode::CLAMP, transform);
#endif

    aTarget->SetTransform(*aMaskTransform);
    aTarget->MaskSurface(source, aMask, Point(0, 0), DrawOptions(aOpacity, aOperator));
    aTarget->SetTransform(oldTransform);
  } else {
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
    DrawSurfaceOptions options(Filter::POINT);
#else
    DrawSurfaceOptions options;
#endif
    aTarget->DrawSurface(snapshot, ToRect(fillRect),
                         GetSourceRectangle(aXSide, aYSide),
                         options,
                         DrawOptions(aOpacity, aOperator));
  }

  if (aOperator == CompositionOp::OP_SOURCE) {
    aTarget->PopClip();
  }
}

void
RotatedBuffer::DrawBufferWithRotation(gfx::DrawTarget *aTarget, ContextSource aSource,
                                      float aOpacity,
                                      gfx::CompositionOp aOperator,
                                      gfx::SourceSurface* aMask,
                                      const gfx::Matrix* aMaskTransform) const
{
  PROFILER_LABEL("RotatedBuffer", "DrawBufferWithRotation",
    js::ProfileEntry::Category::GRAPHICS);

  
  
  
  DrawBufferQuadrant(aTarget, LEFT, TOP, aSource, aOpacity, aOperator, aMask, aMaskTransform);
  DrawBufferQuadrant(aTarget, RIGHT, TOP, aSource, aOpacity, aOperator, aMask, aMaskTransform);
  DrawBufferQuadrant(aTarget, LEFT, BOTTOM, aSource, aOpacity, aOperator, aMask, aMaskTransform);
  DrawBufferQuadrant(aTarget, RIGHT, BOTTOM, aSource, aOpacity, aOperator,aMask, aMaskTransform);
}

TemporaryRef<SourceSurface>
SourceRotatedBuffer::GetSourceSurface(ContextSource aSource) const
{
  RefPtr<SourceSurface> surf;
  if (aSource == BUFFER_BLACK) {
    surf = mSource;
  } else {
    MOZ_ASSERT(aSource == BUFFER_WHITE);
    surf = mSourceOnWhite;
  }

  MOZ_ASSERT(surf);
  return surf;
}

 bool
RotatedContentBuffer::IsClippingCheap(DrawTarget* aTarget, const nsIntRegion& aRegion)
{
  
  
  return !aTarget->GetTransform().HasNonIntegerTranslation() &&
         aRegion.GetNumRects() <= 1;
}

void
RotatedContentBuffer::DrawTo(PaintedLayer* aLayer,
                             DrawTarget* aTarget,
                             float aOpacity,
                             CompositionOp aOp,
                             SourceSurface* aMask,
                             const Matrix* aMaskTransform)
{
  if (!EnsureBuffer()) {
    return;
  }

  bool clipped = false;

  
  
  
  
  if (!aLayer->GetValidRegion().Contains(BufferRect()) ||
      (ToData(aLayer)->GetClipToVisibleRegion() &&
       !aLayer->GetVisibleRegion().Contains(BufferRect())) ||
      IsClippingCheap(aTarget, aLayer->GetEffectiveVisibleRegion())) {
    
    
    
    
    
    gfxUtils::ClipToRegion(aTarget, aLayer->GetEffectiveVisibleRegion());
    clipped = true;
  }

  DrawBufferWithRotation(aTarget, BUFFER_BLACK, aOpacity, aOp, aMask, aMaskTransform);
  if (clipped) {
    aTarget->PopClip();
  }
}

DrawTarget*
RotatedContentBuffer::BorrowDrawTargetForQuadrantUpdate(const nsIntRect& aBounds,
                                                        ContextSource aSource,
                                                        DrawIterator* aIter)
{
  nsIntRect bounds = aBounds;
  if (aIter) {
    
    
    
    
    
    aIter->mDrawRegion.SetEmpty();
    while (aIter->mCount < 4) {
      nsIntRect quadrant = GetQuadrantRectangle((aIter->mCount & 1) ? LEFT : RIGHT,
        (aIter->mCount & 2) ? TOP : BOTTOM);
      aIter->mDrawRegion.And(aBounds, quadrant);
      aIter->mCount++;
      if (!aIter->mDrawRegion.IsEmpty()) {
        break;
      }
    }
    if (aIter->mDrawRegion.IsEmpty()) {
      return nullptr;
    }
    bounds = aIter->mDrawRegion.GetBounds();
  }

  if (!EnsureBuffer()) {
    return nullptr;
  }

  MOZ_ASSERT(!mLoanedDrawTarget, "draw target has been borrowed and not returned");
  if (aSource == BUFFER_BOTH && HaveBufferOnWhite()) {
    if (!EnsureBufferOnWhite()) {
      return nullptr;
    }
    MOZ_ASSERT(mDTBuffer && mDTBufferOnWhite);
    mLoanedDrawTarget = Factory::CreateDualDrawTarget(mDTBuffer, mDTBufferOnWhite);
  } else if (aSource == BUFFER_WHITE) {
    if (!EnsureBufferOnWhite()) {
      return nullptr;
    }
    mLoanedDrawTarget = mDTBufferOnWhite;
  } else {
    
    mLoanedDrawTarget = mDTBuffer;
  }

  
  int32_t xBoundary = mBufferRect.XMost() - mBufferRotation.x;
  int32_t yBoundary = mBufferRect.YMost() - mBufferRotation.y;
  XSide sideX = bounds.XMost() <= xBoundary ? RIGHT : LEFT;
  YSide sideY = bounds.YMost() <= yBoundary ? BOTTOM : TOP;
  nsIntRect quadrantRect = GetQuadrantRectangle(sideX, sideY);
  NS_ASSERTION(quadrantRect.Contains(bounds), "Messed up quadrants");

  mLoanedTransform = mLoanedDrawTarget->GetTransform();
  mLoanedDrawTarget->SetTransform(Matrix(mLoanedTransform).
                                    PreTranslate(-quadrantRect.x,
                                                 -quadrantRect.y));

  return mLoanedDrawTarget;
}

void
BorrowDrawTarget::ReturnDrawTarget(gfx::DrawTarget*& aReturned)
{
  MOZ_ASSERT(aReturned == mLoanedDrawTarget);
  mLoanedDrawTarget->SetTransform(mLoanedTransform);
  mLoanedDrawTarget = nullptr;
  aReturned = nullptr;
}

gfxContentType
RotatedContentBuffer::BufferContentType()
{
  if (mBufferProvider || mDTBuffer) {
    SurfaceFormat format;

    if (mBufferProvider) {
      format = mBufferProvider->GetFormat();
    } else if (mDTBuffer) {
      format = mDTBuffer->GetFormat();
    }

    return ContentForFormat(format);
  }
  return gfxContentType::SENTINEL;
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
  NS_ASSERTION(!mLoanedDrawTarget, "Loaned draw target must be returned");
  if (!mDTBuffer) {
    if (mBufferProvider) {
      mDTBuffer = mBufferProvider->BorrowDrawTarget();
    }
  }

  NS_WARN_IF_FALSE(mDTBuffer, "no buffer");
  return !!mDTBuffer;
}

bool
RotatedContentBuffer::EnsureBufferOnWhite()
{
  NS_ASSERTION(!mLoanedDrawTarget, "Loaned draw target must be returned");
  if (!mDTBufferOnWhite) {
    if (mBufferProviderOnWhite) {
      mDTBufferOnWhite =
        mBufferProviderOnWhite->BorrowDrawTarget();
    }
  }

  NS_WARN_IF_FALSE(mDTBufferOnWhite, "no buffer");
  return !!mDTBufferOnWhite;
}

bool
RotatedContentBuffer::HaveBuffer() const
{
  return mDTBuffer || mBufferProvider;
}

bool
RotatedContentBuffer::HaveBufferOnWhite() const
{
  return mDTBufferOnWhite || mBufferProviderOnWhite;
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

void
RotatedContentBuffer::FlushBuffers()
{
  if (mDTBuffer) {
    mDTBuffer->Flush();
  }
  if (mDTBufferOnWhite) {
    mDTBufferOnWhite->Flush();
  }
}

RotatedContentBuffer::PaintState
RotatedContentBuffer::BeginPaint(PaintedLayer* aLayer,
                                 uint32_t aFlags)
{
  PaintState result;
  
  
  bool canHaveRotation = gfxPlatform::BufferRotationEnabled() &&
                         !(aFlags & (PAINT_WILL_RESAMPLE | PAINT_NO_ROTATION));

  nsIntRegion validRegion = aLayer->GetValidRegion();

  bool canUseOpaqueSurface = aLayer->CanUseOpaqueSurface();
  ContentType layerContentType =
    canUseOpaqueSurface ? gfxContentType::COLOR :
                          gfxContentType::COLOR_ALPHA;

  SurfaceMode mode;
  nsIntRegion neededRegion;
  nsIntRect destBufferRect;

  bool canReuseBuffer = HaveBuffer();

  while (true) {
    mode = aLayer->GetSurfaceMode();
    neededRegion = aLayer->GetVisibleRegion();
    canReuseBuffer &= BufferSizeOkFor(neededRegion.GetBounds().Size());
    result.mContentType = layerContentType;

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

    if (mode == SurfaceMode::SURFACE_COMPONENT_ALPHA) {
#if defined(MOZ_GFX_OPTIMIZE_MOBILE) || defined(MOZ_WIDGET_GONK)
      mode = SurfaceMode::SURFACE_SINGLE_CHANNEL_ALPHA;
#else
      if (!aLayer->GetParent() ||
          !aLayer->GetParent()->SupportsComponentAlphaChildren() ||
          !aLayer->AsShadowableLayer() ||
          !aLayer->AsShadowableLayer()->HasShadow()) {
        mode = SurfaceMode::SURFACE_SINGLE_CHANNEL_ALPHA;
      } else {
        result.mContentType = gfxContentType::COLOR;
      }
#endif
    }

    if ((aFlags & PAINT_WILL_RESAMPLE) &&
        (!neededRegion.GetBounds().IsEqualInterior(destBufferRect) ||
         neededRegion.GetNumRects() > 1))
    {
      
      if (mode == SurfaceMode::SURFACE_OPAQUE) {
        result.mContentType = gfxContentType::COLOR_ALPHA;
        mode = SurfaceMode::SURFACE_SINGLE_CHANNEL_ALPHA;
      }

      
      
      neededRegion = destBufferRect;
    }

    
    
    if (canReuseBuffer &&
        (result.mContentType != BufferContentType() ||
        (mode == SurfaceMode::SURFACE_COMPONENT_ALPHA) != HaveBufferOnWhite()))
    {
      
      
      canReuseBuffer = false;
      continue;
    }

    break;
  }

  if (HaveBuffer() &&
      (result.mContentType != BufferContentType() ||
      (mode == SurfaceMode::SURFACE_COMPONENT_ALPHA) != HaveBufferOnWhite()))
  {
    
    
    canReuseBuffer = false;
    result.mRegionToInvalidate = aLayer->GetValidRegion();
    validRegion.SetEmpty();
    Clear();

#if defined(MOZ_DUMP_PAINTING)
    if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
      if (result.mContentType != BufferContentType()) {
        printf_stderr("Invalidating entire rotated buffer (layer %p): content type changed\n", aLayer);
      } else if ((mode == SurfaceMode::SURFACE_COMPONENT_ALPHA) != HaveBufferOnWhite()) {
        printf_stderr("Invalidating entire rotated buffer (layer %p): component alpha changed\n", aLayer);
      }
    }
#endif
  }

  NS_ASSERTION(destBufferRect.Contains(neededRegion.GetBounds()),
               "Destination rect doesn't contain what we need to paint");

  result.mRegionToDraw.Sub(neededRegion, validRegion);

  if (result.mRegionToDraw.IsEmpty())
    return result;

  if (HaveBuffer()) {
    
    
    
    FinalizeFrame(result.mRegionToDraw);
  }

  nsIntRect drawBounds = result.mRegionToDraw.GetBounds();
  RefPtr<DrawTarget> destDTBuffer;
  RefPtr<DrawTarget> destDTBufferOnWhite;
  uint32_t bufferFlags = 0;
  if (mode == SurfaceMode::SURFACE_COMPONENT_ALPHA) {
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
      bool drawWrapsBuffer = (drawBounds.x < xBoundary && xBoundary < drawBounds.XMost()) ||
                             (drawBounds.y < yBoundary && yBoundary < drawBounds.YMost());
      if ((drawWrapsBuffer && !(aFlags & PAINT_CAN_DRAW_ROTATED)) ||
          (newRotation != nsIntPoint(0,0) && !canHaveRotation)) {
        
        
        
        
        if (mBufferRotation == nsIntPoint(0,0)) {
          nsIntRect srcRect(nsIntPoint(0, 0), mBufferRect.Size());
          nsIntPoint dest = mBufferRect.TopLeft() - destBufferRect.TopLeft();
          MOZ_ASSERT(mDTBuffer);
          mDTBuffer->CopyRect(IntRect(srcRect.x, srcRect.y, srcRect.width, srcRect.height),
                              IntPoint(dest.x, dest.y));
          if (mode == SurfaceMode::SURFACE_COMPONENT_ALPHA) {
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

            if (mode == SurfaceMode::SURFACE_COMPONENT_ALPHA) {
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
            CreateBuffer(result.mContentType, destBufferRect, bufferFlags,
                         &destDTBuffer, &destDTBufferOnWhite);
            if (!destDTBuffer ||
                (!destDTBufferOnWhite && (bufferFlags & BUFFER_COMPONENT_ALPHA))) {
              gfxCriticalError(CriticalLog::DefaultOptions(Factory::ReasonableSurfaceSize(IntSize(destBufferRect.width, destBufferRect.height)))) << "Failed 1 buffer db=" << hexa(destDTBuffer.get()) << " dw=" << hexa(destDTBufferOnWhite.get()) << " for " << destBufferRect.x << ", " << destBufferRect.y << ", " << destBufferRect.width << ", " << destBufferRect.height;
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
    
    CreateBuffer(result.mContentType, destBufferRect, bufferFlags,
                 &destDTBuffer, &destDTBufferOnWhite);
    if (!destDTBuffer ||
        (!destDTBufferOnWhite && (bufferFlags & BUFFER_COMPONENT_ALPHA))) {
      gfxCriticalError(CriticalLog::DefaultOptions(Factory::ReasonableSurfaceSize(IntSize(destBufferRect.width, destBufferRect.height)))) << "Failed 2 buffer db=" << hexa(destDTBuffer.get()) << " dw=" << hexa(destDTBufferOnWhite.get()) << " for " << destBufferRect.x << ", " << destBufferRect.y << ", " << destBufferRect.width << ", " << destBufferRect.height;
      return result;
    }
  }

  NS_ASSERTION(!(aFlags & PAINT_WILL_RESAMPLE) || destBufferRect == neededRegion.GetBounds(),
               "If we're resampling, we need to validate the entire buffer");

  
  
  bool isClear = !HaveBuffer();

  if (destDTBuffer) {
    if (!isClear && (mode != SurfaceMode::SURFACE_COMPONENT_ALPHA || HaveBufferOnWhite())) {
      
      nsIntPoint offset = -destBufferRect.TopLeft();
      Matrix mat = Matrix::Translation(offset.x, offset.y);
      destDTBuffer->SetTransform(mat);
      if (!EnsureBuffer()) {
        return result;
      }
       MOZ_ASSERT(mDTBuffer, "Have we got a Thebes buffer for some reason?");
      DrawBufferWithRotation(destDTBuffer, BUFFER_BLACK, 1.0, CompositionOp::OP_SOURCE);
      destDTBuffer->SetTransform(Matrix());

      if (mode == SurfaceMode::SURFACE_COMPONENT_ALPHA) {
        if (!destDTBufferOnWhite || !EnsureBufferOnWhite()) {
          return result;
        }
        MOZ_ASSERT(mDTBufferOnWhite, "Have we got a Thebes buffer for some reason?");
        destDTBufferOnWhite->SetTransform(mat);
        DrawBufferWithRotation(destDTBufferOnWhite, BUFFER_WHITE, 1.0, CompositionOp::OP_SOURCE);
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
  result.mClip = DrawRegionClip::DRAW;
  result.mMode = mode;

  return result;
}

DrawTarget*
RotatedContentBuffer::BorrowDrawTargetForPainting(PaintState& aPaintState,
                                                  DrawIterator* aIter )
{
  if (aPaintState.mMode == SurfaceMode::SURFACE_NONE) {
    return nullptr;
  }

  DrawTarget* result = BorrowDrawTargetForQuadrantUpdate(aPaintState.mRegionToDraw.GetBounds(),
                                                         BUFFER_BOTH, aIter);
  if (!result) {
    return nullptr;
  }
  nsIntRegion* drawPtr = &aPaintState.mRegionToDraw;
  if (aIter) {
    
    
    aIter->mDrawRegion.And(aIter->mDrawRegion, aPaintState.mRegionToDraw);
    drawPtr = &aIter->mDrawRegion;
  }
  if (result->GetBackendType() == BackendType::DIRECT2D ||
      result->GetBackendType() == BackendType::DIRECT2D1_1) {
    drawPtr->SimplifyOutwardByArea(100 * 100);
  }

  if (aPaintState.mMode == SurfaceMode::SURFACE_COMPONENT_ALPHA) {
    if (!mDTBuffer || !mDTBufferOnWhite) {
      
      
      
      return nullptr;
    }
    nsIntRegionRectIterator iter(*drawPtr);
    const nsIntRect *iterRect;
    while ((iterRect = iter.Next())) {
      mDTBuffer->FillRect(Rect(iterRect->x, iterRect->y, iterRect->width, iterRect->height),
                          ColorPattern(Color(0.0, 0.0, 0.0, 1.0)));
      mDTBufferOnWhite->FillRect(Rect(iterRect->x, iterRect->y, iterRect->width, iterRect->height),
                                 ColorPattern(Color(1.0, 1.0, 1.0, 1.0)));
    }
  } else if (aPaintState.mContentType == gfxContentType::COLOR_ALPHA && HaveBuffer()) {
    
    nsIntRegionRectIterator iter(*drawPtr);
    const nsIntRect *iterRect;
    while ((iterRect = iter.Next())) {
      result->ClearRect(Rect(iterRect->x, iterRect->y, iterRect->width, iterRect->height));
    }
  }

  return result;
}

TemporaryRef<SourceSurface>
RotatedContentBuffer::GetSourceSurface(ContextSource aSource) const
{
  MOZ_ASSERT(mDTBuffer);
  if (aSource == BUFFER_BLACK) {
    return mDTBuffer->Snapshot();
  } else {
    MOZ_ASSERT(mDTBufferOnWhite);
    MOZ_ASSERT(aSource == BUFFER_WHITE);
    return mDTBufferOnWhite->Snapshot();
  }
}

}
}

