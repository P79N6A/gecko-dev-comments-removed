





#include "nsSVGFilterInstance.h"


#include "gfxPlatform.h"
#include "gfxUtils.h"
#include "nsISVGChildFrame.h"
#include "mozilla/dom/SVGFilterElement.h"
#include "nsReferencedElement.h"
#include "nsSVGFilterFrame.h"
#include "nsSVGUtils.h"
#include "SVGContentUtils.h"
#include "FilterSupport.h"
#include "gfx2DGlue.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::gfx;

nsSVGFilterInstance::nsSVGFilterInstance(const nsStyleFilter& aFilter,
                                         nsIContent* aTargetContent,
                                         const UserSpaceMetrics& aMetrics,
                                         const gfxRect& aTargetBBox,
                                         const gfxSize& aUserSpaceToFilterSpaceScale,
                                         const gfxSize& aFilterSpaceToUserSpaceScale) :
  mFilter(aFilter),
  mTargetContent(aTargetContent),
  mMetrics(aMetrics),
  mTargetBBox(aTargetBBox),
  mUserSpaceToFilterSpaceScale(aUserSpaceToFilterSpaceScale),
  mFilterSpaceToUserSpaceScale(aFilterSpaceToUserSpaceScale),
  mSourceAlphaAvailable(false),
  mInitialized(false) {

  
  mFilterFrame = GetFilterFrame();
  if (!mFilterFrame) {
    return;
  }

  
  mFilterElement = mFilterFrame->GetFilterContent();
  if (!mFilterElement) {
    NS_NOTREACHED("filter frame should have a related element");
    return;
  }

  mPrimitiveUnits =
    mFilterFrame->GetEnumValue(SVGFilterElement::PRIMITIVEUNITS);

  nsresult rv = ComputeBounds();
  if (NS_FAILED(rv)) {
    return;
  }

  mInitialized = true;
}

nsresult
nsSVGFilterInstance::ComputeBounds()
{
  
  
  
  
  
  
  
  
  

  
  nsSVGLength2 XYWH[4];
  static_assert(sizeof(mFilterElement->mLengthAttributes) == sizeof(XYWH),
                "XYWH size incorrect");
  memcpy(XYWH, mFilterElement->mLengthAttributes,
    sizeof(mFilterElement->mLengthAttributes));
  XYWH[0] = *mFilterFrame->GetLengthValue(SVGFilterElement::ATTR_X);
  XYWH[1] = *mFilterFrame->GetLengthValue(SVGFilterElement::ATTR_Y);
  XYWH[2] = *mFilterFrame->GetLengthValue(SVGFilterElement::ATTR_WIDTH);
  XYWH[3] = *mFilterFrame->GetLengthValue(SVGFilterElement::ATTR_HEIGHT);
  uint16_t filterUnits =
    mFilterFrame->GetEnumValue(SVGFilterElement::FILTERUNITS);
  gfxRect userSpaceBounds = nsSVGUtils::GetRelativeRect(filterUnits,
    XYWH, mTargetBBox, mMetrics);

  
  
  
  gfxRect filterSpaceBounds = UserSpaceToFilterSpace(userSpaceBounds);
  filterSpaceBounds.RoundOut();
  if (filterSpaceBounds.width <= 0 || filterSpaceBounds.height <= 0) {
    
    
    return NS_ERROR_FAILURE;
  }

  
  if (!gfxUtils::GfxRectToIntRect(filterSpaceBounds, &mFilterSpaceBounds)) {
    
    return NS_ERROR_FAILURE;
  }

  mUserSpaceBounds = FilterSpaceToUserSpace(filterSpaceBounds);

  return NS_OK;
}

nsSVGFilterFrame*
nsSVGFilterInstance::GetFilterFrame()
{
  if (mFilter.GetType() != NS_STYLE_FILTER_URL) {
    
    return nullptr;
  }

  nsIURI* url = mFilter.GetURL();
  if (!url) {
    NS_NOTREACHED("an nsStyleFilter of type URL should have a non-null URL");
    return nullptr;
  }

  
  
  if (!mTargetContent) {
    return nullptr;
  }

  
  nsReferencedElement filterElement;
  bool watch = false;
  filterElement.Reset(mTargetContent, url, watch);
  Element* element = filterElement.get();
  if (!element) {
    
    return nullptr;
  }

  
  nsIFrame* frame = element->GetPrimaryFrame();
  if (!frame || frame->GetType() != nsGkAtoms::svgFilterFrame) {
    
    
    return nullptr;
  }

  return static_cast<nsSVGFilterFrame*>(frame);
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
    value = nsSVGUtils::UserSpace(mMetrics, &val);
  }

  switch (aCtxType) {
  case SVGContentUtils::X:
    return value * mUserSpaceToFilterSpaceScale.width;
  case SVGContentUtils::Y:
    return value * mUserSpaceToFilterSpaceScale.height;
  case SVGContentUtils::XY:
  default:
    return value * SVGContentUtils::ComputeNormalizedHypotenuse(
                     mUserSpaceToFilterSpaceScale.width,
                     mUserSpaceToFilterSpaceScale.height);
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
    val, mTargetBBox, mMetrics);
  gfxRect r = UserSpaceToFilterSpace(feArea);
  return Point3D(r.x, r.y, GetPrimitiveNumber(SVGContentUtils::XY, aPoint.z));
}

gfxRect
nsSVGFilterInstance::UserSpaceToFilterSpace(const gfxRect& aUserSpaceRect) const
{
  gfxRect filterSpaceRect = aUserSpaceRect;
  filterSpaceRect.Scale(mUserSpaceToFilterSpaceScale.width,
                        mUserSpaceToFilterSpaceScale.height);
  return filterSpaceRect;
}

gfxRect
nsSVGFilterInstance::FilterSpaceToUserSpace(const gfxRect& aFilterSpaceRect) const
{
  gfxRect userSpaceRect = aFilterSpaceRect;
  userSpaceRect.Scale(mFilterSpaceToUserSpaceScale.width,
                      mFilterSpaceToUserSpaceScale.height);
  return userSpaceRect;
}

IntRect
nsSVGFilterInstance::ComputeFilterPrimitiveSubregion(nsSVGFE* aFilterElement,
                                                     const nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs,
                                                     const nsTArray<int32_t>& aInputIndices)
{
  nsSVGFE* fE = aFilterElement;

  IntRect defaultFilterSubregion(0,0,0,0);
  if (fE->SubregionIsUnionOfRegions()) {
    for (uint32_t i = 0; i < aInputIndices.Length(); ++i) {
      int32_t inputIndex = aInputIndices[i];
      bool isStandardInput = inputIndex < 0 || inputIndex == mSourceGraphicIndex;
      IntRect inputSubregion = isStandardInput ?
        mFilterSpaceBounds :
        aPrimitiveDescrs[inputIndex].PrimitiveSubregion();

      defaultFilterSubregion = defaultFilterSubregion.Union(inputSubregion);
    }
  } else {
    defaultFilterSubregion = mFilterSpaceBounds;
  }

  gfxRect feArea = nsSVGUtils::GetRelativeRect(mPrimitiveUnits,
    &fE->mLengthAttributes[nsSVGFE::ATTR_X], mTargetBBox, mMetrics);
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
nsSVGFilterInstance::GetInputsAreTainted(const nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs,
                                         const nsTArray<int32_t>& aInputIndices,
                                         nsTArray<bool>& aOutInputsAreTainted)
{
  for (uint32_t i = 0; i < aInputIndices.Length(); i++) {
    int32_t inputIndex = aInputIndices[i];
    if (inputIndex < 0) {
      
      aOutInputsAreTainted.AppendElement(true);
    } else {
      aOutInputsAreTainted.AppendElement(aPrimitiveDescrs[inputIndex].IsTainted());
    }
  }
}

static int32_t
GetLastResultIndex(const nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs)
{
  uint32_t numPrimitiveDescrs = aPrimitiveDescrs.Length();
  return !numPrimitiveDescrs ?
    FilterPrimitiveDescription::kPrimitiveIndexSourceGraphic :
    numPrimitiveDescrs - 1;
}

int32_t
nsSVGFilterInstance::GetOrCreateSourceAlphaIndex(nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs)
{
  
  
  if (mSourceAlphaAvailable)
    return mSourceAlphaIndex;

  
  
  
  if (mSourceGraphicIndex < 0) {
    mSourceAlphaIndex = FilterPrimitiveDescription::kPrimitiveIndexSourceAlpha;
    mSourceAlphaAvailable = true;
    return mSourceAlphaIndex;
  }

  
  
  FilterPrimitiveDescription descr(PrimitiveType::ToAlpha);
  descr.SetInputPrimitive(0, mSourceGraphicIndex);

  const FilterPrimitiveDescription& sourcePrimitiveDescr =
    aPrimitiveDescrs[mSourceGraphicIndex];
  descr.SetPrimitiveSubregion(sourcePrimitiveDescr.PrimitiveSubregion());
  descr.SetIsTainted(sourcePrimitiveDescr.IsTainted());

  ColorSpace colorSpace = sourcePrimitiveDescr.OutputColorSpace();
  descr.SetInputColorSpace(0, colorSpace);
  descr.SetOutputColorSpace(colorSpace);

  aPrimitiveDescrs.AppendElement(descr);
  mSourceAlphaIndex = aPrimitiveDescrs.Length() - 1;
  mSourceAlphaAvailable = true;
  return mSourceAlphaIndex;
}

nsresult
nsSVGFilterInstance::GetSourceIndices(nsSVGFE* aPrimitiveElement,
                                      nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs,
                                      const nsDataHashtable<nsStringHashKey, int32_t>& aImageTable,
                                      nsTArray<int32_t>& aSourceIndices)
{
  nsAutoTArray<nsSVGStringInfo,2> sources;
  aPrimitiveElement->GetSourceImageNames(sources);

  for (uint32_t j = 0; j < sources.Length(); j++) {
    nsAutoString str;
    sources[j].mString->GetAnimValue(str, sources[j].mElement);

    int32_t sourceIndex = 0;
    if (str.EqualsLiteral("SourceGraphic")) {
      sourceIndex = mSourceGraphicIndex;
    } else if (str.EqualsLiteral("SourceAlpha")) {
      sourceIndex = GetOrCreateSourceAlphaIndex(aPrimitiveDescrs);
    } else if (str.EqualsLiteral("FillPaint")) {
      sourceIndex = FilterPrimitiveDescription::kPrimitiveIndexFillPaint;
    } else if (str.EqualsLiteral("StrokePaint")) {
      sourceIndex = FilterPrimitiveDescription::kPrimitiveIndexStrokePaint;
    } else if (str.EqualsLiteral("BackgroundImage") ||
               str.EqualsLiteral("BackgroundAlpha")) {
      return NS_ERROR_NOT_IMPLEMENTED;
    } else if (str.EqualsLiteral("")) {
      sourceIndex = GetLastResultIndex(aPrimitiveDescrs);
    } else {
      bool inputExists = aImageTable.Get(str, &sourceIndex);
      if (!inputExists)
        return NS_ERROR_FAILURE;
    }

    aSourceIndices.AppendElement(sourceIndex);
  }
  return NS_OK;
}

nsresult
nsSVGFilterInstance::BuildPrimitives(nsTArray<FilterPrimitiveDescription>& aPrimitiveDescrs,
                                     nsTArray<mozilla::RefPtr<SourceSurface>>& aInputImages)
{
  mSourceGraphicIndex = GetLastResultIndex(aPrimitiveDescrs);

  
  if (mSourceGraphicIndex >= 0) {
    FilterPrimitiveDescription& sourceDescr = aPrimitiveDescrs[mSourceGraphicIndex];
    sourceDescr.SetPrimitiveSubregion(sourceDescr.PrimitiveSubregion().Intersect(mFilterSpaceBounds));
  }

  
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

  
  nsDataHashtable<nsStringHashKey, int32_t> imageTable(8);

  
  nsCOMPtr<nsIPrincipal> principal = mTargetContent->NodePrincipal();

  for (uint32_t primitiveElementIndex = 0;
       primitiveElementIndex < primitives.Length();
       ++primitiveElementIndex) {
    nsSVGFE* filter = primitives[primitiveElementIndex];

    nsAutoTArray<int32_t,2> sourceIndices;
    nsresult rv = GetSourceIndices(filter, aPrimitiveDescrs, imageTable, sourceIndices);
    if (NS_FAILED(rv)) {
      return rv;
    }

    IntRect primitiveSubregion =
      ComputeFilterPrimitiveSubregion(filter, aPrimitiveDescrs, sourceIndices);

    nsTArray<bool> sourcesAreTainted;
    GetInputsAreTainted(aPrimitiveDescrs, sourceIndices, sourcesAreTainted);

    FilterPrimitiveDescription descr =
      filter->GetPrimitiveDescription(this, primitiveSubregion, sourcesAreTainted, aInputImages);

    descr.SetIsTainted(filter->OutputIsTainted(sourcesAreTainted, principal));
    descr.SetFilterSpaceBounds(mFilterSpaceBounds);
    descr.SetPrimitiveSubregion(primitiveSubregion.Intersect(descr.FilterSpaceBounds()));

    for (uint32_t i = 0; i < sourceIndices.Length(); i++) {
      int32_t inputIndex = sourceIndices[i];
      descr.SetInputPrimitive(i, inputIndex);

      ColorSpace inputColorSpace = inputIndex >= 0
        ? aPrimitiveDescrs[inputIndex].OutputColorSpace()
        : ColorSpace(ColorSpace::SRGB);

      ColorSpace desiredInputColorSpace = filter->GetInputColorSpace(i, inputColorSpace);
      descr.SetInputColorSpace(i, desiredInputColorSpace);
      if (i == 0) {
        
        descr.SetOutputColorSpace(desiredInputColorSpace);
      }
    }

    if (sourceIndices.Length() == 0) {
      descr.SetOutputColorSpace(filter->GetOutputColorSpace());
    }

    aPrimitiveDescrs.AppendElement(descr);
    uint32_t primitiveDescrIndex = aPrimitiveDescrs.Length() - 1;

    nsAutoString str;
    filter->GetResultImageName().GetAnimValue(str, filter);
    imageTable.Put(str, primitiveDescrIndex);
  }

  return NS_OK;
}
