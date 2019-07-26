






#ifndef nsLeafFrame_h___
#define nsLeafFrame_h___

#include "mozilla/Attributes.h"
#include "nsFrame.h"
#include "nsDisplayList.h"







class nsLeafFrame : public nsFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE {
    DO_GLOBAL_REFLOW_COUNT_DSP("nsLeafFrame");
    DisplayBorderBackgroundOutline(aBuilder, aLists);
  }

  



  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);

  


  virtual nsSize ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, bool aShrinkWrap);

  





  NS_IMETHOD Reflow(nsPresContext*      aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&      aStatus) MOZ_OVERRIDE;
  
  


  NS_IMETHOD DoReflow(nsPresContext*      aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&      aStatus);

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    
    
    return nsFrame::IsFrameOfType(aFlags & ~(nsIFrame::eReplacedContainsBlock));
  }

protected:
  nsLeafFrame(nsStyleContext* aContext) : nsFrame(aContext) {}
  virtual ~nsLeafFrame();

  




  virtual nscoord GetIntrinsicWidth() = 0;

  






  virtual nscoord GetIntrinsicHeight();

  


  void AddBordersAndPadding(const nsHTMLReflowState& aReflowState,
                            nsHTMLReflowMetrics& aDesiredSize);

  


  void SizeToAvailSize(const nsHTMLReflowState& aReflowState,
                       nsHTMLReflowMetrics& aDesiredSize);
};

#endif 
