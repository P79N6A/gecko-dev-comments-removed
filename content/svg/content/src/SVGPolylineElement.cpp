




#include "mozilla/dom/SVGPolylineElement.h"
#include "mozilla/dom/SVGPolylineElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Polyline)

namespace mozilla {
namespace dom {

JSObject*
SVGPolylineElement::WrapNode(JSContext *aCx)
{
  return SVGPolylineElementBinding::Wrap(aCx, this);
}




SVGPolylineElement::SVGPolylineElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGPolylineElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGPolylineElement)

} 
} 
