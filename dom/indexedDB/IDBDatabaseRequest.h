






































#ifndef mozilla_dom_indexeddb_idbdatabaserequest_h__
#define mozilla_dom_indexeddb_idbdatabaserequest_h__

#include "mozilla/dom/indexedDB/IDBRequest.h"
#include "mozilla/dom/indexedDB/LazyIdleThread.h"

#include "mozIStorageConnection.h"
#include "nsIIDBDatabaseRequest.h"
#include "nsIObserver.h"

#include "nsDOMLists.h"

BEGIN_INDEXEDDB_NAMESPACE

class AsyncConnectionHelper;
struct DatabaseInfo;
class IDBTransactionRequest;

class IDBDatabaseRequest : public IDBRequest::Generator,
                           public nsIIDBDatabaseRequest,
                           public nsIObserver
{
  friend class AsyncConnectionHelper;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBDATABASE
  NS_DECL_NSIIDBDATABASEREQUEST
  NS_DECL_NSIOBSERVER

  static already_AddRefed<IDBDatabaseRequest>
  Create(DatabaseInfo* aDatabaseInfo,
         LazyIdleThread* aThread,
         nsCOMPtr<mozIStorageConnection>& aConnection);

  nsIThread* ConnectionThread() {
    return mConnectionThread;
  }

  void CloseConnection();

  PRUint32 Id() {
    return mDatabaseId;
  }

  const nsString& FilePath() {
    return mFilePath;
  }

protected:
  IDBDatabaseRequest();
  ~IDBDatabaseRequest();

  
  nsresult GetOrCreateConnection(mozIStorageConnection** aConnection);

private:
  PRUint32 mDatabaseId;
  nsString mName;
  nsString mDescription;
  nsString mFilePath;

  nsRefPtr<LazyIdleThread> mConnectionThread;

  
  
  nsCOMPtr<mozIStorageConnection> mConnection;
};

END_INDEXEDDB_NAMESPACE

#endif 
