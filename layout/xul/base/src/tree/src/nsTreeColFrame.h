





































#include "nsBoxFrame.h"

class nsITreeBoxObject;

nsIFrame* NS_NewTreeColFrame(nsIPresShell* aPresShell, 
                             nsStyleContext* aContext,
                             PRBool aIsRoot = PR_FALSE,
                             nsIBoxLayout* aLayoutManager = nsnull);

class nsTreeColFrame : public nsBoxFrame
{
public:
  NS_DECL_ISUPPORTS

  nsTreeColFrame(nsIPresShell* aPresShell,
                 nsStyleContext* aContext,
                 PRBool aIsRoot = nsnull,
                 nsIBoxLayout* aLayoutManager = nsnull):
    nsBoxFrame(aPresShell, aContext, aIsRoot, aLayoutManager) {}

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
                                      PRBool aIsRoot,
                                      nsIBoxLayout* aLayoutManager);

protected:
  virtual ~nsTreeColFrame();

  


  nsITreeBoxObject* GetTreeBoxObject();

  



  void InvalidateColumns();
};
