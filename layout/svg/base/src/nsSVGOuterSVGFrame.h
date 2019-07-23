





































#ifndef __NS_SVGOUTERSVGFRAME_H__
#define __NS_SVGOUTERSVGFRAME_H__

#include "nsSVGContainerFrame.h"
#include "nsISVGSVGFrame.h"
#include "nsISVGEnum.h"
#include "nsIDOMSVGPoint.h"
#include "nsIDOMSVGNumber.h"




typedef nsSVGDisplayContainerFrame nsSVGOuterSVGFrameBase;

class nsSVGOuterSVGFrame : public nsSVGOuterSVGFrameBase,
                           public nsISVGSVGFrame
{
  friend nsIFrame*
  NS_NewSVGOuterSVGFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
protected:
  nsSVGOuterSVGFrame(nsStyleContext* aContext);
  NS_IMETHOD InitSVG();

   
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return 1; }
  NS_IMETHOD_(nsrefcnt) Release() { return 1; }

public:
  
  
  

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD  DidReflow(nsPresContext*   aPresContext,
                        const nsHTMLReflowState*  aReflowState,
                        nsDidReflowStatus aStatus);

  NS_IMETHOD  InsertFrames(nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsIFrame*       aFrameList);

  nsIFrame* GetFrameForPoint(const nsPoint& aPoint);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  




  virtual nsIAtom* GetType() const;

  void Paint(nsIRenderingContext& aRenderingContext,
             const nsRect& aDirtyRect, nsPoint aPt);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGOuterSVG"), aResult);
  }
#endif

  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType);

  

  
  nsresult InvalidateRect(nsRect aRect);
  PRBool IsRedrawSuspended();

  
  NS_IMETHOD SuspendRedraw();
  NS_IMETHOD UnsuspendRedraw();
  NS_IMETHOD NotifyViewportChange();

  
  virtual already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM();

protected:

  void CalculateAvailableSpace(nsRect *maxRect, nsRect *preferredRect,
                               nsPresContext* aPresContext,
                               const nsHTMLReflowState& aReflowState);

  PRUint32 mRedrawSuspendCount;
  nsCOMPtr<nsIDOMSVGMatrix> mCanvasTM;

  
  nsCOMPtr<nsISVGEnum>      mZoomAndPan;
  nsCOMPtr<nsIDOMSVGPoint>  mCurrentTranslate;
  nsCOMPtr<nsIDOMSVGNumber> mCurrentScale;

  PRPackedBool mViewportInitialized;
};

#endif
