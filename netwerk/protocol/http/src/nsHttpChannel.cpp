












































#include "nsHttpChannel.h"
#include "nsHttpTransaction.h"
#include "nsHttpConnection.h"
#include "nsHttpHandler.h"
#include "nsHttpAuthCache.h"
#include "nsHttpResponseHead.h"
#include "nsHttp.h"
#include "nsIHttpAuthenticator.h"
#include "nsIApplicationCacheService.h"
#include "nsIApplicationCacheContainer.h"
#include "nsIAuthInformation.h"
#include "nsIAuthPrompt2.h"
#include "nsIAuthPromptProvider.h"
#include "nsIStringBundle.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIURL.h"
#include "nsIIDNService.h"
#include "nsIStreamListenerTee.h"
#include "nsISeekableStream.h"
#include "nsMimeTypes.h"
#include "nsNetUtil.h"
#include "nsString.h"
#include "nsPrintfCString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsAutoPtr.h"
#include "plstr.h"
#include "prprf.h"
#include "nsEscape.h"
#include "nsICookieService.h"
#include "nsIResumableChannel.h"
#include "nsInt64.h"
#include "nsIVariant.h"
#include "nsChannelProperties.h"
#include "nsStreamUtils.h"
#include "nsIOService.h"
#include "nsAuthInformationHolder.h"
#include "nsICacheService.h"
#include "nsDNSPrefetch.h"
#include "nsNetSegmentUtils.h"


#define BYPASS_LOCAL_CACHE(loadFlags) \
        (loadFlags & (nsIRequest::LOAD_BYPASS_CACHE | \
                      nsICachingChannel::LOAD_BYPASS_LOCAL_CACHE))

static NS_DEFINE_CID(kStreamListenerTeeCID, NS_STREAMLISTENERTEE_CID);





nsHttpChannel::nsHttpChannel()
    : mResponseHead(nsnull)
    , mTransaction(nsnull)
    , mConnectionInfo(nsnull)
    , mLoadFlags(LOAD_NORMAL)
    , mStatus(NS_OK)
    , mLogicalOffset(0)
    , mCaps(0)
    , mPriority(PRIORITY_NORMAL)
    , mCachedResponseHead(nsnull)
    , mCacheAccess(0)
    , mPostID(0)
    , mRequestTime(0)
    , mProxyAuthContinuationState(nsnull)
    , mAuthContinuationState(nsnull)
    , mStartPos(LL_MAXUINT)
    , mPendingAsyncCallOnResume(nsnull)
    , mSuspendCount(0)
    , mRedirectionLimit(gHttpHandler->RedirectionLimit())
    , mIsPending(PR_FALSE)
    , mWasOpened(PR_FALSE)
    , mApplyConversion(PR_TRUE)
    , mAllowPipelining(PR_TRUE)
    , mCachedContentIsValid(PR_FALSE)
    , mCachedContentIsPartial(PR_FALSE)
    , mResponseHeadersModified(PR_FALSE)
    , mCanceled(PR_FALSE)
    , mTransactionReplaced(PR_FALSE)
    , mUploadStreamHasHeaders(PR_FALSE)
    , mAuthRetryPending(PR_FALSE)
    , mProxyAuth(PR_FALSE)
    , mSuppressDefensiveAuth(PR_FALSE)
    , mResuming(PR_FALSE)
    , mInitedCacheEntry(PR_FALSE)
    , mCacheForOfflineUse(PR_FALSE)
    , mCachingOpportunistically(PR_FALSE)
    , mFallbackChannel(PR_FALSE)
    , mInheritApplicationCache(PR_TRUE)
    , mChooseApplicationCache(PR_FALSE)
    , mLoadedFromApplicationCache(PR_FALSE)
    , mTracingEnabled(PR_TRUE)
    , mForceAllowThirdPartyCookie(PR_FALSE)
{
    LOG(("Creating nsHttpChannel @%x\n", this));

    
    nsHttpHandler *handler = gHttpHandler;
    NS_ADDREF(handler);
}

nsHttpChannel::~nsHttpChannel()
{
    LOG(("Destroying nsHttpChannel @%x\n", this));

    NS_IF_RELEASE(mConnectionInfo);
    NS_IF_RELEASE(mTransaction);

    NS_IF_RELEASE(mProxyAuthContinuationState);
    NS_IF_RELEASE(mAuthContinuationState);

    delete mResponseHead;
    delete mCachedResponseHead;

    
    nsHttpHandler *handler = gHttpHandler;
    NS_RELEASE(handler);
}

nsresult
nsHttpChannel::Init(nsIURI *uri,
                    PRUint8 caps,
                    nsProxyInfo *proxyInfo)
{
    LOG(("nsHttpChannel::Init [this=%x]\n", this));

    NS_PRECONDITION(uri, "null uri");

    nsresult rv = nsHashPropertyBag::Init();
    if (NS_FAILED(rv))
        return rv;

    mURI = uri;
    mOriginalURI = uri;
    mDocumentURI = nsnull;
    mCaps = caps;

    
    
    
    nsCAutoString host;
    PRInt32 port = -1;
    PRBool usingSSL = PR_FALSE;
    
    rv = mURI->SchemeIs("https", &usingSSL);
    if (NS_FAILED(rv)) return rv;

    rv = mURI->GetAsciiHost(host);
    if (NS_FAILED(rv)) return rv;

    
    if (host.IsEmpty())
        return NS_ERROR_MALFORMED_URI;

    rv = mURI->GetPort(&port);
    if (NS_FAILED(rv)) return rv;

    LOG(("host=%s port=%d\n", host.get(), port));

    rv = mURI->GetAsciiSpec(mSpec);
    if (NS_FAILED(rv)) return rv;

    LOG(("uri=%s\n", mSpec.get()));

    mConnectionInfo = new nsHttpConnectionInfo(host, port,
                                               proxyInfo, usingSSL);
    if (!mConnectionInfo)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(mConnectionInfo);

    
    mRequestHead.SetMethod(nsHttp::Get);

    
    
    
    nsCAutoString hostLine;
    if (strchr(host.get(), ':')) {
        
        hostLine.Assign('[');
        
        int scopeIdPos = host.FindChar('%');
        if (scopeIdPos == kNotFound)
            hostLine.Append(host);
        else if (scopeIdPos > 0)
            hostLine.Append(Substring(host, 0, scopeIdPos));
        else
          return NS_ERROR_MALFORMED_URI;
        hostLine.Append(']');
    }
    else
        hostLine.Assign(host);
    if (port != -1) {
        hostLine.Append(':');
        hostLine.AppendInt(port);
    }

    rv = mRequestHead.SetHeader(nsHttp::Host, hostLine);
    if (NS_FAILED(rv)) return rv;

    rv = gHttpHandler->
        AddStandardRequestHeaders(&mRequestHead.Headers(), caps,
                                  !mConnectionInfo->UsingSSL() &&
                                  mConnectionInfo->UsingHttpProxy());

    return rv;
}





nsresult
nsHttpChannel::AsyncCall(nsAsyncCallback funcPtr,
                         nsRunnableMethod<nsHttpChannel> **retval)
{
    nsresult rv;

    nsRefPtr<nsRunnableMethod<nsHttpChannel> > event =
            new nsRunnableMethod<nsHttpChannel>(this, funcPtr);
    rv = NS_DispatchToCurrentThread(event);
    if (NS_SUCCEEDED(rv) && retval) {
        *retval = event;
    }

    return rv;
}

PRBool
nsHttpChannel::RequestIsConditional()
{
    
    return mRequestHead.PeekHeader(nsHttp::If_Modified_Since) ||
           mRequestHead.PeekHeader(nsHttp::If_None_Match) ||
           mRequestHead.PeekHeader(nsHttp::If_Unmodified_Since) ||
           mRequestHead.PeekHeader(nsHttp::If_Match) ||
           mRequestHead.PeekHeader(nsHttp::If_Range);
}

nsresult
nsHttpChannel::Connect(PRBool firstTime)
{
    nsresult rv;

    LOG(("nsHttpChannel::Connect [this=%x]\n", this));

    
    if (!net_IsValidHostName(nsDependentCString(mConnectionInfo->Host())))
        return NS_ERROR_UNKNOWN_HOST;

    
    if (firstTime) {
        PRBool delayed = PR_FALSE;

        
        PRBool offline = gIOService->IsOffline();
        if (offline)
            mLoadFlags |= LOAD_ONLY_FROM_CACHE;
        else if (PL_strcmp(mConnectionInfo->ProxyType(), "unknown") == 0)
            return ResolveProxy();  

        
        if (mResuming && (mLoadFlags & LOAD_ONLY_FROM_CACHE)) {
            LOG(("Resuming from cache is not supported yet"));
            return NS_ERROR_DOCUMENT_NOT_CACHED;
        }

        
        rv = OpenCacheEntry(offline, &delayed);

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

        if (NS_SUCCEEDED(rv) && delayed)
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

    
    AddAuthorizationHeaders();

    if (mLoadFlags & LOAD_NO_NETWORK_IO) {
        return NS_ERROR_DOCUMENT_NOT_CACHED;
    }

    
    rv = SetupTransaction();
    if (NS_FAILED(rv)) return rv;

    rv = gHttpHandler->InitiateTransaction(mTransaction, mPriority);
    if (NS_FAILED(rv)) return rv;

    return mTransactionPump->AsyncRead(this, nsnull);
}


nsresult
nsHttpChannel::AsyncAbort(nsresult status)
{
    LOG(("nsHttpChannel::AsyncAbort [this=%x status=%x]\n", this, status));

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
        mListener->OnStopRequest(this, mListenerContext, mStatus);
        mListener = 0;
        mListenerContext = 0;
    }
    
    mCallbacks = nsnull;
    mProgressSink = nsnull;
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
        rv = ProcessRedirection(mResponseHead->Status());
        if (NS_FAILED(rv)) {
            
            
            LOG(("ProcessRedirection failed [rv=%x]\n", rv));
            mStatus = rv;
            DoNotifyListener();
        }
    }

    
    
    if (mCacheEntry) {
        if (NS_FAILED(rv))
            mCacheEntry->Doom();
        CloseCacheEntry(PR_FALSE);
    }

    mIsPending = PR_FALSE;

    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nsnull, mStatus);
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
        PRBool fallingBack;
        rv = ProcessFallback(&fallingBack);
        if (NS_FAILED(rv) || !fallingBack) {
            
            
            LOG(("ProcessFallback failed [rv=%x, %d]\n", rv, fallingBack));
            mStatus = NS_FAILED(rv) ? rv : NS_ERROR_DOCUMENT_NOT_CACHED;
            DoNotifyListener();
        }
    }

    mIsPending = PR_FALSE;

    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nsnull, mStatus);
}

nsresult
nsHttpChannel::SetupTransaction()
{
    LOG(("nsHttpChannel::SetupTransaction [this=%x]\n", this));

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
    NS_ADDREF(mTransaction);

    
    if (mLoadFlags & LOAD_ANONYMOUS) {
        mCaps |= NS_HTTP_LOAD_ANONYMOUS;
        mConnectionInfo->SetAnonymous();
    }

    nsCOMPtr<nsIAsyncInputStream> responseStream;
    rv = mTransaction->Init(mCaps, mConnectionInfo, &mRequestHead,
                            mUploadStream, mUploadStreamHasHeaders,
                            NS_GetCurrentThread(), callbacks, this,
                            getter_AddRefs(responseStream));
    if (NS_FAILED(rv)) {
        NS_RELEASE(mTransaction);
        return rv;
    }

    rv = nsInputStreamPump::Create(getter_AddRefs(mTransactionPump),
                                   responseStream);
    return rv;
}

void
nsHttpChannel::AddCookiesToRequest()
{
    if (mLoadFlags & LOAD_ANONYMOUS) {
      return;
    }

    nsXPIDLCString cookie;

    nsICookieService *cs = gHttpHandler->GetCookieService();
    if (cs)
        cs->GetCookieStringFromHttp(mURI,
                                    mDocumentURI ? mDocumentURI : mOriginalURI,
                                    this,
                                    getter_Copies(cookie));
    if (cookie.IsEmpty())
        cookie = mUserSetCookieHeader;
    else if (!mUserSetCookieHeader.IsEmpty())
        cookie.Append(NS_LITERAL_CSTRING("; ") + mUserSetCookieHeader);

    
    
    
    mRequestHead.SetHeader(nsHttp::Cookie, cookie, PR_FALSE);
}

nsresult
nsHttpChannel::ApplyContentConversions()
{
    if (!mResponseHead)
        return NS_OK;

    LOG(("nsHttpChannel::ApplyContentConversions [this=%x]\n", this));

    if (!mApplyConversion) {
        LOG(("not applying conversion per mApplyConversion\n"));
        return NS_OK;
    }

    const char *val = mResponseHead->PeekHeader(nsHttp::Content_Encoding);
    if (gHttpHandler->IsAcceptableEncoding(val)) {
        nsCOMPtr<nsIStreamConverterService> serv;
        nsresult rv = gHttpHandler->
                GetStreamConverterService(getter_AddRefs(serv));
        
        
        if (NS_SUCCEEDED(rv)) {
            nsCOMPtr<nsIStreamListener> converter;
            nsCAutoString from(val);
            ToLowerCase(from);
            rv = serv->AsyncConvertData(from.get(),
                                        "uncompressed",
                                        mListener,
                                        mListenerContext,
                                        getter_AddRefs(converter));
            if (NS_SUCCEEDED(rv)) {
                LOG(("converter installed from \'%s\' to \'uncompressed\'\n", val));
                mListener = converter;
            }
        }
    } else if (val != nsnull) {
        LOG(("Unknown content encoding '%s', ignoring\n", val));
    }

    return NS_OK;
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

    if (mResponseHead)
        SetPropertyAsInt64(NS_CHANNEL_PROP_CONTENT_LENGTH,
                           mResponseHead->ContentLength());

    
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
    LOG(("Cancelling failed SSL proxy connection [this=%x httpStatus=%u]\n",
         this, httpStatus)); 
    Cancel(rv);
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
nsHttpChannel::ProcessResponse()
{
    nsresult rv;
    PRUint32 httpStatus = mResponseHead->Status();

    LOG(("nsHttpChannel::ProcessResponse [this=%x httpStatus=%u]\n",
        this, httpStatus));

    if (mTransaction->SSLConnectFailed() &&
        !ShouldSSLProxyResponseContinue(httpStatus))
        return ProcessFailedSSLConnect(httpStatus);

    
    gHttpHandler->OnExamineResponse(this);

    
    
    SetCookie(mResponseHead->PeekHeader(nsHttp::Set_Cookie));

    
    if (httpStatus != 401 && httpStatus != 407) {
        CheckForSuperfluousAuth();
        if (mCanceled)
            return CallOnStartRequest();

        if (mAuthContinuationState) {
            
            
            NS_RELEASE(mAuthContinuationState);
            LOG(("  continuation state has been reset"));
        }
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
        rv = ProcessRedirection(httpStatus);
        if (NS_SUCCEEDED(rv)) {
            InitCacheEntry();
            CloseCacheEntry(PR_FALSE);

            if (mCacheForOfflineUse) {
                
                InitOfflineCacheEntry();
                CloseOfflineCacheEntry();
            }
        }    
        else {
            LOG(("ProcessRedirection failed [rv=%x]\n", rv));
            if (mTransaction->SSLConnectFailed())
                return ProcessFailedSSLConnect(httpStatus);
            rv = ProcessNormal();
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
        rv = ProcessAuthentication(httpStatus);
        if (NS_FAILED(rv)) {
            LOG(("ProcessAuthentication failed [rv=%x]\n", rv));
            if (mTransaction->SSLConnectFailed())
                return ProcessFailedSSLConnect(httpStatus);
            CheckForSuperfluousAuth();
            rv = ProcessNormal();
        }
        break;
    default:
        rv = ProcessNormal();
        MaybeInvalidateCacheEntryForSubsequentGet();
        break;
    }

    return rv;
}

nsresult
nsHttpChannel::ProcessNormal()
{
    nsresult rv;

    LOG(("nsHttpChannel::ProcessNormal [this=%x]\n", this));

    PRBool succeeded;
    rv = GetRequestSucceeded(&succeeded);
    if (NS_SUCCEEDED(rv) && !succeeded) {
        PRBool fallingBack;
        rv = ProcessFallback(&fallingBack);
        if (NS_FAILED(rv)) {
            DoNotifyListener();
            return rv;
        }

        if (fallingBack) {
            
            
            return NS_OK;
        }
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
    LOG(("nsHttpChannel::ProxyFailover [this=%x]\n", this));

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

    
    
    return DoReplaceWithProxy(pi);
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
        status = DoReplaceWithProxy(pi);
        if (mLoadGroup && NS_SUCCEEDED(status)) {
            mLoadGroup->RemoveRequest(this, nsnull, mStatus);
        }
    }

    if (NS_FAILED(status)) {
        AsyncAbort(status);
    }
}

nsresult
nsHttpChannel::DoReplaceWithProxy(nsIProxyInfo* pi)
{
    nsresult rv;

    nsCOMPtr<nsIChannel> newChannel;
    rv = gHttpHandler->NewProxiedChannel(mURI, pi, getter_AddRefs(newChannel));
    if (NS_FAILED(rv))
        return rv;

    rv = SetupReplacementChannel(mURI, newChannel, PR_TRUE);
    if (NS_FAILED(rv))
        return rv;

    
    PRUint32 flags = nsIChannelEventSink::REDIRECT_INTERNAL;
    rv = gHttpHandler->OnChannelRedirect(this, newChannel, flags);
    if (NS_FAILED(rv))
        return rv;

    
    newChannel->SetOriginalURI(mOriginalURI);

    
    rv = newChannel->AsyncOpen(mListener, mListenerContext);
    if (NS_FAILED(rv))
        return rv;

    mStatus = NS_BINDING_REDIRECTED;

    
    mListener = nsnull;
    mListenerContext = nsnull;

    
    mCallbacks = nsnull;
    mProgressSink = nsnull;

    return rv;
}

nsresult
nsHttpChannel::ResolveProxy()
{
    LOG(("nsHttpChannel::ResolveProxy [this=%x]\n", this));

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
    PRBool result = PR_FALSE;
    nsCAutoString buf, metaKey;
    mCachedResponseHead->GetHeader(nsHttp::Vary, buf);
    if (!buf.IsEmpty()) {
        NS_NAMED_LITERAL_CSTRING(prefix, "request-");

        
        char *val = buf.BeginWriting(); 
        char *token = nsCRT::strtok(val, NS_HTTP_HEADER_SEPS, &val);
        while (token) {
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            if ((*token == '*') || (PL_strcasecmp(token, "cookie") == 0)) {
                result = PR_TRUE;
                break;
            }
            else {
                
                metaKey = prefix + nsDependentCString(token);

                
                
                nsXPIDLCString lastVal;
                mCacheEntry->GetMetaDataElement(metaKey.get(), getter_Copies(lastVal));
                if (lastVal) {
                    nsHttpAtom atom = nsHttp::ResolveAtom(token);
                    const char *newVal = mRequestHead.PeekHeader(atom);
                    if (newVal && (strcmp(newVal, lastVal) != 0)) {
                        result = PR_TRUE; 
                        break;
                    }
                }
                
                
                token = nsCRT::strtok(val, NS_HTTP_HEADER_SEPS, &val);
            }
        }
    }
    return result;
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
    
    
    
    

    LOG(("nsHttpChannel::ProcessPartialContent [this=%x]\n", this)); 

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

    
    delete mResponseHead;
    mResponseHead = mCachedResponseHead;
    mCachedResponseHead = 0;

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

    LOG(("nsHttpChannel::OnDoneReadingPartialCacheEntry [this=%x]", this));

    
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

    LOG(("nsHttpChannel::ProcessNotModified [this=%x]\n", this)); 

    NS_ENSURE_TRUE(mCachedResponseHead, NS_ERROR_NOT_INITIALIZED);
    NS_ENSURE_TRUE(mCacheEntry, NS_ERROR_NOT_INITIALIZED);

    
    rv = mCachedResponseHead->UpdateHeaders(mResponseHead->Headers());
    if (NS_FAILED(rv)) return rv;

    
    nsCAutoString head;
    mCachedResponseHead->Flatten(head, PR_TRUE);
    rv = mCacheEntry->SetMetaDataElement("response-head", head.get());
    if (NS_FAILED(rv)) return rv;

    
    delete mResponseHead;
    mResponseHead = mCachedResponseHead;
    mCachedResponseHead = 0;

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
nsHttpChannel::ProcessFallback(PRBool *fallingBack)
{
    LOG(("nsHttpChannel::ProcessFallback [this=%x]\n", this));
    nsresult rv;

    *fallingBack = PR_FALSE;

    
    
    
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

    
    PRUint32 redirectFlags = nsIChannelEventSink::REDIRECT_INTERNAL;
    rv = gHttpHandler->OnChannelRedirect(this, newChannel, redirectFlags);
    if (NS_FAILED(rv))
        return rv;

    
    newChannel->SetOriginalURI(mOriginalURI);
    
    rv = newChannel->AsyncOpen(mListener, mListenerContext);
    NS_ENSURE_SUCCESS(rv, rv);

    
    Cancel(NS_BINDING_REDIRECTED);

    
    mListener = 0;
    mListenerContext = 0;
    
    mCallbacks = nsnull;
    mProgressSink = nsnull;

    *fallingBack = PR_TRUE;

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
nsHttpChannel::OpenCacheEntry(PRBool offline, PRBool *delayed)
{
    nsresult rv;

    *delayed = PR_FALSE;
    mLoadedFromApplicationCache = PR_FALSE;

    LOG(("nsHttpChannel::OpenCacheEntry [this=%x]", this));

    
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

    if (RequestIsConditional()) {
        
        
        return NS_OK;
    }

    GenerateCacheKey(mPostID, cacheKey);

    
    nsCacheStoragePolicy storagePolicy = DetermineStoragePolicy();

    
    nsCacheAccessMode accessRequested;
    if (offline || (mLoadFlags & INHIBIT_CACHING)) {
        
        
        
        if (BYPASS_LOCAL_CACHE(mLoadFlags) && !offline)
            return NS_ERROR_NOT_AVAILABLE;
        accessRequested = nsICache::ACCESS_READ;
    }
    else if (BYPASS_LOCAL_CACHE(mLoadFlags))
        accessRequested = nsICache::ACCESS_WRITE; 
    else
        accessRequested = nsICache::ACCESS_READ_WRITE; 

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

    
    
    PRBool waitingForValidation = PR_FALSE;

    
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

        
        
        
        
        
        
        
        
        rv = session->OpenCacheEntry(cacheKey,
                                     nsICache::ACCESS_READ, PR_FALSE,
                                     getter_AddRefs(mCacheEntry));
        if (rv == NS_ERROR_CACHE_WAIT_FOR_VALIDATION) {
            accessRequested = nsICache::ACCESS_READ;
            waitingForValidation = PR_TRUE;
            rv = NS_OK;
        }

        if (NS_FAILED(rv) && !mCacheForOfflineUse && !mFallbackChannel) {
            
            nsCOMPtr<nsIApplicationCacheNamespace> namespaceEntry;
            rv = mApplicationCache->GetMatchingNamespace
                (cacheKey, getter_AddRefs(namespaceEntry));
            NS_ENSURE_SUCCESS(rv, rv);

            PRUint32 namespaceType = 0;
            if (!namespaceEntry ||
                NS_FAILED(namespaceEntry->GetItemType(&namespaceType)) ||
                (namespaceType &
                 (nsIApplicationCacheNamespace::NAMESPACE_FALLBACK |
                  nsIApplicationCacheNamespace::NAMESPACE_OPPORTUNISTIC |
                  nsIApplicationCacheNamespace::NAMESPACE_BYPASS)) == 0) {
                
                
                
                
                mLoadFlags |= LOAD_ONLY_FROM_CACHE;

                
                
                return NS_ERROR_CACHE_KEY_NOT_FOUND;
            }

            if (namespaceType &
                nsIApplicationCacheNamespace::NAMESPACE_FALLBACK) {
                rv = namespaceEntry->GetData(mFallbackKey);
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
        else if (NS_SUCCEEDED(rv)) {
            
            
            mLoadedFromApplicationCache = PR_TRUE;
        }
    }

    if (!mCacheEntry && !waitingForValidation) {
        rv = gHttpHandler->GetCacheSession(storagePolicy,
                                           getter_AddRefs(session));
        if (NS_FAILED(rv)) return rv;

        rv = session->OpenCacheEntry(cacheKey, accessRequested, PR_FALSE,
                                     getter_AddRefs(mCacheEntry));
        if (rv == NS_ERROR_CACHE_WAIT_FOR_VALIDATION) {
            waitingForValidation = PR_TRUE;
            rv = NS_OK;
        }
        if (NS_FAILED(rv)) return rv;
    }

    if (waitingForValidation) {
        
        
        if (mLoadFlags & LOAD_BYPASS_LOCAL_CACHE_IF_BUSY) {
            LOG(("bypassing local cache since it is busy\n"));
            return NS_ERROR_NOT_AVAILABLE;
        }
        rv = session->AsyncOpenCacheEntry(cacheKey, accessRequested, this);
        if (NS_FAILED(rv)) return rv;
        
        *delayed = PR_TRUE;
    }
    else if (NS_SUCCEEDED(rv)) {
        mCacheEntry->GetAccessGranted(&mCacheAccess);
        LOG(("nsHttpChannel::OpenCacheEntry [this=%x grantedAccess=%d]", this, mCacheAccess));
    }
    return rv;
}


nsresult
nsHttpChannel::OpenOfflineCacheEntryForWriting()
{
    nsresult rv;

    LOG(("nsHttpChannel::OpenOfflineCacheEntryForWriting [this=%x]", this));

    
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

    if (RequestIsConditional()) {
        
        
        return NS_OK;
    }

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

    LOG(("nsHTTPChannel::CheckCache enter [this=%x entry=%x access=%d]",
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

    
    NS_ASSERTION(!mCachedResponseHead, "memory leak detected");
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

    if (method != nsHttp::Head && !isCachedRedirect) {
        
        
        
        
        nsInt64 contentLength = mCachedResponseHead->ContentLength();
        if (contentLength != nsInt64(-1)) {
            PRUint32 size;
            rv = mCacheEntry->GetDataSize(&size);
            NS_ENSURE_SUCCESS(rv, rv);

            if (nsInt64(size) != contentLength) {
                LOG(("Cached data size does not match the Content-Length header "
                     "[content-length=%lld size=%u]\n", PRInt64(contentLength), size));
                if ((nsInt64(size) < contentLength) && mCachedResponseHead->IsResumable()) {
                    
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

    
    mRequestHead.ClearHeader(nsHttp::If_Modified_Since);
    mRequestHead.ClearHeader(nsHttp::If_None_Match);

    
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

    if (!doValidation) {
        
        
        
        
        
        
        
        
        
        
        
        mCacheEntry->GetMetaDataElement("auth", getter_Copies(buf));
        doValidation =
            (fromPreviousSession && !buf.IsEmpty()) ||
            (buf.IsEmpty() && mRequestHead.PeekHeader(nsHttp::Authorization));
    }

    if (!doValidation) {
        
        
        
        if (isCachedRedirect && mRequestHead.PeekHeader(nsHttp::Cookie))
            doValidation = PR_TRUE;
    }

    mCachedContentIsValid = !doValidation;

    if (doValidation) {
        
        
        
        
        
        
        
        
        
        
        if (!mCachedResponseHead->NoStore() &&
            (mRequestHead.Method() == nsHttp::Get ||
             mRequestHead.Method() == nsHttp::Head)) {
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
    }

    LOG(("nsHTTPChannel::CheckCache exit [this=%x doValidation=%d]\n", this, doValidation));
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

    LOG(("nsHttpChannel::ReadFromCache [this=%x] "
         "Using cached copy of: %s\n", this, mSpec.get()));

    if (mCachedResponseHead) {
        NS_ASSERTION(!mResponseHead, "memory leak");
        mResponseHead = mCachedResponseHead;
        mCachedResponseHead = 0;
    }

    
    
    
    
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
                                   stream, nsInt64(-1), nsInt64(-1), 0, 0,
                                   PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    return mCachePump->AsyncRead(this, mListenerContext);
}

void
nsHttpChannel::CloseCacheEntry(PRBool doomOnFailure)
{
    if (!mCacheEntry)
        return;

    LOG(("nsHttpChannel::CloseCacheEntry [this=%x]", this));

    
    
    
    

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

    if (mCachedResponseHead) {
        delete mCachedResponseHead;
        mCachedResponseHead = 0;
    }

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

    LOG(("nsHttpChannel::CloseOfflineCacheEntry [this=%x]", this));

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

    LOG(("nsHttpChannel::InitCacheEntry [this=%x entry=%x]\n",
        this, mCacheEntry.get()));

    
    
    if (mResponseHead->NoStore())
        mLoadFlags |= INHIBIT_PERSISTENT_CACHING;

    
    
    if (!gHttpHandler->CanCacheAllSSLContent() &&
        mConnectionInfo->UsingSSL() && !mResponseHead->CacheControlPublic())
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

    if (mResponseHead->NoStore()) {
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
                if ((*token != '*') && (PL_strcasecmp(token, "cookie") != 0)) {
                    nsHttpAtom atom = nsHttp::ResolveAtom(token);
                    const char *requestVal = mRequestHead.PeekHeader(atom);
                    if (requestVal) {
                        
                        metaKey = prefix + nsDependentCString(token);
                        entry->SetMetaDataElement(metaKey.get(), requestVal);
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
    LOG(("nsHttpChannel::FinalizeCacheEntry [this=%x]\n", this));

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

    rv = tee->Init(mListener, out);
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

    rv = tee->Init(mListener, out);
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





static PLDHashOperator
CopyProperties(const nsAString& aKey, nsIVariant *aData, void *aClosure)
{
    nsIWritablePropertyBag* bag = static_cast<nsIWritablePropertyBag*>
                                             (aClosure);
    bag->SetProperty(aKey, aData);
    return PL_DHASH_NEXT;
}

nsresult
nsHttpChannel::SetupReplacementChannel(nsIURI       *newURI, 
                                       nsIChannel   *newChannel,
                                       PRBool        preserveMethod)
{
    PRUint32 newLoadFlags = mLoadFlags | LOAD_REPLACE;
    
    
    
    
    
    
    if (mConnectionInfo->UsingSSL())
        newLoadFlags &= ~INHIBIT_PERSISTENT_CACHING;

    
    newLoadFlags &= ~LOAD_CHECK_OFFLINE_CACHE;

    newChannel->SetLoadGroup(mLoadGroup); 
    newChannel->SetNotificationCallbacks(mCallbacks);
    newChannel->SetLoadFlags(newLoadFlags);

    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(newChannel);
    if (!httpChannel)
        return NS_OK; 

    if (preserveMethod) {
        nsCOMPtr<nsIUploadChannel> uploadChannel = do_QueryInterface(httpChannel);
        if (mUploadStream && uploadChannel) {
            
            nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(mUploadStream);
            if (seekable)
                seekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);

            
            if (mUploadStreamHasHeaders)
                uploadChannel->SetUploadStream(mUploadStream, EmptyCString(), -1);
            else {
                const char *ctype = mRequestHead.PeekHeader(nsHttp::Content_Type);
                const char *clen  = mRequestHead.PeekHeader(nsHttp::Content_Length);
                if (ctype && clen)
                    uploadChannel->SetUploadStream(mUploadStream,
                                                   nsDependentCString(ctype),
                                                   atoi(clen));
            }
        }
        
        
        httpChannel->SetRequestMethod(nsDependentCString(mRequestHead.Method()));
    }
    
    if (mReferrer)
        httpChannel->SetReferrer(mReferrer);
    
    httpChannel->SetAllowPipelining(mAllowPipelining);
    
    httpChannel->SetRedirectionLimit(mRedirectionLimit - 1);

    nsCOMPtr<nsIHttpChannelInternal> httpInternal = do_QueryInterface(newChannel);
    if (httpInternal) {
        
        
        
        
        if (newURI && (mURI == mDocumentURI))
            httpInternal->SetDocumentURI(newURI);
        else
            httpInternal->SetDocumentURI(mDocumentURI);
    } 
    
    
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

    
    nsCOMPtr<nsIApplicationCacheChannel> appCacheChannel =
        do_QueryInterface(newChannel);
    if (appCacheChannel) {
        appCacheChannel->SetApplicationCache(mApplicationCache);
        appCacheChannel->SetInheritApplicationCache(mInheritApplicationCache);
        
    }

    
    nsCOMPtr<nsIWritablePropertyBag> bag(do_QueryInterface(newChannel));
    if (bag)
        mPropertyHash.EnumerateRead(CopyProperties, bag.get());

    return NS_OK;
}

nsresult
nsHttpChannel::ProcessRedirection(PRUint32 redirectType)
{
    LOG(("nsHttpChannel::ProcessRedirection [this=%x type=%u]\n",
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

    LOG(("redirecting to: %s [redirection-limit=%u]\n",
        location, PRUint32(mRedirectionLimit)));

    nsresult rv;
    nsCOMPtr<nsIChannel> newChannel;
    nsCOMPtr<nsIURI> newURI;

    
    
    nsCOMPtr<nsIIOService> ioService;
    rv = gHttpHandler->GetIOService(getter_AddRefs(ioService));
    if (NS_FAILED(rv)) return rv;

    
    nsCAutoString originCharset;
    rv = mURI->GetOriginCharset(originCharset);
    if (NS_FAILED(rv))
        originCharset.Truncate();

    rv = ioService->NewURI(nsDependentCString(location), originCharset.get(), mURI,
                           getter_AddRefs(newURI));
    if (NS_FAILED(rv)) return rv;

    if (mApplicationCache) {
        
        
        
        if (!NS_SecurityCompareURIs(mURI, newURI, PR_FALSE)) {
            PRBool fallingBack;
            rv = ProcessFallback(&fallingBack);
            if (NS_SUCCEEDED(rv) && fallingBack) {
                
                
                return NS_OK;
            }
        }
    }

    
    
    PRBool redirectingBackToSameURI = PR_FALSE;
    if (mCacheEntry && (mCacheAccess & nsICache::ACCESS_WRITE) &&
        NS_SUCCEEDED(mURI->Equals(newURI, &redirectingBackToSameURI)) &&
        redirectingBackToSameURI)
            mCacheEntry->Doom();

    
    
    nsCOMPtr<nsIURL> newURL = do_QueryInterface(newURI);
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

    
    PRBool preserveMethod = (redirectType == 307);
    if (preserveMethod && mUploadStream) {
        rv = PromptTempRedirect();
        if (NS_FAILED(rv)) return rv;
    }

    rv = ioService->NewChannelFromURI(newURI, getter_AddRefs(newChannel));
    if (NS_FAILED(rv)) return rv;

    rv = SetupReplacementChannel(newURI, newChannel, preserveMethod);
    if (NS_FAILED(rv)) return rv;

    PRUint32 redirectFlags;
    if (redirectType == 301) 
        redirectFlags = nsIChannelEventSink::REDIRECT_PERMANENT;
    else
        redirectFlags = nsIChannelEventSink::REDIRECT_TEMPORARY;

    
    rv = gHttpHandler->OnChannelRedirect(this, newChannel, redirectFlags);
    if (NS_FAILED(rv))
        return rv;

    
    newChannel->SetOriginalURI(mOriginalURI);    

    
    nsCOMPtr<nsIHttpEventSink> httpEventSink;
    GetCallback(httpEventSink);
    if (httpEventSink) {
        
        
        rv = httpEventSink->OnRedirect(this, newChannel);
        if (NS_FAILED(rv)) return rv;
    }
    
    

    
    rv = newChannel->AsyncOpen(mListener, mListenerContext);
    if (NS_FAILED(rv)) return rv;

    
    Cancel(NS_BINDING_REDIRECTED);
    
    
    mListener = 0;
    mListenerContext = 0;
    
    mCallbacks = nsnull;
    mProgressSink = nsnull;
    return NS_OK;
}






static void
ParseUserDomain(PRUnichar *buf,
                const PRUnichar **user,
                const PRUnichar **domain)
{
    PRUnichar *p = buf;
    while (*p && *p != '\\') ++p;
    if (!*p)
        return;
    *p = '\0';
    *domain = buf;
    *user = p + 1;
}


static void
SetIdent(nsHttpAuthIdentity &ident,
         PRUint32 authFlags,
         PRUnichar *userBuf,
         PRUnichar *passBuf)
{
    const PRUnichar *user = userBuf;
    const PRUnichar *domain = nsnull;

    if (authFlags & nsIHttpAuthenticator::IDENTITY_INCLUDES_DOMAIN)
        ParseUserDomain(userBuf, &user, &domain);

    ident.Set(domain, user, passBuf);
}


static void
GetAuthPrompt(nsIInterfaceRequestor *ifreq, PRBool proxyAuth,
              nsIAuthPrompt2 **result)
{
    if (!ifreq)
        return;

    PRUint32 promptReason;
    if (proxyAuth)
        promptReason = nsIAuthPromptProvider::PROMPT_PROXY;
    else 
        promptReason = nsIAuthPromptProvider::PROMPT_NORMAL;

    nsCOMPtr<nsIAuthPromptProvider> promptProvider = do_GetInterface(ifreq);
    if (promptProvider)
        promptProvider->GetAuthPrompt(promptReason,
                                      NS_GET_IID(nsIAuthPrompt2),
                                      reinterpret_cast<void**>(result));
    else
        NS_QueryAuthPrompt2(ifreq, result);
}


nsresult
nsHttpChannel::GenCredsAndSetEntry(nsIHttpAuthenticator *auth,
                                   PRBool proxyAuth,
                                   const char *scheme,
                                   const char *host,
                                   PRInt32 port,
                                   const char *directory,
                                   const char *realm,
                                   const char *challenge,
                                   const nsHttpAuthIdentity &ident,
                                   nsCOMPtr<nsISupports> &sessionState,
                                   char **result)
{
    nsresult rv;
    PRUint32 authFlags;

    rv = auth->GetAuthFlags(&authFlags);
    if (NS_FAILED(rv)) return rv;

    nsISupports *ss = sessionState;

    
    
    
    nsISupports **continuationState;

    if (proxyAuth) {
        continuationState = &mProxyAuthContinuationState;
    } else {
        continuationState = &mAuthContinuationState;
    }

    rv = auth->GenerateCredentials(this,
                                   challenge,
                                   proxyAuth,
                                   ident.Domain(),
                                   ident.User(),
                                   ident.Password(),
                                   &ss,
                                   &*continuationState,
                                   result);

    sessionState.swap(ss);
    if (NS_FAILED(rv)) return rv;

    
#ifdef DEBUG 
    LOG(("generated creds: %s\n", *result));
#endif

    
    
    PRBool saveCreds =
        0 != (authFlags & nsIHttpAuthenticator::REUSABLE_CREDENTIALS);
    PRBool saveChallenge =
        0 != (authFlags & nsIHttpAuthenticator::REUSABLE_CHALLENGE);

    
    nsHttpAuthCache *authCache = gHttpHandler->AuthCache();

    
    
    
    
    
    
    rv = authCache->SetAuthEntry(scheme, host, port, directory, realm,
                                 saveCreds ? *result : nsnull,
                                 saveChallenge ? challenge : nsnull,
                                 ident, sessionState);
    return rv;
}

nsresult
nsHttpChannel::ProcessAuthentication(PRUint32 httpStatus)
{
    LOG(("nsHttpChannel::ProcessAuthentication [this=%x code=%u]\n",
        this, httpStatus));

    if (mLoadFlags & LOAD_ANONYMOUS) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    const char *challenges;
    mProxyAuth = (httpStatus == 407);

    nsresult rv = PrepareForAuthentication(mProxyAuth);
    if (NS_FAILED(rv))
        return rv;

    if (mProxyAuth) {
        
        
        
        
        
        
        if (!mConnectionInfo->UsingHttpProxy()) {
            LOG(("rejecting 407 when proxy server not configured!\n"));
            return NS_ERROR_UNEXPECTED;
        }
        if (mConnectionInfo->UsingSSL() && !mTransaction->SSLConnectFailed()) {
            
            
            
            LOG(("rejecting 407 from origin server!\n"));
            return NS_ERROR_UNEXPECTED;
        }
        challenges = mResponseHead->PeekHeader(nsHttp::Proxy_Authenticate);
    }
    else
        challenges = mResponseHead->PeekHeader(nsHttp::WWW_Authenticate);
    NS_ENSURE_TRUE(challenges, NS_ERROR_UNEXPECTED);

    nsCAutoString creds;
    rv = GetCredentials(challenges, mProxyAuth, creds);
    if (rv == NS_ERROR_IN_PROGRESS)  {
        
        
        mAuthRetryPending = PR_TRUE;
        
        
        
        
        LOG(("Suspending the transaction, asynchronously prompting for credentials"));
        mTransactionPump->Suspend();
        return NS_OK;
    }
    else if (NS_FAILED(rv))
        LOG(("unable to authenticate\n"));
    else {
        
        if (mProxyAuth)
            mRequestHead.SetHeader(nsHttp::Proxy_Authorization, creds);
        else
            mRequestHead.SetHeader(nsHttp::Authorization, creds);

        mAuthRetryPending = PR_TRUE; 
    }
    return rv;
}

nsresult
nsHttpChannel::PrepareForAuthentication(PRBool proxyAuth)
{
    LOG(("nsHttpChannel::PrepareForAuthentication [this=%x]\n", this));

    if (!proxyAuth) {
        
        
        NS_IF_RELEASE(mProxyAuthContinuationState);
        LOG(("  proxy continuation state has been reset"));
    }

    if (!mConnectionInfo->UsingHttpProxy() || mProxyAuthType.IsEmpty())
        return NS_OK;

    
    

    nsCAutoString contractId;
    contractId.Assign(NS_HTTP_AUTHENTICATOR_CONTRACTID_PREFIX);
    contractId.Append(mProxyAuthType);

    nsresult rv;
    nsCOMPtr<nsIHttpAuthenticator> precedingAuth =
        do_GetService(contractId.get(), &rv);
    if (NS_FAILED(rv))
        return rv;

    PRUint32 precedingAuthFlags;
    rv = precedingAuth->GetAuthFlags(&precedingAuthFlags);
    if (NS_FAILED(rv))
        return rv;

    if (!(precedingAuthFlags & nsIHttpAuthenticator::REQUEST_BASED)) {
        const char *challenges =
                mResponseHead->PeekHeader(nsHttp::Proxy_Authenticate);
        if (!challenges) {
            
            
            mRequestHead.ClearHeader(nsHttp::Proxy_Authorization);
            LOG(("  cleared proxy authorization header"));
        }
    }

    return NS_OK;
}

nsresult
nsHttpChannel::GetCredentials(const char *challenges,
                              PRBool proxyAuth,
                              nsAFlatCString &creds)
{
    nsCOMPtr<nsIHttpAuthenticator> auth;
    nsCAutoString challenge;

    nsCString authType; 
                        

    
    
    nsISupports **currentContinuationState;
    nsCString *currentAuthType;

    if (proxyAuth) {
        currentContinuationState = &mProxyAuthContinuationState;
        currentAuthType = &mProxyAuthType;
    } else {
        currentContinuationState = &mAuthContinuationState;
        currentAuthType = &mAuthType;
    }

    nsresult rv = NS_ERROR_NOT_AVAILABLE;
    PRBool gotCreds = PR_FALSE;
    
    
    for (const char *eol = challenges - 1; eol; ) {
        const char *p = eol + 1;

        
        if ((eol = strchr(p, '\n')) != nsnull)
            challenge.Assign(p, eol - p);
        else
            challenge.Assign(p);

        rv = GetAuthenticator(challenge.get(), authType, getter_AddRefs(auth));
        if (NS_SUCCEEDED(rv)) {
            
            
            
            
            
            
            if (!currentAuthType->IsEmpty() && authType != *currentAuthType)
                continue;

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            rv = GetCredentialsForChallenge(challenge.get(), authType.get(),
                                            proxyAuth, auth, creds);
            if (NS_SUCCEEDED(rv)) {
                gotCreds = PR_TRUE;
                *currentAuthType = authType;

                break;
            }
            else if (rv == NS_ERROR_IN_PROGRESS) {
                
                
                
                
                mCurrentChallenge = challenge;
                mRemainingChallenges = eol ? eol+1 : nsnull;
                return rv;
            }

            
            NS_IF_RELEASE(*currentContinuationState);
            currentAuthType->Truncate();
        }
    }

    if (!gotCreds && !currentAuthType->IsEmpty()) {
        
        
        currentAuthType->Truncate();
        NS_IF_RELEASE(*currentContinuationState);

        rv = GetCredentials(challenges, proxyAuth, creds);
    }

    return rv;
}

nsresult
nsHttpChannel::GetAuthorizationMembers(PRBool proxyAuth,
                                       nsCSubstring& scheme,
                                       const char*& host,
                                       PRInt32& port,
                                       nsCSubstring& path,
                                       nsHttpAuthIdentity*& ident,
                                       nsISupports**& continuationState)
{
    if (proxyAuth) {
        NS_ASSERTION (mConnectionInfo->UsingHttpProxy(), "proxyAuth is true, but no HTTP proxy is configured!");

        host = mConnectionInfo->ProxyHost();
        port = mConnectionInfo->ProxyPort();
        ident = &mProxyIdent;
        scheme.AssignLiteral("http");

        continuationState = &mProxyAuthContinuationState;
    }
    else {
        host = mConnectionInfo->Host();
        port = mConnectionInfo->Port();
        ident = &mIdent;

        nsresult rv;
        rv = GetCurrentPath(path);
        if (NS_FAILED(rv)) return rv;

        rv = mURI->GetScheme(scheme);
        if (NS_FAILED(rv)) return rv;

        continuationState = &mAuthContinuationState;
    }

    return NS_OK;
}

nsresult
nsHttpChannel::GetCredentialsForChallenge(const char *challenge,
                                          const char *authType,
                                          PRBool proxyAuth,
                                          nsIHttpAuthenticator *auth,
                                          nsAFlatCString &creds)
{
    LOG(("nsHttpChannel::GetCredentialsForChallenge [this=%x proxyAuth=%d challenges=%s]\n",
        this, proxyAuth, challenge));

    
    nsHttpAuthCache *authCache = gHttpHandler->AuthCache();

    PRUint32 authFlags;
    nsresult rv = auth->GetAuthFlags(&authFlags);
    if (NS_FAILED(rv)) return rv;

    nsCAutoString realm;
    ParseRealm(challenge, realm);

    
    
    
    
    






    
    
    
    const char *host;
    PRInt32 port;
    nsHttpAuthIdentity *ident;
    nsCAutoString path, scheme;
    PRBool identFromURI = PR_FALSE;
    nsISupports **continuationState;

    rv = GetAuthorizationMembers(proxyAuth, scheme, host, port, path, ident, continuationState);
    if (NS_FAILED(rv)) return rv;

    if (!proxyAuth) {
        
        
        if (mIdent.IsEmpty()) {
            GetIdentityFromURI(authFlags, mIdent);
            identFromURI = !mIdent.IsEmpty();
        }
    }

    
    
    
    
    
    
    nsHttpAuthEntry *entry = nsnull;
    authCache->GetAuthEntryForDomain(scheme.get(), host, port, realm.get(), &entry);

    
    
    nsCOMPtr<nsISupports> sessionStateGrip;
    if (entry)
        sessionStateGrip = entry->mMetaData;

    
    PRBool identityInvalid;
    nsISupports *sessionState = sessionStateGrip;
    rv = auth->ChallengeReceived(this,
                                 challenge,
                                 proxyAuth,
                                 &sessionState,
                                 &*continuationState,
                                 &identityInvalid);
    sessionStateGrip.swap(sessionState);
    if (NS_FAILED(rv)) return rv;

    LOG(("  identity invalid = %d\n", identityInvalid));

    if (identityInvalid) {
        if (entry) {
            if (ident->Equals(entry->Identity())) {
                LOG(("  clearing bad auth cache entry\n"));
                
                
                authCache->ClearAuthEntry(scheme.get(), host, port, realm.get());
                entry = nsnull;
                ident->Clear();
            }
            else if (!identFromURI || nsCRT::strcmp(ident->User(), entry->Identity().User()) == 0) {
                LOG(("  taking identity from auth cache\n"));
                
                
                
                
                
                
                ident->Set(entry->Identity());
                identFromURI = PR_FALSE;
                if (entry->Creds()[0] != '\0') {
                    LOG(("    using cached credentials!\n"));
                    creds.Assign(entry->Creds());
                    return entry->AddPath(path.get());
                }
            }
        }
        else if (!identFromURI) {
            
            
            ident->Clear();
        }

        if (!entry && ident->IsEmpty()) {
            PRUint32 level = nsIAuthPrompt2::LEVEL_NONE;
            if (scheme.EqualsLiteral("https"))
                level = nsIAuthPrompt2::LEVEL_SECURE;
            else if (authFlags & nsIHttpAuthenticator::IDENTITY_ENCRYPTED)
                level = nsIAuthPrompt2::LEVEL_PW_ENCRYPTED;

            
            
            rv = PromptForIdentity(level, proxyAuth, realm.get(), 
                                   authType, authFlags, *ident);
            if (NS_FAILED(rv)) return rv;
            identFromURI = PR_FALSE;
        }
    }

    if (identFromURI) {
        
        
        if (!ConfirmAuth(NS_LITERAL_STRING("AutomaticAuth"), PR_FALSE)) {
            
            
            Cancel(NS_ERROR_ABORT);
            
            
            
            
            return NS_ERROR_ABORT;
        }
    }

    
    
    
    
    
    
    
    
    
    
    nsXPIDLCString result;
    rv = GenCredsAndSetEntry(auth, proxyAuth, scheme.get(), host, port, path.get(),
                             realm.get(), challenge, *ident, sessionStateGrip,
                             getter_Copies(result));
    if (NS_SUCCEEDED(rv))
        creds = result;
    return rv;
}

nsresult
nsHttpChannel::GetAuthenticator(const char *challenge,
                                nsCString &authType,
                                nsIHttpAuthenticator **auth)
{
    LOG(("nsHttpChannel::GetAuthenticator [this=%x]\n", this));

    GetAuthType(challenge, authType);
 
    
    ToLowerCase(authType);

    nsCAutoString contractid;
    contractid.Assign(NS_HTTP_AUTHENTICATOR_CONTRACTID_PREFIX);
    contractid.Append(authType);

    return CallGetService(contractid.get(), auth);
}

void
nsHttpChannel::GetIdentityFromURI(PRUint32 authFlags, nsHttpAuthIdentity &ident)
{
    LOG(("nsHttpChannel::GetIdentityFromURI [this=%x]\n", this));

    nsAutoString userBuf;
    nsAutoString passBuf;

    
    nsCAutoString buf;
    mURI->GetUsername(buf);
    if (!buf.IsEmpty()) {
        NS_UnescapeURL(buf);
        CopyASCIItoUTF16(buf, userBuf);
        mURI->GetPassword(buf);
        if (!buf.IsEmpty()) {
            NS_UnescapeURL(buf);
            CopyASCIItoUTF16(buf, passBuf);
        }
    }

    if (!userBuf.IsEmpty())
        SetIdent(ident, authFlags, (PRUnichar *) userBuf.get(), (PRUnichar *) passBuf.get());
}

void
nsHttpChannel::ParseRealm(const char *challenge, nsACString &realm)
{
    
    
    
    
    
    
    
    
    
    const char *p = PL_strcasestr(challenge, "realm=");
    if (p) {
        PRBool has_quote = PR_FALSE;
        p += 6;
        if (*p == '"') {
            has_quote = PR_TRUE;
            p++;
        }

        const char *end = p;
        while (*end && has_quote) {
           
           
            if (*end == '"' && end[-1] != '\\')
                break;
            ++end;
        }

        if (!has_quote)
            end = strchr(p, ' '); 
        if (end)
            realm.Assign(p, end - p);
        else
            realm.Assign(p);
    }
}


class nsHTTPAuthInformation : public nsAuthInformationHolder {
public:
    nsHTTPAuthInformation(PRUint32 aFlags, const nsString& aRealm,
                          const nsCString& aAuthType)
        : nsAuthInformationHolder(aFlags, aRealm, aAuthType) {}

    void SetToHttpAuthIdentity(PRUint32 authFlags, nsHttpAuthIdentity& identity);
};

void
nsHTTPAuthInformation::SetToHttpAuthIdentity(PRUint32 authFlags, nsHttpAuthIdentity& identity)
{
    identity.Set(Domain().get(), User().get(), Password().get());
}

nsresult
nsHttpChannel::PromptForIdentity(PRUint32    level,
                                 PRBool      proxyAuth,
                                 const char *realm,
                                 const char *authType,
                                 PRUint32 authFlags,
                                 nsHttpAuthIdentity &ident)
{
    LOG(("nsHttpChannel::PromptForIdentity [this=%x]\n", this));

    nsCOMPtr<nsIAuthPrompt2> authPrompt;
    GetAuthPrompt(mCallbacks, proxyAuth, getter_AddRefs(authPrompt));
    if (!authPrompt && mLoadGroup) {
        nsCOMPtr<nsIInterfaceRequestor> cbs;
        mLoadGroup->GetNotificationCallbacks(getter_AddRefs(cbs));
        GetAuthPrompt(cbs, proxyAuth, getter_AddRefs(authPrompt));
    }
    if (!authPrompt)
        return NS_ERROR_NO_INTERFACE;

    
    NS_ConvertASCIItoUTF16 realmU(realm);

    nsresult rv;

    
    PRUint32 promptFlags = 0;
    if (proxyAuth)
        promptFlags |= nsIAuthInformation::AUTH_PROXY;
    else
        promptFlags |= nsIAuthInformation::AUTH_HOST;

    if (authFlags & nsIHttpAuthenticator::IDENTITY_INCLUDES_DOMAIN)
        promptFlags |= nsIAuthInformation::NEED_DOMAIN;

    nsRefPtr<nsHTTPAuthInformation> holder =
        new nsHTTPAuthInformation(promptFlags, realmU,
                                  nsDependentCString(authType));
    if (!holder)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = authPrompt->AsyncPromptAuth(this, this, nsnull, level, holder,
                     getter_AddRefs(mAsyncPromptAuthCancelable));

    if (NS_SUCCEEDED(rv)) {
        
        
        rv = NS_ERROR_IN_PROGRESS;
    }
    else {
        
        PRBool retval = PR_FALSE;
        rv = authPrompt->PromptAuth(this, level, holder, &retval);
        if (NS_FAILED(rv))
            return rv;

        if (!retval)
            rv = NS_ERROR_ABORT;
        else
            holder->SetToHttpAuthIdentity(authFlags, ident);
    }

    
    if (!proxyAuth)
        mSuppressDefensiveAuth = PR_TRUE;

    return rv;
}

NS_IMETHODIMP nsHttpChannel::OnAuthAvailable(nsISupports *aContext,
                                             nsIAuthInformation *aAuthInfo)
{
    LOG(("nsHttpChannel::OnAuthAvailable [this=%x]", this));
    mAsyncPromptAuthCancelable = nsnull;

    nsresult rv;

    const char *host;
    PRInt32 port;
    nsHttpAuthIdentity *ident;
    nsCAutoString path, scheme;
    nsISupports **continuationState;
    rv = GetAuthorizationMembers(mProxyAuth, scheme, host, port, path, ident, continuationState);
    if (NS_FAILED(rv))
        OnAuthCancelled(aContext, PR_FALSE);

    nsCAutoString realm;
    ParseRealm(mCurrentChallenge.get(), realm);

    nsHttpAuthCache *authCache = gHttpHandler->AuthCache();
    nsHttpAuthEntry *entry = nsnull;
    authCache->GetAuthEntryForDomain(scheme.get(), host, port, realm.get(), &entry);

    nsCOMPtr<nsISupports> sessionStateGrip;
    if (entry)
        sessionStateGrip = entry->mMetaData;

    nsAuthInformationHolder* holder =
            static_cast<nsAuthInformationHolder*>(aAuthInfo);
    ident->Set(holder->Domain().get(),
               holder->User().get(),
               holder->Password().get());

    nsCAutoString unused;
    nsCOMPtr<nsIHttpAuthenticator> auth;
    rv = GetAuthenticator(mCurrentChallenge.get(), unused, getter_AddRefs(auth));
    if (NS_FAILED(rv)) {
        NS_ASSERTION(PR_FALSE, "GetAuthenticator failed");
        OnAuthCancelled(aContext, PR_TRUE);
        return NS_OK;
    }

    nsXPIDLCString creds;
    rv = GenCredsAndSetEntry(auth, mProxyAuth,
                             scheme.get(), host, port, path.get(),
                             realm.get(), mCurrentChallenge.get(), *ident, sessionStateGrip,
                             getter_Copies(creds));

    mCurrentChallenge.Truncate();
    if (NS_FAILED(rv)) {
        OnAuthCancelled(aContext, PR_TRUE);
        return NS_OK;
    }

    return ContinueOnAuthAvailable(creds);
}

NS_IMETHODIMP nsHttpChannel::OnAuthCancelled(nsISupports *aContext, 
                                             PRBool userCancel)
{
    LOG(("nsHttpChannel::OnAuthCancelled [this=%x]", this));
    mAsyncPromptAuthCancelable = nsnull;
    if (userCancel) {
        if (!mRemainingChallenges.IsEmpty()) {
            
            nsresult rv;

            nsCAutoString creds;
            rv = GetCredentials(mRemainingChallenges.get(), mProxyAuth, creds);
            if (NS_SUCCEEDED(rv)) {
                
                
                
                mRemainingChallenges.Truncate();
                return ContinueOnAuthAvailable(creds);
            }
            else if (rv == NS_ERROR_IN_PROGRESS) {
                
                
                
                return NS_OK;
            }

            
        }

        mRemainingChallenges.Truncate();

        
        
        nsresult rv = CallOnStartRequest();

        
        
        mAuthRetryPending = PR_FALSE;
        LOG(("Resuming the transaction, user cancelled the auth dialog"));
        mTransactionPump->Resume();

        if (NS_FAILED(rv))
            mTransactionPump->Cancel(rv);
    }

    return NS_OK;
}

nsresult
nsHttpChannel::ContinueOnAuthAvailable(const nsCSubstring& creds)
{
    if (mProxyAuth)
        mRequestHead.SetHeader(nsHttp::Proxy_Authorization, creds);
    else
        mRequestHead.SetHeader(nsHttp::Authorization, creds);

    
    
    
    
    mRemainingChallenges.Truncate();

    
    
    
    mAuthRetryPending = PR_TRUE;
    LOG(("Resuming the transaction, we got credentials from user"));
    mTransactionPump->Resume();

    return NS_OK;
}

PRBool
nsHttpChannel::ConfirmAuth(const nsString &bundleKey, PRBool doYesNoPrompt)
{
    
    
    
    

    if (mSuppressDefensiveAuth || !(mLoadFlags & LOAD_INITIAL_DOCUMENT_URI))
        return PR_TRUE;

    nsresult rv;
    nsCAutoString userPass;
    rv = mURI->GetUserPass(userPass);
    if (NS_FAILED(rv) || (userPass.Length() < gHttpHandler->PhishyUserPassLength()))
        return PR_TRUE;

    
    
    

    nsCOMPtr<nsIStringBundleService> bundleService =
            do_GetService(NS_STRINGBUNDLE_CONTRACTID);
    if (!bundleService)
        return PR_TRUE;

    nsCOMPtr<nsIStringBundle> bundle;
    bundleService->CreateBundle(NECKO_MSGS_URL, getter_AddRefs(bundle));
    if (!bundle)
        return PR_TRUE;

    nsCAutoString host;
    rv = mURI->GetHost(host);
    if (NS_FAILED(rv))
        return PR_TRUE;

    nsCAutoString user;
    rv = mURI->GetUsername(user);
    if (NS_FAILED(rv))
        return PR_TRUE;

    NS_ConvertUTF8toUTF16 ucsHost(host), ucsUser(user);
    const PRUnichar *strs[2] = { ucsHost.get(), ucsUser.get() };

    nsXPIDLString msg;
    bundle->FormatStringFromName(bundleKey.get(), strs, 2, getter_Copies(msg));
    if (!msg)
        return PR_TRUE;
    
    nsCOMPtr<nsIPrompt> prompt;
    GetCallback(prompt);
    if (!prompt)
        return PR_TRUE;

    
    mSuppressDefensiveAuth = PR_TRUE;

    PRBool confirmed;
    if (doYesNoPrompt) {
        PRInt32 choice;
        PRBool checkState;
        rv = prompt->ConfirmEx(nsnull, msg,
                               nsIPrompt::BUTTON_POS_1_DEFAULT +
                               nsIPrompt::STD_YES_NO_BUTTONS,
                               nsnull, nsnull, nsnull, nsnull, &checkState, &choice);
        if (NS_FAILED(rv))
            return PR_TRUE;

        confirmed = choice == 0;
    }
    else {
        rv = prompt->Confirm(nsnull, msg, &confirmed);
        if (NS_FAILED(rv))
            return PR_TRUE;
    }

    return confirmed;
}

void
nsHttpChannel::CheckForSuperfluousAuth()
{
    
    
    
    
    
    if (!mAuthRetryPending) {
        
        if (!ConfirmAuth(NS_LITERAL_STRING("SuperfluousAuth"), PR_TRUE)) {
            
            
            Cancel(NS_ERROR_ABORT);
        }
    }
}

void
nsHttpChannel::SetAuthorizationHeader(nsHttpAuthCache *authCache,
                                      nsHttpAtom header,
                                      const char *scheme,
                                      const char *host,
                                      PRInt32 port,
                                      const char *path,
                                      nsHttpAuthIdentity &ident)
{
    nsHttpAuthEntry *entry = nsnull;
    nsresult rv;

    
    
    
    nsISupports **continuationState;

    if (header == nsHttp::Proxy_Authorization) {
        continuationState = &mProxyAuthContinuationState;
    } else {
        continuationState = &mAuthContinuationState;
    }

    rv = authCache->GetAuthEntryForPath(scheme, host, port, path, &entry);
    if (NS_SUCCEEDED(rv)) {
        
        
        
        
        
        
        
        if (header == nsHttp::Authorization && entry->Domain()[0] == '\0') {
            GetIdentityFromURI(0, ident);
            
            
            if (nsCRT::strcmp(ident.User(), entry->User()) == 0)
                ident.Clear();
        }
        PRBool identFromURI;
        if (ident.IsEmpty()) {
            ident.Set(entry->Identity());
            identFromURI = PR_FALSE;
        }
        else
            identFromURI = PR_TRUE;

        nsXPIDLCString temp;
        const char *creds     = entry->Creds();
        const char *challenge = entry->Challenge();
        
        
        
        
        if ((!creds[0] || identFromURI) && challenge[0]) {
            nsCOMPtr<nsIHttpAuthenticator> auth;
            nsCAutoString unused;
            rv = GetAuthenticator(challenge, unused, getter_AddRefs(auth));
            if (NS_SUCCEEDED(rv)) {
                PRBool proxyAuth = (header == nsHttp::Proxy_Authorization);
                rv = GenCredsAndSetEntry(auth, proxyAuth, scheme, host, port, path,
                                         entry->Realm(), challenge, ident,
                                         entry->mMetaData, getter_Copies(temp));
                if (NS_SUCCEEDED(rv))
                    creds = temp.get();

                
                
                NS_IF_RELEASE(*continuationState);
            }
        }
        if (creds[0]) {
            LOG(("   adding \"%s\" request header\n", header.get()));
            mRequestHead.SetHeader(header, nsDependentCString(creds));

            
            
            
            
            if (header == nsHttp::Authorization)
                mSuppressDefensiveAuth = PR_TRUE;
        }
        else
            ident.Clear(); 
    }
}

void
nsHttpChannel::AddAuthorizationHeaders()
{
    LOG(("nsHttpChannel::AddAuthorizationHeaders? [this=%x]\n", this));

    if (mLoadFlags & LOAD_ANONYMOUS) {
      return;
    }

    
    nsHttpAuthCache *authCache = gHttpHandler->AuthCache();

    
    const char *proxyHost = mConnectionInfo->ProxyHost();
    if (proxyHost && mConnectionInfo->UsingHttpProxy())
        SetAuthorizationHeader(authCache, nsHttp::Proxy_Authorization,
                               "http", proxyHost, mConnectionInfo->ProxyPort(),
                               nsnull, 
                               mProxyIdent);

    
    nsCAutoString path, scheme;
    if (NS_SUCCEEDED(GetCurrentPath(path)) &&
        NS_SUCCEEDED(mURI->GetScheme(scheme))) {
        SetAuthorizationHeader(authCache, nsHttp::Authorization,
                               scheme.get(),
                               mConnectionInfo->Host(),
                               mConnectionInfo->Port(),
                               path.get(),
                               mIdent);
    }
}

nsresult
nsHttpChannel::GetCurrentPath(nsACString &path)
{
    nsresult rv;
    nsCOMPtr<nsIURL> url = do_QueryInterface(mURI);
    if (url)
        rv = url->GetDirectory(path);
    else
        rv = mURI->GetPath(path);
    return rv;
}





NS_IMPL_ADDREF_INHERITED(nsHttpChannel, nsHashPropertyBag)
NS_IMPL_RELEASE_INHERITED(nsHttpChannel, nsHashPropertyBag)

NS_INTERFACE_MAP_BEGIN(nsHttpChannel)
    NS_INTERFACE_MAP_ENTRY(nsIRequest)
    NS_INTERFACE_MAP_ENTRY(nsIChannel)
    NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
    NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
    NS_INTERFACE_MAP_ENTRY(nsIHttpChannel)
    NS_INTERFACE_MAP_ENTRY(nsICachingChannel)
    NS_INTERFACE_MAP_ENTRY(nsIUploadChannel)
    NS_INTERFACE_MAP_ENTRY(nsICacheListener)
    NS_INTERFACE_MAP_ENTRY(nsIEncodedChannel)
    NS_INTERFACE_MAP_ENTRY(nsIHttpChannelInternal)
    NS_INTERFACE_MAP_ENTRY(nsIResumableChannel)
    NS_INTERFACE_MAP_ENTRY(nsITransportEventSink)
    NS_INTERFACE_MAP_ENTRY(nsISupportsPriority)
    NS_INTERFACE_MAP_ENTRY(nsIProtocolProxyCallback)
    NS_INTERFACE_MAP_ENTRY(nsIProxiedChannel)
    NS_INTERFACE_MAP_ENTRY(nsITraceableChannel)
    NS_INTERFACE_MAP_ENTRY(nsIApplicationCacheContainer)
    NS_INTERFACE_MAP_ENTRY(nsIApplicationCacheChannel)
    NS_INTERFACE_MAP_ENTRY(nsIAuthPromptCallback)
NS_INTERFACE_MAP_END_INHERITING(nsHashPropertyBag)





NS_IMETHODIMP
nsHttpChannel::GetName(nsACString &aName)
{
    aName = mSpec;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::IsPending(PRBool *value)
{
    NS_ENSURE_ARG_POINTER(value);
    *value = mIsPending;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetStatus(nsresult *aStatus)
{
    NS_ENSURE_ARG_POINTER(aStatus);
    *aStatus = mStatus;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::Cancel(nsresult status)
{
    LOG(("nsHttpChannel::Cancel [this=%x status=%x]\n", this, status));
    if (mCanceled) {
        LOG(("  ignoring; already canceled\n"));
        return NS_OK;
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
    if (mAsyncPromptAuthCancelable)
        mAsyncPromptAuthCancelable->Cancel(status);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::Suspend()
{
    NS_ENSURE_TRUE(mIsPending, NS_ERROR_NOT_AVAILABLE);
    
    LOG(("nsHttpChannel::Suspend [this=%x]\n", this));

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
    
    LOG(("nsHttpChannel::Resume [this=%x]\n", this));
        
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
nsHttpChannel::GetLoadGroup(nsILoadGroup **aLoadGroup)
{
    NS_ENSURE_ARG_POINTER(aLoadGroup);
    *aLoadGroup = mLoadGroup;
    NS_IF_ADDREF(*aLoadGroup);
    return NS_OK;
}
NS_IMETHODIMP
nsHttpChannel::SetLoadGroup(nsILoadGroup *aLoadGroup)
{
    mLoadGroup = aLoadGroup;
    mProgressSink = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
    NS_ENSURE_ARG_POINTER(aLoadFlags);
    *aLoadFlags = mLoadFlags;
    return NS_OK;
}
NS_IMETHODIMP
nsHttpChannel::SetLoadFlags(nsLoadFlags aLoadFlags)
{
    mLoadFlags = aLoadFlags;
    return NS_OK;
}





NS_IMETHODIMP
nsHttpChannel::GetOriginalURI(nsIURI **originalURI)
{
    NS_ENSURE_ARG_POINTER(originalURI);
    *originalURI = mOriginalURI;
    NS_ADDREF(*originalURI);
    return NS_OK;
}
NS_IMETHODIMP
nsHttpChannel::SetOriginalURI(nsIURI *originalURI)
{
    NS_ENSURE_ARG_POINTER(originalURI);
    mOriginalURI = originalURI;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetURI(nsIURI **URI)
{
    NS_ENSURE_ARG_POINTER(URI);
    *URI = mURI;
    NS_IF_ADDREF(*URI);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetOwner(nsISupports **owner)
{
    NS_ENSURE_ARG_POINTER(owner);
    *owner = mOwner;
    NS_IF_ADDREF(*owner);
    return NS_OK;
}
NS_IMETHODIMP
nsHttpChannel::SetOwner(nsISupports *owner)
{
    mOwner = owner;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetNotificationCallbacks(nsIInterfaceRequestor **callbacks)
{
    NS_IF_ADDREF(*callbacks = mCallbacks);
    return NS_OK;
}
NS_IMETHODIMP
nsHttpChannel::SetNotificationCallbacks(nsIInterfaceRequestor *callbacks)
{
    mCallbacks = callbacks;
    mProgressSink = nsnull;
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
nsHttpChannel::GetContentType(nsACString &value)
{
    if (!mResponseHead) {
        
        value.Truncate();
        return NS_ERROR_NOT_AVAILABLE;
    }

    if (!mResponseHead->ContentType().IsEmpty()) {
        value = mResponseHead->ContentType();
        return NS_OK;
    }

    
    value.AssignLiteral(UNKNOWN_CONTENT_TYPE);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetContentType(const nsACString &value)
{
    if (mListener || mWasOpened) {
        if (!mResponseHead)
            return NS_ERROR_NOT_AVAILABLE;

        nsCAutoString contentTypeBuf, charsetBuf;
        PRBool hadCharset;
        net_ParseContentType(value, contentTypeBuf, charsetBuf, &hadCharset);

        mResponseHead->SetContentType(contentTypeBuf);

        
        if (hadCharset)
            mResponseHead->SetContentCharset(charsetBuf);
    } else {
        
        PRBool dummy;
        net_ParseContentType(value, mContentTypeHint, mContentCharsetHint,
                             &dummy);
    }
    
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetContentCharset(nsACString &value)
{
    if (!mResponseHead)
        return NS_ERROR_NOT_AVAILABLE;

    value = mResponseHead->ContentCharset();
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetContentCharset(const nsACString &value)
{
    if (mListener) {
        if (!mResponseHead)
            return NS_ERROR_NOT_AVAILABLE;

        mResponseHead->SetContentCharset(value);
    } else {
        
        mContentCharsetHint = value;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetContentLength(PRInt32 *value)
{
    NS_ENSURE_ARG_POINTER(value);

    if (!mResponseHead)
        return NS_ERROR_NOT_AVAILABLE;

    
    LL_L2I(*value, mResponseHead->ContentLength());
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetContentLength(PRInt32 value)
{
    NS_NOTYETIMPLEMENTED("nsHttpChannel::SetContentLength");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::Open(nsIInputStream **_retval)
{
    NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_IN_PROGRESS);
    return NS_ImplementChannelOpen(this, _retval);
}

NS_IMETHODIMP
nsHttpChannel::AsyncOpen(nsIStreamListener *listener, nsISupports *context)
{
    LOG(("nsHttpChannel::AsyncOpen [this=%x]\n", this));

    NS_ENSURE_ARG_POINTER(listener);
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);
    NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);

    nsresult rv;

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
    if (cookieHeader)
        mUserSetCookieHeader = cookieHeader;

    
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
    }
    return NS_OK;
}




NS_IMETHODIMP
nsHttpChannel::GetRequestMethod(nsACString &method)
{
    method = mRequestHead.Method();
    return NS_OK;
}
NS_IMETHODIMP
nsHttpChannel::SetRequestMethod(const nsACString &method)
{
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);

    const nsCString &flatMethod = PromiseFlatCString(method);

    
    if (!nsHttp::IsValidToken(flatMethod))
        return NS_ERROR_INVALID_ARG;

    nsHttpAtom atom = nsHttp::ResolveAtom(flatMethod.get());
    if (!atom)
        return NS_ERROR_FAILURE;

    mRequestHead.SetMethod(atom);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetReferrer(nsIURI **referrer)
{
    NS_ENSURE_ARG_POINTER(referrer);
    *referrer = mReferrer;
    NS_IF_ADDREF(*referrer);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetReferrer(nsIURI *referrer)
{
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);

    
    mReferrer = nsnull;
    mRequestHead.ClearHeader(nsHttp::Referer);

    if (!referrer)
        return NS_OK;

    
    PRUint32 referrerLevel;
    if (mLoadFlags & LOAD_INITIAL_DOCUMENT_URI)
        referrerLevel = 1; 
    else
        referrerLevel = 2; 
    if (gHttpHandler->ReferrerLevel() < referrerLevel)
        return NS_OK;

    nsCOMPtr<nsIURI> referrerGrip;
    nsresult rv;
    PRBool match;

    
    
    
    
    
    
    
    
    rv = referrer->SchemeIs("wyciwyg", &match);
    if (NS_FAILED(rv)) return rv;
    if (match) {
        nsCAutoString path;
        rv = referrer->GetPath(path);
        if (NS_FAILED(rv)) return rv;

        PRUint32 pathLength = path.Length();
        if (pathLength <= 2) return NS_ERROR_FAILURE;

        
        
        
        PRInt32 slashIndex = path.FindChar('/', 2);
        if (slashIndex == kNotFound) return NS_ERROR_FAILURE;

        
        nsCAutoString charset;
        referrer->GetOriginCharset(charset);

        
        rv = NS_NewURI(getter_AddRefs(referrerGrip),
                       Substring(path, slashIndex + 1, pathLength - slashIndex - 1),
                       charset.get());
        if (NS_FAILED(rv)) return rv;

        referrer = referrerGrip.get();
    }

    
    
    
    static const char *const referrerWhiteList[] = {
        "http",
        "https",
        "ftp",
        "gopher",
        nsnull
    };
    match = PR_FALSE;
    const char *const *scheme = referrerWhiteList;
    for (; *scheme && !match; ++scheme) {
        rv = referrer->SchemeIs(*scheme, &match);
        if (NS_FAILED(rv)) return rv;
    }
    if (!match)
        return NS_OK; 

    
    
    
    
    
    
    rv = referrer->SchemeIs("https", &match);
    if (NS_FAILED(rv)) return rv;
    if (match) {
        rv = mURI->SchemeIs("https", &match);
        if (NS_FAILED(rv)) return rv;
        if (!match)
            return NS_OK;

        if (!gHttpHandler->SendSecureXSiteReferrer()) {
            nsCAutoString referrerHost;
            nsCAutoString host;

            rv = referrer->GetAsciiHost(referrerHost);
            if (NS_FAILED(rv)) return rv;

            rv = mURI->GetAsciiHost(host);
            if (NS_FAILED(rv)) return rv;

            
            if (!referrerHost.Equals(host))
                return NS_OK;
        }
    }

    nsCOMPtr<nsIURI> clone;
    
    
    
    
    
    rv = referrer->Clone(getter_AddRefs(clone));
    if (NS_FAILED(rv)) return rv;

    
    clone->SetUserPass(EmptyCString());

    
    nsCOMPtr<nsIURL> url = do_QueryInterface(clone);
    if (url)
        url->SetRef(EmptyCString());

    nsCAutoString spec;
    rv = clone->GetAsciiSpec(spec);
    if (NS_FAILED(rv)) return rv;

    
    mReferrer = clone;
    mRequestHead.SetHeader(nsHttp::Referer, spec);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetRequestHeader(const nsACString &header, nsACString &value)
{
    
    

    nsHttpAtom atom = nsHttp::ResolveAtom(header);
    if (!atom)
        return NS_ERROR_NOT_AVAILABLE;

    return mRequestHead.GetHeader(atom, value);
}

NS_IMETHODIMP
nsHttpChannel::SetRequestHeader(const nsACString &header,
                                const nsACString &value,
                                PRBool merge)
{
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);

    const nsCString &flatHeader = PromiseFlatCString(header);
    const nsCString &flatValue  = PromiseFlatCString(value);

    LOG(("nsHttpChannel::SetRequestHeader [this=%x header=\"%s\" value=\"%s\" merge=%u]\n",
        this, flatHeader.get(), flatValue.get(), merge));

    
    if (!nsHttp::IsValidToken(flatHeader))
        return NS_ERROR_INVALID_ARG;
    
    
    
    
    
    
    if (flatValue.FindCharInSet("\r\n") != kNotFound ||
        flatValue.Length() != strlen(flatValue.get()))
        return NS_ERROR_INVALID_ARG;

    nsHttpAtom atom = nsHttp::ResolveAtom(flatHeader.get());
    if (!atom) {
        NS_WARNING("failed to resolve atom");
        return NS_ERROR_NOT_AVAILABLE;
    }

    return mRequestHead.SetHeader(atom, flatValue, merge);
}

NS_IMETHODIMP
nsHttpChannel::VisitRequestHeaders(nsIHttpHeaderVisitor *visitor)
{
    return mRequestHead.Headers().VisitHeaders(visitor);
}

NS_IMETHODIMP
nsHttpChannel::GetUploadStream(nsIInputStream **stream)
{
    NS_ENSURE_ARG_POINTER(stream);
    *stream = mUploadStream;
    NS_IF_ADDREF(*stream);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetUploadStream(nsIInputStream *stream, const nsACString &contentType, PRInt32 contentLength)
{
    
    
    
    
    
    
    
    if (stream) {
        if (!contentType.IsEmpty()) {
            if (contentLength < 0) {
                stream->Available((PRUint32 *) &contentLength);
                if (contentLength < 0) {
                    NS_ERROR("unable to determine content length");
                    return NS_ERROR_FAILURE;
                }
            }
            mRequestHead.SetHeader(nsHttp::Content_Length, nsPrintfCString("%d", contentLength));
            mRequestHead.SetHeader(nsHttp::Content_Type, contentType);
            mUploadStreamHasHeaders = PR_FALSE;
            mRequestHead.SetMethod(nsHttp::Put); 
        }
        else {
            mUploadStreamHasHeaders = PR_TRUE;
            mRequestHead.SetMethod(nsHttp::Post); 
        }
    }
    else {
        mUploadStreamHasHeaders = PR_FALSE;
        mRequestHead.SetMethod(nsHttp::Get); 
    }
    mUploadStream = stream;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetResponseStatus(PRUint32 *value)
{
    NS_ENSURE_ARG_POINTER(value);
    if (!mResponseHead)
        return NS_ERROR_NOT_AVAILABLE;
    *value = mResponseHead->Status();
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetResponseStatusText(nsACString &value)
{
    if (!mResponseHead)
        return NS_ERROR_NOT_AVAILABLE;
    value = mResponseHead->StatusText();
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetRequestSucceeded(PRBool *value)
{
    NS_PRECONDITION(value, "Don't ever pass a null arg to this function");
    if (!mResponseHead)
        return NS_ERROR_NOT_AVAILABLE;
    PRUint32 status = mResponseHead->Status();
    *value = (status / 100 == 2);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetResponseHeader(const nsACString &header, nsACString &value)
{
    if (!mResponseHead)
        return NS_ERROR_NOT_AVAILABLE;
    nsHttpAtom atom = nsHttp::ResolveAtom(header);
    if (!atom)
        return NS_ERROR_NOT_AVAILABLE;
    return mResponseHead->GetHeader(atom, value);
}

NS_IMETHODIMP
nsHttpChannel::SetResponseHeader(const nsACString &header,
                                 const nsACString &value,
                                 PRBool merge)
{
    LOG(("nsHttpChannel::SetResponseHeader [this=%x header=\"%s\" value=\"%s\" merge=%u]\n",
        this, PromiseFlatCString(header).get(), PromiseFlatCString(value).get(), merge));

    if (!mResponseHead)
        return NS_ERROR_NOT_AVAILABLE;
    nsHttpAtom atom = nsHttp::ResolveAtom(header);
    if (!atom)
        return NS_ERROR_NOT_AVAILABLE;

    
    if (atom == nsHttp::Content_Type ||
        atom == nsHttp::Content_Length ||
        atom == nsHttp::Content_Encoding ||
        atom == nsHttp::Trailer ||
        atom == nsHttp::Transfer_Encoding)
        return NS_ERROR_ILLEGAL_VALUE;

    mResponseHeadersModified = PR_TRUE;

    return mResponseHead->SetHeader(atom, value, merge);
}

NS_IMETHODIMP
nsHttpChannel::VisitResponseHeaders(nsIHttpHeaderVisitor *visitor)
{
    if (!mResponseHead)
        return NS_ERROR_NOT_AVAILABLE;
    return mResponseHead->Headers().VisitHeaders(visitor);
}

NS_IMETHODIMP
nsHttpChannel::IsNoStoreResponse(PRBool *value)
{
    if (!mResponseHead)
        return NS_ERROR_NOT_AVAILABLE;
    *value = mResponseHead->NoStore();
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::IsNoCacheResponse(PRBool *value)
{
    if (!mResponseHead)
        return NS_ERROR_NOT_AVAILABLE;
    *value = mResponseHead->NoCache();
    if (!*value)
        *value = mResponseHead->ExpiresInPast();
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetApplyConversion(PRBool *value)
{
    NS_ENSURE_ARG_POINTER(value);
    *value = mApplyConversion;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetApplyConversion(PRBool value)
{
    LOG(("nsHttpChannel::SetApplyConversion [this=%x value=%d]\n", this, value));
    mApplyConversion = value;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetAllowPipelining(PRBool *value)
{
    NS_ENSURE_ARG_POINTER(value);
    *value = mAllowPipelining;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetAllowPipelining(PRBool value)
{
    if (mIsPending)
        return NS_ERROR_FAILURE;
    mAllowPipelining = value;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetRedirectionLimit(PRUint32 *value)
{
    NS_ENSURE_ARG_POINTER(value);
    *value = PRUint32(mRedirectionLimit);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetRedirectionLimit(PRUint32 value)
{
    mRedirectionLimit = PR_MIN(value, 0xff);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetContentEncodings(nsIUTF8StringEnumerator** aEncodings)
{
    NS_PRECONDITION(aEncodings, "Null out param");
    if (!mResponseHead) {
        *aEncodings = nsnull;
        return NS_OK;
    }
    
    const char *encoding = mResponseHead->PeekHeader(nsHttp::Content_Encoding);
    if (!encoding) {
        *aEncodings = nsnull;
        return NS_OK;
    }
    nsContentEncodings* enumerator = new nsContentEncodings(this, encoding);
    if (!enumerator)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aEncodings = enumerator);
    return NS_OK;
}





NS_IMETHODIMP
nsHttpChannel::GetDocumentURI(nsIURI **aDocumentURI)
{
    NS_ENSURE_ARG_POINTER(aDocumentURI);
    *aDocumentURI = mDocumentURI;
    NS_IF_ADDREF(*aDocumentURI);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetDocumentURI(nsIURI *aDocumentURI)
{
    mDocumentURI = aDocumentURI;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetForceAllowThirdPartyCookie(PRBool *aForceAllowThirdPartyCookie)
{
    *aForceAllowThirdPartyCookie = mForceAllowThirdPartyCookie;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetForceAllowThirdPartyCookie(PRBool aForceAllowThirdPartyCookie)
{
    mForceAllowThirdPartyCookie = aForceAllowThirdPartyCookie;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetRequestVersion(PRUint32 *major, PRUint32 *minor)
{
  int version = mRequestHead.Version();

  if (major) { *major = version / 10; }
  if (minor) { *minor = version % 10; }

  return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetResponseVersion(PRUint32 *major, PRUint32 *minor)
{
  if (!mResponseHead)
  {
    *major = *minor = 0;                   
    return NS_ERROR_NOT_AVAILABLE;
  }

  int version = mResponseHead->Version();

  if (major) { *major = version / 10; }
  if (minor) { *minor = version % 10; }

  return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetCookie(const char *aCookieHeader)
{
    if (mLoadFlags & LOAD_ANONYMOUS) {
      return NS_OK;
    }

    
    if (!(aCookieHeader && *aCookieHeader))
        return NS_OK;

    nsICookieService *cs = gHttpHandler->GetCookieService();
    NS_ENSURE_TRUE(cs, NS_ERROR_FAILURE);

    nsCOMPtr<nsIPrompt> prompt;
    GetCallback(prompt);

    return cs->SetCookieStringFromHttp(mURI,
                                       mDocumentURI ? mDocumentURI : mOriginalURI,
                                       prompt,
                                       aCookieHeader,
                                       mResponseHead->PeekHeader(nsHttp::Date),
                                       this);
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
nsHttpChannel::GetPriority(PRInt32 *value)
{
    *value = mPriority;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetPriority(PRInt32 value)
{
    PRInt16 newValue = CLAMP(value, PR_INT16_MIN, PR_INT16_MAX);
    if (mPriority == newValue)
        return NS_OK;
    mPriority = newValue;
    if (mTransaction)
        gHttpHandler->RescheduleTransaction(mTransaction, mPriority);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::AdjustPriority(PRInt32 delta)
{
    return SetPriority(mPriority + delta);
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
nsHttpChannel::OnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
    if (!(mCanceled || NS_FAILED(mStatus))) {
        
        
        request->GetStatus(&mStatus);
    }

    LOG(("nsHttpChannel::OnStartRequest [this=%x request=%x status=%x]\n",
        this, request, mStatus));

    
    NS_ASSERTION(request == mCachePump || request == mTransactionPump,
                 "Unexpected request");
    NS_ASSERTION(!(mTransactionPump && mCachePump) || mCachedContentIsPartial,
                 "If we have both pumps, the cache content must be partial");

    if (!mSecurityInfo && !mCachePump && mTransaction) {
        
        
        mSecurityInfo = mTransaction->SecurityInfo();
    }

    
    if (NS_SUCCEEDED(mStatus) && !mCachePump && mTransaction) {
        NS_ASSERTION(mResponseHead == nsnull, "leaking mResponseHead");

        
        
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
        if (NS_SUCCEEDED(ProxyFailover()))
            return NS_OK;
    }

    
    PRBool fallingBack;
    if (NS_FAILED(mStatus) &&
        NS_SUCCEEDED(ProcessFallback(&fallingBack)) &&
        fallingBack) {

        return NS_OK;
    }

    return CallOnStartRequest();
}

NS_IMETHODIMP
nsHttpChannel::OnStopRequest(nsIRequest *request, nsISupports *ctxt, nsresult status)
{
    LOG(("nsHttpChannel::OnStopRequest [this=%x request=%x status=%x]\n",
        this, request, status));

    
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

        
        NS_RELEASE(mTransaction);
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

    
    if (mCacheEntry && (mCacheAccess & nsICache::ACCESS_WRITE))
        FinalizeCacheEntry();
    
    if (mListener) {
        LOG(("  calling OnStopRequest\n"));
        mListener->OnStopRequest(this, mListenerContext, status);
        mListener = 0;
        mListenerContext = 0;
    }

    if (mCacheEntry)
        CloseCacheEntry(PR_TRUE);

    if (mOfflineCacheEntry)
        CloseOfflineCacheEntry();

    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nsnull, status);

    mCallbacks = nsnull;
    mProgressSink = nsnull;
    
    return NS_OK;
}





NS_IMETHODIMP
nsHttpChannel::OnDataAvailable(nsIRequest *request, nsISupports *ctxt,
                               nsIInputStream *input,
                               PRUint32 offset, PRUint32 count)
{
    LOG(("nsHttpChannel::OnDataAvailable [this=%x request=%x offset=%u count=%u]\n",
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
        LOG(("sending status notification [this=%x status=%x progress=%llu/%llu]\n",
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
        LOG(("skipping status notification [this=%x sink=%x pending=%u background=%x]\n",
            this, mProgressSink.get(), mIsPending, (mLoadFlags & LOAD_BACKGROUND)));
#endif

    return NS_OK;
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

NS_IMETHODIMP
nsHttpChannel::GetCacheKey(nsISupports **key)
{
    nsresult rv;
    NS_ENSURE_ARG_POINTER(key);

    LOG(("nsHttpChannel::GetCacheKey [this=%x]\n", this));

    *key = nsnull;

    nsCOMPtr<nsISupportsPRUint32> container =
        do_CreateInstance(NS_SUPPORTS_PRUINT32_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = container->SetData(mPostID);
    if (NS_FAILED(rv)) return rv;

    return CallQueryInterface(container, key);
}

NS_IMETHODIMP
nsHttpChannel::SetCacheKey(nsISupports *key)
{
    nsresult rv;

    LOG(("nsHttpChannel::SetCacheKey [this=%x key=%x]\n", this, key));

    
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
nsHttpChannel::IsFromCache(PRBool *value)
{
    if (!mIsPending)
        return NS_ERROR_NOT_AVAILABLE;

    
    

    *value = (mCachePump || (mLoadFlags & LOAD_ONLY_IF_MODIFIED)) &&
              mCachedContentIsValid && !mCachedContentIsPartial;

    return NS_OK;
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
nsHttpChannel::GetEntityID(nsACString& aEntityID)
{
    
    
    if (mRequestHead.Method() != nsHttp::Get) {
        return NS_ERROR_NOT_RESUMABLE;
    }

    
    
    
    
    const char* acceptRanges =
        mResponseHead->PeekHeader(nsHttp::Accept_Ranges);
    if (acceptRanges &&
        !nsHttp::FindToken(acceptRanges, "bytes", HTTP_HEADER_VALUE_SEPS)) {
        return NS_ERROR_NOT_RESUMABLE;
    }

    PRUint64 size = LL_MAXUINT;
    nsCAutoString etag, lastmod;
    if (mResponseHead) {
        size = mResponseHead->TotalEntitySize();
        const char* cLastMod = mResponseHead->PeekHeader(nsHttp::Last_Modified);
        if (cLastMod)
            lastmod = cLastMod;
        const char* cEtag = mResponseHead->PeekHeader(nsHttp::ETag);
        if (cEtag)
            etag = cEtag;
    }
    nsCString entityID;
    NS_EscapeURL(etag.BeginReading(), etag.Length(), esc_AlwaysCopy |
            esc_FileBaseName | esc_Forced, entityID);
    entityID.Append('/');
    entityID.AppendInt(PRInt64(size));
    entityID.Append('/');
    entityID.Append(lastmod);
    

    aEntityID = entityID;

    return NS_OK;
}





NS_IMETHODIMP
nsHttpChannel::OnCacheEntryAvailable(nsICacheEntryDescriptor *entry,
                                     nsCacheAccessMode access,
                                     nsresult status)
{
    LOG(("nsHttpChannel::OnCacheEntryAvailable [this=%x entry=%x "
         "access=%x status=%x]\n", this, entry, access, status));

    
    
    if (!mIsPending)
        return NS_OK;

    
    if (NS_SUCCEEDED(status)) {
        mCacheEntry = entry;
        mCacheAccess = access;
    }

    nsresult rv;

    if (mCanceled && NS_FAILED(mStatus)) {
        LOG(("channel was canceled [this=%x status=%x]\n", this, mStatus));
        rv = mStatus;
    }
    else if ((mLoadFlags & LOAD_ONLY_FROM_CACHE) && NS_FAILED(status))
        
        
        rv = NS_ERROR_DOCUMENT_NOT_CACHED;
    else
        
        rv = Connect(PR_FALSE);

    
    if (NS_FAILED(rv)) {
        CloseCacheEntry(PR_TRUE);
        AsyncAbort(rv);
    }

    return NS_OK;
}

nsresult
nsHttpChannel::DoAuthRetry(nsAHttpConnection *conn)
{
    LOG(("nsHttpChannel::DoAuthRetry [this=%x]\n", this));

    NS_ASSERTION(!mTransaction, "should not have a transaction");
    nsresult rv;

    
    
    mIsPending = PR_FALSE;

    
    
    
    AddCookiesToRequest();

    
    gHttpHandler->OnModifyRequest(this);

    mIsPending = PR_TRUE;

    
    delete mResponseHead;
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

    return mTransactionPump->AsyncRead(this, nsnull);
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





nsHttpChannel::nsContentEncodings::nsContentEncodings(nsIHttpChannel* aChannel,
                                                          const char* aEncodingHeader) :
    mEncodingHeader(aEncodingHeader), mChannel(aChannel), mReady(PR_FALSE)
{
    mCurEnd = aEncodingHeader + strlen(aEncodingHeader);
    mCurStart = mCurEnd;
}
    
nsHttpChannel::nsContentEncodings::~nsContentEncodings()
{
}





NS_IMETHODIMP
nsHttpChannel::nsContentEncodings::HasMore(PRBool* aMoreEncodings)
{
    if (mReady) {
        *aMoreEncodings = PR_TRUE;
        return NS_OK;
    }
    
    nsresult rv = PrepareForNext();
    *aMoreEncodings = NS_SUCCEEDED(rv);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::nsContentEncodings::GetNext(nsACString& aNextEncoding)
{
    aNextEncoding.Truncate();
    if (!mReady) {
        nsresult rv = PrepareForNext();
        if (NS_FAILED(rv)) {
            return NS_ERROR_FAILURE;
        }
    }

    const nsACString & encoding = Substring(mCurStart, mCurEnd);

    nsACString::const_iterator start, end;
    encoding.BeginReading(start);
    encoding.EndReading(end);

    PRBool haveType = PR_FALSE;
    if (CaseInsensitiveFindInReadable(NS_LITERAL_CSTRING("gzip"),
                                      start,
                                      end)) {
        aNextEncoding.AssignLiteral(APPLICATION_GZIP);
        haveType = PR_TRUE;
    }

    if (!haveType) {
        encoding.BeginReading(start);
        if (CaseInsensitiveFindInReadable(NS_LITERAL_CSTRING("compress"),
                                          start,
                                          end)) {
            aNextEncoding.AssignLiteral(APPLICATION_COMPRESS);
                                           
            haveType = PR_TRUE;
        }
    }
    
    if (! haveType) {
        encoding.BeginReading(start);
        if (CaseInsensitiveFindInReadable(NS_LITERAL_CSTRING("deflate"),
                                          start,
                                          end)) {
            aNextEncoding.AssignLiteral(APPLICATION_ZIP);
            haveType = PR_TRUE;
        }
    }

    
    mCurEnd = mCurStart;
    mReady = PR_FALSE;
    
    if (haveType)
        return NS_OK;

    NS_WARNING("Unknown encoding type");
    return NS_ERROR_FAILURE;
}





NS_IMPL_ISUPPORTS1(nsHttpChannel::nsContentEncodings, nsIUTF8StringEnumerator)





nsresult
nsHttpChannel::nsContentEncodings::PrepareForNext(void)
{
    NS_PRECONDITION(mCurStart == mCurEnd, "Indeterminate state");
    
    
    
    
    while (mCurEnd != mEncodingHeader) {
        --mCurEnd;
        if (*mCurEnd != ',' && !nsCRT::IsAsciiSpace(*mCurEnd))
            break;
    }
    if (mCurEnd == mEncodingHeader)
        return NS_ERROR_NOT_AVAILABLE; 
    ++mCurEnd;
        
    
    
    
    mCurStart = mCurEnd - 1;
    while (mCurStart != mEncodingHeader &&
           *mCurStart != ',' && !nsCRT::IsAsciiSpace(*mCurStart))
        --mCurStart;
    if (*mCurStart == ',' || nsCRT::IsAsciiSpace(*mCurStart))
        ++mCurStart; 
        
    
    
    if (Substring(mCurStart, mCurEnd).Equals("identity",
                                             nsCaseInsensitiveCStringComparator())) {
        mCurEnd = mCurStart;
        return PrepareForNext();
    }
        
    mReady = PR_TRUE;
    return NS_OK;
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
        
    
    
    
    
    
    LOG(("MaybeInvalidateCacheEntryForSubsequentGet [this=%x]\n", this));

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

void
nsHttpChannel::AsyncOnExamineCachedResponse()
{
    gHttpHandler->OnExamineCachedResponse(this);
}
