








































#ifndef nsMathMLmsqrtFrame_h___
#define nsMathMLmsqrtFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"




























class nsMathMLmsqrtFrame : public nsMathMLContainerFrame {
public:
  friend nsIFrame* NS_NewMathMLmsqrtFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual void
  SetAdditionalStyleContext(PRInt32          aIndex, 
                            nsStyleContext*  aStyleContext);
  virtual nsStyleContext*
  GetAdditionalStyleContext(PRInt32 aIndex) const;

  NS_IMETHOD
  Init(nsIContent* aContent,
       nsIFrame*   aParent,
       nsIFrame*   aPrevInFlow);

  virtual nsresult
  Place(nsIRenderingContext& aRenderingContext,
        PRBool               aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize);

  virtual nscoord
  GetIntrinsicWidth(nsIRenderingContext* aRenderingContext);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent);

  NS_IMETHOD
  TransmitAutomaticData();

  
  
  
  
  virtual nscoord
  FixInterFrameSpacing(nsHTMLReflowMetrics& aDesiredSize);

protected:
  nsMathMLmsqrtFrame(nsStyleContext* aContext);
  virtual ~nsMathMLmsqrtFrame();
  
  virtual PRIntn GetSkipSides() const { return 0; }

  virtual nsresult
  MeasureChildFrames(nsIRenderingContext& aRenderingContext,
                     nsHTMLReflowMetrics& aDesiredSize);

  nsMathMLChar mSqrChar;
  nsRect       mBarRect;
};

#endif 

