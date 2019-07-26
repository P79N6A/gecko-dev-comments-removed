



#ifndef nsTitleBarFrame_h___
#define nsTitleBarFrame_h___

#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "nsBoxFrame.h"

class nsTitleBarFrame : public nsBoxFrame  
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewTitleBarFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);  

  nsTitleBarFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual void BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                           const nsRect&           aDirtyRect,
                                           const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual nsresult HandleEvent(nsPresContext* aPresContext,
                         mozilla::WidgetGUIEvent* aEvent,
                         nsEventStatus* aEventStatus) MOZ_OVERRIDE;

  virtual void MouseClicked(nsPresContext* aPresContext,
                            mozilla::WidgetMouseEvent* aEvent);

  void UpdateMouseThrough() MOZ_OVERRIDE { AddStateBits(NS_FRAME_MOUSE_THROUGH_NEVER); }

protected:
	bool mTrackingMouseMove;	
	nsIntPoint mLastPoint;

}; 

#endif 
