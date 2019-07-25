






































#ifndef mozilla_dom_indexeddb_idbdatabase_h__
#define mozilla_dom_indexeddb_idbdatabase_h__

#include "mozilla/dom/indexedDB/IDBRequest.h"
#include "mozilla/dom/indexedDB/LazyIdleThread.h"

#include "mozIStorageConnection.h"
#include "nsIIDBDatabase.h"
#include "nsIObserver.h"

#include "nsDOMLists.h"

BEGIN_INDEXEDDB_NAMESPACE

class AsyncConnectionHelper;
struct DatabaseInfo;
class IDBTransaction;

class IDBDatabase : public IDBRequest::Generator,
                    public nsIIDBDatabase,
                    public nsIObserver
{
  friend class AsyncConnectionHelper;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBDATABASE
  NS_DECL_NSIOBSERVER

  static already_AddRefed<IDBDatabase>
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
  IDBDatabase();
  ~IDBDatabase();

  
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
