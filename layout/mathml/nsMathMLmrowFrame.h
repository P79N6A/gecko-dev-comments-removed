






































#ifndef nsMathMLmrowFrame_h___
#define nsMathMLmrowFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmrowFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmrowFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  AttributeChanged(PRInt32  aNameSpaceID,
                   nsIAtom* aAttribute,
                   PRInt32  aModType);

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent);

protected:
  nsMathMLmrowFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmrowFrame();

  virtual PRIntn GetSkipSides() const { return 0; }
};

#endif 
