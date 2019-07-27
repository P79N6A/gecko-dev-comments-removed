





#include "nsCSSFilterInstance.h"


#include "gfx2DGlue.h"
#include "gfxUtils.h"
#include "nsIFrame.h"
#include "nsStyleStruct.h"
#include "nsTArray.h"

using namespace mozilla;
using namespace mozilla::gfx;

nsCSSFilterInstance::nsCSSFilterInstance(const nsStyleFilter& aFilter,
                                         nsIFrame *aTargetFrame,
                                         const nsIntRect& aTargetBBoxInFilterSpace,
                                         const gfxMatrix& aFrameSpaceInCSSPxToFilterSpaceTransform)
  : mFilter(aFilter)
  , mTargetFrame(aTargetFrame)
  , mTargetBBoxInFilterSpace(aTargetBBoxInFilterSpace)
  , mFrameSpaceInCSSPxToFilterSpaceTransform(aFrameSpaceInCSSPxToFilterSpaceTransform)
{
}

nsresult
nsCSSFilterInstance::BuildPrimitives(nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs)
{
  FilterPrimitiveDescription descr;
  nsresult result;

  switch(mFilter.GetType()) {
    case NS_STYLE_FILTER_BLUR:
      descr = CreatePrimitiveDescription(PrimitiveType::GaussianBlur, aPrimitiveDescrs);
      result = SetAttributesForBlur(descr);
      break;
    case NS_STYLE_FILTER_BRIGHTNESS:
      return NS_ERROR_NOT_IMPLEMENTED;
    case NS_STYLE_FILTER_CONTRAST:
      return NS_ERROR_NOT_IMPLEMENTED;
    case NS_STYLE_FILTER_DROP_SHADOW:
      descr = CreatePrimitiveDescription(PrimitiveType::DropShadow, aPrimitiveDescrs);
      result = SetAttributesForDropShadow(descr);
      break;
    case NS_STYLE_FILTER_GRAYSCALE:
      return NS_ERROR_NOT_IMPLEMENTED;
    case NS_STYLE_FILTER_HUE_ROTATE:
      return NS_ERROR_NOT_IMPLEMENTED;
    case NS_STYLE_FILTER_INVERT:
      return NS_ERROR_NOT_IMPLEMENTED;
    case NS_STYLE_FILTER_OPACITY:
      return NS_ERROR_NOT_IMPLEMENTED;
    case NS_STYLE_FILTER_SATURATE:
      return NS_ERROR_NOT_IMPLEMENTED;
    case NS_STYLE_FILTER_SEPIA:
      return NS_ERROR_NOT_IMPLEMENTED;
    default:
      NS_NOTREACHED("not a valid CSS filter type");
      return NS_ERROR_FAILURE;
  }

  if (NS_FAILED(result)) {
    return result;
  }

  
  
  SetBounds(descr, aPrimitiveDescrs);

  
  aPrimitiveDescrs.AppendElement(descr);
  return NS_OK;
}

FilterPrimitiveDescription
nsCSSFilterInstance::CreatePrimitiveDescription(PrimitiveType aType,
                                                const nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs) {
  FilterPrimitiveDescription descr(aType);
  int32_t inputIndex = GetLastResultIndex(aPrimitiveDescrs);
  descr.SetInputPrimitive(0, inputIndex);
  descr.SetIsTainted(inputIndex < 0 ? true : aPrimitiveDescrs[inputIndex].IsTainted());
  descr.SetInputColorSpace(0, ColorSpace::SRGB);
  descr.SetOutputColorSpace(ColorSpace::SRGB);
  return descr;
}

nsresult
nsCSSFilterInstance::SetAttributesForBlur(FilterPrimitiveDescription& aDescr)
{
  const nsStyleCoord& radiusInFrameSpace = mFilter.GetFilterParameter();
  if (radiusInFrameSpace.GetUnit() != eStyleUnit_Coord) {
    NS_NOTREACHED("unexpected unit");
    return NS_ERROR_FAILURE;
  }

  Size radiusInFilterSpace = BlurRadiusToFilterSpace(radiusInFrameSpace.GetCoordValue());
  aDescr.Attributes().Set(eGaussianBlurStdDeviation, radiusInFilterSpace);
  return NS_OK;
}

nsresult
nsCSSFilterInstance::SetAttributesForDropShadow(FilterPrimitiveDescription& aDescr)
{
  nsCSSShadowArray* shadows = mFilter.GetDropShadow();
  if (!shadows || shadows->Length() != 1) {
    NS_NOTREACHED("Exactly one drop shadow should have been parsed.");
    return NS_ERROR_FAILURE;
  }

  nsCSSShadowItem* shadow = shadows->ShadowAt(0);

  
  Size radiusInFilterSpace = BlurRadiusToFilterSpace(shadow->mRadius);
  aDescr.Attributes().Set(eDropShadowStdDeviation, radiusInFilterSpace);

  
  IntPoint offsetInFilterSpace = OffsetToFilterSpace(shadow->mXOffset, shadow->mYOffset);
  aDescr.Attributes().Set(eDropShadowOffset, offsetInFilterSpace);

  
  nscolor shadowColor = shadow->mHasColor ?
    shadow->mColor : mTargetFrame->StyleColor()->mColor;
  aDescr.Attributes().Set(eDropShadowColor, ToAttributeColor(shadowColor));

  return NS_OK;
}

Size
nsCSSFilterInstance::BlurRadiusToFilterSpace(nscoord aRadiusInFrameSpace)
{
  float radiusInFrameSpaceInCSSPx =
    nsPresContext::AppUnitsToFloatCSSPixels(aRadiusInFrameSpace);

  
  gfxSize radiusInFilterSpace(radiusInFrameSpaceInCSSPx,
                              radiusInFrameSpaceInCSSPx);
  gfxSize frameSpaceInCSSPxToFilterSpaceScale =
    mFrameSpaceInCSSPxToFilterSpaceTransform.ScaleFactors(true);
  radiusInFilterSpace.Scale(frameSpaceInCSSPxToFilterSpaceScale.width,
                            frameSpaceInCSSPxToFilterSpaceScale.height);

  
  if (radiusInFilterSpace.width < 0 || radiusInFilterSpace.height < 0) {
    NS_NOTREACHED("we shouldn't have parsed a negative radius in the style");
    return Size();
  }
  gfxFloat maxStdDeviation = (gfxFloat)kMaxStdDeviation;
  radiusInFilterSpace.width = std::min(radiusInFilterSpace.width, maxStdDeviation);
  radiusInFilterSpace.height = std::min(radiusInFilterSpace.height, maxStdDeviation);

  return ToSize(radiusInFilterSpace);
}

IntPoint
nsCSSFilterInstance::OffsetToFilterSpace(nscoord aXOffsetInFrameSpace,
                                         nscoord aYOffsetInFrameSpace)
{
  gfxPoint offsetInFilterSpace(nsPresContext::AppUnitsToFloatCSSPixels(aXOffsetInFrameSpace),
                               nsPresContext::AppUnitsToFloatCSSPixels(aYOffsetInFrameSpace));

  
  gfxSize frameSpaceInCSSPxToFilterSpaceScale =
    mFrameSpaceInCSSPxToFilterSpaceTransform.ScaleFactors(true);
  offsetInFilterSpace.x *= frameSpaceInCSSPxToFilterSpaceScale.width;
  offsetInFilterSpace.y *= frameSpaceInCSSPxToFilterSpaceScale.height;

  return IntPoint(int32_t(offsetInFilterSpace.x), int32_t(offsetInFilterSpace.y));
}

Color
nsCSSFilterInstance::ToAttributeColor(nscolor aColor)
{
  return Color(
    NS_GET_R(aColor) / 255.0,
    NS_GET_G(aColor) / 255.0,
    NS_GET_B(aColor) / 255.0,
    NS_GET_A(aColor) / 255.0
  );
}

int32_t
nsCSSFilterInstance::GetLastResultIndex(const nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs)
{
  uint32_t numPrimitiveDescrs = aPrimitiveDescrs.Length();
  return !numPrimitiveDescrs ?
    FilterPrimitiveDescription::kPrimitiveIndexSourceGraphic :
    numPrimitiveDescrs - 1;
}

void
nsCSSFilterInstance::SetBounds(FilterPrimitiveDescription& aDescr,
                               const nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs) {
  int32_t inputIndex = GetLastResultIndex(aPrimitiveDescrs);
  nsIntRect inputBounds = (inputIndex < 0) ?
    mTargetBBoxInFilterSpace : ThebesIntRect(aPrimitiveDescrs[inputIndex].PrimitiveSubregion());

  nsTArray<nsIntRegion> inputExtents;
  inputExtents.AppendElement(inputBounds);

  nsIntRegion outputExtents =
    FilterSupport::PostFilterExtentsForPrimitive(aDescr, inputExtents);
  IntRect outputBounds = ToIntRect(outputExtents.GetBounds());

  aDescr.SetPrimitiveSubregion(outputBounds);
  aDescr.SetFilterSpaceBounds(outputBounds);
}
