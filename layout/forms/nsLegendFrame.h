




































#ifndef nsLegendFrame_h___
#define nsLegendFrame_h___

#include "nsBlockFrame.h"

class nsLegendFrame : public nsBlockFrame {
public:
  NS_DECL_QUERYFRAME_TARGET(nsLegendFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  nsLegendFrame(nsStyleContext* aContext) : nsBlockFrame(aContext) {}

  NS_IMETHOD Reflow(nsPresContext*           aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  virtual void Destroy();

  virtual nsIAtom* GetType() const;

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  PRInt32 GetAlign();
};


#endif 
