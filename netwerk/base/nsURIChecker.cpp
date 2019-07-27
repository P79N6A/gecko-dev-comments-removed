




#include "nsURIChecker.h"
#include "nsIAuthPrompt.h"
#include "nsIHttpChannel.h"
#include "nsNetUtil.h"
#include "nsString.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsNullPrincipal.h"



static bool
ServerIsNES3x(nsIHttpChannel *httpChannel)
{
    nsAutoCString server;
    httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Server"), server);
    
    
    
    return StringBeginsWith(server,
                            NS_LITERAL_CSTRING("Netscape-Enterprise/3."));
}



NS_IMPL_ISUPPORTS(nsURIChecker,
                  nsIURIChecker,
                  nsIRequest,
                  nsIRequestObserver,
                  nsIStreamListener,
                  nsIChannelEventSink,
                  nsIInterfaceRequestor)

nsURIChecker::nsURIChecker()
    : mStatus(NS_OK)
    , mIsPending(false)
    , mAllowHead(true)
{
}

void
nsURIChecker::SetStatusAndCallBack(nsresult aStatus)
{
    mStatus = aStatus;
    mIsPending = false;

    if (mObserver) {
        mObserver->OnStartRequest(this, mObserverContext);
        mObserver->OnStopRequest(this, mObserverContext, mStatus);
        mObserver = nullptr;
        mObserverContext = nullptr;
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

    uint32_t responseStatus;
    rv = httpChannel->GetResponseStatus(&responseStatus);
    if (NS_FAILED(rv))
        return NS_BINDING_FAILED;

    
    if (responseStatus / 100 == 2)
        return NS_BINDING_SUCCEEDED;

    
    
    
    
    if (responseStatus == 404) {
        if (mAllowHead && ServerIsNES3x(httpChannel)) {
            mAllowHead = false;

            
            
            nsCOMPtr<nsIChannel> lastChannel = mChannel;

            nsCOMPtr<nsIURI> uri;
            uint32_t loadFlags;

            rv  = lastChannel->GetOriginalURI(getter_AddRefs(uri));
            nsresult tmp = lastChannel->GetLoadFlags(&loadFlags);
            if (NS_FAILED(tmp)) {
              rv = tmp;
            }

            
            

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
    nsCOMPtr<nsIPrincipal> nullPrincipal = nsNullPrincipal::Create();
    NS_ENSURE_TRUE(nullPrincipal, NS_ERROR_FAILURE);
    rv = NS_NewChannel(getter_AddRefs(mChannel),
                       aURI,
                       nullPrincipal,
                       nsILoadInfo::SEC_NORMAL,
                       nsIContentPolicy::TYPE_OTHER);
    NS_ENSURE_SUCCESS(rv, rv);

    if (mAllowHead) {
        mAllowHead = false;
        
        nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(mChannel);
        if (httpChannel) {
            
            
            
            bool isReallyHTTP = false;
            aURI->SchemeIs("http", &isReallyHTTP);
            if (!isReallyHTTP)
                aURI->SchemeIs("https", &isReallyHTTP);
            if (isReallyHTTP) {
                httpChannel->SetRequestMethod(NS_LITERAL_CSTRING("HEAD"));
                
                
                
                
                mAllowHead = true;
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
    
    
    nsresult rv = mChannel->AsyncOpen(this, nullptr);
    if (NS_FAILED(rv))
        mChannel = nullptr;
    else {
        
        mIsPending = true;
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
nsURIChecker::IsPending(bool *aPendingRet)
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
        
        
        mChannel = nullptr;
    }
    return NS_OK;
}





NS_IMETHODIMP
nsURIChecker::OnDataAvailable(nsIRequest *aRequest, nsISupports *aCtxt,
                               nsIInputStream *aInput, uint64_t aOffset,
                               uint32_t aCount)
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
nsURIChecker::AsyncOnChannelRedirect(nsIChannel *aOldChannel,
                                     nsIChannel *aNewChannel,
                                     uint32_t aFlags,
                                     nsIAsyncVerifyRedirectCallback *callback)
{
    
    mChannel = aNewChannel;
    callback->OnRedirectVerifyCallback(NS_OK);
    return NS_OK;
}
