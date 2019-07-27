




#include "mozilla/dom/SVGSymbolElement.h"
#include "mozilla/dom/SVGSymbolElementBinding.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Symbol)

namespace mozilla {
namespace dom {

JSObject*
SVGSymbolElement::WrapNode(JSContext *aCx)
{
  return SVGSymbolElementBinding::Wrap(aCx, this);
}




NS_IMPL_ISUPPORTS_INHERITED(SVGSymbolElement, SVGSymbolElementBase,
                            nsIDOMNode, nsIDOMElement,
                            nsIDOMSVGElement, mozilla::dom::SVGTests)




SVGSymbolElement::SVGSymbolElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGSymbolElementBase(aNodeInfo)
{
}

SVGSymbolElement::~SVGSymbolElement()
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGSymbolElement)



already_AddRefed<SVGAnimatedRect>
SVGSymbolElement::ViewBox()
{
  return mViewBox.ToSVGAnimatedRect(this);
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
