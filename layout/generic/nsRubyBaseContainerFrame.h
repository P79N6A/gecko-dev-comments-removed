







#ifndef nsRubyBaseContainerFrame_h___
#define nsRubyBaseContainerFrame_h___

#include "nsContainerFrame.h"





nsContainerFrame* NS_NewRubyBaseContainerFrame(nsIPresShell* aPresShell,
                                               nsStyleContext* aContext);

namespace mozilla {
struct RubyColumn;
}

class nsRubyBaseContainerFrame final : public nsContainerFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsRubyBaseContainerFrame)
  NS_DECL_QUERYFRAME

  
  virtual nsIAtom* GetType() const override;
  virtual bool IsFrameOfType(uint32_t aFlags) const override;
  virtual bool CanContinueTextRun() const override;
  virtual void AddInlineMinISize(nsRenderingContext *aRenderingContext,
                                 InlineMinISizeData *aData) override;
  virtual void AddInlinePrefISize(nsRenderingContext *aRenderingContext,
                                  InlinePrefISizeData *aData) override;
  virtual mozilla::LogicalSize
    ComputeSize(nsRenderingContext *aRenderingContext,
                mozilla::WritingMode aWritingMode,
                const mozilla::LogicalSize& aCBSize,
                nscoord aAvailableISize,
                const mozilla::LogicalSize& aMargin,
                const mozilla::LogicalSize& aBorder,
                const mozilla::LogicalSize& aPadding,
                ComputeSizeFlags aFlags) override;
  virtual void Reflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus) override;

  virtual nscoord
    GetLogicalBaseline(mozilla::WritingMode aWritingMode) const override;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif

protected:
  friend nsContainerFrame*
    NS_NewRubyBaseContainerFrame(nsIPresShell* aPresShell,
                                 nsStyleContext* aContext);
  explicit nsRubyBaseContainerFrame(nsStyleContext* aContext) : nsContainerFrame(aContext) {}

  struct ReflowState;
  nscoord ReflowColumns(const ReflowState& aReflowState,
                        nsReflowStatus& aStatus);
  nscoord ReflowOneColumn(const ReflowState& aReflowState,
                          uint32_t aColumnIndex,
                          const mozilla::RubyColumn& aColumn,
                          nsReflowStatus& aStatus);
  nscoord ReflowSpans(const ReflowState& aReflowState);

  struct PullFrameState;

  
  
  void PullOneColumn(nsLineLayout* aLineLayout,
                     PullFrameState& aPullFrameState,
                     mozilla::RubyColumn& aColumn,
                     bool& aIsComplete);

  nscoord mBaseline;
};

#endif 
