





#include "nsFilterInstance.h"


#include "gfxPlatform.h"
#include "gfxUtils.h"
#include "nsISVGChildFrame.h"
#include "nsRenderingContext.h"
#include "nsSVGFilterInstance.h"
#include "nsSVGFilterPaintCallback.h"
#include "nsSVGUtils.h"
#include "SVGContentUtils.h"
#include "FilterSupport.h"
#include "gfx2DGlue.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::gfx;

nsresult
nsFilterInstance::PaintFilteredFrame(nsRenderingContext *aContext,
                                     nsIFrame *aFilteredFrame,
                                     nsSVGFilterPaintCallback *aPaintCallback,
                                     const nsRect *aDirtyArea,
                                     nsIFrame* aTransformRoot)
{
  nsFilterInstance instance(aFilteredFrame, aPaintCallback, aDirtyArea,
                            nullptr, nullptr, nullptr,
                            aTransformRoot);
  if (!instance.IsInitialized()) {
    return NS_OK;
  }
  return instance.Render(aContext->ThebesContext());
}

nsRect
nsFilterInstance::GetPostFilterDirtyArea(nsIFrame *aFilteredFrame,
                                         const nsRect& aPreFilterDirtyRect)
{
  if (aPreFilterDirtyRect.IsEmpty()) {
    return nsRect();
  }

  nsFilterInstance instance(aFilteredFrame, nullptr, nullptr,
                            &aPreFilterDirtyRect);
  if (!instance.IsInitialized()) {
    return nsRect();
  }
  
  
  
  nsRect dirtyRect;
  nsresult rv = instance.ComputePostFilterDirtyRect(&dirtyRect);
  if (NS_SUCCEEDED(rv)) {
    return dirtyRect;
  }
  return nsRect();
}

nsRect
nsFilterInstance::GetPreFilterNeededArea(nsIFrame *aFilteredFrame,
                                         const nsRect& aPostFilterDirtyRect)
{
  nsFilterInstance instance(aFilteredFrame, nullptr, &aPostFilterDirtyRect);
  if (!instance.IsInitialized()) {
    return nsRect();
  }
  
  
  nsRect neededRect;
  nsresult rv = instance.ComputeSourceNeededRect(&neededRect);
  if (NS_SUCCEEDED(rv)) {
    return neededRect;
  }
  return nsRect();
}

nsRect
nsFilterInstance::GetPostFilterBounds(nsIFrame *aFilteredFrame,
                                      const gfxRect *aOverrideBBox,
                                      const nsRect *aPreFilterBounds)
{
  MOZ_ASSERT(!(aFilteredFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT) ||
             !(aFilteredFrame->GetStateBits() & NS_FRAME_IS_NONDISPLAY),
             "Non-display SVG do not maintain visual overflow rects");

  nsFilterInstance instance(aFilteredFrame, nullptr, nullptr,
                            aPreFilterBounds, aPreFilterBounds,
                            aOverrideBBox);
  if (!instance.IsInitialized()) {
    return nsRect();
  }
  nsRect bbox;
  nsresult rv = instance.ComputePostFilterExtents(&bbox);
  if (NS_SUCCEEDED(rv)) {
    return bbox;
  }
  return nsRect();
}

nsFilterInstance::nsFilterInstance(nsIFrame *aTargetFrame,
                                   nsSVGFilterPaintCallback *aPaintCallback,
                                   const nsRect *aPostFilterDirtyRect,
                                   const nsRect *aPreFilterDirtyRect,
                                   const nsRect *aPreFilterVisualOverflowRectOverride,
                                   const gfxRect *aOverrideBBox,
                                   nsIFrame* aTransformRoot) :
  mTargetFrame(aTargetFrame),
  mPaintCallback(aPaintCallback),
  mTransformRoot(aTransformRoot),
  mInitialized(false) {

  mTargetBBox = aOverrideBBox ?
    *aOverrideBBox : nsSVGUtils::GetBBox(mTargetFrame);

  nsresult rv = BuildPrimitives();
  if (NS_FAILED(rv)) {
    return;
  }

  if (mPrimitiveDescriptions.IsEmpty()) {
    
    return;
  }

  

  gfxMatrix filterToUserSpace(mFilterRegion.Width() / mFilterSpaceBounds.width, 0.0f,
                              0.0f, mFilterRegion.Height() / mFilterSpaceBounds.height,
                              mFilterRegion.X(), mFilterRegion.Y());

  
  if (mPaintCallback) {
    mFilterSpaceToDeviceSpaceTransform = filterToUserSpace *
              nsSVGUtils::GetCanvasTM(mTargetFrame, nsISVGChildFrame::FOR_PAINTING);
  }

  

  mAppUnitsPerCSSPx = mTargetFrame->PresContext()->AppUnitsPerCSSPixel();

  mFilterSpaceToFrameSpaceInCSSPxTransform =
    filterToUserSpace * GetUserSpaceToFrameSpaceInCSSPxTransform();
  
  mFrameSpaceInCSSPxToFilterSpaceTransform =
    mFilterSpaceToFrameSpaceInCSSPxTransform;
  mFrameSpaceInCSSPxToFilterSpaceTransform.Invert();

  mPostFilterDirtyRect = FrameSpaceToFilterSpace(aPostFilterDirtyRect);
  mPreFilterDirtyRect = FrameSpaceToFilterSpace(aPreFilterDirtyRect);
  if (aPreFilterVisualOverflowRectOverride) {
    mTargetBounds = 
      FrameSpaceToFilterSpace(aPreFilterVisualOverflowRectOverride);
  } else {
    nsRect preFilterVOR = mTargetFrame->GetPreEffectsVisualOverflowRect();
    mTargetBounds = FrameSpaceToFilterSpace(&preFilterVOR);
  }

  mInitialized = true;
}

gfxRect
nsFilterInstance::UserSpaceToFilterSpace(const gfxRect& aRect) const
{
  gfxRect r = aRect - mFilterRegion.TopLeft();
  r.Scale(mFilterSpaceBounds.width / mFilterRegion.Width(),
          mFilterSpaceBounds.height / mFilterRegion.Height());
  return r;
}

gfxMatrix
nsFilterInstance::GetUserSpaceToFilterSpaceTransform() const
{
  gfxFloat widthScale = mFilterSpaceBounds.width / mFilterRegion.Width();
  gfxFloat heightScale = mFilterSpaceBounds.height / mFilterRegion.Height();
  return gfxMatrix(widthScale, 0.0f,
                   0.0f, heightScale,
                   -mFilterRegion.X() * widthScale, -mFilterRegion.Y() * heightScale);
}

nsresult
nsFilterInstance::BuildPrimitives()
{
  NS_ASSERTION(!mPrimitiveDescriptions.Length(),
               "expected to start building primitives from scratch");

  const nsTArray<nsStyleFilter>& filters = mTargetFrame->StyleSVGReset()->mFilters;
  for (uint32_t i = 0; i < filters.Length(); i++) {
    nsresult rv = BuildPrimitivesForFilter(filters[i]);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }
  return NS_OK;
}

nsresult
nsFilterInstance::BuildPrimitivesForFilter(const nsStyleFilter& aFilter)
{
  if (aFilter.GetType() == NS_STYLE_FILTER_URL) {
    
    nsSVGFilterInstance svgFilterInstance(aFilter, mTargetFrame, mTargetBBox);
    if (!svgFilterInstance.IsInitialized()) {
      return NS_ERROR_FAILURE;
    }

    
    
    
    mFilterRegion = svgFilterInstance.GetFilterRegion();
    mFilterSpaceBounds = svgFilterInstance.GetFilterSpaceBounds();

    
    bool overflow;
    gfxIntSize surfaceSize =
      nsSVGUtils::ConvertToSurfaceSize(mFilterSpaceBounds.Size(), &overflow);
    mFilterSpaceBounds.SizeTo(surfaceSize);

    return svgFilterInstance.BuildPrimitives(mPrimitiveDescriptions, mInputImages);
  }

  
  return NS_ERROR_FAILURE;
}

void
nsFilterInstance::ComputeNeededBoxes()
{
  if (mPrimitiveDescriptions.IsEmpty())
    return;

  nsIntRegion sourceGraphicNeededRegion;
  nsIntRegion fillPaintNeededRegion;
  nsIntRegion strokePaintNeededRegion;

  FilterDescription filter(mPrimitiveDescriptions, ToIntRect(mFilterSpaceBounds));
  FilterSupport::ComputeSourceNeededRegions(
    filter, mPostFilterDirtyRect,
    sourceGraphicNeededRegion, fillPaintNeededRegion, strokePaintNeededRegion);

  nsIntRect sourceBoundsInt;
  gfxRect sourceBounds = UserSpaceToFilterSpace(mTargetBBox);
  sourceBounds.RoundOut();
  
  if (!gfxUtils::GfxRectToIntRect(sourceBounds, &sourceBoundsInt))
    return;
  sourceBoundsInt.UnionRect(sourceBoundsInt, mTargetBounds);

  sourceGraphicNeededRegion.And(sourceGraphicNeededRegion, sourceBoundsInt);

  mSourceGraphic.mNeededBounds = sourceGraphicNeededRegion.GetBounds();
  mFillPaint.mNeededBounds = fillPaintNeededRegion.GetBounds();
  mStrokePaint.mNeededBounds = strokePaintNeededRegion.GetBounds();
}

nsresult
nsFilterInstance::BuildSourcePaint(SourceInfo *aSource,
                                   gfxASurface* aTargetSurface,
                                   DrawTarget* aTargetDT)
{
  nsIntRect neededRect = aSource->mNeededBounds;

  RefPtr<DrawTarget> offscreenDT;
  nsRefPtr<gfxASurface> offscreenSurface;
  nsRefPtr<gfxContext> ctx;
  if (aTargetSurface) {
    offscreenSurface = gfxPlatform::GetPlatform()->CreateOffscreenSurface(
      neededRect.Size().ToIntSize(), gfxContentType::COLOR_ALPHA);
    if (!offscreenSurface || offscreenSurface->CairoStatus()) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    ctx = new gfxContext(offscreenSurface);
  } else {
    offscreenDT = gfxPlatform::GetPlatform()->CreateOffscreenContentDrawTarget(
      ToIntSize(neededRect.Size()), SurfaceFormat::B8G8R8A8);
    if (!offscreenDT) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    ctx = new gfxContext(offscreenDT);
  }

  ctx->Translate(-neededRect.TopLeft());

  nsRenderingContext tmpCtx;
  tmpCtx.Init(mTargetFrame->PresContext()->DeviceContext(), ctx);

  gfxMatrix m = GetUserSpaceToFilterSpaceTransform();
  m.Invert();
  gfxRect r = m.TransformBounds(mFilterSpaceBounds);

  gfxMatrix deviceToFilterSpace = GetFilterSpaceToDeviceSpaceTransform().Invert();
  gfxContext *gfx = tmpCtx.ThebesContext();
  gfx->Multiply(deviceToFilterSpace);

  gfx->Save();

  gfxMatrix matrix =
    nsSVGUtils::GetCanvasTM(mTargetFrame, nsISVGChildFrame::FOR_PAINTING,
                            mTransformRoot);
  if (!matrix.IsSingular()) {
    gfx->Multiply(matrix);
    gfx->Rectangle(r);
    if ((aSource == &mFillPaint && 
         nsSVGUtils::SetupCairoFillPaint(mTargetFrame, gfx)) ||
        (aSource == &mStrokePaint &&
         nsSVGUtils::SetupCairoStrokePaint(mTargetFrame, gfx))) {
      gfx->Fill();
    }
  }
  gfx->Restore();

  if (offscreenSurface) {
    aSource->mSourceSurface =
      gfxPlatform::GetPlatform()->GetSourceSurfaceForSurface(aTargetDT, offscreenSurface);
  } else {
    aSource->mSourceSurface = offscreenDT->Snapshot();
  }
  aSource->mSurfaceRect = ToIntRect(neededRect);

  return NS_OK;
}

nsresult
nsFilterInstance::BuildSourcePaints(gfxASurface* aTargetSurface,
                                    DrawTarget* aTargetDT)
{
  nsresult rv = NS_OK;

  if (!mFillPaint.mNeededBounds.IsEmpty()) {
    rv = BuildSourcePaint(&mFillPaint, aTargetSurface, aTargetDT);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!mStrokePaint.mNeededBounds.IsEmpty()) {
    rv = BuildSourcePaint(&mStrokePaint, aTargetSurface, aTargetDT);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return  rv;
}

nsresult
nsFilterInstance::BuildSourceImage(gfxASurface* aTargetSurface,
                                   DrawTarget* aTargetDT)
{
  nsIntRect neededRect = mSourceGraphic.mNeededBounds;
  if (neededRect.IsEmpty()) {
    return NS_OK;
  }

  RefPtr<DrawTarget> offscreenDT;
  nsRefPtr<gfxASurface> offscreenSurface;
  nsRefPtr<gfxContext> ctx;
  if (aTargetSurface) {
    offscreenSurface = gfxPlatform::GetPlatform()->CreateOffscreenSurface(
      neededRect.Size().ToIntSize(), gfxContentType::COLOR_ALPHA);
    if (!offscreenSurface || offscreenSurface->CairoStatus()) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    ctx = new gfxContext(offscreenSurface);
  } else {
    offscreenDT = gfxPlatform::GetPlatform()->CreateOffscreenContentDrawTarget(
      ToIntSize(neededRect.Size()), SurfaceFormat::B8G8R8A8);
    if (!offscreenDT) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    ctx = new gfxContext(offscreenDT);
  }

  ctx->Translate(-neededRect.TopLeft());

  nsRenderingContext tmpCtx;
  tmpCtx.Init(mTargetFrame->PresContext()->DeviceContext(), ctx);

  gfxMatrix m = GetUserSpaceToFilterSpaceTransform();
  m.Invert();
  gfxRect r = m.TransformBounds(neededRect);
  r.RoundOut();
  nsIntRect dirty;
  if (!gfxUtils::GfxRectToIntRect(r, &dirty))
    return NS_ERROR_FAILURE;

  
  
  
  
  
  
  
  
  
  
  
  gfxMatrix deviceToFilterSpace = GetFilterSpaceToDeviceSpaceTransform().Invert();
  tmpCtx.ThebesContext()->Multiply(deviceToFilterSpace);
  mPaintCallback->Paint(&tmpCtx, mTargetFrame, &dirty, mTransformRoot);

  RefPtr<SourceSurface> sourceGraphicSource;

  if (offscreenSurface) {
    sourceGraphicSource =
      gfxPlatform::GetPlatform()->GetSourceSurfaceForSurface(aTargetDT, offscreenSurface);
  } else {
    sourceGraphicSource = offscreenDT->Snapshot();
  }

  mSourceGraphic.mSourceSurface = sourceGraphicSource;
  mSourceGraphic.mSurfaceRect = ToIntRect(neededRect);
   
  return NS_OK;
}

nsresult
nsFilterInstance::Render(gfxContext* aContext)
{
  nsIntRect filterRect = mPostFilterDirtyRect.Intersect(mFilterSpaceBounds);
  gfxMatrix ctm = GetFilterSpaceToDeviceSpaceTransform();

  if (filterRect.IsEmpty() || ctm.IsSingular()) {
    return NS_OK;
  }

  Matrix oldDTMatrix;
  nsRefPtr<gfxASurface> resultImage;
  RefPtr<DrawTarget> dt;
  if (aContext->IsCairo()) {
    resultImage =
      gfxPlatform::GetPlatform()->CreateOffscreenSurface(filterRect.Size().ToIntSize(),
                                                         gfxContentType::COLOR_ALPHA);
    if (!resultImage || resultImage->CairoStatus())
      return NS_ERROR_OUT_OF_MEMORY;

    
    dt = gfxPlatform::GetPlatform()->CreateDrawTargetForSurface(
           resultImage, ToIntSize(filterRect.Size()));
  } else {
    
    
    dt = aContext->GetDrawTarget();
    oldDTMatrix = dt->GetTransform();
    Matrix matrix = ToMatrix(ctm);
    matrix.Translate(filterRect.x, filterRect.y);
    dt->SetTransform(matrix * oldDTMatrix);
  }

  ComputeNeededBoxes();

  nsresult rv = BuildSourceImage(resultImage, dt);
  if (NS_FAILED(rv))
    return rv;
  rv = BuildSourcePaints(resultImage, dt);
  if (NS_FAILED(rv))
    return rv;

  IntRect filterSpaceBounds = ToIntRect(mFilterSpaceBounds);
  FilterDescription filter(mPrimitiveDescriptions, filterSpaceBounds);

  FilterSupport::RenderFilterDescription(
    dt, filter, ToRect(filterRect),
    mSourceGraphic.mSourceSurface, mSourceGraphic.mSurfaceRect,
    mFillPaint.mSourceSurface, mFillPaint.mSurfaceRect,
    mStrokePaint.mSourceSurface, mStrokePaint.mSurfaceRect,
    mInputImages);

  if (resultImage) {
    aContext->Save();
    aContext->Multiply(ctm);
    aContext->Translate(filterRect.TopLeft());
    aContext->SetSource(resultImage);
    aContext->Paint();
    aContext->Restore();
  } else {
    dt->SetTransform(oldDTMatrix);
  }

  return NS_OK;
}

nsresult
nsFilterInstance::ComputePostFilterDirtyRect(nsRect* aPostFilterDirtyRect)
{
  *aPostFilterDirtyRect = nsRect();
  if (mPreFilterDirtyRect.IsEmpty()) {
    return NS_OK;
  }

  IntRect filterSpaceBounds = ToIntRect(mFilterSpaceBounds);
  FilterDescription filter(mPrimitiveDescriptions, filterSpaceBounds);
  nsIntRegion resultChangeRegion =
    FilterSupport::ComputeResultChangeRegion(filter,
      mPreFilterDirtyRect, nsIntRegion(), nsIntRegion());
  *aPostFilterDirtyRect =
    FilterSpaceToFrameSpace(resultChangeRegion.GetBounds());
  return NS_OK;
}

nsresult
nsFilterInstance::ComputePostFilterExtents(nsRect* aPostFilterExtents)
{
  *aPostFilterExtents = nsRect();

  nsIntRect sourceBoundsInt;
  gfxRect sourceBounds = UserSpaceToFilterSpace(mTargetBBox);
  sourceBounds.RoundOut();
  
  if (!gfxUtils::GfxRectToIntRect(sourceBounds, &sourceBoundsInt))
    return NS_ERROR_FAILURE;
  sourceBoundsInt.UnionRect(sourceBoundsInt, mTargetBounds);

  IntRect filterSpaceBounds = ToIntRect(mFilterSpaceBounds);
  FilterDescription filter(mPrimitiveDescriptions, filterSpaceBounds);
  nsIntRegion postFilterExtents =
    FilterSupport::ComputePostFilterExtents(filter, sourceBoundsInt);
  *aPostFilterExtents = FilterSpaceToFrameSpace(postFilterExtents.GetBounds());
  return NS_OK;
}

nsresult
nsFilterInstance::ComputeSourceNeededRect(nsRect* aDirty)
{
  ComputeNeededBoxes();
  *aDirty = FilterSpaceToFrameSpace(mSourceGraphic.mNeededBounds);

  return NS_OK;
}

nsIntRect
nsFilterInstance::FrameSpaceToFilterSpace(const nsRect* aRect) const
{
  nsIntRect rect = mFilterSpaceBounds;
  if (aRect) {
    if (aRect->IsEmpty()) {
      return nsIntRect();
    }
    gfxRect rectInCSSPx =
      nsLayoutUtils::RectToGfxRect(*aRect, mAppUnitsPerCSSPx);
    gfxRect rectInFilterSpace =
      mFrameSpaceInCSSPxToFilterSpaceTransform.TransformBounds(rectInCSSPx);
    rectInFilterSpace.RoundOut();
    nsIntRect intRect;
    if (gfxUtils::GfxRectToIntRect(rectInFilterSpace, &intRect)) {
      rect = intRect;
    }
  }
  return rect;
}

nsRect
nsFilterInstance::FilterSpaceToFrameSpace(const nsIntRect& aRect) const
{
  if (aRect.IsEmpty()) {
    return nsRect();
  }
  gfxRect r(aRect.x, aRect.y, aRect.width, aRect.height);
  r = mFilterSpaceToFrameSpaceInCSSPxTransform.TransformBounds(r);
  return nsLayoutUtils::RoundGfxRectToAppRect(r, mAppUnitsPerCSSPx);
}

gfxMatrix
nsFilterInstance::GetUserSpaceToFrameSpaceInCSSPxTransform() const
{
  gfxMatrix userToFrameSpaceInCSSPx;

  if ((mTargetFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT)) {
    
    
    
    
    
    
    
    
    
    
    if (mTargetFrame->GetType() == nsGkAtoms::svgInnerSVGFrame) {
      userToFrameSpaceInCSSPx =
        static_cast<nsSVGElement*>(mTargetFrame->GetContent())->
          PrependLocalTransformsTo(gfxMatrix());
    } else {
      gfxPoint targetsUserSpaceOffset =
        nsLayoutUtils::RectToGfxRect(mTargetFrame->GetRect(),
                                     mAppUnitsPerCSSPx).TopLeft();
      userToFrameSpaceInCSSPx.Translate(-targetsUserSpaceOffset);
    }
  }
  
  return userToFrameSpaceInCSSPx;
}
