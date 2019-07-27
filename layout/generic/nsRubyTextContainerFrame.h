







#ifndef nsRubyTextContainerFrame_h___
#define nsRubyTextContainerFrame_h___

#include "nsBlockFrame.h"
#include "nsRubyBaseFrame.h"
#include "nsRubyTextFrame.h"
#include "nsLineLayout.h"

typedef nsContainerFrame nsRubyTextContainerFrameSuper;





nsContainerFrame* NS_NewRubyTextContainerFrame(nsIPresShell* aPresShell,
                                               nsStyleContext* aContext);

class nsRubyTextContainerFrame MOZ_FINAL : public nsRubyTextContainerFrameSuper
{
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsRubyTextContainerFrame)
  NS_DECL_QUERYFRAME

  
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;
  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE;
  virtual void Reflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus) MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif
  void SetISize(nscoord aISize) { mISize = aISize; }

protected:
  friend nsContainerFrame*
    NS_NewRubyTextContainerFrame(nsIPresShell* aPresShell,
                                 nsStyleContext* aContext);
  explicit nsRubyTextContainerFrame(nsStyleContext* aContext)
    : nsRubyTextContainerFrameSuper(aContext) {}

  
  
  
  nscoord mISize;
};

#endif 
