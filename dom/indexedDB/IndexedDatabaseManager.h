





#ifndef mozilla_dom_indexeddb_indexeddatabasemanager_h__
#define mozilla_dom_indexeddb_indexeddatabasemanager_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

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
class nsIAtom;
class nsIFile;
class nsITimer;
class nsPIDOMWindow;
class nsEventChainPostVisitor;

BEGIN_INDEXEDDB_NAMESPACE

class AsyncConnectionHelper;
class CheckQuotaHelper;
class FileManager;
class IDBDatabase;

class IndexedDatabaseManager MOZ_FINAL : public nsIIndexedDatabaseManager,
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

  static bool IsClosed();

  typedef void
  (*WaitingOnDatabasesCallback)(nsTArray<nsRefPtr<IDBDatabase> >&, void*);

  
  
  
  nsresult AcquireExclusiveAccess(IDBDatabase* aDatabase,
                                  const nsACString& aOrigin,
                                  AsyncConnectionHelper* aHelper,
                                  WaitingOnDatabasesCallback aCallback,
                                  void* aClosure)
  {
    NS_ASSERTION(aDatabase, "Need a DB here!");
    return AcquireExclusiveAccess(aOrigin, aDatabase, aHelper, nullptr,
                                  aCallback, aClosure);
  }

  nsresult AcquireExclusiveAccess(const nsACString& aOrigin,
                                  nsIRunnable* aRunnable,
                                  WaitingOnDatabasesCallback aCallback,
                                  void* aClosure)
  {
    return AcquireExclusiveAccess(aOrigin, nullptr, nullptr, aRunnable, aCallback,
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

  static uint32_t
  GetIndexedDBQuotaMB();

  nsresult EnsureOriginIsInitialized(const nsACString& aOrigin,
                                     FactoryPrivilege aPrivilege,
                                     nsIFile** aDirectory);

  
  
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

  static bool
  IsMainProcess()
#ifdef DEBUG
  ;
#else
  {
    return sIsMainProcess;
  }
#endif

  already_AddRefed<FileManager>
  GetOrCreateFileManager(const nsACString& aOrigin,
                         const nsAString& aDatabaseName);

  already_AddRefed<FileManager>
  GetFileManager(const nsACString& aOrigin,
                 const nsAString& aDatabaseName);

  void InvalidateFileManagersForOrigin(const nsACString& aOrigin);

  void InvalidateFileManager(const nsACString& aOrigin,
                             const nsAString& aDatabaseName);

  nsresult AsyncDeleteFile(FileManager* aFileManager,
                           int64_t aFileId);

  const nsString&
  GetBaseDirectory() const
  {
    return mDatabaseBasePath;
  }

  nsresult
  GetDirectoryForOrigin(const nsACString& aASCIIOrigin,
                        nsIFile** aDirectory) const;

  static mozilla::Mutex& FileMutex()
  {
    IndexedDatabaseManager* mgr = Get();
    NS_ASSERTION(mgr, "Must have a manager here!");

    return mgr->mFileMutex;
  }

  static already_AddRefed<nsIAtom>
  GetDatabaseId(const nsACString& aOrigin,
                const nsAString& aName);

  static nsresult
  FireWindowOnError(nsPIDOMWindow* aOwner, nsEventChainPostVisitor& aVisitor);
private:
  IndexedDatabaseManager();
  ~IndexedDatabaseManager();

  nsresult AcquireExclusiveAccess(const nsACString& aOrigin,
                                  IDBDatabase* aDatabase,
                                  AsyncConnectionHelper* aHelper,
                                  nsIRunnable* aRunnable,
                                  WaitingOnDatabasesCallback aCallback,
                                  void* aClosure);

  void SetCurrentWindowInternal(nsPIDOMWindow* aWindow);
  bool QuotaIsLiftedInternal();
  void CancelPromptsForWindowInternal(nsPIDOMWindow* aWindow);

  
  bool RegisterDatabase(IDBDatabase* aDatabase);

  
  void UnregisterDatabase(IDBDatabase* aDatabase);

  
  void OnDatabaseClosed(IDBDatabase* aDatabase);

  
  
  
  
  
  
  
  
  class OriginClearRunnable MOZ_FINAL : public nsIRunnable
  {
    enum CallbackState {
      
      Pending = 0,

      
      OpenAllowed,

      
      IO,

      
      Complete
    };

  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIRUNNABLE

    OriginClearRunnable(const nsACString& aOrigin)
    : mOrigin(aOrigin),
      mCallbackState(Pending)
    { }

    void AdvanceState()
    {
      switch (mCallbackState) {
        case Pending:
          mCallbackState = OpenAllowed;
          return;
        case OpenAllowed:
          mCallbackState = IO;
          return;
        case IO:
          mCallbackState = Complete;
          return;
        default:
          NS_NOTREACHED("Can't advance past Complete!");
      }
    }

    static void InvalidateOpenedDatabases(
                                   nsTArray<nsRefPtr<IDBDatabase> >& aDatabases,
                                   void* aClosure);

  private:
    nsCString mOrigin;
    CallbackState mCallbackState;
  };

  
  
  
  
  
  
  
  
  
  class AsyncUsageRunnable MOZ_FINAL : public nsIRunnable
  {
    enum CallbackState {
      
      Pending = 0,

      
      OpenAllowed,

      
      IO,

      
      Complete,

      
      Shortcut
    };
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIRUNNABLE

    AsyncUsageRunnable(nsIURI* aURI,
                       const nsACString& aOrigin,
                       nsIIndexedDatabaseUsageCallback* aCallback);

    
    void Cancel();

    void AdvanceState()
    {
      switch (mCallbackState) {
        case Pending:
          mCallbackState = OpenAllowed;
          return;
        case OpenAllowed:
          mCallbackState = IO;
          return;
        case IO:
          mCallbackState = Complete;
          return;
        default:
          NS_NOTREACHED("Can't advance past Complete!");
      }
    }

    nsresult TakeShortcut();

    
    
    inline nsresult RunInternal();

    nsresult GetUsageForDirectory(nsIFile* aDirectory,
                                  uint64_t* aUsage);

    nsCOMPtr<nsIURI> mURI;
    nsCString mOrigin;

    nsCOMPtr<nsIIndexedDatabaseUsageCallback> mCallback;
    uint64_t mUsage;
    uint64_t mFileUsage;
    int32_t mCanceled;
    CallbackState mCallbackState;
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
    nsCOMPtr<nsIRunnable> mRunnable;
    nsTArray<nsCOMPtr<nsIRunnable> > mDelayedRunnables;
    nsTArray<IDBDatabase*> mDatabases;
  };

  
  
  class WaitForTransactionsToFinishRunnable MOZ_FINAL : public nsIRunnable
  {
  public:
    WaitForTransactionsToFinishRunnable(SynchronizedOp* aOp,
                                        uint32_t aCountdown)
    : mOp(aOp), mCountdown(aCountdown)
    {
      NS_ASSERTION(mOp, "Why don't we have a runnable?");
      NS_ASSERTION(mOp->mDatabases.IsEmpty(), "We're here too early!");
      NS_ASSERTION(mOp->mHelper || mOp->mRunnable,
                   "What are we supposed to do when we're done?");
      NS_ASSERTION(mCountdown, "Wrong countdown!");
    }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIRUNNABLE

  private:
    
    SynchronizedOp* mOp;
    uint32_t mCountdown;
  };

  class WaitForLockedFilesToFinishRunnable MOZ_FINAL : public nsIRunnable
  {
  public:
    WaitForLockedFilesToFinishRunnable()
    : mBusy(true)
    { }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIRUNNABLE

    bool IsBusy() const
    {
      return mBusy;
    }

  private:
    bool mBusy;
  };

  class AsyncDeleteFileRunnable MOZ_FINAL : public nsIRunnable
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIRUNNABLE
    AsyncDeleteFileRunnable(const nsAString& aFilePath)
    : mFilePath(aFilePath)
    { }

  private:
    nsString mFilePath;
  };

  static nsresult RunSynchronizedOp(IDBDatabase* aDatabase,
                                    SynchronizedOp* aOp);

  SynchronizedOp* FindSynchronizedOp(const nsACString& aOrigin,
                                     nsIAtom* aId)
  {
    for (uint32_t index = 0; index < mSynchronizedOps.Length(); index++) {
      const nsAutoPtr<SynchronizedOp>& currentOp = mSynchronizedOps[index];
      if (currentOp->mOrigin == aOrigin &&
          (!currentOp->mId || currentOp->mId == aId)) {
        return currentOp;
      }
    }
    return nullptr;
  }

  bool IsClearOriginPending(const nsACString& aOrigin)
  {
    return !!FindSynchronizedOp(aOrigin, nullptr);
  }

  
  nsClassHashtable<nsCStringHashKey, nsTArray<IDBDatabase*> > mLiveDatabases;

  
  unsigned mCurrentWindowIndex;

  
  mozilla::Mutex mQuotaHelperMutex;

  
  nsRefPtrHashtable<nsPtrHashKey<nsPIDOMWindow>, CheckQuotaHelper> mQuotaHelperHash;

  
  
  
  nsClassHashtable<nsCStringHashKey,
                   nsTArray<nsRefPtr<FileManager> > > mFileManagers;

  
  
  nsAutoTArray<nsRefPtr<AsyncUsageRunnable>, 1> mUsageRunnables;

  
  nsAutoTArray<nsAutoPtr<SynchronizedOp>, 5> mSynchronizedOps;

  
  nsCOMPtr<nsIThread> mIOThread;

  
  nsCOMPtr<nsITimer> mShutdownTimer;

  
  
  nsCOMPtr<mozIStorageQuotaCallback> mQuotaCallbackSingleton;

  
  
  
  mozilla::Mutex mFileMutex;

  nsString mDatabaseBasePath;

  static bool sIsMainProcess;
};

class AutoEnterWindow
{
public:
  AutoEnterWindow(nsPIDOMWindow* aWindow)
  {
    IndexedDatabaseManager::SetCurrentWindow(aWindow);
  }

  ~AutoEnterWindow()
  {
    IndexedDatabaseManager::SetCurrentWindow(nullptr);
  }
};

END_INDEXEDDB_NAMESPACE

#endif 
