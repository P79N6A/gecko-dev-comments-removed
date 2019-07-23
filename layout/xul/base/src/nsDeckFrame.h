












































#ifndef nsDeckFrame_h___
#define nsDeckFrame_h___

#include "nsBoxFrame.h"

class nsDeckFrame : public nsBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewDeckFrame(nsIPresShell* aPresShell,
                                   nsStyleContext* aContext);

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

  
  
  NS_IMETHOD  SetInitialChildList(nsIAtom*        aListName,
                                  nsFrameList&    aChildList);
  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsFrameList&    aFrameList);
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList);

  virtual nsIAtom* GetType() const;

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
      return MakeFrameName(NS_LITERAL_STRING("Deck"), aResult);
  }
#endif

  nsDeckFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

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

