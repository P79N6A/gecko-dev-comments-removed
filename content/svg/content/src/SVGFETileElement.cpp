




#include "mozilla/dom/SVGFETileElement.h"
#include "mozilla/dom/SVGFETileElementBinding.h"
#include "nsSVGFilterInstance.h"
#include "gfxUtils.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FETile)

using namespace mozilla::gfx;

namespace mozilla {
namespace dom {

JSObject*
SVGFETileElement::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aScope)
{
  return SVGFETileElementBinding::Wrap(aCx, aScope, this);
}

nsSVGElement::StringInfo SVGFETileElement::sStringInfo[2] =
{
  { &nsGkAtoms::result, kNameSpaceID_None, true },
  { &nsGkAtoms::in, kNameSpaceID_None, true }
};





NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGFETileElement)

already_AddRefed<SVGAnimatedString>
SVGFETileElement::In1()
{
  return mStringAttributes[IN1].ToDOMAnimatedString(this);
}

void
SVGFETileElement::GetSourceImageNames(nsTArray<nsSVGStringInfo>& aSources)
{
  aSources.AppendElement(nsSVGStringInfo(&mStringAttributes[IN1], this));
}




FilterPrimitiveDescription
SVGFETileElement::GetPrimitiveDescription(nsSVGFilterInstance* aInstance,
                                          const IntRect& aFilterSubregion,
                                          nsTArray<nsRefPtr<gfxASurface> >& aInputImages)
{
  return FilterPrimitiveDescription(FilterPrimitiveDescription::eTile);
}

bool
SVGFETileElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                            nsIAtom* aAttribute) const
{
  return SVGFETileElementBase::AttributeAffectsRendering(aNameSpaceID,
                                                         aAttribute) ||
           (aNameSpaceID == kNameSpaceID_None && aAttribute == nsGkAtoms::in);
}




nsSVGElement::StringAttributesInfo
SVGFETileElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}

} 
} 
