

































#include "nsDOMHashChangeEvent.h"
#include "nsContentUtils.h"

NS_IMPL_ADDREF_INHERITED(nsDOMHashChangeEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMHashChangeEvent, nsDOMEvent)

DOMCI_DATA(HashChangeEvent, nsDOMHashChangeEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMHashChangeEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHashChangeEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(HashChangeEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

nsDOMHashChangeEvent::~nsDOMHashChangeEvent()
{
}

NS_IMETHODIMP
nsDOMHashChangeEvent::GetOldURL(nsAString &aURL)
{
  aURL.Assign(mOldURL);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMHashChangeEvent::GetNewURL(nsAString &aURL)
{
  aURL.Assign(mNewURL);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMHashChangeEvent::InitHashChangeEvent(const nsAString &aTypeArg,
                                          PRBool aCanBubbleArg,
                                          PRBool aCancelableArg,
                                          const nsAString &aOldURL,
                                          const nsAString &aNewURL)
{
  nsresult rv = nsDOMEvent::InitEvent(aTypeArg, aCanBubbleArg, aCancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);

  mOldURL.Assign(aOldURL);
  mNewURL.Assign(aNewURL);
  return NS_OK;
}

nsresult NS_NewDOMHashChangeEvent(nsIDOMEvent** aInstancePtrResult,
                                nsPresContext* aPresContext,
                                nsEvent* aEvent)
{
  nsDOMHashChangeEvent* event =
    new nsDOMHashChangeEvent(aPresContext, aEvent);

  return CallQueryInterface(event, aInstancePtrResult);
}
