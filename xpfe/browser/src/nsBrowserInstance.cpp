







































#include "nsBrowserInstance.h"


#include "nsIGenericFactory.h"


#include "nsIBaseWindow.h"
#include "nsIWebNavigationInfo.h"
#include "nsDocShellCID.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIHttpProtocolHandler.h"
#include "nsISHistory.h"
#include "nsIWebNavigation.h"



#include "nsIObserver.h"
#include "pratom.h"
#include "prprf.h"
#include "nsIComponentManager.h"
#include "nsCRT.h"

#include "nsIDOMDocument.h"
#include "nsPIDOMWindow.h"

#include "nsIContentViewer.h"
#include "nsIContentViewerEdit.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWindowWatcher.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsThreadUtils.h"

#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefLocalizedString.h"
#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsIIOService.h"
#include "nsIWidget.h"
#include "plstr.h"

#include "nsIAppStartup.h"

#include "nsIObserverService.h"

#include "nsILocalFile.h"
#include "nsDirectoryServiceDefs.h"

#include "nsNetUtil.h"


#include "nsIProxyObjectManager.h" 

#include "nsXPFEComponentsCID.h"



#if defined(DEBUG) || defined(NS_BUILD_REFCNT_LOGGING) || defined(MOZ_JPROF)
#define ENABLE_PAGE_CYCLER
#endif

#ifdef DEBUG                                                           
static int APP_DEBUG = 0; 
#else                                                                  
#define APP_DEBUG 0                                                    
#endif                                                                 


#define PREF_HOMEPAGE_OVERRIDE_URL "startup.homepage_override_url"
#define PREF_HOMEPAGE_OVERRIDE_MSTONE "browser.startup.homepage_override.mstone"
#define PREF_BROWSER_STARTUP_PAGE "browser.startup.page"
#define PREF_BROWSER_STARTUP_HOMEPAGE "browser.startup.homepage"

const char *kIgnoreOverrideMilestone = "ignore";





#ifdef ENABLE_PAGE_CYCLER
#include "nsITimer.h"

static void TimesUp(nsITimer *aTimer, void *aClosure);
  

class PageCycler : public nsIObserver {
public:
  NS_DECL_ISUPPORTS

  struct PageCyclerEvent : public nsRunnable {
    PageCyclerEvent(PageCycler *pc) : mPageCycler(pc) {}
    NS_IMETHOD Run() {
      mPageCycler->DoCycle();
      return NS_OK;
    }
    nsRefPtr<PageCycler> mPageCycler;
  };

  PageCycler(nsBrowserInstance* appCore, const char *aTimeoutValue = nsnull, const char *aWaitValue = nsnull)
    : mAppCore(appCore), mBuffer(nsnull), mCursor(nsnull), mTimeoutValue(0), mWaitValue(1 ) { 
    NS_ADDREF(mAppCore);
    if (aTimeoutValue){
      mTimeoutValue = atol(aTimeoutValue);
    }
    if (aWaitValue) {
      mWaitValue = atol(aWaitValue);
    }
  }

  virtual ~PageCycler() { 
    if (mBuffer) delete[] mBuffer;
    NS_RELEASE(mAppCore);
  }

  nsresult Init(const char* nativePath) {
    nsresult rv;
    if (!mFile) {
      nsCOMPtr<nsIFile> aFile;
      rv = NS_GetSpecialDirectory(NS_XPCOM_CURRENT_PROCESS_DIR,
                                  getter_AddRefs(aFile));
      if (NS_FAILED(rv)) return rv;

      mFile = do_QueryInterface(aFile);
      NS_ASSERTION(mFile, "QI to nsILocalFile should not fail");
      rv = mFile->AppendRelativeNativePath(nsDependentCString(nativePath));
      if (NS_FAILED(rv)) return rv;
    }

    nsCOMPtr<nsIInputStream> inStr;
    rv = NS_NewLocalFileInputStream(getter_AddRefs(inStr), mFile);
    if (NS_FAILED(rv)) return rv;

    PRUint32 avail;
    rv = inStr->Available(&avail);
    if (NS_FAILED(rv)) return rv;

    mBuffer = new char[avail + 1];
    if (mBuffer == nsnull)
      return NS_ERROR_OUT_OF_MEMORY;
    PRUint32 amt;
    rv = inStr->Read(mBuffer, avail, &amt);
    if (NS_FAILED(rv)) return rv;
    NS_ASSERTION(amt == avail, "Didn't get the whole file.");
    mBuffer[avail] = '\0';
    mCursor = mBuffer;

    nsCOMPtr<nsIObserverService> obsServ = 
             do_GetService("@mozilla.org/observer-service;1", &rv);
    if (NS_FAILED(rv)) return rv;
    rv = obsServ->AddObserver(this, "EndDocumentLoad", PR_FALSE );
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to add self to observer service");
    return rv; 
  }

  nsresult GetNextURL(nsString& result) {
    result.AssignWithConversion(mCursor);
    PRInt32 pos = result.Find(NS_LINEBREAK);
    if (pos > 0) {
      result.Truncate(pos);
      mLastRequest.Assign(result);
      mCursor += pos + NS_LINEBREAK_LEN;
      return NS_OK;
    }
    else if ( !result.IsEmpty() ) {
      
      mCursor += result.Length(); 
      mLastRequest.Assign(result);
      return NS_OK;
    }
    else {
      
      nsresult rv;
      
      StopTimer();

      
      
      nsCOMPtr<nsIAppStartup> appStartup = 
               do_GetService(NS_APPSTARTUP_CONTRACTID, &rv);
      if(NS_FAILED(rv)) return rv;
      nsCOMPtr<nsIAppStartup> appStartupProxy;
      rv = NS_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                                NS_GET_IID(nsIAppStartup), appStartup,
                                NS_PROXY_ASYNC | NS_PROXY_ALWAYS,
                                getter_AddRefs(appStartupProxy));

      (void)appStartupProxy->Quit(nsIAppStartup::eAttemptQuit);
      return NS_ERROR_FAILURE;
    }
  }

  NS_IMETHOD Observe(nsISupports* aSubject, 
                     const char* aTopic,
                     const PRUnichar* someData) {
    nsresult rv = NS_OK;
    nsString data(someData);
    if (data.Find(mLastRequest) == 0) {
      char* dataStr = ToNewCString(data);
      printf("########## PageCycler loaded (%d ms): %s\n", 
             PR_IntervalToMilliseconds(PR_IntervalNow() - mIntervalTime), 
             dataStr);
      NS_Free(dataStr);

      nsAutoString url;
      rv = GetNextURL(url);
      if (NS_SUCCEEDED(rv)) {
        
        StopTimer();
        if (aSubject == this){
          
          
          nsCOMPtr<nsIDocShell> docShell;
          mAppCore->GetContentAreaDocShell(getter_AddRefs(docShell));

          nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(docShell));
          if(webNav)
            webNav->Stop(nsIWebNavigation::STOP_ALL);
        }

        
        
        
        
        nsCOMPtr<nsIRunnable> ev = new PageCyclerEvent(this);
        if (ev)
          rv = NS_DispatchToCurrentThread(ev);

        if (NS_FAILED(rv)) {
          printf("######### PageCycler couldn't asynchronously load: %s\n", NS_ConvertUTF16toUTF8(mLastRequest).get());
        }
      }
    }
    else {
      char* dataStr = ToNewCString(data);
      printf("########## PageCycler possible failure for: %s\n", dataStr);
      NS_Free(dataStr);
    }
    return rv;
  }

  void DoCycle()
  {
    
    const PRUnichar* url = mLastRequest.get();
    printf("########## PageCycler starting: %s\n", NS_ConvertUTF16toUTF8(url).get());

    mIntervalTime = PR_IntervalNow();
    mAppCore->LoadUrl(url);

    
    StartTimer();
  }

  const nsAutoString &GetLastRequest( void )
  { 
    return mLastRequest; 
  }

  
  
  
  nsresult StartTimer(void)
  {
    nsresult rv=NS_OK;
    if (mTimeoutValue > 0){
      mShutdownTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
      NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create timer for PageCycler...");
      if (NS_SUCCEEDED(rv) && mShutdownTimer){
        mShutdownTimer->InitWithFuncCallback(TimesUp, (void *)this, mTimeoutValue*1000,
                                             nsITimer::TYPE_ONE_SHOT);
      }
    }
    return rv;
  }

  
  nsresult StopTimer(void)
  {
    if (mShutdownTimer){
      mShutdownTimer->Cancel();
    }
    return NS_OK;
  }

protected:
  nsBrowserInstance*    mAppCore;
  nsCOMPtr<nsILocalFile> mFile;
  char*                 mBuffer;
  char*                 mCursor;
  nsAutoString          mLastRequest;
  nsCOMPtr<nsITimer>    mShutdownTimer;
  PRUint32              mTimeoutValue;
  PRUint32              mWaitValue;
  PRIntervalTime        mIntervalTime;
};

NS_IMPL_ISUPPORTS1(PageCycler, nsIObserver)











void TimesUp(nsITimer *aTimer, void *aClosure)
{
  if(aClosure){
    PageCycler *pCycler = (PageCycler *)aClosure;
    pCycler->StopTimer();
    const nsAutoString &url  = pCycler->GetLastRequest();
    fprintf(stderr,"########## PageCycler Timeout on URL: %s\n",
            NS_LossyConvertUTF16toASCII(url).get());
    pCycler->Observe( pCycler, nsnull, url.get() );
  }
}

#endif 

PRBool nsBrowserInstance::sCmdLineURLUsed = PR_FALSE;





nsBrowserInstance::nsBrowserInstance() : mIsClosed(PR_FALSE)
{
  mDOMWindow            = nsnull;
  mContentAreaDocShellWeak  = nsnull;
}

nsBrowserInstance::~nsBrowserInstance()
{
  Close();
}

void
nsBrowserInstance::ReinitializeContentVariables()
{
  NS_ASSERTION(mDOMWindow,"Reinitializing Content Variables without a window will cause a crash. see Bugzilla Bug 46454");
  if (!mDOMWindow)
    return;

  nsCOMPtr<nsIDOMWindow> contentWindow;
  mDOMWindow->GetContent(getter_AddRefs(contentWindow));

  nsCOMPtr<nsPIDOMWindow> pcontentWindow(do_QueryInterface(contentWindow));

  if (pcontentWindow) {
    nsIDocShell *docShell = pcontentWindow->GetDocShell();

    mContentAreaDocShellWeak = do_GetWeakReference(docShell); 

    if (APP_DEBUG) {
      nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(docShell));
      if (docShellAsItem) {
        nsXPIDLString name;
        docShellAsItem->GetName(getter_Copies(name));
        printf("Attaching to Content DocShell [%s]\n", NS_LossyConvertUTF16toASCII(name).get());
      }
    }
  }
}

nsresult nsBrowserInstance::GetContentAreaDocShell(nsIDocShell** outDocShell)
{
  nsCOMPtr<nsIDocShell> docShell(do_QueryReferent(mContentAreaDocShellWeak));
  if (!mIsClosed && docShell) {
    
    nsCOMPtr<nsIBaseWindow> hack = do_QueryInterface(docShell);
    if (hack) {
      nsCOMPtr<nsIWidget> parent;
      hack->GetParentWidget(getter_AddRefs(parent));
      if (!parent)
        
        docShell = 0;
    }
  }
  if (!mIsClosed && !docShell)
    ReinitializeContentVariables();

  docShell = do_QueryReferent(mContentAreaDocShellWeak);
  *outDocShell = docShell;
  NS_IF_ADDREF(*outDocShell);
  return NS_OK;
}
    




NS_IMPL_ADDREF(nsBrowserInstance)
NS_IMPL_RELEASE(nsBrowserInstance)

NS_INTERFACE_MAP_BEGIN(nsBrowserInstance)
   NS_INTERFACE_MAP_ENTRY(nsIBrowserInstance)
   NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIBrowserInstance)
NS_INTERFACE_MAP_END





nsresult
nsBrowserInstance::LoadUrl(const PRUnichar * urlToLoad)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIDocShell> docShell;
  GetContentAreaDocShell(getter_AddRefs(docShell));

  
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(docShell));
    
  
  rv = webNav->LoadURI( urlToLoad,                          
                        nsIWebNavigation::LOAD_FLAGS_NONE,  
                        nsnull,                             
                        nsnull,                             
                        nsnull );                           

  return rv;
}

NS_IMETHODIMP    
nsBrowserInstance::SetCmdLineURLUsed(PRBool aCmdLineURLUsed)
{
  sCmdLineURLUsed = aCmdLineURLUsed;
  return NS_OK;
}

NS_IMETHODIMP    
nsBrowserInstance::GetCmdLineURLUsed(PRBool* aCmdLineURLUsed)
{
  NS_ASSERTION(aCmdLineURLUsed, "aCmdLineURLUsed can't be null");
  if (!aCmdLineURLUsed)
    return NS_ERROR_NULL_POINTER;

  *aCmdLineURLUsed = sCmdLineURLUsed;
  return NS_OK;
}

NS_IMETHODIMP    
nsBrowserInstance::StartPageCycler(PRBool* aIsPageCycling)
{
  *aIsPageCycling = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP    
nsBrowserInstance::SetWebShellWindow(nsIDOMWindowInternal* aWin)
{
  NS_ENSURE_ARG(aWin);
  mDOMWindow = aWin;

  nsCOMPtr<nsPIDOMWindow> win( do_QueryInterface(aWin) );
  if (!win) {
    return NS_ERROR_FAILURE;
  }

  if (APP_DEBUG) {
    nsCOMPtr<nsIDocShellTreeItem> docShellAsItem =
      do_QueryInterface(win->GetDocShell());

    if (docShellAsItem) {
      
      
      nsXPIDLString name;
      docShellAsItem->GetName(getter_Copies(name));
      printf("Attaching to WebShellWindow[%s]\n", NS_LossyConvertUTF16toASCII(name).get());
    }
  }

  ReinitializeContentVariables();

  return NS_OK;
}

NS_IMETHODIMP    
nsBrowserInstance::Close()
{ 
  
  if (mIsClosed) 
    return NS_OK;
  else
    mIsClosed = PR_TRUE;

  return NS_OK;
}

