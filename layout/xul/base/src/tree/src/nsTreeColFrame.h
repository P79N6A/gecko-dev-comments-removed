





































#include "nsBoxFrame.h"

class nsITreeBoxObject;

nsIFrame* NS_NewTreeColFrame(nsIPresShell* aPresShell, 
                             nsStyleContext* aContext);

class nsTreeColFrame : public nsBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsTreeColFrame(nsIPresShell* aPresShell,
                 nsStyleContext* aContext):
    nsBoxFrame(aPresShell, aContext) {}

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  virtual void Destroy();

  NS_IMETHOD BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists);

  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType);

  virtual void SetBounds(nsBoxLayoutState& aBoxLayoutState, const nsRect& aRect,
                         PRBool aRemoveOverflowArea = PR_FALSE);

  friend nsIFrame* NS_NewTreeColFrame(nsIPresShell* aPresShell,
                                      nsStyleContext* aContext);

protected:
  virtual ~nsTreeColFrame();

  


  nsITreeBoxObject* GetTreeBoxObject();

  



  void InvalidateColumns(PRBool aCanWalkFrameTree = PR_TRUE);
};
