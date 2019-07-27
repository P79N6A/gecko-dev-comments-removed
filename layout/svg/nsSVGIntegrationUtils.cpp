





#include "nsSVGIntegrationUtils.h"


#include "gfxDrawable.h"
#include "nsCSSAnonBoxes.h"
#include "nsDisplayList.h"
#include "nsFilterInstance.h"
#include "nsLayoutUtils.h"
#include "nsRenderingContext.h"
#include "nsSVGClipPathFrame.h"
#include "nsSVGEffects.h"
#include "nsSVGElement.h"
#include "nsSVGFilterPaintCallback.h"
#include "nsSVGMaskFrame.h"
#include "nsSVGPaintServerFrame.h"
#include "nsSVGUtils.h"
#include "FrameLayerBuilder.h"
#include "BasicLayers.h"
#include "mozilla/gfx/Point.h"

using namespace mozilla;
using namespace mozilla::layers;










class PreEffectsVisualOverflowCollector : public nsLayoutUtils::BoxCallback
{
public:
  






  PreEffectsVisualOverflowCollector(nsIFrame* aFirstContinuation,
                                    nsIFrame* aCurrentFrame,
                                    const nsRect& aCurrentFrameOverflowArea)
    : mFirstContinuation(aFirstContinuation)
    , mCurrentFrame(aCurrentFrame)
    , mCurrentFrameOverflowArea(aCurrentFrameOverflowArea)
  {
    NS_ASSERTION(!mFirstContinuation->GetPrevContinuation(),
                 "We want the first continuation here");
  }

  virtual void AddBox(nsIFrame* aFrame) MOZ_OVERRIDE {
    nsRect overflow = (aFrame == mCurrentFrame) ?
      mCurrentFrameOverflowArea : GetPreEffectsVisualOverflowRect(aFrame);
    mResult.UnionRect(mResult, overflow + aFrame->GetOffsetTo(mFirstContinuation));
  }

  nsRect GetResult() const {
    return mResult;
  }

private:

  static nsRect GetPreEffectsVisualOverflowRect(nsIFrame* aFrame) {
    nsRect* r = static_cast<nsRect*>
      (aFrame->Properties().Get(nsIFrame::PreEffectsBBoxProperty()));
    if (r) {
      return *r;
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    NS_ASSERTION(aFrame->GetParent()->StyleContext()->GetPseudo() ==
                   nsCSSAnonBoxes::mozAnonymousBlock,
                 "How did we getting here, then?");
    NS_ASSERTION(!aFrame->Properties().Get(
                   aFrame->PreTransformOverflowAreasProperty()),
                 "GetVisualOverflowRect() won't return the pre-effects rect!");
    return aFrame->GetVisualOverflowRect();
  }

  nsIFrame*     mFirstContinuation;
  nsIFrame*     mCurrentFrame;
  const nsRect& mCurrentFrameOverflowArea;
  nsRect        mResult;
};





static nsRect
GetPreEffectsVisualOverflowUnion(nsIFrame* aFirstContinuation,
                                 nsIFrame* aCurrentFrame,
                                 const nsRect& aCurrentFramePreEffectsOverflow,
                                 const nsPoint& aFirstContinuationToUserSpace)
{
  NS_ASSERTION(!aFirstContinuation->GetPrevContinuation(),
               "Need first continuation here");
  PreEffectsVisualOverflowCollector collector(aFirstContinuation,
                                              aCurrentFrame,
                                              aCurrentFramePreEffectsOverflow);
  
  nsLayoutUtils::GetAllInFlowBoxes(aFirstContinuation, &collector);
  
  return collector.GetResult() + aFirstContinuationToUserSpace;
}


bool
nsSVGIntegrationUtils::UsingEffectsForFrame(const nsIFrame* aFrame)
{
  
  
  
  
  const nsStyleSVGReset *style = aFrame->StyleSVGReset();
  return (style->HasFilters() || style->mClipPath || style->mMask);
}



static nsPoint
GetOffsetToBoundingBox(nsIFrame* aFrame)
{
  if ((aFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT)) {
    
    
    
    
    return nsPoint();
  }
  
  
  
  
  NS_ASSERTION(!aFrame->GetPrevContinuation(), "Not first continuation");

  
  
  
  return -nsLayoutUtils::GetAllInFlowRectsUnion(aFrame, aFrame).TopLeft();
}

 nsSize
nsSVGIntegrationUtils::GetContinuationUnionSize(nsIFrame* aNonSVGFrame)
{
  NS_ASSERTION(!aNonSVGFrame->IsFrameOfType(nsIFrame::eSVG),
               "SVG frames should not get here");
  nsIFrame* firstFrame =
    nsLayoutUtils::FirstContinuationOrIBSplitSibling(aNonSVGFrame);
  return nsLayoutUtils::GetAllInFlowRectsUnion(firstFrame, firstFrame).Size();
}

 gfx::Size
nsSVGIntegrationUtils::GetSVGCoordContextForNonSVGFrame(nsIFrame* aNonSVGFrame)
{
  NS_ASSERTION(!aNonSVGFrame->IsFrameOfType(nsIFrame::eSVG),
               "SVG frames should not get here");
  nsIFrame* firstFrame =
    nsLayoutUtils::FirstContinuationOrIBSplitSibling(aNonSVGFrame);
  nsRect r = nsLayoutUtils::GetAllInFlowRectsUnion(firstFrame, firstFrame);
  nsPresContext* presContext = firstFrame->PresContext();
  return gfx::Size(presContext->AppUnitsToFloatCSSPixels(r.width),
                   presContext->AppUnitsToFloatCSSPixels(r.height));
}

gfxRect
nsSVGIntegrationUtils::GetSVGBBoxForNonSVGFrame(nsIFrame* aNonSVGFrame)
{
  NS_ASSERTION(!aNonSVGFrame->IsFrameOfType(nsIFrame::eSVG),
               "SVG frames should not get here");
  nsIFrame* firstFrame =
    nsLayoutUtils::FirstContinuationOrIBSplitSibling(aNonSVGFrame);
  
  nsRect r = GetPreEffectsVisualOverflowUnion(firstFrame, nullptr, nsRect(),
                                              GetOffsetToBoundingBox(firstFrame));
  return nsLayoutUtils::RectToGfxRect(r,
           aNonSVGFrame->PresContext()->AppUnitsPerCSSPixel());
}

































nsRect
  nsSVGIntegrationUtils::
    ComputePostEffectsVisualOverflowRect(nsIFrame* aFrame,
                                         const nsRect& aPreEffectsOverflowRect)
{
  NS_ASSERTION(!(aFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT),
                 "Don't call this on SVG child frames");

  nsIFrame* firstFrame =
    nsLayoutUtils::FirstContinuationOrIBSplitSibling(aFrame);
  nsSVGEffects::EffectProperties effectProperties =
    nsSVGEffects::GetEffectProperties(firstFrame);
  if (!effectProperties.HasValidFilter()) {
    return aPreEffectsOverflowRect;
  }

  
  nsPoint firstFrameToBoundingBox = GetOffsetToBoundingBox(firstFrame);
  
  
  
  gfxRect overrideBBox =
    nsLayoutUtils::RectToGfxRect(
      GetPreEffectsVisualOverflowUnion(firstFrame, aFrame,
                                       aPreEffectsOverflowRect,
                                       firstFrameToBoundingBox),
      aFrame->PresContext()->AppUnitsPerCSSPixel());
  overrideBBox.RoundOut();

  nsRect overflowRect =
    nsFilterInstance::GetPostFilterBounds(firstFrame, &overrideBBox);

  
  return overflowRect - (aFrame->GetOffsetTo(firstFrame) + firstFrameToBoundingBox);
}

nsIntRegion
nsSVGIntegrationUtils::AdjustInvalidAreaForSVGEffects(nsIFrame* aFrame,
                                                      const nsPoint& aToReferenceFrame,
                                                      const nsIntRegion& aInvalidRegion)
{
  if (aInvalidRegion.IsEmpty()) {
    return nsIntRect();
  }

  
  
  nsIFrame* firstFrame =
    nsLayoutUtils::FirstContinuationOrIBSplitSibling(aFrame);
  nsSVGFilterProperty *prop = nsSVGEffects::GetFilterProperty(firstFrame);
  if (!prop || !prop->IsInObserverLists()) {
    return aInvalidRegion;
  }

  int32_t appUnitsPerDevPixel = aFrame->PresContext()->AppUnitsPerDevPixel();

  if (!prop || !prop->ReferencesValidResources()) {
    
    
    
    
    nsRect overflow = aFrame->GetVisualOverflowRect() + aToReferenceFrame;
    return overflow.ToOutsidePixels(appUnitsPerDevPixel);
  }

  
  nsPoint toBoundingBox =
    aFrame->GetOffsetTo(firstFrame) + GetOffsetToBoundingBox(firstFrame);
  
  
  toBoundingBox -= aToReferenceFrame;
  nsRegion preEffectsRegion = aInvalidRegion.ToAppUnits(appUnitsPerDevPixel).MovedBy(toBoundingBox);

  
  
  nsRegion result = nsFilterInstance::GetPostFilterDirtyArea(firstFrame,
    preEffectsRegion).MovedBy(-toBoundingBox);
  
  return result.ToOutsidePixels(appUnitsPerDevPixel);
}

nsRect
nsSVGIntegrationUtils::GetRequiredSourceForInvalidArea(nsIFrame* aFrame,
                                                       const nsRect& aDirtyRect)
{
  
  
  nsIFrame* firstFrame =
    nsLayoutUtils::FirstContinuationOrIBSplitSibling(aFrame);
  nsSVGFilterProperty *prop = nsSVGEffects::GetFilterProperty(firstFrame);
  if (!prop || !prop->ReferencesValidResources()) {
    return aDirtyRect;
  }

  
  nsPoint toUserSpace =
    aFrame->GetOffsetTo(firstFrame) + GetOffsetToBoundingBox(firstFrame);
  nsRect postEffectsRect = aDirtyRect + toUserSpace;

  
  return nsFilterInstance::GetPreFilterNeededArea(firstFrame, postEffectsRect).GetBounds()
    - toUserSpace;
}

bool
nsSVGIntegrationUtils::HitTestFrameForEffects(nsIFrame* aFrame, const nsPoint& aPt)
{
  nsIFrame* firstFrame =
    nsLayoutUtils::FirstContinuationOrIBSplitSibling(aFrame);
  
  nsPoint toUserSpace;
  if (aFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT) {
    
    toUserSpace = aFrame->GetPosition();
  } else {
    toUserSpace =
      aFrame->GetOffsetTo(firstFrame) + GetOffsetToBoundingBox(firstFrame);
  }
  nsPoint pt = aPt + toUserSpace;
  gfxPoint userSpacePt =
    gfxPoint(pt.x, pt.y) / aFrame->PresContext()->AppUnitsPerCSSPixel();
  return nsSVGUtils::HitTestClip(firstFrame, userSpacePt);
}

class RegularFramePaintCallback : public nsSVGFilterPaintCallback
{
public:
  RegularFramePaintCallback(nsDisplayListBuilder* aBuilder,
                            LayerManager* aManager,
                            const nsPoint& aOffset)
    : mBuilder(aBuilder), mLayerManager(aManager),
      mOffset(aOffset) {}

  virtual void Paint(nsRenderingContext *aContext, nsIFrame *aTarget,
                     const gfxMatrix& aTransform,
                     const nsIntRect* aDirtyRect) MOZ_OVERRIDE
  {
    BasicLayerManager* basic = static_cast<BasicLayerManager*>(mLayerManager);
    basic->SetTarget(aContext->ThebesContext());
    nsRenderingContext::AutoPushTranslation push(aContext, -mOffset);
    mLayerManager->EndTransaction(FrameLayerBuilder::DrawThebesLayer, mBuilder);
  }

private:
  nsDisplayListBuilder* mBuilder;
  LayerManager* mLayerManager;
  nsPoint mOffset;
};

void
nsSVGIntegrationUtils::PaintFramesWithEffects(nsRenderingContext* aCtx,
                                              nsIFrame* aFrame,
                                              const nsRect& aDirtyRect,
                                              nsDisplayListBuilder* aBuilder,
                                              LayerManager *aLayerManager)
{
#ifdef DEBUG
  NS_ASSERTION(!(aFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT) ||
               (NS_SVGDisplayListPaintingEnabled() &&
                !(aFrame->GetStateBits() & NS_FRAME_IS_NONDISPLAY)),
               "Should not use nsSVGIntegrationUtils on this SVG frame");
#endif

  













  const nsIContent* content = aFrame->GetContent();
  bool hasSVGLayout = (aFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT);
  if (hasSVGLayout) {
    nsISVGChildFrame *svgChildFrame = do_QueryFrame(aFrame);
    if (!svgChildFrame || !aFrame->GetContent()->IsSVG()) {
      NS_ASSERTION(false, "why?");
      return;
    }
    if (!static_cast<const nsSVGElement*>(content)->HasValidDimensions()) {
      return; 
    }
  }

  float opacity = aFrame->StyleDisplay()->mOpacity;
  if (opacity == 0.0f) {
    return;
  }
  if (opacity != 1.0f &&
      hasSVGLayout && nsSVGUtils::CanOptimizeOpacity(aFrame)) {
    opacity = 1.0f;
  }

  

  nsIFrame* firstFrame =
    nsLayoutUtils::FirstContinuationOrIBSplitSibling(aFrame);
  nsSVGEffects::EffectProperties effectProperties =
    nsSVGEffects::GetEffectProperties(firstFrame);

  bool isOK = effectProperties.HasNoFilterOrHasValidFilter();
  nsSVGClipPathFrame *clipPathFrame = effectProperties.GetClipPathFrame(&isOK);
  nsSVGMaskFrame *maskFrame = effectProperties.GetMaskFrame(&isOK);
  if (!isOK) {
    return; 
  }

  bool isTrivialClip = clipPathFrame ? clipPathFrame->IsTrivial() : true;

  gfxContext* gfx = aCtx->ThebesContext();
  gfxContextMatrixAutoSaveRestore matrixAutoSaveRestore(gfx);

  nsPoint firstFrameOffset = GetOffsetToBoundingBox(firstFrame);
  nsPoint offsetToBoundingBox = aBuilder->ToReferenceFrame(firstFrame) - firstFrameOffset;
  if (!firstFrame->IsFrameOfType(nsIFrame::eSVG)) {
    

    offsetToBoundingBox = nsPoint(
      aFrame->PresContext()->RoundAppUnitsToNearestDevPixels(offsetToBoundingBox.x),
      aFrame->PresContext()->RoundAppUnitsToNearestDevPixels(offsetToBoundingBox.y));
  }

  
  
  
  
  
  
  
  
  
  

  gfxPoint toUserSpaceGfx = nsSVGUtils::FrameSpaceInCSSPxToUserSpaceOffset(aFrame);
  nsPoint toUserSpace(nsPresContext::CSSPixelsToAppUnits(float(toUserSpaceGfx.x)),
                      nsPresContext::CSSPixelsToAppUnits(float(toUserSpaceGfx.y)));
  nsPoint offsetToUserSpace = offsetToBoundingBox - toUserSpace;

  NS_ASSERTION(hasSVGLayout || offsetToBoundingBox == offsetToUserSpace,
               "For non-SVG frames there shouldn't be any additional offset");

  aCtx->Translate(offsetToUserSpace);

  gfxMatrix cssPxToDevPxMatrix = GetCSSPxToDevPxMatrix(aFrame);

  bool complexEffects = false;
  

  if (opacity != 1.0f || maskFrame || (clipPathFrame && !isTrivialClip)
      || aFrame->StyleDisplay()->mMixBlendMode != NS_STYLE_BLEND_NORMAL) {
    complexEffects = true;
    gfx->Save();
    aCtx->IntersectClip(aFrame->GetVisualOverflowRectRelativeToSelf() +
                        toUserSpace);
    gfx->PushGroup(gfxContentType::COLOR_ALPHA);
  }

  


  if (clipPathFrame && isTrivialClip) {
    gfx->Save();
    clipPathFrame->ApplyClipOrPaintClipMask(aCtx, aFrame, cssPxToDevPxMatrix);
  }

  
  if (effectProperties.HasValidFilter()) {
    RegularFramePaintCallback callback(aBuilder, aLayerManager,
                                       offsetToUserSpace);

    nsRegion dirtyRegion = aDirtyRect - offsetToBoundingBox;
    gfxMatrix tm = nsSVGIntegrationUtils::GetCSSPxToDevPxMatrix(aFrame);
    nsFilterInstance::PaintFilteredFrame(aFrame, aCtx, tm, &callback, &dirtyRegion);
  } else {
    gfx->SetMatrix(matrixAutoSaveRestore.Matrix());
    aLayerManager->EndTransaction(FrameLayerBuilder::DrawThebesLayer, aBuilder);
    aCtx->Translate(offsetToUserSpace);
  }

  if (clipPathFrame && isTrivialClip) {
    gfx->Restore();
  }

  
  if (!complexEffects) {
    return;
  }

  gfx->PopGroupToSource();

  nsRefPtr<gfxPattern> maskSurface =
    maskFrame ? maskFrame->GetMaskForMaskedFrame(aCtx->ThebesContext(),
                                                 aFrame, cssPxToDevPxMatrix,
                                                 opacity)
              : nullptr;

  nsRefPtr<gfxPattern> clipMaskSurface;
  if (clipPathFrame && !isTrivialClip) {
    gfx->PushGroup(gfxContentType::COLOR_ALPHA);

    nsresult rv = clipPathFrame->ApplyClipOrPaintClipMask(aCtx, aFrame, cssPxToDevPxMatrix);
    clipMaskSurface = gfx->PopGroup();

    if (NS_SUCCEEDED(rv) && clipMaskSurface) {
      
      if (maskSurface || opacity != 1.0f) {
        gfx->PushGroup(gfxContentType::COLOR_ALPHA);
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
nsSVGIntegrationUtils::GetCSSPxToDevPxMatrix(nsIFrame* aNonSVGFrame)
{
  int32_t appUnitsPerDevPixel = aNonSVGFrame->PresContext()->AppUnitsPerDevPixel();
  float devPxPerCSSPx =
    1 / nsPresContext::AppUnitsToFloatCSSPixels(appUnitsPerDevPixel);

  return gfxMatrix(devPxPerCSSPx, 0.0,
                   0.0, devPxPerCSSPx,
                   0.0, 0.0);
}

class PaintFrameCallback : public gfxDrawingCallback {
public:
  PaintFrameCallback(nsIFrame* aFrame,
                     const nsSize aPaintServerSize,
                     const gfxIntSize aRenderSize,
                     uint32_t aFlags)
   : mFrame(aFrame)
   , mPaintServerSize(aPaintServerSize)
   , mRenderSize(aRenderSize)
   , mFlags (aFlags)
  {}
  virtual bool operator()(gfxContext* aContext,
                            const gfxRect& aFillRect,
                            const GraphicsFilter& aFilter,
                            const gfxMatrix& aTransform) MOZ_OVERRIDE;
private:
  nsIFrame* mFrame;
  nsSize mPaintServerSize;
  gfxIntSize mRenderSize;
  uint32_t mFlags;
};

bool
PaintFrameCallback::operator()(gfxContext* aContext,
                               const gfxRect& aFillRect,
                               const GraphicsFilter& aFilter,
                               const gfxMatrix& aTransform)
{
  if (mFrame->GetStateBits() & NS_FRAME_DRAWING_AS_PAINTSERVER)
    return false;

  mFrame->AddStateBits(NS_FRAME_DRAWING_AS_PAINTSERVER);

  nsRefPtr<nsRenderingContext> context(new nsRenderingContext());
  context->Init(mFrame->PresContext()->DeviceContext(), aContext);
  aContext->Save();

  
  aContext->NewPath();
  aContext->Rectangle(aFillRect);
  aContext->Clip();

  gfxMatrix invmatrix = aTransform;
  if (!invmatrix.Invert()) {
    return false;
  }
  aContext->Multiply(invmatrix);

  
  
  
  int32_t appUnitsPerDevPixel = mFrame->PresContext()->AppUnitsPerDevPixel();
  nsPoint offset = GetOffsetToBoundingBox(mFrame);
  gfxPoint devPxOffset = gfxPoint(offset.x, offset.y) / appUnitsPerDevPixel;
  aContext->Multiply(gfxMatrix::Translation(devPxOffset));

  gfxSize paintServerSize =
    gfxSize(mPaintServerSize.width, mPaintServerSize.height) /
      mFrame->PresContext()->AppUnitsPerDevPixel();

  
  
  gfxFloat scaleX = mRenderSize.width / paintServerSize.width;
  gfxFloat scaleY = mRenderSize.height / paintServerSize.height;
  aContext->Multiply(gfxMatrix::Scaling(scaleX, scaleY));

  
  nsRect dirty(-offset.x, -offset.y,
               mPaintServerSize.width, mPaintServerSize.height);

  uint32_t flags = nsLayoutUtils::PAINT_IN_TRANSFORM |
                   nsLayoutUtils::PAINT_ALL_CONTINUATIONS;
  if (mFlags & nsSVGIntegrationUtils::FLAG_SYNC_DECODE_IMAGES) {
    flags |= nsLayoutUtils::PAINT_SYNC_DECODE_IMAGES;
  }
  nsLayoutUtils::PaintFrame(context, mFrame,
                            dirty, NS_RGBA(0, 0, 0, 0),
                            flags);

  aContext->Restore();

  mFrame->RemoveStateBits(NS_FRAME_DRAWING_AS_PAINTSERVER);

  return true;
}

 already_AddRefed<gfxDrawable>
nsSVGIntegrationUtils::DrawableFromPaintServer(nsIFrame*         aFrame,
                                               nsIFrame*         aTarget,
                                               const nsSize&     aPaintServerSize,
                                               const gfxIntSize& aRenderSize,
                                               const gfxMatrix&  aContextMatrix,
                                               uint32_t          aFlags)
{
  
  
  
  
  
  
  if (aFrame->IsFrameOfType(nsIFrame::eSVGPaintServer)) {
    
    
    
    nsSVGPaintServerFrame* server =
      static_cast<nsSVGPaintServerFrame*>(aFrame);

    gfxRect overrideBounds(0, 0,
                           aPaintServerSize.width, aPaintServerSize.height);
    overrideBounds.ScaleInverse(aFrame->PresContext()->AppUnitsPerDevPixel());
    nsRefPtr<gfxPattern> pattern =
      server->GetPaintServerPattern(aTarget, aContextMatrix,
                                    &nsStyleSVG::mFill, 1.0, &overrideBounds);

    if (!pattern)
      return nullptr;

    
    
    
    
    
    gfxFloat scaleX = overrideBounds.Width() / aRenderSize.width;
    gfxFloat scaleY = overrideBounds.Height() / aRenderSize.height;
    gfxMatrix scaleMatrix = gfxMatrix::Scaling(scaleX, scaleY);
    pattern->SetMatrix(scaleMatrix * pattern->GetMatrix());
    nsRefPtr<gfxDrawable> drawable =
      new gfxPatternDrawable(pattern, aRenderSize);
    return drawable.forget();
  }

  
  
  nsRefPtr<gfxDrawingCallback> cb =
    new PaintFrameCallback(aFrame, aPaintServerSize, aRenderSize, aFlags);
  nsRefPtr<gfxDrawable> drawable = new gfxCallbackDrawable(cb, aRenderSize);
  return drawable.forget();
}
