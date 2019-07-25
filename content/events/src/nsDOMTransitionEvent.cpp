




































#include "nsDOMTransitionEvent.h"
#include "nsGUIEvent.h"
#include "nsDOMClassInfoID.h"
#include "nsIClassInfo.h"
#include "nsIXPCScriptable.h"

nsDOMTransitionEvent::nsDOMTransitionEvent(nsPresContext *aPresContext,
                                           nsTransitionEvent *aEvent)
  : nsDOMEvent(aPresContext, aEvent ? aEvent
                                    : new nsTransitionEvent(PR_FALSE, 0,
                                                            EmptyString(),
                                                            0.0))
{
  if (aEvent) {
    mEventIsInternal = PR_FALSE;
  }
  else {
    mEventIsInternal = PR_TRUE;
    mEvent->time = PR_Now();
  }
}

nsDOMTransitionEvent::~nsDOMTransitionEvent()
{
  if (mEventIsInternal) {
    delete TransitionEvent();
    mEvent = nsnull;
  }
}

DOMCI_DATA(TransitionEvent, nsDOMTransitionEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMTransitionEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMTransitionEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(TransitionEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMTransitionEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMTransitionEvent, nsDOMEvent)

NS_IMETHODIMP
nsDOMTransitionEvent::GetPropertyName(nsAString & aPropertyName)
{
  aPropertyName = TransitionEvent()->propertyName;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMTransitionEvent::GetElapsedTime(float *aElapsedTime)
{
  *aElapsedTime = TransitionEvent()->elapsedTime;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMTransitionEvent::InitTransitionEvent(const nsAString & typeArg,
                                          PRBool canBubbleArg,
                                          PRBool cancelableArg,
                                          const nsAString & propertyNameArg,
                                          float elapsedTimeArg)
{
  nsresult rv = nsDOMEvent::InitEvent(typeArg, canBubbleArg, cancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);

  TransitionEvent()->propertyName = propertyNameArg;
  TransitionEvent()->elapsedTime = elapsedTimeArg;

  return NS_OK;
}

nsresult
NS_NewDOMTransitionEvent(nsIDOMEvent **aInstancePtrResult,
                         nsPresContext *aPresContext,
                         nsTransitionEvent *aEvent)
{
  nsDOMTransitionEvent *it = new nsDOMTransitionEvent(aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}
