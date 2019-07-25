






































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
class IDBTransactionRequest;

class ObjectStoreInfo
{
public:
  nsString name;
  PRInt64 id;
  nsString keyPath;
  bool autoIncrement;

  ObjectStoreInfo()
  : id(0), autoIncrement(false)
  { }

  ObjectStoreInfo(const nsAString& aName,
                  PRInt64 aId)
  : name(aName),
    id(aId),
    autoIncrement(false)
  { }

  ObjectStoreInfo(const nsAString& aName,
                  PRInt64 aId,
                  const nsAString& aKeyPath,
                  bool aAutoIncrement)
  : name(aName),
    id(aId),
    keyPath(aKeyPath),
    autoIncrement(false)
  { }

  bool operator==(const ObjectStoreInfo& aOther) const {
    if (id == aOther.id) {
      NS_ASSERTION(name == aOther.name, "Huh?!");
      return true;
    }
    return false;
  }

  bool operator<(const ObjectStoreInfo& aOther) const {
    return id < aOther.id;
  }

};

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
  Create(const nsAString& aName,
         const nsAString& aDescription,
         nsTArray<ObjectStoreInfo>& aObjectStores,
         const nsAString& aVersion,
         LazyIdleThread* aThread,
         const nsAString& aDatabaseFilePath,
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

  void OnVersionSet(const nsString& aVersion);
  void OnObjectStoreCreated(const ObjectStoreInfo& aInfo);
  void OnObjectStoreRemoved(const ObjectStoreInfo& aInfo);

  void DisableConnectionThreadTimeout() {
    mConnectionThread->DisableIdleTimeout();
  }

  void EnableConnectionThreadTimeout() {
    mConnectionThread->EnableIdleTimeout();
  }

protected:
  IDBDatabaseRequest();
  ~IDBDatabaseRequest();

  
  nsCOMPtr<mozIStorageConnection>& Connection();

  
  nsresult EnsureConnection();

  nsresult QueueDatabaseWork(nsIRunnable* aRunnable);

  bool IdForObjectStoreName(const nsAString& aName,
                            PRInt64* aIndex) {
    NS_ASSERTION(aIndex, "Null pointer!");
    PRUint32 count = mObjectStores.Length();
    for (PRUint32 index = 0; index < count; index++) {
      ObjectStoreInfo& store = mObjectStores[index];
      if (store.name == aName) {
        *aIndex = PRInt64(index);
        return true;
      }
    }
    return false;
  }

private:
  nsString mName;
  nsString mDescription;
  nsString mVersion;
  nsString mDatabaseFilePath;

  nsTArray<ObjectStoreInfo> mObjectStores;

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

  nsTArray<nsCOMPtr<nsIRunnable> > mPendingDatabaseWork;
  nsTArray<nsRefPtr<IDBTransactionRequest> > mTransactions;
};

END_INDEXEDDB_NAMESPACE

#endif 
