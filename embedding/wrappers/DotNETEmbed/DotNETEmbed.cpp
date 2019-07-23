





































#include <vcclr.h>
#include "nsCOMPtr.h"
#include "nsIWebBrowser.h"
#include "nsIWebBrowserChrome.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIBaseWindow.h"
#include "nsIWebNavigation.h"
#include "nsIWindowWatcher.h"
#include "nsIInputStream.h"
#include "nsEmbedAPI.h"

#include "DotNETEmbed.h"
#include "umWebChrome.h"

using namespace Mozilla::Embedding;



public __gc class UTF8EncodingHolder
{
public:
  static Text::UTF8Encoding *sUTF8Encoding = new Text::UTF8Encoding();
};

String *
Mozilla::Embedding::CopyString(const nsAFlatCString& aStr)
{
  return new String(aStr.get(), 0, aStr.Length(),
                    UTF8EncodingHolder::sUTF8Encoding);
}





nsAFlatString&
Mozilla::Embedding::CopyString(String *aSrc, nsAFlatString& aDest)
{
  const wchar_t __pin * strbuf = PtrToStringChars(aSrc);

  aDest.Assign(strbuf, aSrc->Length);

  return aDest;
}

nsAFlatCString&
Mozilla::Embedding::CopyString(String *aSrc, nsAFlatCString& aDest)
{
  const wchar_t __pin * strbuf = PtrToStringChars(aSrc);

  CopyUTF16toUTF8(nsDependentString(strbuf, aSrc->Length), aDest);

  return aDest;
}

void
Mozilla::Embedding::ThrowIfFailed(nsresult rv)
{
  if (NS_FAILED(rv)) {
    

    throw "rv is an error code!";
  }
}

#pragma unmanaged

#define NS_WEBBROWSER_CONTRACTID "@mozilla.org/embedding/browser/nsWebBrowser;1"




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


NS_IMETHODIMP
WebBrowserChrome::GetSiteWindow(void * *aSiteWindow)
{
  *aSiteWindow = mNativeWindow;
  return NS_OK;
}

NS_IMETHODIMP
WebBrowserChrome::GetTitle(PRUnichar * *aTitle)
{
  NS_ENSURE_ARG_POINTER(aTitle);
  *aTitle = nsnull;
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
WebBrowserChrome::SetTitle(const PRUnichar * aTitle)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
WebBrowserChrome::GetVisibility(PRBool * aVisibility)
{
  NS_ENSURE_ARG_POINTER(aVisibility);
  *aVisibility = PR_TRUE;
  return NS_OK;
}


NS_IMETHODIMP
WebBrowserChrome::SetVisibility(PRBool aVisibility)
{
  return NS_OK;
}


NS_IMETHODIMP
WebBrowserChrome::SetFocus()
{
  return NS_OK;
}

NS_IMETHODIMP
WebBrowserChrome::SetDimensions(PRUint32 aFlags, PRInt32 x, PRInt32 y,
                                PRInt32 cx, PRInt32 cy)
{
  return NS_OK;
}

NS_IMETHODIMP
WebBrowserChrome::GetDimensions(PRUint32 aFlags, PRInt32 *x, PRInt32 *y,
                                PRInt32 *cx, PRInt32 *cy)
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

NS_IMETHODIMP
WebBrowserChrome::ExitModalEventLoop(nsresult aStatus)
{
  return NS_OK;
}

NS_IMETHODIMP
WebBrowserChrome::ShowAsModal(void)
{
  return NS_OK;
}

NS_IMETHODIMP
WebBrowserChrome::IsWindowModal(PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
WebBrowserChrome::SetStatus(PRUint32 aType, const PRUnichar* aStatus)
{
  return NS_OK;
}

NS_IMETHODIMP
WebBrowserChrome::GetWebBrowser(nsIWebBrowser** aWebBrowser)
{
  NS_ENSURE_ARG_POINTER(aWebBrowser);
  *aWebBrowser = mWebBrowser;
  NS_IF_ADDREF(*aWebBrowser);
  return NS_OK;
}

NS_IMETHODIMP
WebBrowserChrome::SetWebBrowser(nsIWebBrowser* aWebBrowser)
{
  mWebBrowser = aWebBrowser;
  return NS_OK;
}

NS_IMETHODIMP
WebBrowserChrome::GetChromeFlags(PRUint32* aChromeMask)
{
  *aChromeMask = mChromeFlags;
  return NS_OK;
}

NS_IMETHODIMP
WebBrowserChrome::SetChromeFlags(PRUint32 aChromeMask)
{
  mChromeFlags = aChromeMask;
  return NS_OK;
}

NS_IMETHODIMP
WebBrowserChrome::DestroyBrowserWindow(void)
{
  return NS_OK;
}


NS_IMETHODIMP
WebBrowserChrome::SizeBrowserTo(PRInt32 aWidth, PRInt32 aHeight)
{
  ::MoveWindow((HWND)mNativeWindow, 0, 0, aWidth, aHeight, TRUE);
  return NS_OK;
}

nsresult
WebBrowserChrome::CreateBrowser(HWND hWnd, PRInt32 aX, PRInt32 aY, PRInt32 aCX,
                                PRInt32 aCY, nsIWebBrowser **aBrowser)
{
  NS_ENSURE_ARG_POINTER(aBrowser);
  *aBrowser = nsnull;

  mWebBrowser = do_CreateInstance(NS_WEBBROWSER_CONTRACTID);

  if (!mWebBrowser)
    return NS_ERROR_FAILURE;

  mWebBrowser->SetContainerWindow(NS_STATIC_CAST(nsIWebBrowserChrome*, this));

  nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(mWebBrowser);
  dsti->SetItemType(nsIDocShellTreeItem::typeContentWrapper);

  nsCOMPtr<nsIBaseWindow> browserBaseWindow = do_QueryInterface(mWebBrowser);

  mNativeWindow = hWnd;

  if (!mNativeWindow)
    return NS_ERROR_FAILURE;

  browserBaseWindow->InitWindow(mNativeWindow, nsnull, aX, aY, aCX, aCY);
  browserBaseWindow->Create();

  if (mWebBrowser)
  {
    *aBrowser = mWebBrowser;
    NS_ADDREF(*aBrowser);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

#pragma managed


Gecko::Gecko()
  : mChrome(nsnull)
{
}

void
Gecko::TermEmbedding()
{
  if (!sIsInitialized) {
    return;
  }

  sIsInitialized = false;

  nsresult rv = NS_TermEmbedding();
  ThrowIfFailed(rv);
}

void
Gecko::OpenURL(String *aUrl)
{
  if (!sIsInitialized) {
    nsresult rv = NS_InitEmbedding(nsnull, nsnull);
    ThrowIfFailed(rv);

    sIsInitialized = true;
  }

  const wchar_t __pin * url = PtrToStringChars(aUrl);
  nsresult rv;

  HWND hWnd = (HWND)Handle.ToPointer();

  if (!mChrome) {
    CreateBrowserWindow(nsIWebBrowserChrome::CHROME_ALL, nsnull);
  }

  
  nsCOMPtr<nsIWebBrowser> newBrowser;
  mChrome->GetWebBrowser(getter_AddRefs(newBrowser));
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(newBrowser));

  rv = webNav->LoadURI(url, nsIWebNavigation::LOAD_FLAGS_NONE, nsnull, nsnull,
                       nsnull);

  ThrowIfFailed(rv);
}

void
Gecko::OnResize(EventArgs *e)
{
  if (mChrome) {
    nsCOMPtr<nsIEmbeddingSiteWindow> embeddingSite =
      do_QueryInterface(mChrome);

    RECT rect;
    GetClientRect((HWND)Handle.ToPointer(), &rect);

    
    nsCOMPtr<nsIWebBrowser> webBrowser;
    mChrome->GetWebBrowser(getter_AddRefs(webBrowser));
    nsCOMPtr<nsIBaseWindow> webBrowserAsWin = do_QueryInterface(webBrowser);
    if (webBrowserAsWin) {
      webBrowserAsWin->SetPositionAndSize(rect.left,
                                          rect.top,
                                          rect.right - rect.left,
                                          rect.bottom - rect.top,
                                          PR_TRUE);
      webBrowserAsWin->SetVisibility(PR_TRUE);
    }
  }

  UserControl::OnResize(e);
}

void
Gecko::CreateBrowserWindow(PRUint32 aChromeFlags, nsIWebBrowserChrome *aParent)
{
  WebBrowserChrome * chrome = new WebBrowserChrome();
  if (!chrome) {
    throw new OutOfMemoryException();
  }

  mChrome = chrome;
  NS_ADDREF(mChrome);

  
  
  
  NS_ADDREF(mChrome);

  chrome->SetChromeFlags(aChromeFlags);

  HWND hWnd = (HWND)Handle.ToPointer();

  
  nsCOMPtr<nsIWebBrowser> newBrowser;
  chrome->CreateBrowser(hWnd, -1, -1, -1, -1, getter_AddRefs(newBrowser));
  if (!newBrowser) {
    ThrowIfFailed(NS_ERROR_FAILURE);
  }

  
  OnResize(0);

  
  
  if (!(aChromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME))
    ::ShowWindow(hWnd, SW_RESTORE);
}

