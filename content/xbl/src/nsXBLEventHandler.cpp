





































#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsIContent.h"
#include "nsIDOMEventGroup.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMText.h"
#include "nsIDOM3EventTarget.h"
#include "nsGkAtoms.h"
#include "nsXBLPrototypeHandler.h"
#include "nsIDOMNSEvent.h"

nsXBLEventHandler::nsXBLEventHandler(nsXBLPrototypeHandler* aHandler)
  : mProtoHandler(aHandler)
{
}

nsXBLEventHandler::~nsXBLEventHandler()
{
}

NS_IMPL_ISUPPORTS1(nsXBLEventHandler, nsIDOMEventListener)

NS_IMETHODIMP
nsXBLEventHandler::HandleEvent(nsIDOMEvent* aEvent)
{
  if (!mProtoHandler)
    return NS_ERROR_FAILURE;

  PRUint8 phase = mProtoHandler->GetPhase();
  if (phase == NS_PHASE_TARGET) {
    PRUint16 eventPhase;
    aEvent->GetEventPhase(&eventPhase);
    if (eventPhase != nsIDOMEvent::AT_TARGET)
      return NS_OK;
  }

  if (!EventMatched(aEvent))
    return NS_OK;

  nsCOMPtr<nsIDOMEventTarget> target;
  aEvent->GetCurrentTarget(getter_AddRefs(target));
  nsCOMPtr<nsIDOMEventReceiver> receiver = do_QueryInterface(target);

  mProtoHandler->ExecuteHandler(receiver, aEvent);

  return NS_OK;
}

nsXBLMouseEventHandler::nsXBLMouseEventHandler(nsXBLPrototypeHandler* aHandler)
  : nsXBLEventHandler(aHandler)
{
}

nsXBLMouseEventHandler::~nsXBLMouseEventHandler()
{
}

PRBool
nsXBLMouseEventHandler::EventMatched(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIDOMMouseEvent> mouse(do_QueryInterface(aEvent));
  return mProtoHandler->MouseEventMatched(mouse);
}

nsXBLKeyEventHandler::nsXBLKeyEventHandler(nsIAtom* aEventType, PRUint8 aPhase,
                                           PRUint8 aType)
  : mEventType(aEventType),
    mPhase(aPhase),
    mType(aType)
{
}

nsXBLKeyEventHandler::~nsXBLKeyEventHandler()
{
}

NS_IMPL_ISUPPORTS1(nsXBLKeyEventHandler, nsIDOMEventListener)

NS_IMETHODIMP
nsXBLKeyEventHandler::HandleEvent(nsIDOMEvent* aEvent)
{
  PRUint32 count = mProtoHandlers.Count();
  if (count == 0)
    return NS_ERROR_FAILURE;

  if (mPhase == NS_PHASE_TARGET) {
    PRUint16 eventPhase;
    aEvent->GetEventPhase(&eventPhase);
    if (eventPhase != nsIDOMEvent::AT_TARGET)
      return NS_OK;
  }

  nsCOMPtr<nsIDOMEventTarget> target;
  aEvent->GetCurrentTarget(getter_AddRefs(target));
  nsCOMPtr<nsIDOMEventReceiver> receiver = do_QueryInterface(target);

  nsCOMPtr<nsIDOMKeyEvent> key(do_QueryInterface(aEvent));

  nsCOMPtr<nsIDOMNSEvent> domNSEvent = do_QueryInterface(aEvent);
  PRBool trustedEvent = PR_FALSE;
  if (domNSEvent) {
    domNSEvent->GetIsTrusted(&trustedEvent);
  }

  PRUint32 i;
  for (i = 0; i < count; ++i) {
    nsXBLPrototypeHandler* handler = NS_STATIC_CAST(nsXBLPrototypeHandler*,
                                                    mProtoHandlers[i]);
    if ((trustedEvent || handler->AllowUntrustedEvents()) &&
        handler->KeyEventMatched(key)) {
      handler->ExecuteHandler(receiver, aEvent);
    }
  }

  return NS_OK;
}



nsresult
NS_NewXBLEventHandler(nsXBLPrototypeHandler* aHandler,
                      nsIAtom* aEventType,
                      nsXBLEventHandler** aResult)
{
  if (aEventType == nsGkAtoms::mousedown ||
      aEventType == nsGkAtoms::mouseup ||
      aEventType == nsGkAtoms::click ||
      aEventType == nsGkAtoms::dblclick ||
      aEventType == nsGkAtoms::mouseover ||
      aEventType == nsGkAtoms::mouseout ||
      aEventType == nsGkAtoms::mousemove ||
      aEventType == nsGkAtoms::contextmenu ||
      aEventType == nsGkAtoms::dragenter ||
      aEventType == nsGkAtoms::dragover ||
      aEventType == nsGkAtoms::dragdrop ||
      aEventType == nsGkAtoms::dragexit ||
      aEventType == nsGkAtoms::draggesture) {
    *aResult = new nsXBLMouseEventHandler(aHandler);
  }
  else {
    *aResult = new nsXBLEventHandler(aHandler);
  }

  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);

  return NS_OK;
}

nsresult
NS_NewXBLKeyEventHandler(nsIAtom* aEventType, PRUint8 aPhase, PRUint8 aType,
                         nsXBLKeyEventHandler** aResult)
{
  *aResult = new nsXBLKeyEventHandler(aEventType, aPhase, aType);

  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);

  return NS_OK;
}
