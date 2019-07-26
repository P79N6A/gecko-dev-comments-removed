



#ifndef nsTitleBarFrame_h___
#define nsTitleBarFrame_h___

#include "mozilla/Attributes.h"
#include "nsBoxFrame.h"

class nsTitleBarFrame : public nsBoxFrame  
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewTitleBarFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);  

  nsTitleBarFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                                      nsGUIEvent* aEvent,
                                      nsEventStatus* aEventStatus) MOZ_OVERRIDE;

  virtual void MouseClicked(nsPresContext* aPresContext, nsGUIEvent* aEvent);

  void UpdateMouseThrough() MOZ_OVERRIDE { AddStateBits(NS_FRAME_MOUSE_THROUGH_NEVER); }

protected:
	bool mTrackingMouseMove;	
	nsIntPoint mLastPoint;

}; 

#endif 
