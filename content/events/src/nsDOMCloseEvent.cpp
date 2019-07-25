





































#include "nsDOMCloseEvent.h"
#include "nsContentUtils.h"

NS_IMPL_ADDREF_INHERITED(nsDOMCloseEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMCloseEvent, nsDOMEvent)

DOMCI_DATA(CloseEvent, nsDOMCloseEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMCloseEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCloseEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CloseEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMETHODIMP
nsDOMCloseEvent::GetWasClean(bool *aWasClean)
{
  *aWasClean = mWasClean;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMCloseEvent::GetCode(PRUint16 *aCode)
{
  *aCode = mReasonCode;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMCloseEvent::GetReason(nsAString & aReason)
{
  aReason = mReason;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMCloseEvent::InitCloseEvent(const nsAString& aType,
                                bool aCanBubble,
                                bool aCancelable,
                                bool aWasClean,
                                PRUint16 aReasonCode,
                                const nsAString &aReason)
{
  nsresult rv = nsDOMEvent::InitEvent(aType, aCanBubble, aCancelable);
  NS_ENSURE_SUCCESS(rv, rv);

  mWasClean = aWasClean;
  mReasonCode = aReasonCode;
  mReason = aReason;

  return NS_OK;
}

nsresult
nsDOMCloseEvent::InitFromCtor(const nsAString& aType, nsISupports* aDict,
                              JSContext* aCx, JSObject* aObj)
{
  nsCOMPtr<nsICloseEventInit> eventInit = do_QueryInterface(aDict);
  bool bubbles = false;
  bool cancelable = false;
  bool wasClean = false;
  PRUint16 code = 0;
  nsAutoString reason;
  if (eventInit) {
    nsresult rv = eventInit->GetBubbles(&bubbles);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = eventInit->GetCancelable(&cancelable);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = eventInit->GetWasClean(&wasClean);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = eventInit->GetCode(&code);
    NS_ENSURE_SUCCESS(rv, rv);
    JSBool found = JS_FALSE;
    if (JS_HasProperty(aCx, aObj, "reason", &found) && found) {
      rv = eventInit->GetReason(reason);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return InitCloseEvent(aType, bubbles, cancelable, wasClean, code, reason);
}

nsresult
NS_NewDOMCloseEvent(nsIDOMEvent** aInstancePtrResult,
                    nsPresContext* aPresContext,
                    nsEvent* aEvent) 
{
  nsDOMCloseEvent* it = new nsDOMCloseEvent(aPresContext, aEvent);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aInstancePtrResult);
}
