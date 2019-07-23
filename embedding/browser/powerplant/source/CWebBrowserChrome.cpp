








































#include "CWebBrowserChrome.h"
#include "CBrowserWindow.h"
#include "CBrowserShell.h"

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

#include "UMacUnicode.h"
#include "ApplIDs.h"

#include <LStaticText.h>
#include <LCheckBox.h>
#include <LEditText.h>
#include <UModalDialogs.h>
#include <LPushButton.h>




#define USE_BALLOONS_FOR_TOOL_TIPS 0 // Using balloons for this is really obnoxious


const PRInt32     kGrowIconSize = 15;





CWebBrowserChrome::CWebBrowserChrome() :
   mBrowserWindow(nsnull), mBrowserShell(nsnull),
   mPreviousBalloonState(false), mInModalLoop(false)
{
}

CWebBrowserChrome::~CWebBrowserChrome()
{
}





NS_IMPL_ADDREF(CWebBrowserChrome)
NS_IMPL_RELEASE(CWebBrowserChrome)

NS_INTERFACE_MAP_BEGIN(CWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
   NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChromeFocus)
   NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
   NS_INTERFACE_MAP_ENTRY(nsIEmbeddingSiteWindow)
   NS_INTERFACE_MAP_ENTRY(nsIContextMenuListener)
   NS_INTERFACE_MAP_ENTRY(nsITooltipListener)
   NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END





NS_IMETHODIMP CWebBrowserChrome::GetInterface(const nsIID &aIID, void** aInstancePtr)
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





NS_IMETHODIMP CWebBrowserChrome::SetStatus(PRUint32 statusType, const PRUnichar *status)
{
   NS_ENSURE_TRUE(mBrowserWindow, NS_ERROR_NOT_INITIALIZED);

   if (statusType == STATUS_SCRIPT) 
      mBrowserWindow->SetStatus(status);
   else if (statusType == STATUS_LINK)
      mBrowserWindow->SetOverLink(status);
  
   return NS_OK;
}

NS_IMETHODIMP CWebBrowserChrome::GetWebBrowser(nsIWebBrowser** aWebBrowser)
{
   NS_ENSURE_ARG_POINTER(aWebBrowser);
   NS_ENSURE_TRUE(mBrowserShell, NS_ERROR_NOT_INITIALIZED);

   mBrowserShell->GetWebBrowser(aWebBrowser);
   return NS_OK;
}

NS_IMETHODIMP CWebBrowserChrome::SetWebBrowser(nsIWebBrowser* aWebBrowser)
{
   NS_ENSURE_ARG(aWebBrowser);   
   NS_ENSURE_TRUE(mBrowserShell, NS_ERROR_NOT_INITIALIZED);

   mBrowserShell->SetWebBrowser(aWebBrowser);
   return NS_OK;
}

NS_IMETHODIMP CWebBrowserChrome::GetChromeFlags(PRUint32* aChromeMask)
{
   NS_ERROR("Haven't Implemented this yet");
   return NS_ERROR_FAILURE;
}

NS_IMETHODIMP CWebBrowserChrome::SetChromeFlags(PRUint32 aChromeMask)
{
   NS_ERROR("Haven't Implemented this yet");
   return NS_ERROR_FAILURE;
}

NS_IMETHODIMP CWebBrowserChrome::DestroyBrowserWindow()
{
    mInModalLoop = false;
    delete mBrowserWindow;
    return NS_OK;
}

NS_IMETHODIMP CWebBrowserChrome::IsWindowModal(PRBool *_retval)
{
    *_retval = PR_FALSE;
    return NS_OK;
}


NS_IMETHODIMP CWebBrowserChrome::SizeBrowserTo(PRInt32 aCX, PRInt32 aCY)
{
   CBrowserShell *browserShell = mBrowserWindow->GetBrowserShell();
   NS_ENSURE_TRUE(browserShell, NS_ERROR_NULL_POINTER);
   
   SDimension16 curSize;
   browserShell->GetFrameSize(curSize);
   mBrowserWindow->ResizeWindowBy(aCX - curSize.width, aCY - curSize.height);
   mBrowserWindow->SetSizeToContent(false);
   return NS_OK;
}


NS_IMETHODIMP CWebBrowserChrome::ShowAsModal(void)
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

NS_IMETHODIMP CWebBrowserChrome::ExitModalEventLoop(nsresult aStatus)
{
   mInModalLoop = false;
   return NS_OK;
}





NS_IMETHODIMP CWebBrowserChrome::FocusNextElement()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP CWebBrowserChrome::FocusPrevElement()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}





NS_IMETHODIMP CWebBrowserChrome::OnProgressChange(nsIWebProgress *progress, nsIRequest *request,
                                                  PRInt32 curSelfProgress, PRInt32 maxSelfProgress,
                                                  PRInt32 curTotalProgress, PRInt32 maxTotalProgress)
{
	NS_ENSURE_TRUE(mBrowserWindow, NS_OK);
	
   return mBrowserWindow->OnProgressChange(progress, request,
                                           curSelfProgress, maxSelfProgress,
                                           curTotalProgress, maxTotalProgress);
}

NS_IMETHODIMP CWebBrowserChrome::OnStateChange(nsIWebProgress *progress, nsIRequest *request,
                                               PRInt32 progressStateFlags, PRUint32 status)
{
	NS_ENSURE_TRUE(mBrowserWindow, NS_OK);
	
    if (progressStateFlags & STATE_IS_NETWORK) {
      if (progressStateFlags & STATE_START)
         mBrowserWindow->OnStatusNetStart(progress, request, progressStateFlags, status);
      else if (progressStateFlags & STATE_STOP)
	      mBrowserWindow->OnStatusNetStop(progress, request, progressStateFlags, status);
    }

   return NS_OK;
}


NS_IMETHODIMP CWebBrowserChrome::OnLocationChange(nsIWebProgress* aWebProgress,
                                                  nsIRequest* aRequest,
                                                  nsIURI *location)
{
	NS_ENSURE_TRUE(mBrowserWindow, NS_OK);

 
	nsCAutoString buf;
	if (location)
		location->GetSpec(buf);

	nsAutoString tmp;
	CopyUTF8toUTF16(buf, tmp);
	mBrowserWindow->SetLocation(tmp);

	return NS_OK;
}

NS_IMETHODIMP 
CWebBrowserChrome::OnStatusChange(nsIWebProgress* aWebProgress,
                                  nsIRequest* aRequest,
                                  nsresult aStatus,
                                  const PRUnichar* aMessage)
{
    return NS_OK;
}



NS_IMETHODIMP 
CWebBrowserChrome::OnSecurityChange(nsIWebProgress *aWebProgress, 
                                    nsIRequest *aRequest, 
                                    PRUint32 state)
{
    return NS_OK;
}






NS_IMETHODIMP CWebBrowserChrome::SetDimensions(PRUint32 flags, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy)
{
    NS_ENSURE_STATE(mBrowserWindow);

    nsresult rv = NS_OK;
    CBrowserShell *browserShell;
        
    if (flags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION)    
    {
        if (flags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER)
        {
            
            rv = NS_ERROR_UNEXPECTED;
        }
        else 
        {
            mBrowserWindow->MoveWindowTo(x, y);
        }
    }
    else                                                        
    {
        mBrowserWindow->SetSizeToContent(false);
        if (flags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER)
        {
            browserShell = mBrowserWindow->GetBrowserShell();
            NS_ENSURE_TRUE(browserShell, NS_ERROR_NULL_POINTER);
            SDimension16 curSize;
            browserShell->GetFrameSize(curSize);
            mBrowserWindow->ResizeWindowBy(cx - curSize.width, cy - curSize.height);
        }
        else 
        {
            if (mBrowserWindow->HasAttribute(windAttr_Resizable ))
                cy += 15;
            mBrowserWindow->ResizeWindowTo(cx, cy);
        }
    }
    return rv;
}

NS_IMETHODIMP CWebBrowserChrome::GetDimensions(PRUint32 flags, PRInt32 *x, PRInt32 *y, PRInt32 *cx, PRInt32 *cy)
{
    NS_ENSURE_STATE(mBrowserWindow);

    nsresult rv = NS_OK;
    CBrowserShell *browserShell;
    Rect bounds;
        
    if (flags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION)    
    {
        if (flags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER)
        {
            browserShell = mBrowserWindow->GetBrowserShell();
            NS_ENSURE_TRUE(browserShell, NS_ERROR_NULL_POINTER);
            SPoint32 curPos;
            browserShell->GetFrameLocation(curPos);
            if (x)
                *x = curPos.h;
            if (y)
                *y = curPos.v;
        }
        else 
        {
            mBrowserWindow->GetGlobalBounds(bounds);
            if (x)
                *x = bounds.left;
            if (y)
                *y = bounds.top;
        }
    }
    else                                                        
    {
        if (flags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER)
        {
            browserShell = mBrowserWindow->GetBrowserShell();
            NS_ENSURE_TRUE(browserShell, NS_ERROR_NULL_POINTER);
            SDimension16 curSize;
            browserShell->GetFrameSize(curSize);
            if (cx)
                *cx = curSize.width;
            if (cy)
                *cy = curSize.height;
        }
        else 
        {
            mBrowserWindow->GetGlobalBounds(bounds);
            if (cx)
                *cx = bounds.right - bounds.left;
            if (cy)
            {
                *cy = bounds.bottom - bounds.top;
                if (mBrowserWindow->HasAttribute(windAttr_Resizable ))
                    *cy -= 15;
            }
        }
    }
    return rv;
}

NS_IMETHODIMP CWebBrowserChrome::SetFocus()
{
    NS_ASSERTION(PR_FALSE, "Not Yet Implemented");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP CWebBrowserChrome::GetVisibility(PRBool *aVisibility)
{
    NS_ENSURE_STATE(mBrowserWindow);
    NS_ENSURE_ARG_POINTER(aVisibility);
    
    mBrowserWindow->GetVisibility(aVisibility);
    return NS_OK;
}

NS_IMETHODIMP CWebBrowserChrome::SetVisibility(PRBool aVisibility)
{
    NS_ENSURE_STATE(mBrowserWindow);
    
    mBrowserWindow->SetVisibility(aVisibility);
    return NS_OK;
}

NS_IMETHODIMP CWebBrowserChrome::GetTitle(PRUnichar * *aTitle)
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

NS_IMETHODIMP CWebBrowserChrome::SetTitle(const PRUnichar * aTitle)
{
    NS_ENSURE_STATE(mBrowserWindow);
    NS_ENSURE_ARG(aTitle);
    
    Str255          pStr;
	
    CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(aTitle), pStr);
    mBrowserWindow->SetDescriptor(pStr);
    return NS_OK;
}

NS_IMETHODIMP CWebBrowserChrome::GetSiteWindow(void * *aSiteWindow)
{
    NS_ENSURE_ARG(aSiteWindow);
    NS_ENSURE_STATE(mBrowserWindow);

    *aSiteWindow = mBrowserWindow->Compat_GetMacWindow();
    
    return NS_OK;
}





NS_IMETHODIMP CWebBrowserChrome::OnShowContextMenu(PRUint32 aContextFlags, nsIDOMEvent *aEvent, nsIDOMNode *aNode)
{
    nsresult rv;
	NS_ENSURE_TRUE(mBrowserWindow, NS_ERROR_NOT_INITIALIZED);
    
    try
    {
        rv = mBrowserWindow->OnShowContextMenu(aContextFlags, aEvent, aNode);
    }
    catch (...)
    {
        rv = NS_ERROR_FAILURE;
    }
    return rv;
}










CBrowserWindow*& CWebBrowserChrome::BrowserWindow()
{
   return mBrowserWindow;
}

CBrowserShell*& CWebBrowserChrome::BrowserShell()
{
   return mBrowserShell;
}

NS_IMETHODIMP
CWebBrowserChrome::OnShowTooltip(PRInt32 aXCoords, PRInt32 aYCoords, const PRUnichar *aTipText)
{
  nsCAutoString printable;
  CPlatformUCSConversion::GetInstance()->UCSToPlatform(nsDependentString(aTipText), printable);

#ifdef DEBUG
  printf("--------- SHOW TOOLTIP AT %ld %ld, |%s|\n", aXCoords, aYCoords, printable.get() );
#endif

#if USE_BALLOONS_FOR_TOOL_TIPS  
  Point where;
  ::GetMouse ( &where );
  ::LocalToGlobal ( &where );
  
  HMMessageRecord helpRec;
  helpRec.hmmHelpType = khmmString;
  helpRec.u.hmmString[0] = strlen(printable);
  memcpy ( &helpRec.u.hmmString[1], printable, strlen(printable) );
  
  mPreviousBalloonState = ::HMGetBalloons();
  ::HMSetBalloons ( true );
  OSErr err = ::HMShowBalloon ( &helpRec, where, NULL, NULL, 0, 0, 0 );
#endif
    
  return NS_OK;
}

NS_IMETHODIMP
CWebBrowserChrome::OnHideTooltip()
{
#ifdef DEBUG
  printf("--------- HIDE TOOLTIP\n");
#endif

#if USE_BALLOONS_FOR_TOOL_TIPS
  ::HMRemoveBalloon();
  ::HMSetBalloons ( mPreviousBalloonState );
#endif

  return NS_OK;
}
