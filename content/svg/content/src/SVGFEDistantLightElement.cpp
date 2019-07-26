




#include "mozilla/dom/SVGFEDistantLightElement.h"

NS_IMPL_NS_NEW_SVG_ELEMENT(FEDistantLight)

nsSVGElement::NumberInfo nsSVGFEDistantLightElement::sNumberInfo[2] =
{
  { &nsGkAtoms::azimuth,   0, false },
  { &nsGkAtoms::elevation, 0, false }
};




NS_IMPL_ADDREF_INHERITED(nsSVGFEDistantLightElement,nsSVGFEDistantLightElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFEDistantLightElement,nsSVGFEDistantLightElementBase)

DOMCI_NODE_DATA(SVGFEDistantLightElement, nsSVGFEDistantLightElement)

NS_INTERFACE_TABLE_HEAD(nsSVGFEDistantLightElement)
  NS_NODE_INTERFACE_TABLE4(nsSVGFEDistantLightElement, nsIDOMNode,
                           nsIDOMElement, nsIDOMSVGElement,
                           nsIDOMSVGFEDistantLightElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGFEDistantLightElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGFEDistantLightElementBase)




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGFEDistantLightElement)




bool
nsSVGFEDistantLightElement::AttributeAffectsRendering(int32_t aNameSpaceID,
                                                      nsIAtom* aAttribute) const
{
  return aNameSpaceID == kNameSpaceID_None &&
         (aAttribute == nsGkAtoms::azimuth ||
          aAttribute == nsGkAtoms::elevation);
}




NS_IMETHODIMP
nsSVGFEDistantLightElement::GetAzimuth(nsIDOMSVGAnimatedNumber **aAzimuth)
{
  return mNumberAttributes[AZIMUTH].ToDOMAnimatedNumber(aAzimuth,
                                                        this);
}

NS_IMETHODIMP
nsSVGFEDistantLightElement::GetElevation(nsIDOMSVGAnimatedNumber **aElevation)
{
  return mNumberAttributes[ELEVATION].ToDOMAnimatedNumber(aElevation,
                                                          this);
}
