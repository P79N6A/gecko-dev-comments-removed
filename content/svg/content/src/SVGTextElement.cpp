




#include "mozilla/dom/SVGTextElement.h"
#include "mozilla/dom/SVGTextElementBinding.h"

DOMCI_NODE_DATA(SVGTextElement, mozilla::dom::SVGTextElement)

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Text)

namespace mozilla {
namespace dom {

JSObject*
SVGTextElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGTextElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}




NS_IMPL_ADDREF_INHERITED(SVGTextElement,SVGTextElementBase)
NS_IMPL_RELEASE_INHERITED(SVGTextElement,SVGTextElementBase)

NS_INTERFACE_TABLE_HEAD(SVGTextElement)
  NS_NODE_INTERFACE_TABLE6(SVGTextElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGTextElement,
                           nsIDOMSVGTextPositioningElement,
                           nsIDOMSVGTextContentElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGTextElement)
NS_INTERFACE_MAP_END_INHERITING(SVGTextElementBase)




SVGTextElement::SVGTextElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGTextElementBase(aNodeInfo)
{
  SetIsDOMBinding();
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
