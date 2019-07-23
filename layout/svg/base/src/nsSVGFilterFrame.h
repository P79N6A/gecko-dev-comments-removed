



































#ifndef __NS_SVGFILTERFRAME_H__
#define __NS_SVGFILTERFRAME_H__

#include "nsRect.h"
#include "nsSVGContainerFrame.h"

class nsSVGRenderState;
class nsSVGFilterPaintCallback;

typedef nsSVGContainerFrame nsSVGFilterFrameBase;
class nsSVGFilterFrame : public nsSVGFilterFrameBase
{
  friend nsIFrame*
  NS_NewSVGFilterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGFilterFrame(nsStyleContext* aContext) : nsSVGFilterFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  nsresult FilterPaint(nsSVGRenderState *aContext,
                       nsIFrame *aTarget, nsSVGFilterPaintCallback *aPaintCallback,
                       const nsIntRect* aDirtyRect);

  




  nsIntRect GetInvalidationBBox(nsIFrame *aTarget, const nsIntRect& aRect);

  





  nsIntRect GetSourceForInvalidArea(nsIFrame *aTarget, const nsIntRect& aRect);

  





  nsIntRect GetFilterBBox(nsIFrame *aTarget, const nsIntRect *aSourceBBox);

#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  




  virtual nsIAtom* GetType() const;
};

#endif
