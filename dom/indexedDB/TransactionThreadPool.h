






































#ifndef mozilla_dom_indexeddb_transactionthreadpool_h__
#define mozilla_dom_indexeddb_transactionthreadpool_h__


#include "IndexedDatabase.h"

#include "nsIObserver.h"
#include "nsIRunnable.h"

#include "mozilla/Mutex.h"
#include "mozilla/CondVar.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"
#include "nsRefPtrHashtable.h"

#include "IDBTransaction.h"

class nsIThreadPool;

BEGIN_INDEXEDDB_NAMESPACE

class FinishTransactionRunnable;
class QueuedDispatchInfo;

class TransactionThreadPool
{
  friend class nsAutoPtr<TransactionThreadPool>;
  friend class FinishTransactionRunnable;

public:
  
  static TransactionThreadPool* GetOrCreate();

  
  static TransactionThreadPool* Get();

  static void Shutdown();

  nsresult Dispatch(IDBTransaction* aTransaction,
                    nsIRunnable* aRunnable,
                    bool aFinish,
                    nsIRunnable* aFinishRunnable);

  typedef void (*DatabaseCompleteCallback)(IDBDatabase* aDatabase,
                                           void* aClosure);

  bool WaitForAllTransactionsToComplete(IDBDatabase* aDatabase,
                                        DatabaseCompleteCallback aCallback,
                                        void* aUserData);

protected:
  class TransactionQueue : public nsIRunnable
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIRUNNABLE

    inline TransactionQueue(IDBTransaction* aTransaction,
                            nsIRunnable* aRunnable);

    inline void Dispatch(nsIRunnable* aRunnable);

    inline void Finish(nsIRunnable* aFinishRunnable);

  private:
    mozilla::Mutex mMutex;
    mozilla::CondVar mCondVar;
    IDBTransaction* mTransaction;
    nsAutoTArray<nsCOMPtr<nsIRunnable>, 10> mQueue;
    nsCOMPtr<nsIRunnable> mFinishRunnable;
    bool mShouldFinish;
  };

  struct TransactionInfo
  {
    nsRefPtr<IDBTransaction> transaction;
    nsRefPtr<TransactionQueue> queue;
    nsTArray<nsString> objectStoreNames;
  };

  struct DatabaseTransactionInfo
  {
    DatabaseTransactionInfo()
    : locked(false), lockPending(false)
    { }

    bool locked;
    bool lockPending;
    nsTArray<TransactionInfo> transactions;
    nsTArray<nsString> storesReading;
    nsTArray<nsString> storesWriting;
  };

  struct QueuedDispatchInfo
  {
    QueuedDispatchInfo()
    : finish(false)
    { }

    nsRefPtr<IDBTransaction> transaction;
    nsCOMPtr<nsIRunnable> runnable;
    nsCOMPtr<nsIRunnable> finishRunnable;
    bool finish;
  };

  class DatabaseCompleteCallbackRunnable : public nsIRunnable
  {
    friend class TransactionThreadPool;

  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIRUNNABLE

    DatabaseCompleteCallbackRunnable(IDBDatabase* aDatabase,
                                     DatabaseCompleteCallback aCallback,
                                     void* aUserData)
    : mDatabase(aDatabase),
      mCallback(aCallback),
      mUserData(aUserData)
    { }

  private:
    nsRefPtr<IDBDatabase> mDatabase;
    DatabaseCompleteCallback mCallback;
    void* mUserData;
  };

  TransactionThreadPool();
  ~TransactionThreadPool();

  nsresult Init();
  nsresult Cleanup();

  void FinishTransaction(IDBTransaction* aTransaction);

  nsresult TransactionCanRun(IDBTransaction* aTransaction,
                             bool* aCanRun,
                             TransactionQueue** aExistingQueue);

  nsresult Dispatch(const QueuedDispatchInfo& aInfo)
  {
    return Dispatch(aInfo.transaction, aInfo.runnable, aInfo.finish,
                    aInfo.finishRunnable);
  }

  nsCOMPtr<nsIThreadPool> mThreadPool;

  nsClassHashtable<nsUint32HashKey, DatabaseTransactionInfo>
    mTransactionsInProgress;

  nsTArray<QueuedDispatchInfo> mDelayedDispatchQueue;

  nsTArray<nsRefPtr<DatabaseCompleteCallbackRunnable> > mCompleteRunnables;
};

END_INDEXEDDB_NAMESPACE

#endif 
