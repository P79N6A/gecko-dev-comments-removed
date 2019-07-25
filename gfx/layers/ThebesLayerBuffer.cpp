




































#include "ThebesLayerBuffer.h"
#include "Layers.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxUtils.h"
#include "nsIDeviceContext.h"

namespace mozilla {
namespace layers {

static nsIntSize
ScaledSize(const nsIntSize& aSize, float aXScale, float aYScale)
{
  if (aXScale == 1.0 && aYScale == 1.0) {
    return aSize;
  }

  gfxRect rect(0, 0, aSize.width, aSize.height);
  rect.Scale(aXScale, aYScale);
  rect.RoundOut();
  return nsIntSize(rect.Width(), rect.Height());
}

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
                                      float aXRes, float aYRes)
{
  
  
  
  
  nsIntRect quadrantRect = GetQuadrantRectangle(aXSide, aYSide);
  nsIntRect fillRect;
  if (!fillRect.IntersectRect(mBufferRect, quadrantRect))
    return;

  aTarget->NewPath();
  aTarget->Rectangle(gfxRect(fillRect.x, fillRect.y,
                             fillRect.width, fillRect.height),
                     PR_TRUE);

  gfxPoint quadrantTranslation(quadrantRect.x, quadrantRect.y);
  nsRefPtr<gfxPattern> pattern = new gfxPattern(mBuffer);

#ifdef MOZ_GFX_OPTIMIZE_MOBILE
  gfxPattern::GraphicsFilter filter = gfxPattern::FILTER_NEAREST;
  pattern->SetFilter(filter);
#endif

  gfxContextMatrixAutoSaveRestore saveMatrix(aTarget);

  
  gfxMatrix transform;
  transform.Scale(aXRes, aYRes);
  transform.Translate(-quadrantTranslation);

  
  
  transform.Scale(1.0 / aXRes, 1.0 / aYRes);
  transform.NudgeToIntegers();

  gfxMatrix ctxMatrix = aTarget->CurrentMatrix();
  ctxMatrix.Scale(1.0 / aXRes, 1.0 / aYRes);
  ctxMatrix.NudgeToIntegers();
  aTarget->SetMatrix(ctxMatrix);

  pattern->SetMatrix(transform);
  aTarget->SetPattern(pattern);

  if (aOpacity != 1.0) {
    aTarget->Save();
    aTarget->Clip();
    aTarget->Paint(aOpacity);
    aTarget->Restore();
  } else {
    aTarget->Fill();
  }
}

void
ThebesLayerBuffer::DrawBufferWithRotation(gfxContext* aTarget, float aOpacity,
                                          float aXRes, float aYRes)
{
  
  
  DrawBufferQuadrant(aTarget, LEFT, TOP, aOpacity, aXRes, aYRes);
  DrawBufferQuadrant(aTarget, RIGHT, TOP, aOpacity, aXRes, aYRes);
  DrawBufferQuadrant(aTarget, LEFT, BOTTOM, aOpacity, aXRes, aYRes);
  DrawBufferQuadrant(aTarget, RIGHT, BOTTOM, aOpacity, aXRes, aYRes);
}

already_AddRefed<gfxContext>
ThebesLayerBuffer::GetContextForQuadrantUpdate(const nsIntRect& aBounds,
                                               float aXResolution,
                                               float aYResolution)
{
  nsRefPtr<gfxContext> ctx = new gfxContext(mBuffer);

  
  PRInt32 xBoundary = mBufferRect.XMost() - mBufferRotation.x;
  PRInt32 yBoundary = mBufferRect.YMost() - mBufferRotation.y;
  XSide sideX = aBounds.XMost() <= xBoundary ? RIGHT : LEFT;
  YSide sideY = aBounds.YMost() <= yBoundary ? BOTTOM : TOP;
  nsIntRect quadrantRect = GetQuadrantRectangle(sideX, sideY);
  NS_ASSERTION(quadrantRect.Contains(aBounds), "Messed up quadrants");
  ctx->Scale(aXResolution, aYResolution);
  ctx->Translate(-gfxPoint(quadrantRect.x, quadrantRect.y));

  return ctx.forget();
}




static void
MovePixels(gfxASurface* aBuffer,
           const nsIntRect& aSourceRect, const nsIntPoint& aDest,
           float aXResolution, float aYResolution)
{
  gfxRect src(aSourceRect.x, aSourceRect.y, aSourceRect.width, aSourceRect.height);
  gfxRect dest(aDest.x, aDest.y,  aSourceRect.width, aSourceRect.height);
  src.Scale(aXResolution, aYResolution);
  dest.Scale(aXResolution, aYResolution);

#ifdef DEBUG
  
  
  
  
  
  static const gfxFloat kPrecision =
    1.0 / gfxFloat(nsIDeviceContext::AppUnitsPerCSSPixel());
  
  
  NS_WARN_IF_FALSE(
    src.WithinEpsilonOfIntegerPixels(2.0 * kPrecision * aXResolution) &&
    dest.WithinEpsilonOfIntegerPixels(2.0 * kPrecision * aXResolution),
    "Rects don't round to device pixels within precision; glitches likely to follow");
#endif

  src.Round();
  dest.Round();

  aBuffer->MovePixels(nsIntRect(src.X(), src.Y(),
                                src.Width(), src.Height()),
                      nsIntPoint(dest.X(), dest.Y()));
}

static void
WrapRotationAxis(PRInt32* aRotationPoint, PRInt32 aSize)
{
  if (*aRotationPoint < 0) {
    *aRotationPoint += aSize;
  } else if (*aRotationPoint >= aSize) {
    *aRotationPoint -= aSize;
  }
}

ThebesLayerBuffer::PaintState
ThebesLayerBuffer::BeginPaint(ThebesLayer* aLayer, ContentType aContentType,
                              float aXResolution, float aYResolution,
                              PRUint32 aFlags)
{
  PaintState result;
  result.mDidSelfCopy = PR_FALSE;
  float curXRes = aLayer->GetXResolution();
  float curYRes = aLayer->GetYResolution();
  
  
  
  
  
  
  PRBool canHaveRotation =
    !(aFlags & PAINT_WILL_RESAMPLE) && aXResolution == 1.0 && aYResolution == 1.0;

  nsIntRegion validRegion = aLayer->GetValidRegion();

  ContentType contentType;
  nsIntRegion neededRegion;
  nsIntSize destBufferDims;
  PRBool canReuseBuffer;
  nsIntRect destBufferRect;

  while (PR_TRUE) {
    contentType = aContentType;
    neededRegion = aLayer->GetVisibleRegion();
    destBufferDims = ScaledSize(neededRegion.GetBounds().Size(),
                                aXResolution, aYResolution);
    canReuseBuffer = BufferSizeOkFor(destBufferDims);

    if (canReuseBuffer) {
      if (mBufferRect.Contains(neededRegion.GetBounds())) {
        
        destBufferRect = mBufferRect;
      } else if (neededRegion.GetBounds().Size() <= mBufferRect.Size()) {
        
        
        destBufferRect = nsIntRect(neededRegion.GetBounds().TopLeft(), mBufferRect.Size());
      } else {
        destBufferRect = neededRegion.GetBounds();
      }
    } else {
      destBufferRect = neededRegion.GetBounds();
    }

    if ((aFlags & PAINT_WILL_RESAMPLE) &&
        (neededRegion.GetBounds() != destBufferRect ||
         neededRegion.GetNumRects() > 1)) {
      
      contentType = gfxASurface::CONTENT_COLOR_ALPHA;

      
      
      neededRegion = destBufferRect;
      destBufferDims = ScaledSize(neededRegion.GetBounds().Size(),
                                  aXResolution, aYResolution);
    }

    if (mBuffer &&
        (contentType != mBuffer->GetContentType() ||
         aXResolution != curXRes || aYResolution != curYRes)) {
      
      
      
      
      
      
      
      
      
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
  PRBool bufferDimsChanged = PR_FALSE;
  PRUint32 bufferFlags = canHaveRotation ? ALLOW_REPEAT : 0;
  if (canReuseBuffer) {
    NS_ASSERTION(curXRes == aXResolution && curYRes == aYResolution,
                 "resolution changes must Clear()!");

    nsIntRect keepArea;
    if (keepArea.IntersectRect(destBufferRect, mBufferRect)) {
      
      
      
      nsIntPoint newRotation = mBufferRotation +
        (destBufferRect.TopLeft() - mBufferRect.TopLeft());
      WrapRotationAxis(&newRotation.x, mBufferRect.width);
      WrapRotationAxis(&newRotation.y, mBufferRect.height);
      NS_ASSERTION(nsIntRect(nsIntPoint(0,0), mBufferRect.Size()).Contains(newRotation),
                   "newRotation out of bounds");
      PRInt32 xBoundary = destBufferRect.XMost() - newRotation.x;
      PRInt32 yBoundary = destBufferRect.YMost() - newRotation.y;
      if ((drawBounds.x < xBoundary && xBoundary < drawBounds.XMost()) ||
          (drawBounds.y < yBoundary && yBoundary < drawBounds.YMost()) ||
          (newRotation != nsIntPoint(0,0) && !canHaveRotation)) {
        
        
        
        if (mBufferRotation == nsIntPoint(0,0)) {
          nsIntRect srcRect(nsIntPoint(0, 0), mBufferRect.Size());
          nsIntPoint dest = mBufferRect.TopLeft() - destBufferRect.TopLeft();
          MovePixels(mBuffer, srcRect, dest, curXRes, curYRes);
          result.mDidSelfCopy = PR_TRUE;
          
          
          mBufferRect = destBufferRect;
        } else {
          
          
          destBufferRect = neededRegion.GetBounds();
          bufferDimsChanged = PR_TRUE;
          destBuffer = CreateBuffer(contentType, destBufferDims, bufferFlags);
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
    
    bufferDimsChanged = PR_TRUE;
    destBuffer = CreateBuffer(contentType, destBufferDims, bufferFlags);
    if (!destBuffer)
      return result;
  }
  NS_ASSERTION(!(aFlags & PAINT_WILL_RESAMPLE) || destBufferRect == neededRegion.GetBounds(),
               "If we're resampling, we need to validate the entire buffer");

  
  
  PRBool isClear = mBuffer == nsnull;

  if (destBuffer) {
    if (mBuffer) {
      
      nsRefPtr<gfxContext> tmpCtx = new gfxContext(destBuffer);
      nsIntPoint offset = -destBufferRect.TopLeft();
      tmpCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
      tmpCtx->Scale(aXResolution, aYResolution);
      tmpCtx->Translate(gfxPoint(offset.x, offset.y));
      NS_ASSERTION(curXRes == aXResolution && curYRes == aYResolution,
                   "resolution changes must Clear()!");
      DrawBufferWithRotation(tmpCtx, 1.0, aXResolution, aYResolution);
    }

    mBuffer = destBuffer.forget();
    mBufferRect = destBufferRect;
    mBufferRotation = nsIntPoint(0,0);
  }
  if (bufferDimsChanged) {
    mBufferDims = destBufferDims;
  }
  NS_ASSERTION(canHaveRotation || mBufferRotation == nsIntPoint(0,0),
               "Rotation disabled, but we have nonzero rotation?");

  nsIntRegion invalidate;
  invalidate.Sub(aLayer->GetValidRegion(), destBufferRect);
  result.mRegionToInvalidate.Or(result.mRegionToInvalidate, invalidate);

  result.mContext = GetContextForQuadrantUpdate(drawBounds,
                                                aXResolution, aYResolution);

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

