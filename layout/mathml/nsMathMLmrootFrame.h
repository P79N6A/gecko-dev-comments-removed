




#ifndef nsMathMLmrootFrame_h___
#define nsMathMLmrootFrame_h___

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmrootFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmrootFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual void
  SetAdditionalStyleContext(int32_t          aIndex, 
                            nsStyleContext*  aStyleContext);
  virtual nsStyleContext*
  GetAdditionalStyleContext(int32_t aIndex) const;

  NS_IMETHOD
  Init(nsIContent*      aContent,
       nsIFrame*        aParent,
       nsIFrame*        aPrevInFlow);

  NS_IMETHOD
  TransmitAutomaticData() MOZ_OVERRIDE;

  NS_IMETHOD
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  virtual nscoord
  GetIntrinsicWidth(nsRenderingContext* aRenderingContext) MOZ_OVERRIDE;

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) MOZ_OVERRIDE;

protected:
  nsMathMLmrootFrame(nsStyleContext* aContext);
  virtual ~nsMathMLmrootFrame();
  
  virtual int GetSkipSides() const { return 0; }

  nsMathMLChar mSqrChar;
  nsRect       mBarRect;
};

#endif 
