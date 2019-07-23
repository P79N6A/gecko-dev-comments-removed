





































#ifndef __NS_SVGTEXTCONTENTELEMENTBASE_H__
#define __NS_SVGTEXTCONTENTELEMENTBASE_H__

#include "nsIDOMSVGTextContentElement.h"
#include "nsSVGTextContainerFrame.h"

class nsSVGTextContentElement
{
public:
  NS_DECL_NSIDOMSVGTEXTCONTENTELEMENT

protected:
  virtual nsSVGTextContainerFrame* GetTextContainerFrame()=0;
};

#endif
