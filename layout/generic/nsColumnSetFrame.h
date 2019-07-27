




#ifndef nsColumnSetFrame_h___
#define nsColumnSetFrame_h___



#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"
#include "nsIFrameInlines.h" 

class nsColumnSetFrame MOZ_FINAL : public nsContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  explicit nsColumnSetFrame(nsStyleContext* aContext);

  virtual void Reflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus) MOZ_OVERRIDE;

#ifdef DEBUG
  virtual void SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList) MOZ_OVERRIDE;
  virtual void AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame) MOZ_OVERRIDE;
#endif

  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;

  



  virtual nscoord GetAvailableContentBSize(const nsHTMLReflowState& aReflowState);

  virtual nsContainerFrame* GetContentInsertionFrame() MOZ_OVERRIDE {
    nsIFrame* frame = GetFirstPrincipalChild();

    
    if (!frame)
      return nullptr;

    return frame->GetContentInsertionFrame();
  }

  virtual nsresult StealFrame(nsIFrame* aChild, bool aForceNormal) MOZ_OVERRIDE
  {
    
    
    return nsContainerFrame::StealFrame(aChild,
                                        IS_TRUE_OVERFLOW_CONTAINER(aChild));
  }

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
   {
     return nsContainerFrame::IsFrameOfType(aFlags &
              ~(nsIFrame::eCanContainOverflowContainers));
   }

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual void PaintColumnRule(nsRenderingContext* aCtx,
                               const nsRect&        aDirtyRect,
                               const nsPoint&       aPt);

  




  void DrainOverflowColumns();

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE {
    return MakeFrameName(NS_LITERAL_STRING("ColumnSet"), aResult);
  }
#endif

protected:
  nscoord        mLastBalanceBSize;
  nsReflowStatus mLastFrameStatus;

  


  struct ReflowConfig {
    
    
    int32_t mBalanceColCount;

    
    nscoord mColISize;

    
    
    nscoord mExpectedISizeLeftOver;

    
    nscoord mColGap;

    
    
    
    nscoord mColMaxBSize;

    
    
    bool mIsBalancing;

    
    
    nscoord mKnownFeasibleBSize;

    
    
    nscoord mKnownInfeasibleBSize;

    
    nscoord mComputedBSize;

    
    
    
    nscoord mConsumedBSize;
  };

  


  struct ColumnBalanceData {
    
    nscoord mMaxBSize;
    
    nscoord mSumBSize;
    
    nscoord mLastBSize;
    
    
    nscoord mMaxOverflowingBSize;
    
    
    
    bool mHasExcessBSize;

    void Reset() {
      mMaxBSize = mSumBSize = mLastBSize = mMaxOverflowingBSize = 0;
      mHasExcessBSize = false;
    }
  };

  bool ReflowColumns(nsHTMLReflowMetrics& aDesiredSize,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus& aReflowStatus,
                     ReflowConfig& aConfig,
                     bool aLastColumnUnbounded,
                     nsCollapsingMargin* aCarriedOutBEndMargin,
                     ColumnBalanceData& aColData);

  






  ReflowConfig ChooseColumnStrategy(const nsHTMLReflowState& aReflowState,
                                    bool aForceAuto, nscoord aFeasibleBSize,
                                    nscoord aInfeasibleBSize);

  























  void FindBestBalanceBSize(const nsHTMLReflowState& aReflowState,
                            nsPresContext* aPresContext,
                            ReflowConfig& aConfig,
                            ColumnBalanceData& aColData,
                            nsHTMLReflowMetrics& aDesiredSize,
                            nsCollapsingMargin& aOutMargin,
                            bool& aUnboundedLastColumn,
                            bool& aRunWasFeasible,
                            nsReflowStatus& aStatus);
  



  bool ReflowChildren(nsHTMLReflowMetrics& aDesiredSize,
                        const nsHTMLReflowState& aReflowState,
                        nsReflowStatus& aStatus,
                        const ReflowConfig& aConfig,
                        bool aLastColumnUnbounded,
                        nsCollapsingMargin* aCarriedOutBEndMargin,
                        ColumnBalanceData& aColData);
};

#endif 
