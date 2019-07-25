








































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
#include "mozilla/storage.h"
#include "mozilla/FunctionTimer.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIXULRuntime.h"
#include "nsIXULAppInfo.h"
#include "nsXPCOMCIDInternal.h"
#include "nsIPropertyBag2.h"

static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

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

nsresult RecordStartupDuration();
nsresult OpenStartupDatabase(mozIStorageConnection **db);
nsresult EnsureTable(mozIStorageConnection *db, const nsACString &table, const nsACString &schema);
nsresult RecordAddonEvent(const PRUnichar *event, nsISupports *details);





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
  os->AddObserver(this, "sessionstore-browser-state-restored", PR_TRUE);
  os->AddObserver(this, "sessionstore-windows-restored", PR_TRUE);
  os->AddObserver(this, "AddonManager-event", PR_TRUE);
  os->AddObserver(this, "profile-change-teardown", PR_TRUE);
  os->AddObserver(this, "xul-window-registered", PR_TRUE);
  os->AddObserver(this, "xul-window-destroyed", PR_TRUE);

  return NS_OK;
}






NS_IMPL_THREADSAFE_ISUPPORTS6(nsAppStartup,
                              nsIAppStartup,
                              nsIAppStartup2,
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
    if (!mRestart)
      mRestart = (aMode & eRestart) != 0;

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
  } else if ((!strcmp(aTopic, "sessionstore-browser-state-restored")) ||
             (!strcmp(aTopic, "sessionstore-windows-restored"))) {
    RecordStartupDuration();
  } else if (!strcmp(aTopic, "AddonManager-event")) {
    RecordAddonEvent(aData, aSubject);
  } else {
    NS_ERROR("Unexpected observer topic.");
  }

  return NS_OK;
}

nsresult RecordStartupDuration()
{
  nsresult rv;
  PRTime launched, started, finished;
  finished = PR_Now();

  nsCOMPtr<mozIStorageConnection> db;
  rv = OpenStartupDatabase(getter_AddRefs(db));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageStatement> statement;
  rv = db->CreateStatement(NS_LITERAL_CSTRING("INSERT INTO duration VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7)"),
                           getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIXULRuntime> runtime = do_GetService(XULRUNTIME_SERVICE_CONTRACTID);
  nsCOMPtr<nsIXULAppInfo> appinfo = do_QueryInterface(runtime);

  runtime->GetLaunchTimestamp((PRUint64*)&launched);
  runtime->GetStartupTimestamp((PRUint64*)&started);

  nsCAutoString appVersion, appBuild, platformVersion, platformBuild;
  appinfo->GetVersion(appVersion);
  appinfo->GetAppBuildID(appBuild);
  appinfo->GetPlatformVersion(platformVersion);
  appinfo->GetPlatformBuildID(platformBuild);

  rv = statement->BindInt64Parameter(0, launched);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindInt64Parameter(1, started - launched);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindInt64Parameter(2, finished - started);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindStringParameter(3, NS_ConvertUTF8toUTF16(appVersion));
  NS_ENSURE_SUCCESS(rv, rv); 
  rv = statement->BindStringParameter(4, NS_ConvertUTF8toUTF16(appBuild));
  NS_ENSURE_SUCCESS(rv, rv); 
  rv = statement->BindStringParameter(5, NS_ConvertUTF8toUTF16(platformVersion));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindStringParameter(6, NS_ConvertUTF8toUTF16(platformBuild));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult RecordAddonEvent(const PRUnichar *event, nsISupports *details)
{
  PRTime now = PR_Now();
  nsresult rv;

  nsCOMPtr<mozIStorageConnection> db;
  rv = OpenStartupDatabase(getter_AddRefs(db));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageStatement> statement;
  rv = db->CreateStatement(NS_LITERAL_CSTRING("INSERT INTO events VALUES (?1, ?2, ?3, ?4, ?5)"),
                           getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPropertyBag2> bag = do_QueryInterface(details);
  nsAutoString id, name, version;
  bag->GetPropertyAsAString(NS_LITERAL_STRING("id"), id);
  bag->GetPropertyAsAString(NS_LITERAL_STRING("name"), name);
  bag->GetPropertyAsAString(NS_LITERAL_STRING("version"), version);

  rv = statement->BindInt64Parameter(0, now);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindStringParameter(1, id);
  NS_ENSURE_SUCCESS(rv, rv); 
  rv = statement->BindStringParameter(2, name);
  NS_ENSURE_SUCCESS(rv, rv); 
  rv = statement->BindStringParameter(3, version);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindStringParameter(4, nsDependentString(event));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult OpenStartupDatabase(mozIStorageConnection **db)
{
  nsresult rv;
  nsCOMPtr<nsIFile> file;
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = file->Append(NS_LITERAL_STRING("startup.sqlite"));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<mozIStorageService> svc = do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = svc->OpenDatabase(file, db);
  if (NS_ERROR_FILE_CORRUPTED == rv)
  {
    svc->BackupDatabaseFile(file, NS_LITERAL_STRING("startup.sqlite.backup"), nsnull, nsnull);
    rv = svc->OpenDatabase(file, db);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  rv = EnsureTable(*db,
                   NS_LITERAL_CSTRING("duration"),
                   NS_LITERAL_CSTRING("timestamp INTEGER, launch INTEGER, startup INTEGER, appVersion TEXT, appBuild TEXT, platformVersion TEXT, platformBuild TEXT"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = EnsureTable(*db,
                   NS_LITERAL_CSTRING("events"),
                   NS_LITERAL_CSTRING("timestamp INTEGER, id TEXT, name TEXT, version TEXT, action TEXT"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult EnsureTable(mozIStorageConnection *db, const nsACString &table, const nsACString &schema)
{
  nsresult rv;
  PRBool exists = false;
  rv = db->TableExists(table, &exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists)
    rv = db->CreateTable(PromiseFlatCString(table).get(),
                         PromiseFlatCString(schema).get());
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}
