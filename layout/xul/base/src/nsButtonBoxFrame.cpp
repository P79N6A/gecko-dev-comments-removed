





































#include "nsCOMPtr.h"
#include "nsButtonBoxFrame.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMXULButtonElement.h"
#include "nsGkAtoms.h"
#include "nsINameSpaceManager.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsGUIEvent.h"
#include "nsIEventStateManager.h"
#include "nsIDOMElement.h"
#include "nsDisplayList.h"






nsIFrame*
NS_NewButtonBoxFrame (nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsButtonBoxFrame(aPresShell, aContext);
} 

NS_IMETHODIMP
nsButtonBoxFrame::BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                              const nsRect&           aDirtyRect,
                                              const nsDisplayListSet& aLists)
{
  
  if (aBuilder->IsForEventDelivery())
    return NS_OK;
  return nsBoxFrame::BuildDisplayListForChildren(aBuilder, aDirtyRect, aLists);
}

NS_IMETHODIMP
nsButtonBoxFrame::HandleEvent(nsPresContext* aPresContext, 
                              nsGUIEvent* aEvent,
                              nsEventStatus* aEventStatus)
{
  switch (aEvent->message) {
    case NS_KEY_DOWN:
      if (NS_KEY_EVENT == aEvent->eventStructType) {
        nsKeyEvent* keyEvent = (nsKeyEvent*)aEvent;
        if (NS_VK_SPACE == keyEvent->keyCode) {
          nsIEventStateManager *esm = aPresContext->EventStateManager();
          
          esm->SetContentState(mContent,
                               NS_EVENT_STATE_HOVER |  NS_EVENT_STATE_ACTIVE);
        }
      }
      break;


#ifndef XP_MACOSX
    case NS_KEY_PRESS:
      if (NS_KEY_EVENT == aEvent->eventStructType) {
        nsKeyEvent* keyEvent = (nsKeyEvent*)aEvent;
        if (NS_VK_RETURN == keyEvent->keyCode) {
          nsCOMPtr<nsIDOMXULButtonElement> buttonEl(do_QueryInterface(mContent));
          if (buttonEl) {
            MouseClicked(aPresContext, aEvent);
            *aEventStatus = nsEventStatus_eConsumeNoDefault;
          }
        }
      }
      break;
#endif

    case NS_KEY_UP:
      if (NS_KEY_EVENT == aEvent->eventStructType) {
        nsKeyEvent* keyEvent = (nsKeyEvent*)aEvent;
        if (NS_VK_SPACE == keyEvent->keyCode) {
          
          PRInt32 buttonState;
          const PRInt32 activeHover = NS_EVENT_STATE_ACTIVE | NS_EVENT_STATE_HOVER;
          nsIEventStateManager *esm = aPresContext->EventStateManager();
          esm->GetContentState(mContent, buttonState);
          if ((buttonState & activeHover) == activeHover) {
            esm->SetContentState(nsnull, activeHover);    
            MouseClicked(aPresContext, aEvent);
          }
        }
      }
      break;

    case NS_MOUSE_CLICK:
      if (NS_IS_MOUSE_LEFT_CLICK(aEvent)) {
        MouseClicked(aPresContext, aEvent);
      }
      break;
  }

  return nsBoxFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}

void 
nsButtonBoxFrame::DoMouseClick(nsGUIEvent* aEvent, PRBool aTrustEvent) 
{
  
  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::disabled,
                            nsGkAtoms::_true, eCaseMatters))
    return;

  
  nsEventStatus status = nsEventStatus_eIgnore;
  nsXULCommandEvent event(aEvent ? NS_IS_TRUSTED_EVENT(aEvent) : aTrustEvent,
                          NS_XUL_COMMAND, nsnull);
  if(aEvent) {
    event.isShift = ((nsInputEvent*)(aEvent))->isShift;
    event.isControl = ((nsInputEvent*)(aEvent))->isControl;
    event.isAlt = ((nsInputEvent*)(aEvent))->isAlt;
    event.isMeta = ((nsInputEvent*)(aEvent))->isMeta;
  }

  
  nsCOMPtr<nsIPresShell> shell = GetPresContext()->GetPresShell();
  if (shell) {
    shell->HandleDOMEventWithTarget(mContent, &event, &status);
  }
}
