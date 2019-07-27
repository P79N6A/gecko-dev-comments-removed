





#ifndef mozilla_dom_indexeddb_transactionthreadpool_h__
#define mozilla_dom_indexeddb_transactionthreadpool_h__


#include "IndexedDatabase.h"

#include "nsIObserver.h"
#include "nsIRunnable.h"

#include "mozilla/Monitor.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"

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

  void WaitForDatabasesToComplete(nsTArray<nsRefPtr<IDBDatabase> >& aDatabases,
                                  nsIRunnable* aCallback);

  
  
  void AbortTransactionsForDatabase(IDBDatabase* aDatabase);

  
  bool HasTransactionsForDatabase(IDBDatabase* aDatabase);

protected:
  class TransactionQueue MOZ_FINAL : public nsIRunnable
  {
  public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIRUNNABLE

    explicit TransactionQueue(IDBTransaction* aTransaction);

    void Unblock();

    void Dispatch(nsIRunnable* aRunnable);

    void Finish(nsIRunnable* aFinishRunnable);

  private:
    ~TransactionQueue() {}

    mozilla::Monitor mMonitor;
    IDBTransaction* mTransaction;
    nsAutoTArray<nsCOMPtr<nsIRunnable>, 10> mQueue;
    nsCOMPtr<nsIRunnable> mFinishRunnable;
    bool mShouldFinish;
  };

  friend class TransactionQueue;

  struct TransactionInfo
  {
    explicit TransactionInfo(IDBTransaction* aTransaction)
    {
      MOZ_COUNT_CTOR(TransactionInfo);

      transaction = aTransaction;
      queue = new TransactionQueue(aTransaction);
    }

    ~TransactionInfo()
    {
      MOZ_COUNT_DTOR(TransactionInfo);
    }

    nsRefPtr<IDBTransaction> transaction;
    nsRefPtr<TransactionQueue> queue;
    nsTHashtable<nsPtrHashKey<TransactionInfo> > blockedOn;
    nsTHashtable<nsPtrHashKey<TransactionInfo> > blocking;
  };

  struct TransactionInfoPair
  {
    TransactionInfoPair()
      : lastBlockingReads(nullptr)
    {
      MOZ_COUNT_CTOR(TransactionInfoPair);
    }

    ~TransactionInfoPair()
    {
      MOZ_COUNT_DTOR(TransactionInfoPair);
    }
    
    nsTArray<TransactionInfo*> lastBlockingWrites;
    
    TransactionInfo* lastBlockingReads;
  };

  struct DatabaseTransactionInfo
  {
    DatabaseTransactionInfo()
    {
      MOZ_COUNT_CTOR(DatabaseTransactionInfo);
    }

    ~DatabaseTransactionInfo()
    {
      MOZ_COUNT_DTOR(DatabaseTransactionInfo);
    }

    typedef nsClassHashtable<nsPtrHashKey<IDBTransaction>, TransactionInfo >
      TransactionHashtable;
    TransactionHashtable transactions;
    nsClassHashtable<nsStringHashKey, TransactionInfoPair> blockingTransactions;
  };

  static PLDHashOperator
  CollectTransactions(IDBTransaction* aKey,
                      TransactionInfo* aValue,
                      void* aUserArg);

  static PLDHashOperator
  FindTransaction(IDBTransaction* aKey,
                  TransactionInfo* aValue,
                  void* aUserArg);

  static PLDHashOperator
  MaybeUnblockTransaction(nsPtrHashKey<TransactionInfo>* aKey,
                          void* aUserArg);

  struct DatabasesCompleteCallback
  {
    nsTArray<nsRefPtr<IDBDatabase> > mDatabases;
    nsCOMPtr<nsIRunnable> mCallback;
  };

  TransactionThreadPool();
  ~TransactionThreadPool();

  nsresult Init();
  nsresult Cleanup();

  void FinishTransaction(IDBTransaction* aTransaction);

  TransactionQueue& GetQueueForTransaction(IDBTransaction* aTransaction);

  bool MaybeFireCallback(DatabasesCompleteCallback aCallback);

  nsCOMPtr<nsIThreadPool> mThreadPool;

  nsClassHashtable<nsCStringHashKey, DatabaseTransactionInfo>
    mTransactionsInProgress;

  nsTArray<DatabasesCompleteCallback> mCompleteCallbacks;
};

END_INDEXEDDB_NAMESPACE

#endif 
