







































#ifndef nsBlockReflowContext_h___
#define nsBlockReflowContext_h___

#include "nsIFrame.h"
#include "nsHTMLReflowMetrics.h"

class nsBlockFrame;
class nsBlockReflowState;
struct nsHTMLReflowState;
class nsLineBox;
class nsIFrame;
class nsPresContext;
class nsLineLayout;
struct nsStylePosition;
struct nsBlockHorizontalAlign;




class nsBlockReflowContext {
public:
  nsBlockReflowContext(nsPresContext* aPresContext,
                       const nsHTMLReflowState& aParentRS);
  ~nsBlockReflowContext() { }

  nsresult ReflowBlock(const nsRect&       aSpace,
                       PRBool              aApplyTopMargin,
                       nsCollapsingMargin& aPrevMargin,
                       nscoord             aClearance,
                       PRBool              aIsAdjacentWithTop,
                       nsMargin&           aComputedOffsets,
                       nsLineBox*          aLine,
                       nsHTMLReflowState&  aReflowState,
                       nsReflowStatus&     aReflowStatus);

  PRBool PlaceBlock(const nsHTMLReflowState& aReflowState,
                    PRBool                   aForceFit,
                    nsLineBox*               aLine,
                    const nsMargin&          aComputedOffsets,
                    nsCollapsingMargin&      aBottomMarginResult ,
                    nsRect&                  aInFlowBounds,
                    nsRect&                  aCombinedRect,
                    nsReflowStatus           aReflowStatus);

  nsCollapsingMargin& GetCarriedOutBottomMargin() {
    return mMetrics.mCarriedOutBottomMargin;
  }

  nscoord GetTopMargin() const {
    return mTopMargin.get();
  }

  const nsHTMLReflowMetrics& GetMetrics() const {
    return mMetrics;
  }

  













  static PRBool ComputeCollapsedTopMargin(const nsHTMLReflowState& aRS,
                                          nsCollapsingMargin* aMargin, nsIFrame* aClearanceFrame,
                                          PRBool* aMayNeedRetry, PRBool* aIsEmpty = nsnull);

protected:
  nsPresContext* mPresContext;
  const nsHTMLReflowState& mOuterReflowState;

  nsIFrame* mFrame;
  nsRect mSpace;

  nscoord mX, mY;
  nsHTMLReflowMetrics mMetrics;
  nsCollapsingMargin mTopMargin;
};

#endif 
