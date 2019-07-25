






































#ifndef mozilla_dom_indexeddb_transactionthreadpool_h__
#define mozilla_dom_indexeddb_transactionthreadpool_h__


#include "IndexedDatabase.h"

#include "nsIObserver.h"

#include "nsClassHashtable.h"
#include "nsHashKeys.h"
#include "nsRefPtrHashtable.h"

#include "IDBTransactionRequest.h"

class nsIRunnable;
class nsIThreadPool;

BEGIN_INDEXEDDB_NAMESPACE

class FinishTransactionRunnable;
class TransactionInfo;
class TransactionQueue;
class QueuedDispatchInfo;

class TransactionThreadPool : public nsIObserver
{
  friend class FinishTransactionRunnable;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  
  static TransactionThreadPool* GetOrCreate();
  static void Shutdown();

  nsresult Dispatch(IDBTransactionRequest* aTransaction,
                    nsIRunnable* aRunnable,
                    bool aFinish = false);

protected:
  TransactionThreadPool();
  ~TransactionThreadPool();

  nsresult Init();
  nsresult Cleanup();

  void FinishTransaction(IDBTransactionRequest* aTransaction);

  bool TransactionCanRun(IDBTransactionRequest* aTransaction,
                         TransactionQueue** aQueue);

  nsCOMPtr<nsIThreadPool> mThreadPool;

  nsClassHashtable<nsUint32HashKey, nsTArray<TransactionInfo> >
    mTransactionsInProgress;

  nsTArray<QueuedDispatchInfo> mDelayedDispatchQueue;
};

END_INDEXEDDB_NAMESPACE

#endif 
