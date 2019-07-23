






































#ifndef nsMathMLmstyleFrame_h___
#define nsMathMLmstyleFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmstyleFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmstyleFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  AttributeChanged(PRInt32         aNameSpaceID,
                   nsIAtom*        aAttribute,
                   PRInt32         aModType);

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent);

  NS_IMETHOD
  TransmitAutomaticData();

  NS_IMETHOD
  UpdatePresentationData(PRUint32        aFlagsValues,
                         PRUint32        aFlagsToUpdate);

  NS_IMETHOD
  UpdatePresentationDataFromChildAt(PRInt32         aFirstIndex,
                                    PRInt32         aLastIndex,
                                    PRUint32        aFlagsValues,
                                    PRUint32        aFlagsToUpdate);

protected:
  nsMathMLmstyleFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmstyleFrame();

  virtual PRIntn GetSkipSides() const { return 0; }
};

#endif 
