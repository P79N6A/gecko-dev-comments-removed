





































#ifndef nsMathMLmspaceFrame_h___
#define nsMathMLmspaceFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmspaceFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmspaceFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual PRBool IsLeaf() const;

  NS_IMETHOD
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus);
  
protected:
  nsMathMLmspaceFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmspaceFrame();
  
  virtual PRIntn GetSkipSides() const { return 0; }

private:
  nscoord mWidth;
  nscoord mHeight;
  nscoord mDepth;

  
  void 
  ProcessAttributes(nsPresContext* aPresContext);
};

#endif 
