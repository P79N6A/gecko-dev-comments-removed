




































#include "nsPrintingPromptService.h"

#include "nsIComponentManager.h"
#include "nsIDialogParamBlock.h"
#include "nsIDOMWindow.h"
#include "nsIServiceManager.h"
#include "nsISupportsUtils.h"
#include "nsISupportsArray.h"
#include "nsString.h"
#include "nsIPrintDialogService.h"


#include "nsPrintProgress.h"
#include "nsPrintProgressParams.h"

static const char *kPrintDialogURL         = "chrome://global/content/printdialog.xul";
static const char *kPrintProgressDialogURL = "chrome://global/content/printProgress.xul";
static const char *kPrtPrvProgressDialogURL = "chrome://global/content/printPreviewProgress.xul";
static const char *kPageSetupDialogURL     = "chrome://global/content/printPageSetup.xul";
static const char *kPrinterPropertiesURL   = "chrome://global/content/printjoboptions.xul";
 




class ParamBlock {

public:
    ParamBlock() 
    {
        mBlock = 0;
    }
    ~ParamBlock() 
    {
        NS_IF_RELEASE(mBlock);
    }
    nsresult Init() {
      return CallCreateInstance(NS_DIALOGPARAMBLOCK_CONTRACTID, &mBlock);
    }
    nsIDialogParamBlock * operator->() const { return mBlock; }
    operator nsIDialogParamBlock * const ()  { return mBlock; }

private:
    nsIDialogParamBlock *mBlock;
};





NS_IMPL_ISUPPORTS2(nsPrintingPromptService, nsIPrintingPromptService, nsIWebProgressListener)

nsPrintingPromptService::nsPrintingPromptService() 
{
}

nsPrintingPromptService::~nsPrintingPromptService() 
{
}

nsresult
nsPrintingPromptService::Init()
{
    nsresult rv;
    mWatcher = do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
    return rv;
}


NS_IMETHODIMP 
nsPrintingPromptService::ShowPrintDialog(nsIDOMWindow *parent, nsIWebBrowserPrint *webBrowserPrint, nsIPrintSettings *printSettings)
{
    NS_ENSURE_ARG(webBrowserPrint);
    NS_ENSURE_ARG(printSettings);

    
    nsCOMPtr<nsIPrintDialogService> dlgPrint(do_GetService(
                                             NS_PRINTDIALOGSERVICE_CONTRACTID));
    if (dlgPrint)
      return dlgPrint->Show(parent, printSettings, webBrowserPrint);

    
    ParamBlock block;
    nsresult rv = block.Init();
    if (NS_FAILED(rv))
      return rv;

    block->SetInt(0, 0);
    return DoDialog(parent, block, webBrowserPrint, printSettings, kPrintDialogURL);
}


NS_IMETHODIMP 
nsPrintingPromptService::ShowProgress(nsIDOMWindow*            parent, 
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

    *notifyOnOpen = PR_FALSE;

    nsPrintProgress* prtProgress = new nsPrintProgress(printSettings);
    nsresult rv = prtProgress->QueryInterface(NS_GET_IID(nsIPrintProgress), (void**)getter_AddRefs(mPrintProgress));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = prtProgress->QueryInterface(NS_GET_IID(nsIWebProgressListener), (void**)getter_AddRefs(mWebProgressListener));
    NS_ENSURE_SUCCESS(rv, rv);

    nsPrintProgressParams* prtProgressParams = new nsPrintProgressParams();
    rv = prtProgressParams->QueryInterface(NS_GET_IID(nsIPrintProgressParams), (void**)printProgressParams);
    NS_ENSURE_SUCCESS(rv, rv);

    if (printProgressParams) 
    {
        nsCOMPtr<nsIDOMWindowInternal> parentDOMIntl(do_QueryInterface(parent));

        if (mWatcher && !parentDOMIntl) 
        {
            nsCOMPtr<nsIDOMWindow> active;
            mWatcher->GetActiveWindow(getter_AddRefs(active));
            parentDOMIntl = do_QueryInterface(active);
        }

        if (parentDOMIntl) 
        {
            mPrintProgress->OpenProgressDialog(parentDOMIntl, 
                                               isForPrinting?kPrintProgressDialogURL:kPrtPrvProgressDialogURL, 
                                               *printProgressParams, openDialogObserver, notifyOnOpen);
        }
    }

    *webProgressListener = static_cast<nsIWebProgressListener*>(this);
    NS_ADDREF(*webProgressListener);

    return rv;
}


NS_IMETHODIMP 
nsPrintingPromptService::ShowPageSetup(nsIDOMWindow *parent, nsIPrintSettings *printSettings, nsIObserver *aObs)
{
    NS_ENSURE_ARG(printSettings);

    
    nsCOMPtr<nsIPrintDialogService> dlgPrint(do_GetService(
                                             NS_PRINTDIALOGSERVICE_CONTRACTID));
    if (dlgPrint)
      return dlgPrint->ShowPageSetup(parent, printSettings);

    ParamBlock block;
    nsresult rv = block.Init();
    if (NS_FAILED(rv))
      return rv;

    block->SetInt(0, 0);
    return DoDialog(parent, block, nsnull, printSettings, kPageSetupDialogURL);
}


NS_IMETHODIMP 
nsPrintingPromptService::ShowPrinterProperties(nsIDOMWindow *parent, const PRUnichar *printerName, nsIPrintSettings *printSettings)
{
    






    NS_ENSURE_ARG(printerName);
    NS_ENSURE_ARG(printSettings);

    ParamBlock block;
    nsresult rv = block.Init();
    if (NS_FAILED(rv))
      return rv;

    block->SetInt(0, 0);
    return DoDialog(parent, block, nsnull, printSettings, kPrinterPropertiesURL);
   
}

nsresult
nsPrintingPromptService::DoDialog(nsIDOMWindow *aParent,
                                  nsIDialogParamBlock *aParamBlock, 
                                  nsIWebBrowserPrint *aWebBrowserPrint, 
                                  nsIPrintSettings* aPS,
                                  const char *aChromeURL)
{
    NS_ENSURE_ARG(aParamBlock);
    NS_ENSURE_ARG(aPS);
    NS_ENSURE_ARG(aChromeURL);

    if (!mWatcher)
        return NS_ERROR_FAILURE;

    nsresult rv = NS_OK;

    
    
    
    nsCOMPtr<nsIDOMWindow> activeParent; 
    if (!aParent) 
    {
        mWatcher->GetActiveWindow(getter_AddRefs(activeParent));
        aParent = activeParent;
    }

    
    
    nsCOMPtr<nsISupportsArray> array;
    NS_NewISupportsArray(getter_AddRefs(array));
    if (!array) return NS_ERROR_FAILURE;

    nsCOMPtr<nsISupports> psSupports(do_QueryInterface(aPS));
    NS_ASSERTION(psSupports, "PrintSettings must be a supports");
    array->AppendElement(psSupports);

    if (aWebBrowserPrint) {
      nsCOMPtr<nsISupports> wbpSupports(do_QueryInterface(aWebBrowserPrint));
      NS_ASSERTION(wbpSupports, "nsIWebBrowserPrint must be a supports");
      array->AppendElement(wbpSupports);
    }

    nsCOMPtr<nsISupports> blkSupps(do_QueryInterface(aParamBlock));
    NS_ASSERTION(blkSupps, "IOBlk must be a supports");
    array->AppendElement(blkSupps);

    nsCOMPtr<nsISupports> arguments(do_QueryInterface(array));
    NS_ASSERTION(array, "array must be a supports");


    nsCOMPtr<nsIDOMWindow> dialog;
    rv = mWatcher->OpenWindow(aParent, aChromeURL, "_blank",
                              "centerscreen,chrome,modal,titlebar", arguments,
                              getter_AddRefs(dialog));

    
    
    if (NS_SUCCEEDED(rv) && aWebBrowserPrint) 
    {
        PRInt32 status;
        aParamBlock->GetInt(0, &status);
        return status == 0?NS_ERROR_ABORT:NS_OK;
    }

    return rv;
}






NS_IMETHODIMP 
nsPrintingPromptService::OnStateChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 aStateFlags, nsresult aStatus)
{
  if ((aStateFlags & STATE_STOP) && mWebProgressListener) {
    mWebProgressListener->OnStateChange(aWebProgress, aRequest, aStateFlags, aStatus);
    if (mPrintProgress) {
      mPrintProgress->CloseProgressDialog(PR_TRUE);
    }
    mPrintProgress       = nsnull;
    mWebProgressListener = nsnull;
  }
  return NS_OK;
}


NS_IMETHODIMP 
nsPrintingPromptService::OnProgressChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRInt32 aCurSelfProgress, PRInt32 aMaxSelfProgress, PRInt32 aCurTotalProgress, PRInt32 aMaxTotalProgress)
{
  if (mWebProgressListener) {
    return mWebProgressListener->OnProgressChange(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress);
  }
  return NS_OK;
}


NS_IMETHODIMP 
nsPrintingPromptService::OnLocationChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsIURI *location)
{
  if (mWebProgressListener) {
    return mWebProgressListener->OnLocationChange(aWebProgress, aRequest, location);
  }
  return NS_OK;
}


NS_IMETHODIMP 
nsPrintingPromptService::OnStatusChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsresult aStatus, const PRUnichar *aMessage)
{
  if (mWebProgressListener) {
    return mWebProgressListener->OnStatusChange(aWebProgress, aRequest, aStatus, aMessage);
  }
  return NS_OK;
}


NS_IMETHODIMP 
nsPrintingPromptService::OnSecurityChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 state)
{
  if (mWebProgressListener) {
    return mWebProgressListener->OnSecurityChange(aWebProgress, aRequest, state);
  }
  return NS_OK;
}
