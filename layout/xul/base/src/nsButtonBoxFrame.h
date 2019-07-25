




































#ifndef nsButtonBoxFrame_h___
#define nsButtonBoxFrame_h___

#include "nsBoxFrame.h"

class nsButtonBoxFrame : public nsBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewButtonBoxFrame(nsIPresShell* aPresShell);

  nsButtonBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
    :nsBoxFrame(aPresShell, aContext, PR_FALSE) {
    UpdateMouseThrough();
  }

  NS_IMETHOD BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists);

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                                      nsGUIEvent* aEvent,
                                      nsEventStatus* aEventStatus);

  virtual void MouseClicked (nsPresContext* aPresContext, nsGUIEvent* aEvent)
  { DoMouseClick(aEvent, PR_FALSE); }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("ButtonBoxFrame"), aResult);
  }
#endif

  



  void DoMouseClick(nsGUIEvent* aEvent, bool aTrustEvent);
  void UpdateMouseThrough() { AddStateBits(NS_FRAME_MOUSE_THROUGH_NEVER); }
}; 

#endif 
