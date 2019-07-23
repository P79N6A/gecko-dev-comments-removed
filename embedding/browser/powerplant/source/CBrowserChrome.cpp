






































#include "CBrowserChrome.h"
#include "CBrowserShell.h"
#include "CBrowserMsgDefs.h"

#include "nsIGenericFactory.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsIURI.h"
#include "nsIWebProgress.h"
#include "nsIDocShellTreeItem.h"
#include "nsIRequest.h"
#include "nsIChannel.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIWindowWatcher.h"
#include "nsServiceManagerUtils.h"

#include "UMacUnicode.h"
#include "ApplIDs.h"

#include <LStaticText.h>
#include <LCheckBox.h>
#include <LEditText.h>
#include <UModalDialogs.h>
#include <LPushButton.h>






const PRInt32     kGrowIconSize = 15;





CBrowserChrome::CBrowserChrome(CBrowserShell *aShell,
                               UInt32 aChromeFlags,
                               Boolean aIsMainContent) :
    mBrowserShell(aShell), mBrowserWindow(nsnull),
    mChromeFlags(aChromeFlags), mIsMainContent(aIsMainContent),
    mSizeToContent(false),    
    mInModalLoop(false), mWindowVisible(false),
    mInitialLoadComplete(false)
{
	ThrowIfNil_(mBrowserShell);
	mBrowserWindow = LWindow::FetchWindowObject(mBrowserShell->GetMacWindow());
	StartListening();
}

CBrowserChrome::~CBrowserChrome()
{
}

void CBrowserChrome::SetBrowserShell(CBrowserShell *aShell)
{
    mBrowserShell = aShell;
    if (mBrowserShell)
	    mBrowserWindow = LWindow::FetchWindowObject(mBrowserShell->GetMacWindow());
    else
        mBrowserWindow = nsnull;    
}





NS_IMPL_ISUPPORTS8(CBrowserChrome,
                   nsIWebBrowserChrome,
                   nsIInterfaceRequestor,
                   nsIWebBrowserChromeFocus,
                   nsIEmbeddingSiteWindow,
                   nsIEmbeddingSiteWindow2,
                   nsIContextMenuListener2,
                   nsITooltipListener,
                   nsISupportsWeakReference);
                   




NS_IMETHODIMP CBrowserChrome::GetInterface(const nsIID &aIID, void** aInstancePtr)
{
   if (aIID.Equals(NS_GET_IID(nsIDOMWindow)))
   {
      nsCOMPtr<nsIWebBrowser> browser;
      GetWebBrowser(getter_AddRefs(browser));
      if (browser)
         return browser->GetContentDOMWindow((nsIDOMWindow **) aInstancePtr);
      return NS_ERROR_NOT_INITIALIZED;
   }

   return QueryInterface(aIID, aInstancePtr);
}





NS_IMETHODIMP CBrowserChrome::SetStatus(PRUint32 statusType, const PRUnichar *status)
{
     NS_ENSURE_TRUE(mBrowserShell, NS_ERROR_NOT_INITIALIZED);

     MsgChromeStatusChangeInfo info(mBrowserShell, statusType, status);
     mBrowserShell->BroadcastMessage(msg_OnChromeStatusChange, &info);
  
     return NS_OK;
}

NS_IMETHODIMP CBrowserChrome::GetWebBrowser(nsIWebBrowser** aWebBrowser)
{
    NS_ENSURE_ARG_POINTER(aWebBrowser);
    NS_ENSURE_TRUE(mBrowserShell, NS_ERROR_NOT_INITIALIZED);

    mBrowserShell->GetWebBrowser(aWebBrowser);
    return NS_OK;
}

NS_IMETHODIMP CBrowserChrome::SetWebBrowser(nsIWebBrowser* aWebBrowser)
{
    NS_ENSURE_ARG(aWebBrowser);   
    NS_ENSURE_TRUE(mBrowserShell, NS_ERROR_NOT_INITIALIZED);

    mBrowserShell->SetWebBrowser(aWebBrowser);
    return NS_OK;
}

NS_IMETHODIMP CBrowserChrome::GetChromeFlags(PRUint32* aChromeMask)
{
    NS_ENSURE_ARG_POINTER(aChromeMask);
   
    *aChromeMask = mChromeFlags;
    return NS_OK;
}

NS_IMETHODIMP CBrowserChrome::SetChromeFlags(PRUint32 aChromeMask)
{
    
    NS_ERROR("Haven't implemented this yet!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP CBrowserChrome::DestroyBrowserWindow()
{
    NS_ENSURE_TRUE(mBrowserShell, NS_ERROR_NOT_INITIALIZED);

    mInModalLoop = false;
    delete mBrowserWindow;
    return NS_OK;
}

NS_IMETHODIMP CBrowserChrome::IsWindowModal(PRBool *_retval)
{
    *_retval = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP CBrowserChrome::SizeBrowserTo(PRInt32 aCX, PRInt32 aCY)
{
   SDimension16 curSize;
   mBrowserShell->GetFrameSize(curSize);
   mBrowserWindow->ResizeWindowBy((aCX - curSize.width), (aCY - curSize.height));
   return NS_OK;
}


NS_IMETHODIMP CBrowserChrome::ShowAsModal(void)
{
    
    
    class CChromeDialogHandler : public StDialogHandler
    {
        public:
						CChromeDialogHandler(LWindow*		inWindow,
								             LCommander*	inSuper) :
					        StDialogHandler(inWindow, inSuper)
					    { }
					        
	    virtual         ~CChromeDialogHandler()
	                    { mDialog = nil; }
    };
    
    CChromeDialogHandler	 theHandler(mBrowserWindow, mBrowserWindow->GetSuperCommander());
	
	
	mInModalLoop = true;
	while (mInModalLoop) 
	    theHandler.DoDialog();

    return NS_OK;

}

NS_IMETHODIMP CBrowserChrome::ExitModalEventLoop(nsresult aStatus)
{
   mInModalLoop = false;
   return NS_OK;
}






NS_IMETHODIMP CBrowserChrome::FocusNextElement()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP CBrowserChrome::FocusPrevElement()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}






NS_IMETHODIMP CBrowserChrome::SetDimensions(PRUint32 flags, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy)
{
    NS_ENSURE_STATE(mBrowserWindow);
    NS_ENSURE_STATE(mBrowserShell);
    
    if ((flags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER) &&
        (flags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER))
        return NS_ERROR_INVALID_ARG;
    
    if (flags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION)
    {
        mBrowserWindow->MoveWindowTo(x, y);
    }        
    
    if (flags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER)
    {
        
        
        SDimension16 curSize;
        mBrowserShell->GetFrameSize(curSize);
        mBrowserWindow->ResizeWindowBy(cx - curSize.width, cy - curSize.height);
    }
    else if (flags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER)
    {
        if (mBrowserWindow->HasAttribute(windAttr_Resizable ))
            cy += 15;
        mBrowserWindow->ResizeWindowTo(cx, cy);
    }
    
    return NS_OK;
}

NS_IMETHODIMP CBrowserChrome::GetDimensions(PRUint32 flags, PRInt32 *x, PRInt32 *y, PRInt32 *cx, PRInt32 *cy)
{
    NS_ENSURE_STATE(mBrowserWindow);
    NS_ENSURE_STATE(mBrowserShell);
     
    if ((flags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER) &&
        (flags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER))
        return NS_ERROR_INVALID_ARG;

    Rect outerBounds;
    if ((flags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION) ||
        (flags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER))    
        mBrowserWindow->GetGlobalBounds(outerBounds);
    
    if (flags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION)
    {
        if (x)
            *x = outerBounds.left;
        if (y)
            *y = outerBounds.top;
    }        
    
    if (flags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER)
    {
        SDimension16 curSize;
        mBrowserShell->GetFrameSize(curSize);
        if (cx)
            *cx = curSize.width;
        if (cy)
            *cy = curSize.height;
    }
    else if (flags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER)
    {
        if (cx)
            *cx = outerBounds.right - outerBounds.left;
        if (cy)
        {
            *cy = outerBounds.bottom - outerBounds.top;
            if (mBrowserWindow->HasAttribute(windAttr_Resizable ))
                *cy -= 15;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP CBrowserChrome::SetFocus()
{
    mBrowserWindow->Select();
    return NS_OK;
}

NS_IMETHODIMP CBrowserChrome::GetVisibility(PRBool *aVisibility)
{
    NS_ENSURE_STATE(mBrowserWindow);
    NS_ENSURE_ARG_POINTER(aVisibility);
    
    *aVisibility = mWindowVisible;
    
    return NS_OK;
}

NS_IMETHODIMP CBrowserChrome::SetVisibility(PRBool aVisibility)
{
    NS_ENSURE_STATE(mBrowserWindow);
    
    if (aVisibility == mWindowVisible)
        return NS_OK;
        
    mWindowVisible = aVisibility;
    
    
    
    
    
    PRBool sizingToContent = mIsMainContent &&
                             (mChromeFlags & CHROME_OPENAS_CHROME) &&
                             (mChromeFlags & CHROME_OPENAS_DIALOG);
    if (sizingToContent && mWindowVisible && !mInitialLoadComplete)
        return NS_OK;    
    aVisibility ? mBrowserWindow->Show() : mBrowserWindow->Hide();
    
    return NS_OK;
}

NS_IMETHODIMP CBrowserChrome::GetTitle(PRUnichar * *aTitle)
{
   NS_ENSURE_STATE(mBrowserWindow);
   NS_ENSURE_ARG_POINTER(aTitle);

   Str255         pStr;
   nsAutoString   titleStr;
   
   mBrowserWindow->GetDescriptor(pStr);
   CPlatformUCSConversion::GetInstance()->PlatformToUCS(pStr, titleStr);
   *aTitle = ToNewUnicode(titleStr);
   
   return NS_OK;
}

NS_IMETHODIMP CBrowserChrome::SetTitle(const PRUnichar * aTitle)
{
    NS_ENSURE_STATE(mBrowserWindow);
    NS_ENSURE_ARG(aTitle);
    
    Str255          pStr;
	
    CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(aTitle), pStr);
    mBrowserWindow->SetDescriptor(pStr);
    
    return NS_OK;
}

NS_IMETHODIMP CBrowserChrome::GetSiteWindow(void * *aSiteWindow)
{
    NS_ENSURE_ARG(aSiteWindow);
    NS_ENSURE_STATE(mBrowserWindow);

    *aSiteWindow = mBrowserWindow->GetMacWindow();
    
    return NS_OK;
}






NS_IMETHODIMP CBrowserChrome::Blur(void)
{    
    WindowPtr   currWindow = ::GetWindowList();
    WindowPtr   nextWindow;

    
    while (currWindow && ((nextWindow = ::MacGetNextWindow(currWindow)) != nsnull))
        currWindow = nextWindow;

    WindowPtr ourWindow = mBrowserWindow->GetMacWindow();
    if (ourWindow != currWindow)
        ::SendBehind(ourWindow, currWindow);
    
    return NS_OK;
}





NS_IMETHODIMP CBrowserChrome::OnShowContextMenu(PRUint32 aContextFlags, nsIContextMenuInfo *aInfo)
{
    nsresult rv;
    
    try
    {
        rv = mBrowserShell->OnShowContextMenu(aContextFlags, aInfo);
    }
    catch (...)
    {
        rv = NS_ERROR_FAILURE;
    }
    return rv;
}





NS_IMETHODIMP CBrowserChrome::OnShowTooltip(PRInt32 aXCoords, PRInt32 aYCoords, const PRUnichar *aTipText)
{
    nsresult rv;
    
    try
    {
        rv = mBrowserShell->OnShowTooltip(aXCoords, aYCoords, aTipText);
    }
    catch (...)
    {
        rv = NS_ERROR_FAILURE;
    }
    return rv;
}

NS_IMETHODIMP CBrowserChrome::OnHideTooltip()
{
    nsresult rv;
    
    try
    {
        rv = mBrowserShell->OnHideTooltip();
    }
    catch (...)
    {
        rv = NS_ERROR_FAILURE;
    }
    return rv;
}





void CBrowserChrome::ListenToMessage(MessageT inMessage, void* ioParam)
{
    switch (inMessage)
    {
        case msg_OnNetStopChange:
            {
                mInitialLoadComplete = true;

                
                if (mIsMainContent &&
                    (mChromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME) &&
                    (mChromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_DIALOG))
                {
                    nsCOMPtr<nsIDOMWindow> domWindow;
                    (void)GetInterface(NS_GET_IID(nsIDOMWindow), getter_AddRefs(domWindow));
                    if (domWindow)
                        domWindow->SizeToContent();
                    if (mWindowVisible != mBrowserWindow->IsVisible())
                        mBrowserWindow->Show();
                }
                
                
                if (mChromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME)
                {
                    nsresult rv;
                    nsCOMPtr<nsIDOMWindow> domWindow;
                    rv = GetInterface(NS_GET_IID(nsIDOMWindow), getter_AddRefs(domWindow));
                    if (NS_FAILED(rv)) return;
                    nsCOMPtr<nsIDOMDocument> domDoc;
                    rv = domWindow->GetDocument(getter_AddRefs(domDoc));
                    if (NS_FAILED(rv)) return;
                    nsCOMPtr<nsIDOMElement> domDocElem;
                    rv = domDoc->GetDocumentElement(getter_AddRefs(domDocElem));
                    if (NS_FAILED(rv)) return;

                    nsAutoString windowTitle;
                    domDocElem->GetAttribute(NS_LITERAL_STRING("title"), windowTitle);
                    if (!windowTitle.IsEmpty()) {
                        Str255 pStr;
                        CPlatformUCSConversion::GetInstance()->UCSToPlatform(windowTitle, pStr);
                        mBrowserWindow->SetDescriptor(pStr);
                    }
                }
            }
            break;
    }
}





LWindow* CBrowserChrome::GetLWindowForDOMWindow(nsIDOMWindow* aDOMWindow)
{
    if (!aDOMWindow)
        return nsnull;
        
    nsCOMPtr<nsIWindowWatcher> windowWatcher(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
    if (!windowWatcher)
        return nsnull;
    nsCOMPtr<nsIWebBrowserChrome> windowChrome;
    windowWatcher->GetChromeForWindow(aDOMWindow, getter_AddRefs(windowChrome));
    if (!windowChrome)
        return nsnull;        
    nsCOMPtr<nsIEmbeddingSiteWindow> siteWindow(do_QueryInterface(windowChrome));
    if (!siteWindow)
        return nsnull;
    WindowPtr macWindow = nsnull;
    siteWindow->GetSiteWindow((void **)&macWindow);
    if (!macWindow)
        return nsnull;
    
    return LWindow::FetchWindowObject(macWindow);
}

