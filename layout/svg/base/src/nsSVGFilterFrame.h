



































#ifndef __NS_SVGFILTERFRAME_H__
#define __NS_SVGFILTERFRAME_H__

#include "nsRect.h"
#include "nsSVGContainerFrame.h"

class nsSVGFilterInstance;

typedef nsSVGContainerFrame nsSVGFilterFrameBase;
class nsSVGFilterFrame : public nsSVGFilterFrameBase
{
  friend nsIFrame*
  NS_NewSVGFilterFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
protected:
  nsSVGFilterFrame(nsStyleContext* aContext) : nsSVGFilterFrameBase(aContext) {}

public:    
  nsresult FilterPaint(nsSVGRenderState *aContext,
                       nsISVGChildFrame *aTarget,
                       const nsRect* aDirtyRect);

  
  
  
  
  
  nsRect GetInvalidationRegion(nsIFrame *aTarget, const nsRect& aRect);

  




  virtual nsIAtom* GetType() const;

private:
  
  nsresult CreateInstance(nsISVGChildFrame *aTarget,
                          const nsRect *aDirtyOutputRect,
                          const nsRect *aDirtyInputRect,
                          nsSVGFilterInstance **aInstance);
};

#endif
