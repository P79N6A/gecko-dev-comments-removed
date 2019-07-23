





































#include "nsSVGContainerFrame.h"
#include "nsISVGSVGFrame.h"
#include "gfxMatrix.h"

typedef nsSVGDisplayContainerFrame nsSVGInnerSVGFrameBase;

class nsSVGInnerSVGFrame : public nsSVGInnerSVGFrameBase,
                           public nsISVGSVGFrame
{
  friend nsIFrame*
  NS_NewSVGInnerSVGFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGInnerSVGFrame(nsStyleContext* aContext) :
    nsSVGInnerSVGFrameBase(aContext) {}
  
public:
  NS_DECL_QUERYFRAME_TARGET(nsSVGInnerSVGFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  
  

#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGInnerSVG"), aResult);
  }
#endif

  
  NS_IMETHOD PaintSVG(nsSVGRenderState *aContext, const nsIntRect *aDirtyRect);
  virtual void NotifySVGChanged(PRUint32 aFlags);
  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint &aPoint);

  
  virtual gfxMatrix GetCanvasTM();

  
  

  
  NS_IMETHOD SuspendRedraw();
  NS_IMETHOD UnsuspendRedraw();
  NS_IMETHOD NotifyViewportChange();

protected:

  nsCOMPtr<nsIDOMSVGMatrix> mCanvasTM;
};
