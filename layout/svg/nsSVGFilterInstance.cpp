





#include "nsSVGFilterInstance.h"


#include "gfxPlatform.h"
#include "gfxUtils.h"
#include "nsISVGChildFrame.h"
#include "nsRenderingContext.h"
#include "mozilla/dom/SVGFilterElement.h"
#include "nsSVGFilterPaintCallback.h"
#include "nsSVGUtils.h"
#include "SVGContentUtils.h"
#include "FilterSupport.h"
#include "gfx2DGlue.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::gfx;

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

  for (uint32_t i = 0; i < primitives.Length(); ++i) {
    nsSVGFE* filter = primitives[i];

    nsAutoTArray<int32_t,2> sourceIndices;
    nsresult rv = GetSourceIndices(filter, i, imageTable, sourceIndices);
    if (NS_FAILED(rv)) {
      return rv;
    }

    IntRect primitiveSubregion =
      ComputeFilterPrimitiveSubregion(filter, sourceIndices);

    FilterPrimitiveDescription descr =
      filter->GetPrimitiveDescription(this, primitiveSubregion, mInputImages);

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
      neededRect.Size(), GFX_CONTENT_COLOR_ALPHA);
    if (!offscreenSurface || offscreenSurface->CairoStatus()) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    ctx = new gfxContext(offscreenSurface);
  } else {
    offscreenDT = gfxPlatform::GetPlatform()->CreateOffscreenContentDrawTarget(
      ToIntSize(neededRect.Size()), FORMAT_B8G8R8A8);
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
      neededRect.Size(), GFX_CONTENT_COLOR_ALPHA);
    if (!offscreenSurface || offscreenSurface->CairoStatus()) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    ctx = new gfxContext(offscreenSurface);
  } else {
    offscreenDT = gfxPlatform::GetPlatform()->CreateOffscreenContentDrawTarget(
      ToIntSize(neededRect.Size()), FORMAT_B8G8R8A8);
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

  if (filterRect.IsEmpty()) {
    return NS_OK;
  }

  nsRefPtr<gfxASurface> resultImage;
  RefPtr<DrawTarget> resultImageDT;
  if (aContext->IsCairo()) {
    resultImage =
      gfxPlatform::GetPlatform()->CreateOffscreenSurface(filterRect.Size(),
                                                         GFX_CONTENT_COLOR_ALPHA);
    if (!resultImage || resultImage->CairoStatus())
      return NS_ERROR_OUT_OF_MEMORY;

    
    resultImageDT =
      gfxPlatform::GetPlatform()->CreateDrawTargetForSurface(
        resultImage, ToIntSize(filterRect.Size()));
  } else {
    resultImageDT = gfxPlatform::GetPlatform()->CreateOffscreenContentDrawTarget(
      ToIntSize(filterRect.Size()), FORMAT_B8G8R8A8);
  }

  ComputeNeededBoxes();

  rv = BuildSourceImage(resultImage, resultImageDT);
  if (NS_FAILED(rv))
    return rv;
  rv = BuildSourcePaints(resultImage, resultImageDT);
  if (NS_FAILED(rv))
    return rv;

  IntRect filterSpaceBounds = ToIntRect(mFilterSpaceBounds);
  FilterDescription filter(mPrimitiveDescriptions, filterSpaceBounds);

  FilterSupport::RenderFilterDescription(
    resultImageDT, filter, ToRect(filterRect),
    mSourceGraphic.mSourceSurface, mSourceGraphic.mSurfaceRect,
    mFillPaint.mSourceSurface, mFillPaint.mSurfaceRect,
    mStrokePaint.mSourceSurface, mStrokePaint.mSurfaceRect,
    mInputImages);

  RefPtr<SourceSurface> resultImageSource;
  if (!resultImage) {
    resultImageSource = resultImageDT->Snapshot();
  }

  gfxMatrix ctm = GetFilterSpaceToDeviceSpaceTransform();
  nsSVGUtils::CompositeSurfaceMatrix(aContext, resultImage, resultImageSource,
                                     filterRect.TopLeft(), ctm);

  return NS_OK;
}

nsresult
nsSVGFilterInstance::ComputePostFilterDirtyRect(nsIntRect* aPostFilterDirtyRect)
{
  *aPostFilterDirtyRect = nsIntRect();
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
  *aPostFilterDirtyRect = resultChangeRegion.GetBounds();
  return NS_OK;
}

nsresult
nsSVGFilterInstance::ComputePostFilterExtents(nsIntRect* aPostFilterExtents)
{
  *aPostFilterExtents = nsIntRect();

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
  *aPostFilterExtents = postFilterExtents.GetBounds();
  return NS_OK;
}

nsresult
nsSVGFilterInstance::ComputeSourceNeededRect(nsIntRect* aDirty)
{
  nsresult rv = BuildPrimitives();
  if (NS_FAILED(rv))
    return rv;

  if (mPrimitiveDescriptions.IsEmpty()) {
    
    return NS_OK;
  }

  ComputeNeededBoxes();
  *aDirty = mSourceGraphic.mNeededBounds;

  return NS_OK;
}
