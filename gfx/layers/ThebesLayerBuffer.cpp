




































#include "ThebesLayerBuffer.h"
#include "Layers.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxUtils.h"

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
  return nsIntSize(rect.size.width, rect.size.height);
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

  
  gfxMatrix transform;
  transform.Scale(aXRes, aYRes);
  transform.Translate(-quadrantTranslation);

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
                              float aXResolution, float aYResolution)
{
  PaintState result;

  result.mRegionToDraw.Sub(aLayer->GetVisibleRegion(), aLayer->GetValidRegion());

  float curXRes = aLayer->GetXResolution();
  float curYRes = aLayer->GetYResolution();
  if (mBuffer &&
      (aContentType != mBuffer->GetContentType() ||
       aXResolution != curXRes || aYResolution != curYRes)) {
    
    
    
    
    
    
    
    
    
    result.mRegionToDraw = aLayer->GetVisibleRegion();
    result.mRegionToInvalidate = aLayer->GetValidRegion();
    Clear();
  }

  if (result.mRegionToDraw.IsEmpty())
    return result;
  nsIntRect drawBounds = result.mRegionToDraw.GetBounds();

  nsIntRect visibleBounds = aLayer->GetVisibleRegion().GetBounds();
  nsIntSize destBufferDims = ScaledSize(visibleBounds.Size(),
                                        aXResolution, aYResolution);
  nsRefPtr<gfxASurface> destBuffer;
  nsIntRect destBufferRect;
  PRBool bufferDimsChanged = PR_FALSE;

  if (BufferSizeOkFor(destBufferDims)) {
    NS_ASSERTION(curXRes == aXResolution && curYRes == aYResolution,
                 "resolution changes must Clear()!");

    
    if (mBufferRect.Contains(visibleBounds)) {
      
      destBufferRect = mBufferRect;
    } else {
      
      
      destBufferRect = nsIntRect(visibleBounds.TopLeft(), mBufferRect.Size());
    }
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
          (drawBounds.y < yBoundary && yBoundary < drawBounds.YMost())) {
        
        
        if (mBufferRotation == nsIntPoint(0,0)) {
          destBuffer = mBuffer;
        } else {
          
          
          destBufferRect = visibleBounds;
          destBufferDims = ScaledSize(destBufferRect.Size(),
                                      aXResolution, aYResolution);
          bufferDimsChanged = PR_TRUE;
          destBuffer = CreateBuffer(aContentType, destBufferDims);
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
    
    destBufferRect = visibleBounds;
    destBufferDims = ScaledSize(destBufferRect.Size(),
                                aXResolution, aYResolution);
    bufferDimsChanged = PR_TRUE;
    destBuffer = CreateBuffer(aContentType, destBufferDims);
    if (!destBuffer)
      return result;
  }

  
  
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

  nsIntRegion invalidate;
  invalidate.Sub(aLayer->GetValidRegion(), destBufferRect);
  result.mRegionToInvalidate.Or(result.mRegionToInvalidate, invalidate);

  result.mContext = new gfxContext(mBuffer);

  
  PRInt32 xBoundary = mBufferRect.XMost() - mBufferRotation.x;
  PRInt32 yBoundary = mBufferRect.YMost() - mBufferRotation.y;
  XSide sideX = drawBounds.XMost() <= xBoundary ? RIGHT : LEFT;
  YSide sideY = drawBounds.YMost() <= yBoundary ? BOTTOM : TOP;
  nsIntRect quadrantRect = GetQuadrantRectangle(sideX, sideY);
  NS_ASSERTION(quadrantRect.Contains(drawBounds), "Messed up quadrants");
  result.mContext->Scale(aXResolution, aYResolution);
  result.mContext->Translate(-gfxPoint(quadrantRect.x, quadrantRect.y));

  gfxUtils::ClipToRegion(result.mContext, result.mRegionToDraw);
  if (aContentType == gfxASurface::CONTENT_COLOR_ALPHA && !isClear) {
    result.mContext->SetOperator(gfxContext::OPERATOR_CLEAR);
    result.mContext->Paint();
    result.mContext->SetOperator(gfxContext::OPERATOR_OVER);
  }
  return result;
}

}
}

