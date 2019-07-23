




































#ifndef nsLegendFrame_h___
#define nsLegendFrame_h___

#include "nsAreaFrame.h"
#include "nsPresContext.h"
#include "nsCOMPtr.h"

class  nsIContent;
class  nsIFrame;
class  nsPresContext;
struct nsHTMLReflowMetrics;
class  nsIRenderingContext;
struct nsRect;

#define NS_LEGEND_FRAME_CID \
{ 0x73805d40, 0x5a24, 0x11d2, { 0x80, 0x46, 0x0, 0x60, 0x8, 0x15, 0xa7, 0x91 } }

class nsLegendFrame : public nsAreaFrame {
public:
  nsLegendFrame(nsStyleContext* aContext) : nsAreaFrame(aContext) {}

  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

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
