





#include "mozilla/ClearOnShutdown.h"
#include "mozilla/dom/ipc/ProcessPriorityManager.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/Hal.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/HalTypes.h"
#include "mozilla/TimeStamp.h"
#include "prlog.h"
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



















class ProcessPriorityManager MOZ_FINAL
  : public nsIObserver
  , public nsIDOMEventListener
{
public:
  ProcessPriorityManager();
  void Init();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIDOMEVENTLISTENER

  ProcessPriority GetPriority() const { return mProcessPriority; }

private:
  void SetPriority(ProcessPriority aPriority);
  void OnContentDocumentGlobalCreated(nsISupports* aOuterWindow);
  void OnInnerWindowDestroyed();
  void OnGracePeriodTimerFired();
  void RecomputeNumVisibleWindows();

  
  
  
  
  ProcessPriority mProcessPriority;

  nsTArray<nsWeakPtr> mWindows;
  nsCOMPtr<nsITimer> mGracePeriodTimer;
  nsWeakPtr mMemoryMinimizerRunnable;
  TimeStamp mStartupTime;
};

NS_IMPL_ISUPPORTS2(ProcessPriorityManager, nsIObserver, nsIDOMEventListener);

ProcessPriorityManager::ProcessPriorityManager()
  : mProcessPriority(PROCESS_PRIORITY_FOREGROUND)
  , mStartupTime(TimeStamp::Now())
{
}

void
ProcessPriorityManager::Init()
{
  LOG("Starting up.");

  
  
  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  os->AddObserver(this, "content-document-global-created",  false);
  os->AddObserver(this, "inner-window-destroyed",  false);

  SetPriority(PROCESS_PRIORITY_FOREGROUND);
}

NS_IMETHODIMP
ProcessPriorityManager::Observe(
  nsISupports* aSubject,
  const char* aTopic,
  const PRUnichar* aData)
{
  if (!strcmp(aTopic, "content-document-global-created")) {
    OnContentDocumentGlobalCreated(aSubject);
  } else if (!strcmp(aTopic, "inner-window-destroyed")) {
    OnInnerWindowDestroyed();
  } else if (!strcmp(aTopic, "timer-callback")) {
    OnGracePeriodTimerFired();
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
  RecomputeNumVisibleWindows();
  return NS_OK;
}

void
ProcessPriorityManager::OnContentDocumentGlobalCreated(
  nsISupports* aOuterWindow)
{
  
  
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
  RecomputeNumVisibleWindows();
}

void
ProcessPriorityManager::OnInnerWindowDestroyed()
{
  RecomputeNumVisibleWindows();
}

void
ProcessPriorityManager::RecomputeNumVisibleWindows()
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

  SetPriority(allHidden ?
              GetBackgroundPriority() :
              PROCESS_PRIORITY_FOREGROUND);
}

void
ProcessPriorityManager::SetPriority(ProcessPriority aPriority)
{
  if (aPriority == mProcessPriority) {
    return;
  }

  if (aPriority == PROCESS_PRIORITY_BACKGROUND ||
      aPriority == PROCESS_PRIORITY_BACKGROUND_HOMESCREEN) {
    
    
    uint32_t gracePeriodMS = Preferences::GetUint("dom.ipc.processPriorityManager.gracePeriodMS", 1000);
    if (mGracePeriodTimer) {
      LOG("Grace period timer already active.");
      return;
    }

    LOG("Initializing grace period timer.");
    mProcessPriority = aPriority;
    mGracePeriodTimer = do_CreateInstance("@mozilla.org/timer;1");
    mGracePeriodTimer->Init(this, gracePeriodMS, nsITimer::TYPE_ONE_SHOT);

  } else if (aPriority == PROCESS_PRIORITY_FOREGROUND) {
    
    
    if (mGracePeriodTimer) {
      mGracePeriodTimer->Cancel();
      mGracePeriodTimer = nullptr;
    }

    
    nsCOMPtr<nsICancelableRunnable> runnable =
      do_QueryReferent(mMemoryMinimizerRunnable);
    if (runnable) {
      runnable->Cancel();
    }

    LOG("Setting priority to %d.", aPriority);
    mProcessPriority = aPriority;
    hal::SetProcessPriority(getpid(), aPriority);

  } else {
    MOZ_ASSERT(false);
  }
}

void
ProcessPriorityManager::OnGracePeriodTimerFired()
{
  LOG("Grace period timer fired; setting priority to %d.",
      mProcessPriority);

  
  
  
  MOZ_ASSERT(mProcessPriority == PROCESS_PRIORITY_BACKGROUND ||
             mProcessPriority == PROCESS_PRIORITY_BACKGROUND_HOMESCREEN);

  mGracePeriodTimer = nullptr;
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

} 

void
InitProcessPriorityManager()
{
  if (sInitialized) {
    return;
  }

  
  if (!Preferences::GetBool("dom.ipc.processPriorityManager.enabled") ||
      Preferences::GetBool("dom.ipc.tabs.disabled")) {
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
  return sManager->GetPriority() >= PROCESS_PRIORITY_FOREGROUND;
}

} 
} 
} 
