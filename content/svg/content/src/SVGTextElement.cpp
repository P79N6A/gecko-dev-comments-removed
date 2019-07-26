




#include "mozilla/dom/SVGTextElement.h"
#include "mozilla/dom/SVGTextElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Text)

namespace mozilla {
namespace dom {

JSObject*
SVGTextElement::WrapNode(JSContext *aCx)
{
  return SVGTextElementBinding::Wrap(aCx, this);
}




SVGTextElement::SVGTextElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGTextElementBase(aNodeInfo)
{
}

nsSVGElement::EnumAttributesInfo
SVGTextElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGElement::LengthAttributesInfo
SVGTextElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGTextElement)





NS_IMETHODIMP_(bool)
SVGTextElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sTextContentElementsMap,
    sFontSpecificationMap
  };

  return FindAttributeDependence(name, map) ||
    SVGTextElementBase::IsAttributeMapped(name);
}

} 
} 
