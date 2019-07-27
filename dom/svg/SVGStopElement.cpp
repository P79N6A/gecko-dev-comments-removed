




#include "mozilla/dom/SVGStopElement.h"
#include "mozilla/dom/SVGStopElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Stop)

namespace mozilla {
namespace dom {

JSObject*
SVGStopElement::WrapNode(JSContext *aCx)
{
  return SVGStopElementBinding::Wrap(aCx, this);
}

nsSVGElement::NumberInfo SVGStopElement::sNumberInfo =
{ &nsGkAtoms::offset, 0, true };




SVGStopElement::SVGStopElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGStopElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGStopElement)



already_AddRefed<SVGAnimatedNumber>
SVGStopElement::Offset()
{
  return mOffset.ToDOMAnimatedNumber(this);
}




nsSVGElement::NumberAttributesInfo
SVGStopElement::GetNumberInfo()
{
  return NumberAttributesInfo(&mOffset, &sNumberInfo, 1);
}




NS_IMETHODIMP_(bool)
SVGStopElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sGradientStopMap
  };

  return FindAttributeDependence(name, map) ||
    SVGStopElementBase::IsAttributeMapped(name);
}

} 
} 

