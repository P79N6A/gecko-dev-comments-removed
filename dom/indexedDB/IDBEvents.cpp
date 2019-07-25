






































#include "IDBEvents.h"

#include "nsIPrivateDOMEvent.h"

#include "nsDOMClassInfo.h"

#include "IDBDatabaseError.h"

USING_INDEXEDDB_NAMESPACE

namespace {

template<class Class>
inline
nsIDOMEvent*
idomevent_cast(Class* aClassPtr)
{
  return static_cast<nsIDOMEvent*>(static_cast<nsDOMEvent*>(aClassPtr));
}

template<class Class>
inline
nsIDOMEvent*
idomevent_cast(nsRefPtr<Class> aClassAutoPtr)
{
  return idomevent_cast(aClassAutoPtr.get());
}

} 


already_AddRefed<nsIDOMEvent>
IDBErrorEvent::Create(PRUint16 aCode)
{
  nsRefPtr<IDBErrorEvent> event(new IDBErrorEvent());
  nsresult rv = event->Init();
  NS_ENSURE_SUCCESS(rv, nsnull);

  event->mError = new IDBDatabaseError(aCode);

  nsCOMPtr<nsIDOMEvent> result(idomevent_cast(event));
  return result.forget();
}

nsresult
IDBErrorEvent::Init()
{
  nsresult rv = InitEvent(NS_LITERAL_STRING(ERROR_EVT_STR), PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetTrusted(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMPL_ADDREF_INHERITED(IDBErrorEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(IDBErrorEvent, nsDOMEvent)

NS_INTERFACE_MAP_BEGIN(IDBErrorEvent)
  NS_INTERFACE_MAP_ENTRY(nsIIDBErrorEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(IDBErrorEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

DOMCI_DATA(IDBErrorEvent, IDBErrorEvent)

NS_IMETHODIMP
IDBErrorEvent::GetError(nsIIDBDatabaseError** aError)
{
  nsCOMPtr<nsIIDBDatabaseError> error(mError);
  error.forget(aError);
  return NS_OK;
}


already_AddRefed<nsIDOMEvent>
IDBSuccessEvent::Create(nsIVariant* aResult)
{
  nsRefPtr<IDBSuccessEvent> event(new IDBSuccessEvent());
  nsresult rv = event->Init();
  NS_ENSURE_SUCCESS(rv, nsnull);

  event->mResult = aResult;

  nsCOMPtr<nsIDOMEvent> result(idomevent_cast(event));
  return result.forget();
}

nsresult
IDBSuccessEvent::Init()
{
  nsresult rv = InitEvent(NS_LITERAL_STRING(SUCCESS_EVT_STR), PR_FALSE,
                          PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetTrusted(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMPL_ADDREF_INHERITED(IDBSuccessEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(IDBSuccessEvent, nsDOMEvent)

NS_INTERFACE_MAP_BEGIN(IDBSuccessEvent)
  NS_INTERFACE_MAP_ENTRY(nsIIDBSuccessEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(IDBSuccessEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

DOMCI_DATA(IDBSuccessEvent, IDBSuccessEvent)

NS_IMETHODIMP
IDBSuccessEvent::GetResult(nsIVariant** aResult)
{
  nsCOMPtr<nsIVariant> result(mResult);
  result.forget(aResult);
  return NS_OK;
}
