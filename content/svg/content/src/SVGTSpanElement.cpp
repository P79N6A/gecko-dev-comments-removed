




#include "mozilla/dom/SVGTSpanElement.h"
#include "mozilla/dom/SVGTSpanElementBinding.h"
#include "nsContentUtils.h"

DOMCI_NODE_DATA(SVGTSpanElement, mozilla::dom::SVGTSpanElement)

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(TSpan)

namespace mozilla {
namespace dom {

JSObject*
SVGTSpanElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGTSpanElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}





NS_IMPL_ADDREF_INHERITED(SVGTSpanElement,SVGTSpanElementBase)
NS_IMPL_RELEASE_INHERITED(SVGTSpanElement,SVGTSpanElementBase)

NS_INTERFACE_TABLE_HEAD(SVGTSpanElement)
  NS_NODE_INTERFACE_TABLE6(SVGTSpanElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGTSpanElement,
                           nsIDOMSVGTextPositioningElement,
                           nsIDOMSVGTextContentElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGTSpanElement)
NS_INTERFACE_MAP_END_INHERITING(SVGTSpanElementBase)




SVGTSpanElement::SVGTSpanElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGTSpanElementBase(aNodeInfo)
{
  SetIsDOMBinding();
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
