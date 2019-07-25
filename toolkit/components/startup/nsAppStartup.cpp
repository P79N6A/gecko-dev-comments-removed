











































#include "nsAppStartup.h"

#include "nsIAppShellService.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMWindowInternal.h"
#include "nsIInterfaceRequestor.h"
#include "nsILocalFile.h"
#include "nsIObserverService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIProfileChangeStatus.h"
#include "nsIPromptService.h"
#include "nsIStringBundle.h"
#include "nsISupportsPrimitives.h"
#include "nsITimelineService.h"
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

static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);
extern PRTime gXRE_mainTimestamp;
extern PRTime gFirstPaintTimestamp;

static PRTime gRestoredTimestamp = 0;       
static PRTime gProcessCreationTimestamp = 0;

PRUint32 gRestartMode = 0;

class nsAppExitEvent : public nsRunnable {
private:
  nsRefPtr<nsAppStartup> mService;

public:
  nsAppExitEvent(nsAppStartup *service) : mService(service) {}

  NS_IMETHOD Run() {
    
    mService->mAppShell->Exit();

    
    mService->mShuttingDown = PR_FALSE;
    mService->mRunning = PR_FALSE;
    return NS_OK;
  }
};





nsAppStartup::nsAppStartup() :
  mConsiderQuitStopper(0),
  mRunning(PR_FALSE),
  mShuttingDown(PR_FALSE),
  mAttemptingQuit(PR_FALSE),
  mRestart(PR_FALSE)
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

  os->AddObserver(this, "quit-application-forced", PR_TRUE);
  os->AddObserver(this, "sessionstore-windows-restored", PR_TRUE);
  os->AddObserver(this, "profile-change-teardown", PR_TRUE);
  os->AddObserver(this, "xul-window-registered", PR_TRUE);
  os->AddObserver(this, "xul-window-destroyed", PR_TRUE);

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

  return appShellService->CreateHiddenWindow(mAppShell);
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

    mRunning = PR_TRUE;

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
  PRBool postedExitEvent = PR_FALSE;

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

      PRBool usefulHiddenWindow;
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
        PRBool more;
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

    mShuttingDown = PR_TRUE;
    if (!mRestart) {
      mRestart = (aMode & eRestart) != 0;
      gRestartMode = (aMode & 0xF0);
    }

    if (mRestart) {
      
      PR_SetEnv(PR_smprintf("MOZ_APP_RESTART=%lld", (PRInt64) PR_Now() / PR_USEC_PER_MSEC));
    }

    obsService = mozilla::services::GetObserverService();

    if (!mAttemptingQuit) {
      mAttemptingQuit = PR_TRUE;
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
          PRBool more;
          while (windowEnumerator->HasMoreElements(&more), more) {
            

            ferocity = eAttemptQuit;
            nsCOMPtr<nsISupports> window;
            windowEnumerator->GetNext(getter_AddRefs(window));
            nsCOMPtr<nsIDOMWindowInternal> domWindow(do_QueryInterface(window));
            if (domWindow) {
              PRBool closed = PR_FALSE;
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
      postedExitEvent = PR_TRUE;
    }
    else {
      
      
      
      nsCOMPtr<nsIRunnable> event = new nsAppExitEvent(this);
      rv = NS_DispatchToCurrentThread(event);
      if (NS_SUCCEEDED(rv)) {
        postedExitEvent = PR_TRUE;
      }
      else {
        NS_WARNING("failed to dispatch nsAppExitEvent");
      }
    }
  }

  
  
  if (!postedExitEvent)
    mShuttingDown = PR_FALSE;
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

  PRBool more;
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
nsAppStartup::GetShuttingDown(PRBool *aResult)
{
  *aResult = mShuttingDown;
  return NS_OK;
}





NS_IMETHODIMP
nsAppStartup::CreateChromeWindow(nsIWebBrowserChrome *aParent,
                                 PRUint32 aChromeFlags,
                                 nsIWebBrowserChrome **_retval)
{
  PRBool cancel;
  return CreateChromeWindow2(aParent, aChromeFlags, 0, 0, &cancel, _retval);
}






NS_IMETHODIMP
nsAppStartup::CreateChromeWindow2(nsIWebBrowserChrome *aParent,
                                  PRUint32 aChromeFlags,
                                  PRUint32 aContextFlags,
                                  nsIURI *aURI,
                                  PRBool *aCancel,
                                  nsIWebBrowserChrome **_retval)
{
  NS_ENSURE_ARG_POINTER(aCancel);
  NS_ENSURE_ARG_POINTER(_retval);
  *aCancel = PR_FALSE;
  *_retval = 0;

  
  if (mAttemptingQuit && (aChromeFlags & nsIWebBrowserChrome::CHROME_MODAL) == 0)
    return NS_ERROR_ILLEGAL_DURING_SHUTDOWN;

  nsCOMPtr<nsIXULWindow> newWindow;

  if (aParent) {
    nsCOMPtr<nsIXULWindow> xulParent(do_GetInterface(aParent));
    NS_ASSERTION(xulParent, "window created using non-XUL parent. that's unexpected, but may work.");

    if (xulParent)
      xulParent->CreateNewWindow(aChromeFlags, mAppShell, getter_AddRefs(newWindow));
    
    
  } else { 
    


    if (aChromeFlags & nsIWebBrowserChrome::CHROME_DEPENDENT)
      NS_WARNING("dependent window created without a parent");

    nsCOMPtr<nsIAppShellService> appShell(do_GetService(NS_APPSHELLSERVICE_CONTRACTID));
    if (!appShell)
      return NS_ERROR_FAILURE;
    
    appShell->CreateTopLevelWindow(0, 0, aChromeFlags,
                                   nsIAppShellService::SIZE_TO_CONTENT,
                                   nsIAppShellService::SIZE_TO_CONTENT,
                                   mAppShell, getter_AddRefs(newWindow));
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
    mShuttingDown = PR_TRUE;
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
    gRestoredTimestamp = PR_Now();
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
  sscanf(s + 2,
         "%*c %*d %*d %*d %*d %*d %*u %*u %*u %*u "
         "%*u %*u %*u %*u %*u %*d %*d %*d %*d %llu",
         &starttime);
  if (!starttime)
    return 0;
  return starttime;
}

static void
ThreadedCalculateProcessCreationTimestamp(void *aClosure)
{
  PRTime now = PR_Now();
  gProcessCreationTimestamp = 0;
  long hz = sysconf(_SC_CLK_TCK);
  if (!hz)
    return;

  char thread_stat[40];
  sprintf(thread_stat, "/proc/self/task/%d/stat", (pid_t) syscall(__NR_gettid));
  
  PRTime interval = (JiffiesSinceBoot(thread_stat) - JiffiesSinceBoot("/proc/self/stat")) * PR_USEC_PER_SEC / hz;;
  gProcessCreationTimestamp = now - interval;
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
  return gProcessCreationTimestamp;
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
 
static void
MaybeDefineProperty(JSContext *cx, JSObject *obj, const char *name, PRTime timestamp)
{
  if (!timestamp)
    return;
  JSObject *date = js_NewDateObjectMsec(cx, timestamp/PR_USEC_PER_MSEC);
  JS_DefineProperty(cx, obj, name, OBJECT_TO_JSVAL(date), NULL, NULL, JSPROP_ENUMERATE);     
}

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
  ncc->SetReturnValueWasSet(PR_TRUE);

  JSContext *cx = nsnull;
  rv = ncc->GetJSContext(&cx);
  NS_ENSURE_SUCCESS(rv, rv);

  JSObject *obj = JS_NewObject(cx, NULL, NULL, NULL);
  *retvalPtr = OBJECT_TO_JSVAL(obj);
  ncc->SetReturnValueWasSet(PR_TRUE);

  char *moz_app_restart = PR_GetEnv("MOZ_APP_RESTART");
  if (moz_app_restart) {
    gProcessCreationTimestamp = nsCRT::atoll(moz_app_restart) * PR_USEC_PER_MSEC;
  } else if (!gProcessCreationTimestamp) {
    gProcessCreationTimestamp = CalculateProcessCreationTimestamp();
  }

  MaybeDefineProperty(cx, obj, "process", gProcessCreationTimestamp);
  MaybeDefineProperty(cx, obj, "main", gXRE_mainTimestamp);
  MaybeDefineProperty(cx, obj, "firstPaint", gFirstPaintTimestamp);
  MaybeDefineProperty(cx, obj, "sessionRestored", gRestoredTimestamp);
  return NS_OK;
}
