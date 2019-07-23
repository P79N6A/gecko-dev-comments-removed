





























#include "resource.h"
#include "winEmbed.h"
#include "WebBrowserChrome.h"


#include <stdio.h>



#include "nsStringAPI.h"
#include "nsIComponentManager.h"
#include "nsIDOMWindow.h"
#include "nsIGenericFactory.h"
#include "nsIInterfaceRequestor.h"
#include "nsIRequest.h"
#include "nsIURI.h"
#include "nsIWebProgress.h"
#include "nsCWebBrowser.h"
#include "nsIProfileChangeStatus.h"


#include "nsComponentManagerUtils.h"


#include "nsIWebNavigation.h"

WebBrowserChrome::WebBrowserChrome()
{
    mNativeWindow = nsnull;
    mSizeSet = PR_FALSE;
}

WebBrowserChrome::~WebBrowserChrome()
{
    WebBrowserChromeUI::Destroyed(this);
}

nsresult WebBrowserChrome::CreateBrowser(PRInt32 aX, PRInt32 aY,
                                         PRInt32 aCX, PRInt32 aCY,
                                         nsIWebBrowser **aBrowser)
{
    NS_ENSURE_ARG_POINTER(aBrowser);
    *aBrowser = nsnull;

    mWebBrowser = do_CreateInstance(NS_WEBBROWSER_CONTRACTID);
    
    if (!mWebBrowser)
        return NS_ERROR_FAILURE;

    (void)mWebBrowser->SetContainerWindow(static_cast<nsIWebBrowserChrome*>(this));

    nsCOMPtr<nsIBaseWindow> browserBaseWindow = do_QueryInterface(mWebBrowser);

    mNativeWindow = WebBrowserChromeUI::CreateNativeWindow(static_cast<nsIWebBrowserChrome*>(this));

    if (!mNativeWindow)
        return NS_ERROR_FAILURE;

    browserBaseWindow->InitWindow( mNativeWindow,
                             nsnull, 
                             aX, aY, aCX, aCY);
    browserBaseWindow->Create();

    nsCOMPtr<nsIWebProgressListener> listener(static_cast<nsIWebProgressListener*>(this));
    nsCOMPtr<nsIWeakReference> thisListener(do_GetWeakReference(listener));
    (void)mWebBrowser->AddWebBrowserListener(thisListener, 
        NS_GET_IID(nsIWebProgressListener));

    
    mWebBrowser->AddWebBrowserListener(thisListener, NS_GET_IID(nsISHistoryListener));

    if (mWebBrowser)
    {
      *aBrowser = mWebBrowser;
      NS_ADDREF(*aBrowser);
      return NS_OK;
    }
    return NS_ERROR_FAILURE;
}





NS_IMPL_ADDREF(WebBrowserChrome)
NS_IMPL_RELEASE(WebBrowserChrome)

NS_INTERFACE_MAP_BEGIN(WebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
   NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY(nsIEmbeddingSiteWindow)
   NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener) 
   NS_INTERFACE_MAP_ENTRY(nsISHistoryListener)
   NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
   NS_INTERFACE_MAP_ENTRY(nsIObserver)
   NS_INTERFACE_MAP_ENTRY(nsIContextMenuListener)
   NS_INTERFACE_MAP_ENTRY(nsITooltipListener)
NS_INTERFACE_MAP_END





NS_IMETHODIMP WebBrowserChrome::GetInterface(const nsIID &aIID, void** aInstancePtr)
{
    NS_ENSURE_ARG_POINTER(aInstancePtr);

    *aInstancePtr = 0;
    if (aIID.Equals(NS_GET_IID(nsIDOMWindow)))
    {
        if (mWebBrowser)
        {
            return mWebBrowser->GetContentDOMWindow((nsIDOMWindow **) aInstancePtr);
        }
        return NS_ERROR_NOT_INITIALIZED;
    }
    return QueryInterface(aIID, aInstancePtr);
}





NS_IMETHODIMP WebBrowserChrome::SetStatus(PRUint32 aType, const PRUnichar* aStatus)
{
    WebBrowserChromeUI::UpdateStatusBarText(this, aStatus);
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetWebBrowser(nsIWebBrowser** aWebBrowser)
{
    NS_ENSURE_ARG_POINTER(aWebBrowser);
    *aWebBrowser = mWebBrowser;
    NS_IF_ADDREF(*aWebBrowser);
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetWebBrowser(nsIWebBrowser* aWebBrowser)
{
    mWebBrowser = aWebBrowser;
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::GetChromeFlags(PRUint32* aChromeMask)
{
    *aChromeMask = mChromeFlags;
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetChromeFlags(PRUint32 aChromeMask)
{
    mChromeFlags = aChromeMask;
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::DestroyBrowserWindow(void)
{
    WebBrowserChromeUI::Destroy(this);
    return NS_OK;
}



NS_IMETHODIMP WebBrowserChrome::SizeBrowserTo(PRInt32 aWidth, PRInt32 aHeight)
{
  


  WebBrowserChromeUI::SizeTo(this, aWidth, aHeight);
  mSizeSet = PR_TRUE;
  return NS_OK;
}


NS_IMETHODIMP WebBrowserChrome::ShowAsModal(void)
{
  if (mDependentParent)
    AppCallbacks::EnableChromeWindow(mDependentParent, PR_FALSE);

  mContinueModalLoop = PR_TRUE;
  AppCallbacks::RunEventLoop(mContinueModalLoop);

  if (mDependentParent)
    AppCallbacks::EnableChromeWindow(mDependentParent, PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::IsWindowModal(PRBool *_retval)
{
    *_retval = PR_FALSE;
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::ExitModalEventLoop(nsresult aStatus)
{
  mContinueModalLoop = PR_FALSE;
  return NS_OK;
}





NS_IMETHODIMP WebBrowserChrome::FocusNextElement()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::FocusPrevElement()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}





NS_IMETHODIMP WebBrowserChrome::OnProgressChange(nsIWebProgress *progress, nsIRequest *request,
                                                  PRInt32 curSelfProgress, PRInt32 maxSelfProgress,
                                                  PRInt32 curTotalProgress, PRInt32 maxTotalProgress)
{
    WebBrowserChromeUI::UpdateProgress(this, curTotalProgress, maxTotalProgress);
    return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::OnStateChange(nsIWebProgress *progress, nsIRequest *request,
                                               PRUint32 progressStateFlags, nsresult status)
{
    if ((progressStateFlags & STATE_START) && (progressStateFlags & STATE_IS_DOCUMENT))
    {
        WebBrowserChromeUI::UpdateBusyState(this, PR_TRUE);
    }

    if ((progressStateFlags & STATE_STOP) && (progressStateFlags & STATE_IS_DOCUMENT))
    {
        WebBrowserChromeUI::UpdateBusyState(this, PR_FALSE);
        WebBrowserChromeUI::UpdateProgress(this, 0, 100);
        WebBrowserChromeUI::UpdateStatusBarText(this, nsnull);
        ContentFinishedLoading();
    }

    return NS_OK;
}


NS_IMETHODIMP WebBrowserChrome::OnLocationChange(nsIWebProgress* aWebProgress,
                                                 nsIRequest* aRequest,
                                                 nsIURI *location)
{
  PRBool isSubFrameLoad = PR_FALSE; 
  if (aWebProgress) {
    nsCOMPtr<nsIDOMWindow>  domWindow;
    nsCOMPtr<nsIDOMWindow>  topDomWindow;
    aWebProgress->GetDOMWindow(getter_AddRefs(domWindow));
    if (domWindow) { 
      domWindow->GetTop(getter_AddRefs(topDomWindow));
    }
    if (domWindow != topDomWindow)
      isSubFrameLoad = PR_TRUE;
  }
  if (!isSubFrameLoad)
    WebBrowserChromeUI::UpdateCurrentURI(this);
  return NS_OK;
}

NS_IMETHODIMP 
WebBrowserChrome::OnStatusChange(nsIWebProgress* aWebProgress,
                                 nsIRequest* aRequest,
                                 nsresult aStatus,
                                 const PRUnichar* aMessage)
{
    WebBrowserChromeUI::UpdateStatusBarText(this, aMessage);
    return NS_OK;
}



NS_IMETHODIMP 
WebBrowserChrome::OnSecurityChange(nsIWebProgress *aWebProgress, 
                                    nsIRequest *aRequest, 
                                    PRUint32 state)
{
    return NS_OK;
}





NS_IMETHODIMP
WebBrowserChrome::OnHistoryNewEntry(nsIURI * aNewURI)
{
    return SendHistoryStatusMessage(aNewURI, "add");
}

NS_IMETHODIMP
WebBrowserChrome::OnHistoryGoBack(nsIURI * aBackURI, PRBool * aContinue)
{
    
    *aContinue = PR_TRUE;
    return SendHistoryStatusMessage(aBackURI, "back");
}


NS_IMETHODIMP
WebBrowserChrome::OnHistoryGoForward(nsIURI * aForwardURI, PRBool * aContinue)
{
    
    *aContinue = PR_TRUE;
    return SendHistoryStatusMessage(aForwardURI, "forward");
}


NS_IMETHODIMP
WebBrowserChrome::OnHistoryGotoIndex(PRInt32 aIndex, nsIURI * aGotoURI, PRBool * aContinue)
{
    
    *aContinue = PR_TRUE;
    return SendHistoryStatusMessage(aGotoURI, "goto", aIndex);
}

NS_IMETHODIMP
WebBrowserChrome::OnHistoryReload(nsIURI * aURI, PRUint32 aReloadFlags, PRBool * aContinue)
{
    
    *aContinue = PR_TRUE;
    return SendHistoryStatusMessage(aURI, "reload", 0 , aReloadFlags);
}

NS_IMETHODIMP
WebBrowserChrome::OnHistoryPurge(PRInt32 aNumEntries, PRBool *aContinue)
{
    
    *aContinue = PR_FALSE;
    return SendHistoryStatusMessage(nsnull, "purge", aNumEntries);
}

static void
AppendIntToCString(PRInt32 info1, nsCString& aResult)
{
  char intstr[10];
  _snprintf(intstr, sizeof(intstr) - 1, "%i", info1);
  intstr[sizeof(intstr) - 1] = '\0';
  aResult.Append(intstr);
}

nsresult
WebBrowserChrome::SendHistoryStatusMessage(nsIURI * aURI, char * operation, PRInt32 info1, PRUint32 aReloadFlags)
{
    nsCString uriSpec;
    if (aURI)
    {
        aURI->GetSpec(uriSpec);
    }

    nsCString status;

    if(!(strcmp(operation, "back")))
    {
        status.Assign("Going back to url: ");
        status.Append(uriSpec);
    }
    else if (!(strcmp(operation, "forward")))
    {
        
        status.Assign("Going forward to url: ");
        status.Append(uriSpec);
    }
    else if (!(strcmp(operation, "reload")))
    {
        
        if (aReloadFlags & nsIWebNavigation::LOAD_FLAGS_BYPASS_PROXY && 
            aReloadFlags & nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE)
        {
            status.Assign("Reloading url, (bypassing proxy and cache): ");
        }
        else if (aReloadFlags & nsIWebNavigation::LOAD_FLAGS_BYPASS_PROXY)
        {
            status.Assign("Reloading url, (bypassing proxy): ");
        }
        else if (aReloadFlags & nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE)
        {
            status.Assign("Reloading url, (bypassing cache): ");
        }
        else
        {
            status.Assign("Reloading url, (normal): ");
        }
        status.Append(uriSpec);
    }
    else if (!(strcmp(operation, "add")))
    {
        status.Assign(uriSpec);
        status.Append(" added to session History");
    }
    else if (!(strcmp(operation, "goto")))
    {
        status.Assign("Going to HistoryIndex: ");

	AppendIntToCString(info1, status);

        status.Append(" Url: ");
        status.Append(uriSpec);
    }
    else if (!(strcmp(operation, "purge")))
    {
        AppendIntToCString(info1, status);
        status.Append(" purged from Session History");
    }

    nsString wstatus;
    NS_CStringToUTF16(status, NS_CSTRING_ENCODING_UTF8, wstatus);
    WebBrowserChromeUI::UpdateStatusBarText(this, wstatus.get());

    return NS_OK;
}

void WebBrowserChrome::ContentFinishedLoading()
{
  
  
  if (mWebBrowser && !mSizeSet &&
     (mChromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME)) {
    nsCOMPtr<nsIDOMWindow> contentWin;
    mWebBrowser->GetContentDOMWindow(getter_AddRefs(contentWin));
    if (contentWin)
        contentWin->SizeToContent();
    WebBrowserChromeUI::ShowWindow(this, PR_TRUE);
  }
}






NS_IMETHODIMP WebBrowserChrome::SetDimensions(PRUint32 aFlags, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP WebBrowserChrome::GetDimensions(PRUint32 aFlags, PRInt32 *x, PRInt32 *y, PRInt32 *cx, PRInt32 *cy)
{
    if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION)
    {
        *x = 0;
        *y = 0;
    }
    if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER ||
        aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER)
    {
        *cx = 0;
        *cy = 0;
    }
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP WebBrowserChrome::SetFocus()
{
    WebBrowserChromeUI::SetFocus(this);
    return NS_OK;
}


NS_IMETHODIMP WebBrowserChrome::GetTitle(PRUnichar * *aTitle)
{
   NS_ENSURE_ARG_POINTER(aTitle);

   *aTitle = nsnull;
   
   return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP WebBrowserChrome::SetTitle(const PRUnichar * aTitle)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP WebBrowserChrome::GetVisibility(PRBool * aVisibility)
{
    NS_ENSURE_ARG_POINTER(aVisibility);
    *aVisibility = PR_TRUE;
    return NS_OK;
}
NS_IMETHODIMP WebBrowserChrome::SetVisibility(PRBool aVisibility)
{
    return NS_OK;
}


NS_IMETHODIMP WebBrowserChrome::GetSiteWindow(void * *aSiteWindow)
{
   NS_ENSURE_ARG_POINTER(aSiteWindow);

   *aSiteWindow = mNativeWindow;
   return NS_OK;
}






NS_IMETHODIMP WebBrowserChrome::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
    nsresult rv = NS_OK;
    if (strcmp(aTopic, "profile-change-teardown") == 0)
    {
        
        WebBrowserChromeUI::Destroy(this);
    }
    return rv;
}






NS_IMETHODIMP WebBrowserChrome::OnShowContextMenu(PRUint32 aContextFlags, nsIDOMEvent *aEvent, nsIDOMNode *aNode)
{
    WebBrowserChromeUI::ShowContextMenu(this, aContextFlags, aEvent, aNode);
    return NS_OK;
}






NS_IMETHODIMP WebBrowserChrome::OnShowTooltip(PRInt32 aXCoords, PRInt32 aYCoords, const PRUnichar *aTipText)
{
    WebBrowserChromeUI::ShowTooltip(this, aXCoords, aYCoords, aTipText);
    return NS_OK;
}


NS_IMETHODIMP WebBrowserChrome::OnHideTooltip()
{
    WebBrowserChromeUI::HideTooltip(this);
    return NS_OK;
}
