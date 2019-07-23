







































#ifndef nsMathMLmoverFrame_h___
#define nsMathMLmoverFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmoverFrame : public nsMathMLContainerFrame {
public:
  friend nsIFrame* NS_NewMathMLmoverFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  Place(nsIRenderingContext& aRenderingContext,
        PRBool               aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize);

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent);

  NS_IMETHOD
  TransmitAutomaticData();

  NS_IMETHOD
  UpdatePresentationData(PRInt32         aScriptLevelIncrement,
                         PRUint32        aFlagsValues,
                         PRUint32        aFlagsToUpdate);

  NS_IMETHOD
  UpdatePresentationDataFromChildAt(PRInt32         aFirstIndex,
                                    PRInt32         aLastIndex,
                                    PRInt32         aScriptLevelIncrement,
                                    PRUint32        aFlagsValues,
                                    PRUint32        aFlagsToUpdate);

  NS_IMETHOD
  AttributeChanged(PRInt32         aNameSpaceID,
                   nsIAtom*        aAttribute,
                   PRInt32         aModType);

protected:
  nsMathMLmoverFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmoverFrame();
  
  virtual PRIntn GetSkipSides() const { return 0; }
};


#endif 
