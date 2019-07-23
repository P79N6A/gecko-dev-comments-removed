






































#include "nsURIChecker.h"
#include "nsIServiceManager.h"
#include "nsIAuthPrompt.h"
#include "nsIHttpChannel.h"
#include "nsNetUtil.h"
#include "nsString.h"



static PRBool
ServerIsNES3x(nsIHttpChannel *httpChannel)
{
    nsCAutoString server;
    httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Server"), server);
    
    
    
    return StringBeginsWith(server,
                            NS_LITERAL_CSTRING("Netscape-Enterprise/3."));
}



NS_IMPL_ISUPPORTS6(nsURIChecker,
                   nsIURIChecker,
                   nsIRequest,
                   nsIRequestObserver,
                   nsIStreamListener,
                   nsIChannelEventSink,
                   nsIInterfaceRequestor)

nsURIChecker::nsURIChecker()
    : mStatus(NS_OK)
    , mIsPending(PR_FALSE)
    , mAllowHead(PR_TRUE)
{
}

void
nsURIChecker::SetStatusAndCallBack(nsresult aStatus)
{
    mStatus = aStatus;
    mIsPending = PR_FALSE;

    if (mObserver) {
        mObserver->OnStartRequest(this, mObserverContext);
        mObserver->OnStopRequest(this, mObserverContext, mStatus);
        mObserver = nsnull;
        mObserverContext = nsnull;
    }
}

nsresult
nsURIChecker::CheckStatus()
{
    NS_ASSERTION(mChannel, "no channel");

    nsresult status;
    nsresult rv = mChannel->GetStatus(&status);
    
    if (NS_FAILED(rv) || NS_FAILED(status))
        return NS_BINDING_FAILED;

    
    
    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(mChannel);
    if (!httpChannel)
        return NS_BINDING_SUCCEEDED;

    PRUint32 responseStatus;
    rv = httpChannel->GetResponseStatus(&responseStatus);
    if (NS_FAILED(rv))
        return NS_BINDING_FAILED;

    
    if (responseStatus / 100 == 2)
        return NS_BINDING_SUCCEEDED;

    
    
    
    
    if (responseStatus == 404) {
        if (mAllowHead && ServerIsNES3x(httpChannel)) {
            mAllowHead = PR_FALSE;

            
            
            nsCOMPtr<nsIChannel> lastChannel = mChannel;

            nsCOMPtr<nsIURI> uri;
            PRUint32 loadFlags;

            rv  = lastChannel->GetOriginalURI(getter_AddRefs(uri));
            rv |= lastChannel->GetLoadFlags(&loadFlags);

            
            

            if (NS_SUCCEEDED(rv)) {
                rv = Init(uri);
                if (NS_SUCCEEDED(rv)) {
                    rv = mChannel->SetLoadFlags(loadFlags);
                    if (NS_SUCCEEDED(rv)) {
                        rv = AsyncCheck(mObserver, mObserverContext);
                        
                        
                        if (NS_SUCCEEDED(rv))
                            return NS_BASE_STREAM_WOULD_BLOCK;
                    }
                }
            }
            
            
            mChannel = lastChannel;
        }
    }

    
    return NS_BINDING_FAILED;
}





NS_IMETHODIMP
nsURIChecker::Init(nsIURI *aURI)
{
    nsresult rv;
    nsCOMPtr<nsIIOService> ios = do_GetIOService(&rv);
    if (NS_FAILED(rv)) return rv;

    rv = ios->NewChannelFromURI(aURI, getter_AddRefs(mChannel));
    if (NS_FAILED(rv)) return rv;

    if (mAllowHead) {
        mAllowHead = PR_FALSE;
        
        nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(mChannel);
        if (httpChannel) {
            
            
            
            PRBool isReallyHTTP = PR_FALSE;
            aURI->SchemeIs("http", &isReallyHTTP);
            if (!isReallyHTTP)
                aURI->SchemeIs("https", &isReallyHTTP);
            if (isReallyHTTP) {
                httpChannel->SetRequestMethod(NS_LITERAL_CSTRING("HEAD"));
                
                
                
                
                mAllowHead = PR_TRUE;
            }
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsURIChecker::AsyncCheck(nsIRequestObserver *aObserver,
                         nsISupports *aObserverContext)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_NOT_INITIALIZED);

    
    
    mChannel->SetNotificationCallbacks(this);
    
    
    nsresult rv = mChannel->AsyncOpen(this, nsnull);
    if (NS_FAILED(rv))
        mChannel = nsnull;
    else {
        
        mIsPending = PR_TRUE;
        mObserver = aObserver;
        mObserverContext = aObserverContext;
    }
    return rv;
}

NS_IMETHODIMP
nsURIChecker::GetBaseChannel(nsIChannel **aChannel)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_NOT_INITIALIZED);
    NS_ADDREF(*aChannel = mChannel);
    return NS_OK;
}





NS_IMETHODIMP
nsURIChecker::GetName(nsACString &aName)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_NOT_INITIALIZED);
    return mChannel->GetName(aName);
}

NS_IMETHODIMP
nsURIChecker::IsPending(PRBool *aPendingRet)
{
    *aPendingRet = mIsPending;
    return NS_OK;
}

NS_IMETHODIMP
nsURIChecker::GetStatus(nsresult* aStatusRet)
{
    *aStatusRet = mStatus;
    return NS_OK;
}

NS_IMETHODIMP
nsURIChecker::Cancel(nsresult status)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_NOT_INITIALIZED);
    return mChannel->Cancel(status);
}

NS_IMETHODIMP
nsURIChecker::Suspend()
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_NOT_INITIALIZED);
    return mChannel->Suspend();
}

NS_IMETHODIMP
nsURIChecker::Resume()
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_NOT_INITIALIZED);
    return mChannel->Resume();
}

NS_IMETHODIMP
nsURIChecker::GetLoadGroup(nsILoadGroup **aLoadGroup)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_NOT_INITIALIZED);
    return mChannel->GetLoadGroup(aLoadGroup);
}

NS_IMETHODIMP
nsURIChecker::SetLoadGroup(nsILoadGroup *aLoadGroup)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_NOT_INITIALIZED);
    return mChannel->SetLoadGroup(aLoadGroup);
}

NS_IMETHODIMP
nsURIChecker::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_NOT_INITIALIZED);
    return mChannel->GetLoadFlags(aLoadFlags);
}

NS_IMETHODIMP
nsURIChecker::SetLoadFlags(nsLoadFlags aLoadFlags)
{
    NS_ENSURE_TRUE(mChannel, NS_ERROR_NOT_INITIALIZED);
    return mChannel->SetLoadFlags(aLoadFlags);
}





NS_IMETHODIMP
nsURIChecker::OnStartRequest(nsIRequest *aRequest, nsISupports *aCtxt)
{
    NS_ASSERTION(aRequest == mChannel, "unexpected request");

    nsresult rv = CheckStatus();
    if (rv != NS_BASE_STREAM_WOULD_BLOCK)
        SetStatusAndCallBack(rv);

    
    return NS_BINDING_ABORTED;
}

NS_IMETHODIMP
nsURIChecker::OnStopRequest(nsIRequest *request, nsISupports *ctxt,
                             nsresult statusCode)
{
    
    
    if (mChannel == request) {
        
        
        mChannel = nsnull;
    }
    return NS_OK;
}





NS_IMETHODIMP
nsURIChecker::OnDataAvailable(nsIRequest *aRequest, nsISupports *aCtxt,
                               nsIInputStream *aInput, PRUint32 aOffset,
                               PRUint32 aCount)
{
    NS_NOTREACHED("nsURIChecker::OnDataAvailable");
    return NS_BINDING_ABORTED;
}





NS_IMETHODIMP
nsURIChecker::GetInterface(const nsIID & aIID, void **aResult)
{
    if (mObserver && aIID.Equals(NS_GET_IID(nsIAuthPrompt))) {
        nsCOMPtr<nsIInterfaceRequestor> req = do_QueryInterface(mObserver);
        if (req)
            return req->GetInterface(aIID, aResult);
    }
    return QueryInterface(aIID, aResult);
}





NS_IMETHODIMP
nsURIChecker::OnChannelRedirect(nsIChannel *aOldChannel,
                                nsIChannel *aNewChannel,
                                PRUint32    aFlags)
{
    
    mChannel = aNewChannel;
    return NS_OK;
}
