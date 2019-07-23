





































#using <mscorlib.dll>

#include "DotNETEmbed.h"
#include "WebChrome.h"

#define NS_WEBBROWSER_CONTRACTID "@mozilla.org/embedding/browser/nsWebBrowser;1"

#pragma unmanaged





NS_IMPL_ADDREF(WebBrowserChrome)
NS_IMPL_RELEASE(WebBrowserChrome)

NS_INTERFACE_MAP_BEGIN(WebBrowserChrome)
 NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIWebBrowserChrome)
 NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome)
 NS_INTERFACE_MAP_ENTRY(nsIEmbeddingSiteWindow)
NS_INTERFACE_MAP_END


WebBrowserChrome::WebBrowserChrome()
{
  mNativeWindow = nsnull;
}

WebBrowserChrome::~WebBrowserChrome()
{
}


NS_IMETHODIMP WebBrowserChrome::GetSiteWindow(void * *aSiteWindow)
{
  *aSiteWindow = mNativeWindow;
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


NS_IMETHODIMP WebBrowserChrome::SetFocus()
{
  return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::SetDimensions(PRUint32 aFlags, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy)
{
  return NS_OK;
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
  return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::ExitModalEventLoop(nsresult aStatus)
{
  return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::ShowAsModal(void)
{
  return NS_OK;
}

NS_IMETHODIMP WebBrowserChrome::IsWindowModal(PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP WebBrowserChrome::SetStatus(PRUint32 aType, const PRUnichar* aStatus)
{
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
  return NS_OK;
}


NS_IMETHODIMP WebBrowserChrome::SizeBrowserTo(PRInt32 aWidth, PRInt32 aHeight)
{
  ::MoveWindow((HWND)mNativeWindow, 0, 0, aWidth, aHeight, TRUE);
  return NS_OK;
}

nsresult WebBrowserChrome::CreateBrowser(HWND hWnd, PRInt32 aX, PRInt32 aY,
                                         PRInt32 aCX, PRInt32 aCY,
                                         nsIWebBrowser **aBrowser)
{
  NS_ENSURE_ARG_POINTER(aBrowser);
  *aBrowser = nsnull;

  mWebBrowser = do_CreateInstance(NS_WEBBROWSER_CONTRACTID);
  
  if (!mWebBrowser)
    return NS_ERROR_FAILURE;

  (void)mWebBrowser->SetContainerWindow(static_cast<nsIWebBrowserChrome*>(this));

  nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(mWebBrowser);
  dsti->SetItemType(nsIDocShellTreeItem::typeContentWrapper);

  nsCOMPtr<nsIBaseWindow> browserBaseWindow = do_QueryInterface(mWebBrowser);

  mNativeWindow = hWnd;

  if (!mNativeWindow)
    return NS_ERROR_FAILURE;

  browserBaseWindow->InitWindow( mNativeWindow,
                           nsnull, 
                           aX, aY, aCX, aCY);
  browserBaseWindow->Create();


  if (mWebBrowser)
  {
    *aBrowser = mWebBrowser;
    NS_ADDREF(*aBrowser);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

