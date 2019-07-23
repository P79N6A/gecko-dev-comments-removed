





































#include "nsIGenericFactory.h"
#include "nsIComponentManager.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsIURI.h"
#include "nsIWebProgress.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMWindow.h"
#include "nsIDOMWindowInternal.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIRequest.h"
#include "nsIChannel.h"
#include "nsCWebBrowser.h"
#include "nsWidgetsCID.h"
#include "nsIProfileChangeStatus.h"
#include "nsCRT.h"
#include "nsWeakReference.h"


#include "resource.h"
#include "mozEmbed.h"
#include "nsQABrowserChrome.h"


nsQABrowserChrome::nsQABrowserChrome()
{
    mNativeWindow = nsnull;
    mSizeSet = PR_FALSE;
}

nsQABrowserChrome::~nsQABrowserChrome()
{
  if (mBrowserUIGlue)
    mBrowserUIGlue->Destroyed(this);
}







NS_IMPL_ISUPPORTS7(nsQABrowserChrome, 
                   nsIQABrowserChrome, 
                   nsIWebBrowserChrome,
                   nsIInterfaceRequestor,
                   nsIEmbeddingSiteWindow,
                   nsIWebProgressListener,
                   nsIWebBrowserChromeFocus,
                   nsISupportsWeakReference);







NS_IMETHODIMP nsQABrowserChrome::GetInterface(const nsIID &aIID, void** aInstancePtr)
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





NS_IMETHODIMP
nsQABrowserChrome::InitQAChrome(nsIQABrowserUIGlue * aUIGlue,  nativeWindow aNativeWnd)
{
  mBrowserUIGlue = aUIGlue;
  mNativeWindow = aNativeWnd;
  
  return NS_OK;
}





NS_IMETHODIMP nsQABrowserChrome::SetStatus(PRUint32 aType, const PRUnichar* aStatus)
{
  if (mBrowserUIGlue)
    return mBrowserUIGlue->UpdateStatusBarText(this, aStatus);
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsQABrowserChrome::GetWebBrowser(nsIWebBrowser** aWebBrowser)
{
    NS_ENSURE_ARG_POINTER(aWebBrowser);
    *aWebBrowser = mWebBrowser;
    NS_IF_ADDREF(*aWebBrowser);
    return NS_OK;
}

NS_IMETHODIMP nsQABrowserChrome::SetWebBrowser(nsIWebBrowser* aWebBrowser)
{
    mWebBrowser = aWebBrowser;
    return NS_OK;
}

NS_IMETHODIMP nsQABrowserChrome::GetChromeFlags(PRUint32* aChromeMask)
{
  if (aChromeMask)
    *aChromeMask = mChromeFlags;
  return NS_OK;
}

NS_IMETHODIMP nsQABrowserChrome::SetChromeFlags(PRUint32 aChromeMask)
{
    mChromeFlags = aChromeMask;
    return NS_OK;
}

NS_IMETHODIMP nsQABrowserChrome::DestroyBrowserWindow(void)
{
  if (mBrowserUIGlue)
    return mBrowserUIGlue->Destroy(this);
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsQABrowserChrome::SizeBrowserTo(PRInt32 aCX, PRInt32 aCY)
{
  



  mSizeSet = PR_TRUE;
  if (mBrowserUIGlue)
    return mBrowserUIGlue->SizeTo(this, aCX, aCY);
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsQABrowserChrome::ShowAsModal(void)
{










  return NS_OK;
}

NS_IMETHODIMP nsQABrowserChrome::IsWindowModal(PRBool *aReturn)
{
  if (aReturn)
    *aReturn = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsQABrowserChrome::ExitModalEventLoop(nsresult aStatus)
{
  mContinueModalLoop = PR_FALSE;
  return NS_OK;
}





NS_IMETHODIMP nsQABrowserChrome::SetDimensions(PRUint32 aFlags, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsQABrowserChrome::GetDimensions(PRUint32 aFlags, PRInt32 *x, PRInt32 *y, PRInt32 *cx, PRInt32 *cy)
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


NS_IMETHODIMP nsQABrowserChrome::SetFocus()
{
  if (mBrowserUIGlue)
    return mBrowserUIGlue->SetFocus(this);
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsQABrowserChrome::GetTitle(PRUnichar * *aTitle)
{
   NS_ENSURE_ARG_POINTER(aTitle);
   *aTitle = nsnull;   
   return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsQABrowserChrome::SetTitle(const PRUnichar * aTitle)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsQABrowserChrome::GetVisibility(PRBool * aVisibility)
{
    NS_ENSURE_ARG_POINTER(aVisibility);
    *aVisibility = PR_TRUE;
    return NS_OK;
}
NS_IMETHODIMP nsQABrowserChrome::SetVisibility(PRBool aVisibility)
{
    return NS_OK;
}


NS_IMETHODIMP nsQABrowserChrome::GetSiteWindow(void * *aSiteWindow)
{
   NS_ENSURE_ARG_POINTER(aSiteWindow);
   *aSiteWindow = mNativeWindow;
   return NS_OK;
}





NS_IMETHODIMP
nsQABrowserChrome::OnProgressChange(nsIWebProgress *progress, nsIRequest *request,
                                    PRInt32 curSelfProgress, PRInt32 maxSelfProgress,
                                    PRInt32 curTotalProgress, PRInt32 maxTotalProgress)
{
  if (mBrowserUIGlue)
    return mBrowserUIGlue->UpdateProgress(this, curTotalProgress, maxTotalProgress);
  return NS_OK;
}

NS_IMETHODIMP
nsQABrowserChrome::OnStateChange(nsIWebProgress *progress, nsIRequest *request,
                                               PRUint32 progressStateFlags, nsresult status)
{
  if (!mBrowserUIGlue)
    return NS_ERROR_FAILURE;

  if ((progressStateFlags & STATE_START) && (progressStateFlags & STATE_IS_DOCUMENT))
  {
    mBrowserUIGlue->UpdateBusyState(this, PR_TRUE);
  }

  if ((progressStateFlags & STATE_STOP) && (progressStateFlags & STATE_IS_DOCUMENT))
  {
    mBrowserUIGlue->UpdateBusyState(this, PR_FALSE);
    mBrowserUIGlue->UpdateProgress(this, 0, 100);
    mBrowserUIGlue->UpdateStatusBarText(this, nsnull);
    ContentFinishedLoading();
  }

  return NS_OK;
}


NS_IMETHODIMP
nsQABrowserChrome::OnLocationChange(nsIWebProgress* aWebProgress,
                                    nsIRequest* aRequest,
                                    nsIURI *location)
{
  if (!mBrowserUIGlue)
    return NS_ERROR_FAILURE;

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
    mBrowserUIGlue->UpdateCurrentURI(this);
  return NS_OK;
}

NS_IMETHODIMP 
nsQABrowserChrome::OnStatusChange(nsIWebProgress* aWebProgress,
                                 nsIRequest* aRequest,
                                 nsresult aStatus,
                                 const PRUnichar* aMessage)
{
  if (mBrowserUIGlue)
    return mBrowserUIGlue->UpdateStatusBarText(this, aMessage);
  return NS_OK;
}



NS_IMETHODIMP 
nsQABrowserChrome::OnSecurityChange(nsIWebProgress *aWebProgress, 
                                    nsIRequest *aRequest, 
                                    PRUint32 state)
{
    return NS_OK;
}




NS_IMETHODIMP
nsQABrowserChrome::FocusNextElement()
{
    return NS_OK;
}

NS_IMETHODIMP
nsQABrowserChrome::FocusPrevElement()
{
    return NS_OK;
}



void
nsQABrowserChrome::ContentFinishedLoading()
{
  
  
  if (mWebBrowser && !mSizeSet && mBrowserUIGlue &&
     (mChromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME)) {
    nsCOMPtr<nsIDOMWindow> contentWin;
    mWebBrowser->GetContentDOMWindow(getter_AddRefs(contentWin));
    if (contentWin)
        contentWin->SizeToContent();
    mBrowserUIGlue->ShowWindow(this, PR_TRUE);
  }
}













