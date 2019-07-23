








































#include "nsAppStartup.h"

#include "nsIAppShellService.h"
#include "nsICloseAllWindows.h"
#include "nsICmdLineService.h"
#include "nsIDOMWindowInternal.h"
#include "nsIInterfaceRequestor.h"
#include "nsILocalFile.h"
#include "nsIObserverService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIProfileChangeStatus.h"
#include "nsIProfileInternal.h"
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

#include "prprf.h"
#include "nsCRT.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsWidgetsCID.h"
#include "nsAppShellCID.h"
#include "nsXPFEComponentsCID.h"
#include "nsEmbedCID.h"

class nsAppStartupExitEvent : public nsRunnable
{
public:
  nsAppStartupExitEvent(nsAppStartup *service)
    : mService(service) {}

  NS_IMETHOD Run() {
    
    mService->mAppShell->Exit();

    
    mService->mShuttingDown = PR_FALSE;
    return NS_OK;
  }

private:
  nsRefPtr<nsAppStartup> mService;
};

NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);





nsAppStartup::nsAppStartup() :
  mConsiderQuitStopper(0),
  mShuttingDown(PR_FALSE),
  mAttemptingQuit(PR_FALSE)
{ }






NS_IMPL_ISUPPORTS5(nsAppStartup,
                   nsIAppStartup,
                   nsIWindowCreator,
                   nsIWindowCreator2,
                   nsIObserver,
                   nsISupportsWeakReference)






NS_IMETHODIMP
nsAppStartup::Initialize(nsISupports *aNativeAppSupportOrSplashScreen)
{
  nsresult rv;

  
  mNativeAppSupport = do_QueryInterface(aNativeAppSupportOrSplashScreen);

  
  if (!mNativeAppSupport)
    mSplashScreen = do_QueryInterface(aNativeAppSupportOrSplashScreen);

  
  mAppShell = do_GetService(kAppShellCID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIObserverService> os
    (do_GetService("@mozilla.org/observer-service;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  os->AddObserver(this, "skin-selected", PR_TRUE);
  os->AddObserver(this, "locale-selected", PR_TRUE);
  os->AddObserver(this, "xpinstall-restart", PR_TRUE);
  os->AddObserver(this, "profile-change-teardown", PR_TRUE);
  os->AddObserver(this, "profile-initial-state", PR_TRUE);
  os->AddObserver(this, "xul-window-registered", PR_TRUE);
  os->AddObserver(this, "xul-window-destroyed", PR_TRUE);
  os->AddObserver(this, "xul-window-visible", PR_TRUE);

  return NS_OK;
}


NS_IMETHODIMP
nsAppStartup::CreateHiddenWindow()
{
  nsCOMPtr<nsIAppShellService> appShellService
    (do_GetService(NS_APPSHELLSERVICE_CONTRACTID));
  NS_ENSURE_TRUE(appShellService, NS_ERROR_FAILURE);

  return appShellService->CreateHiddenWindow(mAppShell);
}


NS_IMETHODIMP
nsAppStartup::DoProfileStartup(nsICmdLineService *aCmdLineService,
                               PRBool canInteract)
{
  nsresult rv;

  nsCOMPtr<nsIProfileInternal> profileMgr
    (do_GetService(NS_PROFILE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv,rv);

  EnterLastWindowClosingSurvivalArea();

  
  rv = profileMgr->StartupWithArgs(aCmdLineService, canInteract);
  if (!canInteract && rv == NS_ERROR_PROFILE_REQUIRES_INTERACTION) {
    NS_WARNING("nsIProfileInternal::StartupWithArgs returned NS_ERROR_PROFILE_REQUIRES_INTERACTION");       
    rv = NS_OK;
  }

  if (NS_SUCCEEDED(rv)) {
    rv = CheckAndRemigrateDefunctProfile();
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to check and remigrate profile");
    rv = NS_OK;
  }

  ExitLastWindowClosingSurvivalArea();

  
  if (mShuttingDown)
    return NS_ERROR_FAILURE;

  return rv;
}


NS_IMETHODIMP
nsAppStartup::GetNativeAppSupport(nsINativeAppSupport **aResult)
{
  NS_PRECONDITION(aResult, "Null out param");

  if (!mNativeAppSupport)
    return NS_ERROR_FAILURE;

  NS_ADDREF(*aResult = mNativeAppSupport);
  return NS_OK;
}


NS_IMETHODIMP
nsAppStartup::Run(void)
{
  return mAppShell->Run();
}


NS_IMETHODIMP
nsAppStartup::Quit(PRUint32 aFerocity)
{
  
  
  
  nsresult rv = NS_OK;
  PRBool postedExitEvent = PR_FALSE;

  if (mShuttingDown)
    return NS_OK;

  


  if (aFerocity == eForceQuit) {
    NS_WARNING("attempted to force quit");
    
  }

  mShuttingDown = PR_TRUE;

  nsCOMPtr<nsIWindowMediator> mediator
    (do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));

  if (aFerocity == eConsiderQuit && mConsiderQuitStopper == 0) {
    

    PRBool windowsRemain = PR_TRUE;

    if (mediator) {
      nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
      mediator->GetEnumerator(nsnull, getter_AddRefs(windowEnumerator));
      if (windowEnumerator)
        windowEnumerator->HasMoreElements(&windowsRemain);
    }
    if (!windowsRemain) {
      aFerocity = eAttemptQuit;

      
      if (mNativeAppSupport) {
        PRBool serverMode = PR_FALSE;
        mNativeAppSupport->GetIsServerMode(&serverMode);
        if (serverMode) {
          
          mShuttingDown = PR_FALSE;
          mNativeAppSupport->OnLastWindowClosing();
          return NS_OK;
        }
      }
    }
  }

  



  if (aFerocity == eAttemptQuit || aFerocity == eForceQuit) {

    AttemptingQuit(PR_TRUE);

    



    if (mediator) {
      nsCOMPtr<nsISimpleEnumerator> windowEnumerator;

      mediator->GetEnumerator(nsnull, getter_AddRefs(windowEnumerator));

      if (windowEnumerator) {

        while (1) {
          PRBool more;
          if (NS_FAILED(rv = windowEnumerator->HasMoreElements(&more)) || !more)
            break;

          nsCOMPtr<nsISupports> isupports;
          rv = windowEnumerator->GetNext(getter_AddRefs(isupports));
          if (NS_FAILED(rv))
            break;

          nsCOMPtr<nsIDOMWindowInternal> window = do_QueryInterface(isupports);
          NS_ASSERTION(window, "not an nsIDOMWindowInternal");
          if (!window)
            continue;

          window->Close();
        }
      }

      if (aFerocity == eAttemptQuit) {

        aFerocity = eForceQuit; 

        





        mediator->GetEnumerator(nsnull, getter_AddRefs(windowEnumerator));
        if (windowEnumerator) {
          PRBool more;
          while (windowEnumerator->HasMoreElements(&more), more) {
            

            aFerocity = eAttemptQuit;
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

  if (aFerocity == eForceQuit) {
    

    
    
    nsCOMPtr<nsIObserverService> obsService = do_GetService("@mozilla.org/observer-service;1", &rv);
    obsService->NotifyObservers(nsnull, "quit-application", nsnull);

    
    
    if (mNativeAppSupport) {
      mNativeAppSupport->Quit();
      mNativeAppSupport = 0;
    }

    nsCOMPtr<nsIAppShellService> appShellService
      (do_GetService(NS_APPSHELLSERVICE_CONTRACTID));
    if (appShellService)
      appShellService->DestroyHiddenWindow();
    
    
    
    
    nsCOMPtr<nsIRunnable> event = new nsAppStartupExitEvent(this);
    if (event) {
      rv = NS_DispatchToCurrentThread(event);
      if (NS_SUCCEEDED(rv)) {
        postedExitEvent = PR_TRUE;
      }
    }
    else {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
  }

  
  
  if (!postedExitEvent)
    mShuttingDown = PR_FALSE;
  return rv;
}





void
nsAppStartup::AttemptingQuit(PRBool aAttempt)
{
#if defined(XP_MAC) || defined(XP_MACOSX)
  if (aAttempt) {
    
    if (!mAttemptingQuit)
      ExitLastWindowClosingSurvivalArea();
    mAttemptingQuit = PR_TRUE;
  } else {
    
    if (mAttemptingQuit)
      EnterLastWindowClosingSurvivalArea();
    mAttemptingQuit = PR_FALSE;
  }
#else
  mAttemptingQuit = aAttempt;
#endif
}

nsresult
nsAppStartup::CheckAndRemigrateDefunctProfile()
{
  nsresult rv;

  nsCOMPtr<nsIPrefBranch> prefBranch;
  nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);
  rv = prefs->GetBranch(nsnull, getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv,rv);

  PRInt32 secondsBeforeDefunct;
  rv = prefBranch->GetIntPref("profile.seconds_until_defunct", &secondsBeforeDefunct);
  NS_ENSURE_SUCCESS(rv,rv);

  
  
  
  if (secondsBeforeDefunct == -1)
    return NS_OK;

  
  
  
  PRInt64 oneThousand = LL_INIT(0, 1000);
  
  PRInt64 defunctInterval;
  
  LL_I2L(defunctInterval, secondsBeforeDefunct);
  
  LL_MUL(defunctInterval, defunctInterval, oneThousand);
        
  nsCOMPtr<nsIProfileInternal> profileMgr(do_GetService(NS_PROFILE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv,rv);

  nsXPIDLString profileName;
  PRInt64 lastModTime;
  profileMgr->GetCurrentProfile(getter_Copies(profileName));
  rv = profileMgr->GetProfileLastModTime(profileName.get(), &lastModTime);
  NS_ENSURE_SUCCESS(rv,rv);

  
  PRInt64 nowInMilliSecs = PR_Now(); 
  LL_DIV(nowInMilliSecs, nowInMilliSecs, oneThousand);
  
  
  PRInt64 defunctIntervalAgo;
  LL_SUB(defunctIntervalAgo, nowInMilliSecs, defunctInterval);

  
  
  if (LL_CMP(lastModTime, >, defunctIntervalAgo))
    return NS_OK;
  
  nsCOMPtr<nsILocalFile> origProfileDir;
  rv = profileMgr->GetOriginalProfileDir(profileName, getter_AddRefs(origProfileDir));
  
  
  
  if (NS_FAILED(rv))
    return NS_OK;
  
  
  
  nsCOMPtr<nsISimpleEnumerator> dirEnum;
  rv = origProfileDir->GetDirectoryEntries(getter_AddRefs(dirEnum));
  NS_ENSURE_SUCCESS(rv,rv);
  
  PRBool promptForRemigration = PR_FALSE;
  PRBool hasMore;
  while (NS_SUCCEEDED(dirEnum->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsILocalFile> currElem;
    rv = dirEnum->GetNext(getter_AddRefs(currElem));
    NS_ENSURE_SUCCESS(rv,rv);
    
    PRInt64 currElemModTime;
    rv = currElem->GetLastModifiedTime(&currElemModTime);
    NS_ENSURE_SUCCESS(rv,rv);
    
    
    if (LL_CMP(currElemModTime, >, lastModTime)) {
      promptForRemigration = PR_TRUE;
      break;
    }
  }
  
  
  if (!promptForRemigration)
    return NS_OK;
 
  nsCOMPtr<nsIStringBundleService> stringBundleService(do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsIStringBundle> migrationBundle, brandBundle;
  rv = stringBundleService->CreateBundle("chrome://communicator/locale/profile/migration.properties", getter_AddRefs(migrationBundle));
  NS_ENSURE_SUCCESS(rv,rv);

  rv = stringBundleService->CreateBundle("chrome://branding/locale/brand.properties", getter_AddRefs(brandBundle));
  NS_ENSURE_SUCCESS(rv,rv);

  nsXPIDLString brandName;
  rv = brandBundle->GetStringFromName(NS_LITERAL_STRING("brandShortName").get(), getter_Copies(brandName));
  NS_ENSURE_SUCCESS(rv,rv);
 
  nsXPIDLString dialogText;
  rv = migrationBundle->GetStringFromName(NS_LITERAL_STRING("confirmRemigration").get(), getter_Copies(dialogText));
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsIPromptService> promptService(do_GetService(NS_PROMPTSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv,rv);
  PRInt32 buttonPressed;
  rv = promptService->ConfirmEx(nsnull, brandName.get(),
    dialogText.get(), nsIPromptService::STD_YES_NO_BUTTONS,
    nsnull, nsnull, nsnull, nsnull, nsnull, &buttonPressed);
  NS_ENSURE_SUCCESS(rv,rv);
  
  if (buttonPressed == 0) {
    
    profileMgr->ShutDownCurrentProfile(nsIProfile::SHUTDOWN_PERSIST);
    
    rv = profileMgr->RemigrateProfile(profileName.get());
    NS_ASSERTION(NS_SUCCEEDED(rv), "Remigration of profile failed.");
    
    profileMgr->SetCurrentProfile(profileName.get());
  }
  return NS_OK;
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
  return NS_OK;
}


NS_IMETHODIMP
nsAppStartup::HideSplashScreen()
{
  
  if ( mNativeAppSupport ) {
    mNativeAppSupport->HideSplashScreen();
  } else if ( mSplashScreen ) {
    mSplashScreen->Hide();
  }
  return NS_OK;
}


NS_IMETHODIMP
nsAppStartup::CreateStartupState(PRInt32 aWindowWidth, PRInt32 aWindowHeight,
                                 PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  nsresult rv;
  
  
  nsCOMPtr<nsINativeAppSupport> nativeApp;
  rv = GetNativeAppSupport(getter_AddRefs(nativeApp));
  if (NS_SUCCEEDED(rv)) {
      PRBool isServerMode = PR_FALSE;
      nativeApp->GetIsServerMode(&isServerMode);
      if (isServerMode) {
          nativeApp->StartServerMode();
      }
      PRBool shouldShowUI = PR_TRUE;
      nativeApp->GetShouldShowUI(&shouldShowUI);
      if (!shouldShowUI) {
          return NS_OK;
      }
  }  

  nsCOMPtr<nsIPrefService> prefService(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (!prefService)
    return NS_ERROR_FAILURE;
  nsCOMPtr<nsIPrefBranch> startupBranch;
  prefService->GetBranch(PREF_STARTUP_PREFIX, getter_AddRefs(startupBranch));
  if (!startupBranch)
    return NS_ERROR_FAILURE;
  
  PRUint32 childCount;
  char **childArray = nsnull;
  rv = startupBranch->GetChildList("", &childCount, &childArray);
  if (NS_FAILED(rv))
    return rv;
    
  for (PRUint32 i = 0; i < childCount; i++) {
    PRBool prefValue;
    rv = startupBranch->GetBoolPref(childArray[i], &prefValue);
    if (NS_SUCCEEDED(rv) && prefValue) {
      PRBool windowOpened;
      rv = LaunchTask(childArray[i], aWindowHeight, aWindowWidth, &windowOpened);
      if (NS_SUCCEEDED(rv) && windowOpened)
        *_retval = PR_TRUE;
    }
  }
  
  NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(childCount, childArray);
  
  return NS_OK;
}


NS_IMETHODIMP
nsAppStartup::Ensure1Window(nsICmdLineService *aCmdLineService)
{
  nsresult rv;
 
  nsCOMPtr<nsIWindowMediator> windowMediator(do_GetService(NS_WINDOWMEDIATOR_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
  if (NS_SUCCEEDED(windowMediator->GetEnumerator(nsnull, getter_AddRefs(windowEnumerator))))
  {
    PRBool more;
    windowEnumerator->HasMoreElements(&more);
    if (!more)
    {
      
      PRInt32 height = nsIAppShellService::SIZE_TO_CONTENT;
      PRInt32 width  = nsIAppShellService::SIZE_TO_CONTENT;
				
      
      nsXPIDLCString tempString;
      rv = aCmdLineService->GetCmdLineValue("-width", getter_Copies(tempString));
      if (NS_SUCCEEDED(rv) && !tempString.IsEmpty())
        PR_sscanf(tempString.get(), "%d", &width);

      
      rv = aCmdLineService->GetCmdLineValue("-height", getter_Copies(tempString));
      if (NS_SUCCEEDED(rv) && !tempString.IsEmpty())
        PR_sscanf(tempString.get(), "%d", &height);

      rv = OpenBrowserWindow(height, width);
    }
  }
  return rv;
}

nsresult
nsAppStartup::LaunchTask(const char *aParam, PRInt32 height, PRInt32 width, PRBool *windowOpened)
{
  nsresult rv;

  nsCOMPtr <nsICmdLineService> cmdLine =
    do_GetService(NS_COMMANDLINESERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr <nsICmdLineHandler> handler;
  rv = cmdLine->GetHandlerForParam(aParam, getter_AddRefs(handler));
  if (NS_FAILED(rv)) return rv;

  nsXPIDLCString chromeUrlForTask;
  rv = handler->GetChromeUrlForTask(getter_Copies(chromeUrlForTask));
  if (NS_FAILED(rv)) return rv;

  PRBool handlesArgs = PR_FALSE;
  rv = handler->GetHandlesArgs(&handlesArgs);
  if (handlesArgs) {
    nsXPIDLString defaultArgs;
    rv = handler->GetDefaultArgs(getter_Copies(defaultArgs));
    if (NS_FAILED(rv)) return rv;
    rv = OpenWindow(chromeUrlForTask, defaultArgs,
                    nsIAppShellService::SIZE_TO_CONTENT,
                    nsIAppShellService::SIZE_TO_CONTENT);
  }
  else {
    rv = OpenWindow(chromeUrlForTask, EmptyString(), width, height);
  }
  
  
  if (NS_SUCCEEDED(rv)) {
    *windowOpened = PR_TRUE;
  }

  return rv;
}

nsresult
nsAppStartup::OpenWindow(const nsAFlatCString& aChromeURL,
                         const nsAFlatString& aAppArgs,
                         PRInt32 aWidth, PRInt32 aHeight)
{
  nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
  nsCOMPtr<nsISupportsString> sarg(do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID));
  if (!wwatch || !sarg)
    return NS_ERROR_FAILURE;

  

  
  
  
  
  
  
  nsCOMPtr<nsINativeAppSupport> nativeApp;
  if (NS_SUCCEEDED(GetNativeAppSupport(getter_AddRefs(nativeApp))))
  {
    nsCOMPtr <nsICmdLineService> cmdLine =
      do_GetService(NS_COMMANDLINESERVICE_CONTRACTID);

    if (cmdLine) {
      
      
      
      
      if (NS_FAILED(nativeApp->EnsureProfile(cmdLine)))
        return NS_ERROR_NOT_INITIALIZED;
    }
  }

  sarg->SetData(aAppArgs);

  nsCAutoString features("chrome,dialog=no,all");
  if (aHeight != nsIAppShellService::SIZE_TO_CONTENT) {
    features.Append(",height=");
    features.AppendInt(aHeight);
  }
  if (aWidth != nsIAppShellService::SIZE_TO_CONTENT) {
    features.Append(",width=");
    features.AppendInt(aWidth);
  }

  nsCOMPtr<nsIDOMWindow> newWindow;
  return wwatch->OpenWindow(0, aChromeURL.get(), "_blank",
                            features.get(), sarg,
                            getter_AddRefs(newWindow));
}


nsresult
nsAppStartup::OpenBrowserWindow(PRInt32 height, PRInt32 width)
{
  nsresult rv;
  nsCOMPtr<nsICmdLineHandler> handler(
    do_GetService("@mozilla.org/commandlinehandler/general-startup;1?type=browser", &rv));
  if (NS_FAILED(rv)) return rv;

  nsXPIDLCString chromeUrlForTask;
  rv = handler->GetChromeUrlForTask(getter_Copies(chromeUrlForTask));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr <nsICmdLineService> cmdLine = do_GetService(NS_COMMANDLINESERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsXPIDLCString urlToLoad;
  rv = cmdLine->GetURLToLoad(getter_Copies(urlToLoad));
  if (!urlToLoad.IsEmpty()) {

#ifdef DEBUG_CMD_LINE
    printf("url to load: %s\n", urlToLoad.get());
#endif 

    nsAutoString url; 
    
    rv = NS_CopyNativeToUnicode(nsDependentCString(urlToLoad), url);
    if (NS_FAILED(rv)) {
      NS_ERROR("Failed to convert commandline url to unicode");
      return rv;
    }
    rv = OpenWindow(chromeUrlForTask, url, width, height);

  } else {

    nsXPIDLString defaultArgs;
    rv = handler->GetDefaultArgs(getter_Copies(defaultArgs));
    if (NS_FAILED(rv)) return rv;

#ifdef DEBUG_CMD_LINE
    printf("default args: %s\n", NS_ConvertUTF16toUTF8(defaultArgs).get());
#endif 

    rv = OpenWindow(chromeUrlForTask, defaultArgs, width, height);
  }

  return rv;
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
  if (!strcmp(aTopic, "skin-selected") ||
      !strcmp(aTopic, "locale-selected") ||
      !strcmp(aTopic, "xpinstall-restart")) {
    if (mNativeAppSupport)
      mNativeAppSupport->SetIsServerMode(PR_FALSE);
  } else if (!strcmp(aTopic, "profile-change-teardown")) {
    nsresult rv;
    EnterLastWindowClosingSurvivalArea();
    
    
    nsCOMPtr<nsICloseAllWindows> closer =
            do_CreateInstance("@mozilla.org/appshell/closeallwindows;1", &rv);
    NS_ASSERTION(closer, "Failed to create nsICloseAllWindows impl.");
    PRBool proceedWithSwitch = PR_FALSE;
    if (closer)
      rv = closer->CloseAll(PR_TRUE, &proceedWithSwitch);
    if (NS_FAILED(rv) || !proceedWithSwitch) {
      nsCOMPtr<nsIProfileChangeStatus> changeStatus(do_QueryInterface(aSubject));
      if (changeStatus)
        changeStatus->VetoChange();
    }
    ExitLastWindowClosingSurvivalArea();
  } else if (!strcmp(aTopic, "profile-initial-state")) {
    if (nsDependentString(aData).EqualsLiteral("switch")) {
      
      PRBool openedWindow;
      CreateStartupState(nsIAppShellService::SIZE_TO_CONTENT,
                         nsIAppShellService::SIZE_TO_CONTENT, &openedWindow);
      if (!openedWindow)
        OpenBrowserWindow(nsIAppShellService::SIZE_TO_CONTENT,
                          nsIAppShellService::SIZE_TO_CONTENT);
    }
  } else if (!strcmp(aTopic, "xul-window-registered")) {
    AttemptingQuit(PR_FALSE);
  } else if (!strcmp(aTopic, "xul-window-destroyed")) {
    Quit(eConsiderQuit);
  } else if (!strcmp(aTopic, "xul-window-visible")) {
    
    static PRBool splashScreenGone = PR_FALSE;
    if(!splashScreenGone) {
      HideSplashScreen();
      splashScreenGone = PR_TRUE;
    }
  } else {
    NS_ERROR("Unexpected observer topic.");
  }

  return NS_OK;
}

