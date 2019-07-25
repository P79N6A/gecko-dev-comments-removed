




































#include "ThebesLayerBuffer.h"
#include "Layers.h"
#include "gfxContext.h"
#include "gfxPlatform.h"

namespace mozilla {
namespace layers {

 void
ThebesLayerBuffer::ClipToRegion(gfxContext* aContext,
                                const nsIntRegion& aRegion)
{
  aContext->NewPath();
  nsIntRegionRectIterator iter(aRegion);
  const nsIntRect* r;
  while ((r = iter.Next()) != nsnull) {
    aContext->Rectangle(gfxRect(r->x, r->y, r->width, r->height));
  }
  aContext->Clip();
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
                                      XSide aXSide, YSide aYSide, float aOpacity)
{
  
  
  
  
  nsIntRect quadrantRect = GetQuadrantRectangle(aXSide, aYSide);
  nsIntRect fillRect;
  if (!fillRect.IntersectRect(mBufferRect, quadrantRect))
    return;

  aTarget->NewPath();
  aTarget->Rectangle(gfxRect(fillRect.x, fillRect.y, fillRect.width, fillRect.height),
                     PR_TRUE);
  aTarget->SetSource(mBuffer, gfxPoint(quadrantRect.x, quadrantRect.y));
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
ThebesLayerBuffer::DrawBufferWithRotation(gfxContext* aTarget, float aOpacity)
{
  
  
  DrawBufferQuadrant(aTarget, LEFT, TOP, aOpacity);
  DrawBufferQuadrant(aTarget, RIGHT, TOP, aOpacity);
  DrawBufferQuadrant(aTarget, LEFT, BOTTOM, aOpacity);
  DrawBufferQuadrant(aTarget, RIGHT, BOTTOM, aOpacity);
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
ThebesLayerBuffer::BeginPaint(ThebesLayer* aLayer, ContentType aContentType)
{
  PaintState result;

  result.mRegionToDraw.Sub(aLayer->GetVisibleRegion(), aLayer->GetValidRegion());

  if (mBuffer && aContentType != mBuffer->GetContentType()) {
    
    
    result.mRegionToDraw = aLayer->GetVisibleRegion();
    result.mRegionToInvalidate = aLayer->GetValidRegion();
    Clear();
  }

  if (result.mRegionToDraw.IsEmpty())
    return result;
  nsIntRect drawBounds = result.mRegionToDraw.GetBounds();

  nsIntRect visibleBounds = aLayer->GetVisibleRegion().GetBounds();
  nsRefPtr<gfxASurface> destBuffer;
  nsIntRect destBufferRect;

  if (BufferSizeOkFor(visibleBounds.Size())) {
    
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
          destBuffer = CreateBuffer(aContentType, destBufferRect.Size());
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
    destBuffer = CreateBuffer(aContentType, destBufferRect.Size());
    if (!destBuffer)
      return result;
  }

  
  
  PRBool isClear = mBuffer == nsnull;

  if (destBuffer) {
    if (mBuffer) {
      
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
  result.mContext->Translate(-gfxPoint(quadrantRect.x, quadrantRect.y));

  ClipToRegion(result.mContext, result.mRegionToDraw);
  if (aContentType == gfxASurface::CONTENT_COLOR_ALPHA && !isClear) {
    result.mContext->SetOperator(gfxContext::OPERATOR_CLEAR);
    result.mContext->Paint();
    result.mContext->SetOperator(gfxContext::OPERATOR_OVER);
  }
  return result;
}

}
}

