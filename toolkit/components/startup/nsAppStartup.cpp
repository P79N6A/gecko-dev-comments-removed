




#include "nsAppStartup.h"

#include "nsIAppShellService.h"
#include "nsPIDOMWindow.h"
#include "nsIInterfaceRequestor.h"
#include "nsIFile.h"
#include "nsIObserverService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIPromptService.h"
#include "nsIStringBundle.h"
#include "nsISupportsPrimitives.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWindowMediator.h"
#include "nsIWindowWatcher.h"
#include "nsIXULRuntime.h"
#include "nsIXULWindow.h"
#include "nsNativeCharsetUtils.h"
#include "nsThreadUtils.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "mozilla/Preferences.h"
#include "GeckoProfiler.h"

#include "prprf.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsWidgetsCID.h"
#include "nsAppShellCID.h"
#include "nsXPCOMCIDInternal.h"
#include "mozilla/Services.h"
#include "nsIXPConnect.h"
#include "jsapi.h"
#include "prenv.h"
#include "nsAppDirectoryServiceDefs.h"

#if defined(XP_WIN)

#undef GetStartupInfo
#endif

#include "mozilla/IOInterposer.h"
#include "mozilla/Telemetry.h"
#include "mozilla/StartupTimeline.h"

static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

#define kPrefLastSuccess "toolkit.startup.last_success"
#define kPrefMaxResumedCrashes "toolkit.startup.max_resumed_crashes"
#define kPrefRecentCrashes "toolkit.startup.recent_crashes"
#define kPrefAlwaysUseSafeMode "toolkit.startup.always_use_safe_mode"

#if defined(XP_WIN)
#include "mozilla/perfprobe.h"








#define NS_APPLICATION_TRACING_CID \
  { 0x509962E0, 0x406B, 0x46F4, \
  { 0x99, 0xBA, 0x5A, 0x00, 0x9F, 0x8D, 0x22, 0x25} }



#define NS_PLACES_INIT_COMPLETE_EVENT_CID \
  { 0xA3DA04E0, 0x57D7, 0x482A, \
  { 0xA1, 0xC1, 0x61, 0xDA, 0x5F, 0x95, 0xBA, 0xCB} }

#define NS_SESSION_STORE_WINDOW_RESTORED_EVENT_CID \
  { 0x917B96B1, 0xECAD, 0x4DAB, \
  { 0xA7, 0x60, 0x8D, 0x49, 0x02, 0x77, 0x48, 0xAE} }

#define NS_XPCOM_SHUTDOWN_EVENT_CID \
  { 0x26D1E091, 0x0AE7, 0x4F49, \
  { 0xA5, 0x54, 0x42, 0x14, 0x44, 0x5C, 0x50, 0x5C} }

static NS_DEFINE_CID(kApplicationTracingCID,
  NS_APPLICATION_TRACING_CID);
static NS_DEFINE_CID(kPlacesInitCompleteCID,
  NS_PLACES_INIT_COMPLETE_EVENT_CID);
static NS_DEFINE_CID(kSessionStoreWindowRestoredCID,
  NS_SESSION_STORE_WINDOW_RESTORED_EVENT_CID);
static NS_DEFINE_CID(kXPCOMShutdownCID,
  NS_XPCOM_SHUTDOWN_EVENT_CID);  
#endif 

using namespace mozilla;

uint32_t gRestartMode = 0;

class nsAppExitEvent : public nsRunnable {
private:
  nsRefPtr<nsAppStartup> mService;

public:
  explicit nsAppExitEvent(nsAppStartup *service) : mService(service) {}

  NS_IMETHOD Run() {
    
    mService->mAppShell->Exit();

    mService->mRunning = false;
    return NS_OK;
  }
};










uint64_t ComputeAbsoluteTimestamp(PRTime prnow, TimeStamp now, TimeStamp stamp)
{
  static PRTime sAbsoluteNow = PR_Now();
  static TimeStamp sMonotonicNow = TimeStamp::Now();

  return sAbsoluteNow - (sMonotonicNow - stamp).ToMicroseconds();
}





nsAppStartup::nsAppStartup() :
  mConsiderQuitStopper(0),
  mRunning(false),
  mShuttingDown(false),
  mStartingUp(true),
  mAttemptingQuit(false),
  mRestart(false),
  mInterrupted(false),
  mIsSafeModeNecessary(false),
  mStartupCrashTrackingEnded(false),
  mRestartNotSameProfile(false)
{ }


nsresult
nsAppStartup::Init()
{
  nsresult rv;

  
  mAppShell = do_GetService(kAppShellCID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIObserverService> os =
    mozilla::services::GetObserverService();
  if (!os)
    return NS_ERROR_FAILURE;

  os->AddObserver(this, "quit-application", true);
  os->AddObserver(this, "quit-application-forced", true);
  os->AddObserver(this, "sessionstore-init-started", true);
  os->AddObserver(this, "sessionstore-windows-restored", true);
  os->AddObserver(this, "profile-change-teardown", true);
  os->AddObserver(this, "xul-window-registered", true);
  os->AddObserver(this, "xul-window-destroyed", true);
  os->AddObserver(this, "profile-before-change", true);
  os->AddObserver(this, "xpcom-shutdown", true);

#if defined(XP_WIN)
  os->AddObserver(this, "places-init-complete", true);
  

  
  mProbesManager =
    new ProbeManager(
                     kApplicationTracingCID,
                     NS_LITERAL_CSTRING("Application startup probe"));
  
  

  if (mProbesManager) {
    mPlacesInitCompleteProbe =
      mProbesManager->
      GetProbe(kPlacesInitCompleteCID,
               NS_LITERAL_CSTRING("places-init-complete"));
    NS_WARN_IF_FALSE(mPlacesInitCompleteProbe,
                     "Cannot initialize probe 'places-init-complete'");

    mSessionWindowRestoredProbe =
      mProbesManager->
      GetProbe(kSessionStoreWindowRestoredCID,
               NS_LITERAL_CSTRING("sessionstore-windows-restored"));
    NS_WARN_IF_FALSE(mSessionWindowRestoredProbe,
                     "Cannot initialize probe 'sessionstore-windows-restored'");
                     
    mXPCOMShutdownProbe =
      mProbesManager->
      GetProbe(kXPCOMShutdownCID,
               NS_LITERAL_CSTRING("xpcom-shutdown"));
    NS_WARN_IF_FALSE(mXPCOMShutdownProbe,
                     "Cannot initialize probe 'xpcom-shutdown'");

    rv = mProbesManager->StartSession();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "Cannot initialize system probe manager");
  }
#endif 

  return NS_OK;
}






NS_IMPL_ISUPPORTS(nsAppStartup,
                  nsIAppStartup,
                  nsIWindowCreator,
                  nsIWindowCreator2,
                  nsIObserver,
                  nsISupportsWeakReference)






NS_IMETHODIMP
nsAppStartup::CreateHiddenWindow()
{
#ifdef MOZ_WIDGET_GONK
  return NS_OK;
#else
  nsCOMPtr<nsIAppShellService> appShellService
    (do_GetService(NS_APPSHELLSERVICE_CONTRACTID));
  NS_ENSURE_TRUE(appShellService, NS_ERROR_FAILURE);

  return appShellService->CreateHiddenWindow();
#endif
}


NS_IMETHODIMP
nsAppStartup::DestroyHiddenWindow()
{
#ifdef MOZ_WIDGET_GONK
  return NS_OK;
#else
  nsCOMPtr<nsIAppShellService> appShellService
    (do_GetService(NS_APPSHELLSERVICE_CONTRACTID));
  NS_ENSURE_TRUE(appShellService, NS_ERROR_FAILURE);

  return appShellService->DestroyHiddenWindow();
#endif
}

NS_IMETHODIMP
nsAppStartup::Run(void)
{
  NS_ASSERTION(!mRunning, "Reentrant appstartup->Run()");

  
  
  
  

  if (!mShuttingDown && mConsiderQuitStopper != 0) {
#ifdef XP_MACOSX
    EnterLastWindowClosingSurvivalArea();
#endif

    mRunning = true;

    nsresult rv = mAppShell->Run();
    if (NS_FAILED(rv))
      return rv;
  }

  nsresult retval = NS_OK;
  if (mRestart) {
    retval = NS_SUCCESS_RESTART_APP;
  } else if (mRestartNotSameProfile) {
    retval = NS_SUCCESS_RESTART_APP_NOT_SAME_PROFILE;
  }

  return retval;
}



NS_IMETHODIMP
nsAppStartup::Quit(uint32_t aMode)
{
  uint32_t ferocity = (aMode & 0xF);

  
  
  
  nsresult rv = NS_OK;
  bool postedExitEvent = false;

  if (mShuttingDown)
    return NS_OK;

  
  if (ferocity == eConsiderQuit) {
#ifdef XP_MACOSX
    nsCOMPtr<nsIAppShellService> appShell
      (do_GetService(NS_APPSHELLSERVICE_CONTRACTID));
    bool hasHiddenPrivateWindow = false;
    if (appShell) {
      appShell->GetHasHiddenPrivateWindow(&hasHiddenPrivateWindow);
    }
    int32_t suspiciousCount = hasHiddenPrivateWindow ? 2 : 1;
#endif

    if (mConsiderQuitStopper == 0) {
      
      ferocity = eAttemptQuit;
    }
#ifdef XP_MACOSX
    else if (mConsiderQuitStopper == suspiciousCount) {
      

      
      if (!appShell)
        return NS_OK;

      bool usefulHiddenWindow;
      appShell->GetApplicationProvidedHiddenWindow(&usefulHiddenWindow);
      nsCOMPtr<nsIXULWindow> hiddenWindow;
      appShell->GetHiddenWindow(getter_AddRefs(hiddenWindow));
      
      nsCOMPtr<nsIXULWindow> hiddenPrivateWindow;
      if (hasHiddenPrivateWindow) {
        appShell->GetHiddenPrivateWindow(getter_AddRefs(hiddenPrivateWindow));
        if ((!hiddenWindow && !hiddenPrivateWindow) || usefulHiddenWindow)
          return NS_OK;
      } else if (!hiddenWindow || usefulHiddenWindow) {
        return NS_OK;
      }

      ferocity = eAttemptQuit;
    }
#endif
  }

  nsCOMPtr<nsIObserverService> obsService;
  if (ferocity == eAttemptQuit || ferocity == eForceQuit) {

    nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
    nsCOMPtr<nsIWindowMediator> mediator (do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));
    if (mediator) {
      mediator->GetEnumerator(nullptr, getter_AddRefs(windowEnumerator));
      if (windowEnumerator) {
        bool more;
        while (windowEnumerator->HasMoreElements(&more), more) {
          nsCOMPtr<nsISupports> window;
          windowEnumerator->GetNext(getter_AddRefs(window));
          nsCOMPtr<nsPIDOMWindow> domWindow(do_QueryInterface(window));
          if (domWindow) {
            MOZ_ASSERT(domWindow->IsOuterWindow());
            if (!domWindow->CanClose())
              return NS_OK;
          }
        }
      }
    }

    PROFILER_MARKER("Shutdown start");
    mozilla::RecordShutdownStartTimeStamp();
    mShuttingDown = true;
    if (!mRestart) {
      mRestart = (aMode & eRestart) != 0;
      gRestartMode = (aMode & 0xF0);
    }

    if (!mRestartNotSameProfile) {
      mRestartNotSameProfile = (aMode & eRestartNotSameProfile) != 0;
      gRestartMode = (aMode & 0xF0);
    }

    if (mRestart || mRestartNotSameProfile) {
      
      PR_SetEnv("MOZ_APP_RESTART=1");

      

      TimeStamp::RecordProcessRestart();
    }

    obsService = mozilla::services::GetObserverService();

    if (!mAttemptingQuit) {
      mAttemptingQuit = true;
#ifdef XP_MACOSX
      
      ExitLastWindowClosingSurvivalArea();
#endif
      if (obsService)
        obsService->NotifyObservers(nullptr, "quit-application-granted", nullptr);
    }

    



    CloseAllWindows();

    if (mediator) {
      if (ferocity == eAttemptQuit) {
        ferocity = eForceQuit; 

        





        mediator->GetEnumerator(nullptr, getter_AddRefs(windowEnumerator));
        if (windowEnumerator) {
          bool more;
          while (windowEnumerator->HasMoreElements(&more), more) {
            

            ferocity = eAttemptQuit;
            nsCOMPtr<nsISupports> window;
            windowEnumerator->GetNext(getter_AddRefs(window));
            nsCOMPtr<nsIDOMWindow> domWindow = do_QueryInterface(window);
            if (domWindow) {
              bool closed = false;
              domWindow->GetClosed(&closed);
              if (!closed) {
                rv = NS_ERROR_FAILURE;
                break;
              }
            }
          }
        }
      }
    }
  }

  if (ferocity == eForceQuit) {
    

    
    
    if (obsService) {
      NS_NAMED_LITERAL_STRING(shutdownStr, "shutdown");
      NS_NAMED_LITERAL_STRING(restartStr, "restart");
      obsService->NotifyObservers(nullptr, "quit-application",
        (mRestart || mRestartNotSameProfile) ?
         restartStr.get() : shutdownStr.get());
    }

    if (!mRunning) {
      postedExitEvent = true;
    }
    else {
      
      
      
      nsCOMPtr<nsIRunnable> event = new nsAppExitEvent(this);
      rv = NS_DispatchToCurrentThread(event);
      if (NS_SUCCEEDED(rv)) {
        postedExitEvent = true;
      }
      else {
        NS_WARNING("failed to dispatch nsAppExitEvent");
      }
    }
  }

  
  
  if (!postedExitEvent)
    mShuttingDown = false;
  return rv;
}


void
nsAppStartup::CloseAllWindows()
{
  nsCOMPtr<nsIWindowMediator> mediator
    (do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));

  nsCOMPtr<nsISimpleEnumerator> windowEnumerator;

  mediator->GetEnumerator(nullptr, getter_AddRefs(windowEnumerator));

  if (!windowEnumerator)
    return;

  bool more;
  while (NS_SUCCEEDED(windowEnumerator->HasMoreElements(&more)) && more) {
    nsCOMPtr<nsISupports> isupports;
    if (NS_FAILED(windowEnumerator->GetNext(getter_AddRefs(isupports))))
      break;

    nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(isupports);
    NS_ASSERTION(window, "not an nsPIDOMWindow");
    if (window) {
      MOZ_ASSERT(window->IsOuterWindow());
      window->ForceClose();
    }
  }
}

NS_IMETHODIMP
nsAppStartup::EnterLastWindowClosingSurvivalArea(void)
{
  ++mConsiderQuitStopper;
  return NS_OK;
}


NS_IMETHODIMP
nsAppStartup::ExitLastWindowClosingSurvivalArea(void)
{
  NS_ASSERTION(mConsiderQuitStopper > 0, "consider quit stopper out of bounds");
  --mConsiderQuitStopper;

  if (mRunning)
    Quit(eConsiderQuit);

  return NS_OK;
}





NS_IMETHODIMP
nsAppStartup::GetShuttingDown(bool *aResult)
{
  *aResult = mShuttingDown;
  return NS_OK;
}

NS_IMETHODIMP
nsAppStartup::GetStartingUp(bool *aResult)
{
  *aResult = mStartingUp;
  return NS_OK;
}

NS_IMETHODIMP
nsAppStartup::DoneStartingUp()
{
  
  MOZ_ASSERT(mStartingUp);

  mStartingUp = false;
  return NS_OK;
}

NS_IMETHODIMP
nsAppStartup::GetRestarting(bool *aResult)
{
  *aResult = mRestart;
  return NS_OK;
}

NS_IMETHODIMP
nsAppStartup::GetWasRestarted(bool *aResult)
{
  char *mozAppRestart = PR_GetEnv("MOZ_APP_RESTART");

  


  *aResult = mozAppRestart && (strcmp(mozAppRestart, "") != 0);

  return NS_OK;
}

NS_IMETHODIMP
nsAppStartup::SetInterrupted(bool aInterrupted)
{
  mInterrupted = aInterrupted;
  return NS_OK;
}

NS_IMETHODIMP
nsAppStartup::GetInterrupted(bool *aInterrupted)
{
  *aInterrupted = mInterrupted;
  return NS_OK;
}





NS_IMETHODIMP
nsAppStartup::CreateChromeWindow(nsIWebBrowserChrome *aParent,
                                 uint32_t aChromeFlags,
                                 nsIWebBrowserChrome **_retval)
{
  bool cancel;
  return CreateChromeWindow2(aParent, aChromeFlags, 0, 0, nullptr, &cancel, _retval);
}






NS_IMETHODIMP
nsAppStartup::CreateChromeWindow2(nsIWebBrowserChrome *aParent,
                                  uint32_t aChromeFlags,
                                  uint32_t aContextFlags,
                                  nsIURI *aURI,
                                  nsITabParent *aOpeningTab,
                                  bool *aCancel,
                                  nsIWebBrowserChrome **_retval)
{
  NS_ENSURE_ARG_POINTER(aCancel);
  NS_ENSURE_ARG_POINTER(_retval);
  *aCancel = false;
  *_retval = 0;

  
  if (mAttemptingQuit && (aChromeFlags & nsIWebBrowserChrome::CHROME_MODAL) == 0)
    return NS_ERROR_ILLEGAL_DURING_SHUTDOWN;

  nsCOMPtr<nsIXULWindow> newWindow;

  if (aParent) {
    nsCOMPtr<nsIXULWindow> xulParent(do_GetInterface(aParent));
    NS_ASSERTION(xulParent, "window created using non-XUL parent. that's unexpected, but may work.");

    if (xulParent)
      xulParent->CreateNewWindow(aChromeFlags, aOpeningTab, getter_AddRefs(newWindow));
    
    
  } else { 
    


    if (aChromeFlags & nsIWebBrowserChrome::CHROME_DEPENDENT)
      NS_WARNING("dependent window created without a parent");

    nsCOMPtr<nsIAppShellService> appShell(do_GetService(NS_APPSHELLSERVICE_CONTRACTID));
    if (!appShell)
      return NS_ERROR_FAILURE;

    appShell->CreateTopLevelWindow(0, 0, aChromeFlags,
                                   nsIAppShellService::SIZE_TO_CONTENT,
                                   nsIAppShellService::SIZE_TO_CONTENT,
                                   aOpeningTab,
                                   getter_AddRefs(newWindow));
  }

  
  if (newWindow) {
    newWindow->SetContextFlags(aContextFlags);
    nsCOMPtr<nsIInterfaceRequestor> thing(do_QueryInterface(newWindow));
    if (thing)
      CallGetInterface(thing.get(), _retval);
  }

  return *_retval ? NS_OK : NS_ERROR_FAILURE;
}






NS_IMETHODIMP
nsAppStartup::Observe(nsISupports *aSubject,
                      const char *aTopic, const char16_t *aData)
{
  NS_ASSERTION(mAppShell, "appshell service notified before appshell built");
  if (!strcmp(aTopic, "quit-application-forced")) {
    mShuttingDown = true;
  }
  else if (!strcmp(aTopic, "profile-change-teardown")) {
    if (!mShuttingDown) {
      EnterLastWindowClosingSurvivalArea();
      CloseAllWindows();
      ExitLastWindowClosingSurvivalArea();
    }
  } else if (!strcmp(aTopic, "xul-window-registered")) {
    EnterLastWindowClosingSurvivalArea();
  } else if (!strcmp(aTopic, "xul-window-destroyed")) {
    ExitLastWindowClosingSurvivalArea();
  } else if (!strcmp(aTopic, "sessionstore-windows-restored")) {
    StartupTimeline::Record(StartupTimeline::SESSION_RESTORED);
    IOInterposer::EnteringNextStage();
#if defined(XP_WIN)
    if (mSessionWindowRestoredProbe) {
      mSessionWindowRestoredProbe->Trigger();
    }
  } else if (!strcmp(aTopic, "places-init-complete")) {
    if (mPlacesInitCompleteProbe) {
      mPlacesInitCompleteProbe->Trigger();
    }
#endif
  } else if (!strcmp(aTopic, "sessionstore-init-started")) {
    StartupTimeline::Record(StartupTimeline::SESSION_RESTORE_INIT);
  } else if (!strcmp(aTopic, "xpcom-shutdown")) {
    IOInterposer::EnteringNextStage();
#if defined(XP_WIN)
    if (mXPCOMShutdownProbe) {
      mXPCOMShutdownProbe->Trigger();
    }
#endif 
  } else if (!strcmp(aTopic, "quit-application")) {
    StartupTimeline::Record(StartupTimeline::QUIT_APPLICATION);
  } else if (!strcmp(aTopic, "profile-before-change")) {
    StartupTimeline::Record(StartupTimeline::PROFILE_BEFORE_CHANGE);
  } else {
    NS_ERROR("Unexpected observer topic.");
  }

  return NS_OK;
}

NS_IMETHODIMP
nsAppStartup::GetStartupInfo(JSContext* aCx, JS::MutableHandle<JS::Value> aRetval)
{
  JS::Rooted<JSObject*> obj(aCx, JS_NewPlainObject(aCx));

  aRetval.setObject(*obj);

  TimeStamp procTime = StartupTimeline::Get(StartupTimeline::PROCESS_CREATION);
  TimeStamp now = TimeStamp::Now();
  PRTime absNow = PR_Now();

  if (procTime.IsNull()) {
    bool error = false;

    procTime = TimeStamp::ProcessCreation(error);

    if (error) {
      Telemetry::Accumulate(Telemetry::STARTUP_MEASUREMENT_ERRORS,
        StartupTimeline::PROCESS_CREATION);
    }

    StartupTimeline::Record(StartupTimeline::PROCESS_CREATION, procTime);
  }

  for (int i = StartupTimeline::PROCESS_CREATION;
       i < StartupTimeline::MAX_EVENT_ID;
       ++i)
  {
    StartupTimeline::Event ev = static_cast<StartupTimeline::Event>(i);
    TimeStamp stamp = StartupTimeline::Get(ev);

    if (stamp.IsNull() && (ev == StartupTimeline::MAIN)) {
      
      stamp = procTime;
      MOZ_ASSERT(!stamp.IsNull());
      Telemetry::Accumulate(Telemetry::STARTUP_MEASUREMENT_ERRORS,
        StartupTimeline::MAIN);
    }

    if (!stamp.IsNull()) {
      if (stamp >= procTime) {
        PRTime prStamp = ComputeAbsoluteTimestamp(absNow, now, stamp)
          / PR_USEC_PER_MSEC;
        JS::Rooted<JSObject*> date(aCx, JS_NewDateObjectMsec(aCx, prStamp));
        JS_DefineProperty(aCx, obj, StartupTimeline::Describe(ev), date, JSPROP_ENUMERATE);
      } else {
        Telemetry::Accumulate(Telemetry::STARTUP_MEASUREMENT_ERRORS, ev);
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsAppStartup::GetAutomaticSafeModeNecessary(bool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  bool alwaysSafe = false;
  Preferences::GetBool(kPrefAlwaysUseSafeMode, &alwaysSafe);

  if (!alwaysSafe) {
#if DEBUG
    mIsSafeModeNecessary = false;
#else
    mIsSafeModeNecessary &= !PR_GetEnv("MOZ_DISABLE_AUTO_SAFE_MODE");
#endif
  }

  *_retval = mIsSafeModeNecessary;
  return NS_OK;
}

NS_IMETHODIMP
nsAppStartup::TrackStartupCrashBegin(bool *aIsSafeModeNecessary)
{
  const int32_t MAX_TIME_SINCE_STARTUP = 6 * 60 * 60 * 1000;
  const int32_t MAX_STARTUP_BUFFER = 10;
  nsresult rv;

  mStartupCrashTrackingEnded = false;

  StartupTimeline::Record(StartupTimeline::STARTUP_CRASH_DETECTION_BEGIN);

  bool hasLastSuccess = Preferences::HasUserValue(kPrefLastSuccess);
  if (!hasLastSuccess) {
    
    
    Preferences::ClearUser(kPrefRecentCrashes);
    return NS_ERROR_NOT_AVAILABLE;
  }

  bool inSafeMode = false;
  nsCOMPtr<nsIXULRuntime> xr = do_GetService(XULRUNTIME_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(xr, NS_ERROR_FAILURE);

  xr->GetInSafeMode(&inSafeMode);

  PRTime replacedLockTime;
  rv = xr->GetReplacedLockTime(&replacedLockTime);

  if (NS_FAILED(rv) || !replacedLockTime) {
    if (!inSafeMode)
      Preferences::ClearUser(kPrefRecentCrashes);
    GetAutomaticSafeModeNecessary(aIsSafeModeNecessary);
    return NS_OK;
  }

  
  int32_t maxResumedCrashes = -1;
  rv = Preferences::GetInt(kPrefMaxResumedCrashes, &maxResumedCrashes);
  NS_ENSURE_SUCCESS(rv, NS_OK);

  int32_t recentCrashes = 0;
  Preferences::GetInt(kPrefRecentCrashes, &recentCrashes);
  mIsSafeModeNecessary = (recentCrashes > maxResumedCrashes && maxResumedCrashes != -1);

  
  
  
  char *xreProfilePath = PR_GetEnv("XRE_PROFILE_PATH");
  if (xreProfilePath) {
    GetAutomaticSafeModeNecessary(aIsSafeModeNecessary);
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  int32_t lastSuccessfulStartup;
  rv = Preferences::GetInt(kPrefLastSuccess, &lastSuccessfulStartup);
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t lockSeconds = (int32_t)(replacedLockTime / PR_MSEC_PER_SEC);

  
  if (lockSeconds <= lastSuccessfulStartup + MAX_STARTUP_BUFFER
      && lockSeconds >= lastSuccessfulStartup - MAX_STARTUP_BUFFER) {
    GetAutomaticSafeModeNecessary(aIsSafeModeNecessary);
    return NS_OK;
  }

  
  if (PR_Now() / PR_USEC_PER_SEC <= lastSuccessfulStartup)
    return NS_ERROR_FAILURE;

  
  Telemetry::Accumulate(Telemetry::STARTUP_CRASH_DETECTED, true);

  if (inSafeMode) {
    GetAutomaticSafeModeNecessary(aIsSafeModeNecessary);
    return NS_OK;
  }

  PRTime now = (PR_Now() / PR_USEC_PER_MSEC);
  
  if (replacedLockTime >= now - MAX_TIME_SINCE_STARTUP) {
    NS_WARNING("Last startup was detected as a crash.");
    recentCrashes++;
    rv = Preferences::SetInt(kPrefRecentCrashes, recentCrashes);
  } else {
    
    
    rv = Preferences::ClearUser(kPrefRecentCrashes);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  mIsSafeModeNecessary = (recentCrashes > maxResumedCrashes && maxResumedCrashes != -1);

  nsCOMPtr<nsIPrefService> prefs = Preferences::GetService();
  rv = prefs->SavePrefFile(nullptr); 
  NS_ENSURE_SUCCESS(rv, rv);

  GetAutomaticSafeModeNecessary(aIsSafeModeNecessary);
  return rv;
}

NS_IMETHODIMP
nsAppStartup::TrackStartupCrashEnd()
{
  bool inSafeMode = false;
  nsCOMPtr<nsIXULRuntime> xr = do_GetService(XULRUNTIME_SERVICE_CONTRACTID);
  if (xr)
    xr->GetInSafeMode(&inSafeMode);

  
  if (mStartupCrashTrackingEnded || (mIsSafeModeNecessary && !inSafeMode))
    return NS_OK;
  mStartupCrashTrackingEnded = true;

  StartupTimeline::Record(StartupTimeline::STARTUP_CRASH_DETECTION_END);

  
  
  TimeStamp mainTime = StartupTimeline::Get(StartupTimeline::MAIN);
  TimeStamp now = TimeStamp::Now();
  PRTime prNow = PR_Now();
  nsresult rv;

  if (mainTime.IsNull()) {
    NS_WARNING("Could not get StartupTimeline::MAIN time.");
  } else {
    uint64_t lockFileTime = ComputeAbsoluteTimestamp(prNow, now, mainTime);

    rv = Preferences::SetInt(kPrefLastSuccess,
      (int32_t)(lockFileTime / PR_USEC_PER_SEC));

    if (NS_FAILED(rv))
      NS_WARNING("Could not set startup crash detection pref.");
  }

  if (inSafeMode && mIsSafeModeNecessary) {
    
    
    int32_t maxResumedCrashes = 0;
    int32_t prefType;
    rv = Preferences::GetDefaultRootBranch()->GetPrefType(kPrefMaxResumedCrashes, &prefType);
    NS_ENSURE_SUCCESS(rv, rv);
    if (prefType == nsIPrefBranch::PREF_INT) {
      rv = Preferences::GetInt(kPrefMaxResumedCrashes, &maxResumedCrashes);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    rv = Preferences::SetInt(kPrefRecentCrashes, maxResumedCrashes);
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (!inSafeMode) {
    
    rv = Preferences::ClearUser(kPrefRecentCrashes);
    if (NS_FAILED(rv)) NS_WARNING("Could not clear startup crash count.");
  }
  nsCOMPtr<nsIPrefService> prefs = Preferences::GetService();
  rv = prefs->SavePrefFile(nullptr); 

  return rv;
}

NS_IMETHODIMP
nsAppStartup::RestartInSafeMode(uint32_t aQuitMode)
{
  PR_SetEnv("MOZ_SAFE_MODE_RESTART=1");
  this->Quit(aQuitMode | nsIAppStartup::eRestart);

  return NS_OK;
}
