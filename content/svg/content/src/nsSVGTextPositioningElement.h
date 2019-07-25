


































#ifndef __NS_SVGTEXTPOSITIONINGELEMENTBASE_H__
#define __NS_SVGTEXTPOSITIONINGELEMENTBASE_H__

#include "nsIDOMSVGTextPositionElem.h"
#include "nsSVGTextContentElement.h"
#include "nsIDOMSVGAnimatedLengthList.h"
#include "nsIDOMSVGAnimatedNumberList.h"
#include "SVGAnimatedLengthList.h"

class nsSVGElement;

namespace mozilla {
class SVGAnimatedLengthList;
}

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

  virtual LengthListAttributesInfo GetLengthListInfo();

  

  enum { X, Y, DX, DY };
  SVGAnimatedLengthList mLengthListAttributes[4];
  static LengthListInfo sLengthListInfo[4];

  nsCOMPtr<nsIDOMSVGAnimatedNumberList> mRotate;
};

#endif
