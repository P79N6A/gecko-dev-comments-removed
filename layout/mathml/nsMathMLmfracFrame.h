




#ifndef nsMathMLmfracFrame_h___
#define nsMathMLmfracFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"









































class nsMathMLmfracFrame : public nsMathMLContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmfracFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual eMathMLFrameType GetMathMLFrameType();

  virtual nsresult
  MeasureForWidth(nsRenderingContext& aRenderingContext,
                  nsHTMLReflowMetrics& aDesiredSize);

  virtual nsresult
  Place(nsRenderingContext& aRenderingContext,
        bool                 aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  NS_IMETHOD
  TransmitAutomaticData();

  NS_IMETHOD
  UpdatePresentationDataFromChildAt(int32_t         aFirstIndex,
                                    int32_t         aLastIndex,
                                    uint32_t        aFlagsValues,
                                    uint32_t        aFlagsToUpdate);

  
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
  
  virtual int GetSkipSides() const { return 0; }

  nsresult PlaceInternal(nsRenderingContext& aRenderingContext,
                         bool                 aPlaceOrigin,
                         nsHTMLReflowMetrics& aDesiredSize,
                         bool                 aWidthOnly);

  
  nsresult DisplaySlash(nsDisplayListBuilder* aBuilder,
                        nsIFrame* aFrame, const nsRect& aRect,
                        nscoord aThickness,
                        const nsDisplayListSet& aLists);

  nsRect        mLineRect;
  nsMathMLChar* mSlashChar;
  nscoord       mLineThickness;
  bool          mIsBevelled;
};

#endif 
