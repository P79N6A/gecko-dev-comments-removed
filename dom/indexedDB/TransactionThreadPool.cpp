






































#include "TransactionThreadPool.h"

#include "nsIObserverService.h"
#include "nsIThreadPool.h"

#include "nsComponentManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsXPCOMCIDInternal.h"

using mozilla::MonitorAutoLock;

USING_INDEXEDDB_NAMESPACE

namespace {

const PRUint32 kThreadLimit = 20;
const PRUint32 kIdleThreadLimit = 5;
const PRUint32 kIdleThreadTimeoutMs = 30000;

TransactionThreadPool* gInstance = nsnull;
bool gShutdown = false;

inline
nsresult
CheckOverlapAndMergeObjectStores(nsTArray<nsString>& aLockedStores,
                                 const nsTArray<nsString>& aObjectStores,
                                 bool aShouldMerge,
                                 bool* aStoresOverlap)
{
  PRUint32 length = aObjectStores.Length();

  bool overlap = false;

  for (PRUint32 index = 0; index < length; index++) {
    const nsString& storeName = aObjectStores[index];
    if (aLockedStores.Contains(storeName)) {
      overlap = true;
    }
    else if (aShouldMerge && !aLockedStores.AppendElement(storeName)) {
      NS_WARNING("Out of memory!");
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  *aStoresOverlap = overlap;
  return NS_OK;
}

} 

BEGIN_INDEXEDDB_NAMESPACE

class FinishTransactionRunnable : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  inline FinishTransactionRunnable(IDBTransaction* aTransaction,
                                   nsCOMPtr<nsIRunnable>& aFinishRunnable);

private:
  IDBTransaction* mTransaction;
  nsCOMPtr<nsIRunnable> mFinishRunnable;
};

END_INDEXEDDB_NAMESPACE

TransactionThreadPool::TransactionThreadPool()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!gInstance, "More than one instance!");
}

TransactionThreadPool::~TransactionThreadPool()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(gInstance == this, "Different instances!");
  gInstance = nsnull;
}


TransactionThreadPool*
TransactionThreadPool::GetOrCreate()
{
  if (!gInstance && !gShutdown) {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    nsAutoPtr<TransactionThreadPool> pool(new TransactionThreadPool());

    nsresult rv = pool->Init();
    NS_ENSURE_SUCCESS(rv, nsnull);

    gInstance = pool.forget();
  }
  return gInstance;
}


TransactionThreadPool*
TransactionThreadPool::Get()
{
  return gInstance;
}


void
TransactionThreadPool::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  gShutdown = true;

  if (gInstance) {
    if (NS_FAILED(gInstance->Cleanup())) {
      NS_WARNING("Failed to shutdown thread pool!");
    }
    delete gInstance;
    gInstance = nsnull;
  }
}

nsresult
TransactionThreadPool::Init()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!mTransactionsInProgress.Init()) {
    NS_WARNING("Failed to init hash!");
    return NS_ERROR_FAILURE;
  }

  nsresult rv;
  mThreadPool = do_CreateInstance(NS_THREADPOOL_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mThreadPool->SetThreadLimit(kThreadLimit);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mThreadPool->SetIdleThreadLimit(kIdleThreadLimit);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mThreadPool->SetIdleThreadTimeout(kIdleThreadTimeoutMs);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
TransactionThreadPool::Cleanup()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsresult rv = mThreadPool->Shutdown();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = NS_ProcessPendingEvents(nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mCompleteCallbacks.IsEmpty()) {
    
    for (PRUint32 index = 0; index < mCompleteCallbacks.Length(); index++) {
      mCompleteCallbacks[index].mCallback->Run();
    }
    mCompleteCallbacks.Clear();

    
    rv = NS_ProcessPendingEvents(nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

void
TransactionThreadPool::FinishTransaction(IDBTransaction* aTransaction)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aTransaction, "Null pointer!");

  
  nsRefPtr<IDBTransaction> transaction(aTransaction);

  nsIAtom* databaseId = aTransaction->mDatabase->Id();

  DatabaseTransactionInfo* dbTransactionInfo;
  if (!mTransactionsInProgress.Get(databaseId, &dbTransactionInfo)) {
    NS_ERROR("We don't know anyting about this database?!");
    return;
  }

  nsTArray<TransactionInfo>& transactionsInProgress =
    dbTransactionInfo->transactions;

  PRUint32 transactionCount = transactionsInProgress.Length();

#ifdef DEBUG
  if (aTransaction->mMode == IDBTransaction::VERSION_CHANGE) {
    NS_ASSERTION(transactionCount == 1,
                 "More transactions running than should be!");
  }
#endif

  if (transactionCount == 1) {
#ifdef DEBUG
    {
      TransactionInfo& info = transactionsInProgress[0];
      NS_ASSERTION(info.transaction == aTransaction, "Transaction mismatch!");
    }
#endif
    mTransactionsInProgress.Remove(databaseId);

    
    for (PRUint32 index = 0; index < mCompleteCallbacks.Length(); index++) {
      MaybeFireCallback(index);
    }
  }
  else {
    
    nsTArray<nsString> storesWriting, storesReading;

    for (PRUint32 index = 0, count = transactionCount; index < count; index++) {
      IDBTransaction* transaction = transactionsInProgress[index].transaction;
      if (transaction == aTransaction) {
        NS_ASSERTION(count == transactionCount, "More than one match?!");

        transactionsInProgress.RemoveElementAt(index);
        index--;
        count--;

        continue;
      }

      const nsTArray<nsString>& objectStores = transaction->mObjectStoreNames;

      bool dummy;
      if (transaction->mMode == nsIIDBTransaction::READ_WRITE) {
        if (NS_FAILED(CheckOverlapAndMergeObjectStores(storesWriting,
                                                       objectStores,
                                                       true, &dummy))) {
          NS_WARNING("Out of memory!");
        }
      }
      else if (transaction->mMode == nsIIDBTransaction::READ_ONLY) {
        if (NS_FAILED(CheckOverlapAndMergeObjectStores(storesReading,
                                                       objectStores,
                                                       true, &dummy))) {
          NS_WARNING("Out of memory!");
        }
      }
      else {
        NS_NOTREACHED("Unknown mode!");
      }
    }

    NS_ASSERTION(transactionsInProgress.Length() == transactionCount - 1,
                 "Didn't find the transaction we were looking for!");

    dbTransactionInfo->storesWriting.SwapElements(storesWriting);
    dbTransactionInfo->storesReading.SwapElements(storesReading);
  }

  
  nsTArray<QueuedDispatchInfo> queuedDispatch;
  queuedDispatch.SwapElements(mDelayedDispatchQueue);

  transactionCount = queuedDispatch.Length();
  for (PRUint32 index = 0; index < transactionCount; index++) {
    if (NS_FAILED(Dispatch(queuedDispatch[index]))) {
      NS_WARNING("Dispatch failed!");
    }
  }
}

nsresult
TransactionThreadPool::TransactionCanRun(IDBTransaction* aTransaction,
                                         bool* aCanRun,
                                         TransactionQueue** aExistingQueue)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aTransaction, "Null pointer!");
  NS_ASSERTION(aCanRun, "Null pointer!");
  NS_ASSERTION(aExistingQueue, "Null pointer!");

  nsIAtom* databaseId = aTransaction->mDatabase->Id();
  const nsTArray<nsString>& objectStoreNames = aTransaction->mObjectStoreNames;
  const PRUint16 mode = aTransaction->mMode;

  
  DatabaseTransactionInfo* dbTransactionInfo;
  if (!mTransactionsInProgress.Get(databaseId, &dbTransactionInfo)) {
    
    *aCanRun = true;
    *aExistingQueue = nsnull;
    return NS_OK;
  }

  nsTArray<TransactionInfo>& transactionsInProgress =
    dbTransactionInfo->transactions;

  PRUint32 transactionCount = transactionsInProgress.Length();
  NS_ASSERTION(transactionCount, "Should never be 0!");

  for (PRUint32 index = 0; index < transactionCount; index++) {
    
    const TransactionInfo& info = transactionsInProgress[index];
    if (info.transaction == aTransaction) {
      *aCanRun = true;
      *aExistingQueue = info.queue;
      return NS_OK;
    }
  }

  NS_ASSERTION(mode != IDBTransaction::VERSION_CHANGE, "How did we get here?");

  bool writeOverlap;
  nsresult rv =
    CheckOverlapAndMergeObjectStores(dbTransactionInfo->storesWriting,
                                     objectStoreNames,
                                     mode == nsIIDBTransaction::READ_WRITE,
                                     &writeOverlap);
  NS_ENSURE_SUCCESS(rv, rv);

  bool readOverlap;
  rv = CheckOverlapAndMergeObjectStores(dbTransactionInfo->storesReading,
                                        objectStoreNames,
                                        mode == nsIIDBTransaction::READ_ONLY,
                                        &readOverlap);
  NS_ENSURE_SUCCESS(rv, rv);

  if (writeOverlap ||
      (readOverlap && mode == nsIIDBTransaction::READ_WRITE)) {
    *aCanRun = false;
    *aExistingQueue = nsnull;
    return NS_OK;
  }

  *aCanRun = true;
  *aExistingQueue = nsnull;
  return NS_OK;
}

nsresult
TransactionThreadPool::Dispatch(IDBTransaction* aTransaction,
                                nsIRunnable* aRunnable,
                                bool aFinish,
                                nsIRunnable* aFinishRunnable)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aTransaction, "Null pointer!");
  NS_ASSERTION(aRunnable, "Null pointer!");

  if (aTransaction->mDatabase->IsInvalidated() && !aFinish) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  bool canRun;
  TransactionQueue* existingQueue;
  nsresult rv = TransactionCanRun(aTransaction, &canRun, &existingQueue);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!canRun) {
    QueuedDispatchInfo* info = mDelayedDispatchQueue.AppendElement();
    NS_ENSURE_TRUE(info, NS_ERROR_OUT_OF_MEMORY);

    info->transaction = aTransaction;
    info->runnable = aRunnable;
    info->finish = aFinish;
    info->finishRunnable = aFinishRunnable;

    return NS_OK;
  }

  if (existingQueue) {
    existingQueue->Dispatch(aRunnable);
    if (aFinish) {
      existingQueue->Finish(aFinishRunnable);
    }
    return NS_OK;
  }

  nsIAtom* databaseId = aTransaction->mDatabase->Id();

#ifdef DEBUG
  if (aTransaction->mMode == IDBTransaction::VERSION_CHANGE) {
    NS_ASSERTION(!mTransactionsInProgress.Get(databaseId, nsnull),
                 "Shouldn't have anything in progress!");
  }
#endif

  DatabaseTransactionInfo* dbTransactionInfo;
  nsAutoPtr<DatabaseTransactionInfo> autoDBTransactionInfo;

  if (!mTransactionsInProgress.Get(databaseId, &dbTransactionInfo)) {
    
    autoDBTransactionInfo = new DatabaseTransactionInfo();
    dbTransactionInfo = autoDBTransactionInfo;
  }

  const nsTArray<nsString>& objectStoreNames = aTransaction->mObjectStoreNames;

  nsTArray<nsString>& storesInUse =
    aTransaction->mMode == nsIIDBTransaction::READ_WRITE ?
    dbTransactionInfo->storesWriting :
    dbTransactionInfo->storesReading;

  if (!storesInUse.AppendElements(objectStoreNames)) {
    NS_WARNING("Out of memory!");
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsTArray<TransactionInfo>& transactionInfoArray =
    dbTransactionInfo->transactions;

  TransactionInfo* transactionInfo = transactionInfoArray.AppendElement();
  NS_ENSURE_TRUE(transactionInfo, NS_ERROR_OUT_OF_MEMORY);

  transactionInfo->transaction = aTransaction;
  transactionInfo->queue = new TransactionQueue(aTransaction, aRunnable);
  if (aFinish) {
    transactionInfo->queue->Finish(aFinishRunnable);
  }

  if (!transactionInfo->objectStoreNames.AppendElements(objectStoreNames)) {
    NS_WARNING("Out of memory!");
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (autoDBTransactionInfo) {
    if (!mTransactionsInProgress.Put(databaseId, autoDBTransactionInfo)) {
      NS_WARNING("Failed to put!");
      return NS_ERROR_OUT_OF_MEMORY;
    }
    autoDBTransactionInfo.forget();
  }

  return mThreadPool->Dispatch(transactionInfo->queue, NS_DISPATCH_NORMAL);
}

bool
TransactionThreadPool::WaitForAllDatabasesToComplete(
                                   nsTArray<nsRefPtr<IDBDatabase> >& aDatabases,
                                   nsIRunnable* aCallback)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!aDatabases.IsEmpty(), "No databases to wait on!");
  NS_ASSERTION(aCallback, "Null pointer!");

  DatabasesCompleteCallback* callback = mCompleteCallbacks.AppendElement();
  if (!callback) {
    NS_WARNING("Out of memory!");
    return false;
  }

  callback->mCallback = aCallback;
  if (!callback->mDatabases.SwapElements(aDatabases)) {
    NS_ERROR("This should never fail!");
  }

  MaybeFireCallback(mCompleteCallbacks.Length() - 1);
  return true;
}

void
TransactionThreadPool::AbortTransactionsForDatabase(IDBDatabase* aDatabase)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aDatabase, "Null pointer!");

  
  DatabaseTransactionInfo* dbTransactionInfo;
  if (!mTransactionsInProgress.Get(aDatabase->Id(), &dbTransactionInfo)) {
    
    return;
  }

  nsAutoTArray<nsRefPtr<IDBTransaction>, 50> transactions;

  
  nsTArray<TransactionInfo>& transactionsInProgress =
    dbTransactionInfo->transactions;

  PRUint32 transactionCount = transactionsInProgress.Length();
  NS_ASSERTION(transactionCount, "Should never be 0!");

  for (PRUint32 index = 0; index < transactionCount; index++) {
    
    IDBTransaction* transaction = transactionsInProgress[index].transaction;
    if (transaction->Database() == aDatabase) {
      transactions.AppendElement(transaction);
    }
  }

  
  for (PRUint32 index = 0; index < mDelayedDispatchQueue.Length(); index++) {
    
    IDBTransaction* transaction = mDelayedDispatchQueue[index].transaction;
    if (transaction->Database() == aDatabase) {
      transactions.AppendElement(transaction);
    }
  }

  
  
  for (PRUint32 index = 0; index < transactions.Length(); index++) {
    
    
    
    transactions[index]->Abort();
  }
}

bool
TransactionThreadPool::HasTransactionsForDatabase(IDBDatabase* aDatabase)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aDatabase, "Null pointer!");

  
  DatabaseTransactionInfo* dbTransactionInfo;
  if (!mTransactionsInProgress.Get(aDatabase->Id(), &dbTransactionInfo)) {
    return false;
  }

  nsTArray<TransactionInfo>& transactionsInProgress =
    dbTransactionInfo->transactions;

  PRUint32 transactionCount = transactionsInProgress.Length();
  NS_ASSERTION(transactionCount, "Should never be 0!");

  for (PRUint32 index = 0; index < transactionCount; index++) {
    
    if (transactionsInProgress[index].transaction->Database() == aDatabase) {
      return true;
    }
  }

  return false;
}

void
TransactionThreadPool::MaybeFireCallback(PRUint32 aCallbackIndex)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  DatabasesCompleteCallback& callback = mCompleteCallbacks[aCallbackIndex];

  bool freeToRun = true;
  for (PRUint32 index = 0; index < callback.mDatabases.Length(); index++) {
    if (mTransactionsInProgress.Get(callback.mDatabases[index]->Id(), nsnull)) {
      freeToRun = false;
      break;
    }
  }

  if (freeToRun) {
    callback.mCallback->Run();
    mCompleteCallbacks.RemoveElementAt(aCallbackIndex);
  }
}

TransactionThreadPool::
TransactionQueue::TransactionQueue(IDBTransaction* aTransaction,
                                   nsIRunnable* aRunnable)
: mMonitor("TransactionQueue::mMonitor"),
  mTransaction(aTransaction),
  mShouldFinish(false)
{
  NS_ASSERTION(aTransaction, "Null pointer!");
  NS_ASSERTION(aRunnable, "Null pointer!");
  mQueue.AppendElement(aRunnable);
}

void
TransactionThreadPool::TransactionQueue::Dispatch(nsIRunnable* aRunnable)
{
  MonitorAutoLock lock(mMonitor);

  NS_ASSERTION(!mShouldFinish, "Dispatch called after Finish!");

  mQueue.AppendElement(aRunnable);

  mMonitor.Notify();
}

void
TransactionThreadPool::TransactionQueue::Finish(nsIRunnable* aFinishRunnable)
{
  MonitorAutoLock lock(mMonitor);

  NS_ASSERTION(!mShouldFinish, "Finish called more than once!");

  mShouldFinish = true;
  mFinishRunnable = aFinishRunnable;

  mMonitor.Notify();
}

NS_IMPL_THREADSAFE_ISUPPORTS1(TransactionThreadPool::TransactionQueue,
                              nsIRunnable)

NS_IMETHODIMP
TransactionThreadPool::TransactionQueue::Run()
{
  nsAutoTArray<nsCOMPtr<nsIRunnable>, 10> queue;
  nsCOMPtr<nsIRunnable> finishRunnable;
  bool shouldFinish = false;

  while(!shouldFinish) {
    NS_ASSERTION(queue.IsEmpty(), "Should have cleared this!");

    {
      MonitorAutoLock lock(mMonitor);
      while (!mShouldFinish && mQueue.IsEmpty()) {
        if (NS_FAILED(mMonitor.Wait())) {
          NS_ERROR("Failed to wait!");
        }
      }

      mQueue.SwapElements(queue);
      if (mShouldFinish) {
        mFinishRunnable.swap(finishRunnable);
        shouldFinish = true;
      }
    }

    PRUint32 count = queue.Length();
    for (PRUint32 index = 0; index < count; index++) {
      nsCOMPtr<nsIRunnable>& runnable = queue[index];
      runnable->Run();
      runnable = nsnull;
    }

    if (count) {
      queue.Clear();
    }
  }

  nsCOMPtr<nsIRunnable> finishTransactionRunnable =
    new FinishTransactionRunnable(mTransaction, finishRunnable);
  if (NS_FAILED(NS_DispatchToMainThread(finishTransactionRunnable,
                                        NS_DISPATCH_NORMAL))) {
    NS_WARNING("Failed to dispatch finishTransactionRunnable!");
  }

  return NS_OK;
}

FinishTransactionRunnable::FinishTransactionRunnable(
                                         IDBTransaction* aTransaction,
                                         nsCOMPtr<nsIRunnable>& aFinishRunnable)
: mTransaction(aTransaction)
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aTransaction, "Null pointer!");
  mFinishRunnable.swap(aFinishRunnable);
}

NS_IMPL_THREADSAFE_ISUPPORTS1(FinishTransactionRunnable, nsIRunnable)

NS_IMETHODIMP
FinishTransactionRunnable::Run()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  if (!gInstance) {
    NS_ERROR("Running after shutdown!");
    return NS_ERROR_FAILURE;
  }

  gInstance->FinishTransaction(mTransaction);

  if (mFinishRunnable) {
    mFinishRunnable->Run();
    mFinishRunnable = nsnull;
  }

  return NS_OK;
}
