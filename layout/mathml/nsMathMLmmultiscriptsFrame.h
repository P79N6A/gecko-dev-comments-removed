







































#ifndef nsMathMLmmultiscriptsFrame_h___
#define nsMathMLmmultiscriptsFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmmultiscriptsFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmmultiscriptsFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  TransmitAutomaticData();

  virtual nsresult
  Place(nsIRenderingContext& aRenderingContext,
        PRBool               aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize);

protected:
  nsMathMLmmultiscriptsFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmmultiscriptsFrame();
  
  virtual PRIntn GetSkipSides() const { return 0; }

private:
  nscoord mSubScriptShift;
  nscoord mSupScriptShift;

  void
  ProcessAttributes();
};

#endif 
