




#ifndef nsLegendFrame_h___
#define nsLegendFrame_h___

#include "mozilla/Attributes.h"
#include "nsBlockFrame.h"

class nsLegendFrame : public nsBlockFrame {
public:
  NS_DECL_QUERYFRAME_TARGET(nsLegendFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  explicit nsLegendFrame(nsStyleContext* aContext) : nsBlockFrame(aContext) {}

  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) override;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;

  virtual nsIAtom* GetType() const override;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif

  int32_t GetAlign();
};


#endif 
