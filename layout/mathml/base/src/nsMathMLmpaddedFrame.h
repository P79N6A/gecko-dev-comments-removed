






































#ifndef nsMathMLmpaddedFrame_h___
#define nsMathMLmpaddedFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmpaddedFrame : public nsMathMLContainerFrame {
public:
  friend nsIFrame* NS_NewMathMLmpaddedFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent);

  NS_IMETHOD
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus);
  
protected:
  nsMathMLmpaddedFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmpaddedFrame();
  
  virtual PRIntn GetSkipSides() const { return 0; }

private:
  nsCSSValue mWidth;
  nsCSSValue mHeight;
  nsCSSValue mDepth;
  nsCSSValue mLeftSpace;

  PRInt32    mWidthSign;
  PRInt32    mHeightSign;
  PRInt32    mDepthSign;
  PRInt32    mLeftSpaceSign;

  PRInt32    mWidthPseudoUnit;
  PRInt32    mHeightPseudoUnit;
  PRInt32    mDepthPseudoUnit;
  PRInt32    mLeftSpacePseudoUnit;

  
  void
  ProcessAttributes();

  static PRBool
  ParseAttribute(nsString&   aString,
                 PRInt32&    aSign,
                 nsCSSValue& aCSSValue,
                 PRInt32&    aPseudoUnit);

  static void
  UpdateValue(nsPresContext*      aPresContext,
              nsStyleContext*      aStyleContext,
              PRInt32              aSign,
              PRInt32              aPseudoUnit,
              nsCSSValue&          aCSSValue,
              nscoord              aLeftSpace,
              nsBoundingMetrics&   aBoundingMetrics,
              nscoord&             aValueToUpdate);
};

#endif 
