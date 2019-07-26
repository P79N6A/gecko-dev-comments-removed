





#include "nsSVGFilterInstance.h"


#include "gfxPlatform.h"
#include "gfxUtils.h"
#include "nsISVGChildFrame.h"
#include "nsRenderingContext.h"
#include "mozilla/dom/SVGFilterElement.h"
#include "nsSVGFilterFrame.h"
#include "nsSVGFilterPaintCallback.h"
#include "nsSVGUtils.h"
#include "SVGContentUtils.h"
#include "FilterSupport.h"
#include "gfx2DGlue.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::gfx;

nsresult
nsSVGFilterInstance::PaintFilteredFrame(nsSVGFilterFrame* aFilterFrame,
                                        nsRenderingContext *aContext,
                                        nsIFrame *aFilteredFrame,
                                        nsSVGFilterPaintCallback *aPaintCallback,
                                        const nsRect *aDirtyArea,
                                        nsIFrame* aTransformRoot)
{
  nsSVGFilterInstance instance(aFilteredFrame, aFilterFrame, aPaintCallback,
                               aDirtyArea, nullptr, nullptr, nullptr,
                               aTransformRoot);
  if (!instance.IsInitialized()) {
    return NS_OK;
  }
  return instance.Render(aContext->ThebesContext());
}

static nsRect
TransformFilterSpaceToFrameSpace(nsSVGFilterInstance *aInstance,
                                 nsIntRect *aRect)
{
  if (aRect->IsEmpty()) {
    return nsRect();
  }
  gfxMatrix m = aInstance->GetFilterSpaceToFrameSpaceInCSSPxTransform();
  gfxRect r(aRect->x, aRect->y, aRect->width, aRect->height);
  r = m.TransformBounds(r);
  return nsLayoutUtils::RoundGfxRectToAppRect(r, aInstance->AppUnitsPerCSSPixel());
}

nsRect
nsSVGFilterInstance::GetPostFilterDirtyArea(nsSVGFilterFrame* aFilterFrame,
                                            nsIFrame *aFilteredFrame,
                                            const nsRect& aPreFilterDirtyRect)
{
  if (aPreFilterDirtyRect.IsEmpty()) {
    return nsRect();
  }

  nsSVGFilterInstance instance(aFilteredFrame, aFilterFrame, nullptr, nullptr,
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
nsSVGFilterInstance::GetPreFilterNeededArea(nsSVGFilterFrame* aFilterFrame,
                                            nsIFrame *aFilteredFrame,
                                            const nsRect& aPostFilterDirtyRect)
{
  nsSVGFilterInstance instance(aFilteredFrame, aFilterFrame, nullptr,
                               &aPostFilterDirtyRect);
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
nsSVGFilterInstance::GetPostFilterBounds(nsSVGFilterFrame* aFilterFrame,
                                         nsIFrame *aFilteredFrame,
                                         const gfxRect *aOverrideBBox,
                                         const nsRect *aPreFilterBounds)
{
  MOZ_ASSERT(!(aFilteredFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT) ||
             !(aFilteredFrame->GetStateBits() & NS_FRAME_IS_NONDISPLAY),
             "Non-display SVG do not maintain visual overflow rects");

  nsSVGFilterInstance instance(aFilteredFrame, aFilterFrame, nullptr, nullptr,
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

nsSVGFilterInstance::nsSVGFilterInstance(nsIFrame *aTargetFrame,
                                         nsSVGFilterFrame *aFilterFrame,
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

  mFilterElement =  aFilterFrame->GetFilterContent();

  mPrimitiveUnits =
    aFilterFrame->GetEnumValue(SVGFilterElement::PRIMITIVEUNITS);

  mTargetBBox = aOverrideBBox ?
    *aOverrideBBox : nsSVGUtils::GetBBox(mTargetFrame);

  

  
  
  
  
  
  
  
  
  
  
  nsSVGLength2 XYWH[4];
  NS_ABORT_IF_FALSE(sizeof(mFilterElement->mLengthAttributes) == sizeof(XYWH),
                    "XYWH size incorrect");
  memcpy(XYWH, mFilterElement->mLengthAttributes, 
    sizeof(mFilterElement->mLengthAttributes));
  XYWH[0] = *aFilterFrame->GetLengthValue(SVGFilterElement::ATTR_X);
  XYWH[1] = *aFilterFrame->GetLengthValue(SVGFilterElement::ATTR_Y);
  XYWH[2] = *aFilterFrame->GetLengthValue(SVGFilterElement::ATTR_WIDTH);
  XYWH[3] = *aFilterFrame->GetLengthValue(SVGFilterElement::ATTR_HEIGHT);
  uint16_t filterUnits =
    aFilterFrame->GetEnumValue(SVGFilterElement::FILTERUNITS);
  
  mFilterRegion = nsSVGUtils::GetRelativeRect(filterUnits,
    XYWH, mTargetBBox, mTargetFrame);

  if (mFilterRegion.Width() <= 0 || mFilterRegion.Height() <= 0) {
    
    
    return;
  }

  
  
  
  
  

  gfxIntSize filterRes;
  const nsSVGIntegerPair* filterResAttrs =
    aFilterFrame->GetIntegerPairValue(SVGFilterElement::FILTERRES);
  if (filterResAttrs->IsExplicitlySet()) {
    int32_t filterResX = filterResAttrs->GetAnimValue(nsSVGIntegerPair::eFirst);
    int32_t filterResY = filterResAttrs->GetAnimValue(nsSVGIntegerPair::eSecond);
    if (filterResX <= 0 || filterResY <= 0) {
      
      return;
    }

    mFilterRegion.Scale(filterResX, filterResY);
    mFilterRegion.RoundOut();
    mFilterRegion.Scale(1.0 / filterResX, 1.0 / filterResY);
    
    
    bool overflow;
    filterRes =
      nsSVGUtils::ConvertToSurfaceSize(gfxSize(filterResX, filterResY),
                                       &overflow);
    
    
  } else {
    
    
    gfxMatrix canvasTM =
      nsSVGUtils::GetCanvasTM(mTargetFrame, nsISVGChildFrame::FOR_OUTERSVG_TM);
    if (canvasTM.IsSingular()) {
      
      return;
    }

    gfxSize scale = canvasTM.ScaleFactors(true);
    mFilterRegion.Scale(scale.width, scale.height);
    mFilterRegion.RoundOut();
    
    
    bool overflow;
    filterRes = nsSVGUtils::ConvertToSurfaceSize(mFilterRegion.Size(),
                                                 &overflow);
    mFilterRegion.Scale(1.0 / scale.width, 1.0 / scale.height);
  }

  mFilterSpaceBounds.SetRect(nsIntPoint(0, 0), filterRes);

  

  gfxMatrix filterToUserSpace(mFilterRegion.Width() / filterRes.width, 0.0f,
                              0.0f, mFilterRegion.Height() / filterRes.height,
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

float
nsSVGFilterInstance::GetPrimitiveNumber(uint8_t aCtxType, float aValue) const
{
  nsSVGLength2 val;
  val.Init(aCtxType, 0xff, aValue,
           nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER);

  float value;
  if (mPrimitiveUnits == SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
    value = nsSVGUtils::ObjectSpace(mTargetBBox, &val);
  } else {
    value = nsSVGUtils::UserSpace(mTargetFrame, &val);
  }

  switch (aCtxType) {
  case SVGContentUtils::X:
    return value * mFilterSpaceBounds.width / mFilterRegion.Width();
  case SVGContentUtils::Y:
    return value * mFilterSpaceBounds.height / mFilterRegion.Height();
  case SVGContentUtils::XY:
  default:
    return value * SVGContentUtils::ComputeNormalizedHypotenuse(
                     mFilterSpaceBounds.width / mFilterRegion.Width(),
                     mFilterSpaceBounds.height / mFilterRegion.Height());
  }
}

Point3D
nsSVGFilterInstance::ConvertLocation(const Point3D& aPoint) const
{
  nsSVGLength2 val[4];
  val[0].Init(SVGContentUtils::X, 0xff, aPoint.x,
              nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER);
  val[1].Init(SVGContentUtils::Y, 0xff, aPoint.y,
              nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER);
  
  val[2].Init(SVGContentUtils::X, 0xff, 0,
              nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER);
  val[3].Init(SVGContentUtils::Y, 0xff, 0,
              nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER);

  gfxRect feArea = nsSVGUtils::GetRelativeRect(mPrimitiveUnits,
    val, mTargetBBox, mTargetFrame);
  gfxRect r = UserSpaceToFilterSpace(feArea);
  return Point3D(r.x, r.y, GetPrimitiveNumber(SVGContentUtils::XY, aPoint.z));
}

gfxRect
nsSVGFilterInstance::UserSpaceToFilterSpace(const gfxRect& aRect) const
{
  gfxRect r = aRect - mFilterRegion.TopLeft();
  r.Scale(mFilterSpaceBounds.width / mFilterRegion.Width(),
          mFilterSpaceBounds.height / mFilterRegion.Height());
  return r;
}

gfxPoint
nsSVGFilterInstance::FilterSpaceToUserSpace(const gfxPoint& aPt) const
{
  return gfxPoint(aPt.x * mFilterRegion.Width() / mFilterSpaceBounds.width + mFilterRegion.X(),
                  aPt.y * mFilterRegion.Height() / mFilterSpaceBounds.height + mFilterRegion.Y());
}

gfxMatrix
nsSVGFilterInstance::GetUserSpaceToFilterSpaceTransform() const
{
  gfxFloat widthScale = mFilterSpaceBounds.width / mFilterRegion.Width();
  gfxFloat heightScale = mFilterSpaceBounds.height / mFilterRegion.Height();
  return gfxMatrix(widthScale, 0.0f,
                   0.0f, heightScale,
                   -mFilterRegion.X() * widthScale, -mFilterRegion.Y() * heightScale);
}

IntRect
nsSVGFilterInstance::ComputeFilterPrimitiveSubregion(nsSVGFE* aFilterElement,
                                                     const nsTArray<int32_t>& aInputIndices)
{
  nsSVGFE* fE = aFilterElement;

  IntRect defaultFilterSubregion(0,0,0,0);
  if (fE->SubregionIsUnionOfRegions()) {
    for (uint32_t i = 0; i < aInputIndices.Length(); ++i) {
      int32_t inputIndex = aInputIndices[i];
      IntRect inputSubregion = inputIndex >= 0 ?
        mPrimitiveDescriptions[inputIndex].PrimitiveSubregion() :
        ToIntRect(mFilterSpaceBounds);

      defaultFilterSubregion = defaultFilterSubregion.Union(inputSubregion);
    }
  } else {
    defaultFilterSubregion = ToIntRect(mFilterSpaceBounds);
  }

  gfxRect feArea = nsSVGUtils::GetRelativeRect(mPrimitiveUnits,
    &fE->mLengthAttributes[nsSVGFE::ATTR_X], mTargetBBox, mTargetFrame);
  Rect region = ToRect(UserSpaceToFilterSpace(feArea));

  if (!fE->mLengthAttributes[nsSVGFE::ATTR_X].IsExplicitlySet())
    region.x = defaultFilterSubregion.X();
  if (!fE->mLengthAttributes[nsSVGFE::ATTR_Y].IsExplicitlySet())
    region.y = defaultFilterSubregion.Y();
  if (!fE->mLengthAttributes[nsSVGFE::ATTR_WIDTH].IsExplicitlySet())
    region.width = defaultFilterSubregion.Width();
  if (!fE->mLengthAttributes[nsSVGFE::ATTR_HEIGHT].IsExplicitlySet())
    region.height = defaultFilterSubregion.Height();

  
  
  
  region.RoundOut();

  return RoundedToInt(region);
}

void
nsSVGFilterInstance::GetInputsAreTainted(const nsTArray<int32_t>& aInputIndices,
                                         nsTArray<bool>& aOutInputsAreTainted)
{
  for (uint32_t i = 0; i < aInputIndices.Length(); i++) {
    int32_t inputIndex = aInputIndices[i];
    if (inputIndex < 0) {
      
      aOutInputsAreTainted.AppendElement(true);
    } else {
      aOutInputsAreTainted.AppendElement(mPrimitiveDescriptions[inputIndex].IsTainted());
    }
  }
}

static nsresult
GetSourceIndices(nsSVGFE* aFilterElement,
                 int32_t aCurrentIndex,
                 const nsDataHashtable<nsStringHashKey, int32_t>& aImageTable,
                 nsTArray<int32_t>& aSourceIndices)
{
  nsAutoTArray<nsSVGStringInfo,2> sources;
  aFilterElement->GetSourceImageNames(sources);

  for (uint32_t j = 0; j < sources.Length(); j++) {
    nsAutoString str;
    sources[j].mString->GetAnimValue(str, sources[j].mElement);

    int32_t sourceIndex = 0;
    if (str.EqualsLiteral("SourceGraphic")) {
      sourceIndex = FilterPrimitiveDescription::kPrimitiveIndexSourceGraphic;
    } else if (str.EqualsLiteral("SourceAlpha")) {
      sourceIndex = FilterPrimitiveDescription::kPrimitiveIndexSourceAlpha;
    } else if (str.EqualsLiteral("FillPaint")) {
      sourceIndex = FilterPrimitiveDescription::kPrimitiveIndexFillPaint;
    } else if (str.EqualsLiteral("StrokePaint")) {
      sourceIndex = FilterPrimitiveDescription::kPrimitiveIndexStrokePaint;
    } else if (str.EqualsLiteral("BackgroundImage") ||
               str.EqualsLiteral("BackgroundAlpha")) {
      return NS_ERROR_NOT_IMPLEMENTED;
    } else if (str.EqualsLiteral("")) {
      sourceIndex = aCurrentIndex == 0 ?
        FilterPrimitiveDescription::kPrimitiveIndexSourceGraphic :
        aCurrentIndex - 1;
    } else {
      bool inputExists = aImageTable.Get(str, &sourceIndex);
      if (!inputExists)
        return NS_ERROR_FAILURE;
    }

    MOZ_ASSERT(sourceIndex < aCurrentIndex);
    aSourceIndices.AppendElement(sourceIndex);
  }
  return NS_OK;
}

nsresult
nsSVGFilterInstance::BuildPrimitives()
{
  nsTArray<nsRefPtr<nsSVGFE> > primitives;
  for (nsIContent* child = mFilterElement->nsINode::GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    nsRefPtr<nsSVGFE> primitive;
    CallQueryInterface(child, (nsSVGFE**)getter_AddRefs(primitive));
    if (primitive) {
      primitives.AppendElement(primitive);
    }
  }

  
  nsDataHashtable<nsStringHashKey, int32_t> imageTable(10);

  
  nsCOMPtr<nsIPrincipal> principal = mTargetFrame->GetContent()->NodePrincipal();

  for (uint32_t i = 0; i < primitives.Length(); ++i) {
    nsSVGFE* filter = primitives[i];

    nsAutoTArray<int32_t,2> sourceIndices;
    nsresult rv = GetSourceIndices(filter, i, imageTable, sourceIndices);
    if (NS_FAILED(rv)) {
      return rv;
    }

    IntRect primitiveSubregion =
      ComputeFilterPrimitiveSubregion(filter, sourceIndices);

    nsTArray<bool> sourcesAreTainted;
    GetInputsAreTainted(sourceIndices, sourcesAreTainted);

    FilterPrimitiveDescription descr =
      filter->GetPrimitiveDescription(this, primitiveSubregion, sourcesAreTainted, mInputImages);

    descr.SetIsTainted(filter->OutputIsTainted(sourcesAreTainted, principal));
    descr.SetPrimitiveSubregion(primitiveSubregion);

    for (uint32_t j = 0; j < sourceIndices.Length(); j++) {
      int32_t inputIndex = sourceIndices[j];
      descr.SetInputPrimitive(j, inputIndex);
      ColorSpace inputColorSpace =
        inputIndex < 0 ? SRGB : mPrimitiveDescriptions[inputIndex].OutputColorSpace();
      ColorSpace desiredInputColorSpace = filter->GetInputColorSpace(j, inputColorSpace);
      descr.SetInputColorSpace(j, desiredInputColorSpace);
      if (j == 0) {
        
        descr.SetOutputColorSpace(desiredInputColorSpace);
      }
    }

    if (sourceIndices.Length() == 0) {
      descr.SetOutputColorSpace(filter->GetOutputColorSpace());
    }

    mPrimitiveDescriptions.AppendElement(descr);

    nsAutoString str;
    filter->GetResultImageName().GetAnimValue(str, filter);
    imageTable.Put(str, i);
  }

  return NS_OK;
}

void
nsSVGFilterInstance::ComputeNeededBoxes()
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
nsSVGFilterInstance::BuildSourcePaint(SourceInfo *aSource,
                                      gfxASurface* aTargetSurface,
                                      DrawTarget* aTargetDT)
{
  nsIntRect neededRect = aSource->mNeededBounds;

  RefPtr<DrawTarget> offscreenDT;
  nsRefPtr<gfxASurface> offscreenSurface;
  nsRefPtr<gfxContext> ctx;
  if (aTargetSurface) {
    offscreenSurface = gfxPlatform::GetPlatform()->CreateOffscreenSurface(
      neededRect.Size(), gfxContentType::COLOR_ALPHA);
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
nsSVGFilterInstance::BuildSourcePaints(gfxASurface* aTargetSurface,
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
nsSVGFilterInstance::BuildSourceImage(gfxASurface* aTargetSurface,
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
      neededRect.Size(), gfxContentType::COLOR_ALPHA);
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
nsSVGFilterInstance::Render(gfxContext* aContext)
{
  nsresult rv = BuildPrimitives();
  if (NS_FAILED(rv))
    return rv;

  if (mPrimitiveDescriptions.IsEmpty()) {
    
    return NS_OK;
  }

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
      gfxPlatform::GetPlatform()->CreateOffscreenSurface(filterRect.Size(),
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

  rv = BuildSourceImage(resultImage, dt);
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
nsSVGFilterInstance::ComputePostFilterDirtyRect(nsRect* aPostFilterDirtyRect)
{
  *aPostFilterDirtyRect = nsRect();
  if (mPreFilterDirtyRect.IsEmpty()) {
    return NS_OK;
  }

  nsresult rv = BuildPrimitives();
  if (NS_FAILED(rv))
    return rv;

  if (mPrimitiveDescriptions.IsEmpty()) {
    
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
nsSVGFilterInstance::ComputePostFilterExtents(nsRect* aPostFilterExtents)
{
  *aPostFilterExtents = nsRect();

  nsresult rv = BuildPrimitives();
  if (NS_FAILED(rv))
    return rv;

  if (mPrimitiveDescriptions.IsEmpty()) {
    return NS_OK;
  }

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
nsSVGFilterInstance::ComputeSourceNeededRect(nsRect* aDirty)
{
  nsresult rv = BuildPrimitives();
  if (NS_FAILED(rv))
    return rv;

  if (mPrimitiveDescriptions.IsEmpty()) {
    
    return NS_OK;
  }

  ComputeNeededBoxes();
  *aDirty = FilterSpaceToFrameSpace(mSourceGraphic.mNeededBounds);

  return NS_OK;
}

nsIntRect
nsSVGFilterInstance::FrameSpaceToFilterSpace(const nsRect* aRect) const
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
nsSVGFilterInstance::FilterSpaceToFrameSpace(const nsIntRect& aRect) const
{
  if (aRect.IsEmpty()) {
    return nsRect();
  }
  gfxRect r(aRect.x, aRect.y, aRect.width, aRect.height);
  r = mFilterSpaceToFrameSpaceInCSSPxTransform.TransformBounds(r);
  return nsLayoutUtils::RoundGfxRectToAppRect(r, mAppUnitsPerCSSPx);
}

gfxMatrix
nsSVGFilterInstance::GetUserSpaceToFrameSpaceInCSSPxTransform() const
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
