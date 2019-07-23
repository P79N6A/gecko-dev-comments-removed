







































#ifndef nsMathMLmsubFrame_h___
#define nsMathMLmsubFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmsubFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmsubFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  TransmitAutomaticData();

  virtual nsresult
  Place(nsIRenderingContext& aRenderingContext,
        PRBool               aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize);

  static nsresult
  PlaceSubScript (nsPresContext*      aPresContext,
                  nsIRenderingContext& aRenderingContext,
                  PRBool               aPlaceOrigin,
                  nsHTMLReflowMetrics& aDesiredSize,
                  nsMathMLContainerFrame* aForFrame,
                  nscoord              aUserSubScriptShift,
                  nscoord              aScriptSpace);

 protected:
  nsMathMLmsubFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmsubFrame();
  
  virtual PRIntn GetSkipSides() const { return 0; }
};

#endif 
