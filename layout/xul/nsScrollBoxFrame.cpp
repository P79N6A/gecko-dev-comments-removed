




#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsButtonBoxFrame.h"
#include "nsITimer.h"
#include "nsRepeatService.h"
#include "mozilla/MouseEvents.h"
#include "nsIContent.h"

using namespace mozilla;

class nsAutoRepeatBoxFrame : public nsButtonBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewAutoRepeatBoxFrame(nsIPresShell* aPresShell,
                                            nsStyleContext* aContext);

  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;

  virtual nsresult AttributeChanged(int32_t aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t aModType) override;

  virtual nsresult HandleEvent(nsPresContext* aPresContext,
                               WidgetGUIEvent* aEvent,
                               nsEventStatus* aEventStatus) override;

  NS_IMETHOD HandlePress(nsPresContext* aPresContext,
                         WidgetGUIEvent* aEvent,
                         nsEventStatus* aEventStatus) override;

  NS_IMETHOD HandleRelease(nsPresContext* aPresContext,
                           WidgetGUIEvent* aEvent,
                           nsEventStatus* aEventStatus) override;

protected:
  explicit nsAutoRepeatBoxFrame(nsStyleContext* aContext):
    nsButtonBoxFrame(aContext) {}
  
  void StartRepeat() {
    if (IsActivatedOnHover()) {
      
      nsRepeatService::GetInstance()->Start(Notify, this, 0);
    } else {
      nsRepeatService::GetInstance()->Start(Notify, this);
    }
  }
  void StopRepeat() {
    nsRepeatService::GetInstance()->Stop(Notify, this);
  }
  void Notify();
  static void Notify(void* aData) {
    static_cast<nsAutoRepeatBoxFrame*>(aData)->Notify();
  }

  bool mTrustedEvent;
  
  bool IsActivatedOnHover();
};

nsIFrame*
NS_NewAutoRepeatBoxFrame (nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsAutoRepeatBoxFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsAutoRepeatBoxFrame)

nsresult
nsAutoRepeatBoxFrame::HandleEvent(nsPresContext* aPresContext,
                                  WidgetGUIEvent* aEvent,
                                  nsEventStatus* aEventStatus)
{  
  NS_ENSURE_ARG_POINTER(aEventStatus);
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  switch(aEvent->message)
  {
    
    
    
    case NS_MOUSE_ENTER:
    case NS_MOUSE_ENTER_SYNTH:
      if (IsActivatedOnHover()) {
        StartRepeat();
        mTrustedEvent = aEvent->mFlags.mIsTrusted;
      }
      break;

    case NS_MOUSE_EXIT:
    case NS_MOUSE_EXIT_SYNTH:
      
      StopRepeat();
      
      mTrustedEvent = false;
      break;

    case NS_MOUSE_CLICK: {
      WidgetMouseEvent* mouseEvent = aEvent->AsMouseEvent();
      if (mouseEvent->IsLeftClickEvent()) {
        
        return nsBoxFrame::HandleEvent(aPresContext, mouseEvent, aEventStatus);
      }
      break;
    }
  }
     
  return nsButtonBoxFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}

NS_IMETHODIMP
nsAutoRepeatBoxFrame::HandlePress(nsPresContext* aPresContext,
                                  WidgetGUIEvent* aEvent,
                                  nsEventStatus* aEventStatus)
{
  if (!IsActivatedOnHover()) {
    StartRepeat();
    mTrustedEvent = aEvent->mFlags.mIsTrusted;
    DoMouseClick(aEvent, mTrustedEvent);
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsAutoRepeatBoxFrame::HandleRelease(nsPresContext* aPresContext,
                                    WidgetGUIEvent* aEvent,
                                    nsEventStatus* aEventStatus)
{
  if (!IsActivatedOnHover()) {
    StopRepeat();
  }
  return NS_OK;
}

nsresult
nsAutoRepeatBoxFrame::AttributeChanged(int32_t aNameSpaceID,
                                       nsIAtom* aAttribute,
                                       int32_t aModType)
{
  if (aAttribute == nsGkAtoms::type) {
    StopRepeat();
  }
  return NS_OK;
}

void
nsAutoRepeatBoxFrame::Notify()
{
  DoMouseClick(nullptr, mTrustedEvent);
}

void
nsAutoRepeatBoxFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  
  
  StopRepeat();
  nsButtonBoxFrame::DestroyFrom(aDestructRoot);
}

bool
nsAutoRepeatBoxFrame::IsActivatedOnHover()
{
  return mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::repeat,
                               nsGkAtoms::hover, eCaseMatters);
}
