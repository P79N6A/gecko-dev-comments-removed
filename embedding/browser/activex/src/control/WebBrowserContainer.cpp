





































#include "stdafx.h"

#include <limits.h>
#include <shlobj.h>

#include "WebBrowserContainer.h"

#include "nsServiceManagerUtils.h"
#include "nsIWebNavigationInfo.h"

CWebBrowserContainer::CWebBrowserContainer(CMozillaBrowser *pOwner) :
    mOwner(pOwner),
    mEvents1(mOwner),
    mEvents2(mOwner),
    mVisible(PR_TRUE)
{
}


CWebBrowserContainer::~CWebBrowserContainer()
{
}





NS_IMPL_ADDREF(CWebBrowserContainer)
NS_IMPL_RELEASE(CWebBrowserContainer)

NS_INTERFACE_MAP_BEGIN(CWebBrowserContainer)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIWebBrowserChrome)
    NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
    NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome)
    NS_INTERFACE_MAP_ENTRY(nsIURIContentListener)
    NS_INTERFACE_MAP_ENTRY(nsIEmbeddingSiteWindow)
    NS_INTERFACE_MAP_ENTRY(nsIEmbeddingSiteWindow2)
    NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
    NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
    NS_INTERFACE_MAP_ENTRY(nsIContextMenuListener)
    NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChromeFocus)

    NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END





NS_IMETHODIMP CWebBrowserContainer::GetInterface(const nsIID & aIID, void * *result)
{
    *result = 0;
    if (aIID.Equals(NS_GET_IID(nsIDOMWindow)))
    {
        if (mOwner && mOwner->mWebBrowser)
        {
            return mOwner->mWebBrowser->GetContentDOMWindow((nsIDOMWindow **) result);
        }
        return NS_ERROR_NOT_INITIALIZED;
    }
    return QueryInterface(aIID, result);
}





NS_IMETHODIMP CWebBrowserContainer::OnShowContextMenu(PRUint32 aContextFlags, nsIDOMEvent *aEvent, nsIDOMNode *aNode)
{
    mOwner->ShowContextMenu(aContextFlags, aEvent, aNode);
    return NS_OK;
}






NS_IMETHODIMP CWebBrowserContainer::OnProgressChange(nsIWebProgress *aProgress, nsIRequest *aRequest, PRInt32 curSelfProgress, PRInt32 maxSelfProgress, PRInt32 curTotalProgress, PRInt32 maxTotalProgress)
{
    NG_TRACE(_T("CWebBrowserContainer::OnProgressChange(...)\n"));
    
    long nProgress = curTotalProgress;
    long nProgressMax = maxTotalProgress;

    if (nProgress == 0)
    {
    }
    if (nProgressMax == 0)
    {
        nProgressMax = LONG_MAX;
    }
    if (nProgress > nProgressMax)
    {
        nProgress = nProgressMax; 
    }

    mEvents1->Fire_ProgressChange(nProgress, nProgressMax);
    mEvents2->Fire_ProgressChange(nProgress, nProgressMax);

    return NS_OK;
}


NS_IMETHODIMP CWebBrowserContainer::OnStateChange(nsIWebProgress* aWebProgress, nsIRequest *aRequest, PRUint32 progressStateFlags, nsresult aStatus)
{
    nsresult rv = NS_OK;

    NG_TRACE(_T("CWebBrowserContainer::OnStateChange(...)\n"));

    BOOL doFireCommandStateChange = FALSE;

    
    if (progressStateFlags & STATE_IS_DOCUMENT)
    {
        if (progressStateFlags & STATE_START)
        {
            NG_TRACE(_T("CWebBrowserContainer::OnStateChange->Doc Start(...,  \"\")\n"));

            nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
            if (channel)
            {
                nsCOMPtr<nsIURI> uri;
                rv = channel->GetURI(getter_AddRefs(uri));
                if (NS_SUCCEEDED(rv))
                {
                    nsCAutoString aURI;
                    uri->GetAsciiSpec(aURI);
                    NG_TRACE(_T("CWebBrowserContainer::OnStateChange->Doc Start(..., %s, \"\")\n"), A2CT(aURI.get()));
                }
            }

            
            mEvents1->Fire_DownloadBegin();
            mEvents2->Fire_DownloadBegin();
        }
        else if (progressStateFlags & STATE_STOP)
        {
            NG_TRACE(_T("CWebBrowserContainer::OnStateChange->Doc Stop(...,  \"\")\n"));

            if (mOwner->mIERootDocument)
            {
                
                mOwner->mIERootDocument->Release();
                mOwner->mIERootDocument = NULL;
            }

            
            mEvents1->Fire_DownloadComplete();
            mEvents2->Fire_DownloadComplete();

            nsCOMPtr<nsIURI> pURI;

            nsCOMPtr<nsIChannel> aChannel = do_QueryInterface(aRequest);
            if (!aChannel) return NS_ERROR_NULL_POINTER;

            rv = aChannel->GetURI(getter_AddRefs(pURI));
            if (NS_FAILED(rv)) return NS_OK;

            nsCAutoString aURI;
            rv = pURI->GetAsciiSpec(aURI);
            if (NS_FAILED(rv)) return NS_OK;

            USES_CONVERSION;
            BSTR bstrURI = SysAllocString(A2OLE(aURI.get())); 
                
            
            CComVariant vURI(bstrURI);
            mEvents2->Fire_DocumentComplete(mOwner, &vURI);
            SysFreeString(bstrURI);

            
            BSTR bstrStatus = SysAllocString(A2OLE((CHAR *) "Done"));
            mEvents1->Fire_StatusTextChange(bstrStatus);
            mEvents2->Fire_StatusTextChange(bstrStatus);
            SysFreeString(bstrStatus);
        }

        
        doFireCommandStateChange = TRUE;
    }

    if (progressStateFlags & STATE_IS_NETWORK)
    {
        if (progressStateFlags & STATE_START)
        {
        }

        if (progressStateFlags & STATE_STOP)
        {
            nsCAutoString aURI;
            if (mCurrentURI)
            {
                mCurrentURI->GetAsciiSpec(aURI);
            }

            
            USES_CONVERSION;
            BSTR bstrURI = SysAllocString(A2OLE(aURI.get()));
            mEvents1->Fire_NavigateComplete(bstrURI);

            
            CComVariant vURI(bstrURI);
            mEvents2->Fire_NavigateComplete2(mOwner, &vURI);

            
            SysFreeString(bstrURI);
            mOwner->mBusyFlag = FALSE;
            mCurrentURI = nsnull;
        }
    }

    if (doFireCommandStateChange)
    {
        nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mOwner->mWebBrowser));

        
        VARIANT_BOOL bEnableForward = VARIANT_FALSE;
        PRBool aCanGoForward = PR_FALSE;
        webNav->GetCanGoForward(&aCanGoForward);
        if (aCanGoForward == PR_TRUE)
        {
            bEnableForward = VARIANT_TRUE;
        }
        mEvents2->Fire_CommandStateChange(CSC_NAVIGATEFORWARD, bEnableForward);

        
        VARIANT_BOOL bEnableBack = VARIANT_FALSE;
        PRBool aCanGoBack = PR_FALSE;
        webNav->GetCanGoBack(&aCanGoBack);
        if (aCanGoBack == PR_TRUE)
        {
            bEnableBack = VARIANT_TRUE;
        }
        mEvents2->Fire_CommandStateChange(CSC_NAVIGATEBACK, bEnableBack);
    }

    return NS_OK;
}



NS_IMETHODIMP CWebBrowserContainer::OnLocationChange(nsIWebProgress* aWebProgress,
                                                     nsIRequest* aRequest,
                                                     nsIURI *location)
{


    return NS_OK;
}

NS_IMETHODIMP 
CWebBrowserContainer::OnStatusChange(nsIWebProgress* aWebProgress,
                                     nsIRequest* aRequest,
                                     nsresult aStatus,
                                     const PRUnichar* aMessage)
{
    NG_TRACE(_T("CWebBrowserContainer::OnStatusChange(...,  \"\")\n"));

    BSTR bstrStatus = SysAllocString(W2OLE((PRUnichar *) aMessage));
    mEvents1->Fire_StatusTextChange(bstrStatus);
    mEvents2->Fire_StatusTextChange(bstrStatus);
    SysFreeString(bstrStatus);

    return NS_OK;
}

NS_IMETHODIMP 
CWebBrowserContainer::OnSecurityChange(nsIWebProgress *aWebProgress, 
                                       nsIRequest *aRequest, 
                                       PRUint32 state)
{
    return NS_OK;
}






NS_IMETHODIMP CWebBrowserContainer::OnStartURIOpen(nsIURI *pURI, PRBool *aAbortOpen)
{
    USES_CONVERSION;
    NG_TRACE(_T("CWebBrowserContainer::OnStartURIOpen(...)\n"));

    mCurrentURI = pURI;
    NG_ASSERT(mCurrentURI);

    nsCAutoString aURI;
    mCurrentURI->GetSpec(aURI);

    
    CComVariant vPostDataRef;
    CComVariant vPostData;
    vPostDataRef.vt = VT_BYREF | VT_VARIANT;
    vPostDataRef.pvarVal = &vPostData;
    


    
    BSTR bstrURI = SysAllocString(A2OLE(aURI.get()));
    BSTR bstrTargetFrameName = NULL;
    BSTR bstrHeaders = NULL;
    VARIANT_BOOL bCancel = VARIANT_FALSE;
    long lFlags = 0;

    mEvents1->Fire_BeforeNavigate(bstrURI, lFlags, bstrTargetFrameName, &vPostDataRef, bstrHeaders, &bCancel);

    
    CComVariant vURI(bstrURI);
    CComVariant vFlags(lFlags);
    CComVariant vTargetFrameName(bstrTargetFrameName);
    CComVariant vHeaders(bstrHeaders);

    mEvents2->Fire_BeforeNavigate2(mOwner, &vURI, &vFlags, &vTargetFrameName, &vPostDataRef, &vHeaders, &bCancel);

    
    SysFreeString(bstrURI);
    SysFreeString(bstrTargetFrameName);
    SysFreeString(bstrHeaders);

    if (bCancel != VARIANT_FALSE)
    {
        *aAbortOpen = PR_TRUE;
        return NS_ERROR_ABORT;
    }
    else
    {
        mOwner->mBusyFlag = TRUE;
    }

    
    
    
    
    
    
    
    
    
    
    

    mEvents1->Fire_DownloadBegin();
    mEvents2->Fire_DownloadBegin();
    mEvents1->Fire_DownloadComplete();
    mEvents2->Fire_DownloadComplete();

    return NS_OK;
}


NS_IMETHODIMP CWebBrowserContainer::DoContent(const char *aContentType, PRBool aIsContentPreferred, nsIRequest *request, nsIStreamListener **aContentHandler, PRBool *aAbortProcess)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP CWebBrowserContainer::IsPreferred(const char *aContentType, char **aDesiredContentType, PRBool *_retval)
{
    return CanHandleContent(aContentType, PR_TRUE, aDesiredContentType, _retval);
}



NS_IMETHODIMP CWebBrowserContainer::CanHandleContent(const char *aContentType, PRBool aIsContentPreferred, char **aDesiredContentType, PRBool *_retval)
{
    *_retval = PR_FALSE;
    *aDesiredContentType = nsnull;
    
    if (aContentType)
    {
        nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mOwner->mWebBrowser));
        nsCOMPtr<nsIWebNavigationInfo> webNavInfo(
           do_GetService("@mozilla.org/webnavigation-info;1"));
        if (webNavInfo)
        {
            PRUint32 canHandle;
            nsresult rv =
                webNavInfo->IsTypeSupported(nsDependentCString(aContentType),
                                            webNav,
                                            &canHandle);
            NS_ENSURE_SUCCESS(rv, rv);
            *_retval = (canHandle != nsIWebNavigationInfo::UNSUPPORTED);
        }
    }

    return NS_OK;
}



NS_IMETHODIMP CWebBrowserContainer::GetLoadCookie(nsISupports * *aLoadCookie)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP CWebBrowserContainer::SetLoadCookie(nsISupports * aLoadCookie)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP CWebBrowserContainer::GetParentContentListener(nsIURIContentListener * *aParentContentListener)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP CWebBrowserContainer::SetParentContentListener(nsIURIContentListener * aParentContentListener)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}





NS_IMETHODIMP 
CWebBrowserContainer::GetDimensions(PRUint32 aFlags, PRInt32 *x, PRInt32 *y, PRInt32 *cx, PRInt32 *cy)
{
    RECT rc = { 0, 0, 1, 1 };
    mOwner->GetClientRect(&rc);
    if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION)
    {
        if (*x)
            *x = rc.left;
        if (*y)
            *y = rc.top;
    }
    if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER ||
        aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER)
    {
        if (*cx)
            *cx = rc.right - rc.left;
        if (*cy)
            *cy = rc.bottom - rc.top;
    }
    return NS_OK;
}


NS_IMETHODIMP 
CWebBrowserContainer::SetDimensions(PRUint32 aFlags, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy)
{
    
    return NS_OK;
}


NS_IMETHODIMP 
CWebBrowserContainer::GetSiteWindow(void **aParentNativeWindow)
{
    NS_ENSURE_ARG_POINTER(aParentNativeWindow);
    HWND *hwndDest = (HWND *) aParentNativeWindow;
    *hwndDest = mOwner->m_hWnd;
    return NS_OK;
}


NS_IMETHODIMP 
CWebBrowserContainer::SetFocus(void)
{
    return NS_OK;
}


NS_IMETHODIMP 
CWebBrowserContainer::GetTitle(PRUnichar * *aTitle)
{
    NG_ASSERT_POINTER(aTitle, PRUnichar **);
    if (!aTitle)
        return E_INVALIDARG;

    *aTitle = ToNewUnicode(mTitle);

    return NS_OK;
}


NS_IMETHODIMP 
CWebBrowserContainer::SetTitle(const PRUnichar * aTitle)
{
    NG_ASSERT_POINTER(aTitle, PRUnichar *);
    if (!aTitle)
        return E_INVALIDARG;

    mTitle = aTitle;
    
    BSTR bstrTitle = SysAllocString(aTitle);
    mEvents1->Fire_TitleChange(bstrTitle);
    mEvents2->Fire_TitleChange(bstrTitle);
    SysFreeString(bstrTitle);

    return NS_OK;
}


NS_IMETHODIMP 
CWebBrowserContainer::GetVisibility(PRBool *aVisibility)
{
    NS_ENSURE_ARG_POINTER(aVisibility);
    *aVisibility = PR_TRUE;
    return NS_OK;
}


NS_IMETHODIMP 
CWebBrowserContainer::SetVisibility(PRBool aVisibility)
{
    VARIANT_BOOL visible = aVisibility ? VARIANT_TRUE : VARIANT_FALSE;
    mVisible = aVisibility;
    
    mEvents2->Fire_OnVisible(visible);
    return NS_OK;
}






NS_IMETHODIMP 
CWebBrowserContainer::Blur()
{
    return NS_OK;
}





NS_IMETHODIMP
CWebBrowserContainer::FocusNextElement()
{
    ATLTRACE(_T("CWebBrowserContainer::FocusNextElement()\n"));
    mOwner->NextDlgControl();
    return NS_OK;
}


NS_IMETHODIMP
CWebBrowserContainer::FocusPrevElement()
{
    ATLTRACE(_T("CWebBrowserContainer::FocusPrevElement()\n"));
    mOwner->PrevDlgControl();
    return NS_OK;
}





NS_IMETHODIMP
CWebBrowserContainer::SetStatus(PRUint32 statusType, const PRUnichar *status)
{
    
    BSTR bstrStatus = SysAllocString(status);
    mEvents1->Fire_StatusTextChange(bstrStatus);
    mEvents2->Fire_StatusTextChange(bstrStatus);
    SysFreeString(bstrStatus);
    return NS_OK;
}


NS_IMETHODIMP
CWebBrowserContainer::GetWebBrowser(nsIWebBrowser * *aWebBrowser)
{
    return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
CWebBrowserContainer::SetWebBrowser(nsIWebBrowser * aWebBrowser)
{
    return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
CWebBrowserContainer::GetChromeFlags(PRUint32 *aChromeFlags)
{
    return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
CWebBrowserContainer::SetChromeFlags(PRUint32 aChromeFlags)
{
    return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
CWebBrowserContainer::DestroyBrowserWindow(void)
{
    return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
CWebBrowserContainer::SizeBrowserTo(PRInt32 aCX, PRInt32 aCY)
{
    return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
CWebBrowserContainer::ShowAsModal(void)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
CWebBrowserContainer::IsWindowModal(PRBool *_retval)
{
    
    *_retval = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
CWebBrowserContainer::ExitModalEventLoop(nsresult aStatus)
{
    
    return NS_OK;
}





NS_IMETHODIMP
CWebBrowserContainer::OnStartRequest(nsIRequest *request, nsISupports* aContext)
{
    USES_CONVERSION;
    NG_TRACE(_T("CWebBrowserContainer::OnStartRequest(...)\n"));

    return NS_OK;
}


NS_IMETHODIMP
CWebBrowserContainer::OnStopRequest(nsIRequest *request, nsISupports* aContext, nsresult aStatus)
{
    USES_CONVERSION;
    NG_TRACE(_T("CWebBrowserContainer::OnStopRequest(..., %d)\n"), (int) aStatus);

    
    mEvents1->Fire_DownloadComplete();
    mEvents2->Fire_DownloadComplete();

    return NS_OK;
}





NS_IMETHODIMP CWebBrowserContainer::Exec(const char *aCommand, const char *aStatus, char **aResult)
{
    return NS_OK;
}


NS_IMETHODIMP CWebBrowserContainer::Query(const char *aCommand, const char *aStatus, char **aResult)
{
    return NS_OK;
}
