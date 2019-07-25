







































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
                       bool                aApplyTopMargin,
                       nsCollapsingMargin& aPrevMargin,
                       nscoord             aClearance,
                       bool                aIsAdjacentWithTop,
                       nsLineBox*          aLine,
                       nsHTMLReflowState&  aReflowState,
                       nsReflowStatus&     aReflowStatus,
                       nsBlockReflowState& aState);

  bool PlaceBlock(const nsHTMLReflowState& aReflowState,
                    bool                     aForceFit,
                    nsLineBox*               aLine,
                    nsCollapsingMargin&      aBottomMarginResult ,
                    nsRect&                  aInFlowBounds,
                    nsOverflowAreas&         aOverflowAreas,
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

  













  static bool ComputeCollapsedTopMargin(const nsHTMLReflowState& aRS,
                                          nsCollapsingMargin* aMargin, nsIFrame* aClearanceFrame,
                                          bool* aMayNeedRetry, bool* aIsEmpty = nsnull);

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
