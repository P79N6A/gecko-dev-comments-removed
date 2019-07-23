






































#include "nsDOMKeyboardEvent.h"
#include "nsContentUtils.h"


nsDOMKeyboardEvent::nsDOMKeyboardEvent(nsPresContext* aPresContext,
                                       nsKeyEvent* aEvent)
  : nsDOMUIEvent(aPresContext, aEvent ? aEvent :
                 new nsKeyEvent(PR_FALSE, 0, nsnull))
{
  NS_ASSERTION(mEvent->eventStructType == NS_KEY_EVENT, "event type mismatch");

  if (aEvent) {
    mEventIsInternal = PR_FALSE;
  }
  else {
    mEventIsInternal = PR_TRUE;
    mEvent->time = PR_Now();
  }
}

NS_IMPL_ADDREF_INHERITED(nsDOMKeyboardEvent, nsDOMUIEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMKeyboardEvent, nsDOMUIEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMKeyboardEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMKeyEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(KeyboardEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMUIEvent)

NS_IMETHODIMP
nsDOMKeyboardEvent::GetAltKey(PRBool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = ((nsInputEvent*)mEvent)->isAlt;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMKeyboardEvent::GetCtrlKey(PRBool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = ((nsInputEvent*)mEvent)->isControl;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMKeyboardEvent::GetShiftKey(PRBool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = ((nsInputEvent*)mEvent)->isShift;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMKeyboardEvent::GetMetaKey(PRBool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = ((nsInputEvent*)mEvent)->isMeta;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMKeyboardEvent::GetCharCode(PRUint32* aCharCode)
{
  NS_ENSURE_ARG_POINTER(aCharCode);

  switch (mEvent->message) {
  case NS_KEY_UP:
  case NS_KEY_DOWN:
    NS_WARNING("GetCharCode used for wrong key event; should use onkeypress.");
    *aCharCode = 0;
    break;
  case NS_KEY_PRESS:
    *aCharCode = ((nsKeyEvent*)mEvent)->charCode;
    break;
  default:
    break;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMKeyboardEvent::GetKeyCode(PRUint32* aKeyCode)
{
  NS_ENSURE_ARG_POINTER(aKeyCode);

  switch (mEvent->message) {
  case NS_KEY_UP:
  case NS_KEY_PRESS:
  case NS_KEY_DOWN:
    *aKeyCode = ((nsKeyEvent*)mEvent)->keyCode;
    break;
  default:
    *aKeyCode = 0;
    break;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMKeyboardEvent::GetWhich(PRUint32* aWhich)
{
  NS_ENSURE_ARG_POINTER(aWhich);

  switch (mEvent->message) {
    case NS_KEY_UP:
    case NS_KEY_DOWN:
      return GetKeyCode(aWhich);
    case NS_KEY_PRESS:
      
      
      {
        PRUint32 keyCode = ((nsKeyEvent*)mEvent)->keyCode;
        if (keyCode == NS_VK_RETURN || keyCode == NS_VK_BACK) {
          *aWhich = keyCode;
          return NS_OK;
        }
	return GetCharCode(aWhich);
      }
      break;
    default:
      *aWhich = 0;
      break;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMKeyboardEvent::InitKeyEvent(const nsAString& aType, PRBool aCanBubble, PRBool aCancelable,
                                 nsIDOMAbstractView* aView, PRBool aCtrlKey, PRBool aAltKey,
                                 PRBool aShiftKey, PRBool aMetaKey,
                                 PRUint32 aKeyCode, PRUint32 aCharCode)
{
  nsresult rv = nsDOMUIEvent::InitUIEvent(aType, aCanBubble, aCancelable, aView, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  nsKeyEvent* keyEvent = static_cast<nsKeyEvent*>(mEvent);
  keyEvent->isControl = aCtrlKey;
  keyEvent->isAlt = aAltKey;
  keyEvent->isShift = aShiftKey;
  keyEvent->isMeta = aMetaKey;
  keyEvent->keyCode = aKeyCode;
  keyEvent->charCode = aCharCode;

  return NS_OK;
}

nsresult NS_NewDOMKeyboardEvent(nsIDOMEvent** aInstancePtrResult,
                                nsPresContext* aPresContext,
                                nsKeyEvent *aEvent)
{
  nsDOMKeyboardEvent* it = new nsDOMKeyboardEvent(aPresContext, aEvent);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aInstancePtrResult);
}
