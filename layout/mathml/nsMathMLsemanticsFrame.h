




































#ifndef nsMathMLsemanticsFrame_h___
#define nsMathMLsemanticsFrame_h___

#include "nsMathMLContainerFrame.h"





class nsMathMLsemanticsFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLsemanticsFrame(nsIPresShell* aPresShell,
                                              nsStyleContext* aContext);

  NS_IMETHOD
  TransmitAutomaticData();

protected:
  nsMathMLsemanticsFrame(nsStyleContext* aContext) :
    nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLsemanticsFrame();
};

#endif 
