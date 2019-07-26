




#include "mozilla/dom/SVGGraphicsElement.h"
#include "mozilla/dom/SVGGraphicsElementBinding.h"

namespace mozilla {
namespace dom {

JSObject*
SVGGraphicsElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGGraphicsElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}




NS_IMPL_ADDREF_INHERITED(SVGGraphicsElement, SVGGraphicsElementBase)
NS_IMPL_RELEASE_INHERITED(SVGGraphicsElement, SVGGraphicsElementBase)

NS_INTERFACE_MAP_BEGIN(SVGGraphicsElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGTests)
NS_INTERFACE_MAP_END_INHERITING(SVGGraphicsElementBase)




SVGGraphicsElement::SVGGraphicsElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGGraphicsElementBase(aNodeInfo)
{
}

} 
} 
