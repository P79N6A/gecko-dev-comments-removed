




#ifndef nsMathMLmmultiscriptsFrame_h___
#define nsMathMLmmultiscriptsFrame_h___

#include "mozilla/Attributes.h"
#include "nsMathMLContainerFrame.h"








class nsMathMLmmultiscriptsFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmmultiscriptsFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  TransmitAutomaticData() MOZ_OVERRIDE;

  virtual nsresult
  Place(nsRenderingContext& aRenderingContext,
        bool                 aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize) MOZ_OVERRIDE;

  static nsresult
  PlaceMultiScript(nsPresContext*      aPresContext,
                    nsRenderingContext& aRenderingContext,
                    bool                 aPlaceOrigin,
                    nsHTMLReflowMetrics& aDesiredSize,
                    nsMathMLContainerFrame* aForFrame,
                    nscoord              aUserSubScriptShift,
                    nscoord              aUserSupScriptShift);

  uint8_t
  ScriptIncrement(nsIFrame* aFrame) MOZ_OVERRIDE;

protected:
  explicit nsMathMLmmultiscriptsFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmmultiscriptsFrame();
  

};

#endif 
