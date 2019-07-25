














































#include "base/basictypes.h"

#include "nsHttpChannel.h"
#include "nsHttpHandler.h"
#include "nsIApplicationCacheService.h"
#include "nsIApplicationCacheContainer.h"
#include "nsIAuthInformation.h"
#include "nsIStringBundle.h"
#include "nsIIDNService.h"
#include "nsIStreamListenerTee.h"
#include "nsISeekableStream.h"
#include "nsMimeTypes.h"
#include "nsPrintfCString.h"
#include "nsNetUtil.h"
#include "prprf.h"
#include "prnetdb.h"
#include "nsEscape.h"
#include "nsStreamUtils.h"
#include "nsIOService.h"
#include "nsICacheService.h"
#include "nsDNSPrefetch.h"
#include "nsChannelClassifier.h"
#include "nsIRedirectResultListener.h"


#define BYPASS_LOCAL_CACHE(loadFlags) \
        (loadFlags & (nsIRequest::LOAD_BYPASS_CACHE | \
                      nsICachingChannel::LOAD_BYPASS_LOCAL_CACHE))

static NS_DEFINE_CID(kStreamListenerTeeCID, NS_STREAMLISTENERTEE_CID);

class AutoRedirectVetoNotifier
{
public:
    AutoRedirectVetoNotifier(nsHttpChannel* channel) : mChannel(channel) {}
    ~AutoRedirectVetoNotifier() {ReportRedirectResult(false);}
    void RedirectSucceeded() {ReportRedirectResult(true);}

private:
    nsHttpChannel* mChannel;
    void ReportRedirectResult(bool succeeded);
};

void
AutoRedirectVetoNotifier::ReportRedirectResult(bool succeeded)
{
    if (!mChannel)
        return;

    mChannel->mRedirectChannel = nsnull;

    nsCOMPtr<nsIRedirectResultListener> vetoHook;
    NS_QueryNotificationCallbacks(mChannel, 
                                  NS_GET_IID(nsIRedirectResultListener), 
                                  getter_AddRefs(vetoHook));
    mChannel = nsnull;
    if (vetoHook)
        vetoHook->OnRedirectResult(succeeded);
}





nsHttpChannel::nsHttpChannel()
    : mLogicalOffset(0)
    , mCacheAccess(0)
    , mPostID(0)
    , mRequestTime(0)
    , mOnCacheEntryAvailableCallback(nsnull)
    , mAsyncCacheOpen(PR_FALSE)
    , mPendingAsyncCallOnResume(nsnull)
    , mSuspendCount(0)
    , mCachedContentIsValid(PR_FALSE)
    , mCachedContentIsPartial(PR_FALSE)
    , mTransactionReplaced(PR_FALSE)
    , mAuthRetryPending(PR_FALSE)
    , mResuming(PR_FALSE)
    , mInitedCacheEntry(PR_FALSE)
    , mCacheForOfflineUse(PR_FALSE)
    , mCachingOpportunistically(PR_FALSE)
    , mFallbackChannel(PR_FALSE)
    , mTracingEnabled(PR_TRUE)
    , mCustomConditionalRequest(PR_FALSE)
    , mFallingBack(PR_FALSE)
    , mWaitingForRedirectCallback(PR_FALSE)
    , mRequestTimeInitialized(PR_FALSE)
{
    LOG(("Creating nsHttpChannel [this=%p]\n", this));
}

nsHttpChannel::~nsHttpChannel()
{
    LOG(("Destroying nsHttpChannel [this=%p]\n", this));

    if (mAuthProvider) 
        mAuthProvider->Disconnect(NS_ERROR_ABORT);
}

nsresult
nsHttpChannel::Init(nsIURI *uri,
                    PRUint8 caps,
                    nsProxyInfo *proxyInfo)
{
    nsresult rv = HttpBaseChannel::Init(uri, caps, proxyInfo);
    if (NS_FAILED(rv)) 
        return rv;

    LOG(("nsHttpChannel::Init [this=%p]\n", this));

    mAuthProvider =
        do_CreateInstance("@mozilla.org/network/http-channel-auth-provider;1",
                          &rv);
    if (NS_FAILED(rv)) 
        return rv;
    rv = mAuthProvider->Init(this);

    return rv;
}




nsresult
nsHttpChannel::AsyncCall(nsAsyncCallback funcPtr,
                         nsRunnableMethod<nsHttpChannel> **retval)
{
    nsresult rv;

    nsRefPtr<nsRunnableMethod<nsHttpChannel> > event =
        NS_NewRunnableMethod(this, funcPtr);
    rv = NS_DispatchToCurrentThread(event);
    if (NS_SUCCEEDED(rv) && retval) {
        *retval = event;
    }

    return rv;
}

nsresult
nsHttpChannel::Connect(PRBool firstTime)
{
    nsresult rv;

    LOG(("nsHttpChannel::Connect [this=%p]\n", this));

    
    
    
    
    PRBool usingSSL = PR_FALSE;
    rv = mURI->SchemeIs("https", &usingSSL);
    NS_ENSURE_SUCCESS(rv,rv);

    if (!usingSSL) {
        
        nsIStrictTransportSecurityService* stss = gHttpHandler->GetSTSService();
        NS_ENSURE_TRUE(stss, NS_ERROR_OUT_OF_MEMORY);

        PRBool isStsHost = PR_FALSE;
        rv = stss->IsStsURI(mURI, &isStsHost);

        
        
        NS_ASSERTION(NS_SUCCEEDED(rv),
                     "Something is wrong with STS: IsStsURI failed.");

        if (NS_SUCCEEDED(rv) && isStsHost) {
            LOG(("nsHttpChannel::Connect() STS permissions found\n"));
            return AsyncCall(&nsHttpChannel::HandleAsyncRedirectChannelToHttps);
        }
    }

    
    if (!net_IsValidHostName(nsDependentCString(mConnectionInfo->Host())))
        return NS_ERROR_UNKNOWN_HOST;

    
    if (firstTime) {
        
        PRBool offline = gIOService->IsOffline();
        if (offline)
            mLoadFlags |= LOAD_ONLY_FROM_CACHE;
        else if (PL_strcmp(mConnectionInfo->ProxyType(), "unknown") == 0)
            return ResolveProxy();  

        
        if (mResuming && (mLoadFlags & LOAD_ONLY_FROM_CACHE)) {
            LOG(("Resuming from cache is not supported yet"));
            return NS_ERROR_DOCUMENT_NOT_CACHED;
        }

        
        rv = OpenCacheEntry();

        if (NS_FAILED(rv)) {
            LOG(("OpenCacheEntry failed [rv=%x]\n", rv));
            
            
            if (mLoadFlags & LOAD_ONLY_FROM_CACHE) {
                
                
                if (!mFallbackChannel && !mFallbackKey.IsEmpty()) {
                    return AsyncCall(&nsHttpChannel::HandleAsyncFallback);
                }
                return NS_ERROR_DOCUMENT_NOT_CACHED;
            }
            
        }

        
        
        if (mCacheForOfflineUse) {
            rv = OpenOfflineCacheEntryForWriting();
            if (NS_FAILED(rv)) return rv;
        }

        if (NS_SUCCEEDED(rv) && mAsyncCacheOpen)
            return NS_OK;
    }

    
    if (mCacheEntry) {
        
        
        
        rv = CheckCache();
        if (NS_FAILED(rv))
            NS_WARNING("cache check failed");

        
        if (mCachedContentIsValid) {
            nsRunnableMethod<nsHttpChannel> *event = nsnull;
            if (!mCachedContentIsPartial) {
                AsyncCall(&nsHttpChannel::AsyncOnExamineCachedResponse, &event);
            }
            rv = ReadFromCache();
            if (NS_FAILED(rv) && event) {
                event->Revoke();
            }
            return rv;
        }
        else if (mLoadFlags & LOAD_ONLY_FROM_CACHE) {
            
            
            
            
            return NS_ERROR_DOCUMENT_NOT_CACHED;
        }
    }
    else if (mLoadFlags & LOAD_ONLY_FROM_CACHE) {
        
        
        if (!mFallbackChannel && !mFallbackKey.IsEmpty()) {
            return AsyncCall(&nsHttpChannel::HandleAsyncFallback);
        }
        return NS_ERROR_DOCUMENT_NOT_CACHED;
    }

    
    mAuthProvider->AddAuthorizationHeaders();

    if (mLoadFlags & LOAD_NO_NETWORK_IO) {
        return NS_ERROR_DOCUMENT_NOT_CACHED;
    }

    
    rv = SetupTransaction();
    if (NS_FAILED(rv)) return rv;

    rv = gHttpHandler->InitiateTransaction(mTransaction, mPriority);
    if (NS_FAILED(rv)) return rv;

    rv = mTransactionPump->AsyncRead(this, nsnull);
    if (NS_FAILED(rv)) return rv;

    PRUint32 suspendCount = mSuspendCount;
    while (suspendCount--)
        mTransactionPump->Suspend();

    return NS_OK;
}


nsresult
nsHttpChannel::AsyncAbort(nsresult status)
{
    LOG(("nsHttpChannel::AsyncAbort [this=%p status=%x]\n", this, status));

    mStatus = status;
    mIsPending = PR_FALSE;

    nsresult rv = AsyncCall(&nsHttpChannel::HandleAsyncNotifyListener);
    
    
    
    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nsnull, status);

    return rv;
}

void
nsHttpChannel::HandleAsyncNotifyListener()
{
    NS_PRECONDITION(!mPendingAsyncCallOnResume, "How did that happen?");
    
    if (mSuspendCount) {
        LOG(("Waiting until resume to do async notification [this=%p]\n",
             this));
        mPendingAsyncCallOnResume = &nsHttpChannel::HandleAsyncNotifyListener;
        return;
    }

    DoNotifyListener();
}

void
nsHttpChannel::DoNotifyListener()
{
    
    
    
    if (mListener) {
        mListener->OnStartRequest(this, mListenerContext);
        mIsPending = PR_FALSE;
        mListener->OnStopRequest(this, mListenerContext, mStatus);
        mListener = 0;
        mListenerContext = 0;
    }
    else {
        mIsPending = PR_FALSE;
    }
    
    mCallbacks = nsnull;
    mProgressSink = nsnull;

    
    CleanRedirectCacheChainIfNecessary();
}

void
nsHttpChannel::HandleAsyncRedirect()
{
    NS_PRECONDITION(!mPendingAsyncCallOnResume, "How did that happen?");
    
    if (mSuspendCount) {
        LOG(("Waiting until resume to do async redirect [this=%p]\n", this));
        mPendingAsyncCallOnResume = &nsHttpChannel::HandleAsyncRedirect;
        return;
    }

    nsresult rv = NS_OK;

    LOG(("nsHttpChannel::HandleAsyncRedirect [this=%p]\n", this));

    
    
    
    if (NS_SUCCEEDED(mStatus)) {
        PushRedirectAsyncFunc(&nsHttpChannel::ContinueHandleAsyncRedirect);
        rv = AsyncProcessRedirection(mResponseHead->Status());
        if (NS_FAILED(rv)) {
            PopRedirectAsyncFunc(&nsHttpChannel::ContinueHandleAsyncRedirect);
            ContinueHandleAsyncRedirect(rv);
        }
    }
    else {
        ContinueHandleAsyncRedirect(NS_OK);
    }
}

nsresult
nsHttpChannel::ContinueHandleAsyncRedirect(nsresult rv)
{
    if (NS_FAILED(rv)) {
        
        
        LOG(("ContinueHandleAsyncRedirect got failure result [rv=%x]\n", rv));
        mStatus = rv;
        DoNotifyListener();
    }

    
    
    if (mCacheEntry) {
        if (NS_FAILED(rv))
            mCacheEntry->Doom();
        CloseCacheEntry(PR_FALSE);
    }

    mIsPending = PR_FALSE;

    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nsnull, mStatus);

    return NS_OK;
}

void
nsHttpChannel::HandleAsyncNotModified()
{
    NS_PRECONDITION(!mPendingAsyncCallOnResume, "How did that happen?");
    
    if (mSuspendCount) {
        LOG(("Waiting until resume to do async not-modified [this=%p]\n",
             this));
        mPendingAsyncCallOnResume = &nsHttpChannel::HandleAsyncNotModified;
        return;
    }
    
    LOG(("nsHttpChannel::HandleAsyncNotModified [this=%p]\n", this));

    DoNotifyListener();

    CloseCacheEntry(PR_TRUE);

    mIsPending = PR_FALSE;

    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nsnull, mStatus);
}

void
nsHttpChannel::HandleAsyncFallback()
{
    NS_PRECONDITION(!mPendingAsyncCallOnResume, "How did that happen?");

    if (mSuspendCount) {
        LOG(("Waiting until resume to do async fallback [this=%p]\n", this));
        mPendingAsyncCallOnResume = &nsHttpChannel::HandleAsyncFallback;
        return;
    }

    nsresult rv = NS_OK;

    LOG(("nsHttpChannel::HandleAsyncFallback [this=%p]\n", this));

    
    
    
    if (!mCanceled) {
        PushRedirectAsyncFunc(&nsHttpChannel::ContinueHandleAsyncFallback);
        PRBool waitingForRedirectCallback;
        rv = ProcessFallback(&waitingForRedirectCallback);
        if (waitingForRedirectCallback)
            return;
        PopRedirectAsyncFunc(&nsHttpChannel::ContinueHandleAsyncFallback);
    }

    ContinueHandleAsyncFallback(rv);
}

nsresult
nsHttpChannel::ContinueHandleAsyncFallback(nsresult rv)
{
    if (!mCanceled && (NS_FAILED(rv) || !mFallingBack)) {
        
        
        LOG(("ProcessFallback failed [rv=%x, %d]\n", rv, mFallingBack));
        mStatus = NS_FAILED(rv) ? rv : NS_ERROR_DOCUMENT_NOT_CACHED;
        DoNotifyListener();
    }

    mIsPending = PR_FALSE;

    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nsnull, mStatus);

    return rv;
}

nsresult
nsHttpChannel::SetupTransaction()
{
    LOG(("nsHttpChannel::SetupTransaction [this=%p]\n", this));

    NS_ENSURE_TRUE(!mTransaction, NS_ERROR_ALREADY_INITIALIZED);

    nsresult rv;

    if (mCaps & NS_HTTP_ALLOW_PIPELINING) {
        
        
        
        
        
        
        
        
        
        if (!mAllowPipelining || (mLoadFlags & LOAD_INITIAL_DOCUMENT_URI) ||
            !(mRequestHead.Method() == nsHttp::Get ||
              mRequestHead.Method() == nsHttp::Head ||
              mRequestHead.Method() == nsHttp::Propfind ||
              mRequestHead.Method() == nsHttp::Proppatch)) {
            LOG(("  pipelining disallowed\n"));
            mCaps &= ~NS_HTTP_ALLOW_PIPELINING;
        }
    }

    
    
    nsCAutoString buf, path;
    nsCString* requestURI;
    if (mConnectionInfo->UsingSSL() || !mConnectionInfo->UsingHttpProxy()) {
        rv = mURI->GetPath(path);
        if (NS_FAILED(rv)) return rv;
        
        if (NS_EscapeURL(path.get(), path.Length(), esc_OnlyNonASCII, buf))
            requestURI = &buf;
        else
            requestURI = &path;
        mRequestHead.SetVersion(gHttpHandler->HttpVersion());
    }
    else {
        rv = mURI->GetUserPass(buf);
        if (NS_FAILED(rv)) return rv;
        if (!buf.IsEmpty() && ((strncmp(mSpec.get(), "http:", 5) == 0) ||
                                strncmp(mSpec.get(), "https:", 6) == 0)) {
            nsCOMPtr<nsIURI> tempURI;
            rv = mURI->Clone(getter_AddRefs(tempURI));
            if (NS_FAILED(rv)) return rv;
            rv = tempURI->SetUserPass(EmptyCString());
            if (NS_FAILED(rv)) return rv;
            rv = tempURI->GetAsciiSpec(path);
            if (NS_FAILED(rv)) return rv;
            requestURI = &path;
        }
        else
            requestURI = &mSpec;
        mRequestHead.SetVersion(gHttpHandler->ProxyHttpVersion());
    }

    
    PRInt32 ref = requestURI->FindChar('#');
    if (ref != kNotFound)
        requestURI->SetLength(ref);

    mRequestHead.SetRequestURI(*requestURI);

    
    mRequestTime = NowInSeconds();
    mRequestTimeInitialized = PR_TRUE;

    
    if (mLoadFlags & LOAD_BYPASS_CACHE) {
        
        
        
        mRequestHead.SetHeader(nsHttp::Pragma, NS_LITERAL_CSTRING("no-cache"), PR_TRUE);
        
        
        if (mRequestHead.Version() >= NS_HTTP_VERSION_1_1)
            mRequestHead.SetHeader(nsHttp::Cache_Control, NS_LITERAL_CSTRING("no-cache"), PR_TRUE);
    }
    else if ((mLoadFlags & VALIDATE_ALWAYS) && (mCacheAccess & nsICache::ACCESS_READ)) {
        
        
        
        
        
        if (mRequestHead.Version() >= NS_HTTP_VERSION_1_1)
            mRequestHead.SetHeader(nsHttp::Cache_Control, NS_LITERAL_CSTRING("max-age=0"), PR_TRUE);
        else
            mRequestHead.SetHeader(nsHttp::Pragma, NS_LITERAL_CSTRING("no-cache"), PR_TRUE);
    }

    if (mResuming) {
        char byteRange[32];
        PR_snprintf(byteRange, sizeof(byteRange), "bytes=%llu-", mStartPos);
        mRequestHead.SetHeader(nsHttp::Range, nsDependentCString(byteRange));

        if (!mEntityID.IsEmpty()) {
            
            
            nsCString::const_iterator start, end, slash;
            mEntityID.BeginReading(start);
            mEntityID.EndReading(end);
            mEntityID.BeginReading(slash);

            if (FindCharInReadable('/', slash, end)) {
                nsCAutoString ifMatch;
                mRequestHead.SetHeader(nsHttp::If_Match,
                        NS_UnescapeURL(Substring(start, slash), 0, ifMatch));

                ++slash; 
                         
            }

            if (FindCharInReadable('/', slash, end)) {
                mRequestHead.SetHeader(nsHttp::If_Unmodified_Since,
                        Substring(++slash, end));
            }
        }
    }

    
    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    NS_NewNotificationCallbacksAggregation(mCallbacks, mLoadGroup,
                                           getter_AddRefs(callbacks));
    if (!callbacks)
        return NS_ERROR_OUT_OF_MEMORY;

    
    mTransaction = new nsHttpTransaction();
    if (!mTransaction)
        return NS_ERROR_OUT_OF_MEMORY;

    
    if (mLoadFlags & LOAD_ANONYMOUS)
        mCaps |= NS_HTTP_LOAD_ANONYMOUS;

    mConnectionInfo->SetAnonymous((mLoadFlags & LOAD_ANONYMOUS) != 0);

    nsCOMPtr<nsIAsyncInputStream> responseStream;
    rv = mTransaction->Init(mCaps, mConnectionInfo, &mRequestHead,
                            mUploadStream, mUploadStreamHasHeaders,
                            NS_GetCurrentThread(), callbacks, this,
                            getter_AddRefs(responseStream));
    if (NS_FAILED(rv)) {
        mTransaction = nsnull;
        return rv;
    }

    rv = nsInputStreamPump::Create(getter_AddRefs(mTransactionPump),
                                   responseStream);
    return rv;
}



static void
CallTypeSniffers(void *aClosure, const PRUint8 *aData, PRUint32 aCount)
{
  nsIChannel *chan = static_cast<nsIChannel*>(aClosure);

  const nsCOMArray<nsIContentSniffer>& sniffers =
    gIOService->GetContentSniffers();
  PRUint32 length = sniffers.Count();
  for (PRUint32 i = 0; i < length; ++i) {
    nsCAutoString newType;
    nsresult rv =
      sniffers[i]->GetMIMETypeFromContent(chan, aData, aCount, newType);
    if (NS_SUCCEEDED(rv) && !newType.IsEmpty()) {
      chan->SetContentType(newType);
      break;
    }
  }
}

nsresult
nsHttpChannel::CallOnStartRequest()
{
    mTracingEnabled = PR_FALSE;

    if (mResponseHead && mResponseHead->ContentType().IsEmpty()) {
        if (!mContentTypeHint.IsEmpty())
            mResponseHead->SetContentType(mContentTypeHint);
        else {
            

            
            

            nsCOMPtr<nsIStreamConverterService> serv;
            nsresult rv = gHttpHandler->
                GetStreamConverterService(getter_AddRefs(serv));
            
            if (NS_SUCCEEDED(rv)) {
                nsCOMPtr<nsIStreamListener> converter;
                rv = serv->AsyncConvertData(UNKNOWN_CONTENT_TYPE,
                                            "*/*",
                                            mListener,
                                            mListenerContext,
                                            getter_AddRefs(converter));
                if (NS_SUCCEEDED(rv)) {
                    mListener = converter;
                }
            }
        }
    }

    if (mResponseHead && mResponseHead->ContentCharset().IsEmpty())
        mResponseHead->SetContentCharset(mContentCharsetHint);

    if (mResponseHead) {
        SetPropertyAsInt64(NS_CHANNEL_PROP_CONTENT_LENGTH,
                           mResponseHead->ContentLength());
        
        
        if (mCacheEntry) {
            nsresult rv;
            PRInt64 predictedDataSize = -1; 
            GetPropertyAsInt64(NS_CHANNEL_PROP_CONTENT_LENGTH, 
                               &predictedDataSize);
            rv = mCacheEntry->SetPredictedDataSize(predictedDataSize);
            if (NS_FAILED(rv)) return rv;
        }
    }
    
    if ((mLoadFlags & LOAD_CALL_CONTENT_SNIFFERS) &&
        gIOService->GetContentSniffers().Count() != 0) {
        
        
        
        

        nsIChannel* thisChannel = static_cast<nsIChannel*>(this);

        PRBool typeSniffersCalled = PR_FALSE;
        if (mCachePump) {
          typeSniffersCalled =
            NS_SUCCEEDED(mCachePump->PeekStream(CallTypeSniffers, thisChannel));
        }
        
        if (!typeSniffersCalled && mTransactionPump) {
          mTransactionPump->PeekStream(CallTypeSniffers, thisChannel);
        }
    }

    LOG(("  calling mListener->OnStartRequest\n"));
    nsresult rv = mListener->OnStartRequest(this, mListenerContext);
    if (NS_FAILED(rv)) return rv;

    
    rv = ApplyContentConversions();
    if (NS_FAILED(rv)) return rv;

    
    if (mCacheEntry && mChannelIsForDownload) {
        mCacheEntry->Doom();
        CloseCacheEntry(PR_FALSE);
    }

    if (!mCanceled) {
        
        if (mCacheForOfflineUse) {
            PRBool shouldCacheForOfflineUse;
            rv = ShouldUpdateOfflineCacheEntry(&shouldCacheForOfflineUse);
            if (NS_FAILED(rv)) return rv;
            
            if (shouldCacheForOfflineUse) {
                LOG(("writing to the offline cache"));
                rv = InitOfflineCacheEntry();
                if (NS_FAILED(rv)) return rv;
                
                if (mOfflineCacheEntry) {
                  rv = InstallOfflineCacheListener();
                  if (NS_FAILED(rv)) return rv;
                }
            } else {
                LOG(("offline cache is up to date, not updating"));
                CloseOfflineCacheEntry();
            }
        }
    }

    return NS_OK;
}

nsresult
nsHttpChannel::ProcessFailedSSLConnect(PRUint32 httpStatus)
{
    
    
    
    
    
    
    
    
    
    

    NS_ABORT_IF_FALSE(mConnectionInfo->UsingSSL(),
                      "SSL connect failed but not using SSL?");
    nsresult rv;
    switch (httpStatus) 
    {
    case 300: case 301: case 302: case 303: case 307:
        
        
        
        
        rv = NS_ERROR_CONNECTION_REFUSED;
        break;
    case 403: 
    case 407: 
    case 501: 
        
        rv = NS_ERROR_PROXY_CONNECTION_REFUSED; 
        break;
    
    case 404: 
    
    
    
    case 400: 
    case 500: 
        


        rv = NS_ERROR_UNKNOWN_HOST; 
        break;
    case 502: 
    
    case 503: 
        




        rv = NS_ERROR_CONNECTION_REFUSED;
        break;
    
    
    case 504: 
        
        
        rv = NS_ERROR_NET_TIMEOUT;
        break;
    
    default:
        rv = NS_ERROR_PROXY_CONNECTION_REFUSED; 
        break;
    }
    LOG(("Cancelling failed SSL proxy connection [this=%p httpStatus=%u]\n",
         this, httpStatus)); 
    Cancel(rv);
    CallOnStartRequest();
    return rv;
}

PRBool
nsHttpChannel::ShouldSSLProxyResponseContinue(PRUint32 httpStatus)
{
    
    
    switch (httpStatus) {
    case 407:
        return PR_TRUE;
    case 300: case 301: case 302: case 303: case 307:
      {
        return ( (mLoadFlags & nsIChannel::LOAD_DOCUMENT_URI) &&
                 mURI == mDocumentURI &&
                 mRequestHead.Method() != nsHttp::Post);
      }
    }
    return PR_FALSE;
}








nsresult
nsHttpChannel::ProcessSTSHeader()
{
    nsresult rv;
    PRBool isHttps = PR_FALSE;
    rv = mURI->SchemeIs("https", &isHttps);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    if (!isHttps)
        return NS_OK;

    nsCAutoString asciiHost;
    rv = mURI->GetAsciiHost(asciiHost);
    NS_ENSURE_SUCCESS(rv, NS_OK);

    
    
    PRNetAddr hostAddr;
    if (PR_SUCCESS == PR_StringToNetAddr(asciiHost.get(), &hostAddr))
        return NS_OK;

    nsIStrictTransportSecurityService* stss = gHttpHandler->GetSTSService();
    NS_ENSURE_TRUE(stss, NS_ERROR_OUT_OF_MEMORY);

    
    
    
    NS_ENSURE_TRUE(mSecurityInfo, NS_OK);

    
    
    
    PRBool tlsIsBroken = PR_FALSE;
    rv = stss->ShouldIgnoreStsHeader(mSecurityInfo, &tlsIsBroken);
    NS_ENSURE_SUCCESS(rv, NS_OK);

    
    
    
    
    
    PRBool wasAlreadySTSHost;
    rv = stss->IsStsURI(mURI, &wasAlreadySTSHost);
    
    
    NS_ENSURE_SUCCESS(rv, NS_OK);
    NS_ASSERTION(!(wasAlreadySTSHost && tlsIsBroken),
                 "connection should have been aborted by nss-bad-cert-handler");

    
    
    
    if (tlsIsBroken) {
        LOG(("STS: Transport layer is not trustworthy, ignoring "
             "STS headers and continuing load\n"));
        return NS_OK;
    }

    
    
    const nsHttpAtom atom = nsHttp::ResolveAtom("Strict-Transport-Security");
    nsCAutoString stsHeader;
    rv = mResponseHead->GetHeader(atom, stsHeader);
    if (rv == NS_ERROR_NOT_AVAILABLE) {
        LOG(("STS: No STS header, continuing load.\n"));
        return NS_OK;
    }
    
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stss->ProcessStsHeader(mURI, stsHeader.get());
    if (NS_FAILED(rv)) {
        LOG(("STS: Failed to parse STS header, continuing load.\n"));
        return NS_OK;
    }

    return NS_OK;
}

nsresult
nsHttpChannel::ProcessResponse()
{
    nsresult rv;
    PRUint32 httpStatus = mResponseHead->Status();

    LOG(("nsHttpChannel::ProcessResponse [this=%p httpStatus=%u]\n",
        this, httpStatus));

    if (mTransaction->SSLConnectFailed()) {
        if (!ShouldSSLProxyResponseContinue(httpStatus))
            return ProcessFailedSSLConnect(httpStatus);
        
        
    } else {
        
        rv = ProcessSTSHeader();
        NS_ASSERTION(NS_SUCCEEDED(rv), "ProcessSTSHeader failed, continuing load.");
    }

    
    gHttpHandler->OnExamineResponse(this);

    SetCookie(mResponseHead->PeekHeader(nsHttp::Set_Cookie));

    
    if (httpStatus != 401 && httpStatus != 407) {
        if (!mAuthRetryPending)
            mAuthProvider->CheckForSuperfluousAuth();
        if (mCanceled)
            return CallOnStartRequest();

        
        
        mAuthProvider->Disconnect(NS_ERROR_ABORT);
        mAuthProvider = nsnull;
        LOG(("  continuation state has been reset"));
    }

    
    
    
    
    switch (httpStatus) {
    case 200:
    case 203:
        
        
        
        
        if (mResuming && mStartPos != 0) {
            LOG(("Server ignored our Range header, cancelling [this=%p]\n", this));
            Cancel(NS_ERROR_NOT_RESUMABLE);
            rv = CallOnStartRequest();
            break;
        }
        
        rv = ProcessNormal();
        MaybeInvalidateCacheEntryForSubsequentGet();
        break;
    case 206:
        if (mCachedContentIsPartial) 
            rv = ProcessPartialContent();
        else
            rv = ProcessNormal();
        break;
    case 300:
    case 301:
    case 302:
    case 307:
    case 303:
#if 0
    case 305: 
#endif
        
        MaybeInvalidateCacheEntryForSubsequentGet();
        PushRedirectAsyncFunc(&nsHttpChannel::ContinueProcessResponse);
        rv = AsyncProcessRedirection(httpStatus);
        if (NS_FAILED(rv)) {
            PopRedirectAsyncFunc(&nsHttpChannel::ContinueProcessResponse);
            LOG(("AsyncProcessRedirection failed [rv=%x]\n", rv));
            rv = ContinueProcessResponse(rv);
        }
        break;
    case 304:
        rv = ProcessNotModified();
        if (NS_FAILED(rv)) {
            LOG(("ProcessNotModified failed [rv=%x]\n", rv));
            rv = ProcessNormal();
        }
        break;
    case 401:
    case 407:
        rv = mAuthProvider->ProcessAuthentication(
            httpStatus, mConnectionInfo->UsingSSL() &&
                        mTransaction->SSLConnectFailed());
        if (rv == NS_ERROR_IN_PROGRESS)  {
            
            
            mAuthRetryPending = PR_TRUE;
            
            
            
            
            LOG(("Suspending the transaction, asynchronously prompting for credentials"));
            mTransactionPump->Suspend();
            rv = NS_OK;
        }
        else if (NS_FAILED(rv)) {
            LOG(("ProcessAuthentication failed [rv=%x]\n", rv));
            if (mTransaction->SSLConnectFailed())
                return ProcessFailedSSLConnect(httpStatus);
            if (!mAuthRetryPending)
                mAuthProvider->CheckForSuperfluousAuth();
            rv = ProcessNormal();
        }
        else
            mAuthRetryPending = PR_TRUE; 
        break;
    default:
        rv = ProcessNormal();
        MaybeInvalidateCacheEntryForSubsequentGet();
        break;
    }

    return rv;
}

nsresult
nsHttpChannel::ContinueProcessResponse(nsresult rv)
{
    if (NS_SUCCEEDED(rv)) {
        InitCacheEntry();
        CloseCacheEntry(PR_FALSE);

        if (mCacheForOfflineUse) {
            
            InitOfflineCacheEntry();
            CloseOfflineCacheEntry();
        }
        return NS_OK;
    }

    LOG(("ContinueProcessResponse got failure result [rv=%x]\n", rv));
    if (mTransaction->SSLConnectFailed()) {
        return ProcessFailedSSLConnect(mRedirectType);
    }
    return ProcessNormal();
}

nsresult
nsHttpChannel::ProcessNormal()
{
    nsresult rv;

    LOG(("nsHttpChannel::ProcessNormal [this=%p]\n", this));

    PRBool succeeded;
    rv = GetRequestSucceeded(&succeeded);
    if (NS_SUCCEEDED(rv) && !succeeded) {
        PushRedirectAsyncFunc(&nsHttpChannel::ContinueProcessNormal);
        PRBool waitingForRedirectCallback;
        (void)ProcessFallback(&waitingForRedirectCallback);
        if (waitingForRedirectCallback) {
            
            return NS_OK;
        }
        PopRedirectAsyncFunc(&nsHttpChannel::ContinueProcessNormal);
    }

    return ContinueProcessNormal(NS_OK);
}

nsresult
nsHttpChannel::ContinueProcessNormal(nsresult rv)
{
    if (NS_FAILED(rv)) {
        
        
        mStatus = rv;
        DoNotifyListener();
        return rv;
    }

    if (mFallingBack) {
        
        
        return NS_OK;
    }

    
    
    
    mCachedContentIsPartial = PR_FALSE;

    ClearBogusContentEncodingIfNeeded();

    
    
    
    if (mCacheEntry) {
        rv = InitCacheEntry();
        if (NS_FAILED(rv))
            CloseCacheEntry(PR_TRUE);
    }

    
    if (mResuming) {
        
        nsCAutoString id;
        rv = GetEntityID(id);
        if (NS_FAILED(rv)) {
            
            Cancel(NS_ERROR_NOT_RESUMABLE);
        }
        else if (mResponseHead->Status() != 206 &&
                 mResponseHead->Status() != 200) {
            
            
            LOG(("Unexpected response status while resuming, aborting [this=%p]\n",
                 this));
            Cancel(NS_ERROR_ENTITY_CHANGED);
        }
        
        else if (!mEntityID.IsEmpty()) {
            if (!mEntityID.Equals(id)) {
                LOG(("Entity mismatch, expected '%s', got '%s', aborting [this=%p]",
                     mEntityID.get(), id.get(), this));
                Cancel(NS_ERROR_ENTITY_CHANGED);
            }
        }
    }

    rv = CallOnStartRequest();
    if (NS_FAILED(rv)) return rv;

    
    if (mCacheEntry && (mCacheAccess & nsICache::ACCESS_WRITE)) {
        rv = InstallCacheListener();
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}

nsresult
nsHttpChannel::PromptTempRedirect()
{
    if (!gHttpHandler->PromptTempRedirect()) {
        return NS_OK;
    }
    nsresult rv;
    nsCOMPtr<nsIStringBundleService> bundleService =
            do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIStringBundle> stringBundle;
    rv = bundleService->CreateBundle(NECKO_MSGS_URL, getter_AddRefs(stringBundle));
    if (NS_FAILED(rv)) return rv;

    nsXPIDLString messageString;
    rv = stringBundle->GetStringFromName(NS_LITERAL_STRING("RepostFormData").get(), getter_Copies(messageString));
    
    if (NS_SUCCEEDED(rv) && messageString) {
        PRBool repost = PR_FALSE;

        nsCOMPtr<nsIPrompt> prompt;
        GetCallback(prompt);
        if (!prompt)
            return NS_ERROR_NO_INTERFACE;

        prompt->Confirm(nsnull, messageString, &repost);
        if (!repost)
            return NS_ERROR_FAILURE;
    }

    return rv;
}

nsresult
nsHttpChannel::ProxyFailover()
{
    LOG(("nsHttpChannel::ProxyFailover [this=%p]\n", this));

    nsresult rv;

    nsCOMPtr<nsIProtocolProxyService> pps =
            do_GetService(NS_PROTOCOLPROXYSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIProxyInfo> pi;
    rv = pps->GetFailoverForProxy(mConnectionInfo->ProxyInfo(), mURI, mStatus,
                                  getter_AddRefs(pi));
    if (NS_FAILED(rv))
        return rv;

    
    
    return AsyncDoReplaceWithProxy(pi);
}

void
nsHttpChannel::HandleAsyncReplaceWithProxy()
{
    NS_PRECONDITION(!mPendingAsyncCallOnResume, "How did that happen?");

    if (mSuspendCount) {
        LOG(("Waiting until resume to do async proxy replacement [this=%p]\n",
             this));
        mPendingAsyncCallOnResume =
            &nsHttpChannel::HandleAsyncReplaceWithProxy;
        return;
    }

    nsresult status = mStatus;
    
    nsCOMPtr<nsIProxyInfo> pi;
    pi.swap(mTargetProxyInfo);
    if (!mCanceled) {
        PushRedirectAsyncFunc(&nsHttpChannel::ContinueHandleAsyncReplaceWithProxy);
        status = AsyncDoReplaceWithProxy(pi);
        if (NS_SUCCEEDED(status))
            return;
        PopRedirectAsyncFunc(&nsHttpChannel::ContinueHandleAsyncReplaceWithProxy);
    }

    if (NS_FAILED(status)) {
        ContinueHandleAsyncReplaceWithProxy(status);
    }
}

nsresult
nsHttpChannel::ContinueHandleAsyncReplaceWithProxy(nsresult status)
{
    if (mLoadGroup && NS_SUCCEEDED(status)) {
        mLoadGroup->RemoveRequest(this, nsnull, mStatus);
    }
    else if (NS_FAILED(status)) {
       AsyncAbort(status);
    }

    
    
    
    
    
    
    return NS_OK;
}

void
nsHttpChannel::HandleAsyncRedirectChannelToHttps()
{
    NS_PRECONDITION(!mPendingAsyncCallOnResume, "How did that happen?");

    if (mSuspendCount) {
        LOG(("Waiting until resume to do async redirect to https [this=%p]\n", this));
        mPendingAsyncCallOnResume = &nsHttpChannel::HandleAsyncRedirectChannelToHttps;
        return;
    }

    nsresult rv = AsyncRedirectChannelToHttps();
    if (NS_FAILED(rv))
        ContinueAsyncRedirectChannelToHttps(rv);
}

nsresult
nsHttpChannel::AsyncRedirectChannelToHttps()
{
    nsresult rv = NS_OK;
    LOG(("nsHttpChannel::HandleAsyncRedirectChannelToHttps() [STS]\n"));

    nsCOMPtr<nsIChannel> newChannel;
    nsCOMPtr<nsIURI> upgradedURI;

    rv = mURI->Clone(getter_AddRefs(upgradedURI));
    NS_ENSURE_SUCCESS(rv,rv);

    upgradedURI->SetScheme(NS_LITERAL_CSTRING("https"));

    PRInt32 oldPort = -1;
    rv = mURI->GetPort(&oldPort);
    if (NS_FAILED(rv)) return rv;

    
    
    
    

    if (oldPort == 80 || oldPort == -1)
        upgradedURI->SetPort(-1);
    else
        upgradedURI->SetPort(oldPort);

    nsCOMPtr<nsIIOService> ioService;
    rv = gHttpHandler->GetIOService(getter_AddRefs(ioService));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = ioService->NewChannelFromURI(upgradedURI, getter_AddRefs(newChannel));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = SetupReplacementChannel(upgradedURI, newChannel, PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    mRedirectChannel = newChannel;
    PRUint32 flags = nsIChannelEventSink::REDIRECT_PERMANENT;

    PushRedirectAsyncFunc(
        &nsHttpChannel::ContinueAsyncRedirectChannelToHttps);
    rv = gHttpHandler->AsyncOnChannelRedirect(this, newChannel, flags);

    if (NS_SUCCEEDED(rv))
        rv = WaitForRedirectCallback();

    if (NS_FAILED(rv)) {
        AutoRedirectVetoNotifier notifier(this);
        PopRedirectAsyncFunc(
            &nsHttpChannel::ContinueAsyncRedirectChannelToHttps);
    }

    return rv;
}

nsresult
nsHttpChannel::ContinueAsyncRedirectChannelToHttps(nsresult rv)
{
    AutoRedirectVetoNotifier notifier(this);

    if (NS_FAILED(rv)) {
        
        
        
        mStatus = rv;
    }

    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nsnull, mStatus);

    if (NS_FAILED(rv)) {
        
        
        
        DoNotifyListener();
        return rv;
    }

    
    mRedirectChannel->SetOriginalURI(mOriginalURI);

    
    nsCOMPtr<nsIHttpEventSink> httpEventSink;
    GetCallback(httpEventSink);
    if (httpEventSink) {
        
        
        rv = httpEventSink->OnRedirect(this, mRedirectChannel);
        if (NS_FAILED(rv)) {
            mStatus = rv;
            DoNotifyListener();
            return rv;
        }
    }

    
    rv = mRedirectChannel->AsyncOpen(mListener, mListenerContext);
    if (NS_FAILED(rv)) {
        mStatus = rv;
        DoNotifyListener();
        return rv;
    }

    mStatus = NS_BINDING_REDIRECTED;

    notifier.RedirectSucceeded();

    
    mListener = nsnull;
    mListenerContext = nsnull;

    
    mCallbacks = nsnull;
    mProgressSink = nsnull;

    return rv;
}

nsresult
nsHttpChannel::AsyncDoReplaceWithProxy(nsIProxyInfo* pi)
{
    LOG(("nsHttpChannel::AsyncDoReplaceWithProxy [this=%p pi=%p]", this, pi));
    nsresult rv;

    nsCOMPtr<nsIChannel> newChannel;
    rv = gHttpHandler->NewProxiedChannel(mURI, pi, getter_AddRefs(newChannel));
    if (NS_FAILED(rv))
        return rv;

    rv = SetupReplacementChannel(mURI, newChannel, PR_TRUE);
    if (NS_FAILED(rv))
        return rv;

    
    mRedirectChannel = newChannel;
    PRUint32 flags = nsIChannelEventSink::REDIRECT_INTERNAL;

    PushRedirectAsyncFunc(&nsHttpChannel::ContinueDoReplaceWithProxy);
    rv = gHttpHandler->AsyncOnChannelRedirect(this, newChannel, flags);

    if (NS_SUCCEEDED(rv))
        rv = WaitForRedirectCallback();

    if (NS_FAILED(rv)) {
        AutoRedirectVetoNotifier notifier(this);
        PopRedirectAsyncFunc(&nsHttpChannel::ContinueDoReplaceWithProxy);
    }

    return rv;
}

nsresult
nsHttpChannel::ContinueDoReplaceWithProxy(nsresult rv)
{
    AutoRedirectVetoNotifier notifier(this);

    if (NS_FAILED(rv))
        return rv;

    NS_PRECONDITION(mRedirectChannel, "No redirect channel?");

    
    mRedirectChannel->SetOriginalURI(mOriginalURI);

    
    rv = mRedirectChannel->AsyncOpen(mListener, mListenerContext);
    if (NS_FAILED(rv))
        return rv;

    mStatus = NS_BINDING_REDIRECTED;

    notifier.RedirectSucceeded();

    
    mListener = nsnull;
    mListenerContext = nsnull;

    
    mCallbacks = nsnull;
    mProgressSink = nsnull;

    return rv;
}

nsresult
nsHttpChannel::ResolveProxy()
{
    LOG(("nsHttpChannel::ResolveProxy [this=%p]\n", this));

    nsresult rv;

    nsCOMPtr<nsIProtocolProxyService> pps =
            do_GetService(NS_PROTOCOLPROXYSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return rv;

    return pps->AsyncResolve(mURI, 0, this, getter_AddRefs(mProxyRequest));
}

PRBool
nsHttpChannel::ResponseWouldVary()
{
    nsresult rv;
    nsCAutoString buf, metaKey;
    mCachedResponseHead->GetHeader(nsHttp::Vary, buf);
    if (!buf.IsEmpty()) {
        NS_NAMED_LITERAL_CSTRING(prefix, "request-");

        
        char *val = buf.BeginWriting(); 
        char *token = nsCRT::strtok(val, NS_HTTP_HEADER_SEPS, &val);
        while (token) {
            LOG(("nsHttpChannel::ResponseWouldVary [this=%x] " \
                 "processing %s\n",
                 this, token));
            
            
            
            
            
            
            
            
            
            
            
            
            
            if (*token == '*')
                return PR_TRUE; 

            
            metaKey = prefix + nsDependentCString(token);

            
            
            nsXPIDLCString lastVal;
            mCacheEntry->GetMetaDataElement(metaKey.get(), getter_Copies(lastVal));
            LOG(("nsHttpChannel::ResponseWouldVary [this=%x] " \
                    "stored value = %c%s%c\n", this, '"', lastVal.get(), '"'));

            
            nsHttpAtom atom = nsHttp::ResolveAtom(token);
            const char *newVal = mRequestHead.PeekHeader(atom);
            if (!lastVal.IsEmpty()) {
                
                if (!newVal)
                    return PR_TRUE; 

                
                
                
                nsCAutoString hash;
                if (atom == nsHttp::Cookie) {
                    rv = Hash(newVal, hash);
                    
                    
                    if (NS_FAILED(rv))
                        return PR_TRUE;
                    newVal = hash.get();

                    LOG(("nsHttpChannel::ResponseWouldVary [this=%x] " \
                            "set-cookie value hashed to %s\n",
                         this, newVal));
                }

                if (strcmp(newVal, lastVal))
                    return PR_TRUE; 

            } else if (newVal) { 
                return PR_TRUE;
            }

            
            token = nsCRT::strtok(val, NS_HTTP_HEADER_SEPS, &val);
        }
    }
    return PR_FALSE;
}

nsresult
nsHttpChannel::Hash(const char *buf, nsACString &hash)
{
    nsresult rv;
    if (!mHasher) {
        mHasher = do_CreateInstance(NS_CRYPTO_HASH_CONTRACTID, &rv);
        if (NS_FAILED(rv)) {
            LOG(("nsHttpChannel: Failed to instantiate crypto-hasher"));
            return rv;
        }
    }

    rv = mHasher->Init(nsICryptoHash::SHA1);
    NS_ENSURE_SUCCESS(rv, rv);

   rv = mHasher->Update(reinterpret_cast<unsigned const char*>(buf),
                         strlen(buf));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mHasher->Finish(PR_TRUE, hash);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}





nsresult
nsHttpChannel::SetupByteRangeRequest(PRUint32 partialLen)
{
    
    

    
    const char *val = mCachedResponseHead->PeekHeader(nsHttp::ETag);
    if (!val)
        val = mCachedResponseHead->PeekHeader(nsHttp::Last_Modified);
    if (!val) {
        
        
        NS_NOTREACHED("no cache validator");
        return NS_ERROR_FAILURE;
    }

    char buf[32];
    PR_snprintf(buf, sizeof(buf), "bytes=%u-", partialLen);

    mRequestHead.SetHeader(nsHttp::Range, nsDependentCString(buf));
    mRequestHead.SetHeader(nsHttp::If_Range, nsDependentCString(val));

    return NS_OK;
}

nsresult
nsHttpChannel::ProcessPartialContent()
{
    
    
    
    

    LOG(("nsHttpChannel::ProcessPartialContent [this=%p]\n", this)); 

    NS_ENSURE_TRUE(mCachedResponseHead, NS_ERROR_NOT_INITIALIZED);
    NS_ENSURE_TRUE(mCacheEntry, NS_ERROR_NOT_INITIALIZED);

    
    ClearBogusContentEncodingIfNeeded();
    
    
    
    if (PL_strcasecmp(mResponseHead->PeekHeader(nsHttp::Content_Encoding),
                      mCachedResponseHead->PeekHeader(nsHttp::Content_Encoding))
                      != 0) {
        Cancel(NS_ERROR_INVALID_CONTENT_ENCODING);
        return CallOnStartRequest();
    }


    
    nsresult rv = mTransactionPump->Suspend();
    if (NS_FAILED(rv)) return rv;

    
    rv = mCachedResponseHead->UpdateHeaders(mResponseHead->Headers());
    if (NS_FAILED(rv)) return rv;

    
    nsCAutoString head;
    mCachedResponseHead->Flatten(head, PR_TRUE);
    rv = mCacheEntry->SetMetaDataElement("response-head", head.get());
    if (NS_FAILED(rv)) return rv;

    
    mResponseHead = mCachedResponseHead;

    rv = UpdateExpirationTime();
    if (NS_FAILED(rv)) return rv;

    
    
    gHttpHandler->OnExamineMergedResponse(this);

    
    mCachedContentIsValid = PR_TRUE;
    return ReadFromCache();
}

nsresult
nsHttpChannel::OnDoneReadingPartialCacheEntry(PRBool *streamDone)
{
    nsresult rv;

    LOG(("nsHttpChannel::OnDoneReadingPartialCacheEntry [this=%p]", this));

    
    *streamDone = PR_TRUE;

    
    PRUint32 size;
    rv = mCacheEntry->GetDataSize(&size);
    if (NS_FAILED(rv)) return rv;

    rv = InstallCacheListener(size);
    if (NS_FAILED(rv)) return rv;

    
    mLogicalOffset = size;

    
    
    mCachedContentIsPartial = PR_FALSE;

    
    
    if (mTransactionPump) {
        rv = mTransactionPump->Resume();
        if (NS_SUCCEEDED(rv))
            *streamDone = PR_FALSE;
    }
    else
        NS_NOTREACHED("no transaction");
    return rv;
}





nsresult
nsHttpChannel::ProcessNotModified()
{
    nsresult rv;

    LOG(("nsHttpChannel::ProcessNotModified [this=%p]\n", this)); 

    if (mCustomConditionalRequest) {
        LOG(("Bypassing ProcessNotModified due to custom conditional headers")); 
        return NS_ERROR_FAILURE;
    }

    NS_ENSURE_TRUE(mCachedResponseHead, NS_ERROR_NOT_INITIALIZED);
    NS_ENSURE_TRUE(mCacheEntry, NS_ERROR_NOT_INITIALIZED);

    
    rv = mCachedResponseHead->UpdateHeaders(mResponseHead->Headers());
    if (NS_FAILED(rv)) return rv;

    
    nsCAutoString head;
    mCachedResponseHead->Flatten(head, PR_TRUE);
    rv = mCacheEntry->SetMetaDataElement("response-head", head.get());
    if (NS_FAILED(rv)) return rv;

    
    mResponseHead = mCachedResponseHead;

    rv = UpdateExpirationTime();
    if (NS_FAILED(rv)) return rv;

    
    
    gHttpHandler->OnExamineMergedResponse(this);

    mCachedContentIsValid = PR_TRUE;
    rv = ReadFromCache();
    if (NS_FAILED(rv)) return rv;

    mTransactionReplaced = PR_TRUE;
    return NS_OK;
}

nsresult
nsHttpChannel::ProcessFallback(PRBool *waitingForRedirectCallback)
{
    LOG(("nsHttpChannel::ProcessFallback [this=%p]\n", this));
    nsresult rv;

    *waitingForRedirectCallback = PR_FALSE;
    mFallingBack = PR_FALSE;

    
    
    
    if (!mApplicationCache || mFallbackKey.IsEmpty() || mFallbackChannel) {
        LOG(("  choosing not to fallback [%p,%s,%d]",
             mApplicationCache.get(), mFallbackKey.get(), mFallbackChannel));
        return NS_OK;
    }

    
    
    PRUint32 fallbackEntryType;
    rv = mApplicationCache->GetTypes(mFallbackKey, &fallbackEntryType);
    NS_ENSURE_SUCCESS(rv, rv);

    if (fallbackEntryType & nsIApplicationCache::ITEM_FOREIGN) {
        
        
        return NS_OK;
    }

    NS_ASSERTION(fallbackEntryType & nsIApplicationCache::ITEM_FALLBACK,
                 "Fallback entry not marked correctly!");

    
    
    if (mOfflineCacheEntry) {
        mOfflineCacheEntry->Doom();
        mOfflineCacheEntry = 0;
        mOfflineCacheAccess = 0;
    }

    mCacheForOfflineUse = PR_FALSE;
    mCachingOpportunistically = PR_FALSE;
    mOfflineCacheClientID.Truncate();
    mOfflineCacheEntry = 0;
    mOfflineCacheAccess = 0;

    
    if (mCacheEntry)
        CloseCacheEntry(PR_TRUE);

    
    nsRefPtr<nsIChannel> newChannel;
    rv = gHttpHandler->NewChannel(mURI, getter_AddRefs(newChannel));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = SetupReplacementChannel(mURI, newChannel, PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIHttpChannelInternal> httpInternal =
        do_QueryInterface(newChannel, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = httpInternal->SetupFallbackChannel(mFallbackKey.get());
    NS_ENSURE_SUCCESS(rv, rv);

    
    PRUint32 newLoadFlags = mLoadFlags | LOAD_REPLACE | LOAD_ONLY_FROM_CACHE;
    rv = newChannel->SetLoadFlags(newLoadFlags);

    
    mRedirectChannel = newChannel;
    PRUint32 redirectFlags = nsIChannelEventSink::REDIRECT_INTERNAL;

    PushRedirectAsyncFunc(&nsHttpChannel::ContinueProcessFallback);
    rv = gHttpHandler->AsyncOnChannelRedirect(this, newChannel, redirectFlags);

    if (NS_SUCCEEDED(rv))
        rv = WaitForRedirectCallback();

    if (NS_FAILED(rv)) {
        AutoRedirectVetoNotifier notifier(this);
        PopRedirectAsyncFunc(&nsHttpChannel::ContinueProcessFallback);
        return rv;
    }

    
    
    *waitingForRedirectCallback = PR_TRUE;
    return NS_OK;
}

nsresult
nsHttpChannel::ContinueProcessFallback(nsresult rv)
{
    AutoRedirectVetoNotifier notifier(this);

    if (NS_FAILED(rv))
        return rv;

    NS_PRECONDITION(mRedirectChannel, "No redirect channel?");

    
    mRedirectChannel->SetOriginalURI(mOriginalURI);

    rv = mRedirectChannel->AsyncOpen(mListener, mListenerContext);
    if (NS_FAILED(rv))
        return rv;

    
    Cancel(NS_BINDING_REDIRECTED);

    notifier.RedirectSucceeded();

    
    mListener = 0;
    mListenerContext = 0;

    
    mCallbacks = nsnull;
    mProgressSink = nsnull;

    mFallingBack = PR_TRUE;

    return NS_OK;
}



static PRBool
IsSubRangeRequest(nsHttpRequestHead &aRequestHead)
{
    if (!aRequestHead.PeekHeader(nsHttp::Range))
        return PR_FALSE;
    nsCAutoString byteRange;
    aRequestHead.GetHeader(nsHttp::Range, byteRange);
    return !byteRange.EqualsLiteral("bytes=0-");
}

nsresult
nsHttpChannel::OpenCacheEntry()
{
    nsresult rv;

    mAsyncCacheOpen = PR_FALSE;
    mLoadedFromApplicationCache = PR_FALSE;

    LOG(("nsHttpChannel::OpenCacheEntry [this=%p]", this));

    
    NS_PRECONDITION(!mCacheEntry, "cache entry already open");

    nsCAutoString cacheKey;

    if (mRequestHead.Method() == nsHttp::Post) {
        
        
        
        if (mPostID == 0)
            mPostID = gHttpHandler->GenerateUniqueID();
    }
    else if ((mRequestHead.Method() != nsHttp::Get) &&
             (mRequestHead.Method() != nsHttp::Head)) {
        
        return NS_OK;
    }

    if (mResuming) {
        
        
        return NS_OK;
    }

    
    
    if (IsSubRangeRequest(mRequestHead))
        return NS_OK;

    GenerateCacheKey(mPostID, cacheKey);

    
    nsCacheAccessMode accessRequested;
    rv = DetermineCacheAccess(&accessRequested);
    if (NS_FAILED(rv)) return rv;

    if (!mApplicationCache && mInheritApplicationCache) {
        
        
        nsCOMPtr<nsIApplicationCacheContainer> appCacheContainer;
        GetCallback(appCacheContainer);

        if (appCacheContainer) {
            appCacheContainer->GetApplicationCache(getter_AddRefs(mApplicationCache));
        }
    }

    if (!mApplicationCache &&
        (mChooseApplicationCache || (mLoadFlags & LOAD_CHECK_OFFLINE_CACHE))) {
        
        
        
        nsCOMPtr<nsIApplicationCacheService> appCacheService =
            do_GetService(NS_APPLICATIONCACHESERVICE_CONTRACTID);
        if (appCacheService) {
            nsresult rv = appCacheService->ChooseApplicationCache
                (cacheKey, getter_AddRefs(mApplicationCache));
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    nsCOMPtr<nsICacheSession> session;

    
    if (mApplicationCache) {
        nsCAutoString appCacheClientID;
        mApplicationCache->GetClientID(appCacheClientID);

        nsCOMPtr<nsICacheService> serv =
            do_GetService(NS_CACHESERVICE_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = serv->CreateSession(appCacheClientID.get(),
                                 nsICache::STORE_OFFLINE,
                                 nsICache::STREAM_BASED,
                                 getter_AddRefs(session));
        NS_ENSURE_SUCCESS(rv, rv);

        if (mLoadFlags & LOAD_BYPASS_LOCAL_CACHE_IF_BUSY) {
            
            rv = session->OpenCacheEntry(cacheKey,
                                         nsICache::ACCESS_READ, PR_FALSE,
                                         getter_AddRefs(mCacheEntry));
            if (NS_SUCCEEDED(rv)) {
                mCacheEntry->GetAccessGranted(&mCacheAccess);
                LOG(("nsHttpChannel::OpenCacheEntry [this=%p grantedAccess=%d]",
                    this, mCacheAccess));
                mLoadedFromApplicationCache = PR_TRUE;
                return NS_OK;
            } else if (rv == NS_ERROR_CACHE_WAIT_FOR_VALIDATION) {
                LOG(("bypassing local cache since it is busy\n"));
                
                return NS_ERROR_NOT_AVAILABLE;
            }
        } else {
            mOnCacheEntryAvailableCallback =
                &nsHttpChannel::OnOfflineCacheEntryAvailable;
            
            
            
            
            rv = session->AsyncOpenCacheEntry(cacheKey,
                                              nsICache::ACCESS_READ,
                                              this);

            if (NS_SUCCEEDED(rv)) {
                mAsyncCacheOpen = PR_TRUE;
                return NS_OK;
            }
        }

        
        return OnOfflineCacheEntryAvailable(nsnull, nsICache::ACCESS_NONE,
                                            rv, PR_TRUE);
    }

    return OpenNormalCacheEntry(PR_TRUE);
}

nsresult
nsHttpChannel::OnOfflineCacheEntryAvailable(nsICacheEntryDescriptor *aEntry,
                                            nsCacheAccessMode aAccess,
                                            nsresult aEntryStatus,
                                            PRBool aIsSync)
{
    nsresult rv;

    if (NS_SUCCEEDED(aEntryStatus)) {
        
        
        mLoadedFromApplicationCache = PR_TRUE;
        mCacheEntry = aEntry;
        mCacheAccess = aAccess;
    }

    if (mCanceled && NS_FAILED(mStatus)) {
        LOG(("channel was canceled [this=%p status=%x]\n", this, mStatus));
        return mStatus;
    }

    if (NS_SUCCEEDED(aEntryStatus))
        
        return Connect(PR_FALSE);

    if (!mCacheForOfflineUse && !mFallbackChannel) {
        nsCAutoString cacheKey;
        GenerateCacheKey(mPostID, cacheKey);

        
        nsCOMPtr<nsIApplicationCacheNamespace> namespaceEntry;
        rv = mApplicationCache->GetMatchingNamespace
            (cacheKey, getter_AddRefs(namespaceEntry));
        if (NS_FAILED(rv) && !aIsSync)
            return Connect(PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);

        PRUint32 namespaceType = 0;
        if (!namespaceEntry ||
            NS_FAILED(namespaceEntry->GetItemType(&namespaceType)) ||
            (namespaceType &
             (nsIApplicationCacheNamespace::NAMESPACE_FALLBACK |
              nsIApplicationCacheNamespace::NAMESPACE_OPPORTUNISTIC |
              nsIApplicationCacheNamespace::NAMESPACE_BYPASS)) == 0) {
            
            
            
            
            mLoadFlags |= LOAD_ONLY_FROM_CACHE;

            
            
            return aIsSync ? NS_ERROR_CACHE_KEY_NOT_FOUND : Connect(PR_FALSE);
        }

        if (namespaceType &
            nsIApplicationCacheNamespace::NAMESPACE_FALLBACK) {
            rv = namespaceEntry->GetData(mFallbackKey);
            if (NS_FAILED(rv) && !aIsSync)
                return Connect(PR_FALSE);
            NS_ENSURE_SUCCESS(rv, rv);
        }

        if ((namespaceType &
             nsIApplicationCacheNamespace::NAMESPACE_OPPORTUNISTIC) &&
            mLoadFlags & LOAD_DOCUMENT_URI) {
            
            
            nsCString clientID;
            mApplicationCache->GetClientID(clientID);

            mCacheForOfflineUse = !clientID.IsEmpty();
            SetOfflineCacheClientID(clientID);
            mCachingOpportunistically = PR_TRUE;
        }
    }

    return OpenNormalCacheEntry(aIsSync);
}


nsresult
nsHttpChannel::OpenNormalCacheEntry(PRBool aIsSync)
{
    NS_ASSERTION(!mCacheEntry, "We have already mCacheEntry");

    nsresult rv;

    nsCAutoString cacheKey;
    GenerateCacheKey(mPostID, cacheKey);

    nsCacheStoragePolicy storagePolicy = DetermineStoragePolicy();

    nsCOMPtr<nsICacheSession> session;
    rv = gHttpHandler->GetCacheSession(storagePolicy,
                                       getter_AddRefs(session));
    if (NS_FAILED(rv)) return rv;

    nsCacheAccessMode accessRequested;
    rv = DetermineCacheAccess(&accessRequested);
    if (NS_FAILED(rv)) return rv;

    if (mLoadFlags & LOAD_BYPASS_LOCAL_CACHE_IF_BUSY) {
        if (!aIsSync) {
            
            
            
            
            NS_WARNING(
                "OpenNormalCacheEntry() called from OnCacheEntryAvailable() "
                "when LOAD_BYPASS_LOCAL_CACHE_IF_BUSY was specified");
        }

        
        rv = session->OpenCacheEntry(cacheKey, accessRequested, PR_FALSE,
                                     getter_AddRefs(mCacheEntry));
        if (NS_SUCCEEDED(rv)) {
            mCacheEntry->GetAccessGranted(&mCacheAccess);
            LOG(("nsHttpChannel::OpenCacheEntry [this=%p grantedAccess=%d]",
                this, mCacheAccess));
        }
        else if (rv == NS_ERROR_CACHE_WAIT_FOR_VALIDATION) {
            LOG(("bypassing local cache since it is busy\n"));
            rv = NS_ERROR_NOT_AVAILABLE;
        }
    }
    else {
        mOnCacheEntryAvailableCallback =
            &nsHttpChannel::OnNormalCacheEntryAvailable;
        rv = session->AsyncOpenCacheEntry(cacheKey, accessRequested, this);
        if (NS_SUCCEEDED(rv)) {
            mAsyncCacheOpen = PR_TRUE;
            return NS_OK;
        }
    }

    if (!aIsSync)
        
        rv = Connect(PR_FALSE);

    return rv;
}

nsresult
nsHttpChannel::OnNormalCacheEntryAvailable(nsICacheEntryDescriptor *aEntry,
                                           nsCacheAccessMode aAccess,
                                           nsresult aEntryStatus,
                                           PRBool aIsSync)
{
    NS_ASSERTION(!aIsSync, "aIsSync should be false");

    if (NS_SUCCEEDED(aEntryStatus)) {
        mCacheEntry = aEntry;
        mCacheAccess = aAccess;
    }

    if (mCanceled && NS_FAILED(mStatus)) {
        LOG(("channel was canceled [this=%p status=%x]\n", this, mStatus));
        return mStatus;
    }

    if ((mLoadFlags & LOAD_ONLY_FROM_CACHE) && NS_FAILED(aEntryStatus))
        
        
        return NS_ERROR_DOCUMENT_NOT_CACHED;

    
    return Connect(PR_FALSE);
}


nsresult
nsHttpChannel::OpenOfflineCacheEntryForWriting()
{
    nsresult rv;

    LOG(("nsHttpChannel::OpenOfflineCacheEntryForWriting [this=%p]", this));

    
    NS_PRECONDITION(!mOfflineCacheEntry, "cache entry already open");

    PRBool offline = gIOService->IsOffline();
    if (offline) {
        
        return NS_OK;
    }

    if (mRequestHead.Method() != nsHttp::Get) {
        
        return NS_OK;
    }

    
    
    if (IsSubRangeRequest(mRequestHead))
        return NS_OK;

    nsCAutoString cacheKey;
    GenerateCacheKey(mPostID, cacheKey);

    NS_ENSURE_TRUE(!mOfflineCacheClientID.IsEmpty(),
                   NS_ERROR_NOT_AVAILABLE);

    nsCOMPtr<nsICacheSession> session;
    nsCOMPtr<nsICacheService> serv =
        do_GetService(NS_CACHESERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = serv->CreateSession(mOfflineCacheClientID.get(),
                             nsICache::STORE_OFFLINE,
                             nsICache::STREAM_BASED,
                             getter_AddRefs(session));
    if (NS_FAILED(rv)) return rv;

    rv = session->OpenCacheEntry(cacheKey, nsICache::ACCESS_READ_WRITE,
                                 PR_FALSE, getter_AddRefs(mOfflineCacheEntry));

    if (rv == NS_ERROR_CACHE_WAIT_FOR_VALIDATION) {
        
        
        
        return NS_OK;
    }

    if (NS_SUCCEEDED(rv)) {
        mOfflineCacheEntry->GetAccessGranted(&mOfflineCacheAccess);
        LOG(("got offline cache entry [access=%x]\n", mOfflineCacheAccess));
    }

    return rv;
}

nsresult
nsHttpChannel::GenerateCacheKey(PRUint32 postID, nsACString &cacheKey)
{
    cacheKey.Truncate();

    if (mLoadFlags & LOAD_ANONYMOUS) {
      cacheKey.AssignLiteral("anon&");
    }

    if (postID) {
        char buf[32];
        PR_snprintf(buf, sizeof(buf), "id=%x&", postID);
        cacheKey.Append(buf);
    }

    if (!cacheKey.IsEmpty()) {
      cacheKey.AppendLiteral("uri=");
    }

    
    const char *spec = mFallbackChannel ? mFallbackKey.get() : mSpec.get();
    const char *p = strchr(spec, '#');
    if (p)
        cacheKey.Append(spec, p - spec);
    else
        cacheKey.Append(spec);
    return NS_OK;
}










nsresult
nsHttpChannel::UpdateExpirationTime()
{
    NS_ENSURE_TRUE(mResponseHead, NS_ERROR_FAILURE);

    nsresult rv;

    PRUint32 expirationTime = 0;
    if (!mResponseHead->MustValidate()) {
        PRUint32 freshnessLifetime = 0;

        rv = mResponseHead->ComputeFreshnessLifetime(&freshnessLifetime);
        if (NS_FAILED(rv)) return rv;

        if (freshnessLifetime > 0) {
            PRUint32 now = NowInSeconds(), currentAge = 0;

            rv = mResponseHead->ComputeCurrentAge(now, mRequestTime, &currentAge); 
            if (NS_FAILED(rv)) return rv;

            LOG(("freshnessLifetime = %u, currentAge = %u\n",
                freshnessLifetime, currentAge));

            if (freshnessLifetime > currentAge) {
                PRUint32 timeRemaining = freshnessLifetime - currentAge;
                
                if (now + timeRemaining < now)
                    expirationTime = PRUint32(-1);
                else
                    expirationTime = now + timeRemaining;
            }
            else
                expirationTime = now;
        }
    }

    rv = mCacheEntry->SetExpirationTime(expirationTime);
    NS_ENSURE_SUCCESS(rv, rv);

    if (mOfflineCacheEntry) {
        rv = mOfflineCacheEntry->SetExpirationTime(expirationTime);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}





nsresult
nsHttpChannel::CheckCache()
{
    nsresult rv = NS_OK;

    LOG(("nsHTTPChannel::CheckCache enter [this=%p entry=%p access=%d]",
        this, mCacheEntry.get(), mCacheAccess));
    
    
    mCachedContentIsValid = PR_FALSE;

    
    if (!mCacheEntry || !(mCacheAccess & nsICache::ACCESS_READ))
        return NS_OK;

    nsXPIDLCString buf;

    
    rv = mCacheEntry->GetMetaDataElement("request-method", getter_Copies(buf));
    NS_ENSURE_SUCCESS(rv, rv);

    nsHttpAtom method = nsHttp::ResolveAtom(buf);
    if (method == nsHttp::Head) {
        
        
        if (mRequestHead.Method() != nsHttp::Head)
            return NS_OK;
    }
    buf.Adopt(0);

    
    PRUint32 lastModifiedTime;
    rv = mCacheEntry->GetLastModified(&lastModifiedTime);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    PRBool fromPreviousSession =
            (gHttpHandler->SessionStartTime() > lastModifiedTime);

    
    rv = mCacheEntry->GetMetaDataElement("response-head", getter_Copies(buf));
    NS_ENSURE_SUCCESS(rv, rv);

    
    mCachedResponseHead = new nsHttpResponseHead();
    if (!mCachedResponseHead)
        return NS_ERROR_OUT_OF_MEMORY;
    rv = mCachedResponseHead->Parse((char *) buf.get());
    NS_ENSURE_SUCCESS(rv, rv);
    buf.Adopt(0);

    
    
    
    
    if (!mCacheForOfflineUse &&
        (mLoadedFromApplicationCache ||
         (mCacheAccess == nsICache::ACCESS_READ &&
          !(mLoadFlags & INHIBIT_CACHING)) ||
         mFallbackChannel)) {
        mCachedContentIsValid = PR_TRUE;
        return NS_OK;
    }

    PRUint16 isCachedRedirect = mCachedResponseHead->Status()/100 == 3;

    mCustomConditionalRequest =
        mRequestHead.PeekHeader(nsHttp::If_Modified_Since) ||
        mRequestHead.PeekHeader(nsHttp::If_None_Match) ||
        mRequestHead.PeekHeader(nsHttp::If_Unmodified_Since) ||
        mRequestHead.PeekHeader(nsHttp::If_Match) ||
        mRequestHead.PeekHeader(nsHttp::If_Range);

    if (method != nsHttp::Head && !isCachedRedirect) {
        
        
        
        
        PRInt64 contentLength = mCachedResponseHead->ContentLength();
        if (contentLength != PRInt64(-1)) {
            PRUint32 size;
            rv = mCacheEntry->GetDataSize(&size);
            NS_ENSURE_SUCCESS(rv, rv);

            if (PRInt64(size) != contentLength) {
                LOG(("Cached data size does not match the Content-Length header "
                     "[content-length=%lld size=%u]\n", PRInt64(contentLength), size));

                PRBool hasContentEncoding =
                    mCachedResponseHead->PeekHeader(nsHttp::Content_Encoding)
                    != nsnull;
                if ((PRInt64(size) < contentLength) &&
                     size > 0 &&
                     !hasContentEncoding &&
                     mCachedResponseHead->IsResumable() &&
                     !mCustomConditionalRequest &&
                     !mCachedResponseHead->NoStore()) {
                    
                    rv = SetupByteRangeRequest(size);
                    NS_ENSURE_SUCCESS(rv, rv);
                    mCachedContentIsPartial = PR_TRUE;
                }
                return NS_OK;
            }
        }
    }

    PRBool doValidation = PR_FALSE;
    PRBool canAddImsHeader = PR_TRUE;

    
    if (mLoadFlags & LOAD_FROM_CACHE) {
        LOG(("NOT validating based on LOAD_FROM_CACHE load flag\n"));
        doValidation = PR_FALSE;
    }
    
    
    else if (mLoadFlags & VALIDATE_ALWAYS) {
        LOG(("Validating based on VALIDATE_ALWAYS load flag\n"));
        doValidation = PR_TRUE;
    }
    
    
    else if (mLoadFlags & VALIDATE_NEVER) {
        LOG(("VALIDATE_NEVER set\n"));
        
        
        if (mCachedResponseHead->NoStore() ||
           (mCachedResponseHead->NoCache() && mConnectionInfo->UsingSSL())) {
            LOG(("Validating based on (no-store || (no-cache && ssl)) logic\n"));
            doValidation = PR_TRUE;
        }
        else {
            LOG(("NOT validating based on VALIDATE_NEVER load flag\n"));
            doValidation = PR_FALSE;
        }
    }
    
    else if (mCachedResponseHead->MustValidate()) {
        LOG(("Validating based on MustValidate() returning TRUE\n"));
        doValidation = PR_TRUE;
    }

    else if (ResponseWouldVary()) {
        LOG(("Validating based on Vary headers returning TRUE\n"));
        canAddImsHeader = PR_FALSE;
        doValidation = PR_TRUE;
    }
    
    else if (MustValidateBasedOnQueryUrl()) {
        LOG(("Validating based on RFC 2616 section 13.9 "
             "(query-url w/o explicit expiration-time)\n"));
        doValidation = PR_TRUE;
    }
    
    else {
        PRUint32 time = 0; 

        rv = mCacheEntry->GetExpirationTime(&time);
        NS_ENSURE_SUCCESS(rv, rv);

        if (NowInSeconds() <= time)
            doValidation = PR_FALSE;
        else if (mCachedResponseHead->MustValidateIfExpired())
            doValidation = PR_TRUE;
        else if (mLoadFlags & VALIDATE_ONCE_PER_SESSION) {
            
            
            
            
            
            rv = mCachedResponseHead->ComputeFreshnessLifetime(&time);
            NS_ENSURE_SUCCESS(rv, rv);

            if (time == 0)
                doValidation = PR_TRUE;
            else
                doValidation = fromPreviousSession;
        }
        else
            doValidation = PR_TRUE;

        LOG(("%salidating based on expiration time\n", doValidation ? "V" : "Not v"));
    }

    if (!doValidation && mRequestHead.PeekHeader(nsHttp::If_Match) &&
        (method == nsHttp::Get || method == nsHttp::Head)) {
        const char *requestedETag, *cachedETag;
        cachedETag = mCachedResponseHead->PeekHeader(nsHttp::ETag);
        requestedETag = mRequestHead.PeekHeader(nsHttp::If_Match);
        if (cachedETag && (!strncmp(cachedETag, "W/", 2) ||
            strcmp(requestedETag, cachedETag))) {
            
            
            
            doValidation = PR_TRUE;
        }
    }

    if (!doValidation) {
        
        
        
        
        
        
        
        
        
        
        
        mCacheEntry->GetMetaDataElement("auth", getter_Copies(buf));
        doValidation =
            (fromPreviousSession && !buf.IsEmpty()) ||
            (buf.IsEmpty() && mRequestHead.PeekHeader(nsHttp::Authorization));
    }

    
    
    
    
    
    
    if (!doValidation && isCachedRedirect) {
        nsCAutoString cacheKey;
        GenerateCacheKey(mPostID, cacheKey);

        if (!mRedirectedCachekeys)
            mRedirectedCachekeys = new nsTArray<nsCString>();
        else if (mRedirectedCachekeys->Contains(cacheKey))
            doValidation = PR_TRUE;

        LOG(("Redirection-chain %s key %s\n",
             doValidation ? "contains" : "does not contain", cacheKey.get()));

        
        if (!doValidation)
            mRedirectedCachekeys->AppendElement(cacheKey);
    }

    mCachedContentIsValid = !doValidation;

    if (doValidation) {
        
        
        
        
        
        
        
        
        
        
        
        if (!mCachedResponseHead->NoStore() &&
            (mRequestHead.Method() == nsHttp::Get ||
             mRequestHead.Method() == nsHttp::Head) &&
             !mCustomConditionalRequest) {
            const char *val;
            
            
            if (canAddImsHeader) {
                val = mCachedResponseHead->PeekHeader(nsHttp::Last_Modified);
                if (val)
                    mRequestHead.SetHeader(nsHttp::If_Modified_Since,
                                           nsDependentCString(val));
            }
            
            val = mCachedResponseHead->PeekHeader(nsHttp::ETag);
            if (val)
                mRequestHead.SetHeader(nsHttp::If_None_Match,
                                       nsDependentCString(val));
        }

        
        CleanRedirectCacheChainIfNecessary();
    }

    LOG(("nsHTTPChannel::CheckCache exit [this=%p doValidation=%d]\n", this, doValidation));
    return NS_OK;
}

PRBool
nsHttpChannel::MustValidateBasedOnQueryUrl()
{
    
    
    
    
    if (mRequestHead.Method() == nsHttp::Get)
    {
        nsCAutoString query;
        nsCOMPtr<nsIURL> url = do_QueryInterface(mURI);
        nsresult rv = url->GetQuery(query);
        if (NS_SUCCEEDED(rv) && !query.IsEmpty()) {
            PRUint32 tmp; 
            rv = mCachedResponseHead->GetExpiresValue(&tmp);
            if (NS_FAILED(rv)) {
                rv = mCachedResponseHead->GetMaxAgeValue(&tmp);
                if (NS_FAILED(rv)) {
                    return PR_TRUE;
                }
            }
        }
    }
    return PR_FALSE;
}


nsresult
nsHttpChannel::ShouldUpdateOfflineCacheEntry(PRBool *shouldCacheForOfflineUse)
{
    *shouldCacheForOfflineUse = PR_FALSE;

    if (!mOfflineCacheEntry) {
        return NS_OK;
    }

    
    if (mCacheEntry && (mCacheAccess & nsICache::ACCESS_WRITE)) {
        *shouldCacheForOfflineUse = PR_TRUE;
        return NS_OK;
    }

    
    if (mOfflineCacheEntry && (mOfflineCacheAccess == nsICache::ACCESS_WRITE)) {
        *shouldCacheForOfflineUse = PR_TRUE;
        return NS_OK;
    }

    
    PRUint32 docLastModifiedTime;
    nsresult rv = mResponseHead->GetLastModifiedValue(&docLastModifiedTime);
    if (NS_FAILED(rv)) {
        *shouldCacheForOfflineUse = PR_TRUE;
        return NS_OK;
    }

    PRUint32 offlineLastModifiedTime;
    rv = mOfflineCacheEntry->GetLastModified(&offlineLastModifiedTime);
    NS_ENSURE_SUCCESS(rv, rv);

    if (docLastModifiedTime > offlineLastModifiedTime) {
        *shouldCacheForOfflineUse = PR_TRUE;
        return NS_OK;
    }

    return NS_OK;
}





nsresult
nsHttpChannel::ReadFromCache()
{
    nsresult rv;

    NS_ENSURE_TRUE(mCacheEntry, NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(mCachedContentIsValid, NS_ERROR_FAILURE);

    LOG(("nsHttpChannel::ReadFromCache [this=%p] "
         "Using cached copy of: %s\n", this, mSpec.get()));

    if (mCachedResponseHead)
        mResponseHead = mCachedResponseHead;

    
    
    
    
    if (!mSecurityInfo)
        mCacheEntry->GetSecurityInfo(getter_AddRefs(mSecurityInfo));

    if ((mCacheAccess & nsICache::ACCESS_WRITE) && !mCachedContentIsPartial) {
        
        
        
        mCacheEntry->MarkValid();
    }

    
    
    
    if (mResponseHead && (mResponseHead->Status() / 100 == 3) 
                      && (mResponseHead->PeekHeader(nsHttp::Location)))
        return AsyncCall(&nsHttpChannel::HandleAsyncRedirect);

    
    if ((mLoadFlags & LOAD_ONLY_IF_MODIFIED) && !mCachedContentIsPartial) {
        
        
        
        PRBool shouldUpdateOffline;
        if (!mCacheForOfflineUse ||
            NS_FAILED(ShouldUpdateOfflineCacheEntry(&shouldUpdateOffline)) ||
            !shouldUpdateOffline) {

            LOG(("skipping read from cache based on LOAD_ONLY_IF_MODIFIED "
                 "load flag\n"));
            return AsyncCall(&nsHttpChannel::HandleAsyncNotModified);
        }
    }

    
    nsCOMPtr<nsIInputStream> stream;
    rv = mCacheEntry->OpenInputStream(0, getter_AddRefs(stream));
    if (NS_FAILED(rv)) return rv;

    rv = nsInputStreamPump::Create(getter_AddRefs(mCachePump),
                                   stream, PRInt64(-1), PRInt64(-1), 0, 0,
                                   PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    rv = mCachePump->AsyncRead(this, mListenerContext);
    if (NS_FAILED(rv)) return rv;

    PRUint32 suspendCount = mSuspendCount;
    while (suspendCount--)
        mCachePump->Suspend();

    return NS_OK;
}

void
nsHttpChannel::CloseCacheEntry(PRBool doomOnFailure)
{
    if (!mCacheEntry)
        return;

    LOG(("nsHttpChannel::CloseCacheEntry [this=%p] mStatus=%x mCacheAccess=%x",
         this, mStatus, mCacheAccess));

    
    
    
    

    PRBool doom = PR_FALSE;
    if (mInitedCacheEntry) {
        NS_ASSERTION(mResponseHead, "oops");
        if (NS_FAILED(mStatus) && doomOnFailure &&
            (mCacheAccess & nsICache::ACCESS_WRITE) &&
            !mResponseHead->IsResumable())
            doom = PR_TRUE;
    }
    else if (mCacheAccess == nsICache::ACCESS_WRITE)
        doom = PR_TRUE;

    if (doom) {
        LOG(("  dooming cache entry!!"));
        mCacheEntry->Doom();
    }

    mCachedResponseHead = nsnull;

    mCachePump = 0;
    mCacheEntry = 0;
    mCacheAccess = 0;
    mInitedCacheEntry = PR_FALSE;
}


void
nsHttpChannel::CloseOfflineCacheEntry()
{
    if (!mOfflineCacheEntry)
        return;

    LOG(("nsHttpChannel::CloseOfflineCacheEntry [this=%p]", this));

    if (NS_FAILED(mStatus)) {
        mOfflineCacheEntry->Doom();
    }
    else {
        PRBool succeeded;
        if (NS_SUCCEEDED(GetRequestSucceeded(&succeeded)) && !succeeded)
            mOfflineCacheEntry->Doom();
    }

    mOfflineCacheEntry = 0;
    mOfflineCacheAccess = 0;

    if (mCachingOpportunistically) {
        nsCOMPtr<nsIApplicationCacheService> appCacheService =
            do_GetService(NS_APPLICATIONCACHESERVICE_CONTRACTID);
        if (appCacheService) {
            nsCAutoString cacheKey;
            GenerateCacheKey(mPostID, cacheKey);
            appCacheService->CacheOpportunistically(mApplicationCache,
                                                    cacheKey);
        }
    }
}







nsresult
nsHttpChannel::InitCacheEntry()
{
    nsresult rv;

    NS_ENSURE_TRUE(mCacheEntry, NS_ERROR_UNEXPECTED);
    
    if (mCacheAccess == nsICache::ACCESS_READ)
        return NS_OK;

    
    if (mCachedContentIsValid)
        return NS_OK;

    LOG(("nsHttpChannel::InitCacheEntry [this=%p entry=%p]\n",
        this, mCacheEntry.get()));

    
    
    if (mResponseHead->NoStore())
        mLoadFlags |= INHIBIT_PERSISTENT_CACHING;

    
    if (!gHttpHandler->IsPersistentHttpsCachingEnabled() &&
        mConnectionInfo->UsingSSL())
        mLoadFlags |= INHIBIT_PERSISTENT_CACHING;

    if (mLoadFlags & INHIBIT_PERSISTENT_CACHING) {
        rv = mCacheEntry->SetStoragePolicy(nsICache::STORE_IN_MEMORY);
        if (NS_FAILED(rv)) return rv;
    }

    
    rv = UpdateExpirationTime();
    if (NS_FAILED(rv)) return rv;

    rv = AddCacheEntryHeaders(mCacheEntry);
    if (NS_FAILED(rv)) return rv;

    mInitedCacheEntry = PR_TRUE;
    return NS_OK;
}


nsresult
nsHttpChannel::InitOfflineCacheEntry()
{
    

    if (!mOfflineCacheEntry) {
        return NS_OK;
    }

    if (mResponseHead && mResponseHead->NoStore()) {
        CloseOfflineCacheEntry();

        return NS_OK;
    }

    
    
    
    if (mCacheEntry) {
        PRUint32 expirationTime;
        nsresult rv = mCacheEntry->GetExpirationTime(&expirationTime);
        NS_ENSURE_SUCCESS(rv, rv);

        mOfflineCacheEntry->SetExpirationTime(expirationTime);
    }

    return AddCacheEntryHeaders(mOfflineCacheEntry);
}


nsresult
nsHttpChannel::AddCacheEntryHeaders(nsICacheEntryDescriptor *entry)
{
    nsresult rv;

    LOG(("nsHttpChannel::AddCacheEntryHeaders [this=%x] begin", this));
    
    if (mSecurityInfo)
        entry->SetSecurityInfo(mSecurityInfo);

    
    
    rv = entry->SetMetaDataElement("request-method",
                                   mRequestHead.Method().get());
    if (NS_FAILED(rv)) return rv;

    
    rv = StoreAuthorizationMetaData(entry);
    if (NS_FAILED(rv)) return rv;

    
    
    
    
    
    
    
    
    
    
    
    {
        nsCAutoString buf, metaKey;
        mResponseHead->GetHeader(nsHttp::Vary, buf);
        if (!buf.IsEmpty()) {
            NS_NAMED_LITERAL_CSTRING(prefix, "request-");
           
            char *val = buf.BeginWriting(); 
            char *token = nsCRT::strtok(val, NS_HTTP_HEADER_SEPS, &val);
            while (token) {
                LOG(("nsHttpChannel::AddCacheEntryHeaders [this=%x] " \
                        "processing %s", this, token));
                if (*token != '*') {
                    nsHttpAtom atom = nsHttp::ResolveAtom(token);
                    const char *val = mRequestHead.PeekHeader(atom);
                    nsCAutoString hash;
                    if (val) {
                        
                        if (atom == nsHttp::Cookie) {
                            LOG(("nsHttpChannel::AddCacheEntryHeaders [this=%x] " \
                                    "cookie-value %s", this, val));
                            rv = Hash(val, hash);
                            
                            
                            if (NS_FAILED(rv))
                                val = "<hash failed>";
                            else
                                val = hash.get();

                            LOG(("   hashed to %s\n", val));
                        }

                        
                        metaKey = prefix + nsDependentCString(token);
                        entry->SetMetaDataElement(metaKey.get(), val);
                    } else {
                        LOG(("nsHttpChannel::AddCacheEntryHeaders [this=%x] " \
                                "clearing metadata for %s", this, token));
                        metaKey = prefix + nsDependentCString(token);
                        entry->SetMetaDataElement(metaKey.get(), nsnull);
                    }
                }
                token = nsCRT::strtok(val, NS_HTTP_HEADER_SEPS, &val);
            }
        }
    }


    
    
    nsCAutoString head;
    mResponseHead->Flatten(head, PR_TRUE);
    rv = entry->SetMetaDataElement("response-head", head.get());

    return rv;
}

inline void
GetAuthType(const char *challenge, nsCString &authType)
{
    const char *p;

    
    if ((p = strchr(challenge, ' ')) != nsnull)
        authType.Assign(challenge, p - challenge);
    else
        authType.Assign(challenge);
}

nsresult
nsHttpChannel::StoreAuthorizationMetaData(nsICacheEntryDescriptor *entry)
{
    
    const char *val = mRequestHead.PeekHeader(nsHttp::Authorization);
    if (!val)
        return NS_OK;

    
    nsCAutoString buf;
    GetAuthType(val, buf);
    return entry->SetMetaDataElement("auth", buf.get());
}





nsresult
nsHttpChannel::FinalizeCacheEntry()
{
    LOG(("nsHttpChannel::FinalizeCacheEntry [this=%p]\n", this));

    if (mResponseHead && mResponseHeadersModified) {
        
        nsresult rv = UpdateExpirationTime();
        if (NS_FAILED(rv)) return rv;
    }
    return NS_OK;
}



nsresult
nsHttpChannel::InstallCacheListener(PRUint32 offset)
{
    nsresult rv;

    LOG(("Preparing to write data into the cache [uri=%s]\n", mSpec.get()));

    NS_ASSERTION(mCacheEntry, "no cache entry");
    NS_ASSERTION(mListener, "no listener");

    nsCOMPtr<nsIOutputStream> out;
    rv = mCacheEntry->OpenOutputStream(offset, getter_AddRefs(out));
    if (NS_FAILED(rv)) return rv;

    
#if 0
    
    rv = mCacheEntry->MarkValid();
    if (NS_FAILED(rv)) return rv;
#endif

    nsCOMPtr<nsIStreamListenerTee> tee =
        do_CreateInstance(kStreamListenerTeeCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsICacheService> serv =
        do_GetService(NS_CACHESERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIEventTarget> cacheIOTarget;
    serv->GetCacheIOTarget(getter_AddRefs(cacheIOTarget));

    nsCacheStoragePolicy policy;
    rv = mCacheEntry->GetStoragePolicy(&policy);

    if (NS_FAILED(rv) || policy == nsICache::STORE_ON_DISK_AS_FILE ||
        !cacheIOTarget) {
        LOG(("nsHttpChannel::InstallCacheListener sync tee %p rv=%x policy=%d "
             "cacheIOTarget=%p", tee.get(), rv, policy, cacheIOTarget.get()));
        rv = tee->Init(mListener, out, nsnull);
    } else {
        LOG(("nsHttpChannel::InstallCacheListener async tee %p", tee.get()));
        rv = tee->InitAsync(mListener, cacheIOTarget, out, nsnull);
    }

    if (NS_FAILED(rv)) return rv;
    mListener = tee;
    return NS_OK;
}

nsresult
nsHttpChannel::InstallOfflineCacheListener()
{
    nsresult rv;

    LOG(("Preparing to write data into the offline cache [uri=%s]\n",
         mSpec.get()));

    NS_ASSERTION(mOfflineCacheEntry, "no offline cache entry");
    NS_ASSERTION(mListener, "no listener");

    nsCOMPtr<nsIOutputStream> out;
    rv = mOfflineCacheEntry->OpenOutputStream(0, getter_AddRefs(out));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIStreamListenerTee> tee =
        do_CreateInstance(kStreamListenerTeeCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = tee->Init(mListener, out, nsnull);
    if (NS_FAILED(rv)) return rv;

    mListener = tee;

    return NS_OK;
}

void
nsHttpChannel::ClearBogusContentEncodingIfNeeded()
{
    
    
    
    
    
    
    if (mResponseHead->HasHeaderValue(nsHttp::Content_Encoding, "gzip") && (
        mResponseHead->ContentType().EqualsLiteral(APPLICATION_GZIP) ||
        mResponseHead->ContentType().EqualsLiteral(APPLICATION_GZIP2) ||
        mResponseHead->ContentType().EqualsLiteral(APPLICATION_GZIP3))) {
        
        mResponseHead->ClearHeader(nsHttp::Content_Encoding);
    }
    else if (mResponseHead->HasHeaderValue(nsHttp::Content_Encoding, "compress") && (
             mResponseHead->ContentType().EqualsLiteral(APPLICATION_COMPRESS) ||
             mResponseHead->ContentType().EqualsLiteral(APPLICATION_COMPRESS2))) {
        
        mResponseHead->ClearHeader(nsHttp::Content_Encoding);
    }
}





nsresult
nsHttpChannel::SetupReplacementChannel(nsIURI       *newURI, 
                                       nsIChannel   *newChannel,
                                       PRBool        preserveMethod)
{
    LOG(("nsHttpChannel::SetupReplacementChannel "
         "[this=%p newChannel=%p preserveMethod=%d]",
         this, newChannel, preserveMethod));

    nsresult rv = HttpBaseChannel::SetupReplacementChannel(newURI, newChannel, preserveMethod);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(newChannel);
    if (!httpChannel)
        return NS_OK; 

    
    nsCOMPtr<nsIEncodedChannel> encodedChannel = do_QueryInterface(httpChannel);
    if (encodedChannel)
        encodedChannel->SetApplyConversion(mApplyConversion);

    
    if (mResuming) {
        nsCOMPtr<nsIResumableChannel> resumableChannel(do_QueryInterface(newChannel));
        if (!resumableChannel) {
            NS_WARNING("Got asked to resume, but redirected to non-resumable channel!");
            return NS_ERROR_NOT_RESUMABLE;
        }
        resumableChannel->ResumeAt(mStartPos, mEntityID);
    }

    return NS_OK;
}

nsresult
nsHttpChannel::AsyncProcessRedirection(PRUint32 redirectType)
{
    LOG(("nsHttpChannel::AsyncProcessRedirection [this=%p type=%u]\n",
        this, redirectType));

    const char *location = mResponseHead->PeekHeader(nsHttp::Location);

    
    
    if (!location)
        return NS_ERROR_FAILURE;

    
    nsCAutoString locationBuf;
    if (NS_EscapeURL(location, -1, esc_OnlyNonASCII, locationBuf))
        location = locationBuf.get();

    if (mRedirectionLimit == 0) {
        LOG(("redirection limit reached!\n"));
        
        Cancel(NS_ERROR_REDIRECT_LOOP);
        return NS_ERROR_REDIRECT_LOOP;
    }

    mRedirectType = redirectType;

    LOG(("redirecting to: %s [redirection-limit=%u]\n",
        location, PRUint32(mRedirectionLimit)));

    nsresult rv;

    
    
    nsCOMPtr<nsIIOService> ioService;
    rv = gHttpHandler->GetIOService(getter_AddRefs(ioService));
    if (NS_FAILED(rv)) return rv;

    
    nsCAutoString originCharset;
    rv = mURI->GetOriginCharset(originCharset);
    if (NS_FAILED(rv))
        originCharset.Truncate();

    rv = ioService->NewURI(nsDependentCString(location),
                           originCharset.get(),
                           mURI,
                           getter_AddRefs(mRedirectURI));
    if (NS_FAILED(rv)) return rv;

    if (mApplicationCache) {
        
        
        
        if (!NS_SecurityCompareURIs(mURI, mRedirectURI, PR_FALSE)) {
            PushRedirectAsyncFunc(&nsHttpChannel::ContinueProcessRedirectionAfterFallback);
            PRBool waitingForRedirectCallback;
            (void)ProcessFallback(&waitingForRedirectCallback);
            if (waitingForRedirectCallback)
                return NS_OK;
            PopRedirectAsyncFunc(&nsHttpChannel::ContinueProcessRedirectionAfterFallback);
        }
    }

    return ContinueProcessRedirectionAfterFallback(NS_OK);
}

nsresult
nsHttpChannel::ContinueProcessRedirectionAfterFallback(nsresult rv)
{
    if (NS_SUCCEEDED(rv) && mFallingBack) {
        
        
        return NS_OK;
    }

    
    
    PRBool redirectingBackToSameURI = PR_FALSE;
    if (mCacheEntry && (mCacheAccess & nsICache::ACCESS_WRITE) &&
        NS_SUCCEEDED(mURI->Equals(mRedirectURI, &redirectingBackToSameURI)) &&
        redirectingBackToSameURI)
            mCacheEntry->Doom();

    
    
    nsCOMPtr<nsIURL> newURL = do_QueryInterface(mRedirectURI);
    if (newURL) {
        nsCAutoString ref;
        rv = newURL->GetRef(ref);
        if (NS_SUCCEEDED(rv) && ref.IsEmpty()) {
            nsCOMPtr<nsIURL> baseURL(do_QueryInterface(mURI));
            if (baseURL) {
                baseURL->GetRef(ref);
                if (!ref.IsEmpty())
                    newURL->SetRef(ref);
            }
        }
    }

    
    PRBool preserveMethod = (mRedirectType == 307);
    if (preserveMethod && mUploadStream) {
        rv = PromptTempRedirect();
        if (NS_FAILED(rv)) return rv;
    }

    nsCOMPtr<nsIIOService> ioService;
    rv = gHttpHandler->GetIOService(getter_AddRefs(ioService));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIChannel> newChannel;
    rv = ioService->NewChannelFromURI(mRedirectURI, getter_AddRefs(newChannel));
    if (NS_FAILED(rv)) return rv;

    rv = SetupReplacementChannel(mRedirectURI, newChannel, preserveMethod);
    if (NS_FAILED(rv)) return rv;

    PRUint32 redirectFlags;
    if (mRedirectType == 301) 
        redirectFlags = nsIChannelEventSink::REDIRECT_PERMANENT;
    else
        redirectFlags = nsIChannelEventSink::REDIRECT_TEMPORARY;

    
    mRedirectChannel = newChannel;

    PushRedirectAsyncFunc(&nsHttpChannel::ContinueProcessRedirection);
    rv = gHttpHandler->AsyncOnChannelRedirect(this, newChannel, redirectFlags);

    if (NS_SUCCEEDED(rv))
        rv = WaitForRedirectCallback();

    if (NS_FAILED(rv)) {
        AutoRedirectVetoNotifier notifier(this);
        PopRedirectAsyncFunc(&nsHttpChannel::ContinueProcessRedirection);
    }

    return rv;
}

nsresult
nsHttpChannel::ContinueProcessRedirection(nsresult rv)
{
    AutoRedirectVetoNotifier notifier(this);

    LOG(("ContinueProcessRedirection [rv=%x]\n", rv));
    if (NS_FAILED(rv))
        return rv;

    NS_PRECONDITION(mRedirectChannel, "No redirect channel?");

    
    mRedirectChannel->SetOriginalURI(mOriginalURI);

    
    nsCOMPtr<nsIHttpEventSink> httpEventSink;
    GetCallback(httpEventSink);
    if (httpEventSink) {
        
        
        rv = httpEventSink->OnRedirect(this, mRedirectChannel);
        if (NS_FAILED(rv))
            return rv;
    }
    
    

    
    rv = mRedirectChannel->AsyncOpen(mListener, mListenerContext);

    if (NS_FAILED(rv))
        return rv;

    
    Cancel(NS_BINDING_REDIRECTED);
    
    notifier.RedirectSucceeded();

    
    mListener = 0;
    mListenerContext = 0;

    
    mCallbacks = nsnull;
    mProgressSink = nsnull;
    return NS_OK;
}





NS_IMETHODIMP nsHttpChannel::OnAuthAvailable()
{
    LOG(("nsHttpChannel::OnAuthAvailable [this=%p]", this));

    
    
    
    mAuthRetryPending = PR_TRUE;
    LOG(("Resuming the transaction, we got credentials from user"));
    mTransactionPump->Resume();
  
    return NS_OK;
}

NS_IMETHODIMP nsHttpChannel::OnAuthCancelled(PRBool userCancel)
{
    LOG(("nsHttpChannel::OnAuthCancelled [this=%p]", this));

    if (mTransactionPump) {
        
        
        nsresult rv = CallOnStartRequest();

        
        
        mAuthRetryPending = PR_FALSE;
        LOG(("Resuming the transaction, user cancelled the auth dialog"));
        mTransactionPump->Resume();

        if (NS_FAILED(rv))
            mTransactionPump->Cancel(rv);
    }
    
    return NS_OK;
}





NS_IMPL_ADDREF_INHERITED(nsHttpChannel, HttpBaseChannel)
NS_IMPL_RELEASE_INHERITED(nsHttpChannel, HttpBaseChannel)

NS_INTERFACE_MAP_BEGIN(nsHttpChannel)
    NS_INTERFACE_MAP_ENTRY(nsIRequest)
    NS_INTERFACE_MAP_ENTRY(nsIChannel)
    NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
    NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
    NS_INTERFACE_MAP_ENTRY(nsIHttpChannel)
    NS_INTERFACE_MAP_ENTRY(nsICacheInfoChannel)
    NS_INTERFACE_MAP_ENTRY(nsICachingChannel)
    NS_INTERFACE_MAP_ENTRY(nsIUploadChannel)
    NS_INTERFACE_MAP_ENTRY(nsIUploadChannel2)
    NS_INTERFACE_MAP_ENTRY(nsICacheListener)
    NS_INTERFACE_MAP_ENTRY(nsIHttpChannelInternal)
    NS_INTERFACE_MAP_ENTRY(nsIResumableChannel)
    NS_INTERFACE_MAP_ENTRY(nsITransportEventSink)
    NS_INTERFACE_MAP_ENTRY(nsISupportsPriority)
    NS_INTERFACE_MAP_ENTRY(nsIProtocolProxyCallback)
    NS_INTERFACE_MAP_ENTRY(nsIProxiedChannel)
    NS_INTERFACE_MAP_ENTRY(nsIHttpAuthenticableChannel)
    NS_INTERFACE_MAP_ENTRY(nsITraceableChannel)
    NS_INTERFACE_MAP_ENTRY(nsIApplicationCacheContainer)
    NS_INTERFACE_MAP_ENTRY(nsIApplicationCacheChannel)
    NS_INTERFACE_MAP_ENTRY(nsIAsyncVerifyRedirectCallback)
NS_INTERFACE_MAP_END_INHERITING(HttpBaseChannel)





NS_IMETHODIMP
nsHttpChannel::Cancel(nsresult status)
{
    LOG(("nsHttpChannel::Cancel [this=%p status=%x]\n", this, status));
    if (mCanceled) {
        LOG(("  ignoring; already canceled\n"));
        return NS_OK;
    }
    if (mWaitingForRedirectCallback) {
        LOG(("channel canceled during wait for redirect callback"));
    }
    mCanceled = PR_TRUE;
    mStatus = status;
    if (mProxyRequest)
        mProxyRequest->Cancel(status);
    if (mTransaction)
        gHttpHandler->CancelTransaction(mTransaction, status);
    if (mTransactionPump)
        mTransactionPump->Cancel(status);
    if (mCachePump)
        mCachePump->Cancel(status);
    if (mAuthProvider)
        mAuthProvider->Cancel(status);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::Suspend()
{
    NS_ENSURE_TRUE(mIsPending, NS_ERROR_NOT_AVAILABLE);
    
    LOG(("nsHttpChannel::Suspend [this=%p]\n", this));

    ++mSuspendCount;

    if (mTransactionPump)
        return mTransactionPump->Suspend();
    if (mCachePump)
        return mCachePump->Suspend();

    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::Resume()
{
    NS_ENSURE_TRUE(mSuspendCount > 0, NS_ERROR_UNEXPECTED);
    
    LOG(("nsHttpChannel::Resume [this=%p]\n", this));
        
    if (--mSuspendCount == 0 && mPendingAsyncCallOnResume) {
        nsresult rv = AsyncCall(mPendingAsyncCallOnResume);
        mPendingAsyncCallOnResume = nsnull;
        NS_ENSURE_SUCCESS(rv, rv);
    }

    if (mTransactionPump)
        return mTransactionPump->Resume();
    if (mCachePump)
        return mCachePump->Resume();

    return NS_OK;
}





NS_IMETHODIMP
nsHttpChannel::GetSecurityInfo(nsISupports **securityInfo)
{
    NS_ENSURE_ARG_POINTER(securityInfo);
    *securityInfo = mSecurityInfo;
    NS_IF_ADDREF(*securityInfo);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::AsyncOpen(nsIStreamListener *listener, nsISupports *context)
{
    LOG(("nsHttpChannel::AsyncOpen [this=%p]\n", this));

    NS_ENSURE_ARG_POINTER(listener);
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);
    NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);

    nsresult rv;

    if (mCanceled)
        return mStatus;

    rv = NS_CheckPortSafety(mURI);
    if (NS_FAILED(rv))
        return rv;

    if (!(mConnectionInfo && mConnectionInfo->UsingHttpProxy())) {
        
        
        
        nsRefPtr<nsDNSPrefetch> prefetch = new nsDNSPrefetch(mURI);
        if (prefetch) {
            prefetch->PrefetchHigh();
        }
    }
    
    
    const char *cookieHeader = mRequestHead.PeekHeader(nsHttp::Cookie);
    if (cookieHeader) {
        mUserSetCookieHeader = cookieHeader;
    }

    AddCookiesToRequest();

    
    gHttpHandler->OnModifyRequest(this);

    
    
    
    if (mRequestHead.HasHeaderValue(nsHttp::Connection, "close"))
        mCaps &= ~(NS_HTTP_ALLOW_KEEPALIVE | NS_HTTP_ALLOW_PIPELINING);
    
    if ((mLoadFlags & VALIDATE_ALWAYS) || 
        (BYPASS_LOCAL_CACHE(mLoadFlags)))
        mCaps |= NS_HTTP_REFRESH_DNS;

    mIsPending = PR_TRUE;
    mWasOpened = PR_TRUE;

    mListener = listener;
    mListenerContext = context;

    
    
    if (mLoadGroup)
        mLoadGroup->AddRequest(this, nsnull);

    
    
    
    if (mCanceled)
        rv = mStatus;
    else
        rv = Connect();
    if (NS_FAILED(rv)) {
        LOG(("Calling AsyncAbort [rv=%x mCanceled=%i]\n", rv, mCanceled));
        CloseCacheEntry(PR_TRUE);
        AsyncAbort(rv);
    } else if (mLoadFlags & LOAD_CLASSIFY_URI) {
        nsRefPtr<nsChannelClassifier> classifier = new nsChannelClassifier();
        if (!classifier) {
            Cancel(NS_ERROR_OUT_OF_MEMORY);
            return NS_OK;
        }

        rv = classifier->Start(this);
        if (NS_FAILED(rv)) {
            Cancel(rv);
        }
    }

    return NS_OK;
}





NS_IMETHODIMP
nsHttpChannel::SetupFallbackChannel(const char *aFallbackKey)
{
    LOG(("nsHttpChannel::SetupFallbackChannel [this=%x, key=%s]",
         this, aFallbackKey));
    mFallbackChannel = PR_TRUE;
    mFallbackKey = aFallbackKey;

    return NS_OK;
}





NS_IMETHODIMP
nsHttpChannel::SetPriority(PRInt32 value)
{
    PRInt16 newValue = NS_CLAMP(value, PR_INT16_MIN, PR_INT16_MAX);
    if (mPriority == newValue)
        return NS_OK;
    mPriority = newValue;
    if (mTransaction)
        gHttpHandler->RescheduleTransaction(mTransaction, mPriority);
    return NS_OK;
}





NS_IMETHODIMP
nsHttpChannel::OnProxyAvailable(nsICancelable *request, nsIURI *uri,
                                nsIProxyInfo *pi, nsresult status)
{
    mProxyRequest = nsnull;

    
    
    
    
    
    
    
    
    mTargetProxyInfo = pi;
    HandleAsyncReplaceWithProxy();
    return NS_OK;
}





NS_IMETHODIMP
nsHttpChannel::GetProxyInfo(nsIProxyInfo **result)
{
    if (!mConnectionInfo)
        *result = nsnull;
    else {
        *result = mConnectionInfo->ProxyInfo();
        NS_IF_ADDREF(*result);
    }
    return NS_OK;
}





NS_IMETHODIMP
nsHttpChannel::GetIsSSL(PRBool *aIsSSL)
{
    *aIsSSL = mConnectionInfo->UsingSSL();
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetProxyMethodIsConnect(PRBool *aProxyMethodIsConnect)
{
    *aProxyMethodIsConnect =
        (mConnectionInfo->UsingHttpProxy() && mConnectionInfo->UsingSSL());
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetServerResponseHeader(nsACString &value)
{
    if (!mResponseHead)
        return NS_ERROR_NOT_AVAILABLE;
    return mResponseHead->GetHeader(nsHttp::Server, value);
}

NS_IMETHODIMP
nsHttpChannel::GetProxyChallenges(nsACString &value)
{
    if (!mResponseHead)
        return NS_ERROR_UNEXPECTED;
    return mResponseHead->GetHeader(nsHttp::Proxy_Authenticate, value);
}

NS_IMETHODIMP
nsHttpChannel::GetWWWChallenges(nsACString &value)
{
    if (!mResponseHead)
        return NS_ERROR_UNEXPECTED;
    return mResponseHead->GetHeader(nsHttp::WWW_Authenticate, value);
}

NS_IMETHODIMP
nsHttpChannel::SetProxyCredentials(const nsACString &value)
{
    return mRequestHead.SetHeader(nsHttp::Proxy_Authorization, value);
}

NS_IMETHODIMP
nsHttpChannel::SetWWWCredentials(const nsACString &value)
{
    return mRequestHead.SetHeader(nsHttp::Authorization, value);
}






NS_IMETHODIMP
nsHttpChannel::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
    return HttpBaseChannel::GetLoadFlags(aLoadFlags);
}

NS_IMETHODIMP
nsHttpChannel::GetURI(nsIURI **aURI)
{
    return HttpBaseChannel::GetURI(aURI);
}

NS_IMETHODIMP
nsHttpChannel::GetNotificationCallbacks(nsIInterfaceRequestor **aCallbacks)
{
    return HttpBaseChannel::GetNotificationCallbacks(aCallbacks);
}

NS_IMETHODIMP
nsHttpChannel::GetLoadGroup(nsILoadGroup **aLoadGroup)
{
    return HttpBaseChannel::GetLoadGroup(aLoadGroup);
}

NS_IMETHODIMP
nsHttpChannel::GetRequestMethod(nsACString& aMethod)
{
    return HttpBaseChannel::GetRequestMethod(aMethod);
}






NS_IMETHODIMP
nsHttpChannel::OnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
    if (!(mCanceled || NS_FAILED(mStatus))) {
        
        
        request->GetStatus(&mStatus);
    }

    LOG(("nsHttpChannel::OnStartRequest [this=%p request=%p status=%x]\n",
        this, request, mStatus));

    
    NS_ASSERTION(request == mCachePump || request == mTransactionPump,
                 "Unexpected request");
    NS_ASSERTION(!(mTransactionPump && mCachePump) || mCachedContentIsPartial,
                 "If we have both pumps, the cache content must be partial");

    if (!mSecurityInfo && !mCachePump && mTransaction) {
        
        
        mSecurityInfo = mTransaction->SecurityInfo();
    }

    
    if (NS_SUCCEEDED(mStatus) && !mCachePump && mTransaction) {
        
        
        mResponseHead = mTransaction->TakeResponseHead();
        
        
        if (mResponseHead)
            return ProcessResponse();

        NS_WARNING("No response head in OnStartRequest");
    }

    
    if (!mListener) {
        NS_NOTREACHED("mListener is null");
        return NS_OK;
    }

    
    if (mConnectionInfo->ProxyInfo() &&
       (mStatus == NS_ERROR_PROXY_CONNECTION_REFUSED ||
        mStatus == NS_ERROR_UNKNOWN_PROXY_HOST ||
        mStatus == NS_ERROR_NET_TIMEOUT)) {

        PushRedirectAsyncFunc(&nsHttpChannel::ContinueOnStartRequest1);
        if (NS_SUCCEEDED(ProxyFailover()))
            return NS_OK;
        PopRedirectAsyncFunc(&nsHttpChannel::ContinueOnStartRequest1);
    }

    return ContinueOnStartRequest2(NS_OK);
}

nsresult
nsHttpChannel::ContinueOnStartRequest1(nsresult result)
{
    
    
    if (NS_SUCCEEDED(result))
        return NS_OK;

    return ContinueOnStartRequest2(result);
}

nsresult
nsHttpChannel::ContinueOnStartRequest2(nsresult result)
{
    
    if (NS_FAILED(mStatus)) {
        PushRedirectAsyncFunc(&nsHttpChannel::ContinueOnStartRequest3);
        PRBool waitingForRedirectCallback;
        ProcessFallback(&waitingForRedirectCallback);
        if (waitingForRedirectCallback)
            return NS_OK;
        PopRedirectAsyncFunc(&nsHttpChannel::ContinueOnStartRequest3);
    }

    return ContinueOnStartRequest3(NS_OK);
}

nsresult
nsHttpChannel::ContinueOnStartRequest3(nsresult result)
{
    if (mFallingBack)
        return NS_OK;

    return CallOnStartRequest();
}

NS_IMETHODIMP
nsHttpChannel::OnStopRequest(nsIRequest *request, nsISupports *ctxt, nsresult status)
{
    LOG(("nsHttpChannel::OnStopRequest [this=%p request=%p status=%x]\n",
        this, request, status));

     
     PRBool contentComplete = NS_SUCCEEDED(status);

    
    if (mCanceled || NS_FAILED(mStatus))
        status = mStatus;

    if (mCachedContentIsPartial) {
        if (NS_SUCCEEDED(status)) {
            
            NS_ASSERTION(request != mTransactionPump,
                "byte-range transaction finished prematurely");

            if (request == mCachePump) {
                PRBool streamDone;
                status = OnDoneReadingPartialCacheEntry(&streamDone);
                if (NS_SUCCEEDED(status) && !streamDone)
                    return status;
                
            }
            else
                NS_NOTREACHED("unexpected request");
        }
        
        if (NS_FAILED(status) && mTransaction)
            gHttpHandler->CancelTransaction(mTransaction, status); 
    }

    if (mTransaction) {
        
        PRBool authRetry = mAuthRetryPending && NS_SUCCEEDED(status);

        
        
        
        
        
        
        
        
        
        
        nsRefPtr<nsAHttpConnection> conn;
        if (authRetry && (mCaps & NS_HTTP_STICKY_CONNECTION)) {
            conn = mTransaction->Connection();
            
            
            
            if (conn && !conn->IsPersistent())
                conn = nsnull;
        }

        
        mTransaction = nsnull;
        mTransactionPump = 0;

        
        if (authRetry) {
            mAuthRetryPending = PR_FALSE;
            status = DoAuthRetry(conn);
            if (NS_SUCCEEDED(status))
                return NS_OK;
        }

        
        
        if (authRetry || (mAuthRetryPending && NS_FAILED(status))) {
            NS_ASSERTION(NS_FAILED(status), "should have a failure code here");
            
            
            mListener->OnStartRequest(this, mListenerContext);
        }

        
        if (mTransactionReplaced)
            return NS_OK;
    }

    mIsPending = PR_FALSE;
    mStatus = status;

    
    if (mCacheEntry && (mCacheAccess & nsICache::ACCESS_WRITE) &&
        mRequestTimeInitialized){
        FinalizeCacheEntry();
    }
    
    if (mListener) {
        LOG(("  calling OnStopRequest\n"));
        mListener->OnStopRequest(this, mListenerContext, status);
        mListener = 0;
        mListenerContext = 0;
    }

    if (mCacheEntry)
        CloseCacheEntry(!contentComplete);

    if (mOfflineCacheEntry)
        CloseOfflineCacheEntry();

    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nsnull, status);

    
    CleanRedirectCacheChainIfNecessary();

    mCallbacks = nsnull;
    mProgressSink = nsnull;
    
    return NS_OK;
}





NS_IMETHODIMP
nsHttpChannel::OnDataAvailable(nsIRequest *request, nsISupports *ctxt,
                               nsIInputStream *input,
                               PRUint32 offset, PRUint32 count)
{
    LOG(("nsHttpChannel::OnDataAvailable [this=%p request=%p offset=%u count=%u]\n",
        this, request, offset, count));

    
    if (mCanceled)
        return mStatus;

    NS_ASSERTION(mResponseHead, "No response head in ODA!!");

    NS_ASSERTION(!(mCachedContentIsPartial && (request == mTransactionPump)),
            "transaction pump not suspended");

    if (mAuthRetryPending || (request == mTransactionPump && mTransactionReplaced)) {
        PRUint32 n;
        return input->ReadSegments(NS_DiscardSegment, nsnull, count, &n);
    }

    if (mListener) {
        
        
        
        
        
        nsresult transportStatus;
        if (request == mCachePump)
            transportStatus = nsITransport::STATUS_READING;
        else
            transportStatus = nsISocketTransport::STATUS_RECEIVING_FROM;

        
        
        
        

        PRUint64 progressMax(PRUint64(mResponseHead->ContentLength()));
        PRUint64 progress = mLogicalOffset + PRUint64(count);
        NS_ASSERTION(progress <= progressMax, "unexpected progress values");

        OnTransportStatus(nsnull, transportStatus, progress, progressMax);

        
        
        
        
        
        
        nsresult rv =  mListener->OnDataAvailable(this,
                                                  mListenerContext,
                                                  input,
                                                  mLogicalOffset,
                                                  count);
        if (NS_SUCCEEDED(rv))
            mLogicalOffset = progress;
        return rv;
    }

    return NS_ERROR_ABORT;
}





NS_IMETHODIMP
nsHttpChannel::OnTransportStatus(nsITransport *trans, nsresult status,
                                 PRUint64 progress, PRUint64 progressMax)
{
    
    if (!mProgressSink)
        GetCallback(mProgressSink);

    
    if (mProgressSink && NS_SUCCEEDED(mStatus) && mIsPending && !(mLoadFlags & LOAD_BACKGROUND)) {
        LOG(("sending status notification [this=%p status=%x progress=%llu/%llu]\n",
            this, status, progress, progressMax));

        nsCAutoString host;
        mURI->GetHost(host);
        mProgressSink->OnStatus(this, nsnull, status,
                                NS_ConvertUTF8toUTF16(host).get());

        if (progress > 0) {
            NS_ASSERTION(progress <= progressMax, "unexpected progress values");
            mProgressSink->OnProgress(this, nsnull, progress, progressMax);
        }
    }
#ifdef DEBUG
    else
        LOG(("skipping status notification [this=%p sink=%p pending=%u background=%x]\n",
            this, mProgressSink.get(), mIsPending, (mLoadFlags & LOAD_BACKGROUND)));
#endif

    return NS_OK;
} 





NS_IMETHODIMP
nsHttpChannel::IsFromCache(PRBool *value)
{
    if (!mIsPending)
        return NS_ERROR_NOT_AVAILABLE;

    
    

    *value = (mCachePump || (mLoadFlags & LOAD_ONLY_IF_MODIFIED)) &&
              mCachedContentIsValid && !mCachedContentIsPartial;

    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetCacheTokenExpirationTime(PRUint32 *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    if (!mCacheEntry)
        return NS_ERROR_NOT_AVAILABLE;

    return mCacheEntry->GetExpirationTime(_retval);
}

NS_IMETHODIMP
nsHttpChannel::GetCacheTokenCachedCharset(nsACString &_retval)
{
    nsresult rv;

    if (!mCacheEntry)
        return NS_ERROR_NOT_AVAILABLE;

    nsXPIDLCString cachedCharset;
    rv = mCacheEntry->GetMetaDataElement("charset",
                                         getter_Copies(cachedCharset));
    if (NS_SUCCEEDED(rv))
        _retval = cachedCharset;

    return rv;
}

NS_IMETHODIMP
nsHttpChannel::SetCacheTokenCachedCharset(const nsACString &aCharset)
{
    if (!mCacheEntry)
        return NS_ERROR_NOT_AVAILABLE;

    return mCacheEntry->SetMetaDataElement("charset",
                                           PromiseFlatCString(aCharset).get());
}





NS_IMETHODIMP
nsHttpChannel::GetCacheToken(nsISupports **token)
{
    NS_ENSURE_ARG_POINTER(token);
    if (!mCacheEntry)
        return NS_ERROR_NOT_AVAILABLE;
    return CallQueryInterface(mCacheEntry, token);
}

NS_IMETHODIMP
nsHttpChannel::SetCacheToken(nsISupports *token)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::GetOfflineCacheToken(nsISupports **token)
{
    NS_ENSURE_ARG_POINTER(token);
    if (!mOfflineCacheEntry)
        return NS_ERROR_NOT_AVAILABLE;
    return CallQueryInterface(mOfflineCacheEntry, token);
}

NS_IMETHODIMP
nsHttpChannel::SetOfflineCacheToken(nsISupports *token)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

class nsHttpChannelCacheKey : public nsISupportsPRUint32,
                              public nsISupportsCString
{
    NS_DECL_ISUPPORTS

    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_FORWARD_NSISUPPORTSPRUINT32(mSupportsPRUint32->)
    
    
    
    NS_SCRIPTABLE NS_IMETHOD GetData(nsACString & aData) 
    { 
        return mSupportsCString->GetData(aData);
    }
    NS_SCRIPTABLE NS_IMETHOD SetData(const nsACString & aData)
    { 
        return mSupportsCString->SetData(aData);
    }
    
public:
    nsresult SetData(PRUint32 aPostID, const nsACString& aKey);

protected:
    nsCOMPtr<nsISupportsPRUint32> mSupportsPRUint32;
    nsCOMPtr<nsISupportsCString> mSupportsCString;
};

NS_IMPL_ADDREF(nsHttpChannelCacheKey)
NS_IMPL_RELEASE(nsHttpChannelCacheKey)
NS_INTERFACE_TABLE_HEAD(nsHttpChannelCacheKey)
NS_INTERFACE_TABLE_BEGIN
NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(nsHttpChannelCacheKey,
                                   nsISupports, nsISupportsPRUint32)
NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(nsHttpChannelCacheKey,
                                   nsISupportsPrimitive, nsISupportsPRUint32)
NS_INTERFACE_TABLE_ENTRY(nsHttpChannelCacheKey,
                         nsISupportsPRUint32)
NS_INTERFACE_TABLE_ENTRY(nsHttpChannelCacheKey,
                         nsISupportsCString)
NS_INTERFACE_TABLE_END
NS_INTERFACE_TABLE_TAIL

NS_IMETHODIMP nsHttpChannelCacheKey::GetType(PRUint16 *aType)
{
    NS_ENSURE_ARG_POINTER(aType);

    *aType = TYPE_PRUINT32;
    return NS_OK;
}

nsresult nsHttpChannelCacheKey::SetData(PRUint32 aPostID,
                                        const nsACString& aKey)
{
    nsresult rv;

    mSupportsCString = 
        do_CreateInstance(NS_SUPPORTS_CSTRING_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    mSupportsCString->SetData(aKey);
    if (NS_FAILED(rv)) return rv;

    mSupportsPRUint32 = 
        do_CreateInstance(NS_SUPPORTS_PRUINT32_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    mSupportsPRUint32->SetData(aPostID);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetCacheKey(nsISupports **key)
{
    nsresult rv;
    NS_ENSURE_ARG_POINTER(key);

    LOG(("nsHttpChannel::GetCacheKey [this=%p]\n", this));

    *key = nsnull;

    nsRefPtr<nsHttpChannelCacheKey> container =
        new nsHttpChannelCacheKey();

    if (!container)
        return NS_ERROR_OUT_OF_MEMORY;

    nsCAutoString cacheKey;
    rv = GenerateCacheKey(mPostID, cacheKey);
    if (NS_FAILED(rv)) return rv;

    rv = container->SetData(mPostID, cacheKey);
    if (NS_FAILED(rv)) return rv;

    return CallQueryInterface(container.get(), key);
}

NS_IMETHODIMP
nsHttpChannel::SetCacheKey(nsISupports *key)
{
    nsresult rv;

    LOG(("nsHttpChannel::SetCacheKey [this=%p key=%p]\n", this, key));

    
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);

    if (!key)
        mPostID = 0;
    else {
        
        nsCOMPtr<nsISupportsPRUint32> container = do_QueryInterface(key, &rv);
        if (NS_FAILED(rv)) return rv;

        rv = container->GetData(&mPostID);
        if (NS_FAILED(rv)) return rv;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetCacheAsFile(PRBool *value)
{
    NS_ENSURE_ARG_POINTER(value);
    if (!mCacheEntry)
        return NS_ERROR_NOT_AVAILABLE;
    nsCacheStoragePolicy storagePolicy;
    mCacheEntry->GetStoragePolicy(&storagePolicy);
    *value = (storagePolicy == nsICache::STORE_ON_DISK_AS_FILE);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetCacheAsFile(PRBool value)
{
    if (!mCacheEntry || mLoadFlags & INHIBIT_PERSISTENT_CACHING)
        return NS_ERROR_NOT_AVAILABLE;
    nsCacheStoragePolicy policy;
    if (value)
        policy = nsICache::STORE_ON_DISK_AS_FILE;
    else
        policy = nsICache::STORE_ANYWHERE;
    return mCacheEntry->SetStoragePolicy(policy);
}


NS_IMETHODIMP
nsHttpChannel::GetCacheForOfflineUse(PRBool *value)
{
    *value = mCacheForOfflineUse;

    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetCacheForOfflineUse(PRBool value)
{
    mCacheForOfflineUse = value;

    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetOfflineCacheClientID(nsACString &value)
{
    value = mOfflineCacheClientID;

    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetOfflineCacheClientID(const nsACString &value)
{
    mOfflineCacheClientID = value;

    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetCacheFile(nsIFile **cacheFile)
{
    if (!mCacheEntry)
        return NS_ERROR_NOT_AVAILABLE;
    return mCacheEntry->GetFile(cacheFile);
}





NS_IMETHODIMP
nsHttpChannel::ResumeAt(PRUint64 aStartPos,
                        const nsACString& aEntityID)
{
    LOG(("nsHttpChannel::ResumeAt [this=%p startPos=%llu id='%s']\n",
         this, aStartPos, PromiseFlatCString(aEntityID).get()));
    mEntityID = aEntityID;
    mStartPos = aStartPos;
    mResuming = PR_TRUE;
    return NS_OK;
}





NS_IMETHODIMP
nsHttpChannel::OnCacheEntryAvailable(nsICacheEntryDescriptor *entry,
                                     nsCacheAccessMode access,
                                     nsresult status)
{
    nsresult rv;

    LOG(("nsHttpChannel::OnCacheEntryAvailable [this=%p entry=%p "
         "access=%x status=%x]\n", this, entry, access, status));

    
    
    if (!mIsPending)
        return NS_OK;

    nsOnCacheEntryAvailableCallback callback = mOnCacheEntryAvailableCallback;
    mOnCacheEntryAvailableCallback = nsnull;

    NS_ASSERTION(callback,
        "nsHttpChannel::OnCacheEntryAvailable called without callback");
    rv = ((*this).*callback)(entry, access, status, PR_FALSE);

    if (NS_FAILED(rv)) {
        LOG(("AsyncOpenCacheEntry failed [rv=%x]\n", rv));
        if (mLoadFlags & LOAD_ONLY_FROM_CACHE) {
            
            
            if (!mFallbackChannel && !mFallbackKey.IsEmpty()) {
                rv = AsyncCall(&nsHttpChannel::HandleAsyncFallback);
                if (NS_SUCCEEDED(rv))
                    return rv;
            }
        }
        CloseCacheEntry(PR_TRUE);
        AsyncAbort(rv);
    }

    return NS_OK;
}

nsresult
nsHttpChannel::DoAuthRetry(nsAHttpConnection *conn)
{
    LOG(("nsHttpChannel::DoAuthRetry [this=%p]\n", this));

    NS_ASSERTION(!mTransaction, "should not have a transaction");
    nsresult rv;

    
    
    mIsPending = PR_FALSE;

    
    
    
    
    AddCookiesToRequest();
    
    
    gHttpHandler->OnModifyRequest(this);

    mIsPending = PR_TRUE;

    
    mResponseHead = nsnull;

    
    mCaps |=  NS_HTTP_STICKY_CONNECTION;
    mCaps &= ~NS_HTTP_ALLOW_PIPELINING;
   
    
    rv = SetupTransaction();
    if (NS_FAILED(rv)) return rv;

    
    if (conn)
        mTransaction->SetConnection(conn);

    
    if (mUploadStream) {
        nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(mUploadStream);
        if (seekable)
            seekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);
    }

    rv = gHttpHandler->InitiateTransaction(mTransaction, mPriority);
    if (NS_FAILED(rv)) return rv;

    rv = mTransactionPump->AsyncRead(this, nsnull);
    if (NS_FAILED(rv)) return rv;

    PRUint32 suspendCount = mSuspendCount;
    while (suspendCount--)
        mTransactionPump->Suspend();

    return NS_OK;
}




NS_IMETHODIMP
nsHttpChannel::GetApplicationCache(nsIApplicationCache **out)
{
    NS_IF_ADDREF(*out = mApplicationCache);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetApplicationCache(nsIApplicationCache *appCache)
{
    NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);

    mApplicationCache = appCache;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetLoadedFromApplicationCache(PRBool *aLoadedFromApplicationCache)
{
    *aLoadedFromApplicationCache = mLoadedFromApplicationCache;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetInheritApplicationCache(PRBool *aInherit)
{
    *aInherit = mInheritApplicationCache;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetInheritApplicationCache(PRBool aInherit)
{
    NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);

    mInheritApplicationCache = aInherit;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetChooseApplicationCache(PRBool *aChoose)
{
    *aChoose = mChooseApplicationCache;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetChooseApplicationCache(PRBool aChoose)
{
    NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);

    mChooseApplicationCache = aChoose;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::MarkOfflineCacheEntryAsForeign()
{
    if (!mApplicationCache)
        return NS_ERROR_NOT_AVAILABLE;

    nsresult rv;

    nsCAutoString cacheKey;
    rv = GenerateCacheKey(mPostID, cacheKey);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mApplicationCache->MarkEntry(cacheKey,
                                      nsIApplicationCache::ITEM_FOREIGN);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}





nsresult
nsHttpChannel::WaitForRedirectCallback()
{
    nsresult rv;
    LOG(("nsHttpChannel::WaitForRedirectCallback [this=%p]\n", this));

    if (mTransactionPump) {
        rv = mTransactionPump->Suspend();
        NS_ENSURE_SUCCESS(rv, rv);
    }
    if (mCachePump) {
        rv = mCachePump->Suspend();
        if (NS_FAILED(rv) && mTransactionPump) {
#ifdef DEBUG
            nsresult resume = 
#endif
            mTransactionPump->Resume();
            NS_ASSERTION(NS_SUCCEEDED(resume),
                "Failed to resume transaction pump");
        }
        NS_ENSURE_SUCCESS(rv, rv);
    }

    mWaitingForRedirectCallback = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::OnRedirectVerifyCallback(nsresult result)
{
    LOG(("nsHttpChannel::OnRedirectVerifyCallback [this=%p] "
         "result=%x stack=%d mWaitingForRedirectCallback=%u\n",
         this, result, mRedirectFuncStack.Length(), mWaitingForRedirectCallback));
    NS_ASSERTION(mWaitingForRedirectCallback,
                 "Someone forgot to call WaitForRedirectCallback() ?!");
    mWaitingForRedirectCallback = PR_FALSE;

    if (mCanceled && NS_SUCCEEDED(result))
        result = NS_BINDING_ABORTED;

    for (PRUint32 i = mRedirectFuncStack.Length(); i > 0;) {
        --i;
        
        nsContinueRedirectionFunc func = mRedirectFuncStack[i];
        mRedirectFuncStack.RemoveElementAt(mRedirectFuncStack.Length() - 1);

        
        
        result = (this->*func)(result);

        
        
        
        if (mWaitingForRedirectCallback)
            break;
    }

    if (NS_FAILED(result) && !mCanceled) {
        
        
        Cancel(result);
    }

    if (!mWaitingForRedirectCallback) {
        
        
        mRedirectChannel = nsnull;
    }

    
    
    
    
    if (mTransactionPump)
        mTransactionPump->Resume();
    if (mCachePump)
        mCachePump->Resume();

    return result;
}

void
nsHttpChannel::PushRedirectAsyncFunc(nsContinueRedirectionFunc func)
{
    mRedirectFuncStack.AppendElement(func);
}

void
nsHttpChannel::PopRedirectAsyncFunc(nsContinueRedirectionFunc func)
{
    NS_ASSERTION(func == mRedirectFuncStack[mRedirectFuncStack.Length() - 1],
        "Trying to pop wrong method from redirect async stack!");

    mRedirectFuncStack.TruncateLength(mRedirectFuncStack.Length() - 1);
}







class nsStreamListenerWrapper : public nsIStreamListener
{
public:
    nsStreamListenerWrapper(nsIStreamListener *listener);

    NS_DECL_ISUPPORTS
    NS_FORWARD_NSIREQUESTOBSERVER(mListener->)
    NS_FORWARD_NSISTREAMLISTENER(mListener->)

private:
    ~nsStreamListenerWrapper() {}
    nsCOMPtr<nsIStreamListener> mListener;
};

nsStreamListenerWrapper::nsStreamListenerWrapper(nsIStreamListener *listener)
    : mListener(listener) 
{
    NS_ASSERTION(mListener, "no stream listener specified");
}

NS_IMPL_ISUPPORTS2(nsStreamListenerWrapper,
                   nsIStreamListener,
                   nsIRequestObserver)





NS_IMETHODIMP
nsHttpChannel::SetNewListener(nsIStreamListener *aListener, nsIStreamListener **_retval)
{
    if (!mTracingEnabled)
        return NS_ERROR_FAILURE;

    NS_ENSURE_ARG_POINTER(aListener);

    nsCOMPtr<nsIStreamListener> wrapper = 
        new nsStreamListenerWrapper(mListener);

    if (!wrapper)
        return NS_ERROR_OUT_OF_MEMORY;

    wrapper.forget(_retval);
    mListener = aListener;
    return NS_OK;
}

void
nsHttpChannel::MaybeInvalidateCacheEntryForSubsequentGet()
{
    
    
    
    
    
    if (mRequestHead.Method() == nsHttp::Options ||
       mRequestHead.Method() == nsHttp::Get ||
       mRequestHead.Method() == nsHttp::Head ||
       mRequestHead.Method() == nsHttp::Trace ||
       mRequestHead.Method() == nsHttp::Connect)
        return;
        
    
    
    
    
    
    LOG(("MaybeInvalidateCacheEntryForSubsequentGet [this=%p]\n", this));

    nsCAutoString tmpCacheKey;
    
    GenerateCacheKey(0, tmpCacheKey);

    
    nsCOMPtr<nsICacheSession> session;
    nsCacheStoragePolicy storagePolicy = DetermineStoragePolicy();

    nsresult rv;
    rv = gHttpHandler->GetCacheSession(storagePolicy,
                                       getter_AddRefs(session));

    if (NS_FAILED(rv)) return;

    
    nsCOMPtr<nsICacheEntryDescriptor> tmpCacheEntry;
    rv = session->OpenCacheEntry(tmpCacheKey, nsICache::ACCESS_READ,
                                 PR_FALSE,
                                 getter_AddRefs(tmpCacheEntry));
    
    
    if(NS_SUCCEEDED(rv)) {
       tmpCacheEntry->SetExpirationTime(0);
    }
}

nsCacheStoragePolicy
nsHttpChannel::DetermineStoragePolicy()
{
    nsCacheStoragePolicy policy = nsICache::STORE_ANYWHERE;
    if (mLoadFlags & INHIBIT_PERSISTENT_CACHING)
        policy = nsICache::STORE_IN_MEMORY;

    return policy;
}

nsresult
nsHttpChannel::DetermineCacheAccess(nsCacheAccessMode *_retval)
{
    PRBool offline = gIOService->IsOffline();

    if (offline || (mLoadFlags & INHIBIT_CACHING)) {
        
        
        
        if (BYPASS_LOCAL_CACHE(mLoadFlags) && !offline)
            return NS_ERROR_NOT_AVAILABLE;
        *_retval = nsICache::ACCESS_READ;
    }
    else if (BYPASS_LOCAL_CACHE(mLoadFlags))
        *_retval = nsICache::ACCESS_WRITE; 
    else
        *_retval = nsICache::ACCESS_READ_WRITE; 

    return NS_OK;
}

void
nsHttpChannel::AsyncOnExamineCachedResponse()
{
    gHttpHandler->OnExamineCachedResponse(this);
}
