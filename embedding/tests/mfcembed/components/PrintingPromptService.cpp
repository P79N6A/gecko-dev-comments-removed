



































#include "stdafx.h"
#include "Dialogs.h"
#include "PrintingPromptService.h"
#include "nsCOMPtr.h"
#include "nsMemory.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsThreadUtils.h"
#include "nsIDOMWindow.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIFactory.h"
#include "nsIPrintingPromptService.h"
#include "nsIServiceManager.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWindowWatcher.h"
#include "nsIWebProgressListener.h"
#include "nsPrintProgressParams.h"
#include "nsPrintDialogUtil.h"
#include "PrintProgressDialog.h"

static HINSTANCE gInstance;





class ResourceState {
public:
  ResourceState() {
    mPreviousInstance = ::AfxGetResourceHandle();
    ::AfxSetResourceHandle(gInstance);
  }
  ~ResourceState() {
    ::AfxSetResourceHandle(mPreviousInstance);
  }
private:
  HINSTANCE mPreviousInstance;
};






class CPrintingPromptService: public nsIPrintingPromptService,
                              public nsIWebProgressListener {
public:
                 CPrintingPromptService();
  virtual       ~CPrintingPromptService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRINTINGPROMPTSERVICE
  NS_DECL_NSIWEBPROGRESSLISTENER

  void NotifyObserver();

private:
  PRBool FirePauseEvent();
  CWnd *CWndForDOMWindow(nsIDOMWindow *aWindow);

  nsCOMPtr<nsIWindowWatcher>       mWWatch;
  nsCOMPtr<nsIWebProgressListener> mWebProgressListener;
  nsCOMPtr<nsIObserver>            mObserver;
  CPrintProgressDialog* m_PPDlg;
};



NS_IMPL_ISUPPORTS2(CPrintingPromptService, nsIPrintingPromptService, nsIWebProgressListener)

CPrintingPromptService::CPrintingPromptService() :
  mWWatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID)),
  m_PPDlg(NULL)
{
}

CPrintingPromptService::~CPrintingPromptService() {
}

CWnd *
CPrintingPromptService::CWndForDOMWindow(nsIDOMWindow *aWindow)
{
  nsCOMPtr<nsIWebBrowserChrome> chrome;
  CWnd *val = 0;

  if (mWWatch) {
    nsCOMPtr<nsIDOMWindow> fosterParent;
    if (!aWindow) { 
      mWWatch->GetActiveWindow(getter_AddRefs(fosterParent));
      aWindow = fosterParent;
    }
    mWWatch->GetChromeForWindow(aWindow, getter_AddRefs(chrome));
  }

  if (chrome) {
    nsCOMPtr<nsIEmbeddingSiteWindow> site(do_QueryInterface(chrome));
    if (site) {
      HWND w;
      site->GetSiteWindow(reinterpret_cast<void **>(&w));
      val = CWnd::FromHandle(w);
    }
  }

  return val;
}





NS_IMETHODIMP 
CPrintingPromptService::ShowPrintDialog(nsIDOMWindow *parent, nsIWebBrowserPrint *webBrowserPrint, nsIPrintSettings *printSettings)
{
    

    CWnd* wnd = CWndForDOMWindow(parent);

    NS_ASSERTION(wnd && wnd->m_hWnd, "Couldn't get native window for PRint Dialog!");
    if (wnd && wnd->m_hWnd) {
      return NativeShowPrintDialog(wnd->m_hWnd, webBrowserPrint, printSettings);
    } else {
      return NS_ERROR_FAILURE;
    }
}


PRBool
CPrintingPromptService::FirePauseEvent()
{
  nsCOMPtr<nsIRunnable> event =
      NS_NEW_RUNNABLE_METHOD(CPrintingPromptService, this, NotifyObserver);
  return NS_SUCCEEDED(NS_DispatchToCurrentThread(event));
}


NS_IMETHODIMP 
CPrintingPromptService::ShowProgress(nsIDOMWindow*            parent, 
                                      nsIWebBrowserPrint*      webBrowserPrint,    
                                      nsIPrintSettings*        printSettings,      
                                      nsIObserver*             openDialogObserver, 
                                      PRBool                   isForPrinting,
                                      nsIWebProgressListener** webProgressListener,
                                      nsIPrintProgressParams** printProgressParams,
                                      PRBool*                  notifyOnOpen)
{
    NS_ENSURE_ARG(webProgressListener);
    NS_ENSURE_ARG(printProgressParams);
    NS_ENSURE_ARG(notifyOnOpen);

    ResourceState setState;
    nsresult rv;

    nsPrintProgressParams* prtProgressParams = new nsPrintProgressParams();
    rv = prtProgressParams->QueryInterface(NS_GET_IID(nsIPrintProgressParams), (void**)printProgressParams);
    NS_ENSURE_SUCCESS(rv, rv);

    mObserver = openDialogObserver;

    *notifyOnOpen = PR_FALSE;
    if (printProgressParams) 
    {
      CWnd *wnd = CWndForDOMWindow(parent);
      m_PPDlg = new CPrintProgressDialog(wnd, isForPrinting, *printProgressParams, webBrowserPrint, printSettings);
      m_PPDlg->Create(IDD_PRINT_PROGRESS_DIALOG);
      m_PPDlg->ShowWindow(SW_SHOW);
      m_PPDlg->UpdateWindow();

      *notifyOnOpen = FirePauseEvent();
    }

    *webProgressListener = NS_STATIC_CAST(nsIWebProgressListener*, this);
    NS_ADDREF(*webProgressListener);

    return rv;
}


NS_IMETHODIMP 
CPrintingPromptService::ShowPageSetup(nsIDOMWindow *parent, nsIPrintSettings *printSettings, nsIObserver *aObs)
{
    NS_ENSURE_ARG(printSettings);

    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP 
CPrintingPromptService::ShowPrinterProperties(nsIDOMWindow *parent, const PRUnichar *printerName, nsIPrintSettings *printSettings)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


void CPrintingPromptService::NotifyObserver()
{
  if (mObserver) {
    mObserver->Observe(nsnull, nsnull, nsnull);
  }
}






NS_IMETHODIMP 
CPrintingPromptService::OnStateChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 aStateFlags, nsresult aStatus)
{
    if (aStateFlags & STATE_START) 
    {
      if (m_PPDlg)
      {
        m_PPDlg->OnStartPrinting();
      }
    }

    if (aStateFlags & STATE_STOP) 
    {
      if (m_PPDlg)
      {
        m_PPDlg->OnProgressPrinting(100, 100);
        m_PPDlg->DestroyWindow();
        m_PPDlg = NULL;
      }
    }
    return NS_OK;
}


NS_IMETHODIMP 
CPrintingPromptService::OnProgressChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRInt32 aCurSelfProgress, PRInt32 aMaxSelfProgress, PRInt32 aCurTotalProgress, PRInt32 aMaxTotalProgress)
{
    if (m_PPDlg)
    {
      m_PPDlg->OnProgressPrinting(aCurTotalProgress, aMaxTotalProgress);
    }
    return NS_OK;
}


NS_IMETHODIMP 
CPrintingPromptService::OnLocationChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsIURI *location)
{
    return NS_OK;
}


NS_IMETHODIMP 
CPrintingPromptService::OnStatusChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsresult aStatus, const PRUnichar *aMessage)
{
    return NS_OK;
}


NS_IMETHODIMP 
CPrintingPromptService::OnSecurityChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 state)
{
    return NS_OK;
}


 




class CPrintingPromptServiceFactory : public nsIFactory {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFACTORY

  CPrintingPromptServiceFactory();
  virtual ~CPrintingPromptServiceFactory();
};



NS_IMPL_ISUPPORTS1(CPrintingPromptServiceFactory, nsIFactory)

CPrintingPromptServiceFactory::CPrintingPromptServiceFactory() {
}

CPrintingPromptServiceFactory::~CPrintingPromptServiceFactory() {
}

NS_IMETHODIMP CPrintingPromptServiceFactory::CreateInstance(nsISupports *aOuter, const nsIID & aIID, void **aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  
  *aResult = NULL;  
  CPrintingPromptService *inst = new CPrintingPromptService;    
  if (!inst)
    return NS_ERROR_OUT_OF_MEMORY;
    
  nsresult rv = inst->QueryInterface(aIID, aResult);
  if (rv != NS_OK) {  
    
    delete inst;  
  }  
    
  return rv;
}

NS_IMETHODIMP CPrintingPromptServiceFactory::LockFactory(PRBool lock)
{
  return NS_OK;
}



void InitPrintingPromptService(HINSTANCE instance) {

  gInstance = instance;
}

nsresult NS_NewPrintingPromptServiceFactory(nsIFactory** aFactory)
{
  NS_ENSURE_ARG_POINTER(aFactory);
  *aFactory = nsnull;
  
  CPrintingPromptServiceFactory *result = new CPrintingPromptServiceFactory;
  if (!result)
    return NS_ERROR_OUT_OF_MEMORY;
    
  NS_ADDREF(result);
  *aFactory = result;
  
  return NS_OK;
}
