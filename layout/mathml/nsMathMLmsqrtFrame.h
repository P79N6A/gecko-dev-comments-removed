




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
  InheritAutomaticData(nsIFrame* aParent);

  NS_IMETHOD
  AttributeChanged(int32_t         aNameSpaceID,
                   nsIAtom*        aAttribute,
                   int32_t         aModType);

protected:
  nsMathMLmsqrtFrame(nsStyleContext* aContext);
  virtual ~nsMathMLmsqrtFrame();
};

#endif 

