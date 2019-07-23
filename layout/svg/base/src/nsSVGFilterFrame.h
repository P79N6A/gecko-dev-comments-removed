



































#ifndef __NS_SVGFILTERFRAME_H__
#define __NS_SVGFILTERFRAME_H__

#include "nsRect.h"
#include "nsSVGContainerFrame.h"

typedef nsSVGContainerFrame nsSVGFilterFrameBase;

class nsSVGFilterFrame : public nsSVGFilterFrameBase
{
  friend nsIFrame*
  NS_NewSVGFilterFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);

protected:
  NS_IMETHOD InitSVG();

public:
  nsSVGFilterFrame(nsStyleContext* aContext) : nsSVGFilterFrameBase(aContext) {}

  nsresult FilterPaint(nsSVGRenderState *aContext,
                       nsISVGChildFrame *aTarget);
  nsRect GetInvalidationRegion(nsIFrame *aTarget);

  




  virtual nsIAtom* GetType() const;

private:
  
  void FilterFailCleanup(nsSVGRenderState *aContext,
                         nsISVGChildFrame *aTarget);
};

nsIContent *
NS_GetSVGFilterElement(nsIURI *aURI, nsIContent *aContent);

#endif
