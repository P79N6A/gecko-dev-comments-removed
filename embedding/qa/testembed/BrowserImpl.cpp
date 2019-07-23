













































































#ifdef _WINDOWS
  #include "stdafx.h"
#endif

#include "nsIDOMWindow.h"
#include "BrowserImpl.h"

#include "QaUtils.h"

#include "nsirequest.h"
#include "Tests.h"
#include "prmem.h"
#include "nsichanneltests.h"
#include "nsihttpchanneltests.h"

CBrowserImpl::CBrowserImpl()
{
  m_pBrowserFrameGlue = NULL;
  mWebBrowser = nsnull;
}


CBrowserImpl::~CBrowserImpl()
{
}

extern storage getSupportObj;




NS_METHOD CBrowserImpl::Init(PBROWSERFRAMEGLUE pBrowserFrameGlue, 
                             nsIWebBrowser* aWebBrowser)
{
	  m_pBrowserFrameGlue = pBrowserFrameGlue;
	  
	  SetWebBrowser(aWebBrowser);

	  return NS_OK;
}





NS_IMPL_ADDREF(CBrowserImpl)
NS_IMPL_RELEASE(CBrowserImpl)

NS_INTERFACE_MAP_BEGIN(CBrowserImpl)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
   NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChromeFocus)
   NS_INTERFACE_MAP_ENTRY(nsIEmbeddingSiteWindow)
   NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
   NS_INTERFACE_MAP_ENTRY(nsIContextMenuListener)
   NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
   NS_INTERFACE_MAP_ENTRY(nsISHistoryListener) 
   NS_INTERFACE_MAP_ENTRY(nsIStreamListener) 
   NS_INTERFACE_MAP_ENTRY(nsIRequestObserver) 
   NS_INTERFACE_MAP_ENTRY(nsITooltipListener) 
   NS_INTERFACE_MAP_ENTRY(nsIURIContentListener) 

NS_INTERFACE_MAP_END





NS_IMETHODIMP CBrowserImpl::GetInterface(const nsIID &aIID, void** aInstancePtr)
{
	if(aIID.Equals(NS_GET_IID(nsIDOMWindow)))
	{
		if (mWebBrowser)
			return mWebBrowser->GetContentDOMWindow((nsIDOMWindow **) aInstancePtr);
		return NS_ERROR_NOT_INITIALIZED;
	}

	return QueryInterface(aIID, aInstancePtr);
}







NS_IMETHODIMP CBrowserImpl::SetStatus(PRUint32 aType, const PRUnichar* aStatus)
{
    QAOutput("\n", 1);
    QAOutput("inside nsIWebBrowserChrome::SetStatus().", 1);
	FormatAndPrintOutput("SetStatus() type = ", aType, 1);
	FormatAndPrintOutput("SetStatus() aStatus = ", *aStatus, 1);

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	m_pBrowserFrameGlue->UpdateStatusBarText(aStatus);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetWebBrowser(nsIWebBrowser** aWebBrowser)
{
   QAOutput("inside nsIWebBrowserChrome::GetWebBrowser().", 1);

   NS_ENSURE_ARG_POINTER(aWebBrowser);

   *aWebBrowser = mWebBrowser;
   if (!aWebBrowser)
      QAOutput("aWebBrowser is null", 1);

   NS_IF_ADDREF(*aWebBrowser);

   return NS_OK;
}




NS_IMETHODIMP CBrowserImpl::SetWebBrowser(nsIWebBrowser* aWebBrowser)
{
   QAOutput("inside nsIWebBrowserChrome::SetWebBrowser().", 1);

   NS_ENSURE_ARG_POINTER(aWebBrowser);

   if (!aWebBrowser)
      QAOutput("aWebBrowser is null", 1);

   mWebBrowser = aWebBrowser;

   return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetChromeFlags(PRUint32* aChromeMask)
{
    QAOutput("inside nsIWebBrowserChrome::GetChromeFlags().", 1);

	*aChromeMask = nsIWebBrowserChrome::CHROME_ALL;
	FormatAndPrintOutput("GetChromeFlags() chromeMask = ", *aChromeMask, 1);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetChromeFlags(PRUint32 aChromeMask)
{
    QAOutput("nsIWebBrowserChrome::SetChromeFlags().", 1);

	FormatAndPrintOutput("SetChromeFlags() chromeMask = ", aChromeMask, 1);

	mChromeMask = aChromeMask;

	return NS_OK;
}





























NS_IMETHODIMP CBrowserImpl::DestroyBrowserWindow()
{
	if(! m_pBrowserFrameGlue)
	{
		QAOutput("inside nsIWebBrowserChrome::DestroyBrowserWindow(): Browser Window not destroyed.", 1);
		return NS_ERROR_FAILURE;
	}

	m_pBrowserFrameGlue->DestroyBrowserFrame();
	QAOutput("nsIWebBrowserChrome::DestroyBrowserWindow(): Browser Window destroyed.", 1);

	return NS_OK;
}










NS_IMETHODIMP CBrowserImpl::SizeBrowserTo(PRInt32 aCX, PRInt32 aCY)
{
    QAOutput("\n", 1);
	QAOutput("inside nsIWebBrowserChrome::SizeBrowserTo(): Browser sized.", 1);

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	FormatAndPrintOutput("SizeBrowserTo() x coordinate = ", aCX, 1);
	FormatAndPrintOutput("SizeBrowserTo() y coordinate = ", aCY, 1);

	m_pBrowserFrameGlue->SetBrowserFrameSize(aCX, aCY);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::ShowAsModal(void)
{
	QAOutput("inside nsIWebBrowserChrome::ShowAsModal()", 2);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::IsWindowModal(PRBool *retval)
{
  QAOutput("inside nsIWebBrowserChrome::IsWindowModal()", 1);

  
  *retval = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::ExitModalEventLoop(nsresult aStatus)
{
  QAOutput("inside nsIWebBrowserChrome::ExitModalEventLoop()", 1);
  RvTestResult(aStatus, "ExitModalEventLoop status test", 1);

  return NS_OK;
}





NS_IMETHODIMP CBrowserImpl::FocusNextElement()
{
	QAOutput("inside nsIWebBrowserChromeFocus::FocusNextElement()", 1);

    return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::FocusPrevElement()
{
	QAOutput("inside nsIWebBrowserChromeFocus::FocusPrevElement()", 1);

    return NS_OK;
}





NS_IMETHODIMP CBrowserImpl::SetDimensions(PRUint32 aFlags, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy)
{
    QAOutput("\n", 1);
	QAOutput("inside nsIEmbeddingSiteWindow::SetDimensions()", 1);

	FormatAndPrintOutput("SetDimensions() flags = ", aFlags, 1);
	FormatAndPrintOutput("SetDimensions() x1 coordinate = ", x, 1);
	FormatAndPrintOutput("SetDimensions() y1 coordinate = ", y, 1);
	FormatAndPrintOutput("SetDimensions() x2 coordinate = ", cx, 1);
	FormatAndPrintOutput("SetDimensions() y2 coordinate = ", cy, 1);

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

    if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION &&
        (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER || 
         aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER))
    {
    	m_pBrowserFrameGlue->SetBrowserFramePositionAndSize(x, y, cx, cy, PR_TRUE);
    }
    else
    {
        if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION)
        {
    	    m_pBrowserFrameGlue->SetBrowserFramePosition(x, y);
        }
        if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER || 
            aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER)
        {
    	    m_pBrowserFrameGlue->SetBrowserFrameSize(cx, cy);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetDimensions(PRUint32 aFlags, PRInt32 *x, PRInt32 *y, PRInt32 *cx, PRInt32 *cy)
{
    QAOutput("\n", 1);
	QAOutput("inside nsIEmbeddingSiteWindow::GetDimensions()", 1);

	FormatAndPrintOutput("GetDimensions() flags = ", aFlags, 1);
	FormatAndPrintOutput("GetDimensions() x1 coordinate = ", *x, 1);
	FormatAndPrintOutput("GetDimensions() y1 coordinate = ", *y, 1);
	FormatAndPrintOutput("GetDimensions() x2 coordinate = ", *cx, 1);
	FormatAndPrintOutput("GetDimensions() y2 coordinate = ", *cy, 1);

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;
    
    if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION)
    {
    	m_pBrowserFrameGlue->GetBrowserFramePosition(x, y);
    }
    if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER || 
        aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER)
    {
    	m_pBrowserFrameGlue->GetBrowserFrameSize(cx, cy);
    }

    return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetSiteWindow(void** aSiteWindow)
{
  QAOutput("inside nsIEmbeddingSiteWindow::GetSiteWindow()", 1);

  if (!aSiteWindow) {
	QAOutput("GetSiteWindow: Didn't get siteWindow.");
    return NS_ERROR_NULL_POINTER;
  }

  *aSiteWindow = 0;
  if (m_pBrowserFrameGlue) {
    HWND w = m_pBrowserFrameGlue->GetBrowserFrameNativeWnd();
    *aSiteWindow = reinterpret_cast<void *>(w);
  }
  return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetFocus()
{
    QAOutput("inside nsIEmbeddingSiteWindow::SetFocus()", 1);

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	m_pBrowserFrameGlue->SetFocus();

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetTitle(PRUnichar** aTitle)
{
    QAOutput("inside nsIEmbeddingSiteWindow::GetTitle()", 1);

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	m_pBrowserFrameGlue->GetBrowserFrameTitle(aTitle);
	FormatAndPrintOutput("GetTitle() title = ", **aTitle, 1);
	
	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetTitle(const PRUnichar* aTitle)
{
    QAOutput("inside nsIEmbeddingSiteWindow::SetTitle()", 1);
	FormatAndPrintOutput("SetTitle() title = ", *aTitle, 1);

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	m_pBrowserFrameGlue->SetBrowserFrameTitle(aTitle);
	
	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetVisibility(PRBool *aVisibility)
{
    QAOutput("inside nsIEmbeddingSiteWindow::GetVisibility()", 1);

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

    m_pBrowserFrameGlue->GetBrowserFrameVisibility(aVisibility);
	FormatAndPrintOutput("GetVisibility() boolean = ", *aVisibility, 1);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetVisibility(PRBool aVisibility)
{
    QAOutput("inside nsIEmbeddingSiteWindow::SetVisibility()", 1);
	FormatAndPrintOutput("SetVisibility() boolean = ", aVisibility, 1);

    if(! m_pBrowserFrameGlue)
        return NS_ERROR_FAILURE;

    m_pBrowserFrameGlue->ShowBrowserFrame(aVisibility);

    return NS_OK;
}






NS_IMETHODIMP CBrowserImpl::OnDataAvailable(nsIRequest *request,
				nsISupports *ctxt, nsIInputStream *input,
				PRUint32 offset, PRUint32 count)
{
	nsCString stringMsg;
	PRUint32 readLen;
	QAOutput("\n");
	QAOutput("##### inside nsIStreamListener::OnDataAvailable(). #####");

	RequestName(request, stringMsg, 1);
	readLen = count;
		
	char *buf = (char *)PR_Malloc(count);
	if (!input)
		QAOutput("We didn't get the nsIInputStream object.", 1);
	else {
		
		rv = input->Read(buf, count, &readLen);
		RvTestResult(rv, "nsIInputStream->Read() consumer", 1);
	}

	FormatAndPrintOutput("OnDataAvailable() offset = ", offset, 1);
	FormatAndPrintOutput("OnDataAvailable() count = ", count, 1);

	if (!ctxt)
		QAOutput("OnDataAvailable():We didn't get the nsISupports object.", 1);
	else
		QAOutput("OnDataAvailable():We got the nsISupports object.", 1);

	return NS_OK;
}






NS_IMETHODIMP CBrowserImpl::OnStartRequest(nsIRequest *request,
				nsISupports *ctxt)
{
	nsCString stringMsg;

	QAOutput("\n");
	QAOutput("##### BEGIN: nsIRequestObserver::OnStartRequest() #####");

	if (!ctxt)
		QAOutput("We did NOT get the context object.\n");

	if (ctxt == getSupportObj.sup)
		QAOutput("Context objects equal each other.\n");
	else
		QAOutput("Context objects don't equal each other.\n");

	RequestName(request, stringMsg, 1);

	
	nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
	nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(channel);
	CBrowserImpl *aBrowserImpl = new CBrowserImpl();
	CnsIChannelTests  *channelTests = new CnsIChannelTests(mWebBrowser, aBrowserImpl);
	CnsIHttpChannelTests *httpChannelTests = new CnsIHttpChannelTests(mWebBrowser, aBrowserImpl);
	if (channelTests && channel) {
		QAOutput("\n  Start nsIChannel PostAsyncOpenTests tests:");
		channelTests->PostAsyncOpenTests(channel, 1);
	}
	else if (!channelTests)
		QAOutput("No object to run PostAsyncOpenTests() for nsIChannel.", 1);

	if (!httpChannelTests)
		QAOutput("No object to run CallResponseTests() for nsIHttpChannel.", 1);
	else
	{
		QAOutput("\n  Start nsIHttpChannel response tests:");
		httpChannelTests->CallResponseTests(httpChannel, 1);
	}

	if (!ctxt)
		QAOutput("OnStartRequest():We didn't get the nsISupports object.", 1);
	else
		QAOutput("OnStartRequest():We got the nsISupports object.", 1);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::OnStopRequest(nsIRequest *request,
				nsISupports *ctxt, nsresult rv)
{
	nsCString stringMsg;
	QAOutput("\n");
	RvTestResult(rv, "nsIRequestObserver::OnStopRequest rv input", 1);
	RequestName(request, stringMsg, 1);

	if (!ctxt)
		QAOutput("OnStopRequest():We didn't get the nsISupports object.", 1);
	else
		QAOutput("OnStopRequest():We got the nsISupports object.", 1);

	RvTestResult(rv, "OnStopRequest() rv test", 1);

	QAOutput("##### END: nsIStreamListener::OnStopRequest() #####");
		
	return NS_OK;
}





NS_IMETHODIMP CBrowserImpl::OnShowTooltip(PRInt32 aXCoords, PRInt32 aYCoords,
										  const PRUnichar *aTipText)
{
    if(! m_pBrowserFrameGlue)
        return NS_ERROR_FAILURE;

    m_pBrowserFrameGlue->ShowTooltip(aXCoords, aYCoords, aTipText);

	QAOutput("nsITooltipListener->OnShowTooltip()",1);
	FormatAndPrintOutput("OnShowTooltip() aXCoords = ", aXCoords, 1);
	FormatAndPrintOutput("OnShowTooltip() aYCoords = ", aYCoords, 1);
	FormatAndPrintOutput("OnShowTooltip() aTipText = ", *aTipText, 1);	
	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::OnHideTooltip() 
{
    if(! m_pBrowserFrameGlue)
        return NS_ERROR_FAILURE;

    m_pBrowserFrameGlue->HideTooltip();
	QAOutput("nsITooltipListener->OnHideTooltip()",1);
	return NS_OK;
}






NS_IMETHODIMP CBrowserImpl::OnStartURIOpen(nsIURI *aURI, PRBool *_retval)
{
	QAOutput("nsIURIContentListener->OnStartURIOpen()",1);

	GetTheURI(aURI, 1);
	
	*_retval = PR_FALSE;
	FormatAndPrintOutput("_retval set to = ", *_retval, 1);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::DoContent(const char *aContentType, PRBool aIsContentPreferred, nsIRequest *aRequest, nsIStreamListener **aContentHandler, PRBool *_retval)
{
	nsCString stringMsg;

	QAOutput("nsIURIContentListener->DoContent()",1);

	FormatAndPrintOutput("DoContent() content type = ", *aContentType, 1);
	FormatAndPrintOutput("DoContent() aIsContentPreferred = ", aIsContentPreferred, 1);
	RequestName(aRequest, stringMsg);	

	QueryInterface(NS_GET_IID(nsIStreamListener), (void **) aContentHandler);

	*_retval = PR_FALSE;
	FormatAndPrintOutput("_retval set to = ", *_retval, 1);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::IsPreferred(const char *aContentType, char **aDesiredContentType, PRBool *_retval)
{
	nsCAutoString contentStr;

	QAOutput("nsIURIContentListener->IsPreferred()",1);

	FormatAndPrintOutput("IsPreferred() content type = ", *aContentType, 1);
	*aDesiredContentType = nsCRT::strdup("text/html");
	FormatAndPrintOutput("aDesiredContentType set to = ", *aDesiredContentType, 1);
	*_retval = PR_TRUE;
	FormatAndPrintOutput("_retval set to = ", *_retval, 1);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::CanHandleContent(const char *aContentType, PRBool aIsContentPreferred, char **aDesiredContentType, PRBool *_retval)
{
	QAOutput("nsIURIContentListener->CanHandleContent()",1);

	FormatAndPrintOutput("CanHandleContent() content type = ", *aContentType, 1);
	FormatAndPrintOutput("CanHandleContent() preferred content type = ", aIsContentPreferred, 1);
	*aDesiredContentType = nsCRT::strdup("text/html");
	FormatAndPrintOutput("aDesiredContentType set to = ", *aDesiredContentType, 1);
	*_retval = PR_TRUE;
	FormatAndPrintOutput("_retval set to = ", *_retval, 1);
	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetLoadCookie(nsISupports **aLoadCookie)
{
	QAOutput("nsIURIContentListener->GetLoadCookie()",1);

	if (!aLoadCookie)
		QAOutput("aLoadCookie object is null",1);
	*aLoadCookie = mLoadCookie;
	NS_IF_ADDREF(*aLoadCookie);

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetLoadCookie(nsISupports * aLoadCookie)
{
	QAOutput("nsIURIContentListener->SetLoadCookie()",1);

	if (!aLoadCookie)
		QAOutput("aLoadCookie object is null",1);
	mLoadCookie = aLoadCookie;

	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::GetParentContentListener(nsIURIContentListener **aParentContentListener)
{
	QAOutput("nsIURIContentListener->GetParentContentListener()",1);

	if (!aParentContentListener)
		QAOutput("aParentContentListener object is null",1);
	*aParentContentListener = mParentContentListener;
	NS_IF_ADDREF(*aParentContentListener);



	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::SetParentContentListener(nsIURIContentListener * aParentContentListener)
{
	QAOutput("nsIURIContentListener->SetParentContentListener()",1);

	if (!aParentContentListener)
		QAOutput("aParentContentListener object is null",1);
	mParentContentListener = aParentContentListener;

	return NS_OK;	
}
