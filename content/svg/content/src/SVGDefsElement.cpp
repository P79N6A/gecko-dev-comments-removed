




#include "mozilla/dom/SVGDefsElement.h"
#include "mozilla/dom/SVGDefsElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Defs)

namespace mozilla {
namespace dom {

JSObject*
SVGDefsElement::WrapNode(JSContext* aCx, JSObject* aScope)
{
  return SVGDefsElementBinding::Wrap(aCx, aScope, this);
}




NS_IMPL_ISUPPORTS_INHERITED3(SVGDefsElement, SVGGraphicsElement,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement)




SVGDefsElement::SVGDefsElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGGraphicsElement(aNodeInfo)
{
  SetIsDOMBinding();
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGDefsElement)





NS_IMETHODIMP_(bool)
SVGDefsElement::IsAttributeMapped(const nsIAtom* name) const
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

