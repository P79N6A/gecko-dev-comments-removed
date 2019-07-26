




#include "mozilla/dom/SVGAltGlyphElement.h"
#include "mozilla/dom/SVGAltGlyphElementBinding.h"
#include "nsContentUtils.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(AltGlyph)

namespace mozilla {
namespace dom {

JSObject*
SVGAltGlyphElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGAltGlyphElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

nsSVGElement::StringInfo SVGAltGlyphElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink, false }
};





NS_IMPL_ISUPPORTS_INHERITED6(SVGAltGlyphElement, SVGAltGlyphElementBase,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement,
                             nsIDOMSVGTextPositioningElement,
                             nsIDOMSVGTextContentElement,
                             nsIDOMSVGURIReference)




SVGAltGlyphElement::SVGAltGlyphElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGAltGlyphElementBase(aNodeInfo)
{
  SetIsDOMBinding();
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGAltGlyphElement)





NS_IMETHODIMP SVGAltGlyphElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  *aHref = Href().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGAnimatedString>
SVGAltGlyphElement::Href()
{
  nsCOMPtr<nsIDOMSVGAnimatedString> href;
  mStringAttributes[HREF].ToDOMAnimatedString(getter_AddRefs(href), this);
  return href.forget();
}

void
SVGAltGlyphElement::GetGlyphRef(nsAString & aGlyphRef)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::glyphRef, aGlyphRef);
}

void
SVGAltGlyphElement::SetGlyphRef(const nsAString & aGlyphRef, ErrorResult& rv)
{
  rv = SetAttr(kNameSpaceID_None, nsGkAtoms::glyphRef, aGlyphRef, true);
}

void
SVGAltGlyphElement::GetFormat(nsAString & aFormat)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::format, aFormat);
}

void
SVGAltGlyphElement::SetFormat(const nsAString & aFormat, ErrorResult& rv)
{
  rv = SetAttr(kNameSpaceID_None, nsGkAtoms::format, aFormat, true);
}




NS_IMETHODIMP_(bool)
SVGAltGlyphElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sColorMap,
    sFillStrokeMap,
    sFontSpecificationMap,
    sGraphicsMap,
    sTextContentElementsMap
  };

  return FindAttributeDependence(name, map) ||
    SVGAltGlyphElementBase::IsAttributeMapped(name);
}




bool
SVGAltGlyphElement::IsEventName(nsIAtom* aName)
{
  return nsContentUtils::IsEventAttributeName(aName, EventNameType_SVGGraphic);
}

nsSVGElement::StringAttributesInfo
SVGAltGlyphElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}

} 
} 
