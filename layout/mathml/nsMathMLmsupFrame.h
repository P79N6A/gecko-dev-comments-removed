







































#ifndef nsMathMLmsupFrame_h___
#define nsMathMLmsupFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmsupFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmsupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  TransmitAutomaticData();

  virtual nsresult
  Place(nsIRenderingContext& aRenderingContext,
        PRBool               aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize);

  static nsresult
  PlaceSuperScript (nsPresContext*      aPresContext,
                    nsIRenderingContext& aRenderingContext,
                    PRBool               aPlaceOrigin,
                    nsHTMLReflowMetrics& aDesiredSize,
                    nsMathMLContainerFrame* aForFrame,
                    nscoord              aUserSupScriptShift,
                    nscoord              aScriptSpace);

protected:
  nsMathMLmsupFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmsupFrame();
  
  virtual PRIntn GetSkipSides() const { return 0; }
};

#endif 
