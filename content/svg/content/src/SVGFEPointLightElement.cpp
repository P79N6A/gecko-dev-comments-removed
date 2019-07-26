




#include "mozilla/dom/SVGFEPointLightElement.h"
#include "mozilla/dom/SVGFEPointLightElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FEPointLight)

namespace mozilla {
namespace dom {

JSObject*
SVGFEPointLightElement::WrapNode(JSContext *aCx, JSObject *aScope)
{
  return SVGFEPointLightElementBinding::Wrap(aCx, aScope, this);
}

nsSVGElement::NumberInfo SVGFEPointLightElement::sNumberInfo[3] =
{
  { &nsGkAtoms::x, 0, false },
  { &nsGkAtoms::y, 0, false },
  { &nsGkAtoms::z, 0, false }
};




NS_IMPL_ADDREF_INHERITED(SVGFEPointLightElement,SVGFEPointLightElementBase)
NS_IMPL_RELEASE_INHERITED(SVGFEPointLightElement,SVGFEPointLightElementBase)

NS_INTERFACE_TABLE_HEAD(SVGFEPointLightElement)
  NS_NODE_INTERFACE_TABLE3(SVGFEPointLightElement, nsIDOMNode,
                           nsIDOMElement, nsIDOMSVGElement)
NS_INTERFACE_MAP_END_INHERITING(SVGFEPointLightElementBase)




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGFEPointLightElement)




bool
SVGFEPointLightElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                  nsIAtom* aAttribute) const
{
  return aNameSpaceID == kNameSpaceID_None &&
         (aAttribute == nsGkAtoms::x ||
          aAttribute == nsGkAtoms::y ||
          aAttribute == nsGkAtoms::z);
}



already_AddRefed<nsIDOMSVGAnimatedNumber>
SVGFEPointLightElement::X()
{
  return mNumberAttributes[ATTR_X].ToDOMAnimatedNumber(this);
}

already_AddRefed<nsIDOMSVGAnimatedNumber>
SVGFEPointLightElement::Y()
{
  return mNumberAttributes[ATTR_Y].ToDOMAnimatedNumber(this);
}

already_AddRefed<nsIDOMSVGAnimatedNumber>
SVGFEPointLightElement::Z()
{
  return mNumberAttributes[ATTR_Z].ToDOMAnimatedNumber(this);
}




nsSVGElement::NumberAttributesInfo
SVGFEPointLightElement::GetNumberInfo()
{
  return NumberAttributesInfo(mNumberAttributes, sNumberInfo,
                              ArrayLength(sNumberInfo));
}

} 
} 
