




#include "mozilla/dom/SVGTSpanElement.h"
#include "mozilla/dom/SVGTSpanElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(TSpan)

namespace mozilla {
namespace dom {

JSObject*
SVGTSpanElement::WrapNode(JSContext *aCx)
{
  return SVGTSpanElementBinding::Wrap(aCx, this);
}





SVGTSpanElement::SVGTSpanElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGTSpanElementBase(aNodeInfo)
{
}

nsSVGElement::EnumAttributesInfo
SVGTSpanElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGElement::LengthAttributesInfo
SVGTSpanElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGTSpanElement)




NS_IMETHODIMP_(bool)
SVGTSpanElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sColorMap,
    sFillStrokeMap,
    sFontSpecificationMap,
    sGraphicsMap,
    sTextContentElementsMap
  };

  return FindAttributeDependence(name, map) ||
    SVGTSpanElementBase::IsAttributeMapped(name);
}

} 
} 
