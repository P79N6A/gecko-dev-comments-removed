





#include "ProcessPriorityManager.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/Hal.h"
#include "mozilla/IntegerPrintfMacros.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/unused.h"
#include "AudioChannelService.h"
#include "prlog.h"
#include "nsPrintfCString.h"
#include "nsXULAppAPI.h"
#include "nsIFrameLoader.h"
#include "nsIObserverService.h"
#include "StaticPtr.h"
#include "nsIMozBrowserFrame.h"
#include "nsIObserver.h"
#include "nsITimer.h"
#include "nsIPropertyBag2.h"
#include "nsComponentManagerUtils.h"

#ifdef XP_WIN
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#ifdef LOG
#undef LOG
#endif

#include <utility>









#if defined(ANDROID) && defined(ENABLE_LOGGING)
#  include <android/log.h>
#  define LOG(fmt, ...) \
     __android_log_print(ANDROID_LOG_INFO, \
       "Gecko:ProcessPriorityManager", \
       fmt, ## __VA_ARGS__)
#  define LOGP(fmt, ...) \
    __android_log_print(ANDROID_LOG_INFO, \
      "Gecko:ProcessPriorityManager", \
      "[%schild-id=%" PRIu64 ", pid=%d] " fmt, \
      NameWithComma().get(), \
      static_cast<uint64_t>(ChildID()), Pid(), ## __VA_ARGS__)

#elif defined(ENABLE_LOGGING)
#  define LOG(fmt, ...) \
     printf("ProcessPriorityManager - " fmt "\n", ##__VA_ARGS__)
#  define LOGP(fmt, ...) \
     printf("ProcessPriorityManager[%schild-id=%" PRIu64 ", pid=%d] - " \
       fmt "\n", \
       NameWithComma().get(), \
       static_cast<uint64_t>(ChildID()), Pid(), ##__VA_ARGS__)

#elif defined(PR_LOGGING)
  static PRLogModuleInfo*
  GetPPMLog()
  {
    static PRLogModuleInfo *sLog;
    if (!sLog)
      sLog = PR_NewLogModule("ProcessPriorityManager");
    return sLog;
  }
#  define LOG(fmt, ...) \
     PR_LOG(GetPPMLog(), PR_LOG_DEBUG, \
            ("ProcessPriorityManager - " fmt, ##__VA_ARGS__))
#  define LOGP(fmt, ...) \
     PR_LOG(GetPPMLog(), PR_LOG_DEBUG, \
            ("ProcessPriorityManager[%schild-id=%" PRIu64 ", pid=%d] - " fmt, \
            NameWithComma().get(), \
            static_cast<uint64_t>(ChildID()), Pid(), ##__VA_ARGS__))
#else
#define LOG(fmt, ...)
#define LOGP(fmt, ...)
#endif

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::hal;

namespace {

class ParticularProcessPriorityManager;

class ProcessLRUPool final
{
public:
  


  ProcessLRUPool(ProcessPriority aPriority, uint32_t aBias);

  



  void Remove(ParticularProcessPriorityManager* aParticularManager);

  



  void Add(ParticularProcessPriorityManager* aParticularManager);

private:
  ProcessPriority mPriority;
  uint32_t mLRUPoolLevels;
  uint32_t mLRUPoolSize;
  uint32_t mBias;
  nsTArray<ParticularProcessPriorityManager*> mLRUPool;

  uint32_t CalculateLRULevel(uint32_t aLRUPoolIndex);

  void AdjustLRUValues(
    nsTArray<ParticularProcessPriorityManager*>::index_type aStart,
    bool removed);

  DISALLOW_EVIL_CONSTRUCTORS(ProcessLRUPool);
};












class ProcessPriorityManagerImpl final
  : public nsIObserver
  , public WakeLockObserver
{
public:
  



  static ProcessPriorityManagerImpl* GetSingleton();

  static void StaticInit();
  static bool PrefsEnabled();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  


  void SetProcessPriority(ContentParent* aContentParent,
                          ProcessPriority aPriority,
                          uint32_t aLRU = 0);

  



  void FireTestOnlyObserverNotification(const char* aTopic,
                                        const nsACString& aData = EmptyCString());

  


  bool ChildProcessHasHighPriority();

  



  void NotifyProcessPriorityChanged(
    ParticularProcessPriorityManager* aParticularManager,
    hal::ProcessPriority aOldPriority);

  



  virtual void Notify(const WakeLockInformation& aInfo) override;

  


  void Freeze();

  



  void Unfreeze();

  



  uint32_t NumberOfForegroundProcesses();

  


  void ScheduleDelayedSetPriority(
    ParticularProcessPriorityManager* aParticularManager,
    hal::ProcessPriority aPriority);

  



  void PerformDelayedSetPriority(
    ParticularProcessPriorityManager* aLastParticularManager);

private:
  static bool sPrefListenersRegistered;
  static bool sInitialized;
  static bool sFrozen;
  static StaticRefPtr<ProcessPriorityManagerImpl> sSingleton;

  static void PrefChangedCallback(const char* aPref, void* aClosure);

  ProcessPriorityManagerImpl();
  ~ProcessPriorityManagerImpl();
  DISALLOW_EVIL_CONSTRUCTORS(ProcessPriorityManagerImpl);

  void Init();

  already_AddRefed<ParticularProcessPriorityManager>
  GetParticularProcessPriorityManager(ContentParent* aContentParent);

  void ObserveContentParentCreated(nsISupports* aContentParent);
  void ObserveContentParentDestroyed(nsISupports* aSubject);

  nsDataHashtable<nsUint64HashKey, nsRefPtr<ParticularProcessPriorityManager> >
    mParticularManagers;

  
  bool mHighPriority;

  
  nsTHashtable<nsUint64HashKey> mHighPriorityChildIDs;

  
  ProcessLRUPool mBackgroundLRUPool;

  
  ProcessLRUPool mForegroundLRUPool;

  
  std::pair<nsRefPtr<ParticularProcessPriorityManager>, hal::ProcessPriority>
    mDelayedSetPriority;
};





class ProcessPriorityManagerChild final
  : public nsIObserver
{
public:
  static void StaticInit();
  static ProcessPriorityManagerChild* Singleton();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  bool CurrentProcessIsForeground();
  bool CurrentProcessIsHighPriority();

private:
  static StaticRefPtr<ProcessPriorityManagerChild> sSingleton;

  ProcessPriorityManagerChild();
  ~ProcessPriorityManagerChild() {}
  DISALLOW_EVIL_CONSTRUCTORS(ProcessPriorityManagerChild);

  void Init();

  hal::ProcessPriority mCachedPriority;
};





class ParticularProcessPriorityManager final
  : public WakeLockObserver
  , public nsIObserver
  , public nsITimerCallback
  , public nsSupportsWeakReference
{
  ~ParticularProcessPriorityManager();
public:
  explicit ParticularProcessPriorityManager(ContentParent* aContentParent,
                                            bool aFrozen = false);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSITIMERCALLBACK

  virtual void Notify(const WakeLockInformation& aInfo) override;
  void Init();

  int32_t Pid() const;
  uint64_t ChildID() const;
  bool IsPreallocated() const;

  








  const nsAutoCString& NameWithComma();

  bool HasAppType(const char* aAppType);
  bool IsExpectingSystemMessage();

  void OnAudioChannelProcessChanged(nsISupports* aSubject);
  void OnRemoteBrowserFrameShown(nsISupports* aSubject);
  void OnTabParentDestroyed(nsISupports* aSubject);
  void OnFrameloaderVisibleChanged(nsISupports* aSubject);

  ProcessPriority CurrentPriority();
  ProcessPriority ComputePriority();

  void ScheduleResetPriority(const char* aTimeoutPref);
  void ResetPriority();
  void ResetPriorityNow();
  void SetPriorityNow(ProcessPriority aPriority, uint32_t aLRU = 0);
  void Freeze();
  void Unfreeze();

  void ShutDown();

private:
  void FireTestOnlyObserverNotification(
    const char* aTopic,
    const nsACString& aData = EmptyCString());

  void FireTestOnlyObserverNotification(
    const char* aTopic,
    const char* aData = nullptr);

  ContentParent* mContentParent;
  uint64_t mChildID;
  ProcessPriority mPriority;
  uint32_t mLRU;
  bool mHoldsCPUWakeLock;
  bool mHoldsHighPriorityWakeLock;
  bool mFrozen;

  


  nsAutoCString mNameWithComma;

  nsCOMPtr<nsITimer> mResetPriorityTimer;
};

 bool ProcessPriorityManagerImpl::sInitialized = false;
 bool ProcessPriorityManagerImpl::sPrefListenersRegistered = false;
 bool ProcessPriorityManagerImpl::sFrozen = false;
 StaticRefPtr<ProcessPriorityManagerImpl>
  ProcessPriorityManagerImpl::sSingleton;

NS_IMPL_ISUPPORTS(ProcessPriorityManagerImpl,
                  nsIObserver);

 void
ProcessPriorityManagerImpl::PrefChangedCallback(const char* aPref,
                                                void* aClosure)
{
  StaticInit();
}

 bool
ProcessPriorityManagerImpl::PrefsEnabled()
{
  return Preferences::GetBool("dom.ipc.processPriorityManager.enabled") &&
         !Preferences::GetBool("dom.ipc.tabs.disabled");
}

 void
ProcessPriorityManagerImpl::StaticInit()
{
  if (sInitialized) {
    return;
  }

  
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    sInitialized = true;
    return;
  }

  
  if (!PrefsEnabled()) {
    LOG("InitProcessPriorityManager bailing due to prefs.");

    
    
    if (!sPrefListenersRegistered) {
      sPrefListenersRegistered = true;
      Preferences::RegisterCallback(PrefChangedCallback,
                                    "dom.ipc.processPriorityManager.enabled");
      Preferences::RegisterCallback(PrefChangedCallback,
                                    "dom.ipc.tabs.disabled");
    }
    return;
  }

  sInitialized = true;

  sSingleton = new ProcessPriorityManagerImpl();
  sSingleton->Init();
  ClearOnShutdown(&sSingleton);
}

 ProcessPriorityManagerImpl*
ProcessPriorityManagerImpl::GetSingleton()
{
  if (!sSingleton) {
    StaticInit();
  }

  return sSingleton;
}

ProcessPriorityManagerImpl::ProcessPriorityManagerImpl()
    : mHighPriority(false)
    , mBackgroundLRUPool(PROCESS_PRIORITY_BACKGROUND, 1)
    , mForegroundLRUPool(PROCESS_PRIORITY_FOREGROUND, 0)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  RegisterWakeLockObserver(this);
  mDelayedSetPriority = std::make_pair(nullptr, PROCESS_PRIORITY_UNKNOWN);
}

ProcessPriorityManagerImpl::~ProcessPriorityManagerImpl()
{
  UnregisterWakeLockObserver(this);
}

void
ProcessPriorityManagerImpl::Init()
{
  LOG("Starting up.  This is the master process.");

  
  
  
  hal::SetProcessPriority(getpid(), PROCESS_PRIORITY_MASTER);

  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  if (os) {
    os->AddObserver(this, "ipc:content-created",  false);
    os->AddObserver(this, "ipc:content-shutdown",  false);
  }
}

NS_IMETHODIMP
ProcessPriorityManagerImpl::Observe(
  nsISupports* aSubject,
  const char* aTopic,
  const char16_t* aData)
{
  nsDependentCString topic(aTopic);
  if (topic.EqualsLiteral("ipc:content-created")) {
    ObserveContentParentCreated(aSubject);
  } else if (topic.EqualsLiteral("ipc:content-shutdown")) {
    ObserveContentParentDestroyed(aSubject);
  } else {
    MOZ_ASSERT(false);
  }

  return NS_OK;
}

already_AddRefed<ParticularProcessPriorityManager>
ProcessPriorityManagerImpl::GetParticularProcessPriorityManager(
  ContentParent* aContentParent)
{
#ifdef MOZ_NUWA_PROCESS
  
  if (aContentParent->IsNuwaProcess()) {
    return nullptr;
  }
#endif

  nsRefPtr<ParticularProcessPriorityManager> pppm;
  uint64_t cpId = aContentParent->ChildID();
  mParticularManagers.Get(cpId, &pppm);
  if (!pppm) {
    pppm = new ParticularProcessPriorityManager(aContentParent, sFrozen);
    pppm->Init();
    mParticularManagers.Put(cpId, pppm);

    FireTestOnlyObserverNotification("process-created",
      nsPrintfCString("%lld", cpId));
  }

  return pppm.forget();
}

void
ProcessPriorityManagerImpl::SetProcessPriority(ContentParent* aContentParent,
                                               ProcessPriority aPriority,
                                               uint32_t aLRU)
{
  MOZ_ASSERT(aContentParent);
  nsRefPtr<ParticularProcessPriorityManager> pppm =
    GetParticularProcessPriorityManager(aContentParent);
  if (pppm) {
    pppm->SetPriorityNow(aPriority, aLRU);
  }
}

void
ProcessPriorityManagerImpl::ObserveContentParentCreated(
  nsISupports* aContentParent)
{
  
  
  nsCOMPtr<nsIContentParent> cp = do_QueryInterface(aContentParent);
  nsRefPtr<ParticularProcessPriorityManager> pppm =
    GetParticularProcessPriorityManager(cp->AsContentParent());
}

void
ProcessPriorityManagerImpl::ObserveContentParentDestroyed(nsISupports* aSubject)
{
  nsCOMPtr<nsIPropertyBag2> props = do_QueryInterface(aSubject);
  NS_ENSURE_TRUE_VOID(props);

  uint64_t childID = CONTENT_PROCESS_ID_UNKNOWN;
  props->GetPropertyAsUint64(NS_LITERAL_STRING("childID"), &childID);
  NS_ENSURE_TRUE_VOID(childID != CONTENT_PROCESS_ID_UNKNOWN);

  nsRefPtr<ParticularProcessPriorityManager> pppm;
  mParticularManagers.Get(childID, &pppm);
  if (pppm) {
    
    mBackgroundLRUPool.Remove(pppm);
    mForegroundLRUPool.Remove(pppm);

    pppm->ShutDown();

    mParticularManagers.Remove(childID);

    if (mHighPriorityChildIDs.Contains(childID)) {
      mHighPriorityChildIDs.RemoveEntry(childID);
    }

    if (mDelayedSetPriority.first == pppm) {
      mDelayedSetPriority = std::make_pair(nullptr, PROCESS_PRIORITY_UNKNOWN);
    }
  }
}

bool
ProcessPriorityManagerImpl::ChildProcessHasHighPriority( void )
{
  return mHighPriorityChildIDs.Count() > 0;
}

void
ProcessPriorityManagerImpl::NotifyProcessPriorityChanged(
  ParticularProcessPriorityManager* aParticularManager,
  ProcessPriority aOldPriority)
{
  ProcessPriority newPriority = aParticularManager->CurrentPriority();
  bool isPreallocated = aParticularManager->IsPreallocated();

  if (newPriority == PROCESS_PRIORITY_BACKGROUND &&
      aOldPriority != PROCESS_PRIORITY_BACKGROUND &&
      !isPreallocated) {
    mBackgroundLRUPool.Add(aParticularManager);
  } else if (newPriority != PROCESS_PRIORITY_BACKGROUND &&
      aOldPriority == PROCESS_PRIORITY_BACKGROUND &&
      !isPreallocated) {
    mBackgroundLRUPool.Remove(aParticularManager);
  }

  if (newPriority == PROCESS_PRIORITY_FOREGROUND &&
      aOldPriority != PROCESS_PRIORITY_FOREGROUND) {
    mForegroundLRUPool.Add(aParticularManager);
  } else if (newPriority != PROCESS_PRIORITY_FOREGROUND &&
      aOldPriority == PROCESS_PRIORITY_FOREGROUND) {
    mForegroundLRUPool.Remove(aParticularManager);
  }

  if (newPriority >= PROCESS_PRIORITY_FOREGROUND_HIGH &&
    aOldPriority < PROCESS_PRIORITY_FOREGROUND_HIGH) {
    mHighPriorityChildIDs.PutEntry(aParticularManager->ChildID());
  } else if (newPriority < PROCESS_PRIORITY_FOREGROUND_HIGH &&
    aOldPriority >= PROCESS_PRIORITY_FOREGROUND_HIGH) {
    mHighPriorityChildIDs.RemoveEntry(aParticularManager->ChildID());
  }
}

 void
ProcessPriorityManagerImpl::Notify(const WakeLockInformation& aInfo)
{
  


  if (aInfo.topic().EqualsLiteral("high-priority")) {
    if (aInfo.lockingProcesses().Contains((uint64_t)0)) {
      mHighPriority = true;
    } else {
      mHighPriority = false;
    }

    LOG("Got wake lock changed event. "
        "Now mHighPriorityParent = %d\n", mHighPriority);
  }
}

static PLDHashOperator
FreezeParticularProcessPriorityManagers(
  const uint64_t& aKey,
  nsRefPtr<ParticularProcessPriorityManager> aValue,
  void* aUserData)
{
  aValue->Freeze();
  return PL_DHASH_NEXT;
}

void
ProcessPriorityManagerImpl::Freeze()
{
  sFrozen = true;
  mParticularManagers.EnumerateRead(&FreezeParticularProcessPriorityManagers,
                                    nullptr);
}

static PLDHashOperator
UnfreezeParticularProcessPriorityManagers(
  const uint64_t& aKey,
  nsRefPtr<ParticularProcessPriorityManager> aValue,
  void* aUserData)
{
  aValue->Unfreeze();
  return PL_DHASH_NEXT;
}

void
ProcessPriorityManagerImpl::Unfreeze()
{
  sFrozen = false;
  mParticularManagers.EnumerateRead(&UnfreezeParticularProcessPriorityManagers,
                                    nullptr);
}

static PLDHashOperator
CountNumberOfForegroundProcesses(
  const uint64_t& aKey,
  nsRefPtr<ParticularProcessPriorityManager> aValue,
  void* aUserData)
{
  uint32_t* accumulator = static_cast<uint32_t*>(aUserData);
  if (aValue->CurrentPriority() == PROCESS_PRIORITY_FOREGROUND ||
      aValue->CurrentPriority() == PROCESS_PRIORITY_FOREGROUND_HIGH) {
    (*accumulator)++;
  }
  return PL_DHASH_NEXT;
}

uint32_t
ProcessPriorityManagerImpl::NumberOfForegroundProcesses()
{
  uint32_t accumulator = 0;
  mParticularManagers.EnumerateRead(&CountNumberOfForegroundProcesses,
                                    &accumulator);
  return accumulator;
}

void
ProcessPriorityManagerImpl::ScheduleDelayedSetPriority(
  ParticularProcessPriorityManager* aParticularManager,
  ProcessPriority aPriority)
{
  mDelayedSetPriority = std::make_pair(aParticularManager, aPriority);
}

void
ProcessPriorityManagerImpl::PerformDelayedSetPriority(
  ParticularProcessPriorityManager* aLastParticularManager)
{
  nsRefPtr<ParticularProcessPriorityManager> pppm = mDelayedSetPriority.first;
  ProcessPriority priority = mDelayedSetPriority.second;

  mDelayedSetPriority = std::make_pair(nullptr, PROCESS_PRIORITY_UNKNOWN);

  if (pppm == aLastParticularManager) {
    return;
  }

  if (pppm && priority != PROCESS_PRIORITY_UNKNOWN) {
    pppm->SetPriorityNow(priority);
  }
}

NS_IMPL_ISUPPORTS(ParticularProcessPriorityManager,
                  nsIObserver,
                  nsITimerCallback,
                  nsISupportsWeakReference);

ParticularProcessPriorityManager::ParticularProcessPriorityManager(
  ContentParent* aContentParent, bool aFrozen)
  : mContentParent(aContentParent)
  , mChildID(aContentParent->ChildID())
  , mPriority(PROCESS_PRIORITY_UNKNOWN)
  , mLRU(0)
  , mHoldsCPUWakeLock(false)
  , mHoldsHighPriorityWakeLock(false)
  , mFrozen(aFrozen)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  LOGP("Creating ParticularProcessPriorityManager.");
}

void
ParticularProcessPriorityManager::Init()
{
  RegisterWakeLockObserver(this);

  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  if (os) {
    os->AddObserver(this, "audio-channel-process-changed",  true);
    os->AddObserver(this, "remote-browser-shown",  true);
    os->AddObserver(this, "ipc:browser-destroyed",  true);
    os->AddObserver(this, "frameloader-visible-changed",  true);
  }

  
  
  WakeLockInformation info1, info2;
  GetWakeLockInfo(NS_LITERAL_STRING("cpu"), &info1);
  mHoldsCPUWakeLock = info1.lockingProcesses().Contains(ChildID());

  GetWakeLockInfo(NS_LITERAL_STRING("high-priority"), &info2);
  mHoldsHighPriorityWakeLock = info2.lockingProcesses().Contains(ChildID());
  LOGP("Done starting up.  mHoldsCPUWakeLock=%d, mHoldsHighPriorityWakeLock=%d",
       mHoldsCPUWakeLock, mHoldsHighPriorityWakeLock);
}

ParticularProcessPriorityManager::~ParticularProcessPriorityManager()
{
  LOGP("Destroying ParticularProcessPriorityManager.");

  
  
  
  

  if (mContentParent) {
    UnregisterWakeLockObserver(this);
  }
}

 void
ParticularProcessPriorityManager::Notify(const WakeLockInformation& aInfo)
{
  if (!mContentParent) {
    
    return;
  }

  bool* dest = nullptr;
  if (aInfo.topic().EqualsLiteral("cpu")) {
    dest = &mHoldsCPUWakeLock;
  } else if (aInfo.topic().EqualsLiteral("high-priority")) {
    dest = &mHoldsHighPriorityWakeLock;
  }

  if (dest) {
    bool thisProcessLocks = aInfo.lockingProcesses().Contains(ChildID());
    if (thisProcessLocks != *dest) {
      *dest = thisProcessLocks;
      LOGP("Got wake lock changed event. "
           "Now mHoldsCPUWakeLock=%d, mHoldsHighPriorityWakeLock=%d",
           mHoldsCPUWakeLock, mHoldsHighPriorityWakeLock);
      ResetPriority();
    }
  }
}

NS_IMETHODIMP
ParticularProcessPriorityManager::Observe(nsISupports* aSubject,
                                          const char* aTopic,
                                          const char16_t* aData)
{
  if (!mContentParent) {
    
    return NS_OK;
  }

  nsDependentCString topic(aTopic);

  if (topic.EqualsLiteral("audio-channel-process-changed")) {
    OnAudioChannelProcessChanged(aSubject);
  } else if (topic.EqualsLiteral("remote-browser-shown")) {
    OnRemoteBrowserFrameShown(aSubject);
  } else if (topic.EqualsLiteral("ipc:browser-destroyed")) {
    OnTabParentDestroyed(aSubject);
  } else if (topic.EqualsLiteral("frameloader-visible-changed")) {
    OnFrameloaderVisibleChanged(aSubject);
  } else {
    MOZ_ASSERT(false);
  }

  return NS_OK;
}

uint64_t
ParticularProcessPriorityManager::ChildID() const
{
  
  
  
  
  return mChildID;
}

int32_t
ParticularProcessPriorityManager::Pid() const
{
  return mContentParent ? mContentParent->Pid() : -1;
}

bool
ParticularProcessPriorityManager::IsPreallocated() const
{
  return mContentParent ? mContentParent->IsPreallocated() : false;
}

const nsAutoCString&
ParticularProcessPriorityManager::NameWithComma()
{
  mNameWithComma.Truncate();
  if (!mContentParent) {
    return mNameWithComma; 
  }

  nsAutoString name;
  mContentParent->FriendlyName(name);
  if (name.IsEmpty()) {
    return mNameWithComma; 
  }

  mNameWithComma = NS_ConvertUTF16toUTF8(name);
  mNameWithComma.AppendLiteral(", ");
  return mNameWithComma;
}

void
ParticularProcessPriorityManager::OnAudioChannelProcessChanged(nsISupports* aSubject)
{
  nsCOMPtr<nsIPropertyBag2> props = do_QueryInterface(aSubject);
  NS_ENSURE_TRUE_VOID(props);

  uint64_t childID = CONTENT_PROCESS_ID_UNKNOWN;
  props->GetPropertyAsUint64(NS_LITERAL_STRING("childID"), &childID);
  if (childID == ChildID()) {
    ResetPriority();
  }
}

void
ParticularProcessPriorityManager::OnRemoteBrowserFrameShown(nsISupports* aSubject)
{
  nsCOMPtr<nsIFrameLoader> fl = do_QueryInterface(aSubject);
  NS_ENSURE_TRUE_VOID(fl);

  TabParent* tp = TabParent::GetFrom(fl);
  NS_ENSURE_TRUE_VOID(tp);

  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  if (tp->Manager() != mContentParent) {
    return;
  }

  
  bool isBrowserOrApp;
  fl->GetOwnerIsBrowserOrAppFrame(&isBrowserOrApp);
  if (isBrowserOrApp) {
    ResetPriority();
  }

  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  if (os) {
    os->RemoveObserver(this, "remote-browser-shown");
  }
}

void
ParticularProcessPriorityManager::OnTabParentDestroyed(nsISupports* aSubject)
{
  nsCOMPtr<nsITabParent> tp = do_QueryInterface(aSubject);
  NS_ENSURE_TRUE_VOID(tp);

  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  if (TabParent::GetFrom(tp)->Manager() != mContentParent) {
    return;
  }

  ResetPriority();
}

void
ParticularProcessPriorityManager::OnFrameloaderVisibleChanged(nsISupports* aSubject)
{
  nsCOMPtr<nsIFrameLoader> fl = do_QueryInterface(aSubject);
  NS_ENSURE_TRUE_VOID(fl);

  TabParent* tp = TabParent::GetFrom(fl);
  if (!tp) {
    return;
  }

  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  if (tp->Manager() != mContentParent) {
    return;
  }

  
  
  
  
  
  
  
  
  
  
  
  

  ResetPriorityNow();
}

void
ParticularProcessPriorityManager::ResetPriority()
{
  ProcessPriority processPriority = ComputePriority();
  if (mPriority == PROCESS_PRIORITY_UNKNOWN ||
      mPriority > processPriority) {
    
    
    
    
    
    if (mPriority == PROCESS_PRIORITY_BACKGROUND_PERCEIVABLE) {
      ScheduleResetPriority("backgroundPerceivableGracePeriodMS");
    } else {
      ScheduleResetPriority("backgroundGracePeriodMS");
    }
    return;
  }

  SetPriorityNow(processPriority);
}

void
ParticularProcessPriorityManager::ResetPriorityNow()
{
  SetPriorityNow(ComputePriority());
}

void
ParticularProcessPriorityManager::ScheduleResetPriority(const char* aTimeoutPref)
{
  if (mResetPriorityTimer) {
    LOGP("ScheduleResetPriority bailing; the timer is already running.");
    return;
  }

  uint32_t timeout = Preferences::GetUint(
    nsPrintfCString("dom.ipc.processPriorityManager.%s", aTimeoutPref).get());
  LOGP("Scheduling reset timer to fire in %dms.", timeout);
  mResetPriorityTimer = do_CreateInstance("@mozilla.org/timer;1");
  mResetPriorityTimer->InitWithCallback(this, timeout, nsITimer::TYPE_ONE_SHOT);
}

NS_IMETHODIMP
ParticularProcessPriorityManager::Notify(nsITimer* aTimer)
{
  LOGP("Reset priority timer callback; about to ResetPriorityNow.");
  ResetPriorityNow();
  mResetPriorityTimer = nullptr;
  return NS_OK;
}

bool
ParticularProcessPriorityManager::HasAppType(const char* aAppType)
{
  const InfallibleTArray<PBrowserParent*>& browsers =
    mContentParent->ManagedPBrowserParent();
  for (uint32_t i = 0; i < browsers.Length(); i++) {
    nsAutoString appType;
    TabParent::GetFrom(browsers[i])->GetAppType(appType);
    if (appType.EqualsASCII(aAppType)) {
      return true;
    }
  }

  return false;
}

bool
ParticularProcessPriorityManager::IsExpectingSystemMessage()
{
  const InfallibleTArray<PBrowserParent*>& browsers =
    mContentParent->ManagedPBrowserParent();
  for (uint32_t i = 0; i < browsers.Length(); i++) {
    TabParent* tp = TabParent::GetFrom(browsers[i]);
    nsCOMPtr<nsIMozBrowserFrame> bf = do_QueryInterface(tp->GetOwnerElement());
    if (!bf) {
      continue;
    }

    if (bf->GetIsExpectingSystemMessage()) {
      return true;
    }
  }

  return false;
}

ProcessPriority
ParticularProcessPriorityManager::CurrentPriority()
{
  return mPriority;
}

ProcessPriority
ParticularProcessPriorityManager::ComputePriority()
{
  if ((mHoldsCPUWakeLock || mHoldsHighPriorityWakeLock) &&
      HasAppType("critical")) {
    return PROCESS_PRIORITY_FOREGROUND_HIGH;
  }

  bool isVisible = false;
  const InfallibleTArray<PBrowserParent*>& browsers =
    mContentParent->ManagedPBrowserParent();
  for (uint32_t i = 0; i < browsers.Length(); i++) {
    if (TabParent::GetFrom(browsers[i])->IsVisible()) {
      isVisible = true;
      break;
    }
  }

  if (isVisible) {
    return HasAppType("inputmethod") ?
      PROCESS_PRIORITY_FOREGROUND_KEYBOARD :
      PROCESS_PRIORITY_FOREGROUND;
  }

  if ((mHoldsCPUWakeLock || mHoldsHighPriorityWakeLock) &&
      IsExpectingSystemMessage()) {
    return PROCESS_PRIORITY_BACKGROUND_PERCEIVABLE;
  }

  AudioChannelService* service = AudioChannelService::GetOrCreateAudioChannelService();
  if (service->ProcessContentOrNormalChannelIsActive(ChildID())) {
    return PROCESS_PRIORITY_BACKGROUND_PERCEIVABLE;
  }

  return HasAppType("homescreen") ?
         PROCESS_PRIORITY_BACKGROUND_HOMESCREEN :
         PROCESS_PRIORITY_BACKGROUND;
}

void
ParticularProcessPriorityManager::SetPriorityNow(ProcessPriority aPriority,
                                                 uint32_t aLRU)
{
  if (aPriority == PROCESS_PRIORITY_UNKNOWN) {
    MOZ_ASSERT(false);
    return;
  }

  if (!ProcessPriorityManagerImpl::PrefsEnabled() ||
      !mContentParent ||
      mFrozen ||
      ((mPriority == aPriority) && (mLRU == aLRU))) {
    return;
  }

  if ((mPriority == aPriority) && (mLRU != aLRU)) {
    mLRU = aLRU;
    hal::SetProcessPriority(Pid(), mPriority, aLRU);

    nsPrintfCString processPriorityWithLRU("%s:%d",
      ProcessPriorityToString(mPriority), aLRU);

    FireTestOnlyObserverNotification("process-priority-with-LRU-set",
      processPriorityWithLRU.get());
    return;
  }

  LOGP("Changing priority from %s to %s.",
       ProcessPriorityToString(mPriority),
       ProcessPriorityToString(aPriority));

  ProcessPriority oldPriority = mPriority;

  if (oldPriority == PROCESS_PRIORITY_FOREGROUND &&
      aPriority < PROCESS_PRIORITY_FOREGROUND &&
      ProcessPriorityManagerImpl::GetSingleton()->
        NumberOfForegroundProcesses() == 1) {
    LOGP("Attempting to demote the last foreground process is delayed.");

    ProcessPriorityManagerImpl::GetSingleton()->
      ScheduleDelayedSetPriority(this, aPriority);

    FireTestOnlyObserverNotification("process-priority-delayed",
      ProcessPriorityToString(aPriority));
    return;
  }

  mPriority = aPriority;
  hal::SetProcessPriority(Pid(), mPriority);

  if (oldPriority != mPriority) {
    ProcessPriorityManagerImpl::GetSingleton()->
      NotifyProcessPriorityChanged(this, oldPriority);

    unused << mContentParent->SendNotifyProcessPriorityChanged(mPriority);
  }

  if (aPriority < PROCESS_PRIORITY_FOREGROUND) {
    unused << mContentParent->SendFlushMemory(NS_LITERAL_STRING("low-memory"));
  }

  FireTestOnlyObserverNotification("process-priority-set",
    ProcessPriorityToString(mPriority));

  if (aPriority >= PROCESS_PRIORITY_FOREGROUND) {
    LOGP("More than one foreground processes. Run delayed priority change");
    ProcessPriorityManagerImpl::GetSingleton()->
      PerformDelayedSetPriority(this);
  }
}

void
ParticularProcessPriorityManager::Freeze()
{
  mFrozen = true;
}

void
ParticularProcessPriorityManager::Unfreeze()
{
  mFrozen = false;
  ResetPriorityNow();
}

void
ParticularProcessPriorityManager::ShutDown()
{
  MOZ_ASSERT(mContentParent);

  UnregisterWakeLockObserver(this);

  if (mResetPriorityTimer) {
    mResetPriorityTimer->Cancel();
    mResetPriorityTimer = nullptr;
  }

  mContentParent = nullptr;
}

void
ProcessPriorityManagerImpl::FireTestOnlyObserverNotification(
  const char* aTopic,
  const nsACString& aData )
{
  if (!Preferences::GetBool("dom.ipc.processPriorityManager.testMode")) {
    return;
  }

  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  NS_ENSURE_TRUE_VOID(os);

  nsPrintfCString topic("process-priority-manager:TEST-ONLY:%s", aTopic);

  LOG("Notifying observer %s, data %s",
      topic.get(), PromiseFlatCString(aData).get());
  os->NotifyObservers(nullptr, topic.get(), NS_ConvertUTF8toUTF16(aData).get());
}

void
ParticularProcessPriorityManager::FireTestOnlyObserverNotification(
  const char* aTopic,
  const char* aData  )
{
  if (!Preferences::GetBool("dom.ipc.processPriorityManager.testMode")) {
    return;
  }

  nsAutoCString data;
  if (aData) {
    data.AppendASCII(aData);
  }

  FireTestOnlyObserverNotification(aTopic, data);
}

void
ParticularProcessPriorityManager::FireTestOnlyObserverNotification(
  const char* aTopic,
  const nsACString& aData )
{
  if (!Preferences::GetBool("dom.ipc.processPriorityManager.testMode")) {
    return;
  }

  nsAutoCString data(nsPrintfCString("%lld", ChildID()));
  if (!aData.IsEmpty()) {
    data.Append(':');
    data.Append(aData);
  }

  
  
  

  ProcessPriorityManagerImpl::GetSingleton()->
    FireTestOnlyObserverNotification(aTopic, data);
}

StaticRefPtr<ProcessPriorityManagerChild>
ProcessPriorityManagerChild::sSingleton;

 void
ProcessPriorityManagerChild::StaticInit()
{
  if (!sSingleton) {
    sSingleton = new ProcessPriorityManagerChild();
    sSingleton->Init();
    ClearOnShutdown(&sSingleton);
  }
}

 ProcessPriorityManagerChild*
ProcessPriorityManagerChild::Singleton()
{
  StaticInit();
  return sSingleton;
}

NS_IMPL_ISUPPORTS(ProcessPriorityManagerChild,
                  nsIObserver)

ProcessPriorityManagerChild::ProcessPriorityManagerChild()
{
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    mCachedPriority = PROCESS_PRIORITY_MASTER;
  } else {
    mCachedPriority = PROCESS_PRIORITY_UNKNOWN;
  }
}

void
ProcessPriorityManagerChild::Init()
{
  
  
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    nsCOMPtr<nsIObserverService> os = services::GetObserverService();
    NS_ENSURE_TRUE_VOID(os);
    os->AddObserver(this, "ipc:process-priority-changed",  false);
  }
}

NS_IMETHODIMP
ProcessPriorityManagerChild::Observe(
  nsISupports* aSubject,
  const char* aTopic,
  const char16_t* aData)
{
  MOZ_ASSERT(!strcmp(aTopic, "ipc:process-priority-changed"));

  nsCOMPtr<nsIPropertyBag2> props = do_QueryInterface(aSubject);
  NS_ENSURE_TRUE(props, NS_OK);

  int32_t priority = static_cast<int32_t>(PROCESS_PRIORITY_UNKNOWN);
  props->GetPropertyAsInt32(NS_LITERAL_STRING("priority"), &priority);
  NS_ENSURE_TRUE(ProcessPriority(priority) != PROCESS_PRIORITY_UNKNOWN, NS_OK);

  mCachedPriority = static_cast<ProcessPriority>(priority);

  return NS_OK;
}

bool
ProcessPriorityManagerChild::CurrentProcessIsForeground()
{
  return mCachedPriority == PROCESS_PRIORITY_UNKNOWN ||
         mCachedPriority >= PROCESS_PRIORITY_FOREGROUND;
}

bool
ProcessPriorityManagerChild::CurrentProcessIsHighPriority()
{
  return mCachedPriority == PROCESS_PRIORITY_UNKNOWN ||
         mCachedPriority >= PROCESS_PRIORITY_FOREGROUND_HIGH;
}

ProcessLRUPool::ProcessLRUPool(ProcessPriority aPriority, uint32_t aBias)
  : mPriority(aPriority)
  , mLRUPoolLevels(1)
  , mBias(aBias)
{
  
  
  const char* str = ProcessPriorityToString(aPriority);
  nsPrintfCString pref("dom.ipc.processPriorityManager.%s.LRUPoolLevels", str);

  Preferences::GetUint(pref.get(), &mLRUPoolLevels);

  
  
  
  
  
  
  MOZ_ASSERT(aPriority != PROCESS_PRIORITY_BACKGROUND || mLRUPoolLevels <= 6);
  MOZ_ASSERT(aPriority != PROCESS_PRIORITY_FOREGROUND || mLRUPoolLevels <= 4);

  
  mLRUPoolSize = (1 << mLRUPoolLevels) - 1;

  LOG("Making %s LRU pool with size(%d)", str, mLRUPoolSize);
}

uint32_t
ProcessLRUPool::CalculateLRULevel(uint32_t aLRU)
{
  
  
  

  
  
  
  
  
  
  

  

  int exp;
  unused << frexp(static_cast<double>(aLRU), &exp);
  uint32_t level = std::max(exp - 1, 0);

  return std::min(mLRUPoolLevels - 1, level);
}

void
ProcessLRUPool::Remove(ParticularProcessPriorityManager* aParticularManager)
{
  nsTArray<ParticularProcessPriorityManager*>::index_type index =
    mLRUPool.IndexOf(aParticularManager);

  if (index == nsTArray<ParticularProcessPriorityManager*>::NoIndex) {
    return;
  }

  mLRUPool.RemoveElementAt(index);
  AdjustLRUValues(index,  true);

  LOG("Remove ChildID(%" PRIu64 ") from %s LRU pool",
      static_cast<uint64_t>(aParticularManager->ChildID()),
      ProcessPriorityToString(mPriority));
}







void
ProcessLRUPool::AdjustLRUValues(
  nsTArray<ParticularProcessPriorityManager*>::index_type aStart,
  bool removed)
{
  uint32_t adj = (removed ? 1 : 0) + mBias;

  for (nsTArray<ParticularProcessPriorityManager*>::index_type i = aStart;
       i < mLRUPool.Length();
       i++) {
    



    if (((i + adj) & (i + adj - 1)) == 0) {
      mLRUPool[i]->SetPriorityNow(mPriority, CalculateLRULevel(i + mBias));
    }
  }
}

void
ProcessLRUPool::Add(ParticularProcessPriorityManager* aParticularManager)
{
  
  
  mLRUPool.InsertElementAt(0, aParticularManager);
  AdjustLRUValues(1,  false);

  LOG("Add ChildID(%" PRIu64 ") into %s LRU pool",
      static_cast<uint64_t>(aParticularManager->ChildID()),
      ProcessPriorityToString(mPriority));
}

} 

namespace mozilla {

 void
ProcessPriorityManager::Init()
{
  ProcessPriorityManagerImpl::StaticInit();
  ProcessPriorityManagerChild::StaticInit();
}

 void
ProcessPriorityManager::SetProcessPriority(ContentParent* aContentParent,
                                           ProcessPriority aPriority)
{
  MOZ_ASSERT(aContentParent);

  ProcessPriorityManagerImpl* singleton =
    ProcessPriorityManagerImpl::GetSingleton();
  if (singleton) {
    singleton->SetProcessPriority(aContentParent, aPriority);
  }
}

 bool
ProcessPriorityManager::CurrentProcessIsForeground()
{
  return ProcessPriorityManagerChild::Singleton()->
    CurrentProcessIsForeground();
}

 bool
ProcessPriorityManager::AnyProcessHasHighPriority()
{
  ProcessPriorityManagerImpl* singleton =
    ProcessPriorityManagerImpl::GetSingleton();

  if (singleton) {
    return singleton->ChildProcessHasHighPriority();
  } else {
    return ProcessPriorityManagerChild::Singleton()->
      CurrentProcessIsHighPriority();
  }
}

 void
ProcessPriorityManager::Freeze()
{
  ProcessPriorityManagerImpl* singleton =
    ProcessPriorityManagerImpl::GetSingleton();
  if (singleton) {
    singleton->Freeze();
  }
}

 void
ProcessPriorityManager::Unfreeze()
{
  ProcessPriorityManagerImpl* singleton =
    ProcessPriorityManagerImpl::GetSingleton();
  if (singleton) {
    singleton->Unfreeze();
  }
}

} 
