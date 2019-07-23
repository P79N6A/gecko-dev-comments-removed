





































#include "nsSVGLength.h"
#include "nsCOMPtr.h"
#include "nsISVGSVGElement.h"
#include "nsSVGForeignObjectElement.h"

nsSVGElement::LengthInfo nsSVGForeignObjectElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::y, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
  { &nsGkAtoms::width, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::height, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
};

NS_IMPL_NS_NEW_SVG_ELEMENT(ForeignObject)




NS_IMPL_ADDREF_INHERITED(nsSVGForeignObjectElement,nsSVGForeignObjectElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGForeignObjectElement,nsSVGForeignObjectElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGForeignObjectElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGForeignObjectElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGForeignObjectElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGForeignObjectElementBase)




nsSVGForeignObjectElement::nsSVGForeignObjectElement(nsINodeInfo *aNodeInfo)
  : nsSVGForeignObjectElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGForeignObjectElement)





NS_IMETHODIMP nsSVGForeignObjectElement::GetX(nsIDOMSVGAnimatedLength * *aX)
{
  return mLengthAttributes[X].ToDOMAnimatedLength(aX, this);
}


NS_IMETHODIMP nsSVGForeignObjectElement::GetY(nsIDOMSVGAnimatedLength * *aY)
{
  return mLengthAttributes[Y].ToDOMAnimatedLength(aY, this);
}


NS_IMETHODIMP nsSVGForeignObjectElement::GetWidth(nsIDOMSVGAnimatedLength * *aWidth)
{
  return mLengthAttributes[WIDTH].ToDOMAnimatedLength(aWidth, this);
}


NS_IMETHODIMP nsSVGForeignObjectElement::GetHeight(nsIDOMSVGAnimatedLength * *aHeight)
{
  return mLengthAttributes[HEIGHT].ToDOMAnimatedLength(aHeight, this);
}




NS_IMETHODIMP_(PRBool)
nsSVGForeignObjectElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sFEFloodMap,
    sFiltersMap,
    sFontSpecificationMap,
    sGradientStopMap,
    sMarkersMap,
    sTextContentElementsMap,
    sViewportsMap
  };

  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGForeignObjectElementBase::IsAttributeMapped(name);
}




nsSVGElement::LengthAttributesInfo
nsSVGForeignObjectElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              NS_ARRAY_LENGTH(sLengthInfo));
}
