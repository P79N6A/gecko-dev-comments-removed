






































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
class DatabaseInfo;

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

  











  already_AddRefed<mozIStorageStatement> AddStatement(bool aCreate,
                                                      bool aOverwrite,
                                                      bool aAutoIncrement);

  









  already_AddRefed<mozIStorageStatement> RemoveStatement(bool aAutoIncrement);

  









  already_AddRefed<mozIStorageStatement> GetStatement(bool aAutoIncrement);

  nsIThread* ConnectionThread() {
    return mConnectionThread;
  }

  void FireCloseConnectionRunnable();

  void DisableConnectionThreadTimeout() {
    mConnectionThread->DisableIdleTimeout();
  }

  void EnableConnectionThreadTimeout() {
    mConnectionThread->EnableIdleTimeout();
  }

  PRUint32 Id() {
    return mDatabaseId;
  }

protected:
  IDBDatabaseRequest();
  ~IDBDatabaseRequest();

  
  nsCOMPtr<mozIStorageConnection>& Connection();

  
  nsresult EnsureConnection();

private:
  PRUint32 mDatabaseId;
  nsString mName;
  nsString mDescription;
  nsString mFilePath;

  nsRefPtr<LazyIdleThread> mConnectionThread;

  
  
  nsCOMPtr<mozIStorageConnection> mConnection;
  nsCOMPtr<mozIStorageStatement> mAddStmt;
  nsCOMPtr<mozIStorageStatement> mAddAutoIncrementStmt;
  nsCOMPtr<mozIStorageStatement> mModifyStmt;
  nsCOMPtr<mozIStorageStatement> mModifyAutoIncrementStmt;
  nsCOMPtr<mozIStorageStatement> mAddOrModifyStmt;
  nsCOMPtr<mozIStorageStatement> mAddOrModifyAutoIncrementStmt;
  nsCOMPtr<mozIStorageStatement> mRemoveStmt;
  nsCOMPtr<mozIStorageStatement> mRemoveAutoIncrementStmt;
  nsCOMPtr<mozIStorageStatement> mGetStmt;
  nsCOMPtr<mozIStorageStatement> mGetAutoIncrementStmt;
};

END_INDEXEDDB_NAMESPACE

#endif 
