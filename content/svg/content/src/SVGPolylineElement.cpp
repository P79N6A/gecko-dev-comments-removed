




#include "mozilla/dom/SVGPolylineElement.h"
#include "mozilla/dom/SVGPolylineElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Polyline)

namespace mozilla {
namespace dom {

JSObject*
SVGPolylineElement::WrapNode(JSContext *aCx, JSObject *aScope)
{
  return SVGPolylineElementBinding::Wrap(aCx, aScope, this);
}




SVGPolylineElement::SVGPolylineElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGPolylineElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGPolylineElement)

} 
} 
