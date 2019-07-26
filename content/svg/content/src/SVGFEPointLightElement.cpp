




#include "mozilla/dom/SVGFEPointLightElement.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(FEPointLight)

namespace mozilla {
namespace dom {

nsSVGElement::NumberInfo nsSVGFEPointLightElement::sNumberInfo[3] =
{
  { &nsGkAtoms::x, 0, false },
  { &nsGkAtoms::y, 0, false },
  { &nsGkAtoms::z, 0, false }
};




NS_IMPL_ADDREF_INHERITED(nsSVGFEPointLightElement,nsSVGFEPointLightElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFEPointLightElement,nsSVGFEPointLightElementBase)

NS_INTERFACE_TABLE_HEAD(nsSVGFEPointLightElement)
  NS_NODE_INTERFACE_TABLE4(nsSVGFEPointLightElement, nsIDOMNode,
                           nsIDOMElement, nsIDOMSVGElement,
                           nsIDOMSVGFEPointLightElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGFEPointLightElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGFEPointLightElementBase)




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGFEPointLightElement)




bool
nsSVGFEPointLightElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                    nsIAtom* aAttribute) const
{
  return aNameSpaceID == kNameSpaceID_None &&
         (aAttribute == nsGkAtoms::x ||
          aAttribute == nsGkAtoms::y ||
          aAttribute == nsGkAtoms::z);
}




NS_IMETHODIMP
nsSVGFEPointLightElement::GetX(nsIDOMSVGAnimatedNumber **aX)
{
  return mNumberAttributes[X].ToDOMAnimatedNumber(aX, this);
}

NS_IMETHODIMP
nsSVGFEPointLightElement::GetY(nsIDOMSVGAnimatedNumber **aY)
{
  return mNumberAttributes[Y].ToDOMAnimatedNumber(aY, this);
}

NS_IMETHODIMP
nsSVGFEPointLightElement::GetZ(nsIDOMSVGAnimatedNumber **aZ)
{
  return mNumberAttributes[Z].ToDOMAnimatedNumber(aZ, this);
}




nsSVGElement::NumberAttributesInfo
nsSVGFEPointLightElement::GetNumberInfo()
{
  return NumberAttributesInfo(mNumberAttributes, sNumberInfo,
                              ArrayLength(sNumberInfo));
}

} 
} 
