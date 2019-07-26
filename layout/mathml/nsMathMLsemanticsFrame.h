




#ifndef nsMathMLsemanticsFrame_h___
#define nsMathMLsemanticsFrame_h___

#include "nsMathMLSelectedFrame.h"





class nsMathMLsemanticsFrame : public nsMathMLSelectedFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLsemanticsFrame(nsIPresShell* aPresShell,
                                              nsStyleContext* aContext);

protected:
  nsMathMLsemanticsFrame(nsStyleContext* aContext) :
    nsMathMLSelectedFrame(aContext) {}
  virtual ~nsMathMLsemanticsFrame();

  nsIFrame* GetSelectedFrame();
};

#endif 
