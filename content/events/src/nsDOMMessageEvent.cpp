





































#include "nsDOMMessageEvent.h"
#include "nsContentUtils.h"

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMMessageEvent)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDOMMessageEvent, nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mSource)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDOMMessageEvent, nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mSource)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDOMMessageEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMessageEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(MessageEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMMessageEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMMessageEvent, nsDOMEvent)

NS_IMETHODIMP
nsDOMMessageEvent::GetData(nsAString& aData)
{
  aData = mData;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMessageEvent::GetOrigin(nsAString& aOrigin)
{
  aOrigin = mOrigin;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMessageEvent::GetLastEventId(nsAString& aLastEventId)
{
  aLastEventId = mLastEventId;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMessageEvent::GetSource(nsIDOMWindow** aSource)
{
  NS_IF_ADDREF(*aSource = mSource);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMessageEvent::InitMessageEvent(const nsAString& aType,
                                    PRBool aCanBubble,
                                    PRBool aCancelable,
                                    const nsAString& aData,
                                    const nsAString& aOrigin,
                                    const nsAString& aLastEventId,
                                    nsIDOMWindow* aSource)
{
  nsresult rv = nsDOMEvent::InitEvent(aType, aCanBubble, aCancelable);
  NS_ENSURE_SUCCESS(rv, rv);

  mData = aData;
  mOrigin = aOrigin;
  mLastEventId = aLastEventId;
  mSource = aSource;

  return NS_OK;
}

nsresult
NS_NewDOMMessageEvent(nsIDOMEvent** aInstancePtrResult,
                      nsPresContext* aPresContext,
                      nsEvent* aEvent) 
{
  nsDOMMessageEvent* it = new nsDOMMessageEvent(aPresContext, aEvent);
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  return CallQueryInterface(it, aInstancePtrResult);
}
