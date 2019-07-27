




#ifndef nsMathMLmfracFrame_h___
#define nsMathMLmfracFrame_h___

#include "mozilla/Attributes.h"
#include "nsMathMLContainerFrame.h"









































class nsMathMLmfracFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmfracFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual eMathMLFrameType GetMathMLFrameType() MOZ_OVERRIDE;

  virtual nsresult
  MeasureForWidth(nsRenderingContext& aRenderingContext,
                  nsHTMLReflowMetrics& aDesiredSize) MOZ_OVERRIDE;

  virtual nsresult
  Place(nsRenderingContext& aRenderingContext,
        bool                 aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize) MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  NS_IMETHOD
  TransmitAutomaticData() MOZ_OVERRIDE;

  
  virtual nscoord
  FixInterFrameSpacing(nsHTMLReflowMetrics& aDesiredSize) MOZ_OVERRIDE;

  
  static nscoord 
  CalcLineThickness(nsPresContext*  aPresContext,
                    nsStyleContext*  aStyleContext,
                    nsString&        aThicknessAttribute,
                    nscoord          onePixel,
                    nscoord          aDefaultRuleThickness);

  uint8_t
  ScriptIncrement(nsIFrame* aFrame) MOZ_OVERRIDE;

protected:
  explicit nsMathMLmfracFrame(nsStyleContext* aContext) : nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLmfracFrame();
  
  nsresult PlaceInternal(nsRenderingContext& aRenderingContext,
                         bool                 aPlaceOrigin,
                         nsHTMLReflowMetrics& aDesiredSize,
                         bool                 aWidthOnly);

  
  void DisplaySlash(nsDisplayListBuilder* aBuilder,
                    nsIFrame* aFrame, const nsRect& aRect,
                    nscoord aThickness,
                    const nsDisplayListSet& aLists);

  nsRect        mLineRect;
  nsMathMLChar* mSlashChar;
  nscoord       mLineThickness;
  bool          mIsBevelled;
};

#endif 
