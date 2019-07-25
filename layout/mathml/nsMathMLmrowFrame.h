




#ifndef nsMathMLmrowFrame_h___
#define nsMathMLmrowFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmrowFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmrowFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  AttributeChanged(int32_t  aNameSpaceID,
                   nsIAtom* aAttribute,
                   int32_t  aModType);

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent);

  NS_IMETHOD
  TransmitAutomaticData() {
    return TransmitAutomaticDataForMrowLikeElement();
  }

protected:
  nsMathMLmrowFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmrowFrame();

  virtual int GetSkipSides() const { return 0; }
};

#endif 
