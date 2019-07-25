



































#ifndef __NS_SVGFILTERFRAME_H__
#define __NS_SVGFILTERFRAME_H__

#include "nsFrame.h"
#include "nsQueryFrame.h"
#include "nsRect.h"
#include "nsSVGContainerFrame.h"
#include "nsSVGUtils.h"

class nsIAtom;
class nsIContent;
class nsIFrame;
class nsIPresShell;
class nsRenderingContext;
class nsStyleContext;
class nsSVGFilterPaintCallback;

typedef nsSVGContainerFrame nsSVGFilterFrameBase;

class nsSVGFilterFrame : public nsSVGFilterFrameBase
{
  friend nsIFrame*
  NS_NewSVGFilterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGFilterFrame(nsStyleContext* aContext)
    : nsSVGFilterFrameBase(aContext)
  {
    AddStateBits(NS_STATE_SVG_NONDISPLAY_CHILD);
  }

public:
  NS_DECL_FRAMEARENA_HELPERS

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

  nsresult FilterPaint(nsRenderingContext *aContext,
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
