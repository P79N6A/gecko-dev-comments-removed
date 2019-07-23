






































#include "nsBoxFrame.h"
#include "nsGridRowLeafFrame.h"

nsIFrame* NS_NewListItemFrame(nsIPresShell* aPresShell,
                              nsStyleContext *aContext);

class nsListItemFrame : public nsGridRowLeafFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewListItemFrame(nsIPresShell* aPresShell,
                                       nsStyleContext *aContext);

  
  
  NS_IMETHOD BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists);

  
  virtual nsSize GetPrefSize(nsBoxLayoutState& aState);
  
protected:
  nsListItemFrame(nsIPresShell* aPresShell,
                  nsStyleContext *aContext,
                  PRBool aIsRoot = nsnull,
                  nsIBoxLayout* aLayoutManager = nsnull);
  virtual ~nsListItemFrame();

}; 
