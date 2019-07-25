






































#include "TransactionThreadPool.h"

#include "nsIObserverService.h"
#include "nsIThreadPool.h"

#include "nsComponentManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsXPCOMCIDInternal.h"

using mozilla::MutexAutoLock;
using mozilla::MutexAutoUnlock;

USING_INDEXEDDB_NAMESPACE

namespace {

const PRUint32 kThreadLimit = 20;
const PRUint32 kIdleThreadLimit = 5;
const PRUint32 kIdleThreadTimeoutMs = 30000;

TransactionThreadPool* gInstance = nsnull;
bool gShutdown = false;

} 

BEGIN_INDEXEDDB_NAMESPACE

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
  NS_ASSERTION(!gInstance, "More than one instance!");
}


TransactionThreadPool*
TransactionThreadPool::GetOrCreate()
{
  if (!gInstance && !gShutdown) {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    nsRefPtr<TransactionThreadPool> pool(new TransactionThreadPool());

    nsresult rv = pool->Init();
    NS_ENSURE_SUCCESS(rv, nsnull);

    nsCOMPtr<nsIObserverService> obs =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, nsnull);

    rv = obs->AddObserver(pool, "xpcom-shutdown-threads", PR_FALSE);
    NS_ENSURE_SUCCESS(rv, nsnull);

    
    gInstance = pool;
  }
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

  if (mTransactionsInProgress.Count()) {
    
    
    NS_ERROR("Transactions still in progress!");
  }

  nsresult rv = mThreadPool->Shutdown();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
TransactionThreadPool::FinishTransaction(IDBTransaction* aTransaction)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aTransaction, "Null pointer!");

  
  nsRefPtr<IDBTransaction> transaction(aTransaction);

  const PRUint32 databaseId = aTransaction->mDatabase->Id();

  DatabaseTransactionInfo* dbTransactionInfo;
  if (!mTransactionsInProgress.Get(databaseId, &dbTransactionInfo)) {
    NS_ERROR("We don't know anyting about this database?!");
    return;
  }

  nsTArray<TransactionInfo>& transactionsInProgress =
    dbTransactionInfo->transactions;

  PRUint32 count = transactionsInProgress.Length();

#ifdef DEBUG
  if (aTransaction->mMode == IDBTransaction::FULL_LOCK) {
    NS_ASSERTION(dbTransactionInfo->locked, "Should be locked!");
    NS_ASSERTION(count == 1, "More transactions running than should be!");
  }
#endif

  if (count == 1) {
#ifdef DEBUG
    {
      TransactionInfo& info = transactionsInProgress[0];
      NS_ASSERTION(info.transaction == aTransaction, "Transaction mismatch!");
      NS_ASSERTION(info.mode == aTransaction->mMode, "Mode mismatch!");
    }
#endif
    mTransactionsInProgress.Remove(databaseId);
  }
  else {
    for (PRUint32 index = 0; index < count; index++) {
      TransactionInfo& info = transactionsInProgress[index];
      if (info.transaction == aTransaction) {
        transactionsInProgress.RemoveElementAt(index);
        break;
      }
    }

    NS_ASSERTION(transactionsInProgress.Length() == count - 1,
                 "Didn't find the transaction we were looking for!");
  }

  
  nsTArray<QueuedDispatchInfo> queuedDispatch;
  queuedDispatch.SwapElements(mDelayedDispatchQueue);

  count = queuedDispatch.Length();
  for (PRUint32 index = 0; index < count; index++) {
    QueuedDispatchInfo& info = queuedDispatch[index];
    if (NS_FAILED(Dispatch(info.transaction, info.runnable, info.finish,
                           info.finishRunnable))) {
      NS_WARNING("Dispatch failed!");
    }
  }
}

bool
TransactionThreadPool::TransactionCanRun(IDBTransaction* aTransaction,
                                         TransactionQueue** aQueue)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aTransaction, "Null pointer!");
  NS_ASSERTION(aQueue, "Null pointer!");

  const PRUint32 databaseId = aTransaction->mDatabase->Id();
  const nsTArray<nsString>& objectStoreNames = aTransaction->mObjectStoreNames;
  const PRUint16 mode = aTransaction->mMode;

  
  DatabaseTransactionInfo* dbTransactionInfo;
  if (!mTransactionsInProgress.Get(databaseId, &dbTransactionInfo)) {
    
    *aQueue = nsnull;
    return true;
  }

  nsTArray<TransactionInfo>& transactionsInProgress =
    dbTransactionInfo->transactions;

  PRUint32 transactionCount = transactionsInProgress.Length();

  if (mode == IDBTransaction::FULL_LOCK) {
    switch (transactionCount) {
      case 0: {
        *aQueue = nsnull;
        return true;
      }

      case 1: {
        if (transactionsInProgress[0].transaction == aTransaction) {
          *aQueue = transactionsInProgress[0].queue;
          return true;
        }
        return false;
      }

      default: {
        dbTransactionInfo->lockPending = true;
        return false;
      }
    }
  }

  bool locked = dbTransactionInfo->locked || dbTransactionInfo->lockPending;

  bool mayRun = true;

  
  
  for (PRUint32 transactionIndex = 0;
       transactionIndex < transactionCount;
       transactionIndex++) {
    TransactionInfo& transactionInfo = transactionsInProgress[transactionIndex];

    if (transactionInfo.transaction == aTransaction) {
      
      *aQueue = transactionInfo.queue;
      return true;
    }

    if (locked) {
      
      continue;
    }

    
    nsTArray<TransactionObjectStoreInfo>& objectStoreInfoArray =
      transactionInfo.objectStoreInfo;

    PRUint32 objectStoreCount = objectStoreInfoArray.Length();
    for (PRUint32 objectStoreIndex = 0;
         objectStoreIndex < objectStoreCount;
         objectStoreIndex++) {
      TransactionObjectStoreInfo& objectStoreInfo =
        objectStoreInfoArray[objectStoreIndex];

      if (objectStoreNames.Contains(objectStoreInfo.objectStoreName)) {
        
        switch (mode) {
          case nsIIDBTransaction::READ_WRITE: {
            
            
            objectStoreInfo.writerWaiting = true;

            
            
            
            mayRun = false;
          } break;

          case nsIIDBTransaction::READ_ONLY: {
            if (objectStoreInfo.writing || objectStoreInfo.writerWaiting) {
              
              return false;
            }
          } break;

          case nsIIDBTransaction::SNAPSHOT_READ: {
            NS_NOTYETIMPLEMENTED("Not implemented!");
          } break;

          default: {
            NS_NOTREACHED("Should never get here!");
          }
        }
      }

      
    }

    
  }

  if (locked) {
    
    
    return false;
  }

  if (!mayRun) {
    
    
    return false;
  }

  
  
  *aQueue = nsnull;
  return true;
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

  TransactionQueue* existingQueue;
  if (!TransactionCanRun(aTransaction, &existingQueue)) {
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

  const PRUint32 databaseId = aTransaction->mDatabase->Id();

#ifdef DEBUG
  if (aTransaction->mMode == IDBTransaction::FULL_LOCK) {
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

  if (aTransaction->mMode == IDBTransaction::FULL_LOCK) {
    NS_ASSERTION(!dbTransactionInfo->locked, "Already locked?!");
    dbTransactionInfo->locked = true;
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
  transactionInfo->mode = aTransaction->mMode;

  const nsTArray<nsString>& objectStoreNames = aTransaction->mObjectStoreNames;
  PRUint32 count = objectStoreNames.Length();
  for (PRUint32 index = 0; index < count; index++) {
    TransactionObjectStoreInfo* info =
      transactionInfo->objectStoreInfo.AppendElement();
    NS_ENSURE_TRUE(info, NS_ERROR_OUT_OF_MEMORY);

    info->objectStoreName = objectStoreNames[index];
    info->writing = transactionInfo->mode == nsIIDBTransaction::READ_WRITE;
  }

  if (autoDBTransactionInfo) {
    if (!mTransactionsInProgress.Put(databaseId, autoDBTransactionInfo)) {
      NS_ERROR("Failed to put!");
      return NS_ERROR_FAILURE;
    }
    autoDBTransactionInfo.forget();
  }

  return mThreadPool->Dispatch(transactionInfo->queue, NS_DISPATCH_NORMAL);
}

NS_IMPL_THREADSAFE_ISUPPORTS1(TransactionThreadPool, nsIObserver)

NS_IMETHODIMP
TransactionThreadPool::Observe(nsISupports* ,
                               const char*  aTopic,
                               const PRUnichar* )
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!strcmp("xpcom-shutdown-threads", aTopic), "Wrong topic!");

  Shutdown();

  return NS_OK;
}

TransactionThreadPool::
TransactionQueue::TransactionQueue(IDBTransaction* aTransaction,
                                   nsIRunnable* aRunnable)
: mMutex("TransactionQueue::mMutex"),
  mCondVar(mMutex, "TransactionQueue::mCondVar"),
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
  MutexAutoLock lock(mMutex);

  NS_ASSERTION(!mShouldFinish, "Dispatch called after Finish!");

  if (!mQueue.AppendElement(aRunnable)) {
    MutexAutoUnlock unlock(mMutex);
    NS_RUNTIMEABORT("Out of memory!");
  }

  mCondVar.Notify();
}

void
TransactionThreadPool::TransactionQueue::Finish(nsIRunnable* aFinishRunnable)
{
  MutexAutoLock lock(mMutex);

  NS_ASSERTION(!mShouldFinish, "Finish called more than once!");

  mShouldFinish = true;
  mFinishRunnable = aFinishRunnable;

  mCondVar.Notify();
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
      MutexAutoLock lock(mMutex);
      while (!mShouldFinish && mQueue.IsEmpty()) {
        if (NS_FAILED(mCondVar.Wait())) {
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
