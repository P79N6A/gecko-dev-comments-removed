







































#ifndef nsMathMLmpaddedFrame_h___
#define nsMathMLmpaddedFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmpaddedFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmpaddedFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent);

  NS_IMETHOD
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus);
  
  virtual nsresult
  Place(nsIRenderingContext& aRenderingContext,
        PRBool               aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize);

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

  void
  UpdateValue(PRInt32                  aSign,
              PRInt32                  aPseudoUnit,
              const nsCSSValue&        aCSSValue,
              nscoord                  aLeftSpace,
              const nsBoundingMetrics& aBoundingMetrics,
              nscoord&                 aValueToUpdate) const;
};

#endif 
