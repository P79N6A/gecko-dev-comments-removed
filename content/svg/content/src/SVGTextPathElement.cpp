




#include "mozilla/dom/SVGTextPathElement.h"
#include "mozilla/dom/SVGTextPathElementBinding.h"
#include "nsSVGElement.h"
#include "nsGkAtoms.h"
#include "nsIDOMSVGURIReference.h"
#include "nsIFrame.h"
#include "nsError.h"
#include "nsContentUtils.h"
#include "mozilla/dom/SVGAnimatedLength.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(TextPath)

namespace mozilla {
namespace dom {

JSObject*
SVGTextPathElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGTextPathElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

nsSVGElement::LengthInfo SVGTextPathElement::sLengthInfo[1] =
{
  { &nsGkAtoms::startOffset, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::X },
};

nsSVGEnumMapping SVGTextPathElement::sMethodMap[] = {
  {&nsGkAtoms::align, TEXTPATH_METHODTYPE_ALIGN},
  {&nsGkAtoms::stretch, TEXTPATH_METHODTYPE_STRETCH},
  {nullptr, 0}
};

nsSVGEnumMapping SVGTextPathElement::sSpacingMap[] = {
  {&nsGkAtoms::_auto, TEXTPATH_SPACINGTYPE_AUTO},
  {&nsGkAtoms::exact, TEXTPATH_SPACINGTYPE_EXACT},
  {nullptr, 0}
};

nsSVGElement::EnumInfo SVGTextPathElement::sEnumInfo[2] =
{
  { &nsGkAtoms::method,
    sMethodMap,
    TEXTPATH_METHODTYPE_ALIGN
  },
  { &nsGkAtoms::spacing,
    sSpacingMap,
    TEXTPATH_SPACINGTYPE_EXACT
  }
};

nsSVGElement::StringInfo SVGTextPathElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink, true }
};




NS_IMPL_ISUPPORTS_INHERITED3(SVGTextPathElement, SVGTextPathElementBase,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement)




SVGTextPathElement::SVGTextPathElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGTextPathElementBase(aNodeInfo)
{
  SetIsDOMBinding();
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGTextPathElement)

already_AddRefed<nsIDOMSVGAnimatedString>
SVGTextPathElement::Href()
{
  return mStringAttributes[HREF].ToDOMAnimatedString(this);
}



already_AddRefed<SVGAnimatedLength>
SVGTextPathElement::StartOffset()
{
  return mLengthAttributes[STARTOFFSET].ToDOMAnimatedLength(this);
}

already_AddRefed<nsIDOMSVGAnimatedEnumeration>
SVGTextPathElement::Method()
{
  return mEnumAttributes[METHOD].ToDOMAnimatedEnum(this);
}

already_AddRefed<nsIDOMSVGAnimatedEnumeration>
SVGTextPathElement::Spacing()
{
  return mEnumAttributes[SPACING].ToDOMAnimatedEnum(this);
}




NS_IMETHODIMP_(bool)
SVGTextPathElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sColorMap,
    sFillStrokeMap,
    sFontSpecificationMap,
    sGraphicsMap,
    sTextContentElementsMap
  };

  return FindAttributeDependence(name, map) ||
    SVGTextPathElementBase::IsAttributeMapped(name);
}


bool
SVGTextPathElement::IsEventAttributeName(nsIAtom* aName)
{
  return nsContentUtils::IsEventAttributeName(aName, EventNameType_SVGGraphic);
}




nsSVGElement::LengthAttributesInfo
SVGTextPathElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}

nsSVGElement::EnumAttributesInfo
SVGTextPathElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGElement::StringAttributesInfo
SVGTextPathElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}

} 
} 
