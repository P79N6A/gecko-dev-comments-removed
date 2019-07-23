



































#ifndef nsSelectsAreaFrame_h___
#define nsSelectsAreaFrame_h___

#include "nsAreaFrame.h"
class nsIContent;







class nsSelectsAreaFrame : public nsAreaFrame
{
public:
  friend nsIFrame* NS_NewSelectsAreaFrame(nsIPresShell* aShell, nsStyleContext* aContext, PRUint32 aFlags);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  nsresult BuildDisplayListInternal(nsDisplayListBuilder*   aBuilder,
                                    const nsRect&           aDirtyRect,
                                    const nsDisplayListSet& aLists);

  NS_IMETHOD Reflow(nsPresContext*          aCX,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  static PRBool IsOptionElement(nsIContent* aContent);
  static PRBool IsOptionElementFrame(nsIFrame *aFrame);
  
  nscoord HeightOfARow() const { return mHeightOfARow; }
  
protected:
  nsSelectsAreaFrame(nsStyleContext* aContext) :
    nsAreaFrame(aContext),
    mHeightOfARow(0)
  {}

  
  
  
  
  nscoord mHeightOfARow;
};

#endif 
