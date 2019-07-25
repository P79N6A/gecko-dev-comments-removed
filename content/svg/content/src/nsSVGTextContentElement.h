





































#ifndef __NS_SVGTEXTCONTENTELEMENTBASE_H__
#define __NS_SVGTEXTCONTENTELEMENTBASE_H__

#include "nsIDOMSVGTextContentElement.h"
#include "nsSVGTextContainerFrame.h"
#include "nsSVGStylableElement.h"
#include "DOMSVGTests.h"

typedef nsSVGStylableElement nsSVGTextContentElementBase;







class nsSVGTextContentElement : public nsSVGTextContentElementBase,
                                public DOMSVGTests
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
