





#ifndef mozilla_dom_quota_quotamanager_h__
#define mozilla_dom_quota_quotamanager_h__

#include "QuotaCommon.h"

#include "nsIObserver.h"
#include "nsIQuotaManager.h"

#include "mozilla/Mutex.h"
#include "nsClassHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsThreadUtils.h"

#include "ArrayCluster.h"
#include "Client.h"
#include "StoragePrivilege.h"

#define QUOTA_MANAGER_CONTRACTID "@mozilla.org/dom/quota/manager;1"

class nsIAtom;
class nsIOfflineStorage;
class nsIPrincipal;
class nsIThread;
class nsITimer;
class nsIURI;
class nsPIDOMWindow;

BEGIN_QUOTA_NAMESPACE

class AcquireListener;
class AsyncUsageRunnable;
class CheckQuotaHelper;
class OriginClearRunnable;
class OriginInfo;
class OriginOrPatternString;
class QuotaObject;
struct SynchronizedOp;

class QuotaManager MOZ_FINAL : public nsIQuotaManager,
                               public nsIObserver
{
  friend class AsyncUsageRunnable;
  friend class OriginClearRunnable;
  friend class OriginInfo;
  friend class QuotaObject;

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
  InitQuotaForOrigin(const nsACString& aOrigin,
                     int64_t aLimitBytes,
                     int64_t aUsageBytes);

  void
  DecreaseUsageForOrigin(const nsACString& aOrigin,
                         int64_t aSize);

  void
  RemoveQuotaForPattern(const nsACString& aPattern);

  already_AddRefed<QuotaObject>
  GetQuotaObject(const nsACString& aOrigin,
                 nsIFile* aFile);

  already_AddRefed<QuotaObject>
  GetQuotaObject(const nsACString& aOrigin,
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

  
  bool
  HasOpenTransactions(nsPIDOMWindow* aWindow);

  
  
  nsresult
  WaitForOpenAllowed(const OriginOrPatternString& aOriginOrPattern,
                     nsIAtom* aId,
                     nsIRunnable* aRunnable);

  
  
  
  nsresult
  AcquireExclusiveAccess(nsIOfflineStorage* aStorage,
                         const nsACString& aOrigin,
                         AcquireListener* aListener,
                         WaitingOnStoragesCallback aCallback,
                         void* aClosure)
  {
    NS_ASSERTION(aStorage, "Need a storage here!");
    return AcquireExclusiveAccess(aOrigin, aStorage, aListener, aCallback,
                                  aClosure);
  }

  nsresult
  AcquireExclusiveAccess(const nsACString& aOrigin,
                         AcquireListener* aListener,
                         WaitingOnStoragesCallback aCallback,
                         void* aClosure)
  {
    return AcquireExclusiveAccess(aOrigin, nullptr, aListener, aCallback,
                                  aClosure);
  }

  void
  AllowNextSynchronizedOp(const OriginOrPatternString& aOriginOrPattern,
                          nsIAtom* aId);

  bool
  IsClearOriginPending(const nsACString& aPattern)
  {
    return !!FindSynchronizedOp(aPattern, nullptr);
  }

  nsresult
  GetDirectoryForOrigin(const nsACString& aASCIIOrigin,
                        nsIFile** aDirectory) const;

  nsresult
  EnsureOriginIsInitialized(const nsACString& aOrigin,
                            bool aTrackQuota,
                            nsIFile** aDirectory);

  void
  UninitializeOriginsByPattern(const nsACString& aPattern);

  nsIThread*
  IOThread()
  {
    NS_ASSERTION(mIOThread, "This should never be null!");
    return mIOThread;
  }

  already_AddRefed<Client>
  GetClient(Client::Type aClientType);

  const nsString&
  GetBaseDirectory() const
  {
    return mStorageBasePath;
  }

  static uint32_t
  GetStorageQuotaMB();

  static already_AddRefed<nsIAtom>
  GetStorageId(const nsACString& aOrigin,
               const nsAString& aName);

  static nsresult
  GetASCIIOriginFromURI(nsIURI* aURI,
                        uint32_t aAppId,
                        bool aInMozBrowser,
                        nsACString& aASCIIOrigin);

  static nsresult
  GetASCIIOriginFromPrincipal(nsIPrincipal* aPrincipal,
                              nsACString& aASCIIOrigin);

  static nsresult
  GetASCIIOriginFromWindow(nsPIDOMWindow* aWindow,
                           nsACString& aASCIIOrigin);

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

  nsresult
  AcquireExclusiveAccess(const nsACString& aOrigin,
                         nsIOfflineStorage* aStorage,
                         AcquireListener* aListener,
                         WaitingOnStoragesCallback aCallback,
                         void* aClosure);

  nsresult
  RunSynchronizedOp(nsIOfflineStorage* aStorage,
                    SynchronizedOp* aOp);

  SynchronizedOp*
  FindSynchronizedOp(const nsACString& aPattern,
                     nsISupports* aId);

  nsresult
  ClearStoragesForApp(uint32_t aAppId, bool aBrowserOnly);

  nsresult
  MaybeUpgradeOriginDirectory(nsIFile* aDirectory);

  static void
  GetOriginPatternString(uint32_t aAppId,
                         MozBrowserPatternFlag aBrowserFlag,
                         const nsACString& aOrigin,
                         nsAutoCString& _retval);

  
  unsigned int mCurrentWindowIndex;

  mozilla::Mutex mQuotaMutex;

  nsRefPtrHashtable<nsCStringHashKey, OriginInfo> mOriginInfos;

  
  nsRefPtrHashtable<nsPtrHashKey<nsPIDOMWindow>,
                    CheckQuotaHelper> mCheckQuotaHelpers;

  
  nsClassHashtable<nsCStringHashKey,
                   ArrayCluster<nsIOfflineStorage*> > mLiveStorages;

  
  nsAutoTArray<nsAutoPtr<SynchronizedOp>, 5> mSynchronizedOps;

  
  nsCOMPtr<nsIThread> mIOThread;

  
  nsCOMPtr<nsITimer> mShutdownTimer;

  
  
  nsTArray<nsCString> mInitializedOrigins;

  nsAutoTArray<nsRefPtr<Client>, Client::TYPE_MAX> mClients;

  nsString mStorageBasePath;
};

class AutoEnterWindow
{
public:
  AutoEnterWindow(nsPIDOMWindow* aWindow)
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
