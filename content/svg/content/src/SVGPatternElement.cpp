




#include "mozilla/Util.h"

#include "DOMSVGAnimatedTransformList.h"
#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "mozilla/dom/SVGPatternElement.h"
#include "mozilla/dom/SVGPatternElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Pattern)

namespace mozilla {
namespace dom {

JSObject*
SVGPatternElement::WrapNode(JSContext *aCx, JSObject *aScope)
{
  return SVGPatternElementBinding::Wrap(aCx, aScope, this);
}



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
    SVG_UNIT_TYPE_OBJECTBOUNDINGBOX
  },
  { &nsGkAtoms::patternContentUnits,
    sSVGUnitTypesMap,
    SVG_UNIT_TYPE_USERSPACEONUSE
  }
};

nsSVGElement::StringInfo SVGPatternElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink, true }
};




NS_IMPL_ISUPPORTS_INHERITED3(SVGPatternElement, SVGPatternElementBase,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement)




SVGPatternElement::SVGPatternElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGPatternElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGPatternElement)



already_AddRefed<nsIDOMSVGAnimatedRect>
SVGPatternElement::ViewBox()
{
  nsCOMPtr<nsIDOMSVGAnimatedRect> rect;
  mViewBox.ToDOMAnimatedRect(getter_AddRefs(rect), this);
  return rect.forget();
}

already_AddRefed<DOMSVGAnimatedPreserveAspectRatio>
SVGPatternElement::PreserveAspectRatio()
{
  nsRefPtr<DOMSVGAnimatedPreserveAspectRatio> ratio;
  mPreserveAspectRatio.ToDOMAnimatedPreserveAspectRatio(getter_AddRefs(ratio), this);
  return ratio.forget();
}



already_AddRefed<nsIDOMSVGAnimatedEnumeration>
SVGPatternElement::PatternUnits()
{
  return mEnumAttributes[PATTERNUNITS].ToDOMAnimatedEnum(this);
}

already_AddRefed<nsIDOMSVGAnimatedEnumeration>
SVGPatternElement::PatternContentUnits()
{
  return mEnumAttributes[PATTERNCONTENTUNITS].ToDOMAnimatedEnum(this);
}

already_AddRefed<DOMSVGAnimatedTransformList>
SVGPatternElement::PatternTransform()
{
  
  
  return DOMSVGAnimatedTransformList::GetDOMWrapper(
           GetAnimatedTransformList(DO_ALLOCATE), this);
}

already_AddRefed<SVGAnimatedLength>
SVGPatternElement::X()
{
  return mLengthAttributes[ATTR_X].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGPatternElement::Y()
{
  return mLengthAttributes[ATTR_Y].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGPatternElement::Width()
{
  return mLengthAttributes[ATTR_WIDTH].ToDOMAnimatedLength(this);
}

already_AddRefed<SVGAnimatedLength>
SVGPatternElement::Height()
{
  return mLengthAttributes[ATTR_HEIGHT].ToDOMAnimatedLength(this);
}

already_AddRefed<nsIDOMSVGAnimatedString>
SVGPatternElement::Href()
{
  return mStringAttributes[HREF].ToDOMAnimatedString(this);
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
  return mLengthAttributes[ATTR_WIDTH].IsExplicitlySet() &&
         mLengthAttributes[ATTR_WIDTH].GetAnimValInSpecifiedUnits() > 0 &&
         mLengthAttributes[ATTR_HEIGHT].IsExplicitlySet() &&
         mLengthAttributes[ATTR_HEIGHT].GetAnimValInSpecifiedUnits() > 0;
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
