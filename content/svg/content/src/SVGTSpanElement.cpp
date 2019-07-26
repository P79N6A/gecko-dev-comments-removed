




#include "mozilla/dom/SVGTSpanElement.h"
#include "mozilla/dom/SVGTSpanElementBinding.h"
#include "nsContentUtils.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(TSpan)

namespace mozilla {
namespace dom {

JSObject*
SVGTSpanElement::WrapNode(JSContext *aCx, JSObject *aScope)
{
  return SVGTSpanElementBinding::Wrap(aCx, aScope, this);
}





NS_IMPL_ISUPPORTS_INHERITED3(SVGTSpanElement, SVGTSpanElementBase,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement)




SVGTSpanElement::SVGTSpanElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGTSpanElementBase(aNodeInfo)
{
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




bool
SVGTSpanElement::IsEventName(nsIAtom* aName)
{
  return nsContentUtils::IsEventAttributeName(aName, EventNameType_SVGGraphic);
}

} 
} 
