




































#ifndef nsLegendFrame_h___
#define nsLegendFrame_h___

#include "nsBlockFrame.h"
#include "nsPresContext.h"
#include "nsCOMPtr.h"

class  nsIContent;
class  nsIFrame;
class  nsPresContext;
struct nsHTMLReflowMetrics;
class  nsIRenderingContext;
struct nsRect;

class nsLegendFrame : public nsBlockFrame {
public:
  NS_DECLARE_FRAME_ACCESSOR(nsLegendFrame)

  nsLegendFrame(nsStyleContext* aContext) : nsBlockFrame(aContext) {}

  NS_DECL_QUERYFRAME

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
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
