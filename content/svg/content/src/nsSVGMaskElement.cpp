



































#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "nsSVGMaskElement.h"



nsSVGElement::LengthInfo nsSVGMaskElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, -10, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::y, -10, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
  { &nsGkAtoms::width, 120, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::height, 120, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
};

nsSVGElement::EnumInfo nsSVGMaskElement::sEnumInfo[2] =
{
  { &nsGkAtoms::maskUnits,
    sSVGUnitTypesMap,
    nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX
  },
  { &nsGkAtoms::maskContentUnits,
    sSVGUnitTypesMap,
    nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE
  }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Mask)




NS_IMPL_ADDREF_INHERITED(nsSVGMaskElement,nsSVGMaskElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGMaskElement,nsSVGMaskElementBase)

DOMCI_NODE_DATA(SVGMaskElement, nsSVGMaskElement)

NS_INTERFACE_TABLE_HEAD(nsSVGMaskElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGMaskElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGMaskElement,
                           nsIDOMSVGUnitTypes)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGMaskElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGMaskElementBase)




nsSVGMaskElement::nsSVGMaskElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGMaskElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGMaskElement)





NS_IMETHODIMP nsSVGMaskElement::GetMaskUnits(nsIDOMSVGAnimatedEnumeration * *aMaskUnits)
{
  return mEnumAttributes[MASKUNITS].ToDOMAnimatedEnum(aMaskUnits, this);
}


NS_IMETHODIMP nsSVGMaskElement::GetMaskContentUnits(nsIDOMSVGAnimatedEnumeration * *aMaskUnits)
{
  return mEnumAttributes[MASKCONTENTUNITS].ToDOMAnimatedEnum(aMaskUnits, this);
}


NS_IMETHODIMP nsSVGMaskElement::GetX(nsIDOMSVGAnimatedLength * *aX)
{
  return mLengthAttributes[X].ToDOMAnimatedLength(aX, this);
}


NS_IMETHODIMP nsSVGMaskElement::GetY(nsIDOMSVGAnimatedLength * *aY)
{
  return mLengthAttributes[Y].ToDOMAnimatedLength(aY, this);
}


NS_IMETHODIMP nsSVGMaskElement::GetWidth(nsIDOMSVGAnimatedLength * *aWidth)
{
  return mLengthAttributes[WIDTH].ToDOMAnimatedLength(aWidth, this);
}


NS_IMETHODIMP nsSVGMaskElement::GetHeight(nsIDOMSVGAnimatedLength * *aHeight)
{
  return mLengthAttributes[HEIGHT].ToDOMAnimatedLength(aHeight, this);
}




nsSVGElement::LengthAttributesInfo
nsSVGMaskElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              NS_ARRAY_LENGTH(sLengthInfo));
}

nsSVGElement::EnumAttributesInfo
nsSVGMaskElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            NS_ARRAY_LENGTH(sEnumInfo));
}




NS_IMETHODIMP_(bool)
nsSVGMaskElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sFEFloodMap,
    sFiltersMap,
    sFontSpecificationMap,
    sGradientStopMap,
    sGraphicsMap,
    sMarkersMap,
    sTextContentElementsMap,
    sViewportsMap
  };

  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGMaskElementBase::IsAttributeMapped(name);
}
