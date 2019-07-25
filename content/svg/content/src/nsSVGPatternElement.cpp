





































#include "DOMSVGAnimatedTransformList.h"
#include "nsIDOMMutationEvent.h"
#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "nsSVGPatternElement.h"
#include "nsIFrame.h"

using namespace mozilla;



nsSVGElement::LengthInfo nsSVGPatternElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::y, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
  { &nsGkAtoms::width, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::height, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
};

nsSVGElement::EnumInfo nsSVGPatternElement::sEnumInfo[2] =
{
  { &nsGkAtoms::patternUnits,
    sSVGUnitTypesMap,
    nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX
  },
  { &nsGkAtoms::patternContentUnits,
    sSVGUnitTypesMap,
    nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE
  }
};

nsSVGElement::StringInfo nsSVGPatternElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink, PR_TRUE }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Pattern)




NS_IMPL_ADDREF_INHERITED(nsSVGPatternElement,nsSVGPatternElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGPatternElement,nsSVGPatternElementBase)

DOMCI_NODE_DATA(SVGPatternElement, nsSVGPatternElement)

NS_INTERFACE_TABLE_HEAD(nsSVGPatternElement)
  NS_NODE_INTERFACE_TABLE7(nsSVGPatternElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGFitToViewBox,
                           nsIDOMSVGURIReference, nsIDOMSVGPatternElement,
                           nsIDOMSVGUnitTypes)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGPatternElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGPatternElementBase)




nsSVGPatternElement::nsSVGPatternElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGPatternElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGPatternElement)





NS_IMETHODIMP nsSVGPatternElement::GetViewBox(nsIDOMSVGAnimatedRect * *aViewBox)
{
  return mViewBox.ToDOMAnimatedRect(aViewBox, this);
}


NS_IMETHODIMP
nsSVGPatternElement::GetPreserveAspectRatio(nsIDOMSVGAnimatedPreserveAspectRatio
                                            **aPreserveAspectRatio)
{
  return mPreserveAspectRatio.ToDOMAnimatedPreserveAspectRatio(aPreserveAspectRatio, this);
}





NS_IMETHODIMP nsSVGPatternElement::GetPatternUnits(nsIDOMSVGAnimatedEnumeration * *aPatternUnits)
{
  return mEnumAttributes[PATTERNUNITS].ToDOMAnimatedEnum(aPatternUnits, this);
}


NS_IMETHODIMP nsSVGPatternElement::GetPatternContentUnits(nsIDOMSVGAnimatedEnumeration * *aPatternUnits)
{
  return mEnumAttributes[PATTERNCONTENTUNITS].ToDOMAnimatedEnum(aPatternUnits, this);
}


NS_IMETHODIMP nsSVGPatternElement::GetPatternTransform(nsIDOMSVGAnimatedTransformList * *aPatternTransform)
{
  *aPatternTransform =
    DOMSVGAnimatedTransformList::GetDOMWrapper(GetAnimatedTransformList(), this)
    .get();
  return NS_OK;
}


NS_IMETHODIMP nsSVGPatternElement::GetX(nsIDOMSVGAnimatedLength * *aX)
{
  return mLengthAttributes[X].ToDOMAnimatedLength(aX, this);
}


NS_IMETHODIMP nsSVGPatternElement::GetY(nsIDOMSVGAnimatedLength * *aY)
{
  return mLengthAttributes[Y].ToDOMAnimatedLength(aY, this);
}


NS_IMETHODIMP nsSVGPatternElement::GetWidth(nsIDOMSVGAnimatedLength * *aWidth)
{
  return mLengthAttributes[WIDTH].ToDOMAnimatedLength(aWidth, this);
}


NS_IMETHODIMP nsSVGPatternElement::GetHeight(nsIDOMSVGAnimatedLength * *aHeight)
{
  return mLengthAttributes[HEIGHT].ToDOMAnimatedLength(aHeight, this);
}






NS_IMETHODIMP
nsSVGPatternElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  return mStringAttributes[HREF].ToDOMAnimatedString(aHref, this);
}




NS_IMETHODIMP_(bool)
nsSVGPatternElement::IsAttributeMapped(const nsIAtom* name) const
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
    nsSVGPatternElementBase::IsAttributeMapped(name);
}




SVGAnimatedTransformList*
nsSVGPatternElement::GetAnimatedTransformList()
{
  if (!mPatternTransform) {
    mPatternTransform = new SVGAnimatedTransformList();
  }
  return mPatternTransform;
}

nsSVGElement::LengthAttributesInfo
nsSVGPatternElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              NS_ARRAY_LENGTH(sLengthInfo));
}

nsSVGElement::EnumAttributesInfo
nsSVGPatternElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            NS_ARRAY_LENGTH(sEnumInfo));
}

nsSVGViewBox *
nsSVGPatternElement::GetViewBox()
{
  return &mViewBox;
}

SVGAnimatedPreserveAspectRatio *
nsSVGPatternElement::GetPreserveAspectRatio()
{
  return &mPreserveAspectRatio;
}

nsSVGElement::StringAttributesInfo
nsSVGPatternElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              NS_ARRAY_LENGTH(sStringInfo));
}

