





































#include "nsDOMProgressEvent.h"
#include "nsContentUtils.h"


NS_INTERFACE_MAP_BEGIN(nsDOMProgressEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMProgressEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(ProgressEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMProgressEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMProgressEvent, nsDOMEvent)

NS_IMETHODIMP
nsDOMProgressEvent::GetLengthComputable(PRBool* aLengthComputable)
{
  *aLengthComputable = mLengthComputable;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMProgressEvent::GetLoaded(PRUint64* aLoaded)
{
  *aLoaded = mLoaded;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMProgressEvent::GetTotal(PRUint64* aTotal)
{
  *aTotal = mTotal;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMProgressEvent::InitProgressEvent(const nsAString& aType,
                                      PRBool aCanBubble,
                                      PRBool aCancelable,
                                      PRBool aLengthComputable,
                                      PRUint64 aLoaded,
                                      PRUint64 aTotal)
{
  nsresult rv = nsDOMEvent::InitEvent(aType, aCanBubble, aCancelable);
  NS_ENSURE_SUCCESS(rv, rv);

  mLoaded = aLoaded;
  mLengthComputable = aLengthComputable;
  mTotal = aTotal;

  return NS_OK;
}

nsresult
NS_NewDOMProgressEvent(nsIDOMEvent** aInstancePtrResult,
                       nsPresContext* aPresContext,
                       nsEvent* aEvent) 
{
  nsDOMProgressEvent* it = new nsDOMProgressEvent(aPresContext, aEvent);
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  return CallQueryInterface(it, aInstancePtrResult);
}
