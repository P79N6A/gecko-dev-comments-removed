



































#ifndef nsSelectsAreaFrame_h___
#define nsSelectsAreaFrame_h___

#include "nsBlockFrame.h"
class nsIContent;

class nsSelectsAreaFrame : public nsBlockFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

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
    nsBlockFrame(aContext),
    mHeightOfARow(0)
  {}

  
  
  
  
  nscoord mHeightOfARow;
};

#endif 
