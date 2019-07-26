




#include "mozilla/dom/SVGFEDistantLightElement.h"
#include "mozilla/dom/SVGFEDistantLightElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FEDistantLight)

namespace mozilla {
namespace dom {

JSObject*
SVGFEDistantLightElement::WrapNode(JSContext* aCx, JSObject* aScope)
{
  return SVGFEDistantLightElementBinding::Wrap(aCx, aScope, this);
}

nsSVGElement::NumberInfo SVGFEDistantLightElement::sNumberInfo[2] =
{
  { &nsGkAtoms::azimuth,   0, false },
  { &nsGkAtoms::elevation, 0, false }
};




NS_IMPL_ADDREF_INHERITED(SVGFEDistantLightElement,SVGFEDistantLightElementBase)
NS_IMPL_RELEASE_INHERITED(SVGFEDistantLightElement,SVGFEDistantLightElementBase)

NS_INTERFACE_TABLE_HEAD(SVGFEDistantLightElement)
  NS_NODE_INTERFACE_TABLE3(SVGFEDistantLightElement, nsIDOMNode,
                           nsIDOMElement, nsIDOMSVGElement)
NS_INTERFACE_MAP_END_INHERITING(SVGFEDistantLightElementBase)




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGFEDistantLightElement)



bool
SVGFEDistantLightElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                    nsIAtom* aAttribute) const
{
  return aNameSpaceID == kNameSpaceID_None &&
         (aAttribute == nsGkAtoms::azimuth ||
          aAttribute == nsGkAtoms::elevation);
}

already_AddRefed<nsIDOMSVGAnimatedNumber>
SVGFEDistantLightElement::Azimuth()
{
  return mNumberAttributes[AZIMUTH].ToDOMAnimatedNumber(this);
}

already_AddRefed<nsIDOMSVGAnimatedNumber>
SVGFEDistantLightElement::Elevation()
{
  return mNumberAttributes[ELEVATION].ToDOMAnimatedNumber(this);
}




nsSVGElement::NumberAttributesInfo
SVGFEDistantLightElement::GetNumberInfo()
{
  return NumberAttributesInfo(mNumberAttributes, sNumberInfo,
                              ArrayLength(sNumberInfo));
}

} 
} 
