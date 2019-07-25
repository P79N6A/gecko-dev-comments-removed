






































#ifndef mozilla_dom_indexeddb_indexeddatabasemanager_h__
#define mozilla_dom_indexeddb_indexeddatabasemanager_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"
#include "mozilla/dom/indexedDB/IDBDatabase.h"

#include "nsIIndexedDatabaseManager.h"
#include "nsIObserver.h"
#include "nsIRunnable.h"
#include "nsIThread.h"
#include "nsIURI.h"

#include "nsClassHashtable.h"
#include "nsHashKeys.h"

#define INDEXEDDB_MANAGER_CONTRACTID \
  "@mozilla.org/dom/indexeddb/manager;1"

class nsITimer;

BEGIN_INDEXEDDB_NAMESPACE

class IndexedDatabaseManager : public nsIIndexedDatabaseManager,
                               public nsIObserver
{
  friend class IDBDatabase;

public:
  static already_AddRefed<IndexedDatabaseManager> GetOrCreate();

  
  static IndexedDatabaseManager* Get();

  
  static IndexedDatabaseManager* FactoryCreate();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIINDEXEDDATABASEMANAGER
  NS_DECL_NSIOBSERVER

  nsresult WaitForClearAndDispatch(const nsACString& aOrigin,
                                   nsIRunnable* aRunnable);

  nsIThread* IOThread()
  {
    NS_ASSERTION(mIOThread, "This should never be null!");
    return mIOThread;
  }

  static bool IsShuttingDown();

private:
  IndexedDatabaseManager();
  ~IndexedDatabaseManager();

  bool RegisterDatabase(IDBDatabase* aDatabase);
  void UnregisterDatabase(IDBDatabase* aDatabase);

  
  
  class OriginClearRunnable : public nsIRunnable
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIRUNNABLE

    static void DatabaseCompleteCallback(IDBDatabase* aDatabase,
                                         void* aClosure)
    {
      nsRefPtr<OriginClearRunnable> runnable =
        static_cast<OriginClearRunnable*>(aClosure);
      runnable->OnDatabaseComplete(aDatabase);
    }

    void OnDatabaseComplete(IDBDatabase* aDatabase);

    OriginClearRunnable(const nsACString& aOrigin,
                        nsIThread* aThread,
                        nsTArray<nsRefPtr<IDBDatabase> >& aDatabasesWaiting)
    : mOrigin(aOrigin),
      mThread(aThread)
    {
      mDatabasesWaiting.SwapElements(aDatabasesWaiting);
    }

    nsCString mOrigin;
    nsCOMPtr<nsIThread> mThread;
    nsTArray<nsRefPtr<IDBDatabase> > mDatabasesWaiting;
    nsTArray<nsCOMPtr<nsIRunnable> > mDelayedRunnables;
  };

  inline void OnOriginClearComplete(OriginClearRunnable* aRunnable);

  
  
  class AsyncUsageRunnable : public nsIRunnable
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIRUNNABLE

    AsyncUsageRunnable(nsIURI* aURI,
                       const nsACString& aOrigin,
                       nsIIndexedDatabaseUsageCallback* aCallback);

    void Cancel();

    inline nsresult RunInternal();

    nsCOMPtr<nsIURI> mURI;
    nsCString mOrigin;
    nsCOMPtr<nsIIndexedDatabaseUsageCallback> mCallback;
    PRUint64 mUsage;
    PRInt32 mCanceled;
  };

  inline void OnUsageCheckComplete(AsyncUsageRunnable* aRunnable);

  
  nsClassHashtable<nsCStringHashKey, nsTArray<IDBDatabase*> > mLiveDatabases;

  
  nsAutoTArray<nsRefPtr<OriginClearRunnable>, 1> mOriginClearRunnables;

  
  
  nsAutoTArray<nsRefPtr<AsyncUsageRunnable>, 1> mUsageRunnables;

  nsCOMPtr<nsIThread> mIOThread;
  nsCOMPtr<nsITimer> mShutdownTimer;
};

END_INDEXEDDB_NAMESPACE

#endif 
