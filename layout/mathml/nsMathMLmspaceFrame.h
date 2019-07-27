




#ifndef nsMathMLmspaceFrame_h___
#define nsMathMLmspaceFrame_h___

#include "mozilla/Attributes.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmspaceFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmspaceFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  TransmitAutomaticData() MOZ_OVERRIDE {
    
    
    mPresentationData.flags |= NS_MATHML_SPACE_LIKE;
    return NS_OK;
  }

  virtual bool IsLeaf() const MOZ_OVERRIDE;

  virtual void
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus) MOZ_OVERRIDE;
  
protected:
  explicit nsMathMLmspaceFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmspaceFrame();

  virtual nsresult
  MeasureForWidth(nsRenderingContext& aRenderingContext,
                  nsHTMLReflowMetrics& aDesiredSize) MOZ_OVERRIDE;

private:
  nscoord mWidth;
  nscoord mHeight;
  nscoord mDepth;

  
  void 
  ProcessAttributes(nsPresContext* aPresContext);
};

#endif 
