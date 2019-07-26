





#include "mozilla/ClearOnShutdown.h"
#include "mozilla/dom/ipc/ProcessPriorityManager.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/Hal.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/HalTypes.h"
#include "mozilla/TimeStamp.h"
#include "AudioChannelService.h"
#include "prlog.h"
#include "nsPrintfCString.h"
#include "nsWeakPtr.h"
#include "nsXULAppAPI.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsITimer.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIDocument.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMWindow.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMDocument.h"
#include "nsPIDOMWindow.h"
#include "StaticPtr.h"

#ifdef XP_WIN
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

using namespace mozilla::hal;

namespace mozilla {
namespace dom {
namespace ipc {

namespace {
static bool sInitialized = false;
class ProcessPriorityManager;
static StaticRefPtr<ProcessPriorityManager> sManager;


#ifdef LOG
#undef LOG
#endif








#if defined(ANDROID) && 0
#include <android/log.h>
#define LOG(fmt, ...) \
  __android_log_print(ANDROID_LOG_INFO, \
      "Gecko:ProcessPriorityManager", \
      fmt, ## __VA_ARGS__)
#elif defined(PR_LOGGING)
static PRLogModuleInfo*
GetPPMLog()
{
  static PRLogModuleInfo *sLog;
  if (!sLog)
    sLog = PR_NewLogModule("ProcessPriorityManager");
  return sLog;
}
#define LOG(fmt, ...) \
  PR_LOG(GetPPMLog(), PR_LOG_DEBUG,                                     \
         ("[%d] ProcessPriorityManager - " fmt, getpid(), ##__VA_ARGS__))
#else
#define LOG(fmt, ...)
#endif

uint64_t
GetContentChildID()
{
  ContentChild* contentChild = ContentChild::GetSingleton();
  if (!contentChild) {
    return 0;
  }

  return contentChild->GetID();
}




bool
IsBackgroundPriority(ProcessPriority aPriority)
{
  return (aPriority == PROCESS_PRIORITY_BACKGROUND ||
          aPriority == PROCESS_PRIORITY_BACKGROUND_HOMESCREEN ||
          aPriority == PROCESS_PRIORITY_BACKGROUND_PERCEIVABLE);
}





























class ProcessPriorityManager MOZ_FINAL
  : public nsIObserver
  , public nsIDOMEventListener
  , public nsITimerCallback
  , public WakeLockObserver
{
public:
  ProcessPriorityManager();
  void Init();

  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIDOMEVENTLISTENER
  void Notify(const WakeLockInformation& aWakeLockInfo);

  ProcessPriority GetPriority() const { return mProcessPriority; }

  



  void TemporarilyLockProcessPriority();

  













  void ResetPriority();

  


  void ResetPriorityNow();

private:
  void OnContentDocumentGlobalCreated(nsISupports* aOuterWindow);

  



  bool IsCriticalProcessWithWakeLock();

  


  ProcessPriority GetForegroundPriority();

  


  ProcessPriority GetBackgroundPriority();

  


  bool ComputeIsInForeground();

  



  void SetIsForeground();

  



  void SetIsBackgroundNow();

  




  void
  ScheduleResetPriority(const char* aTimeoutPref);

  
  bool mHoldsCPUWakeLock;

  
  bool mHoldsHighPriorityWakeLock;

  
  ProcessPriority mProcessPriority;

  nsTArray<nsWeakPtr> mWindows;

  
  
  nsCOMPtr<nsITimer> mResetPriorityTimer;

  nsWeakPtr mMemoryMinimizerRunnable;
};

NS_IMPL_ISUPPORTS2(ProcessPriorityManager, nsIObserver, nsIDOMEventListener)

ProcessPriorityManager::ProcessPriorityManager()
  : mHoldsCPUWakeLock(false)
  , mHoldsHighPriorityWakeLock(false)
  , mProcessPriority(ProcessPriority(-1))
{
  
  
  
  
  
  
  
}

void
ProcessPriorityManager::Init()
{
  LOG("Starting up.");

  
  
  
  
  
  
  
  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  os->AddObserver(this, "content-document-global-created",  false);
  os->AddObserver(this, "inner-window-destroyed",  false);
  os->AddObserver(this, "audio-channel-agent-changed",  false);
  os->AddObserver(this, "process-priority:reset-now",  false);

  RegisterWakeLockObserver(this);

  
  
  WakeLockInformation info1, info2;
  GetWakeLockInfo(NS_LITERAL_STRING("cpu"), &info1);
  mHoldsCPUWakeLock = info1.lockingProcesses().Contains(GetContentChildID());

  GetWakeLockInfo(NS_LITERAL_STRING("high-priority"), &info2);
  mHoldsHighPriorityWakeLock = info2.lockingProcesses().Contains(GetContentChildID());

  LOG("Done starting up.  mHoldsCPUWakeLock=%d, mHoldsHighPriorityWakeLock=%d",
      mHoldsCPUWakeLock, mHoldsHighPriorityWakeLock);
}

NS_IMETHODIMP
ProcessPriorityManager::Observe(
  nsISupports* aSubject,
  const char* aTopic,
  const PRUnichar* aData)
{
  if (!strcmp(aTopic, "content-document-global-created")) {
    OnContentDocumentGlobalCreated(aSubject);
  } else if (!strcmp(aTopic, "inner-window-destroyed") ||
             !strcmp(aTopic, "audio-channel-agent-changed")) {
    ResetPriority();
  } else if (!strcmp(aTopic, "process-priority:reset-now")) {
    LOG("Got process-priority:reset-now notification.");
    ResetPriorityNow();
  } else {
    MOZ_ASSERT(false);
  }
  return NS_OK;
}

void
ProcessPriorityManager::Notify(const WakeLockInformation& aInfo)
{
  bool* dest = nullptr;
  if (aInfo.topic() == NS_LITERAL_STRING("cpu")) {
    dest = &mHoldsCPUWakeLock;
  } else if (aInfo.topic() == NS_LITERAL_STRING("high-priority")) {
    dest = &mHoldsHighPriorityWakeLock;
  }

  if (dest) {
    bool thisProcessLocks =
      aInfo.lockingProcesses().Contains(GetContentChildID());

    if (thisProcessLocks != *dest) {
      *dest = thisProcessLocks;
      LOG("Got wake lock changed event. "
          "Now mHoldsCPUWakeLock=%d, mHoldsHighPriorityWakeLock=%d",
          mHoldsCPUWakeLock, mHoldsHighPriorityWakeLock);
      ResetPriority();
    }
  }
}

NS_IMETHODIMP
ProcessPriorityManager::HandleEvent(
  nsIDOMEvent* aEvent)
{
  LOG("Got visibilitychange.");
  ResetPriority();
  return NS_OK;
}

void
ProcessPriorityManager::OnContentDocumentGlobalCreated(
  nsISupports* aOuterWindow)
{
  LOG("DocumentGlobalCreated");
  
  
  nsCOMPtr<nsPIDOMWindow> outerWindow = do_QueryInterface(aOuterWindow);
  NS_ENSURE_TRUE_VOID(outerWindow);
  nsCOMPtr<nsPIDOMWindow> innerWindow = outerWindow->GetCurrentInnerWindow();
  NS_ENSURE_TRUE_VOID(innerWindow);

  
  nsCOMPtr<nsIDOMWindow> parentOuterWindow;
  innerWindow->GetScriptableParent(getter_AddRefs(parentOuterWindow));
  NS_ENSURE_TRUE_VOID(parentOuterWindow);
  if (parentOuterWindow != outerWindow) {
    return;
  }

  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(innerWindow);
  NS_ENSURE_TRUE_VOID(target);

  nsWeakPtr weakWin = do_GetWeakReference(innerWindow);
  NS_ENSURE_TRUE_VOID(weakWin);

  if (mWindows.Contains(weakWin)) {
    return;
  }

  target->AddSystemEventListener(NS_LITERAL_STRING("visibilitychange"),
                                 this,
                                  false,
                                  false);

  mWindows.AppendElement(weakWin);

  ResetPriority();
}

bool
ProcessPriorityManager::IsCriticalProcessWithWakeLock()
{
  if (!(mHoldsCPUWakeLock || mHoldsHighPriorityWakeLock)) {
    return false;
  }

  ContentChild* contentChild = ContentChild::GetSingleton();
  if (!contentChild) {
    return false;
  }

  const InfallibleTArray<PBrowserChild*>& browsers =
    contentChild->ManagedPBrowserChild();
  for (uint32_t i = 0; i < browsers.Length(); i++) {
    nsAutoString appType;
    static_cast<TabChild*>(browsers[i])->GetAppType(appType);
    if (appType.EqualsLiteral("critical")) {
      return true;
    }
  }

  return false;
}

ProcessPriority
ProcessPriorityManager::GetForegroundPriority()
{
  return IsCriticalProcessWithWakeLock() ? PROCESS_PRIORITY_FOREGROUND_HIGH :
                                           PROCESS_PRIORITY_FOREGROUND;
}




ProcessPriority
ProcessPriorityManager::GetBackgroundPriority()
{
  AudioChannelService* service = AudioChannelService::GetAudioChannelService();
  if (service->ContentOrNormalChannelIsActive()) {
    return PROCESS_PRIORITY_BACKGROUND_PERCEIVABLE;
  }

  bool isHomescreen = false;

  ContentChild* contentChild = ContentChild::GetSingleton();
  if (contentChild) {
    const InfallibleTArray<PBrowserChild*>& browsers =
      contentChild->ManagedPBrowserChild();
    for (uint32_t i = 0; i < browsers.Length(); i++) {
      nsAutoString appType;
      static_cast<TabChild*>(browsers[i])->GetAppType(appType);
      if (appType.EqualsLiteral("homescreen")) {
        isHomescreen = true;
        break;
      }
    }
  }

  return isHomescreen ?
         PROCESS_PRIORITY_BACKGROUND_HOMESCREEN :
         PROCESS_PRIORITY_BACKGROUND;
}


void
ProcessPriorityManager::ResetPriority()
{
  if (ComputeIsInForeground()) {
    SetIsForeground();
  } else if (IsBackgroundPriority(mProcessPriority)) {
    
    
    SetIsBackgroundNow();
  } else {
    ScheduleResetPriority("backgroundGracePeriodMS");
  }
}

void
ProcessPriorityManager::ResetPriorityNow()
{
  if (ComputeIsInForeground()) {
    SetIsForeground();
  } else {
    SetIsBackgroundNow();
  }
}

bool
ProcessPriorityManager::ComputeIsInForeground()
{
  
  
  if (IsCriticalProcessWithWakeLock()) {
    return true;
  }

  
  
  
  
  
  

  bool allHidden = true;
  for (uint32_t i = 0; i < mWindows.Length(); i++) {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mWindows[i]);
    if (!window) {
      mWindows.RemoveElementAt(i);
      i--;
      continue;
    }

    nsCOMPtr<nsIDocShell> docshell = do_GetInterface(window);
    if (!docshell) {
      continue;
    }

    bool isActive = false;
    docshell->GetIsActive(&isActive);


#ifdef DEBUG
    nsAutoCString spec;
    nsCOMPtr<nsIURI> uri = window->GetDocumentURI();
    if (uri) {
      uri->GetSpec(spec);
    }
    LOG("Docshell at %s has visibility %d.", spec.get(), isActive);
#endif

    allHidden = allHidden && !isActive;

    
    
    
  }

  return !allHidden;
}

void
ProcessPriorityManager::SetIsForeground()
{
  ProcessPriority foregroundPriority = GetForegroundPriority();
  if (foregroundPriority == mProcessPriority) {
    return;
  }

  
  nsCOMPtr<nsICancelableRunnable> runnable =
    do_QueryReferent(mMemoryMinimizerRunnable);
  if (runnable) {
    runnable->Cancel();
  }

  mProcessPriority = foregroundPriority;
  LOG("Setting priority to %s.", ProcessPriorityToString(mProcessPriority));
  hal::SetProcessPriority(getpid(), mProcessPriority);
}

void
ProcessPriorityManager::SetIsBackgroundNow()
{
  ProcessPriority backgroundPriority = GetBackgroundPriority();
  if (mProcessPriority == backgroundPriority) {
    return;
  }

  mProcessPriority = backgroundPriority;
  LOG("Setting priority to %s", ProcessPriorityToString(mProcessPriority));
  hal::SetProcessPriority(getpid(), mProcessPriority);

  
  nsCOMPtr<nsIMemoryReporterManager> mgr =
    do_GetService("@mozilla.org/memory-reporter-manager;1");
  if (mgr) {
    nsCOMPtr<nsICancelableRunnable> runnable =
      do_QueryReferent(mMemoryMinimizerRunnable);

    
    if (runnable) {
      runnable->Cancel();
    }

    mgr->MinimizeMemoryUsage( nullptr,
                             getter_AddRefs(runnable));
    mMemoryMinimizerRunnable = do_GetWeakReference(runnable);
  }
}

void
ProcessPriorityManager::ScheduleResetPriority(const char* aTimeoutPref)
{
  if (mResetPriorityTimer) {
    LOG("ScheduleResetPriority bailing; the timer is already running.");
    return;
  }

  uint32_t timeout = Preferences::GetUint(
    nsPrintfCString("dom.ipc.processPriorityManager.%s", aTimeoutPref).get());
  LOG("Scheduling reset timer to fire in %dms.", timeout);
  mResetPriorityTimer = do_CreateInstance("@mozilla.org/timer;1");
  mResetPriorityTimer->InitWithCallback(this, timeout, nsITimer::TYPE_ONE_SHOT);
}

NS_IMETHODIMP
ProcessPriorityManager::Notify(nsITimer* aTimer)
{
  LOG("Reset priority timer callback; about to ResetPriorityNow.");
  ResetPriorityNow();
  mResetPriorityTimer = nullptr;
  return NS_OK;
}

void
ProcessPriorityManager::TemporarilyLockProcessPriority()
{
  LOG("TemporarilyLockProcessPriority");

  
  
  
  
  
  if (mResetPriorityTimer) {
    mResetPriorityTimer->Cancel();
    mResetPriorityTimer = nullptr;
  }
  ScheduleResetPriority("temporaryPriorityLockMS");
}

} 

void
InitProcessPriorityManager()
{
  if (sInitialized) {
    return;
  }

  
  if (!Preferences::GetBool("dom.ipc.processPriorityManager.enabled") ||
      Preferences::GetBool("dom.ipc.tabs.disabled")) {
    LOG("InitProcessPriorityManager bailing due to prefs.");
    return;
  }

  sInitialized = true;

  
  
  
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    LOG("This is the master process.");
    hal::SetProcessPriority(getpid(), PROCESS_PRIORITY_MASTER);
    return;
  }

  sManager = new ProcessPriorityManager();
  sManager->Init();
  ClearOnShutdown(&sManager);
}

bool
CurrentProcessIsForeground()
{
  
  
  if (!sManager) {
    return true;
  }

  return sManager->GetPriority() >= PROCESS_PRIORITY_FOREGROUND;
}

void
TemporarilyLockProcessPriority()
{
  if (sManager) {
    sManager->TemporarilyLockProcessPriority();
  } else {
    LOG("TemporarilyLockProcessPriority called before "
        "InitProcessPriorityManager.  Bailing.");
  }
}

} 
} 
} 
