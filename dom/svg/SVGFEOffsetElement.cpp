




#include "mozilla/dom/SVGFEOffsetElement.h"
#include "mozilla/dom/SVGFEOffsetElementBinding.h"
#include "nsSVGFilterInstance.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FEOffset)

using namespace mozilla::gfx;

namespace mozilla {
namespace dom {

JSObject*
SVGFEOffsetElement::WrapNode(JSContext* aCx)
{
  return SVGFEOffsetElementBinding::Wrap(aCx, this);
}

nsSVGElement::NumberInfo SVGFEOffsetElement::sNumberInfo[2] =
{
  { &nsGkAtoms::dx, 0, false },
  { &nsGkAtoms::dy, 0, false }
};

nsSVGElement::StringInfo SVGFEOffsetElement::sStringInfo[2] =
{
  { &nsGkAtoms::result, kNameSpaceID_None, true },
  { &nsGkAtoms::in, kNameSpaceID_None, true }
};





NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGFEOffsetElement)




already_AddRefed<SVGAnimatedString>
SVGFEOffsetElement::In1()
{
  return mStringAttributes[IN1].ToDOMAnimatedString(this);
}

already_AddRefed<SVGAnimatedNumber>
SVGFEOffsetElement::Dx()
{
  return mNumberAttributes[DX].ToDOMAnimatedNumber(this);
}

already_AddRefed<SVGAnimatedNumber>
SVGFEOffsetElement::Dy()
{
  return mNumberAttributes[DY].ToDOMAnimatedNumber(this);
}

FilterPrimitiveDescription
SVGFEOffsetElement::GetPrimitiveDescription(nsSVGFilterInstance* aInstance,
                                            const IntRect& aFilterSubregion,
                                            const nsTArray<bool>& aInputsAreTainted,
                                            nsTArray<RefPtr<SourceSurface>>& aInputImages)
{
  FilterPrimitiveDescription descr(PrimitiveType::Offset);
  IntPoint offset(int32_t(aInstance->GetPrimitiveNumber(
                            SVGContentUtils::X, &mNumberAttributes[DX])),
                  int32_t(aInstance->GetPrimitiveNumber(
                            SVGContentUtils::Y, &mNumberAttributes[DY])));
  descr.Attributes().Set(eOffsetOffset, offset);
  return descr;
}

bool
SVGFEOffsetElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                              nsIAtom* aAttribute) const
{
  return SVGFEOffsetElementBase::AttributeAffectsRendering(aNameSpaceID, aAttribute) ||
         (aNameSpaceID == kNameSpaceID_None &&
          (aAttribute == nsGkAtoms::in ||
           aAttribute == nsGkAtoms::dx ||
           aAttribute == nsGkAtoms::dy));
}

void
SVGFEOffsetElement::GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources)
{
  aSources.AppendElement(nsSVGStringInfo(&mStringAttributes[IN1], this));
}




nsSVGElement::NumberAttributesInfo
SVGFEOffsetElement::GetNumberInfo()
{
  return NumberAttributesInfo(mNumberAttributes, sNumberInfo,
                              ArrayLength(sNumberInfo));
}

nsSVGElement::StringAttributesInfo
SVGFEOffsetElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}

} 
} 
