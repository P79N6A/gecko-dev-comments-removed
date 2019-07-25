











































#include "nsAppStartup.h"

#include "nsIAppShellService.h"
#include "nsPIDOMWindow.h"
#include "nsIInterfaceRequestor.h"
#include "nsILocalFile.h"
#include "nsIObserverService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIProfileChangeStatus.h"
#include "nsIPromptService.h"
#include "nsIStringBundle.h"
#include "nsISupportsPrimitives.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWindowMediator.h"
#include "nsIWindowWatcher.h"
#include "nsIXULWindow.h"
#include "nsNativeCharsetUtils.h"
#include "nsThreadUtils.h"
#include "nsAutoPtr.h"
#include "nsStringGlue.h"

#include "prprf.h"
#include "nsCRT.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsWidgetsCID.h"
#include "nsAppShellCID.h"
#include "mozilla/Services.h"
#include "mozilla/FunctionTimer.h"
#include "nsIXPConnect.h"
#include "jsapi.h"
#include "jsdate.h"
#include "prenv.h"

#if defined(XP_WIN)
#include <windows.h>

#undef GetStartupInfo
#elif defined(XP_UNIX)
#include <unistd.h>
#include <sys/syscall.h>
#endif

#ifdef XP_MACOSX
#include <sys/sysctl.h>
#endif

#ifdef __OpenBSD__
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#include "mozilla/Telemetry.h"
#include "mozilla/StartupTimeline.h"

static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

using namespace mozilla;

PRUint32 gRestartMode = 0;

class nsAppExitEvent : public nsRunnable {
private:
  nsRefPtr<nsAppStartup> mService;

public:
  nsAppExitEvent(nsAppStartup *service) : mService(service) {}

  NS_IMETHOD Run() {
    
    mService->mAppShell->Exit();

    
    mService->mShuttingDown = false;
    mService->mRunning = false;
    return NS_OK;
  }
};





nsAppStartup::nsAppStartup() :
  mConsiderQuitStopper(0),
  mRunning(false),
  mShuttingDown(false),
  mAttemptingQuit(false),
  mRestart(false),
  mInterrupted(false)
{ }


nsresult
nsAppStartup::Init()
{
  NS_TIME_FUNCTION;
  nsresult rv;

  
  mAppShell = do_GetService(kAppShellCID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_TIME_FUNCTION_MARK("Got AppShell service");

  nsCOMPtr<nsIObserverService> os =
    mozilla::services::GetObserverService();
  if (!os)
    return NS_ERROR_FAILURE;

  NS_TIME_FUNCTION_MARK("Got Observer service");

  os->AddObserver(this, "quit-application-forced", true);
  os->AddObserver(this, "sessionstore-windows-restored", true);
  os->AddObserver(this, "profile-change-teardown", true);
  os->AddObserver(this, "xul-window-registered", true);
  os->AddObserver(this, "xul-window-destroyed", true);

  return NS_OK;
}






NS_IMPL_THREADSAFE_ISUPPORTS5(nsAppStartup,
                              nsIAppStartup,
                              nsIWindowCreator,
                              nsIWindowCreator2,
                              nsIObserver,
                              nsISupportsWeakReference)






NS_IMETHODIMP
nsAppStartup::CreateHiddenWindow()
{
  nsCOMPtr<nsIAppShellService> appShellService
    (do_GetService(NS_APPSHELLSERVICE_CONTRACTID));
  NS_ENSURE_TRUE(appShellService, NS_ERROR_FAILURE);

  return appShellService->CreateHiddenWindow();
}


NS_IMETHODIMP
nsAppStartup::DestroyHiddenWindow()
{
  nsCOMPtr<nsIAppShellService> appShellService
    (do_GetService(NS_APPSHELLSERVICE_CONTRACTID));
  NS_ENSURE_TRUE(appShellService, NS_ERROR_FAILURE);

  return appShellService->DestroyHiddenWindow();
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

  return mRestart ? NS_SUCCESS_RESTART_APP : NS_OK;
}


NS_IMETHODIMP
nsAppStartup::Quit(PRUint32 aMode)
{
  PRUint32 ferocity = (aMode & 0xF);

  
  
  
  nsresult rv = NS_OK;
  bool postedExitEvent = false;

  if (mShuttingDown)
    return NS_OK;

  
  if (ferocity == eConsiderQuit) {
    if (mConsiderQuitStopper == 0) {
      
      ferocity = eAttemptQuit;
    }
#ifdef XP_MACOSX
    else if (mConsiderQuitStopper == 1) {
      
      nsCOMPtr<nsIAppShellService> appShell
        (do_GetService(NS_APPSHELLSERVICE_CONTRACTID));

      
      if (!appShell)
        return NS_OK;

      bool usefulHiddenWindow;
      appShell->GetApplicationProvidedHiddenWindow(&usefulHiddenWindow);
      nsCOMPtr<nsIXULWindow> hiddenWindow;
      appShell->GetHiddenWindow(getter_AddRefs(hiddenWindow));
      
      if (!hiddenWindow || usefulHiddenWindow)
        return NS_OK;

      ferocity = eAttemptQuit;
    }
#endif
  }

  nsCOMPtr<nsIObserverService> obsService;
  if (ferocity == eAttemptQuit || ferocity == eForceQuit) {

    nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
    nsCOMPtr<nsIWindowMediator> mediator (do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));
    if (mediator) {
      mediator->GetEnumerator(nsnull, getter_AddRefs(windowEnumerator));
      if (windowEnumerator) {
        bool more;
        while (windowEnumerator->HasMoreElements(&more), more) {
          nsCOMPtr<nsISupports> window;
          windowEnumerator->GetNext(getter_AddRefs(window));
          nsCOMPtr<nsPIDOMWindow> domWindow(do_QueryInterface(window));
          if (domWindow) {
            if (!domWindow->CanClose())
              return NS_OK;
          }
        }
      }
    }

    mShuttingDown = true;
    if (!mRestart) {
      mRestart = (aMode & eRestart) != 0;
      gRestartMode = (aMode & 0xF0);
    }

    if (mRestart) {
      
      PR_SetEnv(PR_smprintf("MOZ_APP_RESTART=%lld", (PRInt64) PR_Now() / PR_USEC_PER_MSEC));
    }

    obsService = mozilla::services::GetObserverService();

    if (!mAttemptingQuit) {
      mAttemptingQuit = true;
#ifdef XP_MACOSX
      
      ExitLastWindowClosingSurvivalArea();
#endif
      if (obsService)
        obsService->NotifyObservers(nsnull, "quit-application-granted", nsnull);
    }

    



    CloseAllWindows();

    if (mediator) {
      if (ferocity == eAttemptQuit) {
        ferocity = eForceQuit; 

        





        mediator->GetEnumerator(nsnull, getter_AddRefs(windowEnumerator));
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
      obsService->NotifyObservers(nsnull, "quit-application",
        mRestart ? restartStr.get() : shutdownStr.get());
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

  mediator->GetEnumerator(nsnull, getter_AddRefs(windowEnumerator));

  if (!windowEnumerator)
    return;

  bool more;
  while (NS_SUCCEEDED(windowEnumerator->HasMoreElements(&more)) && more) {
    nsCOMPtr<nsISupports> isupports;
    if (NS_FAILED(windowEnumerator->GetNext(getter_AddRefs(isupports))))
      break;

    nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(isupports);
    NS_ASSERTION(window, "not an nsPIDOMWindow");
    if (window)
      window->ForceClose();
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
                                 PRUint32 aChromeFlags,
                                 nsIWebBrowserChrome **_retval)
{
  bool cancel;
  return CreateChromeWindow2(aParent, aChromeFlags, 0, 0, &cancel, _retval);
}






NS_IMETHODIMP
nsAppStartup::CreateChromeWindow2(nsIWebBrowserChrome *aParent,
                                  PRUint32 aChromeFlags,
                                  PRUint32 aContextFlags,
                                  nsIURI *aURI,
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
      xulParent->CreateNewWindow(aChromeFlags, getter_AddRefs(newWindow));
    
    
  } else { 
    


    if (aChromeFlags & nsIWebBrowserChrome::CHROME_DEPENDENT)
      NS_WARNING("dependent window created without a parent");

    nsCOMPtr<nsIAppShellService> appShell(do_GetService(NS_APPSHELLSERVICE_CONTRACTID));
    if (!appShell)
      return NS_ERROR_FAILURE;
    
    appShell->CreateTopLevelWindow(0, 0, aChromeFlags,
                                   nsIAppShellService::SIZE_TO_CONTENT,
                                   nsIAppShellService::SIZE_TO_CONTENT,
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
                      const char *aTopic, const PRUnichar *aData)
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
  } else {
    NS_ERROR("Unexpected observer topic.");
  }

  return NS_OK;
}

#if defined(LINUX) || defined(ANDROID)
static PRUint64 
JiffiesSinceBoot(const char *file)
{
  char stat[512];
  FILE *f = fopen(file, "r");
  if (!f)
    return 0;
  int n = fread(&stat, 1, sizeof(stat) - 1, f);
  fclose(f);
  if (n <= 0)
    return 0;
  stat[n] = 0;
  
  long long unsigned starttime = 0; 
  
  char *s = strrchr(stat, ')');
  if (!s)
    return 0;
  int ret = sscanf(s + 2,
                   "%*c %*d %*d %*d %*d %*d %*u %*u %*u %*u "
                   "%*u %*u %*u %*u %*u %*d %*d %*d %*d %llu",
                   &starttime);
  if (ret != 1 || !starttime)
    return 0;
  return starttime;
}

static void
ThreadedCalculateProcessCreationTimestamp(void *aClosure)
{
  PRTime now = PR_Now();
  long hz = sysconf(_SC_CLK_TCK);
  if (!hz)
    return;

  char thread_stat[40];
  sprintf(thread_stat, "/proc/self/task/%d/stat", (pid_t) syscall(__NR_gettid));
  
  PRUint64 thread_jiffies = JiffiesSinceBoot(thread_stat);
  PRUint64 self_jiffies = JiffiesSinceBoot("/proc/self/stat");
  
  if (!thread_jiffies || !self_jiffies)
    return;

  PRTime interval = (thread_jiffies - self_jiffies) * PR_USEC_PER_SEC / hz;
  StartupTimeline::Record(StartupTimeline::PROCESS_CREATION, now - interval);
}

static PRTime
CalculateProcessCreationTimestamp()
{
 PRThread *thread = PR_CreateThread(PR_USER_THREAD,
                                    ThreadedCalculateProcessCreationTimestamp,
                                    NULL,
                                    PR_PRIORITY_NORMAL,
                                    PR_LOCAL_THREAD,
                                    PR_JOINABLE_THREAD,
                                    0);

  PR_JoinThread(thread);
  return StartupTimeline::Get(StartupTimeline::PROCESS_CREATION);
}
#elif defined(XP_WIN)
static PRTime
CalculateProcessCreationTimestamp()
{
  FILETIME start, foo, bar, baz;
  bool success = GetProcessTimes(GetCurrentProcess(), &start, &foo, &bar, &baz);
  if (!success)
    return 0;
  
  PRUint64 timestamp = 0;
  CopyMemory(&timestamp, &start, sizeof(PRTime));
#ifdef __GNUC__
  timestamp = (timestamp - 116444736000000000LL) / 10LL;
#else
  timestamp = (timestamp - 116444736000000000i64) / 10i64;
#endif
  return timestamp;
}
#elif defined(XP_MACOSX)
static PRTime
CalculateProcessCreationTimestamp()
{
  int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };
  size_t buffer_size;
  if (sysctl(mib, 4, NULL, &buffer_size, NULL, 0))
    return 0;

  struct kinfo_proc *proc = (kinfo_proc*) malloc(buffer_size);  
  if (sysctl(mib, 4, proc, &buffer_size, NULL, 0)) {
    free(proc);
    return 0;
  }
  PRTime starttime = static_cast<PRTime>(proc->kp_proc.p_un.__p_starttime.tv_sec) * PR_USEC_PER_SEC;
  starttime += proc->kp_proc.p_un.__p_starttime.tv_usec;
  free(proc);
  return starttime;
}
#elif defined(__OpenBSD__)
static PRTime
CalculateProcessCreationTimestamp()
{
  int mib[6] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid(), sizeof(struct kinfo_proc), 1 };
  size_t buffer_size;
  if (sysctl(mib, 6, NULL, &buffer_size, NULL, 0))
    return 0;

  struct kinfo_proc *proc = (struct kinfo_proc*) malloc(buffer_size);
  if (sysctl(mib, 6, proc, &buffer_size, NULL, 0)) {
    free(proc);
    return 0;
  }
  PRTime starttime = static_cast<PRTime>(proc->p_ustart_sec) * PR_USEC_PER_SEC;
  starttime += proc->p_ustart_usec;
  free(proc);
  return starttime;
}
#else
static PRTime
CalculateProcessCreationTimestamp()
{
  return 0;
}
#endif
 
NS_IMETHODIMP
nsAppStartup::GetStartupInfo()
{
  nsAXPCNativeCallContext *ncc = nsnull;
  nsresult rv;
  nsCOMPtr<nsIXPConnect> xpConnect = do_GetService(nsIXPConnect::GetCID(), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = xpConnect->GetCurrentNativeCallContext(&ncc);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!ncc)
    return NS_ERROR_FAILURE;

  jsval *retvalPtr;
  ncc->GetRetValPtr(&retvalPtr);

  *retvalPtr = JSVAL_NULL;
  ncc->SetReturnValueWasSet(true);

  JSContext *cx = nsnull;
  rv = ncc->GetJSContext(&cx);
  NS_ENSURE_SUCCESS(rv, rv);

  JSObject *obj = JS_NewObject(cx, NULL, NULL, NULL);
  *retvalPtr = OBJECT_TO_JSVAL(obj);
  ncc->SetReturnValueWasSet(true);

  PRTime ProcessCreationTimestamp = StartupTimeline::Get(StartupTimeline::PROCESS_CREATION);

  if (!ProcessCreationTimestamp) {
    char *moz_app_restart = PR_GetEnv("MOZ_APP_RESTART");
    if (moz_app_restart) {
      ProcessCreationTimestamp = nsCRT::atoll(moz_app_restart) * PR_USEC_PER_MSEC;
    } else {
      ProcessCreationTimestamp = CalculateProcessCreationTimestamp();
    }
    
    if (PR_Now() <= ProcessCreationTimestamp) {
      ProcessCreationTimestamp = -1;
      Telemetry::Accumulate(Telemetry::STARTUP_MEASUREMENT_ERRORS, StartupTimeline::PROCESS_CREATION);
    }
    StartupTimeline::Record(StartupTimeline::PROCESS_CREATION, ProcessCreationTimestamp);
  }

  for (int i = StartupTimeline::PROCESS_CREATION; i < StartupTimeline::MAX_EVENT_ID; ++i) {
    StartupTimeline::Event ev = static_cast<StartupTimeline::Event>(i);
    if (StartupTimeline::Get(ev) > 0) {
      
      if ((ev != StartupTimeline::MAIN) &&
          (StartupTimeline::Get(ev) < StartupTimeline::Get(StartupTimeline::PROCESS_CREATION))) {
        Telemetry::Accumulate(Telemetry::STARTUP_MEASUREMENT_ERRORS, i);
        StartupTimeline::Record(ev, -1);
      } else {
        JSObject *date = js_NewDateObjectMsec(cx, StartupTimeline::Get(ev)/PR_USEC_PER_MSEC);
        JS_DefineProperty(cx, obj, StartupTimeline::Describe(ev), OBJECT_TO_JSVAL(date), NULL, NULL, JSPROP_ENUMERATE);
      }
    }
  }

  return NS_OK;
}
