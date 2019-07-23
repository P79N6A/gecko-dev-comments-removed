






































#include "nsDOMMouseEvent.h"
#include "nsGUIEvent.h"
#include "nsIContent.h"
#include "nsIEventStateManager.h"
#include "nsContentUtils.h"

nsDOMMouseEvent::nsDOMMouseEvent(nsPresContext* aPresContext,
                                 nsInputEvent* aEvent)
  : nsDOMUIEvent(aPresContext, aEvent ? aEvent :
                 new nsMouseEvent(PR_FALSE, 0, nsnull,
                                  nsMouseEvent::eReal))
{
  
  
  
  
  if (aEvent) {
    mEventIsInternal = PR_FALSE;
  }
  else {
    mEventIsInternal = PR_TRUE;
    mEvent->time = PR_Now();
    mEvent->refPoint.x = mEvent->refPoint.y = 0;
  }

  switch (mEvent->eventStructType)
  {
    case NS_MOUSE_EVENT:
      mDetail = NS_STATIC_CAST(nsMouseEvent*, mEvent)->clickCount;
      break;
    case NS_MOUSE_SCROLL_EVENT:
      mDetail = NS_STATIC_CAST(nsMouseScrollEvent*, mEvent)->delta;
      break;
    default:
      break;
  }
}

nsDOMMouseEvent::~nsDOMMouseEvent()
{
  if (mEventIsInternal) {
    switch (mEvent->eventStructType)
    {
      case NS_MOUSE_EVENT:
        delete NS_STATIC_CAST(nsMouseEvent*, mEvent);
        break;
      case NS_MOUSE_SCROLL_EVENT:
        delete NS_STATIC_CAST(nsMouseScrollEvent*, mEvent);
        break;
      default:
        delete mEvent;
        break;
    }
    mEvent = nsnull;
  }
}

NS_IMPL_ADDREF_INHERITED(nsDOMMouseEvent, nsDOMUIEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMMouseEvent, nsDOMUIEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMMouseEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(MouseEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMUIEvent)

NS_IMETHODIMP
nsDOMMouseEvent::InitMouseEvent(const nsAString & aType, PRBool aCanBubble, PRBool aCancelable,
                                nsIDOMAbstractView *aView, PRInt32 aDetail, PRInt32 aScreenX, 
                                PRInt32 aScreenY, PRInt32 aClientX, PRInt32 aClientY, 
                                PRBool aCtrlKey, PRBool aAltKey, PRBool aShiftKey, 
                                PRBool aMetaKey, PRUint16 aButton, nsIDOMEventTarget *aRelatedTarget)
{
  nsresult rv = nsDOMUIEvent::InitUIEvent(aType, aCanBubble, aCancelable, aView, aDetail);
  NS_ENSURE_SUCCESS(rv, rv);
  
  switch(mEvent->eventStructType)
  {
    case NS_MOUSE_EVENT:
    case NS_MOUSE_SCROLL_EVENT:
    {
       NS_STATIC_CAST(nsMouseEvent_base*, mEvent)->relatedTarget = aRelatedTarget;
       NS_STATIC_CAST(nsMouseEvent_base*, mEvent)->button = aButton;
       nsInputEvent* inputEvent = NS_STATIC_CAST(nsInputEvent*, mEvent);
       inputEvent->isControl = aCtrlKey;
       inputEvent->isAlt = aAltKey;
       inputEvent->isShift = aShiftKey;
       inputEvent->isMeta = aMetaKey;
       mClientPoint.x = aClientX;
       mClientPoint.y = aClientY;
       inputEvent->refPoint.x = aScreenX;
       inputEvent->refPoint.y = aScreenY;

       if (mEvent->eventStructType == NS_MOUSE_SCROLL_EVENT) {
         nsMouseScrollEvent* scrollEvent = NS_STATIC_CAST(nsMouseScrollEvent*, mEvent);
         scrollEvent->delta = aDetail;
       } else {
         nsMouseEvent* mouseEvent = NS_STATIC_CAST(nsMouseEvent*, mEvent);
         mouseEvent->clickCount = aDetail;
       }
       break;
    }
    default:
       break;
  }

  return NS_OK;
}   

NS_IMETHODIMP
nsDOMMouseEvent::GetButton(PRUint16* aButton)
{
  NS_ENSURE_ARG_POINTER(aButton);
  switch(mEvent->eventStructType)
  {
    case NS_MOUSE_EVENT:
    case NS_MOUSE_SCROLL_EVENT:
      *aButton = NS_STATIC_CAST(nsMouseEvent_base*, mEvent)->button;
      break;
    default:
      NS_WARNING("Tried to get mouse button for non-mouse event!");
      *aButton = nsMouseEvent::eLeftButton;
      break;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMouseEvent::GetRelatedTarget(nsIDOMEventTarget** aRelatedTarget)
{
  NS_ENSURE_ARG_POINTER(aRelatedTarget);
  *aRelatedTarget = nsnull;
  nsISupports* relatedTarget = nsnull;
  switch(mEvent->eventStructType)
  {
    case NS_MOUSE_EVENT:
    case NS_MOUSE_SCROLL_EVENT:
      relatedTarget = NS_STATIC_CAST(nsMouseEvent_base*, mEvent)->relatedTarget;
      break;
    default:
      break;
  }

  if (relatedTarget) {
    CallQueryInterface(relatedTarget, aRelatedTarget);
  }
  return NS_OK;
}

NS_METHOD nsDOMMouseEvent::GetScreenX(PRInt32* aScreenX)
{
  NS_ENSURE_ARG_POINTER(aScreenX);
  *aScreenX = GetScreenPoint().x;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMouseEvent::GetScreenY(PRInt32* aScreenY)
{
  NS_ENSURE_ARG_POINTER(aScreenY);
  *aScreenY = GetScreenPoint().y;
  return NS_OK;
}


NS_METHOD nsDOMMouseEvent::GetClientX(PRInt32* aClientX)
{
  NS_ENSURE_ARG_POINTER(aClientX);
  *aClientX = GetClientPoint().x;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMouseEvent::GetClientY(PRInt32* aClientY)
{
  NS_ENSURE_ARG_POINTER(aClientY);
  *aClientY = GetClientPoint().y;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMouseEvent::GetAltKey(PRBool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = ((nsInputEvent*)mEvent)->isAlt;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMouseEvent::GetCtrlKey(PRBool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = ((nsInputEvent*)mEvent)->isControl;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMouseEvent::GetShiftKey(PRBool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = ((nsInputEvent*)mEvent)->isShift;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMouseEvent::GetMetaKey(PRBool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = ((nsInputEvent*)mEvent)->isMeta;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMouseEvent::GetWhich(PRUint32* aWhich)
{
  NS_ENSURE_ARG_POINTER(aWhich);
  PRUint16 button;
  (void) GetButton(&button);
  *aWhich = button + 1;
  return NS_OK;
}

nsresult NS_NewDOMMouseEvent(nsIDOMEvent** aInstancePtrResult,
                             nsPresContext* aPresContext,
                             nsInputEvent *aEvent) 
{
  nsDOMMouseEvent* it = new nsDOMMouseEvent(aPresContext, aEvent);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aInstancePtrResult);
}
