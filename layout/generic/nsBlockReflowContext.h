







#ifndef nsBlockReflowContext_h___
#define nsBlockReflowContext_h___

#include "nsIFrame.h"
#include "nsHTMLReflowMetrics.h"

class nsBlockReflowState;
struct nsHTMLReflowState;
class nsLineBox;
class nsPresContext;




class nsBlockReflowContext {
public:
  nsBlockReflowContext(nsPresContext* aPresContext,
                       const nsHTMLReflowState& aParentRS);
  ~nsBlockReflowContext() { }

  void ReflowBlock(const mozilla::LogicalRect& aSpace,
                   bool                        aApplyBStartMargin,
                   nsCollapsingMargin&         aPrevMargin,
                   nscoord                     aClearance,
                   bool                        aIsAdjacentWithBStart,
                   nsLineBox*                  aLine,
                   nsHTMLReflowState&          aReflowState,
                   nsReflowStatus&             aReflowStatus,
                   nsBlockReflowState&         aState);

  bool PlaceBlock(const nsHTMLReflowState& aReflowState,
                  bool                     aForceFit,
                  nsLineBox*               aLine,
                  nsCollapsingMargin&      aBEndMarginResult ,
                  nsOverflowAreas&         aOverflowAreas,
                  nsReflowStatus           aReflowStatus);

  nsCollapsingMargin& GetCarriedOutBEndMargin() {
    return mMetrics.mCarriedOutBEndMargin;
  }

  const nsHTMLReflowMetrics& GetMetrics() const {
    return mMetrics;
  }

  


















  bool ComputeCollapsedBStartMargin(const nsHTMLReflowState& aRS,
                                    nsCollapsingMargin* aMargin,
                                    nsIFrame* aClearanceFrame,
                                    bool* aMayNeedRetry,
                                    bool* aIsEmpty = nullptr);

protected:
  nsPresContext* mPresContext;
  const nsHTMLReflowState& mOuterReflowState;

  nsIFrame* mFrame;
  mozilla::LogicalRect mSpace;

  nscoord mICoord, mBCoord, mContainerWidth;
  mozilla::WritingMode mWritingMode;
  nsHTMLReflowMetrics mMetrics;
  nsCollapsingMargin mBStartMargin;
};

#endif 
