


































#include "mozilla/Util.h"

#include "nsSVGTextPositioningElement.h"
#include "SVGAnimatedLengthList.h"
#include "DOMSVGAnimatedLengthList.h"
#include "SVGLengthList.h"
#include "DOMSVGAnimatedNumberList.h"

using namespace mozilla;


nsSVGElement::LengthListInfo nsSVGTextPositioningElement::sLengthListInfo[4] =
{
  { &nsGkAtoms::x,  nsSVGUtils::X, PR_FALSE },
  { &nsGkAtoms::y,  nsSVGUtils::Y, PR_FALSE },
  { &nsGkAtoms::dx, nsSVGUtils::X, PR_TRUE },
  { &nsGkAtoms::dy, nsSVGUtils::Y, PR_TRUE }
};

nsSVGElement::LengthListAttributesInfo
nsSVGTextPositioningElement::GetLengthListInfo()
{
  return LengthListAttributesInfo(mLengthListAttributes, sLengthListInfo,
                                  ArrayLength(sLengthListInfo));
}


nsSVGElement::NumberListInfo nsSVGTextPositioningElement::sNumberListInfo[1] =
{
  { &nsGkAtoms::rotate }
};

nsSVGElement::NumberListAttributesInfo
nsSVGTextPositioningElement::GetNumberListInfo()
{
  return NumberListAttributesInfo(mNumberListAttributes, sNumberListInfo,
                                  ArrayLength(sNumberListInfo));
}





NS_IMETHODIMP nsSVGTextPositioningElement::GetX(nsIDOMSVGAnimatedLengthList * *aX)
{
  *aX = DOMSVGAnimatedLengthList::GetDOMWrapper(&mLengthListAttributes[X],
                                                this, X, nsSVGUtils::X).get();
  return NS_OK;
}


NS_IMETHODIMP nsSVGTextPositioningElement::GetY(nsIDOMSVGAnimatedLengthList * *aY)
{
  *aY = DOMSVGAnimatedLengthList::GetDOMWrapper(&mLengthListAttributes[Y],
                                                this, Y, nsSVGUtils::Y).get();
  return NS_OK;
}


NS_IMETHODIMP nsSVGTextPositioningElement::GetDx(nsIDOMSVGAnimatedLengthList * *aDx)
{
  *aDx = DOMSVGAnimatedLengthList::GetDOMWrapper(&mLengthListAttributes[DX],
                                                 this, DX, nsSVGUtils::X).get();
  return NS_OK;
}


NS_IMETHODIMP nsSVGTextPositioningElement::GetDy(nsIDOMSVGAnimatedLengthList * *aDy)
{
  *aDy = DOMSVGAnimatedLengthList::GetDOMWrapper(&mLengthListAttributes[DY],
                                                 this, DY, nsSVGUtils::Y).get();
  return NS_OK;
}


NS_IMETHODIMP nsSVGTextPositioningElement::GetRotate(nsIDOMSVGAnimatedNumberList * *aRotate)
{
  *aRotate = DOMSVGAnimatedNumberList::GetDOMWrapper(&mNumberListAttributes[ROTATE],
                                                     this, ROTATE).get();
  return NS_OK;
}
