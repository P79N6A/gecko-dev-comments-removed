





































#ifndef __NS_SVGOUTERSVGFRAME_H__
#define __NS_SVGOUTERSVGFRAME_H__

#include "nsSVGContainerFrame.h"
#include "nsISVGSVGFrame.h"
#include "nsIDOMSVGPoint.h"
#include "nsIDOMSVGNumber.h"
#include "gfxMatrix.h"

class nsSVGForeignObjectFrame;




typedef nsSVGDisplayContainerFrame nsSVGOuterSVGFrameBase;

class nsSVGOuterSVGFrame : public nsSVGOuterSVGFrameBase,
                           public nsISVGSVGFrame
{
  friend nsIFrame*
  NS_NewSVGOuterSVGFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGOuterSVGFrame(nsStyleContext* aContext);

public:
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

#ifdef DEBUG
  ~nsSVGOuterSVGFrame() {
    NS_ASSERTION(mForeignObjectHash.Count() == 0,
                 "foreignObject(s) still registered!");
  }
#endif

  
  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);

  virtual IntrinsicSize GetIntrinsicSize();
  virtual nsSize  GetIntrinsicRatio();

  virtual nsSize ComputeSize(nsIRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             PRBool aShrinkWrap);

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD  DidReflow(nsPresContext*   aPresContext,
                        const nsHTMLReflowState*  aReflowState,
                        nsDidReflowStatus aStatus);

  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint& aPoint);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  virtual nsSplittableType GetSplittableType() const;

  




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

  

  void InvalidateCoveredRegion(nsIFrame *aFrame);
  
  
  
  
  PRBool UpdateAndInvalidateCoveredRegion(nsIFrame *aFrame);

  PRBool IsRedrawSuspended();

  
  NS_IMETHOD SuspendRedraw();
  NS_IMETHOD UnsuspendRedraw();
  NS_IMETHOD NotifyViewportChange();

  
  virtual gfxMatrix GetCanvasTM();

  




  void RegisterForeignObject(nsSVGForeignObjectFrame* aFrame);
  void UnregisterForeignObject(nsSVGForeignObjectFrame* aFrame);

protected:

  



  PRBool EmbeddedByReference(nsIFrame **aEmbeddingFrame = nsnull);

  
  
  
  nsTHashtable<nsVoidPtrHashKey> mForeignObjectHash;

  PRUint32 mRedrawSuspendCount;
  nsCOMPtr<nsIDOMSVGMatrix> mCanvasTM;

  float mFullZoom;

  PRPackedBool mViewportInitialized;
#ifdef XP_MACOSX
  PRPackedBool mEnableBitmapFallback;
#endif
  PRPackedBool mIsRootContent;
};

#endif
