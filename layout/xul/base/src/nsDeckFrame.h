












































#ifndef nsDeckFrame_h___
#define nsDeckFrame_h___

#include "nsBoxFrame.h"

class nsDeckFrame : public nsBoxFrame
{
public:

  friend nsIFrame* NS_NewDeckFrame(nsIPresShell* aPresShell,
                                   nsStyleContext* aContext,
                                   nsIBoxLayout* aLayoutManager);

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

  NS_IMETHOD DoLayout(nsBoxLayoutState& aState);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  NS_IMETHOD BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists);
                                         
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  virtual PRBool ChildrenMustHaveWidgets() const { return PR_TRUE; }

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
      return MakeFrameName(NS_LITERAL_STRING("Deck"), aResult);
  }
#endif

  nsDeckFrame(nsIPresShell* aPresShell,
              nsStyleContext* aContext,
              nsIBoxLayout* aLayout = nsnull);

protected:

  
  nsIBox* GetSelectedBox();
  void IndexChanged(nsPresContext* aPresContext);
  PRInt32 GetSelectedIndex();
  void HideBox(nsPresContext* aPresContext, nsIBox* aBox);
  void ShowBox(nsPresContext* aPresContext, nsIBox* aBox);

private:

  PRInt32 mIndex;

}; 

#endif

