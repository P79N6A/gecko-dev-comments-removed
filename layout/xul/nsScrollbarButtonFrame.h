











#ifndef nsScrollbarButtonFrame_h___
#define nsScrollbarButtonFrame_h___

#include "mozilla/Attributes.h"
#include "nsButtonBoxFrame.h"
#include "nsITimer.h"
#include "nsRepeatService.h"

class nsSliderFrame;

class nsScrollbarButtonFrame : public nsButtonBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsScrollbarButtonFrame(nsIPresShell* aPresShell, nsStyleContext* aContext):
    nsButtonBoxFrame(aPresShell, aContext), mCursorOnThis(false) {}

  
  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  friend nsIFrame* NS_NewScrollbarButtonFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual nsresult HandleEvent(nsPresContext* aPresContext,
                               mozilla::WidgetGUIEvent* aEvent,
                               nsEventStatus* aEventStatus) MOZ_OVERRIDE;

  static nsresult GetChildWithTag(nsPresContext* aPresContext,
                                  nsIAtom* atom, nsIFrame* start, nsIFrame*& result);
  static nsresult GetParentWithTag(nsIAtom* atom, nsIFrame* start, nsIFrame*& result);

  bool HandleButtonPress(nsPresContext* aPresContext,
                         mozilla::WidgetGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  NS_IMETHOD HandleMultiplePress(nsPresContext* aPresContext,
                                 mozilla::WidgetGUIEvent* aEvent,
                                 nsEventStatus* aEventStatus,
                                 bool aControlHeld) MOZ_OVERRIDE
 {
   return NS_OK;
 }

  NS_IMETHOD HandleDrag(nsPresContext* aPresContext,
                        mozilla::WidgetGUIEvent* aEvent,
                        nsEventStatus* aEventStatus) MOZ_OVERRIDE
  {
    return NS_OK;
  }

  NS_IMETHOD HandleRelease(nsPresContext* aPresContext,
                           mozilla::WidgetGUIEvent* aEvent,
                           nsEventStatus* aEventStatus) MOZ_OVERRIDE;

protected:
  virtual void MouseClicked(nsPresContext* aPresContext,
                            mozilla::WidgetGUIEvent* aEvent) MOZ_OVERRIDE;

  void StartRepeat() {
    nsRepeatService::GetInstance()->Start(Notify, this);
  }
  void StopRepeat() {
    nsRepeatService::GetInstance()->Stop(Notify, this);
  }
  void Notify();
  static void Notify(void* aData) {
    static_cast<nsScrollbarButtonFrame*>(aData)->Notify();
  }
  
  bool mCursorOnThis;
};

#endif
