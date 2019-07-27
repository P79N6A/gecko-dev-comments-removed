



#ifndef nsButtonBoxFrame_h___
#define nsButtonBoxFrame_h___

#include "mozilla/Attributes.h"
#include "nsBoxFrame.h"

class nsButtonBoxFrame : public nsBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewButtonBoxFrame(nsIPresShell* aPresShell);

  explicit nsButtonBoxFrame(nsStyleContext* aContext)
    :nsBoxFrame(aContext, false) {
    UpdateMouseThrough();
  }

  virtual void BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                           const nsRect&           aDirtyRect,
                                           const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual nsresult HandleEvent(nsPresContext* aPresContext, 
                               mozilla::WidgetGUIEvent* aEvent,
                               nsEventStatus* aEventStatus) MOZ_OVERRIDE;

  virtual void MouseClicked(nsPresContext* aPresContext,
                            mozilla::WidgetGUIEvent* aEvent)
  { DoMouseClick(aEvent, false); }

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE {
    return MakeFrameName(NS_LITERAL_STRING("ButtonBoxFrame"), aResult);
  }
#endif

  



  void DoMouseClick(mozilla::WidgetGUIEvent* aEvent, bool aTrustEvent);
  void UpdateMouseThrough() MOZ_OVERRIDE { AddStateBits(NS_FRAME_MOUSE_THROUGH_NEVER); }
}; 

#endif 
