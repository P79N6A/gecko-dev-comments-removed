




#include "mozilla/dom/SVGSymbolElement.h"
#include "mozilla/dom/SVGSymbolElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Symbol)

DOMCI_NODE_DATA(SVGSymbolElement, mozilla::dom::SVGSymbolElement)

namespace mozilla {
namespace dom {

JSObject*
SVGSymbolElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGSymbolElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}




NS_IMPL_ADDREF_INHERITED(SVGSymbolElement,SVGSymbolElementBase)
NS_IMPL_RELEASE_INHERITED(SVGSymbolElement,SVGSymbolElementBase)

NS_INTERFACE_TABLE_HEAD(SVGSymbolElement)
  NS_NODE_INTERFACE_TABLE5(SVGSymbolElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGFitToViewBox,
                           nsIDOMSVGSymbolElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGSymbolElement)
NS_INTERFACE_MAP_END_INHERITING(SVGSymbolElementBase)




SVGSymbolElement::SVGSymbolElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGSymbolElementBase(aNodeInfo)
{
  SetIsDOMBinding();
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGSymbolElement)





NS_IMETHODIMP SVGSymbolElement::GetViewBox(nsIDOMSVGAnimatedRect * *aViewBox)
{
  *aViewBox = ViewBox().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGAnimatedRect>
SVGSymbolElement::ViewBox()
{
  nsCOMPtr<nsIDOMSVGAnimatedRect> rect;
  mViewBox.ToDOMAnimatedRect(getter_AddRefs(rect), this);
  return rect.forget();
}


NS_IMETHODIMP
SVGSymbolElement::GetPreserveAspectRatio(nsISupports
                                         **aPreserveAspectRatio)
{
  *aPreserveAspectRatio = PreserveAspectRatio().get();
  return NS_OK;
}

already_AddRefed<DOMSVGAnimatedPreserveAspectRatio>
SVGSymbolElement::PreserveAspectRatio()
{
  nsRefPtr<DOMSVGAnimatedPreserveAspectRatio> ratio;
  mPreserveAspectRatio.ToDOMAnimatedPreserveAspectRatio(getter_AddRefs(ratio), this);
  return ratio.forget();
}




NS_IMETHODIMP_(bool)
SVGSymbolElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sColorMap,
    sFEFloodMap,
    sFillStrokeMap,
    sFiltersMap,
    sFontSpecificationMap,
    sGradientStopMap,
    sGraphicsMap,
    sLightingEffectsMap,
    sMarkersMap,
    sTextContentElementsMap,
    sViewportsMap
   };

  return FindAttributeDependence(name, map) ||
    SVGSymbolElementBase::IsAttributeMapped(name);
}




nsSVGViewBox *
SVGSymbolElement::GetViewBox()
{
  return &mViewBox;
}

SVGAnimatedPreserveAspectRatio *
SVGSymbolElement::GetPreserveAspectRatio()
{
  return &mPreserveAspectRatio;
}

} 
} 
