






































#ifndef mozilla_dom_indexeddb_idbtransaction_h__
#define mozilla_dom_indexeddb_idbtransaction_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"
#include "mozilla/dom/indexedDB/IDBDatabase.h"

#include "nsIIDBTransaction.h"
#include "nsIRunnable.h"
#include "nsIThreadInternal.h"

#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"

#include "nsAutoPtr.h"
#include "nsHashKeys.h"
#include "nsInterfaceHashtable.h"

class mozIStorageConnection;
class mozIStorageStatement;
class nsIThread;

BEGIN_INDEXEDDB_NAMESPACE

class AsyncConnectionHelper;
class CommitHelper;
struct ObjectStoreInfo;
class TransactionThreadPool;

class IDBTransactionListener
{
public:
  NS_IMETHOD_(nsrefcnt) AddRef() = 0;
  NS_IMETHOD_(nsrefcnt) Release() = 0;

  virtual nsresult NotifyTransactionComplete(IDBTransaction* aTransaction) = 0;
};

class IDBTransaction : public nsDOMEventTargetHelper,
                       public nsIIDBTransaction,
                       public nsIThreadObserver
{
  friend class AsyncConnectionHelper;
  friend class CommitHelper;
  friend class ThreadObserver;
  friend class TransactionThreadPool;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIIDBTRANSACTION
  NS_DECL_NSITHREADOBSERVER

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBTransaction,
                                           nsDOMEventTargetHelper)

  static already_AddRefed<IDBTransaction>
  Create(IDBDatabase* aDatabase,
         nsTArray<nsString>& aObjectStoreNames,
         PRUint16 aMode,
         PRUint32 aTimeout,
         bool aDispatchDelayed = false);

  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  void OnNewRequest();
  void OnRequestFinished();

  void SetTransactionListener(IDBTransactionListener* aListener);

  bool StartSavepoint();
  nsresult ReleaseSavepoint();
  void RollbackSavepoint();

  
  nsresult GetOrCreateConnection(mozIStorageConnection** aConnection);

  already_AddRefed<mozIStorageStatement>
  AddStatement(bool aCreate,
               bool aOverwrite,
               bool aAutoIncrement);

  already_AddRefed<mozIStorageStatement>
  DeleteStatement(bool aAutoIncrement);

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

  bool IsOpen() const;

  bool IsWriteAllowed() const
  {
    return mMode == nsIIDBTransaction::READ_WRITE ||
           mMode == nsIIDBTransaction::VERSION_CHANGE;
  }

  bool IsAborted() const
  {
    return mAborted;
  }

  PRUint16 Mode()
  {
    return mMode;
  }

  IDBDatabase* Database()
  {
    NS_ASSERTION(mDatabase, "This should never be null!");
    return mDatabase;
  }

  already_AddRefed<IDBObjectStore>
  GetOrCreateObjectStore(const nsAString& aName,
                         ObjectStoreInfo* aObjectStoreInfo);

private:
  IDBTransaction();
  ~IDBTransaction();

  nsresult CommitOrRollback();

  nsRefPtr<IDBDatabase> mDatabase;
  nsTArray<nsString> mObjectStoreNames;
  PRUint16 mReadyState;
  PRUint16 mMode;
  PRUint32 mTimeout;
  PRUint32 mPendingRequests;
  PRUint32 mCreatedRecursionDepth;

  
  nsRefPtr<nsDOMEventListenerWrapper> mOnErrorListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnCompleteListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnAbortListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnTimeoutListener;

  nsInterfaceHashtable<nsCStringHashKey, mozIStorageStatement>
    mCachedStatements;

  nsRefPtr<IDBTransactionListener> mListener;

  
  nsCOMPtr<mozIStorageConnection> mConnection;

  
  PRUint32 mSavepointCount;

  nsTArray<nsRefPtr<IDBObjectStore> > mCreatedObjectStores;

  bool mAborted;
  bool mCreating;

#ifdef DEBUG
  bool mFiredCompleteOrAbort;
#endif
};

class CommitHelper : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  CommitHelper(IDBTransaction* aTransaction,
               IDBTransactionListener* aListener);
  ~CommitHelper();

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
  nsRefPtr<IDBTransactionListener> mListener;
  nsCOMPtr<mozIStorageConnection> mConnection;
  nsAutoTArray<nsCOMPtr<nsISupports>, 10> mDoomedObjects;

  PRUint64 mOldVersion;
  nsTArray<nsAutoPtr<ObjectStoreInfo> > mOldObjectStores;

  bool mAborted;
  bool mHaveMetadata;
};

END_INDEXEDDB_NAMESPACE

#endif 
