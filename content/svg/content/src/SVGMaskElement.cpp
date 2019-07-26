




#include "mozilla/Util.h"

#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "mozilla/dom/SVGMaskElement.h"

DOMCI_NODE_DATA(SVGMaskElement, mozilla::dom::SVGMaskElement)

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Mask)

namespace mozilla {
namespace dom {



nsSVGElement::LengthInfo SVGMaskElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, -10, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::X },
  { &nsGkAtoms::y, -10, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::Y },
  { &nsGkAtoms::width, 120, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::X },
  { &nsGkAtoms::height, 120, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::Y },
};

nsSVGElement::EnumInfo SVGMaskElement::sEnumInfo[2] =
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




NS_IMPL_ADDREF_INHERITED(SVGMaskElement,SVGMaskElementBase)
NS_IMPL_RELEASE_INHERITED(SVGMaskElement,SVGMaskElementBase)

NS_INTERFACE_TABLE_HEAD(SVGMaskElement)
  NS_NODE_INTERFACE_TABLE5(SVGMaskElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGMaskElement, nsIDOMSVGUnitTypes)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGMaskElement)
NS_INTERFACE_MAP_END_INHERITING(SVGMaskElementBase)




SVGMaskElement::SVGMaskElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGMaskElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGMaskElement)





NS_IMETHODIMP SVGMaskElement::GetMaskUnits(nsIDOMSVGAnimatedEnumeration * *aMaskUnits)
{
  return mEnumAttributes[MASKUNITS].ToDOMAnimatedEnum(aMaskUnits, this);
}


NS_IMETHODIMP SVGMaskElement::GetMaskContentUnits(nsIDOMSVGAnimatedEnumeration * *aMaskUnits)
{
  return mEnumAttributes[MASKCONTENTUNITS].ToDOMAnimatedEnum(aMaskUnits, this);
}


NS_IMETHODIMP SVGMaskElement::GetX(nsIDOMSVGAnimatedLength * *aX)
{
  return mLengthAttributes[X].ToDOMAnimatedLength(aX, this);
}


NS_IMETHODIMP SVGMaskElement::GetY(nsIDOMSVGAnimatedLength * *aY)
{
  return mLengthAttributes[Y].ToDOMAnimatedLength(aY, this);
}


NS_IMETHODIMP SVGMaskElement::GetWidth(nsIDOMSVGAnimatedLength * *aWidth)
{
  return mLengthAttributes[WIDTH].ToDOMAnimatedLength(aWidth, this);
}


NS_IMETHODIMP SVGMaskElement::GetHeight(nsIDOMSVGAnimatedLength * *aHeight)
{
  return mLengthAttributes[HEIGHT].ToDOMAnimatedLength(aHeight, this);
}




 bool
SVGMaskElement::HasValidDimensions() const
{
  return (!mLengthAttributes[WIDTH].IsExplicitlySet() ||
           mLengthAttributes[WIDTH].GetAnimValInSpecifiedUnits() > 0) &&
         (!mLengthAttributes[HEIGHT].IsExplicitlySet() || 
           mLengthAttributes[HEIGHT].GetAnimValInSpecifiedUnits() > 0);
}

nsSVGElement::LengthAttributesInfo
SVGMaskElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}

nsSVGElement::EnumAttributesInfo
SVGMaskElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}




NS_IMETHODIMP_(bool)
SVGMaskElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sFEFloodMap,
    sFiltersMap,
    sFontSpecificationMap,
    sGradientStopMap,
    sGraphicsMap,
    sMarkersMap,
    sMaskMap,
    sTextContentElementsMap,
    sViewportsMap
  };

  return FindAttributeDependence(name, map) ||
    SVGMaskElementBase::IsAttributeMapped(name);
}

} 
} 
