






#include "nsContainerFrame.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsISupports.h"
#include "nsIAtom.h"
#include "nsPresContext.h"
#include "nsHTMLParts.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsCOMPtr.h"
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"
#include "nsCSSRendering.h"
#include <algorithm>

class nsColumnSetFrame : public nsContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsColumnSetFrame(nsStyleContext* aContext);

  NS_IMETHOD SetInitialChildList(ChildListID     aListID,
                                 nsFrameList&    aChildList);

  NS_IMETHOD Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus);

  NS_IMETHOD  AppendFrames(ChildListID     aListID,
                           nsFrameList&    aFrameList);
  NS_IMETHOD  InsertFrames(ChildListID     aListID,
                           nsIFrame*       aPrevFrame,
                           nsFrameList&    aFrameList);
  NS_IMETHOD  RemoveFrame(ChildListID     aListID,
                          nsIFrame*       aOldFrame);

  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);

  virtual nsIFrame* GetContentInsertionFrame() {
    nsIFrame* frame = GetFirstPrincipalChild();

    
    if (!frame)
      return nullptr;

    return frame->GetContentInsertionFrame();
  }

  virtual nsresult StealFrame(nsPresContext* aPresContext,
                              nsIFrame*      aChild,
                              bool           aForceNormal)
  { 
    return nsContainerFrame::StealFrame(aPresContext, aChild, true);
  }

  virtual bool IsFrameOfType(uint32_t aFlags) const
   {
     return nsContainerFrame::IsFrameOfType(aFlags &
              ~(nsIFrame::eCanContainOverflowContainers));
   }

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual nsIAtom* GetType() const;

  virtual void PaintColumnRule(nsRenderingContext* aCtx,
                               const nsRect&        aDirtyRect,
                               const nsPoint&       aPt);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("ColumnSet"), aResult);
  }
#endif

protected:
  nscoord        mLastBalanceHeight;
  nsReflowStatus mLastFrameStatus;

  


  struct ReflowConfig {
    
    
    int32_t mBalanceColCount;

    
    nscoord mColWidth;

    
    
    nscoord mExpectedWidthLeftOver;

    
    nscoord mColGap;

    
    
    
    nscoord mColMaxHeight;

    
    
    bool mIsBalancing;

    
    
    nscoord mKnownFeasibleHeight;

    
    
    nscoord mKnownInfeasibleHeight;
  };

  


  struct ColumnBalanceData {
    
    nscoord mMaxHeight;
    
    nscoord mSumHeight;
    
    nscoord mLastHeight;
    
    
    nscoord mMaxOverflowingHeight;
    
    
    
    bool mHasExcessHeight;

    void Reset() {
      mMaxHeight = mSumHeight = mLastHeight = mMaxOverflowingHeight = 0;
      mHasExcessHeight = false;
    }
  };

  




  void DrainOverflowColumns();

  bool ReflowColumns(nsHTMLReflowMetrics& aDesiredSize,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus& aReflowStatus,
                     ReflowConfig& aConfig,
                     bool aLastColumnUnbounded,
                     nsCollapsingMargin* aCarriedOutBottomMargin,
                     ColumnBalanceData& aColData);

  






  ReflowConfig ChooseColumnStrategy(const nsHTMLReflowState& aReflowState,
                                    bool aForceAuto, nscoord aFeasibleHeight,
                                    nscoord aInfeasibleHeight);

  



  bool ReflowChildren(nsHTMLReflowMetrics& aDesiredSize,
                        const nsHTMLReflowState& aReflowState,
                        nsReflowStatus& aStatus,
                        const ReflowConfig& aConfig,
                        bool aLastColumnUnbounded,
                        nsCollapsingMargin* aCarriedOutBottomMargin,
                        ColumnBalanceData& aColData);
};
