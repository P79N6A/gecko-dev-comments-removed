






































#include "IDBTransactionRequest.h"

#include "nsDOMClassInfo.h"
#include "nsThreadUtils.h"

USING_INDEXEDDB_NAMESPACE

NS_IMPL_CYCLE_COLLECTION_CLASS(IDBTransactionRequest)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(IDBTransactionRequest,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnCompleteListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnAbortListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnTimeoutListener)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(IDBTransactionRequest,
                                                nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnCompleteListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnAbortListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnTimeoutListener)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(IDBTransactionRequest)
  NS_INTERFACE_MAP_ENTRY(nsIIDBTransactionRequest)
  NS_INTERFACE_MAP_ENTRY(nsIIDBTransaction)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(IDBTransactionRequest)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(IDBTransactionRequest, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(IDBTransactionRequest, nsDOMEventTargetHelper)

DOMCI_DATA(IDBTransactionRequest, IDBTransactionRequest)

NS_IMETHODIMP
IDBTransactionRequest::GetDb(nsIIDBDatabase** aDb)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBTransactionRequest::GetReadyState(PRUint16* aReadyState)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBTransactionRequest::GetMode(PRUint16* aMode)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBTransactionRequest::ObjectStore(const nsAString& aName,
                                   nsIIDBObjectStoreRequest** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBTransactionRequest::Abort()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBTransactionRequest::GetOncomplete(nsIDOMEventListener** aOncomplete)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBTransactionRequest::SetOncomplete(nsIDOMEventListener* aOncomplete)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBTransactionRequest::GetOnabort(nsIDOMEventListener** aOnabort)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBTransactionRequest::SetOnabort(nsIDOMEventListener* aOnabort)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBTransactionRequest::GetOntimeout(nsIDOMEventListener** aOntimeout)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBTransactionRequest::SetOntimeout(nsIDOMEventListener* aOntimeout)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}
