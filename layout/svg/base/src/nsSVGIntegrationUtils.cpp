




































#include "nsSVGIntegrationUtils.h"

#include "nsSVGUtils.h"
#include "nsSVGEffects.h"
#include "nsRegion.h"
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"
#include "nsSVGFilterPaintCallback.h"
#include "nsSVGFilterFrame.h"
#include "nsSVGClipPathFrame.h"
#include "nsSVGMaskFrame.h"



PRBool
nsSVGIntegrationUtils::UsingEffectsForFrame(const nsIFrame* aFrame)
{
  const nsStyleSVGReset *style = aFrame->GetStyleSVGReset();
  return (style->mFilter || style->mClipPath || style->mMask) &&
          !aFrame->IsFrameOfType(nsIFrame::eSVG);
}



static nsRect GetNonSVGUserSpace(nsIFrame* aFirst)
{
  NS_ASSERTION(!aFirst->GetPrevContinuation(), "Not first continuation");
  return nsLayoutUtils::GetAllInFlowRectsUnion(aFirst, aFirst);
}

static nsRect
GetPreEffectsOverflowRect(nsIFrame* aFrame)
{
  nsRect* r = static_cast<nsRect*>(aFrame->GetProperty(nsGkAtoms::preEffectsBBoxProperty));
  if (r)
    return *r;
  return aFrame->GetOverflowRect();
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
GetSVGBBox(nsIFrame* aNonSVGFrame, nsIFrame* aCurrentFrame,
           const nsRect& aCurrentOverflow, const nsRect& aUserSpaceRect)
{
  NS_ASSERTION(!aNonSVGFrame->GetPrevContinuation(),
               "Need first continuation here");
  
  BBoxCollector collector(aNonSVGFrame, aCurrentFrame, aCurrentOverflow);
  nsLayoutUtils::GetAllInFlowBoxes(aNonSVGFrame, &collector);
  
  return collector.mResult - aUserSpaceRect.TopLeft();
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

  
  
  
  nsRect userSpaceRect = GetNonSVGUserSpace(firstFrame);
  nsRect r = GetSVGBBox(firstFrame, aFrame, aOverflowRect, userSpaceRect);
  
  PRUint32 appUnitsPerDevPixel = aFrame->PresContext()->AppUnitsPerDevPixel();
  nsIntRect p = r.ToOutsidePixels(appUnitsPerDevPixel);
  p = filterFrame->GetFilterBBox(firstFrame, &p);
  r = p.ToAppUnits(appUnitsPerDevPixel);
  
  return r + userSpaceRect.TopLeft() - aFrame->GetOffsetTo(firstFrame);
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
  nsSVGFilterFrame* filterFrame = nsSVGEffects::GetFilterFrame(firstFrame);
  if (!filterFrame) {
    
    
    
    return aFrame->GetOverflowRect();
  }

  PRInt32 appUnitsPerDevPixel = aFrame->PresContext()->AppUnitsPerDevPixel();
  nsRect userSpaceRect = GetNonSVGUserSpace(firstFrame);
  nsPoint offset = aFrame->GetOffsetTo(firstFrame) - userSpaceRect.TopLeft();
  nsRect r = aInvalidRect + offset;
  nsIntRect p = r.ToOutsidePixels(appUnitsPerDevPixel);
  p = filterFrame->GetInvalidationBBox(firstFrame, p);
  r = p.ToAppUnits(appUnitsPerDevPixel);
  return r - offset;
}

nsRect
nsSVGIntegrationUtils::GetRequiredSourceForInvalidArea(nsIFrame* aFrame,
                                                       const nsRect& aDamageRect)
{
  
  
  nsIFrame* firstFrame =
    nsLayoutUtils::GetFirstContinuationOrSpecialSibling(aFrame);
  nsSVGFilterFrame* filterFrame =
    nsSVGEffects::GetFilterFrame(firstFrame);
  if (!filterFrame)
    return aDamageRect;
  
  PRInt32 appUnitsPerDevPixel = aFrame->PresContext()->AppUnitsPerDevPixel();
  nsRect userSpaceRect = GetNonSVGUserSpace(firstFrame);
  nsPoint offset = aFrame->GetOffsetTo(firstFrame) - userSpaceRect.TopLeft();
  nsRect r = aDamageRect + offset;
  nsIntRect p = r.ToOutsidePixels(appUnitsPerDevPixel);
  p = filterFrame->GetSourceForInvalidArea(firstFrame, p);
  r = p.ToAppUnits(appUnitsPerDevPixel);
  return r - offset;
}

PRBool
nsSVGIntegrationUtils::HitTestFrameForEffects(nsIFrame* aFrame, const nsPoint& aPt)
{
  nsIFrame* firstFrame =
    nsLayoutUtils::GetFirstContinuationOrSpecialSibling(aFrame);
  nsRect userSpaceRect = GetNonSVGUserSpace(firstFrame);
  
  nsPoint pt = aPt + aFrame->GetOffsetTo(firstFrame) - userSpaceRect.TopLeft();
  return nsSVGUtils::HitTestClip(firstFrame, pt);
}

class RegularFramePaintCallback : public nsSVGFilterPaintCallback
{
public:
  RegularFramePaintCallback(nsDisplayListBuilder* aBuilder,
                            nsDisplayList* aInnerList,
                            const nsPoint& aOffset)
    : mBuilder(aBuilder), mInnerList(aInnerList), mOffset(aOffset) {}

  virtual void Paint(nsSVGRenderState *aContext, nsIFrame *aTarget,
                     const nsIntRect* aDirtyRect)
  {
    nsIRenderingContext* ctx = aContext->GetRenderingContext(aTarget);
    nsIRenderingContext::AutoPushTranslation push(ctx, -mOffset.x, -mOffset.y);
    mInnerList->Paint(mBuilder, ctx);
  }

private:
  nsDisplayListBuilder* mBuilder;
  nsDisplayList* mInnerList;
  nsPoint mOffset;
};

void
nsSVGIntegrationUtils::PaintFramesWithEffects(nsIRenderingContext* aCtx,
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

  













  PRBool isOK = PR_TRUE;
  nsSVGClipPathFrame *clipPathFrame = effectProperties.GetClipPathFrame(&isOK);
  nsSVGFilterFrame *filterFrame = effectProperties.GetFilterFrame(&isOK);
  nsSVGMaskFrame *maskFrame = effectProperties.GetMaskFrame(&isOK);

  PRBool isTrivialClip = clipPathFrame ? clipPathFrame->IsTrivial() : PR_TRUE;

  if (!isOK) {
    
    return;
  }

  gfxContext* gfx = aCtx->ThebesContext();
  gfxMatrix savedCTM = gfx->CurrentMatrix();
  nsSVGRenderState svgContext(aCtx);

  nsRect userSpaceRect = GetNonSVGUserSpace(firstFrame) + aBuilder->ToReferenceFrame(firstFrame);
  PRInt32 appUnitsPerDevPixel = aEffectsFrame->PresContext()->AppUnitsPerDevPixel();
  userSpaceRect = userSpaceRect.ToNearestPixels(appUnitsPerDevPixel).ToAppUnits(appUnitsPerDevPixel);
  aCtx->Translate(userSpaceRect.x, userSpaceRect.y);

  gfxMatrix matrix = GetInitialMatrix(aEffectsFrame);

  PRBool complexEffects = PR_FALSE;
  

  if (opacity != 1.0f || maskFrame || (clipPathFrame && !isTrivialClip)) {
    complexEffects = PR_TRUE;
    gfx->Save();
    gfx->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
  }

  


  if (clipPathFrame && isTrivialClip) {
    gfx->Save();
    clipPathFrame->ClipPaint(&svgContext, aEffectsFrame, matrix);
  }

  
  if (filterFrame) {
    RegularFramePaintCallback paint(aBuilder, aInnerList, userSpaceRect.TopLeft());
    nsIntRect r = (aDirtyRect - userSpaceRect.TopLeft()).ToOutsidePixels(appUnitsPerDevPixel);
    filterFrame->FilterPaint(&svgContext, aEffectsFrame, &paint, &r);
  } else {
    gfx->SetMatrix(savedCTM);
    aInnerList->Paint(aBuilder, aCtx);
    aCtx->Translate(userSpaceRect.x, userSpaceRect.y);
  }

  if (clipPathFrame && isTrivialClip) {
    gfx->Restore();
  }

  
  if (!complexEffects) {
    gfx->SetMatrix(savedCTM);
    return;
  }

  gfx->PopGroupToSource();

  nsRefPtr<gfxPattern> maskSurface =
    maskFrame ? maskFrame->ComputeMaskAlpha(&svgContext, aEffectsFrame,
                                            matrix, opacity) : nsnull;

  nsRefPtr<gfxPattern> clipMaskSurface;
  if (clipPathFrame && !isTrivialClip) {
    gfx->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);

    nsresult rv = clipPathFrame->ClipPaint(&svgContext, aEffectsFrame, matrix);
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
  gfx->SetMatrix(savedCTM);
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
nsSVGIntegrationUtils::GetSVGRectForNonSVGFrame(nsIFrame* aNonSVGFrame)
{
  NS_ASSERTION(!aNonSVGFrame->IsFrameOfType(nsIFrame::eSVG),
               "SVG frames should not get here");
  nsIFrame* firstFrame =
    nsLayoutUtils::GetFirstContinuationOrSpecialSibling(aNonSVGFrame);
  nsRect r = GetNonSVGUserSpace(firstFrame);
  nsPresContext* presContext = firstFrame->PresContext();
  return gfxRect(0, 0, presContext->AppUnitsToFloatCSSPixels(r.width),
                       presContext->AppUnitsToFloatCSSPixels(r.height));
}

gfxRect
nsSVGIntegrationUtils::GetSVGBBoxForNonSVGFrame(nsIFrame* aNonSVGFrame)
{
  NS_ASSERTION(!aNonSVGFrame->IsFrameOfType(nsIFrame::eSVG),
               "SVG frames should not get here");
  nsIFrame* firstFrame =
    nsLayoutUtils::GetFirstContinuationOrSpecialSibling(aNonSVGFrame);
  nsRect userSpaceRect = GetNonSVGUserSpace(firstFrame);
  nsRect r = GetSVGBBox(firstFrame, nsnull, nsRect(), userSpaceRect);
  gfxRect result(r.x, r.y, r.width, r.height);
  nsPresContext* presContext = aNonSVGFrame->PresContext();
  result.ScaleInverse(presContext->AppUnitsPerCSSPixel());
  return result;
}
