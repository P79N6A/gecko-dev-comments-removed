


































#ifndef __NS_SVGTEXTPOSITIONINGELEMENTBASE_H__
#define __NS_SVGTEXTPOSITIONINGELEMENTBASE_H__

#include "nsIDOMSVGTextPositionElem.h"
#include "nsSVGTextContentElement.h"
#include "nsIDOMSVGAnimatedLengthList.h"
#include "nsIDOMSVGAnimatedNumberList.h"

class nsSVGElement;

typedef nsSVGTextContentElement nsSVGTextPositioningElementBase;






class nsSVGTextPositioningElement : public nsSVGTextPositioningElementBase
{
public:
  NS_DECL_NSIDOMSVGTEXTPOSITIONINGELEMENT

protected:

  nsSVGTextPositioningElement(nsINodeInfo *aNodeInfo)
    : nsSVGTextPositioningElementBase(aNodeInfo)
  {}

  nsresult Init();

  
  nsCOMPtr<nsIDOMSVGAnimatedLengthList> mX;
  nsCOMPtr<nsIDOMSVGAnimatedLengthList> mY;
  nsCOMPtr<nsIDOMSVGAnimatedLengthList> mdX;
  nsCOMPtr<nsIDOMSVGAnimatedLengthList> mdY;
  nsCOMPtr<nsIDOMSVGAnimatedNumberList> mRotate;
};

#endif
