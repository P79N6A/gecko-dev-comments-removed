







#ifndef nsRubyBaseContainerFrame_h___
#define nsRubyBaseContainerFrame_h___

#include "nsContainerFrame.h"
#include "nsRubyTextContainerFrame.h"
#include "nsRubyBaseFrame.h"
#include "nsRubyTextFrame.h"

#define RTC_ARRAY_SIZE 1





nsContainerFrame* NS_NewRubyBaseContainerFrame(nsIPresShell* aPresShell,
                                               nsStyleContext* aContext);

class nsRubyBaseContainerFrame MOZ_FINAL : public nsContainerFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsRubyBaseContainerFrame)
  NS_DECL_QUERYFRAME

  
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;
  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE;
  virtual bool CanContinueTextRun() const MOZ_OVERRIDE;
  virtual void AddInlineMinISize(nsRenderingContext *aRenderingContext,
                                 InlineMinISizeData *aData) MOZ_OVERRIDE;
  virtual void AddInlinePrefISize(nsRenderingContext *aRenderingContext,
                                  InlinePrefISizeData *aData) MOZ_OVERRIDE;
  virtual mozilla::LogicalSize
    ComputeSize(nsRenderingContext *aRenderingContext,
                mozilla::WritingMode aWritingMode,
                const mozilla::LogicalSize& aCBSize,
                nscoord aAvailableISize,
                const mozilla::LogicalSize& aMargin,
                const mozilla::LogicalSize& aBorder,
                const mozilla::LogicalSize& aPadding,
                ComputeSizeFlags aFlags) MOZ_OVERRIDE;
  virtual void Reflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus) MOZ_OVERRIDE;

  virtual nscoord
    GetLogicalBaseline(mozilla::WritingMode aWritingMode) const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

#ifdef DEBUG
  void AssertTextContainersEmpty()
  {
    MOZ_ASSERT(mSpanContainers.IsEmpty());
    MOZ_ASSERT(mTextContainers.IsEmpty());
  }
#endif

  void AppendTextContainer(nsIFrame* aFrame);
  void ClearTextContainers();

protected:
  friend nsContainerFrame*
    NS_NewRubyBaseContainerFrame(nsIPresShell* aPresShell,
                                 nsStyleContext* aContext);
  explicit nsRubyBaseContainerFrame(nsStyleContext* aContext) : nsContainerFrame(aContext) {}

  nscoord CalculateMaxSpanISize(nsRenderingContext* aRenderingContext);

  nscoord ReflowPairs(nsPresContext* aPresContext,
                      const nsHTMLReflowState& aReflowState,
                      nsTArray<nsHTMLReflowState*>& aReflowStates,
                      nsReflowStatus& aStatus);

  nscoord ReflowOnePair(nsPresContext* aPresContext,
                        const nsHTMLReflowState& aReflowState,
                        nsTArray<nsHTMLReflowState*>& aReflowStates,
                        nsIFrame* aBaseFrame,
                        const nsTArray<nsIFrame*>& aTextFrames,
                        nsReflowStatus& aStatus);

  nscoord ReflowSpans(nsPresContext* aPresContext,
                      const nsHTMLReflowState& aReflowState,
                      nsTArray<nsHTMLReflowState*>& aReflowStates);

  struct PullFrameState;

  
  
  void PullOnePair(nsLineLayout* aLineLayout,
                   PullFrameState& aPullFrameState,
                   nsIFrame*& aBaseFrame,
                   nsTArray<nsIFrame*>& aTextFrames,
                   bool& aIsComplete);

  





  
  
  nsTArray<nsRubyTextContainerFrame*> mSpanContainers;
  
  nsTArray<nsRubyTextContainerFrame*> mTextContainers;

  nscoord mBaseline;
  uint32_t mPairCount;
};

#endif 
