




#include "mozilla/dom/SVGStopElement.h"
#include "mozilla/dom/SVGStopElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Stop)

namespace mozilla {
namespace dom {

JSObject*
SVGStopElement::WrapNode(JSContext *aCx, JSObject *aScope)
{
  return SVGStopElementBinding::Wrap(aCx, aScope, this);
}

nsSVGElement::NumberInfo SVGStopElement::sNumberInfo =
{ &nsGkAtoms::offset, 0, true };




NS_IMPL_ISUPPORTS_INHERITED3(SVGStopElement, SVGStopElementBase,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement)




SVGStopElement::SVGStopElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGStopElementBase(aNodeInfo)
{
  SetIsDOMBinding();
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGStopElement)



already_AddRefed<nsIDOMSVGAnimatedNumber>
SVGStopElement::Offset()
{
  nsCOMPtr<nsIDOMSVGAnimatedNumber> offset;
  mOffset.ToDOMAnimatedNumber(getter_AddRefs(offset), this);
  return offset.forget();
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

