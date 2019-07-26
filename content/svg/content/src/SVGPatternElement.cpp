




#include "mozilla/Util.h"

#include "DOMSVGAnimatedTransformList.h"
#include "nsIDOMMutationEvent.h"
#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "mozilla/dom/SVGPatternElement.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Pattern)

DOMCI_NODE_DATA(SVGPatternElement, mozilla::dom::SVGPatternElement)

namespace mozilla {
namespace dom {



nsSVGElement::LengthInfo SVGPatternElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::X },
  { &nsGkAtoms::y, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::Y },
  { &nsGkAtoms::width, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::X },
  { &nsGkAtoms::height, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::Y },
};

nsSVGElement::EnumInfo SVGPatternElement::sEnumInfo[2] =
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

nsSVGElement::StringInfo SVGPatternElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink, true }
};




NS_IMPL_ADDREF_INHERITED(SVGPatternElement,SVGPatternElementBase)
NS_IMPL_RELEASE_INHERITED(SVGPatternElement,SVGPatternElementBase)

NS_INTERFACE_TABLE_HEAD(SVGPatternElement)
  NS_NODE_INTERFACE_TABLE7(SVGPatternElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGFitToViewBox, nsIDOMSVGURIReference,
                           nsIDOMSVGPatternElement, nsIDOMSVGUnitTypes)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGPatternElement)
NS_INTERFACE_MAP_END_INHERITING(SVGPatternElementBase)




SVGPatternElement::SVGPatternElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGPatternElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGPatternElement)





NS_IMETHODIMP SVGPatternElement::GetViewBox(nsIDOMSVGAnimatedRect * *aViewBox)
{
  return mViewBox.ToDOMAnimatedRect(aViewBox, this);
}


NS_IMETHODIMP
SVGPatternElement::GetPreserveAspectRatio(nsISupports
                                          **aPreserveAspectRatio)
{
  nsRefPtr<DOMSVGAnimatedPreserveAspectRatio> ratio;
  mPreserveAspectRatio.ToDOMAnimatedPreserveAspectRatio(getter_AddRefs(ratio), this);
  ratio.forget(aPreserveAspectRatio);
  return NS_OK;
}





NS_IMETHODIMP SVGPatternElement::GetPatternUnits(nsIDOMSVGAnimatedEnumeration * *aPatternUnits)
{
  return mEnumAttributes[PATTERNUNITS].ToDOMAnimatedEnum(aPatternUnits, this);
}


NS_IMETHODIMP SVGPatternElement::GetPatternContentUnits(nsIDOMSVGAnimatedEnumeration * *aPatternUnits)
{
  return mEnumAttributes[PATTERNCONTENTUNITS].ToDOMAnimatedEnum(aPatternUnits, this);
}


NS_IMETHODIMP SVGPatternElement::GetPatternTransform(nsISupports * *aPatternTransform)
{
  
  
  *aPatternTransform = DOMSVGAnimatedTransformList::GetDOMWrapper(
                         GetAnimatedTransformList(DO_ALLOCATE), this).get();
  return NS_OK;
}


NS_IMETHODIMP SVGPatternElement::GetX(nsIDOMSVGAnimatedLength * *aX)
{
  return mLengthAttributes[X].ToDOMAnimatedLength(aX, this);
}


NS_IMETHODIMP SVGPatternElement::GetY(nsIDOMSVGAnimatedLength * *aY)
{
  return mLengthAttributes[Y].ToDOMAnimatedLength(aY, this);
}


NS_IMETHODIMP SVGPatternElement::GetWidth(nsIDOMSVGAnimatedLength * *aWidth)
{
  return mLengthAttributes[WIDTH].ToDOMAnimatedLength(aWidth, this);
}


NS_IMETHODIMP SVGPatternElement::GetHeight(nsIDOMSVGAnimatedLength * *aHeight)
{
  return mLengthAttributes[HEIGHT].ToDOMAnimatedLength(aHeight, this);
}






NS_IMETHODIMP
SVGPatternElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  return mStringAttributes[HREF].ToDOMAnimatedString(aHref, this);
}




NS_IMETHODIMP_(bool)
SVGPatternElement::IsAttributeMapped(const nsIAtom* name) const
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

  return FindAttributeDependence(name, map) ||
    SVGPatternElementBase::IsAttributeMapped(name);
}




SVGAnimatedTransformList*
SVGPatternElement::GetAnimatedTransformList(uint32_t aFlags)
{
  if (!mPatternTransform && (aFlags & DO_ALLOCATE)) {
    mPatternTransform = new SVGAnimatedTransformList();
  }
  return mPatternTransform;
}

 bool
SVGPatternElement::HasValidDimensions() const
{
  return mLengthAttributes[WIDTH].IsExplicitlySet() &&
         mLengthAttributes[WIDTH].GetAnimValInSpecifiedUnits() > 0 &&
         mLengthAttributes[HEIGHT].IsExplicitlySet() &&
         mLengthAttributes[HEIGHT].GetAnimValInSpecifiedUnits() > 0;
}

nsSVGElement::LengthAttributesInfo
SVGPatternElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}

nsSVGElement::EnumAttributesInfo
SVGPatternElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGViewBox *
SVGPatternElement::GetViewBox()
{
  return &mViewBox;
}

SVGAnimatedPreserveAspectRatio *
SVGPatternElement::GetPreserveAspectRatio()
{
  return &mPreserveAspectRatio;
}

nsSVGElement::StringAttributesInfo
SVGPatternElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}

} 
} 
