





































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
