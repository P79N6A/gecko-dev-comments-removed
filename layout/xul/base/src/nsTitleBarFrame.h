




































#ifndef nsTitleBarFrame_h___
#define nsTitleBarFrame_h___

#include "nsBoxFrame.h"

class nsTitleBarFrame : public nsBoxFrame  
{

public:
  friend nsIFrame* NS_NewTitleBarFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);  

  nsTitleBarFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists);

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                                      nsGUIEvent* aEvent,
                                      nsEventStatus* aEventStatus);

  virtual PRBool GetMouseThrough() const { return PR_FALSE; }

  virtual void MouseClicked(nsPresContext* aPresContext, nsGUIEvent* aEvent);

protected:
	PRBool mTrackingMouseMove;	
	nsIntPoint mLastPoint;


}; 

#endif 
