



#ifndef nsSelectsAreaFrame_h___
#define nsSelectsAreaFrame_h___

#include "mozilla/Attributes.h"
#include "nsBlockFrame.h"

class nsSelectsAreaFrame : public nsBlockFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsContainerFrame* NS_NewSelectsAreaFrame(nsIPresShell* aShell,
                                                  nsStyleContext* aContext,
                                                  nsFrameState aFlags);

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  void BuildDisplayListInternal(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists);

  virtual void Reflow(nsPresContext*           aCX,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) override;

  nscoord BSizeOfARow() const { return mBSizeOfARow; }
  
protected:
  explicit nsSelectsAreaFrame(nsStyleContext* aContext) :
    nsBlockFrame(aContext),
    mBSizeOfARow(0)
  {}

  
  
  
  
  
  nscoord mBSizeOfARow;
};

#endif 
