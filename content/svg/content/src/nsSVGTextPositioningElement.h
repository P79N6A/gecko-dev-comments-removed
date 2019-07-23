


































#ifndef __NS_SVGTEXTPOSITIONINGELEMENTBASE_H__
#define __NS_SVGTEXTPOSITIONINGELEMENTBASE_H__

#include "nsIDOMSVGTextPositionElem.h"
#include "nsSVGTextContentElement.h"
#include "nsIDOMSVGAnimatedLengthList.h"
#include "nsIDOMSVGAnimatedNumberList.h"

class nsSVGTextPositioningElement : public nsSVGTextContentElement
{
public:
  NS_DECL_NSIDOMSVGTEXTPOSITIONINGELEMENT

protected:
  nsresult Initialise(nsSVGElement *aSVGElement);

  
  nsCOMPtr<nsIDOMSVGAnimatedLengthList> mX;
  nsCOMPtr<nsIDOMSVGAnimatedLengthList> mY;
  nsCOMPtr<nsIDOMSVGAnimatedLengthList> mdX;
  nsCOMPtr<nsIDOMSVGAnimatedLengthList> mdY;
  nsCOMPtr<nsIDOMSVGAnimatedNumberList> mRotate;
};

#endif
