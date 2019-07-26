





#include "nsSVGIntegrationUtils.h"


#include "gfxDrawable.h"
#include "nsDisplayList.h"
#include "nsLayoutUtils.h"
#include "nsRenderingContext.h"
#include "nsSVGClipPathFrame.h"
#include "nsSVGEffects.h"
#include "nsSVGFilterFrame.h"
#include "nsSVGFilterPaintCallback.h"
#include "nsSVGMaskFrame.h"
#include "nsSVGPaintServerFrame.h"
#include "nsSVGUtils.h"



bool
nsSVGIntegrationUtils::UsingEffectsForFrame(const nsIFrame* aFrame)
{
  if (aFrame->IsFrameOfType(nsIFrame::eSVG)) {
    return false;
  }
  const nsStyleSVGReset *style = aFrame->GetStyleSVGReset();
  return (style->mFilter || style->mClipPath || style->mMask);
}

 nsPoint
nsSVGIntegrationUtils::GetOffsetToUserSpace(nsIFrame* aFrame)
{
  
  
  
  
  NS_ASSERTION(!aFrame->GetPrevContinuation(), "Not first continuation");

  
  
  
  return -nsLayoutUtils::GetAllInFlowRectsUnion(aFrame, aFrame).TopLeft();
}

 nsSize
nsSVGIntegrationUtils::GetContinuationUnionSize(nsIFrame* aNonSVGFrame)
{
  NS_ASSERTION(!aNonSVGFrame->IsFrameOfType(nsIFrame::eSVG),
               "SVG frames should not get here");
  nsIFrame* firstFrame =
    nsLayoutUtils::GetFirstContinuationOrSpecialSibling(aNonSVGFrame);
  return nsLayoutUtils::GetAllInFlowRectsUnion(firstFrame, firstFrame).Size();
}

 gfxSize
nsSVGIntegrationUtils::GetSVGCoordContextForNonSVGFrame(nsIFrame* aNonSVGFrame)
{
  NS_ASSERTION(!aNonSVGFrame->IsFrameOfType(nsIFrame::eSVG),
               "SVG frames should not get here");
  nsIFrame* firstFrame =
    nsLayoutUtils::GetFirstContinuationOrSpecialSibling(aNonSVGFrame);
  nsRect r = nsLayoutUtils::GetAllInFlowRectsUnion(firstFrame, firstFrame);
  nsPresContext* presContext = firstFrame->PresContext();
  return gfxSize(presContext->AppUnitsToFloatCSSPixels(r.width),
                 presContext->AppUnitsToFloatCSSPixels(r.height));
}

static nsRect
GetPreEffectsOverflowRect(nsIFrame* aFrame)
{
  nsRect* r = static_cast<nsRect*>
    (aFrame->Properties().Get(nsIFrame::PreEffectsBBoxProperty()));
  if (r)
    return *r;
  return aFrame->GetVisualOverflowRect();
}

struct BBoxCollector : public nsLayoutUtils::BoxCallback {
  nsIFrame*     mReferenceFrame;
  nsIFrame*     mCurrentFrame;
  const nsRect& mCurrentFrameOverflowArea;
  nsRect        mResult;

  BBoxCollector(nsIFrame* aReferenceFrame, nsIFrame* aCurrentFrame,
                const nsRect& aCurrentFrameOverflowArea)
    : mReferenceFrame(aReferenceFrame), mCurrentFrame(aCurrentFrame),
      mCurrentFrameOverflowArea(aCurrentFrameOverflowArea) {}

  virtual void AddBox(nsIFrame* aFrame) {
    nsRect overflow = aFrame == mCurrentFrame ? mCurrentFrameOverflowArea
        : GetPreEffectsOverflowRect(aFrame);
    mResult.UnionRect(mResult, overflow + aFrame->GetOffsetTo(mReferenceFrame));
  }
};

static nsRect
GetSVGBBox(nsIFrame* aFirstContinuation,
           nsIFrame* aCurrentFrame,
           const nsRect& aCurrentFramesPreEffectsOverflow,
           const nsPoint& aFirstContinuationToUserSpace)
{
  NS_ASSERTION(!aFirstContinuation->GetPrevContinuation(),
               "Need first continuation here");
  
  BBoxCollector collector(aFirstContinuation, aCurrentFrame,
                          aCurrentFramesPreEffectsOverflow);
  nsLayoutUtils::GetAllInFlowBoxes(aFirstContinuation, &collector);
  
  return collector.mResult + aFirstContinuationToUserSpace;
}

nsRect
nsSVGIntegrationUtils::ComputeFrameEffectsRect(nsIFrame* aFrame,
                                               const nsRect& aOverflowRect)
{
  nsIFrame* firstFrame =
    nsLayoutUtils::GetFirstContinuationOrSpecialSibling(aFrame);
  nsSVGEffects::EffectProperties effectProperties =
    nsSVGEffects::GetEffectProperties(firstFrame);
  nsSVGFilterFrame *filterFrame = effectProperties.mFilter ?
    effectProperties.mFilter->GetFilterFrame() : nsnull;
  if (!filterFrame)
    return aOverflowRect;

  
  
  
  nsPoint firstFrameToUserSpace = GetOffsetToUserSpace(firstFrame);
  nsRect r = GetSVGBBox(firstFrame, aFrame, aOverflowRect,
                        firstFrameToUserSpace);
  
  PRUint32 appUnitsPerDevPixel = aFrame->PresContext()->AppUnitsPerDevPixel();
  nsIntRect p = r.ToOutsidePixels(appUnitsPerDevPixel);
  p = filterFrame->GetPostFilterBounds(firstFrame, &p);
  r = p.ToAppUnits(appUnitsPerDevPixel);
  
  return r - (aFrame->GetOffsetTo(firstFrame) + firstFrameToUserSpace);
}

nsRect
nsSVGIntegrationUtils::GetInvalidAreaForChangedSource(nsIFrame* aFrame,
                                                      const nsRect& aInvalidRect)
{
  
  
  nsIFrame* firstFrame =
    nsLayoutUtils::GetFirstContinuationOrSpecialSibling(aFrame);
  nsSVGEffects::EffectProperties effectProperties =
    nsSVGEffects::GetEffectProperties(firstFrame);
  if (!effectProperties.mFilter)
    return aInvalidRect;

  nsSVGFilterProperty *prop = nsSVGEffects::GetFilterProperty(firstFrame);
  if (!prop || !prop->IsInObserverList()) {
    return aInvalidRect;
  }

  nsSVGFilterFrame* filterFrame = prop->GetFilterFrame();
  if (!filterFrame) {
    
    
    
    return aFrame->GetVisualOverflowRect();
  }

  
  PRInt32 appUnitsPerDevPixel = aFrame->PresContext()->AppUnitsPerDevPixel();
  nsPoint toUserSpace =
    aFrame->GetOffsetTo(firstFrame) + GetOffsetToUserSpace(firstFrame);
  nsIntRect preEffectsRect =
    (aInvalidRect + toUserSpace).ToOutsidePixels(appUnitsPerDevPixel);

  nsIntRect postEffectsRect =
    filterFrame->GetPostFilterDirtyArea(firstFrame, preEffectsRect);

  
  return postEffectsRect.ToAppUnits(appUnitsPerDevPixel) - toUserSpace;
}

nsRect
nsSVGIntegrationUtils::GetRequiredSourceForInvalidArea(nsIFrame* aFrame,
                                                       const nsRect& aDirtyRect)
{
  
  
  nsIFrame* firstFrame =
    nsLayoutUtils::GetFirstContinuationOrSpecialSibling(aFrame);
  nsSVGFilterFrame* filterFrame =
    nsSVGEffects::GetFilterFrame(firstFrame);
  if (!filterFrame)
    return aDirtyRect;
  
  
  PRInt32 appUnitsPerDevPixel = aFrame->PresContext()->AppUnitsPerDevPixel();
  nsPoint toUserSpace =
    aFrame->GetOffsetTo(firstFrame) + GetOffsetToUserSpace(firstFrame);
  nsIntRect postEffectsRect =
    (aDirtyRect + toUserSpace).ToOutsidePixels(appUnitsPerDevPixel);

  nsIntRect preEffectsRect =
    filterFrame->GetPreFilterNeededArea(firstFrame, postEffectsRect);

  
  return preEffectsRect.ToAppUnits(appUnitsPerDevPixel) - toUserSpace;
}

bool
nsSVGIntegrationUtils::HitTestFrameForEffects(nsIFrame* aFrame, const nsPoint& aPt)
{
  nsIFrame* firstFrame =
    nsLayoutUtils::GetFirstContinuationOrSpecialSibling(aFrame);
  
  nsPoint toUserSpace =
    aFrame->GetOffsetTo(firstFrame) + GetOffsetToUserSpace(firstFrame);
  nsPoint pt = aPt + toUserSpace;
  return nsSVGUtils::HitTestClip(firstFrame, pt);
}

class RegularFramePaintCallback : public nsSVGFilterPaintCallback
{
public:
  RegularFramePaintCallback(nsDisplayListBuilder* aBuilder,
                            nsDisplayList* aInnerList,
                            nsIFrame* aFrame,
                            const nsPoint& aOffset)
    : mBuilder(aBuilder), mInnerList(aInnerList), mFrame(aFrame),
      mOffset(aOffset) {}

  virtual void Paint(nsRenderingContext *aContext, nsIFrame *aTarget,
                     const nsIntRect* aDirtyRect)
  {
    nsRenderingContext::AutoPushTranslation push(aContext, -mOffset);
    mInnerList->PaintForFrame(mBuilder, aContext, mFrame, nsDisplayList::PAINT_DEFAULT);
  }

private:
  nsDisplayListBuilder* mBuilder;
  nsDisplayList* mInnerList;
  nsIFrame* mFrame;
  nsPoint mOffset;
};

void
nsSVGIntegrationUtils::PaintFramesWithEffects(nsRenderingContext* aCtx,
                                              nsIFrame* aEffectsFrame,
                                              const nsRect& aDirtyRect,
                                              nsDisplayListBuilder* aBuilder,
                                              nsDisplayList* aInnerList)
{
#ifdef DEBUG
  nsISVGChildFrame *svgChildFrame = do_QueryFrame(aEffectsFrame);
  NS_ASSERTION(!svgChildFrame, "Should never be called on an SVG frame");
#endif

  float opacity = aEffectsFrame->GetStyleDisplay()->mOpacity;
  if (opacity == 0.0f)
    return;

  

  nsIFrame* firstFrame =
    nsLayoutUtils::GetFirstContinuationOrSpecialSibling(aEffectsFrame);
  nsSVGEffects::EffectProperties effectProperties =
    nsSVGEffects::GetEffectProperties(firstFrame);

  













  bool isOK = true;
  nsSVGClipPathFrame *clipPathFrame = effectProperties.GetClipPathFrame(&isOK);
  nsSVGFilterFrame *filterFrame = effectProperties.GetFilterFrame(&isOK);
  nsSVGMaskFrame *maskFrame = effectProperties.GetMaskFrame(&isOK);

  bool isTrivialClip = clipPathFrame ? clipPathFrame->IsTrivial() : true;

  if (!isOK) {
    
    return;
  }

  gfxContext* gfx = aCtx->ThebesContext();
  gfxContextMatrixAutoSaveRestore matrixAutoSaveRestore(gfx);

  PRInt32 appUnitsPerDevPixel = 
    aEffectsFrame->PresContext()->AppUnitsPerDevPixel();
  nsPoint firstFrameOffset = GetOffsetToUserSpace(firstFrame);
  nsPoint offset = (aBuilder->ToReferenceFrame(firstFrame) - firstFrameOffset).
                     ToNearestPixels(appUnitsPerDevPixel).
                     ToAppUnits(appUnitsPerDevPixel);
  aCtx->Translate(offset);

  gfxMatrix matrix = GetInitialMatrix(aEffectsFrame);

  bool complexEffects = false;
  

  if (opacity != 1.0f || maskFrame || (clipPathFrame && !isTrivialClip)) {
    complexEffects = true;
    gfx->Save();
    aCtx->IntersectClip(aEffectsFrame->GetVisualOverflowRect());
    gfx->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
  }

  


  if (clipPathFrame && isTrivialClip) {
    gfx->Save();
    clipPathFrame->ClipPaint(aCtx, aEffectsFrame, matrix);
  }

  
  if (filterFrame) {
    RegularFramePaintCallback callback(aBuilder, aInnerList, aEffectsFrame,
                                       offset);
    nsIntRect dirtyRect = (aDirtyRect - offset)
                            .ToOutsidePixels(appUnitsPerDevPixel);
    filterFrame->PaintFilteredFrame(aCtx, aEffectsFrame, &callback, &dirtyRect);
  } else {
    gfx->SetMatrix(matrixAutoSaveRestore.Matrix());
    aInnerList->PaintForFrame(aBuilder, aCtx, aEffectsFrame,
                              nsDisplayList::PAINT_DEFAULT);
    aCtx->Translate(offset);
  }

  if (clipPathFrame && isTrivialClip) {
    gfx->Restore();
  }

  
  if (!complexEffects) {
    return;
  }

  gfx->PopGroupToSource();

  nsRefPtr<gfxPattern> maskSurface =
    maskFrame ? maskFrame->ComputeMaskAlpha(aCtx, aEffectsFrame,
                                            matrix, opacity) : nsnull;

  nsRefPtr<gfxPattern> clipMaskSurface;
  if (clipPathFrame && !isTrivialClip) {
    gfx->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);

    nsresult rv = clipPathFrame->ClipPaint(aCtx, aEffectsFrame, matrix);
    clipMaskSurface = gfx->PopGroup();

    if (NS_SUCCEEDED(rv) && clipMaskSurface) {
      
      if (maskSurface || opacity != 1.0f) {
        gfx->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
        gfx->Mask(clipMaskSurface);
        gfx->PopGroupToSource();
      } else {
        gfx->Mask(clipMaskSurface);
      }
    }
  }

  if (maskSurface) {
    gfx->Mask(maskSurface);
  } else if (opacity != 1.0f) {
    gfx->Paint(opacity);
  }

  gfx->Restore();
}

gfxMatrix
nsSVGIntegrationUtils::GetInitialMatrix(nsIFrame* aNonSVGFrame)
{
  NS_ASSERTION(!aNonSVGFrame->IsFrameOfType(nsIFrame::eSVG),
               "SVG frames should not get here");
  PRInt32 appUnitsPerDevPixel = aNonSVGFrame->PresContext()->AppUnitsPerDevPixel();
  float devPxPerCSSPx =
    1 / nsPresContext::AppUnitsToFloatCSSPixels(appUnitsPerDevPixel);

  return gfxMatrix(devPxPerCSSPx, 0.0,
                   0.0, devPxPerCSSPx,
                   0.0, 0.0);
}

gfxRect
nsSVGIntegrationUtils::GetSVGBBoxForNonSVGFrame(nsIFrame* aNonSVGFrame)
{
  NS_ASSERTION(!aNonSVGFrame->IsFrameOfType(nsIFrame::eSVG),
               "SVG frames should not get here");
  nsIFrame* firstFrame =
    nsLayoutUtils::GetFirstContinuationOrSpecialSibling(aNonSVGFrame);
  
  nsRect r = GetSVGBBox(firstFrame, nsnull, nsRect(),
                        GetOffsetToUserSpace(firstFrame));
  return nsLayoutUtils::RectToGfxRect(r,
           aNonSVGFrame->PresContext()->AppUnitsPerCSSPixel());
}

class PaintFrameCallback : public gfxDrawingCallback {
public:
  PaintFrameCallback(nsIFrame* aFrame,
                     nsIFrame* aTarget,
                     const nsSize aPaintServerSize,
                     const gfxIntSize aRenderSize)
   : mFrame(aFrame)
   , mTarget(aTarget)
   , mPaintServerSize(aPaintServerSize)
   , mRenderSize(aRenderSize)
  {}
  virtual bool operator()(gfxContext* aContext,
                            const gfxRect& aFillRect,
                            const gfxPattern::GraphicsFilter& aFilter,
                            const gfxMatrix& aTransform);
private:
  nsIFrame* mFrame;
  nsIFrame* mTarget;
  nsSize mPaintServerSize;
  gfxIntSize mRenderSize;
};

bool
PaintFrameCallback::operator()(gfxContext* aContext,
                               const gfxRect& aFillRect,
                               const gfxPattern::GraphicsFilter& aFilter,
                               const gfxMatrix& aTransform)
{
  if (mFrame->GetStateBits() & NS_FRAME_DRAWING_AS_PAINTSERVER)
    return false;

  mFrame->AddStateBits(NS_FRAME_DRAWING_AS_PAINTSERVER);

  nsRenderingContext context;
  context.Init(mFrame->PresContext()->DeviceContext(), aContext);
  aContext->Save();

  
  aContext->NewPath();
  aContext->Rectangle(aFillRect);
  aContext->Clip();

  aContext->Multiply(gfxMatrix(aTransform).Invert());

  
  
  
  PRInt32 appUnitsPerDevPixel = mFrame->PresContext()->AppUnitsPerDevPixel();
  nsPoint offset = nsSVGIntegrationUtils::GetOffsetToUserSpace(mFrame);
  gfxPoint devPxOffset = gfxPoint(offset.x, offset.y) / appUnitsPerDevPixel;
  aContext->Multiply(gfxMatrix().Translate(devPxOffset));

  gfxSize paintServerSize =
    gfxSize(mPaintServerSize.width, mPaintServerSize.height) /
      mFrame->PresContext()->AppUnitsPerDevPixel();

  
  
  gfxFloat scaleX = mRenderSize.width / paintServerSize.width;
  gfxFloat scaleY = mRenderSize.height / paintServerSize.height;
  gfxMatrix scaleMatrix = gfxMatrix().Scale(scaleX, scaleY);
  aContext->Multiply(scaleMatrix);

  
  nsRect dirty(-offset.x, -offset.y,
               mPaintServerSize.width, mPaintServerSize.height);
  nsLayoutUtils::PaintFrame(&context, mFrame,
                            dirty, NS_RGBA(0, 0, 0, 0),
                            nsLayoutUtils::PAINT_IN_TRANSFORM |
                            nsLayoutUtils::PAINT_ALL_CONTINUATIONS);

  aContext->Restore();

  mFrame->RemoveStateBits(NS_FRAME_DRAWING_AS_PAINTSERVER);

  return true;
}

static already_AddRefed<gfxDrawable>
DrawableFromPaintServer(nsIFrame*         aFrame,
                        nsIFrame*         aTarget,
                        const nsSize&     aPaintServerSize,
                        const gfxIntSize& aRenderSize)
{
  
  
  
  
  
  
  if (aFrame->IsFrameOfType(nsIFrame::eSVGPaintServer)) {
    
    
    
    nsSVGPaintServerFrame* server =
      static_cast<nsSVGPaintServerFrame*>(aFrame);

    gfxRect overrideBounds(0, 0,
                           aPaintServerSize.width, aPaintServerSize.height);
    overrideBounds.ScaleInverse(aFrame->PresContext()->AppUnitsPerDevPixel());
    nsRefPtr<gfxPattern> pattern =
      server->GetPaintServerPattern(aTarget, &nsStyleSVG::mFill, 1.0, &overrideBounds);

    if (!pattern)
      return nsnull;

    
    
    
    
    
    gfxFloat scaleX = overrideBounds.Width() / aRenderSize.width;
    gfxFloat scaleY = overrideBounds.Height() / aRenderSize.height;
    gfxMatrix scaleMatrix = gfxMatrix().Scale(scaleX, scaleY);
    pattern->SetMatrix(scaleMatrix.Multiply(pattern->GetMatrix()));
    nsRefPtr<gfxDrawable> drawable =
      new gfxPatternDrawable(pattern, aRenderSize);
    return drawable.forget();
  }

  
  
  nsRefPtr<gfxDrawingCallback> cb =
    new PaintFrameCallback(aFrame, aTarget, aPaintServerSize, aRenderSize);
  nsRefPtr<gfxDrawable> drawable = new gfxCallbackDrawable(cb, aRenderSize);
  return drawable.forget();
}

 void
nsSVGIntegrationUtils::DrawPaintServer(nsRenderingContext* aRenderingContext,
                                       nsIFrame*            aTarget,
                                       nsIFrame*            aPaintServer,
                                       gfxPattern::GraphicsFilter aFilter,
                                       const nsRect&        aDest,
                                       const nsRect&        aFill,
                                       const nsPoint&       aAnchor,
                                       const nsRect&        aDirty,
                                       const nsSize&        aPaintServerSize)
{
  if (aDest.IsEmpty() || aFill.IsEmpty())
    return;

  PRInt32 appUnitsPerDevPixel = aTarget->PresContext()->AppUnitsPerDevPixel();
  nsRect destSize = aDest - aDest.TopLeft();
  nsIntSize roundedOut = destSize.ToOutsidePixels(appUnitsPerDevPixel).Size();
  gfxIntSize imageSize(roundedOut.width, roundedOut.height);
  nsRefPtr<gfxDrawable> drawable =
    DrawableFromPaintServer(aPaintServer, aTarget, aPaintServerSize, imageSize);

  if (drawable) {
    nsLayoutUtils::DrawPixelSnapped(aRenderingContext, drawable, aFilter,
                                    aDest, aFill, aAnchor, aDirty);
  }
}
