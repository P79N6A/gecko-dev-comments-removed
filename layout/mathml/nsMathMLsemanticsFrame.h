




#ifndef nsMathMLsemanticsFrame_h___
#define nsMathMLsemanticsFrame_h___

#include "mozilla/Attributes.h"
#include "nsMathMLSelectedFrame.h"





class nsMathMLsemanticsFrame : public nsMathMLSelectedFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLsemanticsFrame(nsIPresShell* aPresShell,
                                              nsStyleContext* aContext);

protected:
  explicit nsMathMLsemanticsFrame(nsStyleContext* aContext) :
    nsMathMLSelectedFrame(aContext) {}
  virtual ~nsMathMLsemanticsFrame();

  nsIFrame* GetSelectedFrame() MOZ_OVERRIDE;
};

#endif 
