





#ifndef mozilla_dom_quota_quotamanager_h__
#define mozilla_dom_quota_quotamanager_h__

#include "QuotaCommon.h"

#include "nsIObserver.h"
#include "nsIQuotaManager.h"

#include "mozilla/dom/Nullable.h"
#include "mozilla/Mutex.h"

#include "nsClassHashtable.h"
#include "nsRefPtrHashtable.h"

#include "ArrayCluster.h"
#include "Client.h"
#include "PersistenceType.h"
#include "StoragePrivilege.h"

#define QUOTA_MANAGER_CONTRACTID "@mozilla.org/dom/quota/manager;1"

class nsIOfflineStorage;
class nsIPrincipal;
class nsIThread;
class nsITimer;
class nsIURI;
class nsPIDOMWindow;
class nsIRunnable;

namespace mozilla {
namespace dom {
class ContentParent;
}
}

BEGIN_QUOTA_NAMESPACE

class AcquireListener;
class AsyncUsageRunnable;
class CheckQuotaHelper;
class CollectOriginsHelper;
class FinalizeOriginEvictionRunnable;
class GroupInfo;
class GroupInfoPair;
class OriginClearRunnable;
class OriginInfo;
class OriginOrPatternString;
class QuotaObject;
class ResetOrClearRunnable;
struct SynchronizedOp;

class QuotaManager MOZ_FINAL : public nsIQuotaManager,
                               public nsIObserver
{
  friend class AsyncUsageRunnable;
  friend class CollectOriginsHelper;
  friend class FinalizeOriginEvictionRunnable;
  friend class GroupInfo;
  friend class OriginClearRunnable;
  friend class OriginInfo;
  friend class QuotaObject;
  friend class ResetOrClearRunnable;

  typedef mozilla::dom::ContentParent ContentParent;

  enum MozBrowserPatternFlag
  {
    MozBrowser = 0,
    NotMozBrowser,
    IgnoreMozBrowser
  };

  typedef void
  (*WaitingOnStoragesCallback)(nsTArray<nsCOMPtr<nsIOfflineStorage> >&, void*);

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIQUOTAMANAGER
  NS_DECL_NSIOBSERVER

  
  static QuotaManager*
  GetOrCreate();

  
  static QuotaManager*
  Get();

  
  static QuotaManager*
  FactoryCreate();

  
  static bool IsShuttingDown();

  void
  InitQuotaForOrigin(PersistenceType aPersistenceType,
                     const nsACString& aGroup,
                     const nsACString& aOrigin,
                     uint64_t aLimitBytes,
                     uint64_t aUsageBytes,
                     int64_t aAccessTime);

  void
  DecreaseUsageForOrigin(PersistenceType aPersistenceType,
                         const nsACString& aGroup,
                         const nsACString& aOrigin,
                         int64_t aSize);

  void
  UpdateOriginAccessTime(PersistenceType aPersistenceType,
                         const nsACString& aGroup,
                         const nsACString& aOrigin);

  void
  RemoveQuota();

  void
  RemoveQuotaForPersistenceType(PersistenceType);

  void
  RemoveQuotaForOrigin(PersistenceType aPersistenceType,
                       const nsACString& aGroup,
                       const nsACString& aOrigin)
  {
    MutexAutoLock lock(mQuotaMutex);
    LockedRemoveQuotaForOrigin(aPersistenceType, aGroup, aOrigin);
  }

  void
  RemoveQuotaForPattern(PersistenceType aPersistenceType,
                        const nsACString& aPattern);

  already_AddRefed<QuotaObject>
  GetQuotaObject(PersistenceType aPersistenceType,
                 const nsACString& aGroup,
                 const nsACString& aOrigin,
                 nsIFile* aFile);

  already_AddRefed<QuotaObject>
  GetQuotaObject(PersistenceType aPersistenceType,
                 const nsACString& aGroup,
                 const nsACString& aOrigin,
                 const nsAString& aPath);

  
  
  static void
  SetCurrentWindow(nsPIDOMWindow* aWindow)
  {
    QuotaManager* quotaManager = Get();
    NS_ASSERTION(quotaManager, "Must have a manager here!");

    quotaManager->SetCurrentWindowInternal(aWindow);
  }

  static void
  CancelPromptsForWindow(nsPIDOMWindow* aWindow)
  {
    NS_ASSERTION(aWindow, "Passed null window!");

    QuotaManager* quotaManager = Get();
    NS_ASSERTION(quotaManager, "Must have a manager here!");

    quotaManager->CancelPromptsForWindowInternal(aWindow);
  }

  
  bool
  RegisterStorage(nsIOfflineStorage* aStorage);

  
  void
  UnregisterStorage(nsIOfflineStorage* aStorage);

  
  void
  OnStorageClosed(nsIOfflineStorage* aStorage);

  
  
  
  void
  AbortCloseStoragesForWindow(nsPIDOMWindow* aWindow);

  
  
  void
  AbortCloseStoragesForProcess(ContentParent* aContentParent);

  
  bool
  HasOpenTransactions(nsPIDOMWindow* aWindow);

  
  
  nsresult
  WaitForOpenAllowed(const OriginOrPatternString& aOriginOrPattern,
                     Nullable<PersistenceType> aPersistenceType,
                     const nsACString& aId, nsIRunnable* aRunnable);

  
  
  
  nsresult
  AcquireExclusiveAccess(nsIOfflineStorage* aStorage,
                         const nsACString& aOrigin,
                         Nullable<PersistenceType> aPersistenceType,
                         AcquireListener* aListener,
                         WaitingOnStoragesCallback aCallback,
                         void* aClosure)
  {
    NS_ASSERTION(aStorage, "Need a storage here!");
    return AcquireExclusiveAccess(aOrigin, aPersistenceType, aStorage,
                                  aListener, aCallback, aClosure);
  }

  nsresult
  AcquireExclusiveAccess(const nsACString& aOrigin,
                         Nullable<PersistenceType> aPersistenceType,
                         AcquireListener* aListener,
                         WaitingOnStoragesCallback aCallback,
                         void* aClosure)
  {
    return AcquireExclusiveAccess(aOrigin, aPersistenceType, nullptr,
                                  aListener, aCallback, aClosure);
  }

  void
  AllowNextSynchronizedOp(const OriginOrPatternString& aOriginOrPattern,
                          Nullable<PersistenceType> aPersistenceType,
                          const nsACString& aId);

  bool
  IsClearOriginPending(const nsACString& aPattern,
                       Nullable<PersistenceType> aPersistenceType)
  {
    return !!FindSynchronizedOp(aPattern, aPersistenceType, EmptyCString());
  }

  nsresult
  GetDirectoryForOrigin(PersistenceType aPersistenceType,
                        const nsACString& aASCIIOrigin,
                        nsIFile** aDirectory) const;

  nsresult
  EnsureOriginIsInitialized(PersistenceType aPersistenceType,
                            const nsACString& aGroup,
                            const nsACString& aOrigin,
                            bool aTrackQuota,
                            nsIFile** aDirectory);

  void
  OriginClearCompleted(PersistenceType aPersistenceType,
                       const OriginOrPatternString& aOriginOrPattern);

  void
  ResetOrClearCompleted();

  void
  AssertCurrentThreadOwnsQuotaMutex()
  {
    mQuotaMutex.AssertCurrentThreadOwns();
  }

  nsIThread*
  IOThread()
  {
    NS_ASSERTION(mIOThread, "This should never be null!");
    return mIOThread;
  }

  already_AddRefed<Client>
  GetClient(Client::Type aClientType);

  const nsString&
  GetStoragePath(PersistenceType aPersistenceType) const
  {
    if (aPersistenceType == PERSISTENCE_TYPE_PERSISTENT) {
      return mPersistentStoragePath;
    }

    NS_ASSERTION(aPersistenceType == PERSISTENCE_TYPE_TEMPORARY, "Huh?");

    return mTemporaryStoragePath;
  }

  uint64_t
  GetGroupLimit() const;

  static uint32_t
  GetStorageQuotaMB();

  static void
  GetStorageId(PersistenceType aPersistenceType,
               const nsACString& aOrigin,
               Client::Type aClientType,
               const nsAString& aName,
               nsACString& aDatabaseId);

  static nsresult
  GetInfoFromURI(nsIURI* aURI,
                 uint32_t aAppId,
                 bool aInMozBrowser,
                 nsACString* aGroup,
                 nsACString* aOrigin,
                 StoragePrivilege* aPrivilege,
                 PersistenceType* aDefaultPersistenceType);

  static nsresult
  GetInfoFromPrincipal(nsIPrincipal* aPrincipal,
                       nsACString* aGroup,
                       nsACString* aOrigin,
                       StoragePrivilege* aPrivilege,
                       PersistenceType* aDefaultPersistenceType);

  static nsresult
  GetInfoFromWindow(nsPIDOMWindow* aWindow,
                    nsACString* aGroup,
                    nsACString* aOrigin,
                    StoragePrivilege* aPrivilege,
                    PersistenceType* aDefaultPersistenceType);

  static void
  GetInfoForChrome(nsACString* aGroup,
                   nsACString* aOrigin,
                   StoragePrivilege* aPrivilege,
                   PersistenceType* aDefaultPersistenceType);

  static void
  ChromeOrigin(nsACString& aOrigin);

  static void
  GetOriginPatternString(uint32_t aAppId, bool aBrowserOnly,
                         const nsACString& aOrigin, nsAutoCString& _retval)
  {
    return GetOriginPatternString(aAppId,
                                  aBrowserOnly ? MozBrowser : NotMozBrowser,
                                  aOrigin, _retval);
  }

  static void
  GetOriginPatternStringMaybeIgnoreBrowser(uint32_t aAppId, bool aBrowserOnly,
                                           nsAutoCString& _retval)
  {
    return GetOriginPatternString(aAppId,
                                  aBrowserOnly ? MozBrowser : IgnoreMozBrowser,
                                  EmptyCString(), _retval);
  }

private:
  QuotaManager();

  virtual ~QuotaManager();

  nsresult
  Init();

  void
  SetCurrentWindowInternal(nsPIDOMWindow* aWindow);

  void
  CancelPromptsForWindowInternal(nsPIDOMWindow* aWindow);

  
  
  bool
  LockedQuotaIsLifted();

  uint64_t
  LockedCollectOriginsForEviction(uint64_t aMinSizeToBeFreed,
                                  nsTArray<OriginInfo*>& aOriginInfos);

  void
  LockedRemoveQuotaForOrigin(PersistenceType aPersistenceType,
                             const nsACString& aGroup,
                             const nsACString& aOrigin);

  nsresult
  AcquireExclusiveAccess(const nsACString& aOrigin,
                         Nullable<PersistenceType> aPersistenceType,
                         nsIOfflineStorage* aStorage,
                         AcquireListener* aListener,
                         WaitingOnStoragesCallback aCallback,
                         void* aClosure);

  void
  AddSynchronizedOp(const OriginOrPatternString& aOriginOrPattern,
                    Nullable<PersistenceType> aPersistenceType);

  nsresult
  RunSynchronizedOp(nsIOfflineStorage* aStorage,
                    SynchronizedOp* aOp);

  SynchronizedOp*
  FindSynchronizedOp(const nsACString& aPattern,
                     Nullable<PersistenceType> aPersistenceType,
                     const nsACString& aId);

  nsresult
  MaybeUpgradeIndexedDBDirectory();

  nsresult
  InitializeOrigin(PersistenceType aPersistenceType,
                   const nsACString& aGroup,
                   const nsACString& aOrigin,
                   bool aTrackQuota,
                   int64_t aAccessTime,
                   nsIFile* aDirectory);

  nsresult
  ClearStoragesForApp(uint32_t aAppId, bool aBrowserOnly);

  void
  CheckTemporaryStorageLimits();

  
  uint64_t
  CollectOriginsForEviction(uint64_t aMinSizeToBeFreed,
                            nsTArray<OriginInfo*>& aOriginInfos);

  void
  DeleteTemporaryFilesForOrigin(const nsACString& aOrigin);

  void
  FinalizeOriginEviction(nsTArray<nsCString>& aOrigins);

  void
  SaveOriginAccessTime(const nsACString& aOrigin, int64_t aTimestamp);

  void
  ReleaseIOThreadObjects()
  {
    AssertIsOnIOThread();

    for (uint32_t index = 0; index < Client::TYPE_MAX; index++) {
      mClients[index]->ReleaseIOThreadObjects();
    }
  }

  template <class OwnerClass>
  void
  AbortCloseStoragesFor(OwnerClass* aOwnerClass);

  static void
  GetOriginPatternString(uint32_t aAppId,
                         MozBrowserPatternFlag aBrowserFlag,
                         const nsACString& aOrigin,
                         nsAutoCString& _retval);

  static PLDHashOperator
  RemoveQuotaForPersistenceTypeCallback(const nsACString& aKey,
                                        nsAutoPtr<GroupInfoPair>& aValue,
                                        void* aUserArg);

  static PLDHashOperator
  RemoveQuotaCallback(const nsACString& aKey,
                      nsAutoPtr<GroupInfoPair>& aValue,
                      void* aUserArg);

  static PLDHashOperator
  RemoveQuotaForPatternCallback(const nsACString& aKey,
                                nsAutoPtr<GroupInfoPair>& aValue,
                                void* aUserArg);

  static PLDHashOperator
  GetOriginsExceedingGroupLimit(const nsACString& aKey,
                                GroupInfoPair* aValue,
                                void* aUserArg);

  static PLDHashOperator
  GetAllTemporaryStorageOrigins(const nsACString& aKey,
                                GroupInfoPair* aValue,
                                void* aUserArg);

  static PLDHashOperator
  AddTemporaryStorageOrigins(const nsACString& aKey,
                             ArrayCluster<nsIOfflineStorage*>* aValue,
                             void* aUserArg);

  static PLDHashOperator
  GetInactiveTemporaryStorageOrigins(const nsACString& aKey,
                                     GroupInfoPair* aValue,
                                     void* aUserArg);

  
  unsigned int mCurrentWindowIndex;

  mozilla::Mutex mQuotaMutex;

  nsClassHashtable<nsCStringHashKey, GroupInfoPair> mGroupInfoPairs;

  
  nsRefPtrHashtable<nsPtrHashKey<nsPIDOMWindow>,
                    CheckQuotaHelper> mCheckQuotaHelpers;

  
  nsClassHashtable<nsCStringHashKey,
                   ArrayCluster<nsIOfflineStorage*> > mLiveStorages;

  
  nsAutoTArray<nsAutoPtr<SynchronizedOp>, 5> mSynchronizedOps;

  
  nsCOMPtr<nsIThread> mIOThread;

  
  nsCOMPtr<nsITimer> mShutdownTimer;

  
  
  nsTArray<nsCString> mInitializedOrigins;

  nsAutoTArray<nsRefPtr<Client>, Client::TYPE_MAX> mClients;

  nsString mIndexedDBPath;
  nsString mPersistentStoragePath;
  nsString mTemporaryStoragePath;

  uint64_t mTemporaryStorageLimit;
  uint64_t mTemporaryStorageUsage;
  bool mTemporaryStorageInitialized;

  bool mStorageAreaInitialized;
};

class AutoEnterWindow
{
public:
  explicit AutoEnterWindow(nsPIDOMWindow* aWindow)
  {
    QuotaManager::SetCurrentWindow(aWindow);
  }

  ~AutoEnterWindow()
  {
    QuotaManager::SetCurrentWindow(nullptr);
  }
};

END_QUOTA_NAMESPACE

#endif 
