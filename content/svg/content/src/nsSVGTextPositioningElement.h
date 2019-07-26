




#ifndef __NS_SVGTEXTPOSITIONINGELEMENTBASE_H__
#define __NS_SVGTEXTPOSITIONINGELEMENTBASE_H__

#include "nsIDOMSVGTextPositionElem.h"
#include "SVGTextContentElement.h"
#include "SVGAnimatedLengthList.h"
#include "SVGAnimatedNumberList.h"

class nsSVGElement;

namespace mozilla {
class SVGAnimatedLengthList;
}

typedef mozilla::dom::SVGTextContentElement nsSVGTextPositioningElementBase;






class nsSVGTextPositioningElement : public nsSVGTextPositioningElementBase
{
public:
  NS_DECL_NSIDOMSVGTEXTPOSITIONINGELEMENT

protected:

  nsSVGTextPositioningElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGTextPositioningElementBase(aNodeInfo)
  {}

  virtual LengthListAttributesInfo GetLengthListInfo();
  virtual NumberListAttributesInfo GetNumberListInfo();

  

  enum { X, Y, DX, DY };
  SVGAnimatedLengthList mLengthListAttributes[4];
  static LengthListInfo sLengthListInfo[4];

  enum { ROTATE };
  SVGAnimatedNumberList mNumberListAttributes[1];
  static NumberListInfo sNumberListInfo[1];
};

#endif
