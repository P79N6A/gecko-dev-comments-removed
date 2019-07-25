






































#ifndef mozilla_dom_indexeddb_idbdatabaserequest_h__
#define mozilla_dom_indexeddb_idbdatabaserequest_h__

#include "mozilla/dom/indexedDB/IDBRequest.h"
#include "mozilla/dom/indexedDB/LazyIdleThread.h"

#include "mozIStorageConnection.h"
#include "nsIDOMDOMStringList.h"
#include "nsIIDBDatabaseRequest.h"
#include "nsIIDBTransaction.h"
#include "nsIObserver.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBDatabaseRequest : public IDBRequest::Generator,
                           public nsIIDBDatabaseRequest,
                           public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBDATABASE
  NS_DECL_NSIIDBDATABASEREQUEST
  NS_DECL_NSIOBSERVER

  static already_AddRefed<nsIIDBDatabaseRequest>
  Create(const nsAString& aName,
         const nsAString& aDescription,
         PRBool aReadOnly);

protected:
  IDBDatabaseRequest();
  ~IDBDatabaseRequest();

private:
  nsString mName;
  nsString mDescription;
  PRBool mReadOnly;
  nsString mVersion;
  nsCOMPtr<nsIDOMDOMStringList> mObjectStores;
  nsCOMPtr<nsIDOMDOMStringList> mIndexes;
  nsCOMPtr<nsIIDBTransaction> mCurrentTransaction;

  nsRefPtr<LazyIdleThread> mStorageThread;

  
  nsCOMPtr<mozIStorageConnection> mStorage;
};

END_INDEXEDDB_NAMESPACE

#endif 
