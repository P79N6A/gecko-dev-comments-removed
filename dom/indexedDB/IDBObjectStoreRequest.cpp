






































#include "IDBObjectStoreRequest.h"

#include "nsDOMClassInfo.h"
#include "nsThreadUtils.h"

USING_INDEXEDDB_NAMESPACE


already_AddRefed<IDBObjectStoreRequest>
IDBObjectStoreRequest::Create(IDBDatabaseRequest* aDatabase,
                              const nsAString& aName,
                              const nsAString& aKeyPath,
                              PRBool aAutoIncrement,
                              PRUint16 aMode)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<IDBObjectStoreRequest> objectStore = new IDBObjectStoreRequest();

  objectStore->mDatabase = aDatabase;
  objectStore->mName.Assign(aName);
  if (aKeyPath.IsVoid()) {
    objectStore->mKeyPath.SetIsVoid(PR_TRUE);
  }
  else {
    objectStore->mKeyPath.Assign(aKeyPath);
  }
  objectStore->mAutoIncrement = aAutoIncrement;
  objectStore->mMode = aMode;

  return objectStore.forget();
}

IDBObjectStoreRequest::IDBObjectStoreRequest()
: mAutoIncrement(PR_FALSE),
  mMode(nsIIDBObjectStore::READ_WRITE),
  mId(LL_MININT)
{

}

IDBObjectStoreRequest::~IDBObjectStoreRequest()
{

}

NS_IMPL_ADDREF(IDBObjectStoreRequest)
NS_IMPL_RELEASE(IDBObjectStoreRequest)

NS_INTERFACE_MAP_BEGIN(IDBObjectStoreRequest)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIIDBObjectStoreRequest)
  NS_INTERFACE_MAP_ENTRY(nsIIDBObjectStoreRequest)
  NS_INTERFACE_MAP_ENTRY(nsIIDBObjectStore)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(IDBObjectStoreRequest)
NS_INTERFACE_MAP_END

DOMCI_DATA(IDBObjectStoreRequest, IDBObjectStoreRequest)

NS_IMETHODIMP
IDBObjectStoreRequest::GetMode(PRUint16* aMode)
{
  *aMode = mMode;
  return NS_OK;
}

NS_IMETHODIMP
IDBObjectStoreRequest::GetName(nsAString& aName)
{
  aName.Assign(mName);
  return NS_OK;
}

NS_IMETHODIMP
IDBObjectStoreRequest::GetKeyPath(nsAString& aKeyPath)
{
  if (mKeyPath.IsVoid()) {
    aKeyPath.SetIsVoid(PR_TRUE);
  }
  else {
    aKeyPath.Assign(mKeyPath);
  }
  return NS_OK;
}

NS_IMETHODIMP
IDBObjectStoreRequest::GetIndexNames(nsIDOMDOMStringList** aIndexNames)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBObjectStoreRequest::Put(nsIVariant* aValue,
                           nsIVariant* aKey,
                           PRBool aNoOverwrite,
                           nsIIDBRequest** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBObjectStoreRequest::Remove(nsIVariant* aKey,
                              nsIIDBRequest** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBObjectStoreRequest::Get(nsIVariant* aKey,
                           nsIIDBRequest** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBObjectStoreRequest::OpenCursor(nsIIDBKeyRange* aRange,
                                  PRUint16 aDirection,
                                  nsIIDBRequest** _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
