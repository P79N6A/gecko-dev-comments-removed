



#include "nsCOMPtr.h"
#include "nsButtonBoxFrame.h"
#include "nsIContent.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMXULButtonElement.h"
#include "nsGkAtoms.h"
#include "nsINameSpaceManager.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsEventStateManager.h"
#include "nsIDOMElement.h"
#include "nsDisplayList.h"
#include "nsContentUtils.h"
#include "mozilla/dom/Element.h"
#include "mozilla/TextEvents.h"

using namespace mozilla;






nsIFrame*
NS_NewButtonBoxFrame (nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsButtonBoxFrame(aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsButtonBoxFrame)

void
nsButtonBoxFrame::BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                              const nsRect&           aDirtyRect,
                                              const nsDisplayListSet& aLists)
{
  
  if (aBuilder->IsForEventDelivery())
    return;
  nsBoxFrame::BuildDisplayListForChildren(aBuilder, aDirtyRect, aLists);
}

NS_IMETHODIMP
nsButtonBoxFrame::HandleEvent(nsPresContext* aPresContext, 
                              WidgetGUIEvent* aEvent,
                              nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  switch (aEvent->message) {
    case NS_KEY_DOWN:
      if (NS_KEY_EVENT == aEvent->eventStructType) {
        WidgetKeyboardEvent* keyEvent =
          static_cast<WidgetKeyboardEvent*>(aEvent);
        if (NS_VK_SPACE == keyEvent->keyCode) {
          nsEventStateManager *esm = aPresContext->EventStateManager();
          
          esm->SetContentState(mContent, NS_EVENT_STATE_HOVER);
          esm->SetContentState(mContent, NS_EVENT_STATE_ACTIVE);
        }
      }
      break;


#ifndef XP_MACOSX
    case NS_KEY_PRESS:
      if (NS_KEY_EVENT == aEvent->eventStructType) {
        WidgetKeyboardEvent* keyEvent =
          static_cast<WidgetKeyboardEvent*>(aEvent);
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
        WidgetKeyboardEvent* keyEvent =
          static_cast<WidgetKeyboardEvent*>(aEvent);
        if (NS_VK_SPACE == keyEvent->keyCode) {
          
          NS_ASSERTION(mContent->IsElement(), "How do we have a non-element?");
          nsEventStates buttonState = mContent->AsElement()->State();
          if (buttonState.HasAllStates(NS_EVENT_STATE_ACTIVE |
                                       NS_EVENT_STATE_HOVER)) {
            
            nsEventStateManager *esm = aPresContext->EventStateManager();
            esm->SetContentState(nullptr, NS_EVENT_STATE_ACTIVE);
            esm->SetContentState(nullptr, NS_EVENT_STATE_HOVER);
            MouseClicked(aPresContext, aEvent);
          }
        }
      }
      break;

    case NS_MOUSE_CLICK:
      if (aEvent->IsLeftClickEvent()) {
        MouseClicked(aPresContext, aEvent);
      }
      break;
  }

  return nsBoxFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}

void 
nsButtonBoxFrame::DoMouseClick(WidgetGUIEvent* aEvent, bool aTrustEvent)
{
  
  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::disabled,
                            nsGkAtoms::_true, eCaseMatters))
    return;

  
  bool isShift = false;
  bool isControl = false;
  bool isAlt = false;
  bool isMeta = false;
  if(aEvent) {
    isShift = static_cast<WidgetInputEvent*>(aEvent)->IsShift();
    isControl = static_cast<WidgetInputEvent*>(aEvent)->IsControl();
    isAlt = static_cast<WidgetInputEvent*>(aEvent)->IsAlt();
    isMeta = static_cast<WidgetInputEvent*>(aEvent)->IsMeta();
  }

  
  nsCOMPtr<nsIPresShell> shell = PresContext()->GetPresShell();
  if (shell) {
    nsContentUtils::DispatchXULCommand(mContent,
                                       aEvent ?
                                         aEvent->mFlags.mIsTrusted : aTrustEvent,
                                       nullptr, shell,
                                       isControl, isAlt, isShift, isMeta);
  }
}
