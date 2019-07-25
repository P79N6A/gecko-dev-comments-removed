






































#include "IndexedDatabaseRequest.h"

#include "nsDOMClassInfo.h"
#include "nsThreadUtils.h"

#include "IDBDatabaseRequest.h"
#include "IDBRequest.h"

USING_INDEXEDDB_NAMESPACE


already_AddRefed<nsIIndexedDatabaseRequest>
IndexedDatabaseRequest::Create()
{
  nsCOMPtr<nsIIndexedDatabaseRequest> request(new IndexedDatabaseRequest());
  return request.forget();
}

NS_IMPL_ADDREF(IndexedDatabaseRequest)
NS_IMPL_RELEASE(IndexedDatabaseRequest)

NS_INTERFACE_MAP_BEGIN(IndexedDatabaseRequest)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIIndexedDatabaseRequest)
  NS_INTERFACE_MAP_ENTRY(nsIIndexedDatabaseRequest)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(IndexedDatabaseRequest)
NS_INTERFACE_MAP_END

DOMCI_DATA(IndexedDatabaseRequest, IndexedDatabaseRequest)

NS_IMETHODIMP
IndexedDatabaseRequest::Open(const nsAString& aName,
                             const nsAString& aDescription,
                             PRBool aModifyDatabase,
                             PRUint8 aOptionalArgCount,
                             nsIIDBDatabaseRequest** _retval)
{
  if (!aOptionalArgCount) {
    
    aModifyDatabase = PR_TRUE;
  }

  nsRefPtr<IDBDatabaseRequest> database =
    IDBDatabaseRequest::Create(aName, aDescription, !aModifyDatabase);
  database.forget(_retval);
  return NS_OK;
}
