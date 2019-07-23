






































#ifndef nsLeafFrame_h___
#define nsLeafFrame_h___

#include "nsFrame.h"
#include "nsDisplayList.h"







class nsLeafFrame : public nsFrame {
public:

  
  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) {
    DO_GLOBAL_REFLOW_COUNT_DSP("nsLeafFrame");
    return DisplayBorderBackgroundOutline(aBuilder, aLists);
  }

  



  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);

  





  NS_IMETHOD Reflow(nsPresContext*      aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&      aStatus);

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
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
