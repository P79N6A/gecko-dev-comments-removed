







#ifndef nsRubyTextContainerFrame_h___
#define nsRubyTextContainerFrame_h___

#include "nsBlockFrame.h"
#include "nsRubyBaseFrame.h"
#include "nsRubyTextFrame.h"
#include "nsLineLayout.h"





nsContainerFrame* NS_NewRubyTextContainerFrame(nsIPresShell* aPresShell,
                                               nsStyleContext* aContext);




class nsRubyTextContainerFrame MOZ_FINAL : public nsBlockFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsRubyTextContainerFrame)
  NS_DECL_QUERYFRAME

  
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;
  virtual void Reflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus) MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

  void ReflowRubyTextFrame(nsRubyTextFrame* rtFrame, nsIFrame* rbFrame,
                           nscoord baseStart, nsPresContext* aPresContext,
                           nsHTMLReflowMetrics& aDesiredSize,
                           const nsHTMLReflowState& aReflowState);
  void BeginRTCLineLayout(nsPresContext* aPresContext,
                          const nsHTMLReflowState& aReflowState);
  nsLineLayout* GetLineLayout() { return mLineLayout.get(); };

protected:
  friend nsContainerFrame*
    NS_NewRubyTextContainerFrame(nsIPresShell* aPresShell,
                                 nsStyleContext* aContext);
  explicit nsRubyTextContainerFrame(nsStyleContext* aContext) : nsBlockFrame(aContext) {}
  
  
  
  mozilla::UniquePtr<nsLineLayout> mLineLayout;
  
  
  
  nscoord mISize;
};

#endif 
