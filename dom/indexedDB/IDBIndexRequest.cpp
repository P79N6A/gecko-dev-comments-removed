







































#include "IDBIndexRequest.h"

#include "nsIIDBDatabaseException.h"

#include "nsDOMClassInfo.h"
#include "nsThreadUtils.h"
#include "mozilla/Storage.h"

#include "AsyncConnectionHelper.h"
#include "IDBEvents.h"
#include "IDBObjectStoreRequest.h"
#include "IDBTransactionRequest.h"
#include "DatabaseInfo.h"

USING_INDEXEDDB_NAMESPACE


already_AddRefed<IDBIndexRequest>
IDBIndexRequest::Create(IDBDatabaseRequest* aDatabase,
                        IDBObjectStoreRequest* aObjectStore,
                        IDBTransactionRequest* aTransaction)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<IDBIndexRequest> index = new IDBIndexRequest();

  index->mDatabase = aDatabase;
  index->mObjectStore = aObjectStore;
  index->mTransaction = aTransaction;

  return index.forget();
}

IDBIndexRequest::IDBIndexRequest()
: mId(LL_MININT)
, mUnique(false)
, mAutoIncrement(false)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");
}

IDBIndexRequest::~IDBIndexRequest()
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");
}

NS_IMPL_ADDREF(IDBIndexRequest)
NS_IMPL_RELEASE(IDBIndexRequest)

NS_INTERFACE_MAP_BEGIN(IDBIndexRequest)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, IDBRequest::Generator)
  NS_INTERFACE_MAP_ENTRY(nsIIDBIndexRequest)
  NS_INTERFACE_MAP_ENTRY(nsIIDBIndex)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(IDBIndexRequest)
NS_INTERFACE_MAP_END

DOMCI_DATA(IDBIndexRequest, IDBIndexRequest)




NS_IMETHODIMP
IDBIndexRequest::GetName(nsAString& aName)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");

  aName.Assign(mName);
  return NS_OK;
}

NS_IMETHODIMP
IDBIndexRequest::GetStoreName(nsAString& aStoreName)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");

  return mObjectStore->GetName(aStoreName);
}

NS_IMETHODIMP
IDBIndexRequest::GetKeyPath(nsAString& aKeyPath)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");

  aKeyPath.Assign(mKeyPath);
  return NS_OK;
}

NS_IMETHODIMP
IDBIndexRequest::GetUnique(PRBool* aUnique)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");

  *aUnique = mUnique;
  return NS_OK;
}




NS_IMETHODIMP
IDBIndexRequest::OpenObjectCursor(nsIIDBKeyRange* aRange,
                                  PRUint16 aDirection,
                                  nsIIDBRequest** _retval)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBIndexRequest::OpenCursor(nsIIDBKeyRange* aRange,
                            PRUint16 aDirection,
                            nsIIDBRequest** _retval)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBIndexRequest::Put(nsIVariant* aValue,
                     nsIVariant* aKey,
                     PRBool aOverwrite,
                     nsIIDBRequest** _retval)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBIndexRequest::GetObject(nsIVariant* aKey,
                           nsIIDBRequest** _retval)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBIndexRequest::Get(nsIVariant* aKey,
                     nsIIDBRequest** _retval)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBIndexRequest::Remove(nsIVariant* aKey,
                        nsIIDBRequest** _retval)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}
