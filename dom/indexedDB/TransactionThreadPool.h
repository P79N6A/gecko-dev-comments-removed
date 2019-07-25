






































#ifndef mozilla_dom_indexeddb_transactionthreadpool_h__
#define mozilla_dom_indexeddb_transactionthreadpool_h__


#include "IndexedDatabase.h"

#include "nsIObserver.h"

#include "mozilla/Mutex.h"
#include "nsHashKeys.h"
#include "nsRefPtrHashtable.h"

class nsIRunnable;
class nsIThreadPool;

BEGIN_INDEXEDDB_NAMESPACE

class IDBTransactionRequest;
class TransactionQueue;

template<class T>
class RefPtrHashKey : public PLDHashEntryHdr
{
 public:
  typedef T *KeyType;
  typedef const T *KeyTypePointer;

  RefPtrHashKey(const T *key) : mKey(const_cast<T*>(key)) {}
  RefPtrHashKey(const RefPtrHashKey<T> &toCopy) : mKey(toCopy.mKey) {}
  ~RefPtrHashKey() {}

  KeyType GetKey() const { return mKey; }

  PRBool KeyEquals(KeyTypePointer key) const { return key == mKey; }

  static KeyTypePointer KeyToPointer(KeyType key) { return key; }
  static PLDHashNumber HashKey(KeyTypePointer key)
  {
    return NS_PTR_TO_INT32(key) >> 2;
  }
  enum { ALLOW_MEMMOVE = PR_TRUE };

 protected:
  nsRefPtr<T> mKey;
};

class TransactionThreadPool : public nsIObserver
{
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

  mozilla::Mutex mMutex;
  nsCOMPtr<nsIThreadPool> mThreadPool;

  
  nsRefPtrHashtable<RefPtrHashKey<IDBTransactionRequest>, TransactionQueue>
    mTransactionsInProgress;
};

END_INDEXEDDB_NAMESPACE

#endif 
