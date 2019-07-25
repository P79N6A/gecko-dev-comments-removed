




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
#include "nsError.h"
#include "nsCharSeparatedTokenizer.h"
#include "mozilla/Preferences.h"

using namespace mozilla;





nsDSURIContentListener::nsDSURIContentListener(nsDocShell* aDocShell)
    : mDocShell(aDocShell), 
      mParentContentListener(nullptr)
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
        *aAbortOpen = true;
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
        *aAbortProcess = true;
        return NS_OK;
    }

    *aAbortProcess = false;

    
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
                            true,
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

    *aCanHandleContent = false;
    *aDesiredContentType = nullptr;

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
        
        
        mParentContentListener = nullptr;
        mWeakParentContentListener = do_GetWeakReference(aParentListener);
        if (!mWeakParentContentListener)
        {
            mParentContentListener = aParentListener;
        }
    }
    else
    {
        mWeakParentContentListener = nullptr;
        mParentContentListener = nullptr;
    }
    return NS_OK;
}

bool nsDSURIContentListener::CheckOneFrameOptionsPolicy(nsIRequest *request,
                                                        const nsAString& policy) {
    
    if (!policy.LowerCaseEqualsLiteral("deny") &&
        !policy.LowerCaseEqualsLiteral("sameorigin"))
        return true;

    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(request);
    if (!httpChannel) {
        return true;
    }

    if (mDocShell) {
        
        
        
        
        nsCOMPtr<nsIDOMWindow> thisWindow = do_GetInterface(static_cast<nsIDocShell*>(mDocShell));
        
        if (!thisWindow)
            return true;

        
        
        nsCOMPtr<nsIDOMWindow> topWindow;
        thisWindow->GetScriptableTop(getter_AddRefs(topWindow));

        
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
        if (!ssm) {
            NS_ASSERTION(ssm, "Failed to get the ScriptSecurityManager.");
            return false;
        }

        
        
        
        while (NS_SUCCEEDED(curDocShellItem->GetParent(getter_AddRefs(parentDocShellItem))) &&
               parentDocShellItem) {

            nsCOMPtr<nsIDocShell> curDocShell = do_QueryInterface(curDocShellItem);
            bool isContentBoundary;
            curDocShell->GetIsContentBoundary(&isContentBoundary);
            if (isContentBoundary) {
              break;
            }

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

        
        
        
        if (policy.LowerCaseEqualsLiteral("deny")) {
            return false;
        }

        
        
        if (policy.LowerCaseEqualsLiteral("sameorigin")) {
            nsCOMPtr<nsIURI> uri;
            httpChannel->GetURI(getter_AddRefs(uri));
            topDoc = do_GetInterface(curDocShellItem);
            nsCOMPtr<nsIURI> topUri;
            topDoc->NodePrincipal()->GetURI(getter_AddRefs(topUri));
            rv = ssm->CheckSameOriginURI(uri, topUri, true);
            if (NS_FAILED(rv))
                return false; 
        }
    }

    return true;
}




bool nsDSURIContentListener::CheckFrameOptions(nsIRequest *request)
{
    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(request);
    if (!httpChannel) {
        return true;
    }

    nsCAutoString xfoHeaderCValue;
    httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("X-Frame-Options"),
                                   xfoHeaderCValue);
    NS_ConvertUTF8toUTF16 xfoHeaderValue(xfoHeaderCValue);

    
    if (xfoHeaderValue.IsEmpty())
        return true;

    
    
    nsCharSeparatedTokenizer tokenizer(xfoHeaderValue, ',');
    while (tokenizer.hasMoreTokens()) {
        const nsSubstring& tok = tokenizer.nextToken();
        if (!CheckOneFrameOptionsPolicy(request, tok)) {
            
            httpChannel->Cancel(NS_BINDING_ABORTED);
            if (mDocShell) {
                nsCOMPtr<nsIWebNavigation> webNav(do_QueryObject(mDocShell));
                if (webNav) {
                    webNav->LoadURI(NS_LITERAL_STRING("about:blank").get(),
                                    0, nullptr, nullptr, nullptr);
                }
            }
            return false;
        }
    }

    return true;
}
