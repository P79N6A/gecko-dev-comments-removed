





































#include "nsCOMPtr.h"
#include "nsSVGForeignObjectElement.h"
#include "nsSVGMatrix.h"

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

DOMCI_NODE_DATA(SVGForeignObjectElement, nsSVGForeignObjectElement)

NS_INTERFACE_TABLE_HEAD(nsSVGForeignObjectElement)
  NS_NODE_INTERFACE_TABLE4(nsSVGForeignObjectElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGForeignObjectElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGForeignObjectElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGForeignObjectElementBase)




nsSVGForeignObjectElement::nsSVGForeignObjectElement(already_AddRefed<nsINodeInfo> aNodeInfo)
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




 gfxMatrix
nsSVGForeignObjectElement::PrependLocalTransformTo(const gfxMatrix &aMatrix) const
{
  
  gfxMatrix matrix = nsSVGForeignObjectElementBase::PrependLocalTransformTo(aMatrix);
  
  
  float x, y;
  const_cast<nsSVGForeignObjectElement*>(this)->
    GetAnimatedLengthValues(&x, &y, nsnull);
  return gfxMatrix().Translate(gfxPoint(x, y)) * matrix;
}




NS_IMETHODIMP_(PRBool)
nsSVGForeignObjectElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sFEFloodMap,
    sFiltersMap,
    sFontSpecificationMap,
    sGradientStopMap,
    sLightingEffectsMap,
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
