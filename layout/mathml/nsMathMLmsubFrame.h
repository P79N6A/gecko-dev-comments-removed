




#ifndef nsMathMLmsubFrame_h___
#define nsMathMLmsubFrame_h___

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmsubFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmsubFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  TransmitAutomaticData() MOZ_OVERRIDE;

  virtual nsresult
  Place(nsRenderingContext& aRenderingContext,
        bool                 aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize) MOZ_OVERRIDE;

  static nsresult
  PlaceSubScript (nsPresContext*      aPresContext,
                  nsRenderingContext& aRenderingContext,
                  bool                 aPlaceOrigin,
                  nsHTMLReflowMetrics& aDesiredSize,
                  nsMathMLContainerFrame* aForFrame,
                  nscoord              aUserSubScriptShift,
                  nscoord              aScriptSpace);

 protected:
  nsMathMLmsubFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmsubFrame();
};

#endif 
