








































#ifndef nsMathMLmsqrtFrame_h___
#define nsMathMLmsqrtFrame_h___

#include "nsMathMLmencloseFrame.h"
























class nsMathMLmsqrtFrame : public nsMathMLmencloseFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmsqrtFrame(nsIPresShell*   aPresShell,
                                          nsStyleContext* aContext);

  NS_IMETHOD
  Init(nsIContent* aContent,
       nsIFrame*   aParent,
       nsIFrame*   aPrevInFlow);

  NS_IMETHOD
  AttributeChanged(PRInt32         aNameSpaceID,
                   nsIAtom*        aAttribute,
                   PRInt32         aModType);

protected:
  nsMathMLmsqrtFrame(nsStyleContext* aContext);
  virtual ~nsMathMLmsqrtFrame();
};

#endif 

