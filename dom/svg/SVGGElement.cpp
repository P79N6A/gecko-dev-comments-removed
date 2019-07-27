




#include "mozilla/dom/SVGGElement.h"
#include "mozilla/dom/SVGGElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(G)

namespace mozilla {
namespace dom {

JSObject*
SVGGElement::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto)
{
  return SVGGElementBinding::Wrap(aCx, this, aGivenProto);
}




SVGGElement::SVGGElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGGraphicsElement(aNodeInfo)
{
}






NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGGElement)





NS_IMETHODIMP_(bool)
SVGGElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sFEFloodMap,
    sFiltersMap,
    sFontSpecificationMap,
    sGradientStopMap,
    sLightingEffectsMap,
    sMarkersMap,
    sTextContentElementsMap,
    sViewportsMap
  };

  return FindAttributeDependence(name, map) ||
    SVGGraphicsElement::IsAttributeMapped(name);
}

} 
} 

