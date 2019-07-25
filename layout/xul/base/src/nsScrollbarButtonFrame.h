











#ifndef nsScrollbarButtonFrame_h___
#define nsScrollbarButtonFrame_h___

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

  
  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  friend nsIFrame* NS_NewScrollbarButtonFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  static nsresult GetChildWithTag(nsPresContext* aPresContext,
                                  nsIAtom* atom, nsIFrame* start, nsIFrame*& result);
  static nsresult GetParentWithTag(nsIAtom* atom, nsIFrame* start, nsIFrame*& result);

  bool HandleButtonPress(nsPresContext* aPresContext,
                         nsGUIEvent *    aEvent,
                         nsEventStatus*  aEventStatus);

  NS_IMETHOD HandleMultiplePress(nsPresContext* aPresContext,
                                 nsGUIEvent *    aEvent,
                                 nsEventStatus*  aEventStatus,
                                 bool aControlHeld)  { return NS_OK; }

  NS_IMETHOD HandleDrag(nsPresContext* aPresContext,
                        nsGUIEvent *    aEvent,
                        nsEventStatus*  aEventStatus) { return NS_OK; }

  NS_IMETHOD HandleRelease(nsPresContext* aPresContext,
                           nsGUIEvent *    aEvent,
                           nsEventStatus*  aEventStatus);

protected:
  virtual void MouseClicked(nsPresContext* aPresContext, nsGUIEvent* aEvent);
  void DoButtonAction(bool aSmoothScroll);

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
  
  PRInt32 mIncrement;  
  bool mCursorOnThis;
};

#endif
