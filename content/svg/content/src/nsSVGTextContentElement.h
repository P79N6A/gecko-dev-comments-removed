





































#ifndef __NS_SVGTEXTCONTENTELEMENTBASE_H__
#define __NS_SVGTEXTCONTENTELEMENTBASE_H__

#include "nsIDOMSVGTextContentElement.h"
#include "nsSVGTextContainerFrame.h"
#include "nsSVGStylableElement.h"

typedef nsSVGStylableElement nsSVGTextContentElementBase;







class nsSVGTextContentElement : public nsSVGTextContentElementBase
{
public:
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
