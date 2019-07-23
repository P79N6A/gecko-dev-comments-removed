







































#ifndef nsMathMLmfracFrame_h___
#define nsMathMLmfracFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"









































class nsMathMLmfracFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmfracFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual void
  SetAdditionalStyleContext(PRInt32          aIndex, 
                            nsStyleContext*  aStyleContext);
  virtual nsStyleContext*
  GetAdditionalStyleContext(PRInt32 aIndex) const;

  virtual eMathMLFrameType GetMathMLFrameType();

  NS_IMETHOD
  AttributeChanged(PRInt32         aNameSpaceID,
                   nsIAtom*        aAttribute,
                   PRInt32         aModType);

  NS_IMETHOD
  Init(nsIContent*      aContent,
       nsIFrame*        aParent,
       nsIFrame*        aPrevInFlow);

  NS_IMETHOD
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus);

  virtual nscoord
  GetIntrinsicWidth(nsIRenderingContext* aRenderingContext);

  virtual nsresult
  Place(nsIRenderingContext& aRenderingContext,
        PRBool               aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  NS_IMETHOD
  TransmitAutomaticData();

  NS_IMETHOD
  UpdatePresentationDataFromChildAt(PRInt32         aFirstIndex,
                                    PRInt32         aLastIndex,
                                    PRUint32        aFlagsValues,
                                    PRUint32        aFlagsToUpdate);

  
  virtual nscoord
  FixInterFrameSpacing(nsHTMLReflowMetrics& aDesiredSize);

  
  static nscoord 
  CalcLineThickness(nsPresContext*  aPresContext,
                    nsStyleContext*  aStyleContext,
                    nsString&        aThicknessAttribute,
                    nscoord          onePixel,
                    nscoord          aDefaultRuleThickness);

protected:
  nsMathMLmfracFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmfracFrame();
  
  virtual PRIntn GetSkipSides() const { return 0; }

  PRBool
  IsBevelled();

  nsRect  mLineRect;
  nsMathMLChar* mSlashChar;
};

#endif 
