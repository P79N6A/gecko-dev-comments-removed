





#ifndef mozilla_dom_quota_quotamanager_h__
#define mozilla_dom_quota_quotamanager_h__

#include "QuotaCommon.h"

#include "nsIObserver.h"
#include "nsIQuotaManager.h"

#include "mozilla/dom/Nullable.h"
#include "mozilla/dom/ipc/IdType.h"
#include "mozilla/Mutex.h"

#include "nsClassHashtable.h"
#include "nsRefPtrHashtable.h"

#include "Client.h"
#include "PersistenceType.h"

#define QUOTA_MANAGER_CONTRACTID "@mozilla.org/dom/quota/manager;1"

class nsIPrincipal;
class nsIThread;
class nsITimer;
class nsIURI;
class nsPIDOMWindow;
class nsIRunnable;

namespace mozilla {
namespace dom {
class OptionalContentId;
}
}

BEGIN_QUOTA_NAMESPACE

class DirectoryLockImpl;
class GroupInfo;
class GroupInfoPair;
class OriginInfo;
class OriginScope;
class QuotaObject;

class NS_NO_VTABLE RefCountedObject
{
public:
  NS_IMETHOD_(MozExternalRefCountType)
  AddRef() = 0;

  NS_IMETHOD_(MozExternalRefCountType)
  Release() = 0;
};



class DirectoryLock
  : public nsISupports
{
  friend class DirectoryLockImpl;

private:
  DirectoryLock()
  { }

  ~DirectoryLock()
  { }
};

class NS_NO_VTABLE OpenDirectoryListener
  : public RefCountedObject
{
public:
  virtual void
  DirectoryLockAcquired(DirectoryLock* aLock) = 0;

  virtual void
  DirectoryLockFailed() = 0;

protected:
  virtual ~OpenDirectoryListener()
  { }
};

struct OriginParams
{
  OriginParams(PersistenceType aPersistenceType,
               const nsACString& aOrigin,
               bool aIsApp)
  : mOrigin(aOrigin)
  , mPersistenceType(aPersistenceType)
  , mIsApp(aIsApp)
  { }

  nsCString mOrigin;
  PersistenceType mPersistenceType;
  bool mIsApp;
};

class QuotaManager final : public nsIQuotaManager,
                           public nsIObserver
{
  friend class DirectoryLockImpl;
  friend class GroupInfo;
  friend class OriginInfo;
  friend class QuotaObject;

  enum MozBrowserPatternFlag
  {
    MozBrowser = 0,
    NotMozBrowser,
    IgnoreMozBrowser
  };

  typedef nsClassHashtable<nsCStringHashKey,
                           nsTArray<DirectoryLockImpl*>> DirectoryLockTable;

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

  bool
  IsOriginInitialized(const nsACString& aOrigin) const
  {
    AssertIsOnIOThread();

    return mInitializedOrigins.Contains(aOrigin);
  }

  bool
  IsTemporaryStorageInitialized() const
  {
    AssertIsOnIOThread();

    return mTemporaryStorageInitialized;
  }

  void
  InitQuotaForOrigin(PersistenceType aPersistenceType,
                     const nsACString& aGroup,
                     const nsACString& aOrigin,
                     bool aIsApp,
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
  RemoveQuotaForOrigin(PersistenceType aPersistenceType,
                       const nsACString& aGroup,
                       const nsACString& aOrigin)
  {
    MutexAutoLock lock(mQuotaMutex);
    LockedRemoveQuotaForOrigin(aPersistenceType, aGroup, aOrigin);
  }

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

  
  
  void
  AbortOperationsForProcess(ContentParentId aContentParentId);

  nsresult
  GetDirectoryForOrigin(PersistenceType aPersistenceType,
                        const nsACString& aASCIIOrigin,
                        nsIFile** aDirectory) const;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void
  OpenDirectory(PersistenceType aPersistenceType,
                const nsACString& aGroup,
                const nsACString& aOrigin,
                bool aIsApp,
                Client::Type aClientType,
                bool aExclusive,
                OpenDirectoryListener* aOpenListener);

  
  void
  OpenDirectoryInternal(Nullable<PersistenceType> aPersistenceType,
                        const OriginScope& aOriginScope,
                        bool aExclusive,
                        OpenDirectoryListener* aOpenListener);

  
  uint64_t
  CollectOriginsForEviction(uint64_t aMinSizeToBeFreed,
                            nsTArray<nsRefPtr<DirectoryLockImpl>>& aLocks);

  nsresult
  EnsureOriginIsInitialized(PersistenceType aPersistenceType,
                            const nsACString& aGroup,
                            const nsACString& aOrigin,
                            bool aIsApp,
                            nsIFile** aDirectory);

  void
  OriginClearCompleted(PersistenceType aPersistenceType,
                       const nsACString& aOrigin,
                       bool aIsApp);

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

  Client*
  GetClient(Client::Type aClientType);

  const nsString&
  GetStoragePath() const
  {
    return mStoragePath;
  }

  const nsString&
  GetStoragePath(PersistenceType aPersistenceType) const
  {
    if (aPersistenceType == PERSISTENCE_TYPE_PERSISTENT) {
      return mPermanentStoragePath;
    }

    if (aPersistenceType == PERSISTENCE_TYPE_TEMPORARY) {
      return mTemporaryStoragePath;
    }

    MOZ_ASSERT(aPersistenceType == PERSISTENCE_TYPE_DEFAULT);

    return mDefaultStoragePath;
  }

  uint64_t
  GetGroupLimit() const;

  static void
  GetStorageId(PersistenceType aPersistenceType,
               const nsACString& aOrigin,
               Client::Type aClientType,
               nsACString& aDatabaseId);

  static nsresult
  GetInfoFromURI(nsIURI* aURI,
                 uint32_t aAppId,
                 bool aInMozBrowser,
                 nsACString* aGroup,
                 nsACString* aOrigin,
                 bool* aIsApp);

  static nsresult
  GetInfoFromPrincipal(nsIPrincipal* aPrincipal,
                       nsACString* aGroup,
                       nsACString* aOrigin,
                       bool* aIsApp);

  static nsresult
  GetInfoFromWindow(nsPIDOMWindow* aWindow,
                    nsACString* aGroup,
                    nsACString* aOrigin,
                    bool* aIsApp);

  static void
  GetInfoForChrome(nsACString* aGroup,
                   nsACString* aOrigin,
                   bool* aIsApp);

  static bool
  IsOriginWhitelistedForPersistentStorage(const nsACString& aOrigin);

  static bool
  IsFirstPromptRequired(PersistenceType aPersistenceType,
                        const nsACString& aOrigin,
                        bool aIsApp);

  static bool
  IsQuotaEnforced(PersistenceType aPersistenceType,
                  const nsACString& aOrigin,
                  bool aIsApp);

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

  static nsresult
  GetDirectoryMetadata(nsIFile* aDirectory,
                       int64_t* aTimestamp,
                       nsACString& aGroup,
                       nsACString& aOrigin,
                       bool* aIsApp);

private:
  QuotaManager();

  virtual ~QuotaManager();

  nsresult
  Init();

  already_AddRefed<DirectoryLockImpl>
  CreateDirectoryLock(Nullable<PersistenceType> aPersistenceType,
                      const nsACString& aGroup,
                      const OriginScope& aOriginScope,
                      Nullable<bool> aIsApp,
                      Nullable<Client::Type> aClientType,
                      bool aExclusive,
                      bool aInternal,
                      OpenDirectoryListener* aOpenListener);

  already_AddRefed<DirectoryLockImpl>
  CreateDirectoryLockForEviction(PersistenceType aPersistenceType,
                                 const nsACString& aGroup,
                                 const nsACString& aOrigin,
                                 bool aIsApp);

  void
  RegisterDirectoryLock(DirectoryLockImpl* aLock);

  void
  UnregisterDirectoryLock(DirectoryLockImpl* aLock);

  void
  RemovePendingDirectoryLock(DirectoryLockImpl* aLock);

  uint64_t
  LockedCollectOriginsForEviction(
                                 uint64_t aMinSizeToBeFreed,
                                 nsTArray<nsRefPtr<DirectoryLockImpl>>& aLocks);

  void
  LockedRemoveQuotaForOrigin(PersistenceType aPersistenceType,
                             const nsACString& aGroup,
                             const nsACString& aOrigin);

  nsresult
  MaybeUpgradeIndexedDBDirectory();

  nsresult
  MaybeUpgradePersistentStorageDirectory();

  nsresult
  MaybeUpgradeStorageArea();

  nsresult
  InitializeRepository(PersistenceType aPersistenceType);

  nsresult
  InitializeOrigin(PersistenceType aPersistenceType,
                   const nsACString& aGroup,
                   const nsACString& aOrigin,
                   bool aIsApp,
                   int64_t aAccessTime,
                   nsIFile* aDirectory);

  nsresult
  ClearStoragesForApp(uint32_t aAppId, bool aBrowserOnly);

  void
  CheckTemporaryStorageLimits();

  void
  DeleteFilesForOrigin(PersistenceType aPersistenceType,
                       const nsACString& aOrigin);

  void
  FinalizeOriginEviction(nsTArray<nsRefPtr<DirectoryLockImpl>>& aLocks);

  void
  ReleaseIOThreadObjects()
  {
    AssertIsOnIOThread();

    for (uint32_t index = 0; index < Client::TYPE_MAX; index++) {
      mClients[index]->ReleaseIOThreadObjects();
    }
  }

  DirectoryLockTable&
  GetDirectoryLockTable(PersistenceType aPersistenceType);

  static void
  GetOriginPatternString(uint32_t aAppId,
                         MozBrowserPatternFlag aBrowserFlag,
                         const nsACString& aOrigin,
                         nsAutoCString& _retval);

  static PLDHashOperator
  RemoveQuotaCallback(const nsACString& aKey,
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

  mozilla::Mutex mQuotaMutex;

  nsClassHashtable<nsCStringHashKey, GroupInfoPair> mGroupInfoPairs;

  
  nsTArray<nsRefPtr<DirectoryLockImpl>> mPendingDirectoryLocks;

  
  nsTArray<DirectoryLockImpl*> mDirectoryLocks;

  
  DirectoryLockTable mTemporaryDirectoryLockTable;
  DirectoryLockTable mDefaultDirectoryLockTable;

  
  nsCOMPtr<nsIThread> mIOThread;

  
  nsCOMPtr<nsITimer> mShutdownTimer;

  
  
  nsTArray<nsCString> mInitializedOrigins;

  nsAutoTArray<nsRefPtr<Client>, Client::TYPE_MAX> mClients;

  nsString mIndexedDBPath;
  nsString mStoragePath;
  nsString mPermanentStoragePath;
  nsString mTemporaryStoragePath;
  nsString mDefaultStoragePath;

  uint64_t mTemporaryStorageLimit;
  uint64_t mTemporaryStorageUsage;
  bool mTemporaryStorageInitialized;

  bool mStorageAreaInitialized;
};

END_QUOTA_NAMESPACE

#endif 
