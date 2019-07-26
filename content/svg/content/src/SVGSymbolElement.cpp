




#include "mozilla/dom/SVGSymbolElement.h"
#include "mozilla/dom/SVGSymbolElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Symbol)

namespace mozilla {
namespace dom {

JSObject*
SVGSymbolElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGSymbolElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}




NS_IMPL_ISUPPORTS_INHERITED4(SVGSymbolElement, SVGSymbolElementBase,
                             nsIDOMNode, nsIDOMElement,
                             nsIDOMSVGElement, mozilla::dom::SVGTests)




SVGSymbolElement::SVGSymbolElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGSymbolElementBase(aNodeInfo)
{
  SetIsDOMBinding();
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGSymbolElement)



already_AddRefed<nsIDOMSVGAnimatedRect>
SVGSymbolElement::ViewBox()
{
  nsCOMPtr<nsIDOMSVGAnimatedRect> rect;
  mViewBox.ToDOMAnimatedRect(getter_AddRefs(rect), this);
  return rect.forget();
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
