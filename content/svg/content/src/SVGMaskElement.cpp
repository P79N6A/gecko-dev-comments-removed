




#include "mozilla/Util.h"

#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "mozilla/dom/SVGMaskElement.h"
#include "mozilla/dom/SVGMaskElementBinding.h"
#include "mozilla/dom/SVGAnimatedLength.h"

DOMCI_NODE_DATA(SVGMaskElement, mozilla::dom::SVGMaskElement)

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Mask)

namespace mozilla {
namespace dom {

JSObject*
SVGMaskElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGMaskElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}



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
    SVG_UNIT_TYPE_OBJECTBOUNDINGBOX
  },
  { &nsGkAtoms::maskContentUnits,
    sSVGUnitTypesMap,
    SVG_UNIT_TYPE_USERSPACEONUSE
  }
};




NS_IMPL_ADDREF_INHERITED(SVGMaskElement,SVGMaskElementBase)
NS_IMPL_RELEASE_INHERITED(SVGMaskElement,SVGMaskElementBase)

NS_INTERFACE_TABLE_HEAD(SVGMaskElement)
  NS_NODE_INTERFACE_TABLE4(SVGMaskElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGMaskElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGMaskElement)
NS_INTERFACE_MAP_END_INHERITING(SVGMaskElementBase)




SVGMaskElement::SVGMaskElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGMaskElementBase(aNodeInfo)
{
  SetIsDOMBinding();
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGMaskElement)





NS_IMETHODIMP SVGMaskElement::GetMaskUnits(nsIDOMSVGAnimatedEnumeration * *aMaskUnits)
{
  *aMaskUnits = MaskUnits().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGAnimatedEnumeration>
SVGMaskElement::MaskUnits()
{
  return mEnumAttributes[MASKUNITS].ToDOMAnimatedEnum(this);
}


NS_IMETHODIMP SVGMaskElement::GetMaskContentUnits(nsIDOMSVGAnimatedEnumeration * *aMaskUnits)
{
  *aMaskUnits = MaskContentUnits().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGAnimatedEnumeration>
SVGMaskElement::MaskContentUnits()
{
  return mEnumAttributes[MASKCONTENTUNITS].ToDOMAnimatedEnum(this);
}


NS_IMETHODIMP SVGMaskElement::GetX(nsIDOMSVGAnimatedLength * *aX)
{
  *aX = X().get();
  return NS_OK;
}

already_AddRefed<SVGAnimatedLength>
SVGMaskElement::X()
{
  return mLengthAttributes[ATTR_X].ToDOMAnimatedLength(this);
}


NS_IMETHODIMP SVGMaskElement::GetY(nsIDOMSVGAnimatedLength * *aY)
{
  *aY = Y().get();
  return NS_OK;
}

already_AddRefed<SVGAnimatedLength>
SVGMaskElement::Y()
{
  return mLengthAttributes[ATTR_Y].ToDOMAnimatedLength(this);
}


NS_IMETHODIMP SVGMaskElement::GetWidth(nsIDOMSVGAnimatedLength * *aWidth)
{
  *aWidth = Width().get();
  return NS_OK;
}

already_AddRefed<SVGAnimatedLength>
SVGMaskElement::Width()
{
  return mLengthAttributes[ATTR_WIDTH].ToDOMAnimatedLength(this);
}


NS_IMETHODIMP SVGMaskElement::GetHeight(nsIDOMSVGAnimatedLength * *aHeight)
{
  *aHeight = Height().get();
  return NS_OK;
}

already_AddRefed<SVGAnimatedLength>
SVGMaskElement::Height()
{
  return mLengthAttributes[ATTR_HEIGHT].ToDOMAnimatedLength(this);
}




 bool
SVGMaskElement::HasValidDimensions() const
{
  return (!mLengthAttributes[ATTR_WIDTH].IsExplicitlySet() ||
           mLengthAttributes[ATTR_WIDTH].GetAnimValInSpecifiedUnits() > 0) &&
         (!mLengthAttributes[ATTR_HEIGHT].IsExplicitlySet() ||
           mLengthAttributes[ATTR_HEIGHT].GetAnimValInSpecifiedUnits() > 0);
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
