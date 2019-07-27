




#include "mozilla/dom/SVGGElement.h"
#include "mozilla/dom/SVGGElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(G)

namespace mozilla {
namespace dom {

JSObject*
SVGGElement::WrapNode(JSContext *aCx)
{
  return SVGGElementBinding::Wrap(aCx, this);
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

