





#include "ProcessPriorityManager.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/Hal.h"
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

#ifdef XP_WIN
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#ifdef LOG
#undef LOG
#endif









#if defined(ANDROID) && defined(ENABLE_LOGGING)
#  include <android/log.h>
#  define LOG(fmt, ...) \
     __android_log_print(ANDROID_LOG_INFO, \
       "Gecko:ProcessPriorityManager", \
       fmt, ## __VA_ARGS__)
#  define LOGP(fmt, ...) \
    __android_log_print(ANDROID_LOG_INFO, \
      "Gecko:ProcessPriorityManager", \
      "[%schild-id=%llu, pid=%d] " fmt, \
      NameWithComma().get(), \
      (long long unsigned) ChildID(), Pid(), ## __VA_ARGS__)

#elif defined(ENABLE_LOGGING)
#  define LOG(fmt, ...) \
     printf("ProcessPriorityManager - " fmt "\n", ##__VA_ARGS__)
#  define LOGP(fmt, ...) \
     printf("ProcessPriorityManager[%schild-id=%llu, pid=%d] - " fmt "\n", \
       NameWithComma().get(), \
       (unsigned long long) ChildID(), Pid(), ##__VA_ARGS__)

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
            ("ProcessPriorityManager[%schild-id=%llu, pid=%d] - " fmt, \
            NameWithComma().get(), \
            (unsigned long long) ChildID(), Pid(), ##__VA_ARGS__))
#else
#define LOG(fmt, ...)
#define LOGP(fmt, ...)
#endif

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::hal;

namespace {

class ParticularProcessPriorityManager;











class ProcessPriorityManagerImpl MOZ_FINAL
  : public nsIObserver
{
public:
  



  static ProcessPriorityManagerImpl* GetSingleton();

  static void StaticInit();
  static bool PrefsEnabled();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  


  void SetProcessPriority(ContentParent* aContentParent,
                          ProcessPriority aPriority);

  



  void FireTestOnlyObserverNotification(const char* aTopic,
                                        const nsACString& aData = EmptyCString());

  



  bool OtherProcessHasHighPriority(
    ParticularProcessPriorityManager* aParticularManager);

  



  void NotifyProcessPriorityChanged(
    ParticularProcessPriorityManager* aParticularManager,
    hal::ProcessPriority aOldPriority);

private:
  static bool sPrefListenersRegistered;
  static bool sInitialized;
  static StaticRefPtr<ProcessPriorityManagerImpl> sSingleton;

  static int PrefChangedCallback(const char* aPref, void* aClosure);

  ProcessPriorityManagerImpl();
  ~ProcessPriorityManagerImpl() {}
  DISALLOW_EVIL_CONSTRUCTORS(ProcessPriorityManagerImpl);

  void Init();

  already_AddRefed<ParticularProcessPriorityManager>
  GetParticularProcessPriorityManager(ContentParent* aContentParent);

  void ObserveContentParentCreated(nsISupports* aContentParent);
  void ObserveContentParentDestroyed(nsISupports* aSubject);

  nsDataHashtable<nsUint64HashKey, nsRefPtr<ParticularProcessPriorityManager> >
    mParticularManagers;

  nsTHashtable<nsUint64HashKey> mHighPriorityChildIDs;
};





class ProcessPriorityManagerChild MOZ_FINAL
  : public nsIObserver
{
public:
  static void StaticInit();
  static ProcessPriorityManagerChild* Singleton();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  bool CurrentProcessIsForeground();

private:
  static StaticRefPtr<ProcessPriorityManagerChild> sSingleton;

  ProcessPriorityManagerChild();
  ~ProcessPriorityManagerChild() {}
  DISALLOW_EVIL_CONSTRUCTORS(ProcessPriorityManagerChild);

  void Init();

  hal::ProcessPriority mCachedPriority;
};





class ParticularProcessPriorityManager MOZ_FINAL
  : public WakeLockObserver
  , public nsIObserver
  , public nsITimerCallback
  , public nsSupportsWeakReference
{
public:
  ParticularProcessPriorityManager(ContentParent* aContentParent);
  ~ParticularProcessPriorityManager();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSITIMERCALLBACK

  virtual void Notify(const WakeLockInformation& aInfo) MOZ_OVERRIDE;
  void Init();

  int32_t Pid() const;
  uint64_t ChildID() const;

  








  const nsAutoCString& NameWithComma();

  bool HasAppType(const char* aAppType);
  bool IsExpectingSystemMessage();

  void OnAudioChannelProcessChanged(nsISupports* aSubject);
  void OnRemoteBrowserFrameShown(nsISupports* aSubject);
  void OnTabParentDestroyed(nsISupports* aSubject);
  void OnFrameloaderVisibleChanged(nsISupports* aSubject);
  void OnChannelConnected(nsISupports* aSubject);

  ProcessPriority CurrentPriority();
  ProcessPriority ComputePriority();
  ProcessCPUPriority ComputeCPUPriority();

  void ScheduleResetPriority(const char* aTimeoutPref);
  void ResetPriority();
  void ResetPriorityNow();
  void ResetCPUPriorityNow();

  



  void SetPriorityNow(ProcessPriority aPriority);

  void SetPriorityNow(ProcessPriority aPriority,
                      ProcessCPUPriority aCPUPriority);

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
  ProcessCPUPriority mCPUPriority;
  bool mHoldsCPUWakeLock;
  bool mHoldsHighPriorityWakeLock;

  


  nsAutoCString mNameWithComma;

  nsCOMPtr<nsITimer> mResetPriorityTimer;
};

 bool ProcessPriorityManagerImpl::sInitialized = false;
 bool ProcessPriorityManagerImpl::sPrefListenersRegistered = false;
 StaticRefPtr<ProcessPriorityManagerImpl>
  ProcessPriorityManagerImpl::sSingleton;

NS_IMPL_ISUPPORTS1(ProcessPriorityManagerImpl,
                   nsIObserver);

 int
ProcessPriorityManagerImpl::PrefChangedCallback(const char* aPref,
                                                void* aClosure)
{
  StaticInit();
  return 0;
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
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
}

void
ProcessPriorityManagerImpl::Init()
{
  LOG("Starting up.  This is the master process.");

  
  
  
  hal::SetProcessPriority(getpid(), PROCESS_PRIORITY_MASTER,
                          PROCESS_CPU_PRIORITY_NORMAL);

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
  const PRUnichar* aData)
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
  nsRefPtr<ParticularProcessPriorityManager> pppm;
  mParticularManagers.Get(aContentParent->ChildID(), &pppm);
  if (!pppm) {
    pppm = new ParticularProcessPriorityManager(aContentParent);
    pppm->Init();
    mParticularManagers.Put(aContentParent->ChildID(), pppm);

    FireTestOnlyObserverNotification("process-created",
      nsPrintfCString("%lld", aContentParent->ChildID()));
  }

  return pppm.forget();
}

void
ProcessPriorityManagerImpl::SetProcessPriority(ContentParent* aContentParent,
                                               ProcessPriority aPriority)
{
  MOZ_ASSERT(aContentParent);
  nsRefPtr<ParticularProcessPriorityManager> pppm =
    GetParticularProcessPriorityManager(aContentParent);
  pppm->SetPriorityNow(aPriority);
}

void
ProcessPriorityManagerImpl::ObserveContentParentCreated(
  nsISupports* aContentParent)
{
  
  
  nsCOMPtr<nsIObserver> cp = do_QueryInterface(aContentParent);
  nsRefPtr<ParticularProcessPriorityManager> pppm =
    GetParticularProcessPriorityManager(static_cast<ContentParent*>(cp.get()));
}

static PLDHashOperator
EnumerateParticularProcessPriorityManagers(
  const uint64_t& aKey,
  nsRefPtr<ParticularProcessPriorityManager> aValue,
  void* aUserData)
{
  nsTArray<nsRefPtr<ParticularProcessPriorityManager> >* aArray =
    static_cast<nsTArray<nsRefPtr<ParticularProcessPriorityManager> >*>(aUserData);
  aArray->AppendElement(aValue);
  return PL_DHASH_NEXT;
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
  MOZ_ASSERT(pppm);
  if (pppm) {
    pppm->ShutDown();
  }

  mParticularManagers.Remove(childID);

  if (mHighPriorityChildIDs.Contains(childID)) {
    mHighPriorityChildIDs.RemoveEntry(childID);

    
    nsTArray<nsRefPtr<ParticularProcessPriorityManager> > pppms;
    mParticularManagers.EnumerateRead(
      &EnumerateParticularProcessPriorityManagers,
      &pppms);

    for (uint32_t i = 0; i < pppms.Length(); i++) {
      pppms[i]->ResetCPUPriorityNow();
    }
  }
}

bool
ProcessPriorityManagerImpl::OtherProcessHasHighPriority(
  ParticularProcessPriorityManager* aParticularManager)
{
  if (mHighPriorityChildIDs.Contains(aParticularManager->ChildID())) {
    return mHighPriorityChildIDs.Count() > 1;
  }
  return mHighPriorityChildIDs.Count() > 0;
}

void
ProcessPriorityManagerImpl::NotifyProcessPriorityChanged(
  ParticularProcessPriorityManager* aParticularManager,
  ProcessPriority aOldPriority)
{
  
  

  if (aOldPriority < PROCESS_PRIORITY_FOREGROUND_HIGH &&
      aParticularManager->CurrentPriority() <
        PROCESS_PRIORITY_FOREGROUND_HIGH) {

    return;
  }

  if (aParticularManager->CurrentPriority() >=
      PROCESS_PRIORITY_FOREGROUND_HIGH) {
    mHighPriorityChildIDs.PutEntry(aParticularManager->ChildID());
  } else {
    mHighPriorityChildIDs.RemoveEntry(aParticularManager->ChildID());
  }

  nsTArray<nsRefPtr<ParticularProcessPriorityManager> > pppms;
  mParticularManagers.EnumerateRead(
    &EnumerateParticularProcessPriorityManagers,
    &pppms);

  for (uint32_t i = 0; i < pppms.Length(); i++) {
    if (pppms[i] != aParticularManager) {
      pppms[i]->ResetCPUPriorityNow();
    }
  }
}

NS_IMPL_ISUPPORTS3(ParticularProcessPriorityManager,
                   nsIObserver,
                   nsITimerCallback,
                   nsISupportsWeakReference);

ParticularProcessPriorityManager::ParticularProcessPriorityManager(
  ContentParent* aContentParent)
  : mContentParent(aContentParent)
  , mChildID(aContentParent->ChildID())
  , mPriority(PROCESS_PRIORITY_UNKNOWN)
  , mCPUPriority(PROCESS_CPU_PRIORITY_NORMAL)
  , mHoldsCPUWakeLock(false)
  , mHoldsHighPriorityWakeLock(false)
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
    os->AddObserver(this, "remote-browser-frame-shown",  true);
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
                                          const PRUnichar* aData)
{
  if (!mContentParent) {
    
    return NS_OK;
  }

  nsDependentCString topic(aTopic);

  if (topic.EqualsLiteral("audio-channel-process-changed")) {
    OnAudioChannelProcessChanged(aSubject);
  } else if (topic.EqualsLiteral("remote-browser-frame-shown")) {
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

  nsCOMPtr<nsITabParent> tp;
  fl->GetTabParent(getter_AddRefs(tp));
  NS_ENSURE_TRUE_VOID(tp);

  if (static_cast<TabParent*>(tp.get())->Manager() != mContentParent) {
    return;
  }

  ResetPriority();
}

void
ParticularProcessPriorityManager::OnTabParentDestroyed(nsISupports* aSubject)
{
  nsCOMPtr<nsITabParent> tp = do_QueryInterface(aSubject);
  NS_ENSURE_TRUE_VOID(tp);

  if (static_cast<TabParent*>(tp.get())->Manager() != mContentParent) {
    return;
  }

  ResetPriority();
}

void
ParticularProcessPriorityManager::OnFrameloaderVisibleChanged(nsISupports* aSubject)
{
  nsCOMPtr<nsIFrameLoader> fl = do_QueryInterface(aSubject);
  NS_ENSURE_TRUE_VOID(fl);

  nsCOMPtr<nsITabParent> tp;
  fl->GetTabParent(getter_AddRefs(tp));
  if (!tp) {
    return;
  }

  if (static_cast<TabParent*>(tp.get())->Manager() != mContentParent) {
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
    ScheduleResetPriority("backgroundGracePeriodMS");
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
    static_cast<TabParent*>(browsers[i])->GetAppType(appType);
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
    TabParent* tp = static_cast<TabParent*>(browsers[i]);
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
    if (static_cast<TabParent*>(browsers[i])->IsVisible()) {
      isVisible = true;
      break;
    }
  }

  if (isVisible) {
    return HasAppType("keyboard") ?
      PROCESS_PRIORITY_FOREGROUND_KEYBOARD :
      PROCESS_PRIORITY_FOREGROUND;
  }

  if ((mHoldsCPUWakeLock || mHoldsHighPriorityWakeLock) &&
      IsExpectingSystemMessage()) {
    return PROCESS_PRIORITY_BACKGROUND_PERCEIVABLE;
  }

  AudioChannelService* service = AudioChannelService::GetAudioChannelService();
  if (service->ProcessContentOrNormalChannelIsActive(ChildID())) {
    return PROCESS_PRIORITY_BACKGROUND_PERCEIVABLE;
  }

  return HasAppType("homescreen") ?
         PROCESS_PRIORITY_BACKGROUND_HOMESCREEN :
         PROCESS_PRIORITY_BACKGROUND;
}

ProcessCPUPriority
ParticularProcessPriorityManager::ComputeCPUPriority()
{
  if (mPriority >= PROCESS_PRIORITY_FOREGROUND_HIGH) {
    return PROCESS_CPU_PRIORITY_NORMAL;
  }

  return ProcessPriorityManagerImpl::GetSingleton()->
    OtherProcessHasHighPriority(this) ?
    PROCESS_CPU_PRIORITY_LOW :
    PROCESS_CPU_PRIORITY_NORMAL;
}

void
ParticularProcessPriorityManager::ResetCPUPriorityNow()
{
  SetPriorityNow(mPriority);
}

void
ParticularProcessPriorityManager::SetPriorityNow(ProcessPriority aPriority)
{
  SetPriorityNow(aPriority, ComputeCPUPriority());
}

void
ParticularProcessPriorityManager::SetPriorityNow(ProcessPriority aPriority,
                                                 ProcessCPUPriority aCPUPriority)
{
  if (aPriority == PROCESS_PRIORITY_UNKNOWN) {
    MOZ_ASSERT(false);
    return;
  }

  if (!mContentParent ||
      !ProcessPriorityManagerImpl::PrefsEnabled() ||
      (mPriority == aPriority && mCPUPriority == aCPUPriority)) {
    return;
  }

  
  
  
  
  if (!ProcessPriorityManagerImpl::PrefsEnabled()) {
    return;
  }

  LOGP("Changing priority from %s to %s.",
       ProcessPriorityToString(mPriority, mCPUPriority),
       ProcessPriorityToString(aPriority, aCPUPriority));

  ProcessPriority oldPriority = mPriority;

  mPriority = aPriority;
  mCPUPriority = aCPUPriority;
  hal::SetProcessPriority(Pid(), mPriority, mCPUPriority);

  if (oldPriority != mPriority) {
    unused << mContentParent->SendNotifyProcessPriorityChanged(mPriority);
  }

  if (aPriority >= PROCESS_PRIORITY_FOREGROUND) {
    unused << mContentParent->SendCancelMinimizeMemoryUsage();
  } else {
    unused << mContentParent->SendMinimizeMemoryUsage();
  }

  FireTestOnlyObserverNotification("process-priority-set",
    ProcessPriorityToString(mPriority, mCPUPriority));

  if (oldPriority != mPriority) {
    ProcessPriorityManagerImpl::GetSingleton()->
      NotifyProcessPriorityChanged(this, oldPriority);
  }
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
    data.AppendLiteral(":");
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

NS_IMPL_ISUPPORTS1(ProcessPriorityManagerChild,
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
  const PRUnichar* aData)
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

} 
