




#ifndef __NS_SVGTEXTCONTENTELEMENTBASE_H__
#define __NS_SVGTEXTCONTENTELEMENTBASE_H__

#include "nsIDOMSVGTextContentElement.h"
#include "nsSVGElement.h"
#include "nsSVGTextContainerFrame.h"

typedef nsSVGElement nsSVGTextContentElementBase;







class nsSVGTextContentElement : public nsSVGTextContentElementBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGTEXTCONTENTELEMENT

protected:

  nsSVGTextContentElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsSVGTextContentElementBase(aNodeInfo)
  {}

  nsSVGTextContainerFrame* GetTextContainerFrame() {
    return do_QueryFrame(GetPrimaryFrame(Flush_Layout));
  }
};

#endif
