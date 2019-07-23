






































#ifndef nsMathMLmphantomFrame_h___
#define nsMathMLmphantomFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmphantomFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmphantomFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) { return NS_OK; }

protected:
  nsMathMLmphantomFrame(nsStyleContext* aContext)
    : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmphantomFrame();
  
  virtual PRIntn GetSkipSides() const { return 0; }
};

#endif 
