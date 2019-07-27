







#ifndef nsRubyBaseContainerFrame_h___
#define nsRubyBaseContainerFrame_h___

#include "nsContainerFrame.h"
#include "nsRubyTextContainerFrame.h"
#include "nsRubyBaseFrame.h"
#include "nsRubyTextFrame.h"
#include "RubyReflowState.h"





nsContainerFrame* NS_NewRubyBaseContainerFrame(nsIPresShell* aPresShell,
                                               nsStyleContext* aContext);

namespace mozilla {
struct RubyColumn;
}

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

protected:
  friend nsContainerFrame*
    NS_NewRubyBaseContainerFrame(nsIPresShell* aPresShell,
                                 nsStyleContext* aContext);
  explicit nsRubyBaseContainerFrame(nsStyleContext* aContext) : nsContainerFrame(aContext) {}

  typedef nsTArray<nsRubyTextContainerFrame*> TextContainerArray;
  typedef nsAutoTArray<nsRubyTextContainerFrame*, RTC_ARRAY_SIZE> AutoTextContainerArray;
  void GetTextContainers(TextContainerArray& aTextContainers);

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
