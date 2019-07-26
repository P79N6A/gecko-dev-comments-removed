




#include "mozilla/dom/SVGDescElement.h"
#include "mozilla/dom/SVGDescElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Desc)

namespace mozilla {
namespace dom {

JSObject*
SVGDescElement::WrapNode(JSContext *aCx, JSObject *aScope)
{
  return SVGDescElementBinding::Wrap(aCx, aScope, this);
}




NS_IMPL_ISUPPORTS_INHERITED3(SVGDescElement, SVGDescElementBase,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement)




SVGDescElement::SVGDescElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGDescElementBase(aNodeInfo)
{
  SetIsDOMBinding();
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGDescElement)

} 
} 

