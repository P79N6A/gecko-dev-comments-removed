



































#ifndef nsPageContentFrame_h___
#define nsPageContentFrame_h___

#include "nsViewportFrame.h"
class nsPageFrame;
class nsSharedPageData;


class nsPageContentFrame : public ViewportFrame {

public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewPageContentFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
  friend class nsPageFrame;

  
  NS_IMETHOD  Reflow(nsPresContext*      aPresContext,
                     nsHTMLReflowMetrics& aDesiredSize,
                     const nsHTMLReflowState& aMaxSize,
                     nsReflowStatus&      aStatus);

  virtual PRBool IsContainingBlock() const;

  virtual void SetSharedPageData(nsSharedPageData* aPD) { mPD = aPD; }

  



  virtual nsSize ComputeSize(nsIRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             PRBool aShrinkWrap);

  




  virtual nsIAtom* GetType() const;
  
#ifdef NS_DEBUG
  
  NS_IMETHOD  GetFrameName(nsAString& aResult) const;
#endif

protected:
  nsPageContentFrame(nsStyleContext* aContext) : ViewportFrame(aContext) {}

  nsSharedPageData*         mPD;
};

#endif 

