




#include "mozilla/dom/SVGDefsElement.h"
#include "mozilla/dom/SVGDefsElementBinding.h"

DOMCI_NODE_DATA(SVGDefsElement, mozilla::dom::SVGDefsElement)

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Defs)

namespace mozilla {
namespace dom {

JSObject*
SVGDefsElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGDefsElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}




NS_IMPL_ADDREF_INHERITED(SVGDefsElement, SVGGraphicsElement)
NS_IMPL_RELEASE_INHERITED(SVGDefsElement, SVGGraphicsElement)

NS_INTERFACE_TABLE_HEAD(SVGDefsElement)
  NS_NODE_INTERFACE_TABLE4(SVGDefsElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGDefsElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGDefsElement)
NS_INTERFACE_MAP_END_INHERITING(SVGGraphicsElement)




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

