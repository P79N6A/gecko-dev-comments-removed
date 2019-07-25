






































#ifndef mozilla_dom_indexeddb_indexeddatabasemanager_h__
#define mozilla_dom_indexeddb_indexeddatabasemanager_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"
#include "mozilla/dom/indexedDB/IDBDatabase.h"
#include "mozilla/dom/indexedDB/IDBRequest.h"

#include "mozilla/Mutex.h"

#include "nsIIndexedDatabaseManager.h"
#include "nsIObserver.h"
#include "nsIRunnable.h"
#include "nsIThread.h"
#include "nsIURI.h"

#include "nsClassHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsHashKeys.h"

#define INDEXEDDB_MANAGER_CONTRACTID "@mozilla.org/dom/indexeddb/manager;1"

class mozIStorageQuotaCallback;
class nsITimer;

BEGIN_INDEXEDDB_NAMESPACE

class AsyncConnectionHelper;

class CheckQuotaHelper;

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

  
  
  nsresult WaitForOpenAllowed(const nsACString& aOrigin,
                              nsIAtom* aId,
                              nsIRunnable* aRunnable);

  void AllowNextSynchronizedOp(const nsACString& aOrigin,
                               nsIAtom* aId);

  nsIThread* IOThread()
  {
    NS_ASSERTION(mIOThread, "This should never be null!");
    return mIOThread;
  }

  
  static bool IsShuttingDown();

  typedef void (*WaitingOnDatabasesCallback)(nsTArray<nsRefPtr<IDBDatabase> >&, void*);

  
  
  
  nsresult AcquireExclusiveAccess(IDBDatabase* aDatabase,
                                  AsyncConnectionHelper* aHelper,
                                  WaitingOnDatabasesCallback aCallback,
                                  void* aClosure)
  {
    NS_ASSERTION(aDatabase, "Need a DB here!");
    return AcquireExclusiveAccess(aDatabase->Origin(), aDatabase, aHelper,
                                  aCallback, aClosure);
  }
  nsresult AcquireExclusiveAccess(const nsACString& aOrigin, 
                                  AsyncConnectionHelper* aHelper,
                                  WaitingOnDatabasesCallback aCallback,
                                  void* aClosure)
  {
    return AcquireExclusiveAccess(aOrigin, nsnull, aHelper, aCallback,
                                  aClosure);
  }

  
  
  
  void AbortCloseDatabasesForWindow(nsPIDOMWindow* aWindow);

  
  bool HasOpenTransactions(nsPIDOMWindow* aWindow);

  
  
  static inline void
  SetCurrentWindow(nsPIDOMWindow* aWindow)
  {
    IndexedDatabaseManager* mgr = Get();
    NS_ASSERTION(mgr, "Must have a manager here!");

    return mgr->SetCurrentWindowInternal(aWindow);
  }

  static PRUint32
  GetIndexedDBQuotaMB();

  nsresult EnsureQuotaManagementForDirectory(nsIFile* aDirectory);

  
  
  static inline bool
  QuotaIsLifted()
  {
    IndexedDatabaseManager* mgr = Get();
    NS_ASSERTION(mgr, "Must have a manager here!");

    return mgr->QuotaIsLiftedInternal();
  }

  static inline void
  CancelPromptsForWindow(nsPIDOMWindow* aWindow)
  {
    IndexedDatabaseManager* mgr = Get();
    NS_ASSERTION(mgr, "Must have a manager here!");

    mgr->CancelPromptsForWindowInternal(aWindow);
  }

  static nsresult
  GetASCIIOriginFromWindow(nsPIDOMWindow* aWindow, nsCString& aASCIIOrigin);

private:
  IndexedDatabaseManager();
  ~IndexedDatabaseManager();

  nsresult AcquireExclusiveAccess(const nsACString& aOrigin, 
                                  IDBDatabase* aDatabase,
                                  AsyncConnectionHelper* aHelper,
                                  WaitingOnDatabasesCallback aCallback,
                                  void* aClosure);

  void SetCurrentWindowInternal(nsPIDOMWindow* aWindow);
  bool QuotaIsLiftedInternal();
  void CancelPromptsForWindowInternal(nsPIDOMWindow* aWindow);

  
  bool RegisterDatabase(IDBDatabase* aDatabase);

  
  void UnregisterDatabase(IDBDatabase* aDatabase);

  
  void OnDatabaseClosed(IDBDatabase* aDatabase);

  
  
  
  
  
  
  
  
  class OriginClearRunnable : public nsIRunnable
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIRUNNABLE

    OriginClearRunnable(const nsACString& aOrigin,
                        nsIThread* aThread)
    : mOrigin(aOrigin),
      mThread(aThread),
      mFirstCallback(true)
    { }

    nsCString mOrigin;
    nsCOMPtr<nsIThread> mThread;
    bool mFirstCallback;
  };

  bool IsClearOriginPending(const nsACString& origin);

  
  
  
  
  
  
  
  
  
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

  
  
  
  struct SynchronizedOp
  {
    SynchronizedOp(const nsACString& aOrigin, nsIAtom* aId);
    ~SynchronizedOp();

    
    bool MustWaitFor(const SynchronizedOp& aRhs) const;

    void DelayRunnable(nsIRunnable* aRunnable);
    void DispatchDelayedRunnables();

    const nsCString mOrigin;
    nsCOMPtr<nsIAtom> mId;
    nsRefPtr<AsyncConnectionHelper> mHelper;
    nsTArray<nsCOMPtr<nsIRunnable> > mDelayedRunnables;
    nsTArray<nsRefPtr<IDBDatabase> > mDatabases;
  };

  
  
  class WaitForTransactionsToFinishRunnable : public nsIRunnable
  {
  public:
    WaitForTransactionsToFinishRunnable(SynchronizedOp* aOp)
    : mOp(aOp)
    {
      NS_ASSERTION(mOp, "Why don't we have a runnable?");
      NS_ASSERTION(mOp->mDatabases.IsEmpty(), "We're here too early!");
      NS_ASSERTION(mOp->mHelper, "What are we supposed to do when we're done?");
    }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIRUNNABLE

  private:
    
    SynchronizedOp* mOp;
  };

  static nsresult DispatchHelper(AsyncConnectionHelper* aHelper);

  
  nsClassHashtable<nsCStringHashKey, nsTArray<IDBDatabase*> > mLiveDatabases;

  
  PRUintn mCurrentWindowIndex;

  
  mozilla::Mutex mQuotaHelperMutex;

  
  nsRefPtrHashtable<nsPtrHashKey<nsPIDOMWindow>, CheckQuotaHelper> mQuotaHelperHash;

  
  
  nsAutoTArray<nsRefPtr<AsyncUsageRunnable>, 1> mUsageRunnables;

  
  nsAutoTArray<nsAutoPtr<SynchronizedOp>, 5> mSynchronizedOps;

  
  nsCOMPtr<nsIThread> mIOThread;

  
  nsCOMPtr<nsITimer> mShutdownTimer;

  
  
  nsCOMPtr<mozIStorageQuotaCallback> mQuotaCallbackSingleton;

  
  
  
  nsTArray<nsCString> mTrackedQuotaPaths;
};

END_INDEXEDDB_NAMESPACE

#endif 
