






































#include "IDBDatabaseRequest.h"

#include "nsDOMClassInfo.h"

#include "IDBRequest.h"

USING_INDEXEDDB_NAMESPACE

namespace {

const PRUint32 kDefaultTimeoutMS = 5000;
  
} 

IDBDatabaseRequest::IDBDatabaseRequest()
{
  
}

NS_IMPL_ADDREF(IDBDatabaseRequest)
NS_IMPL_RELEASE(IDBDatabaseRequest)

NS_INTERFACE_MAP_BEGIN(IDBDatabaseRequest)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIIDBDatabaseRequest)
  NS_INTERFACE_MAP_ENTRY(nsIIDBDatabaseRequest)
  NS_INTERFACE_MAP_ENTRY(nsIIDBDatabase)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(IDBDatabaseRequest)
NS_INTERFACE_MAP_END

DOMCI_DATA(IDBDatabaseRequest, IDBDatabaseRequest)

NS_IMETHODIMP
IDBDatabaseRequest::GetName(nsAString& aName)
{
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBDatabaseRequest::GetDescription(nsAString& aDescription)
{
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBDatabaseRequest::GetVersion(nsAString& aVersion)
{
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBDatabaseRequest::GetObjectStores(nsIDOMDOMStringList** aObjectStores)
{
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBDatabaseRequest::GetIndexes(nsIDOMDOMStringList** aIndexes)
{
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBDatabaseRequest::GetCurrentTransaction(nsIIDBTransaction** aTransaction)
{
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBDatabaseRequest::CreateObjectStore(const nsAString& aName,
                                      const nsAString& aKeyPath,
                                      PRBool aAutoIncrement,
                                      nsIIDBRequest** _retval)
{
  NS_NOTYETIMPLEMENTED("Implement me!");

  nsCOMPtr<nsIIDBRequest> request(new IDBRequest(this));
  request.forget(_retval);

  return NS_OK;
}

NS_IMETHODIMP
IDBDatabaseRequest::OpenObjectStore(const nsAString& aName,
                                    PRUint16 aMode,
                                    nsIIDBRequest** _retval)
{
  NS_NOTYETIMPLEMENTED("Implement me!");

  nsCOMPtr<nsIIDBRequest> request(new IDBRequest(this));
  request.forget(_retval);

  return NS_OK;
}

NS_IMETHODIMP
IDBDatabaseRequest::CreateIndex(const nsAString& aName,
                                const nsAString& aStoreName,
                                const nsAString& aKeyPath,
                                PRBool aUnique,
                                nsIIDBRequest** _retval)
{
  NS_NOTYETIMPLEMENTED("Implement me!");

  nsCOMPtr<nsIIDBRequest> request(new IDBRequest(this));
  request.forget(_retval);

  return NS_OK;
}

NS_IMETHODIMP
IDBDatabaseRequest::OpenIndex(const nsAString& aName,
                              nsIIDBRequest** _retval)
{
  NS_NOTYETIMPLEMENTED("Implement me!");

  nsCOMPtr<nsIIDBRequest> request(new IDBRequest(this));
  request.forget(_retval);

  return NS_OK;
}

NS_IMETHODIMP
IDBDatabaseRequest::RemoveObjectStore(const nsAString& aStoreName,
                                      nsIIDBRequest** _retval)
{
  NS_NOTYETIMPLEMENTED("Implement me!");

  nsCOMPtr<nsIIDBRequest> request(new IDBRequest(this));
  request.forget(_retval);

  return NS_OK;
}

NS_IMETHODIMP
IDBDatabaseRequest::RemoveIndex(const nsAString& aIndexName,
                                nsIIDBRequest** _retval)
{
  NS_NOTYETIMPLEMENTED("Implement me!");

  nsCOMPtr<nsIIDBRequest> request(new IDBRequest(this));
  request.forget(_retval);

  return NS_OK;
}

NS_IMETHODIMP
IDBDatabaseRequest::SetVersion(const nsAString& aVersion,
                               nsIIDBRequest** _retval)
{
  NS_NOTYETIMPLEMENTED("Implement me!");

  nsCOMPtr<nsIIDBRequest> request(new IDBRequest(this));
  request.forget(_retval);

  return NS_OK;
}

NS_IMETHODIMP
IDBDatabaseRequest::OpenTransaction(nsIDOMDOMStringList* aStoreNames,
                                    PRUint32 aTimeout,
                                    PRUint8 aArgCount,
                                    nsIIDBRequest** _retval)
{
  NS_NOTYETIMPLEMENTED("Implement me!");

  if (aArgCount < 2) {
    aTimeout = kDefaultTimeoutMS;
  }

  nsCOMPtr<nsIIDBRequest> request(new IDBRequest(this));
  request.forget(_retval);

  return NS_OK;
}
