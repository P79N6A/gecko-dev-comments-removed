




#include "mozilla/dom/SVGAnimatedLength.h"
#include "mozilla/dom/SVGEllipseElement.h"
#include "mozilla/dom/SVGEllipseElementBinding.h"
#include "gfxContext.h"

DOMCI_NODE_DATA(SVGEllipseElement, mozilla::dom::SVGEllipseElement)

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Ellipse)

namespace mozilla {
namespace dom {

JSObject*
SVGEllipseElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGEllipseElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

nsSVGElement::LengthInfo SVGEllipseElement::sLengthInfo[4] =
{
  { &nsGkAtoms::cx, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::X },
  { &nsGkAtoms::cy, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::Y },
  { &nsGkAtoms::rx, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::X },
  { &nsGkAtoms::ry, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::Y },
};




NS_IMPL_ADDREF_INHERITED(SVGEllipseElement,SVGEllipseElementBase)
NS_IMPL_RELEASE_INHERITED(SVGEllipseElement,SVGEllipseElementBase)

NS_INTERFACE_TABLE_HEAD(SVGEllipseElement)
  NS_NODE_INTERFACE_TABLE4(SVGEllipseElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGEllipseElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGEllipseElement)
NS_INTERFACE_MAP_END_INHERITING(SVGEllipseElementBase)




SVGEllipseElement::SVGEllipseElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGEllipseElementBase(aNodeInfo)
{
  SetIsDOMBinding();
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGEllipseElement)





NS_IMETHODIMP SVGEllipseElement::GetCx(nsIDOMSVGAnimatedLength * *aCx)
{
  *aCx = Cx().get();
  return NS_OK;
}

already_AddRefed<SVGAnimatedLength>
SVGEllipseElement::Cx()
{
  return mLengthAttributes[CX].ToDOMAnimatedLength(this);
}


NS_IMETHODIMP SVGEllipseElement::GetCy(nsIDOMSVGAnimatedLength * *aCy)
{
  *aCy = Cy().get();
  return NS_OK;
}

already_AddRefed<SVGAnimatedLength>
SVGEllipseElement::Cy()
{
  return mLengthAttributes[CY].ToDOMAnimatedLength(this);
}


NS_IMETHODIMP SVGEllipseElement::GetRx(nsIDOMSVGAnimatedLength * *aRx)
{
  *aRx = Rx().get();
  return NS_OK;
}

already_AddRefed<SVGAnimatedLength>
SVGEllipseElement::Rx()
{
  return mLengthAttributes[RX].ToDOMAnimatedLength(this);
}


NS_IMETHODIMP SVGEllipseElement::GetRy(nsIDOMSVGAnimatedLength * *aRy)
{
  *aRy = Ry().get();
  return NS_OK;
}

already_AddRefed<SVGAnimatedLength>
SVGEllipseElement::Ry()
{
  return mLengthAttributes[RY].ToDOMAnimatedLength(this);
}




 bool
SVGEllipseElement::HasValidDimensions() const
{
  return mLengthAttributes[RX].IsExplicitlySet() &&
         mLengthAttributes[RX].GetAnimValInSpecifiedUnits() > 0 &&
         mLengthAttributes[RY].IsExplicitlySet() &&
         mLengthAttributes[RY].GetAnimValInSpecifiedUnits() > 0;
}

nsSVGElement::LengthAttributesInfo
SVGEllipseElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}




void
SVGEllipseElement::ConstructPath(gfxContext *aCtx)
{
  float x, y, rx, ry;

  GetAnimatedLengthValues(&x, &y, &rx, &ry, nullptr);

  if (rx > 0.0f && ry > 0.0f) {
    aCtx->Save();
    aCtx->Translate(gfxPoint(x, y));
    aCtx->Scale(rx, ry);
    aCtx->Arc(gfxPoint(0, 0), 1, 0, 2 * M_PI);
    aCtx->Restore();
  }
}

} 
} 
