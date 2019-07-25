





































#include "nsDocShell.h"
#include "nsDSURIContentListener.h"
#include "nsIChannel.h"
#include "nsServiceManagerUtils.h"
#include "nsXPIDLString.h"
#include "nsDocShellCID.h"
#include "nsIWebNavigationInfo.h"
#include "nsIDOMWindow.h"
#include "nsAutoPtr.h"
#include "nsIHttpChannel.h"
#include "nsIScriptSecurityManager.h"
#include "nsNetError.h"





nsDSURIContentListener::nsDSURIContentListener(nsDocShell* aDocShell)
    : mDocShell(aDocShell), 
      mParentContentListener(nsnull)
{
}

nsDSURIContentListener::~nsDSURIContentListener()
{
}

nsresult
nsDSURIContentListener::Init() 
{
    nsresult rv;
    mNavInfo = do_GetService(NS_WEBNAVIGATION_INFO_CONTRACTID, &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to get webnav info");
    return rv;
}






NS_IMPL_THREADSAFE_ADDREF(nsDSURIContentListener)
NS_IMPL_THREADSAFE_RELEASE(nsDSURIContentListener)

NS_INTERFACE_MAP_BEGIN(nsDSURIContentListener)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIURIContentListener)
    NS_INTERFACE_MAP_ENTRY(nsIURIContentListener)
    NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END





NS_IMETHODIMP
nsDSURIContentListener::OnStartURIOpen(nsIURI* aURI, bool* aAbortOpen)
{
    
    
    
    if (!mDocShell) {
        *aAbortOpen = PR_TRUE;
        return NS_OK;
    }
    
    nsCOMPtr<nsIURIContentListener> parentListener;
    GetParentContentListener(getter_AddRefs(parentListener));
    if (parentListener)
        return parentListener->OnStartURIOpen(aURI, aAbortOpen);

    return NS_OK;
}

NS_IMETHODIMP 
nsDSURIContentListener::DoContent(const char* aContentType, 
                                  bool aIsContentPreferred,
                                  nsIRequest* request,
                                  nsIStreamListener** aContentHandler,
                                  bool* aAbortProcess)
{
    nsresult rv;
    NS_ENSURE_ARG_POINTER(aContentHandler);
    NS_ENSURE_TRUE(mDocShell, NS_ERROR_FAILURE);

    
    
    if (!CheckFrameOptions(request)) {
        *aAbortProcess = PR_TRUE;
        return NS_OK;
    }

    *aAbortProcess = PR_FALSE;

    
    nsLoadFlags loadFlags = 0;
    nsCOMPtr<nsIChannel> aOpenedChannel = do_QueryInterface(request);

    if (aOpenedChannel)
      aOpenedChannel->GetLoadFlags(&loadFlags);

    if(loadFlags & nsIChannel::LOAD_RETARGETED_DOCUMENT_URI)
    {
        
        mDocShell->Stop(nsIWebNavigation::STOP_NETWORK);

        mDocShell->SetLoadType(aIsContentPreferred ? LOAD_LINK : LOAD_NORMAL);
    }

    rv = mDocShell->CreateContentViewer(aContentType, request, aContentHandler);

    if (rv == NS_ERROR_REMOTE_XUL) {
      request->Cancel(rv);
      return NS_OK;
    }

    if (NS_FAILED(rv)) {
       
        return NS_OK;
    }

    if (loadFlags & nsIChannel::LOAD_RETARGETED_DOCUMENT_URI) {
        nsCOMPtr<nsIDOMWindow> domWindow = do_GetInterface(static_cast<nsIDocShell*>(mDocShell));
        NS_ENSURE_TRUE(domWindow, NS_ERROR_FAILURE);
        domWindow->Focus();
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDSURIContentListener::IsPreferred(const char* aContentType,
                                    char ** aDesiredContentType,
                                    bool* aCanHandle)
{
    NS_ENSURE_ARG_POINTER(aCanHandle);
    NS_ENSURE_ARG_POINTER(aDesiredContentType);

    
    

    nsCOMPtr<nsIURIContentListener> parentListener;
    GetParentContentListener(getter_AddRefs(parentListener));
    if (parentListener) {
        return parentListener->IsPreferred(aContentType,
                                                   aDesiredContentType,
                                                   aCanHandle);
    }
    
    
    
    
    
    
    
    
    
    
    
    return CanHandleContent(aContentType,
                            PR_TRUE,
                            aDesiredContentType,
                            aCanHandle);
}

NS_IMETHODIMP
nsDSURIContentListener::CanHandleContent(const char* aContentType,
                                         bool aIsContentPreferred,
                                         char ** aDesiredContentType,
                                         bool* aCanHandleContent)
{
    NS_PRECONDITION(aCanHandleContent, "Null out param?");
    NS_ENSURE_ARG_POINTER(aDesiredContentType);

    *aCanHandleContent = PR_FALSE;
    *aDesiredContentType = nsnull;

    nsresult rv = NS_OK;
    if (aContentType) {
        PRUint32 canHandle = nsIWebNavigationInfo::UNSUPPORTED;
        rv = mNavInfo->IsTypeSupported(nsDependentCString(aContentType),
                                       mDocShell,
                                       &canHandle);
        *aCanHandleContent = (canHandle != nsIWebNavigationInfo::UNSUPPORTED);
    }

    return rv;
}

NS_IMETHODIMP
nsDSURIContentListener::GetLoadCookie(nsISupports ** aLoadCookie)
{
    NS_IF_ADDREF(*aLoadCookie = nsDocShell::GetAsSupports(mDocShell));
    return NS_OK;
}

NS_IMETHODIMP
nsDSURIContentListener::SetLoadCookie(nsISupports * aLoadCookie)
{
#ifdef DEBUG
    nsRefPtr<nsDocLoader> cookieAsDocLoader =
        nsDocLoader::GetAsDocLoader(aLoadCookie);
    NS_ASSERTION(cookieAsDocLoader && cookieAsDocLoader == mDocShell,
                 "Invalid load cookie being set!");
#endif
    return NS_OK;
}

NS_IMETHODIMP 
nsDSURIContentListener::GetParentContentListener(nsIURIContentListener**
                                                 aParentListener)
{
    if (mWeakParentContentListener)
    {
        nsCOMPtr<nsIURIContentListener> tempListener =
            do_QueryReferent(mWeakParentContentListener);
        *aParentListener = tempListener;
        NS_IF_ADDREF(*aParentListener);
    }
    else {
        *aParentListener = mParentContentListener;
        NS_IF_ADDREF(*aParentListener);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsDSURIContentListener::SetParentContentListener(nsIURIContentListener* 
                                                 aParentListener)
{
    if (aParentListener)
    {
        
        
        mParentContentListener = nsnull;
        mWeakParentContentListener = do_GetWeakReference(aParentListener);
        if (!mWeakParentContentListener)
        {
            mParentContentListener = aParentListener;
        }
    }
    else
    {
        mWeakParentContentListener = nsnull;
        mParentContentListener = nsnull;
    }
    return NS_OK;
}


bool nsDSURIContentListener::CheckFrameOptions(nsIRequest* request)
{
    nsCAutoString xfoHeaderValue;

    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(request);
    if (!httpChannel) {
        return true;
    }

    httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("X-Frame-Options"),
                                   xfoHeaderValue);

    
    if (!xfoHeaderValue.LowerCaseEqualsLiteral("deny") &&
        !xfoHeaderValue.LowerCaseEqualsLiteral("sameorigin"))
        return true;

    if (mDocShell) {
        
        
        
        
        nsCOMPtr<nsIDOMWindow> thisWindow = do_GetInterface(static_cast<nsIDocShell*>(mDocShell));
        
        if (!thisWindow)
            return true;

        nsCOMPtr<nsIDOMWindow> topWindow;
        thisWindow->GetTop(getter_AddRefs(topWindow));

        
        if (thisWindow == topWindow)
            return true;

        
        
        
        
        nsCOMPtr<nsIDocShellTreeItem> thisDocShellItem(do_QueryInterface(
                                                       static_cast<nsIDocShell*> (mDocShell)));
        nsCOMPtr<nsIDocShellTreeItem> parentDocShellItem,
                                      curDocShellItem = thisDocShellItem;
        nsCOMPtr<nsIDocument> topDoc;
        nsresult rv;
        nsCOMPtr<nsIScriptSecurityManager> ssm =
            do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
        if (!ssm)
            return false;

        
        
        while (NS_SUCCEEDED(curDocShellItem->GetParent(getter_AddRefs(parentDocShellItem))) &&
               parentDocShellItem) {
            bool system = false;
            topDoc = do_GetInterface(parentDocShellItem);
            if (topDoc) {
                if (NS_SUCCEEDED(ssm->IsSystemPrincipal(topDoc->NodePrincipal(),
                                                        &system)) && system) {
                    break;
                }
            }
            else {
                return false;
            }
            curDocShellItem = parentDocShellItem;
        }

        
        
        if (curDocShellItem == thisDocShellItem)
            return true;

        
        
        if (xfoHeaderValue.LowerCaseEqualsLiteral("sameorigin")) {
            nsCOMPtr<nsIURI> uri;
            httpChannel->GetURI(getter_AddRefs(uri));
            topDoc = do_GetInterface(curDocShellItem);
            nsCOMPtr<nsIURI> topUri;
            topDoc->NodePrincipal()->GetURI(getter_AddRefs(topUri));
            rv = ssm->CheckSameOriginURI(uri, topUri, PR_TRUE);
            if (NS_SUCCEEDED(rv))
                return true;
        }

        else {
            
            
            NS_ASSERTION(xfoHeaderValue.LowerCaseEqualsLiteral("deny"),
                         "How did we get here with some random header value?");
        }

        
        httpChannel->Cancel(NS_BINDING_ABORTED);
        nsCOMPtr<nsIWebNavigation> webNav(do_QueryObject(mDocShell));
        if (webNav) {
            webNav->LoadURI(NS_LITERAL_STRING("about:blank").get(),
                            0, nsnull, nsnull, nsnull);
        }
        return false;
    }

    return true;
}
