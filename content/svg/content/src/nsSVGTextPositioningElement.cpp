


































#include "nsSVGTextPositioningElement.h"
#include "nsSVGAnimatedLengthList.h"
#include "nsSVGAnimatedNumberList.h"
#include "nsSVGLengthList.h"
#include "nsSVGNumberList.h"

nsresult
nsSVGTextPositioningElement::Initialise(nsSVGElement *aSVGElement)
{
  nsresult rv;
  

  
  {
    nsCOMPtr<nsIDOMSVGLengthList> lengthList;
    rv = NS_NewSVGLengthList(getter_AddRefs(lengthList), aSVGElement, nsSVGUtils::X);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedLengthList(getter_AddRefs(mX),
                                     lengthList);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = aSVGElement->AddMappedSVGValue(nsGkAtoms::x, mX);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  
  
  {
    nsCOMPtr<nsIDOMSVGLengthList> lengthList;
    rv = NS_NewSVGLengthList(getter_AddRefs(lengthList), aSVGElement, nsSVGUtils::Y);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedLengthList(getter_AddRefs(mY),
                                     lengthList);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = aSVGElement->AddMappedSVGValue(nsGkAtoms::y, mY);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsIDOMSVGLengthList> lengthList;
    rv = NS_NewSVGLengthList(getter_AddRefs(lengthList), aSVGElement, nsSVGUtils::X);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedLengthList(getter_AddRefs(mdX),
                                     lengthList);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = aSVGElement->AddMappedSVGValue(nsGkAtoms::dx, mdX);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  
  
  {
    nsCOMPtr<nsIDOMSVGLengthList> lengthList;
    rv = NS_NewSVGLengthList(getter_AddRefs(lengthList), aSVGElement, nsSVGUtils::Y);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedLengthList(getter_AddRefs(mdY),
                                     lengthList);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = aSVGElement->AddMappedSVGValue(nsGkAtoms::dy, mdY);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsIDOMSVGNumberList> numberList;
    rv = NS_NewSVGNumberList(getter_AddRefs(numberList));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedNumberList(getter_AddRefs(mRotate),
                                     numberList);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = aSVGElement->AddMappedSVGValue(nsGkAtoms::rotate, mRotate);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  return rv;
}





NS_IMETHODIMP nsSVGTextPositioningElement::GetX(nsIDOMSVGAnimatedLengthList * *aX)
{
  *aX = mX;
  NS_IF_ADDREF(*aX);
  return NS_OK;
}


NS_IMETHODIMP nsSVGTextPositioningElement::GetY(nsIDOMSVGAnimatedLengthList * *aY)
{
  *aY = mY;
  NS_IF_ADDREF(*aY);
  return NS_OK;
}


NS_IMETHODIMP nsSVGTextPositioningElement::GetDx(nsIDOMSVGAnimatedLengthList * *aDx)
{
  *aDx = mdX;
  NS_IF_ADDREF(*aDx);
  return NS_OK;
}


NS_IMETHODIMP nsSVGTextPositioningElement::GetDy(nsIDOMSVGAnimatedLengthList * *aDy)
{
  *aDy = mdY;
  NS_IF_ADDREF(*aDy);
  return NS_OK;
}


NS_IMETHODIMP nsSVGTextPositioningElement::GetRotate(nsIDOMSVGAnimatedNumberList * *aRotate)
{
  *aRotate = mRotate;
  NS_IF_ADDREF(*aRotate);
  return NS_OK;
}
