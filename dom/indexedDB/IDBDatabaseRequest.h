






































#ifndef mozilla_dom_indexeddb_idbdatabaserequest_h__
#define mozilla_dom_indexeddb_idbdatabaserequest_h__

#include "mozilla/dom/indexedDB/IDBRequest.h"
#include "mozilla/dom/indexedDB/LazyIdleThread.h"

#include "mozIStorageConnection.h"
#include "nsIIDBDatabaseRequest.h"
#include "nsIIDBTransaction.h"
#include "nsIObserver.h"

#include "nsDOMLists.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBObjectStoreRequest;

class IDBDatabaseRequest : public IDBRequest::Generator,
                           public nsIIDBDatabaseRequest,
                           public nsIObserver
{
  friend class IDBObjectStoreRequest;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBDATABASE
  NS_DECL_NSIIDBDATABASEREQUEST
  NS_DECL_NSIOBSERVER

  static already_AddRefed<IDBDatabaseRequest>
  Create(const nsAString& aName,
         const nsAString& aDescription,
         PRBool aReadOnly);

  
  nsCOMPtr<mozIStorageConnection>& Connection();

  
  nsresult EnsureConnection();

  nsIThread* ConnectionThread() {
    return mConnectionThread;
  }

  void OnObjectStoreCreated(const nsAString& aName);
  void OnIndexCreated(const nsAString& aName);
  void OnObjectStoreRemoved(const nsAString& aName);

protected:
  IDBDatabaseRequest();
  ~IDBDatabaseRequest();

private:
  nsCString mASCIIOrigin;
  nsString mName;
  nsString mDescription;
  PRBool mReadOnly;
  nsString mVersion;

  nsTArray<nsString> mObjectStoreNames;
  nsTArray<nsString> mIndexNames;

  nsCOMPtr<nsIIDBTransaction> mCurrentTransaction;

  nsRefPtr<LazyIdleThread> mConnectionThread;

  
  nsCOMPtr<mozIStorageConnection> mConnection;
};

END_INDEXEDDB_NAMESPACE

#endif 
