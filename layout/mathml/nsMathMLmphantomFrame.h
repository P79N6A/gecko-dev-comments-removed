




#ifndef nsMathMLmphantomFrame_h___
#define nsMathMLmphantomFrame_h___

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmphantomFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmphantomFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent);

  NS_IMETHOD
  TransmitAutomaticData() {
    return TransmitAutomaticDataForMrowLikeElement();
  }

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE {}

protected:
  nsMathMLmphantomFrame(nsStyleContext* aContext)
    : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmphantomFrame();
};

#endif 
