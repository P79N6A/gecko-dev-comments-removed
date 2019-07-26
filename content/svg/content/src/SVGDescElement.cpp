




#include "mozilla/dom/SVGDescElement.h"
#include "mozilla/dom/SVGDescElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Desc)

namespace mozilla {
namespace dom {

JSObject*
SVGDescElement::WrapNode(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return SVGDescElementBinding::Wrap(aCx, aScope, this);
}




SVGDescElement::SVGDescElement(already_AddRefed<nsINodeInfo>& aNodeInfo)
  : SVGDescElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGDescElement)

} 
} 

