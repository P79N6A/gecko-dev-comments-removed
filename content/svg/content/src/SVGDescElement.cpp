




#include "mozilla/dom/SVGDescElement.h"
#include "mozilla/dom/SVGDescElementBinding.h"

DOMCI_NODE_DATA(SVGDescElement, mozilla::dom::SVGDescElement)

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Desc)

namespace mozilla {
namespace dom {

JSObject*
SVGDescElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGDescElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}




NS_IMPL_ADDREF_INHERITED(SVGDescElement, SVGDescElementBase)
NS_IMPL_RELEASE_INHERITED(SVGDescElement, SVGDescElementBase)

NS_INTERFACE_TABLE_HEAD(SVGDescElement)
  NS_NODE_INTERFACE_TABLE4(SVGDescElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGDescElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGDescElement)
NS_INTERFACE_MAP_END_INHERITING(SVGDescElementBase)




SVGDescElement::SVGDescElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGDescElementBase(aNodeInfo)
{
  SetIsDOMBinding();
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGDescElement)

} 
} 

