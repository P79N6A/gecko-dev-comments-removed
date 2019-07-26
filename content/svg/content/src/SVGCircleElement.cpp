




#include "mozilla/dom/SVGCircleElement.h"
#include "nsGkAtoms.h"
#include "gfxContext.h"
#include "mozilla/dom/SVGCircleElementBinding.h"

DOMCI_NODE_DATA(SVGCircleElement, mozilla::dom::SVGCircleElement)

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Circle)

namespace mozilla {
namespace dom {

JSObject*
SVGCircleElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGCircleElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

nsSVGElement::LengthInfo SVGCircleElement::sLengthInfo[3] =
{
  { &nsGkAtoms::cx, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::X },
  { &nsGkAtoms::cy, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::Y },
  { &nsGkAtoms::r, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::XY }
};




NS_IMPL_ADDREF_INHERITED(SVGCircleElement,SVGCircleElementBase)
NS_IMPL_RELEASE_INHERITED(SVGCircleElement,SVGCircleElementBase)

NS_INTERFACE_TABLE_HEAD(SVGCircleElement)
  NS_NODE_INTERFACE_TABLE4(SVGCircleElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGCircleElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGCircleElement)
NS_INTERFACE_MAP_END_INHERITING(SVGCircleElementBase)




SVGCircleElement::SVGCircleElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGCircleElementBase(aNodeInfo)
{
  SetIsDOMBinding();
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGCircleElement)





NS_IMETHODIMP SVGCircleElement::GetCx(nsIDOMSVGAnimatedLength * *aCx)
{
  *aCx = Cx().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGAnimatedLength>
SVGCircleElement::Cx()
{
  return mLengthAttributes[ATTR_CX].ToDOMAnimatedLength(this);
}


NS_IMETHODIMP SVGCircleElement::GetCy(nsIDOMSVGAnimatedLength * *aCy)
{
  *aCy = Cy().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGAnimatedLength>
SVGCircleElement::Cy()
{
  return mLengthAttributes[ATTR_CY].ToDOMAnimatedLength(this);
}


NS_IMETHODIMP SVGCircleElement::GetR(nsIDOMSVGAnimatedLength * *aR)
{
  *aR = R().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGAnimatedLength>
SVGCircleElement::R()
{
  return mLengthAttributes[ATTR_R].ToDOMAnimatedLength(this);
}




 bool
SVGCircleElement::HasValidDimensions() const
{
  return mLengthAttributes[ATTR_R].IsExplicitlySet() &&
         mLengthAttributes[ATTR_R].GetAnimValInSpecifiedUnits() > 0;
}

nsSVGElement::LengthAttributesInfo
SVGCircleElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}




void
SVGCircleElement::ConstructPath(gfxContext *aCtx)
{
  float x, y, r;

  GetAnimatedLengthValues(&x, &y, &r, nullptr);

  if (r > 0.0f)
    aCtx->Arc(gfxPoint(x, y), r, 0, 2*M_PI);
}

} 
} 
