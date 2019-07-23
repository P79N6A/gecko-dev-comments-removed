







































#ifndef nsMathMLmsubsupFrame_h___
#define nsMathMLmsubsupFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmsubsupFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmsubsupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  TransmitAutomaticData();

  virtual nsresult
  Place(nsIRenderingContext& aRenderingContext,
        PRBool               aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize);

  static nsresult
  PlaceSubSupScript(nsPresContext*      aPresContext,
                    nsIRenderingContext& aRenderingContext,
                    PRBool               aPlaceOrigin,
                    nsHTMLReflowMetrics& aDesiredSize,
                    nsMathMLContainerFrame* aForFrame,
                    nscoord              aUserSubScriptShift,
                    nscoord              aUserSupScriptShift,
                    nscoord              aScriptSpace);

protected:
  nsMathMLmsubsupFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmsubsupFrame();
  
  virtual PRIntn GetSkipSides() const { return 0; }
};

#endif 
