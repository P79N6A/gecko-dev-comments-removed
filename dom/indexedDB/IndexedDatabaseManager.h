






































#ifndef mozilla_dom_indexeddb_indexeddatabasemanager_h__
#define mozilla_dom_indexeddb_indexeddatabasemanager_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"
#include "mozilla/dom/indexedDB/IDBDatabase.h"
#include "mozilla/dom/indexedDB/IDBRequest.h"

#include "nsIIndexedDatabaseManager.h"
#include "nsIObserver.h"
#include "nsIRunnable.h"
#include "nsIThread.h"
#include "nsIURI.h"

#include "nsClassHashtable.h"
#include "nsHashKeys.h"

#define INDEXEDDB_MANAGER_CONTRACTID "@mozilla.org/dom/indexeddb/manager;1"

class mozIStorageQuotaCallback;
class nsITimer;

BEGIN_INDEXEDDB_NAMESPACE

class AsyncConnectionHelper;

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

  
  
  nsresult WaitForOpenAllowed(const nsAString& aName,
                              const nsACString& aOrigin,
                              nsIRunnable* aRunnable);

  nsIThread* IOThread()
  {
    NS_ASSERTION(mIOThread, "This should never be null!");
    return mIOThread;
  }

  
  static bool IsShuttingDown();

  
  nsresult SetDatabaseVersion(IDBDatabase* aDatabase,
                              IDBOpenDBRequest* aRequest,
                              PRInt64 aOldVersion,
                              PRInt64 aNewVersion,
                              AsyncConnectionHelper* aHelper);

  
  
  
  void AbortCloseDatabasesForWindow(nsPIDOMWindow* aWindow);

  
  bool HasOpenTransactions(nsPIDOMWindow* aWindow);

  static bool
  SetCurrentDatabase(IDBDatabase* aDatabase);

  static PRUint32
  GetIndexedDBQuotaMB();

  nsresult EnsureQuotaManagementForDirectory(nsIFile* aDirectory);

private:
  IndexedDatabaseManager();
  ~IndexedDatabaseManager();

  
  bool RegisterDatabase(IDBDatabase* aDatabase);

  
  void UnregisterDatabase(IDBDatabase* aDatabase);

  
  void OnDatabaseClosed(IDBDatabase* aDatabase);

  
  void RunSetVersionTransaction(IDBDatabase* aDatabase)
  {
    OnDatabaseClosed(aDatabase);
  }

  
  
  
  
  
  
  
  
  
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
    nsTArray<nsCOMPtr<nsIRunnable> > mDelayedRunnables;
    bool mFirstCallback;
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

  
  
  
  
  class SetVersionRunnable : public nsIRunnable
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIRUNNABLE

    SetVersionRunnable(IDBDatabase* aDatabase,
                       nsTArray<nsRefPtr<IDBDatabase> >& aDatabases);
    ~SetVersionRunnable();

    nsRefPtr<IDBDatabase> mRequestingDatabase;
    nsTArray<nsRefPtr<IDBDatabase> > mDatabases;
    nsRefPtr<AsyncConnectionHelper> mHelper;
    nsTArray<nsCOMPtr<nsIRunnable> > mDelayedRunnables;
  };

  
  inline void OnSetVersionRunnableComplete(SetVersionRunnable* aRunnable);

  
  nsClassHashtable<nsCStringHashKey, nsTArray<IDBDatabase*> > mLiveDatabases;

  
  nsAutoTArray<nsRefPtr<OriginClearRunnable>, 1> mOriginClearRunnables;

  
  
  nsAutoTArray<nsRefPtr<AsyncUsageRunnable>, 1> mUsageRunnables;

  
  nsAutoTArray<nsRefPtr<SetVersionRunnable>, 1> mSetVersionRunnables;

  
  nsCOMPtr<nsIThread> mIOThread;

  
  nsCOMPtr<nsITimer> mShutdownTimer;

  
  
  nsCOMPtr<mozIStorageQuotaCallback> mQuotaCallbackSingleton;

  
  
  
  nsTArray<nsCString> mTrackedQuotaPaths;
};

END_INDEXEDDB_NAMESPACE

#endif 
