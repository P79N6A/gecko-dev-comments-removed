






































#ifndef mozilla_dom_indexeddb_transactionthreadpool_h__
#define mozilla_dom_indexeddb_transactionthreadpool_h__


#include "IndexedDatabase.h"

#include "nsIObserver.h"

#include "mozilla/Mutex.h"
#include "mozilla/CondVar.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"
#include "nsRefPtrHashtable.h"

#include "IDBTransaction.h"

class nsIRunnable;
class nsIThreadPool;

BEGIN_INDEXEDDB_NAMESPACE

class FinishTransactionRunnable;
class QueuedDispatchInfo;

class TransactionThreadPool : public nsIObserver
{
  friend class FinishTransactionRunnable;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  
  static TransactionThreadPool* GetOrCreate();
  static void Shutdown();

  nsresult Dispatch(IDBTransaction* aTransaction,
                    nsIRunnable* aRunnable,
                    bool aFinish,
                    nsIRunnable* aFinishRunnable);

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

  struct TransactionObjectStoreInfo
  {
    TransactionObjectStoreInfo()
    : writing(false), writerWaiting(false)
    { }

    nsString objectStoreName;
    bool writing;
    bool writerWaiting;
  };

  struct TransactionInfo
  {
    TransactionInfo()
    : mode(nsIIDBTransaction::READ_ONLY)
    { }

    nsRefPtr<IDBTransaction> transaction;
    nsRefPtr<TransactionQueue> queue;
    nsTArray<TransactionObjectStoreInfo> objectStoreInfo;
    PRUint16 mode;
  };

  struct DatabaseTransactionInfo
  {
    DatabaseTransactionInfo()
    : locked(false), lockPending(false)
    { }

    bool locked;
    bool lockPending;
    nsTArray<TransactionInfo> transactions;
  };

  TransactionThreadPool();
  ~TransactionThreadPool();

  nsresult Init();
  nsresult Cleanup();

  void FinishTransaction(IDBTransaction* aTransaction);

  bool TransactionCanRun(IDBTransaction* aTransaction,
                         TransactionQueue** aQueue);

  nsCOMPtr<nsIThreadPool> mThreadPool;

  nsClassHashtable<nsUint32HashKey, DatabaseTransactionInfo>
    mTransactionsInProgress;

  nsTArray<QueuedDispatchInfo> mDelayedDispatchQueue;
};

END_INDEXEDDB_NAMESPACE

#endif 
