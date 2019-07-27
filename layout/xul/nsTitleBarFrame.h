



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

  explicit nsTitleBarFrame(nsStyleContext* aContext);

  virtual void BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                           const nsRect&           aDirtyRect,
                                           const nsDisplayListSet& aLists) override;

  virtual nsresult HandleEvent(nsPresContext* aPresContext,
                               mozilla::WidgetGUIEvent* aEvent,
                               nsEventStatus* aEventStatus) override;

  virtual void MouseClicked(nsPresContext* aPresContext,
                            mozilla::WidgetMouseEvent* aEvent);

  void UpdateMouseThrough() override { AddStateBits(NS_FRAME_MOUSE_THROUGH_NEVER); }

protected:
  bool mTrackingMouseMove;
  mozilla::LayoutDeviceIntPoint mLastPoint;

}; 

#endif 
