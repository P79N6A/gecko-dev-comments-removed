






































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

class AsyncConnectionHelper;
class IDBObjectStoreRequest;

class IDBDatabaseRequest : public IDBRequest::Generator,
                           public nsIIDBDatabaseRequest,
                           public nsIObserver
{
  friend class AsyncConnectionHelper;
  friend class IDBObjectStoreRequest;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBDATABASE
  NS_DECL_NSIIDBDATABASEREQUEST
  NS_DECL_NSIOBSERVER

  static already_AddRefed<IDBDatabaseRequest>
  Create(const nsAString& aName,
         const nsAString& aDescription,
         PRBool aReadOnly,
         nsTArray<nsString>& aObjectStoreNames,
         nsTArray<nsString>& aIndexNames,
         const nsAString& aVersion,
         LazyIdleThread* aThread,
         const nsAString& aDatabaseFilePath,
         nsCOMPtr<mozIStorageConnection>& aConnection);

  











  already_AddRefed<mozIStorageStatement> PutStatement(bool aOverwrite,
                                                      bool aAutoIncrement);

  









  already_AddRefed<mozIStorageStatement> RemoveStatement(bool aAutoIncrement);

  









  already_AddRefed<mozIStorageStatement> GetStatement(bool aAutoIncrement);

  nsIThread* ConnectionThread() {
    return mConnectionThread;
  }

  void FireCloseConnectionRunnable();

  void OnObjectStoreCreated(const nsAString& aName);
  void OnIndexCreated(const nsAString& aName);
  void OnObjectStoreRemoved(const nsAString& aName);

protected:
  IDBDatabaseRequest();
  ~IDBDatabaseRequest();

  
  nsCOMPtr<mozIStorageConnection>& Connection();

  
  nsresult EnsureConnection();

private:
  nsString mName;
  nsString mDescription;
  PRBool mReadOnly;
  nsString mVersion;
  nsString mDatabaseFilePath;

  nsTArray<nsString> mObjectStoreNames;
  nsTArray<nsString> mIndexNames;

  nsCOMPtr<nsIIDBTransaction> mCurrentTransaction;

  nsRefPtr<LazyIdleThread> mConnectionThread;

  
  
  nsCOMPtr<mozIStorageConnection> mConnection;
  nsCOMPtr<mozIStorageStatement> mPutStmt;
  nsCOMPtr<mozIStorageStatement> mPutAutoIncrementStmt;
  nsCOMPtr<mozIStorageStatement> mPutOverwriteStmt;
  nsCOMPtr<mozIStorageStatement> mPutOverwriteAutoIncrementStmt;
  nsCOMPtr<mozIStorageStatement> mRemoveStmt;
  nsCOMPtr<mozIStorageStatement> mRemoveAutoIncrementStmt;
  nsCOMPtr<mozIStorageStatement> mGetStmt;
  nsCOMPtr<mozIStorageStatement> mGetAutoIncrementStmt;
};

END_INDEXEDDB_NAMESPACE

#endif 
