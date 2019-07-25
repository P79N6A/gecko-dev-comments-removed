






































#ifndef mozilla_dom_indexeddb_idbtransaction_h__
#define mozilla_dom_indexeddb_idbtransaction_h__

#include "mozilla/dom/indexedDB/IDBRequest.h"
#include "mozilla/dom/indexedDB/IDBDatabase.h"

#include "nsIIDBTransaction.h"
#include "nsIRunnable.h"

#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"

#include "nsAutoPtr.h"
#include "nsHashKeys.h"
#include "nsInterfaceHashtable.h"

class nsIThread;

BEGIN_INDEXEDDB_NAMESPACE

class AsyncConnectionHelper;
class CommitHelper;
struct ObjectStoreInfo;
class TransactionThreadPool;

class IDBTransaction : public nsDOMEventTargetHelper,
                       public IDBRequest::Generator,
                       public nsIIDBTransaction
{
  friend class AsyncConnectionHelper;
  friend class CommitHelper;
  friend class TransactionThreadPool;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIIDBTRANSACTION

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBTransaction,
                                           nsDOMEventTargetHelper)

  static already_AddRefed<IDBTransaction>
  Create(IDBDatabase* aDatabase,
         nsTArray<nsString>& aObjectStoreNames,
         PRUint16 aMode,
         PRUint32 aTimeout);

  void OnNewRequest();
  void OnRequestFinished();

  bool StartSavepoint();
  nsresult ReleaseSavepoint();
  void RollbackSavepoint();

  already_AddRefed<mozIStorageStatement>
  AddStatement(bool aCreate,
               bool aOverwrite,
               bool aAutoIncrement);

  already_AddRefed<mozIStorageStatement>
  RemoveStatement(bool aAutoIncrement);

  already_AddRefed<mozIStorageStatement>
  GetStatement(bool aAutoIncrement);

  already_AddRefed<mozIStorageStatement>
  IndexGetStatement(bool aUnique,
                    bool aAutoIncrement);

  already_AddRefed<mozIStorageStatement>
  IndexGetObjectStatement(bool aUnique,
                          bool aAutoIncrement);

  already_AddRefed<mozIStorageStatement>
  IndexUpdateStatement(bool aAutoIncrement,
                       bool aUnique,
                       bool aOverwrite);

  already_AddRefed<mozIStorageStatement>
  GetCachedStatement(const nsACString& aQuery);

  template<int N>
  already_AddRefed<mozIStorageStatement>
  GetCachedStatement(const char (&aQuery)[N])
  {
    nsCString query;
    query.AssignLiteral(aQuery);
    return GetCachedStatement(query);
  }

#ifdef DEBUG
  bool TransactionIsOpen() const;
  bool IsWriteAllowed() const;
#else
  bool TransactionIsOpen() const
  {
    return mReadyState == nsIIDBTransaction::INITIAL ||
           mReadyState == nsIIDBTransaction::LOADING;
  }

  bool IsWriteAllowed() const
  {
    return mMode == nsIIDBTransaction::READ_WRITE;
  }
#endif

  enum { FULL_LOCK = nsIIDBTransaction::SNAPSHOT_READ + 1 };

private:
  IDBTransaction();
  ~IDBTransaction();

  
  nsresult GetOrCreateConnection(mozIStorageConnection** aConnection);

  nsresult CommitOrRollback();

  nsRefPtr<IDBDatabase> mDatabase;
  nsTArray<nsString> mObjectStoreNames;
  PRUint16 mReadyState;
  PRUint16 mMode;
  PRUint32 mTimeout;
  PRUint32 mPendingRequests;

  
  nsRefPtr<nsDOMEventListenerWrapper> mOnCompleteListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnAbortListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnTimeoutListener;

  nsInterfaceHashtable<nsCStringHashKey, mozIStorageStatement>
    mCachedStatements;

  
  nsCOMPtr<mozIStorageConnection> mConnection;

  
  PRUint32 mSavepointCount;

  bool mHasInitialSavepoint;
  bool mAborted;
};

class CommitHelper : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  CommitHelper(IDBTransaction* aTransaction)
  : mTransaction(aTransaction),
    mAborted(!!aTransaction->mAborted),
    mHasInitialSavepoint(!!aTransaction->mHasInitialSavepoint)
  {
    mConnection.swap(aTransaction->mConnection);
  }

  template<class T>
  bool AddDoomedObject(nsCOMPtr<T>& aCOMPtr)
  {
    if (aCOMPtr) {
      if (!mDoomedObjects.AppendElement(do_QueryInterface(aCOMPtr))) {
        NS_ERROR("Out of memory!");
        return false;
      }
      aCOMPtr = nsnull;
    }
    return true;
  }

private:
  nsRefPtr<IDBTransaction> mTransaction;
  nsCOMPtr<mozIStorageConnection> mConnection;
  nsAutoTArray<nsCOMPtr<nsISupports>, 10> mDoomedObjects;
  bool mAborted;
  bool mHasInitialSavepoint;
};

END_INDEXEDDB_NAMESPACE

#endif 
