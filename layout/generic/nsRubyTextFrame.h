







#ifndef nsRubyTextFrame_h___
#define nsRubyTextFrame_h___

#include "nsRubyContentFrame.h"

typedef nsRubyContentFrame nsRubyTextFrameSuper;





nsContainerFrame* NS_NewRubyTextFrame(nsIPresShell* aPresShell,
                                      nsStyleContext* aContext);

class nsRubyTextFrame MOZ_FINAL : public nsRubyTextFrameSuper
{
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsRubyTextFrame)
  NS_DECL_QUERYFRAME

  
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;
  virtual bool CanContinueTextRun() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual void Reflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus) MOZ_OVERRIDE;

protected:
  friend nsContainerFrame* NS_NewRubyTextFrame(nsIPresShell* aPresShell,
                                               nsStyleContext* aContext);
  explicit nsRubyTextFrame(nsStyleContext* aContext)
    : nsRubyTextFrameSuper(aContext) {}
};

#endif 
