





#include "mozilla/dom/SVGAltGlyphElement.h"
#include "mozilla/dom/SVGAltGlyphElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(AltGlyph)

namespace mozilla {
namespace dom {

JSObject*
SVGAltGlyphElement::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto)
{
  return SVGAltGlyphElementBinding::Wrap(aCx, this, aGivenProto);
}

nsSVGElement::StringInfo SVGAltGlyphElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink, false }
};





SVGAltGlyphElement::SVGAltGlyphElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGAltGlyphElementBase(aNodeInfo)
{
}

nsSVGElement::EnumAttributesInfo
SVGAltGlyphElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGElement::LengthAttributesInfo
SVGAltGlyphElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGAltGlyphElement)

already_AddRefed<SVGAnimatedString>
SVGAltGlyphElement::Href()
{
  return mStringAttributes[HREF].ToDOMAnimatedString(this);
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




nsSVGElement::StringAttributesInfo
SVGAltGlyphElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}

} 
} 
