





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







#ifdef PR_LOGGING
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




ProcessPriority
GetBackgroundPriority()
{
  AudioChannelService* service = AudioChannelService::GetAudioChannelService();
  if (service->ContentChannelIsActive()) {
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
{
public:
  ProcessPriorityManager();
  void Init();

  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIDOMEVENTLISTENER

  ProcessPriority GetPriority() const { return mProcessPriority; }

  



  void TemporarilySetIsForeground();

  













  void ResetPriority();

  


  void ResetPriorityNow();

private:
  void OnContentDocumentGlobalCreated(nsISupports* aOuterWindow);

  


  bool ComputeIsInForeground();

  


  void SetIsForeground();

  



  void SetIsBackgroundNow();

  




  void
  ScheduleResetPriority(const char* aTimeoutPref);

  
  ProcessPriority mProcessPriority;

  nsTArray<nsWeakPtr> mWindows;

  
  
  nsCOMPtr<nsITimer> mResetPriorityTimer;

  nsWeakPtr mMemoryMinimizerRunnable;
};

NS_IMPL_ISUPPORTS2(ProcessPriorityManager, nsIObserver, nsIDOMEventListener)

ProcessPriorityManager::ProcessPriorityManager()
  : mProcessPriority(ProcessPriority(-1))
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
  } else {
    MOZ_ASSERT(false);
  }
  return NS_OK;
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
  
  
  
  
  
  

  bool allHidden = true;
  for (uint32_t i = 0; i < mWindows.Length(); i++) {
    nsCOMPtr<nsIDOMWindow> window = do_QueryReferent(mWindows[i]);
    if (!window) {
      mWindows.RemoveElementAt(i);
      i--;
      continue;
    }

    nsCOMPtr<nsIDOMDocument> doc;
    window->GetDocument(getter_AddRefs(doc));
    if (!doc) {
      continue;
    }

    bool hidden = false;
    doc->GetHidden(&hidden);
#ifdef DEBUG
    nsAutoString spec;
    doc->GetDocumentURI(spec);
    LOG("Document at %s has visibility %d.", NS_ConvertUTF16toUTF8(spec).get(), !hidden);
#endif

    allHidden = allHidden && hidden;

    
    
    
  }

  return !allHidden;
}

void
ProcessPriorityManager::SetIsForeground()
{
  if (mProcessPriority == PROCESS_PRIORITY_FOREGROUND) {
    return;
  }

  
  nsCOMPtr<nsICancelableRunnable> runnable =
    do_QueryReferent(mMemoryMinimizerRunnable);
  if (runnable) {
    runnable->Cancel();
  }

  LOG("Setting priority to FOREGROUND.");
  mProcessPriority = PROCESS_PRIORITY_FOREGROUND;
  hal::SetProcessPriority(getpid(), PROCESS_PRIORITY_FOREGROUND);
}

void
ProcessPriorityManager::SetIsBackgroundNow()
{
  ProcessPriority backgroundPriority = GetBackgroundPriority();
  if (mProcessPriority == backgroundPriority) {
    return;
  }

  mProcessPriority = backgroundPriority;
  LOG("Setting priority to BACKGROUND (type %d)", mProcessPriority);
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
ProcessPriorityManager::TemporarilySetIsForeground()
{
  LOG("TemporarilySetIsForeground");
  SetIsForeground();

  
  
  
  if (mResetPriorityTimer) {
    mResetPriorityTimer->Cancel();
    mResetPriorityTimer = nullptr;
  }
  ScheduleResetPriority("temporaryPriorityMS");
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
TemporarilySetProcessPriorityToForeground()
{
  if (sManager) {
    sManager->TemporarilySetIsForeground();
  } else {
    LOG("TemporarilySetProcessPriorityToForeground called before "
        "InitProcessPriorityManager.  Bailing.");
  }
}

} 
} 
} 
