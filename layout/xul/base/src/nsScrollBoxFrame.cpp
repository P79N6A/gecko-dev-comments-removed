







































#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsGUIEvent.h"
#include "nsButtonBoxFrame.h"
#include "nsITimer.h"
#include "nsRepeatService.h"

class nsAutoRepeatBoxFrame : public nsButtonBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewAutoRepeatBoxFrame(nsIPresShell* aPresShell,
                                            nsStyleContext* aContext);

  virtual void Destroy();

  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType);

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  NS_IMETHOD HandlePress(nsPresContext* aPresContext, 
                         nsGUIEvent*     aEvent,
                         nsEventStatus*  aEventStatus);

  NS_IMETHOD HandleRelease(nsPresContext* aPresContext, 
                           nsGUIEvent*     aEvent,
                           nsEventStatus*  aEventStatus);

protected:
  nsAutoRepeatBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext):
    nsButtonBoxFrame(aPresShell, aContext) {}
  
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

  PRPackedBool mTrustedEvent;
  
  PRBool IsActivatedOnHover();
};

nsIFrame*
NS_NewAutoRepeatBoxFrame (nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsAutoRepeatBoxFrame (aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsAutoRepeatBoxFrame)

NS_IMETHODIMP
nsAutoRepeatBoxFrame::HandleEvent(nsPresContext* aPresContext, 
                                      nsGUIEvent* aEvent,
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
        mTrustedEvent = NS_IS_TRUSTED_EVENT(aEvent);
      }
      break;

    case NS_MOUSE_EXIT:
    case NS_MOUSE_EXIT_SYNTH:
      
      StopRepeat();
      
      mTrustedEvent = PR_FALSE;
      break;

    case NS_MOUSE_CLICK:
      if (NS_IS_MOUSE_LEFT_CLICK(aEvent)) {
        
         return nsBoxFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
      }
      break;
  }
     
  return nsButtonBoxFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}

NS_IMETHODIMP
nsAutoRepeatBoxFrame::HandlePress(nsPresContext* aPresContext, 
                                  nsGUIEvent* aEvent,
                                  nsEventStatus* aEventStatus)
{
  if (!IsActivatedOnHover()) {
    StartRepeat();
    mTrustedEvent = NS_IS_TRUSTED_EVENT(aEvent);
    DoMouseClick(aEvent, mTrustedEvent);
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsAutoRepeatBoxFrame::HandleRelease(nsPresContext* aPresContext, 
                                    nsGUIEvent* aEvent,
                                    nsEventStatus* aEventStatus)
{
  if (!IsActivatedOnHover()) {
    StopRepeat();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsAutoRepeatBoxFrame::AttributeChanged(PRInt32 aNameSpaceID,
                                       nsIAtom* aAttribute,
                                       PRInt32 aModType)
{
  if (aAttribute == nsGkAtoms::type) {
    StopRepeat();
  }
  return NS_OK;
}

void
nsAutoRepeatBoxFrame::Notify()
{
  DoMouseClick(nsnull, mTrustedEvent);
}

void
nsAutoRepeatBoxFrame::Destroy()
{
  
  
  StopRepeat();
  nsButtonBoxFrame::Destroy();
}

PRBool
nsAutoRepeatBoxFrame::IsActivatedOnHover()
{
  return mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::repeat,
                               nsGkAtoms::hover, eCaseMatters);
}
