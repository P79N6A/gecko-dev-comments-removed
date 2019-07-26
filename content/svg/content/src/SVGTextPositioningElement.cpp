




#include "mozilla/Util.h"

#include "mozilla/dom/SVGTextPositioningElement.h"
#include "mozilla/dom/SVGTextPositioningElementBinding.h"
#include "SVGAnimatedLengthList.h"
#include "DOMSVGAnimatedLengthList.h"
#include "DOMSVGAnimatedNumberList.h"
#include "SVGContentUtils.h"
#include "SVGLengthList.h"

namespace mozilla {
namespace dom {

JSObject*
SVGTextPositioningElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGTextPositioningElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

nsSVGElement::LengthListInfo SVGTextPositioningElement::sLengthListInfo[4] =
{
  { &nsGkAtoms::x,  SVGContentUtils::X, false },
  { &nsGkAtoms::y,  SVGContentUtils::Y, false },
  { &nsGkAtoms::dx, SVGContentUtils::X, true },
  { &nsGkAtoms::dy, SVGContentUtils::Y, true }
};

nsSVGElement::LengthListAttributesInfo
SVGTextPositioningElement::GetLengthListInfo()
{
  return LengthListAttributesInfo(mLengthListAttributes, sLengthListInfo,
                                  ArrayLength(sLengthListInfo));
}


nsSVGElement::NumberListInfo SVGTextPositioningElement::sNumberListInfo[1] =
{
  { &nsGkAtoms::rotate }
};

nsSVGElement::NumberListAttributesInfo
SVGTextPositioningElement::GetNumberListInfo()
{
  return NumberListAttributesInfo(mNumberListAttributes, sNumberListInfo,
                                  ArrayLength(sNumberListInfo));
}





NS_IMETHODIMP SVGTextPositioningElement::GetX(nsISupports * *aX)
{
  *aX = X().get();
  return NS_OK;
}

already_AddRefed<DOMSVGAnimatedLengthList>
SVGTextPositioningElement::X()
{
  return DOMSVGAnimatedLengthList::GetDOMWrapper(&mLengthListAttributes[ATTR_X],
                                                 this, ATTR_X, SVGContentUtils::X);
}


NS_IMETHODIMP SVGTextPositioningElement::GetY(nsISupports * *aY)
{
  *aY = Y().get();
  return NS_OK;
}

already_AddRefed<DOMSVGAnimatedLengthList>
SVGTextPositioningElement::Y()
{
  return DOMSVGAnimatedLengthList::GetDOMWrapper(&mLengthListAttributes[ATTR_Y],
                                                 this, ATTR_Y, SVGContentUtils::Y);
}


NS_IMETHODIMP SVGTextPositioningElement::GetDx(nsISupports * *aDx)
{
  *aDx = Dx().get();
  return NS_OK;
}

already_AddRefed<DOMSVGAnimatedLengthList>
SVGTextPositioningElement::Dx()
{
  return DOMSVGAnimatedLengthList::GetDOMWrapper(&mLengthListAttributes[ATTR_DX],
                                                 this, ATTR_DX, SVGContentUtils::X);
}


NS_IMETHODIMP SVGTextPositioningElement::GetDy(nsISupports * *aDy)
{
  *aDy = Dy().get();
  return NS_OK;
}

already_AddRefed<DOMSVGAnimatedLengthList>
SVGTextPositioningElement::Dy()
{
  return DOMSVGAnimatedLengthList::GetDOMWrapper(&mLengthListAttributes[ATTR_DY],
                                                 this, ATTR_DY, SVGContentUtils::Y);
}


NS_IMETHODIMP SVGTextPositioningElement::GetRotate(nsISupports * *aRotate)
{
  *aRotate = Rotate().get();
  return NS_OK;
}

already_AddRefed<DOMSVGAnimatedNumberList>
SVGTextPositioningElement::Rotate()
{
  return DOMSVGAnimatedNumberList::GetDOMWrapper(&mNumberListAttributes[ROTATE],
                                                 this, ROTATE);
}

} 
} 
