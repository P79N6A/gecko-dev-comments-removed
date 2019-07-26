






#include "HttpLog.h"

#include "nsHttp.h"
#include "nsHttpChannel.h"
#include "nsHttpHandler.h"
#include "nsStandardURL.h"
#include "nsIApplicationCacheService.h"
#include "nsIApplicationCacheContainer.h"
#include "nsICacheStorageService.h"
#include "nsICacheStorage.h"
#include "nsICacheEntry.h"
#include "nsIAuthInformation.h"
#include "nsICryptoHash.h"
#include "nsIStringBundle.h"
#include "nsIStreamListenerTee.h"
#include "nsISeekableStream.h"
#include "nsILoadGroupChild.h"
#include "nsIProtocolProxyService2.h"
#include "nsMimeTypes.h"
#include "nsNetUtil.h"
#include "prprf.h"
#include "prnetdb.h"
#include "nsEscape.h"
#include "nsStreamUtils.h"
#include "nsIOService.h"
#include "nsDNSPrefetch.h"
#include "nsChannelClassifier.h"
#include "nsIRedirectResultListener.h"
#include "mozilla/TimeStamp.h"
#include "nsError.h"
#include "nsPrintfCString.h"
#include "nsAlgorithm.h"
#include "GeckoProfiler.h"
#include "nsIConsoleService.h"
#include "NullHttpTransaction.h"
#include "mozilla/Attributes.h"
#include "mozilla/VisualEventTracer.h"
#include "nsISSLSocketControl.h"
#include "sslt.h"
#include "nsContentUtils.h"
#include "nsIPermissionManager.h"
#include "nsIPrincipal.h"
#include "nsISecurityConsoleMessage.h"
#include "nsIScriptSecurityManager.h"
#include "nsISSLStatus.h"
#include "nsISSLStatusProvider.h"
#include "nsIDOMWindow.h"
#include "LoadContextInfo.h"

namespace mozilla { namespace net {

namespace {


const char kDiskDeviceID[] = "disk";
const char kMemoryDeviceID[] = "memory";
const char kOfflineDeviceID[] = "offline";


#define BYPASS_LOCAL_CACHE(loadFlags) \
        (loadFlags & (nsIRequest::LOAD_BYPASS_CACHE | \
                      nsICachingChannel::LOAD_BYPASS_LOCAL_CACHE))

static NS_DEFINE_CID(kStreamListenerTeeCID, NS_STREAMLISTENERTEE_CID);
static NS_DEFINE_CID(kStreamTransportServiceCID,
                     NS_STREAMTRANSPORTSERVICE_CID);

enum CacheDisposition {
    kCacheHit = 1,
    kCacheHitViaReval = 2,
    kCacheMissedViaReval = 3,
    kCacheMissed = 4
};

const Telemetry::ID UNKNOWN_DEVICE = static_cast<Telemetry::ID>(0);
void
AccumulateCacheHitTelemetry(Telemetry::ID deviceHistogram,
                            CacheDisposition hitOrMiss)
{
    
    
    
    

    Telemetry::Accumulate(Telemetry::HTTP_CACHE_DISPOSITION_2, hitOrMiss);
    if (deviceHistogram != UNKNOWN_DEVICE) {
        Telemetry::Accumulate(deviceHistogram, hitOrMiss);
    }
}



nsresult
Hash(const char *buf, nsACString &hash)
{
    nsresult rv;

    nsCOMPtr<nsICryptoHash> hasher
      = do_CreateInstance(NS_CRYPTO_HASH_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = hasher->Init(nsICryptoHash::SHA1);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = hasher->Update(reinterpret_cast<unsigned const char*>(buf),
                         strlen(buf));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = hasher->Finish(true, hash);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

bool IsRedirectStatus(uint32_t status)
{
    
    return status == 300 || status == 301 || status == 302 || status == 303 ||
           status == 307 || status == 308;
}



bool
WillRedirect(const nsHttpResponseHead * response)
{
    return IsRedirectStatus(response->Status()) &&
           response->PeekHeader(nsHttp::Location);
}

} 

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

    mChannel->mRedirectChannel = nullptr;

    nsCOMPtr<nsIRedirectResultListener> vetoHook;
    NS_QueryNotificationCallbacks(mChannel,
                                  NS_GET_IID(nsIRedirectResultListener),
                                  getter_AddRefs(vetoHook));

#ifdef MOZ_VISUAL_EVENT_TRACER
    nsHttpChannel* channel = mChannel;
#endif
    mChannel = nullptr;
    if (vetoHook)
        vetoHook->OnRedirectResult(succeeded);

    MOZ_EVENT_TRACER_DONE(channel, "net::http::redirect-callbacks");
}





nsHttpChannel::nsHttpChannel()
    : HttpAsyncAborter<nsHttpChannel>(MOZ_THIS_IN_INITIALIZER_LIST())
    , mLogicalOffset(0)
    , mCacheEntryDeviceTelemetryID(UNKNOWN_DEVICE)
    , mPostID(0)
    , mRequestTime(0)
    , mOfflineCacheLastModifiedTime(0)
    , mCachedContentIsValid(false)
    , mCachedContentIsPartial(false)
    , mTransactionReplaced(false)
    , mAuthRetryPending(false)
    , mProxyAuthPending(false)
    , mResuming(false)
    , mInitedCacheEntry(false)
    , mFallbackChannel(false)
    , mCustomConditionalRequest(false)
    , mFallingBack(false)
    , mWaitingForRedirectCallback(false)
    , mRequestTimeInitialized(false)
    , mCacheEntryIsReadOnly(false)
    , mCacheEntryIsWriteOnly(false)
    , mCacheEntriesToWaitFor(0)
    , mHasQueryString(0)
    , mConcurentCacheAccess(0)
    , mIsPartialRequest(0)
    , mDidReval(false)
{
    LOG(("Creating nsHttpChannel [this=%p]\n", this));
    mChannelCreationTime = PR_Now();
    mChannelCreationTimestamp = TimeStamp::Now();
}

nsHttpChannel::~nsHttpChannel()
{
    LOG(("Destroying nsHttpChannel [this=%p]\n", this));

    if (mAuthProvider)
        mAuthProvider->Disconnect(NS_ERROR_ABORT);
}

nsresult
nsHttpChannel::Init(nsIURI *uri,
                    uint32_t caps,
                    nsProxyInfo *proxyInfo,
                    uint32_t proxyResolveFlags,
                    nsIURI *proxyURI)
{
#ifdef MOZ_VISUAL_EVENT_TRACER
    nsAutoCString url;
    uri->GetAsciiSpec(url);
    MOZ_EVENT_TRACER_NAME_OBJECT(this, url.get());
#endif

    nsresult rv = HttpBaseChannel::Init(uri, caps, proxyInfo,
                                        proxyResolveFlags, proxyURI);
    if (NS_FAILED(rv))
        return rv;

    LOG(("nsHttpChannel::Init [this=%p]\n", this));

    return rv;
}




nsresult
nsHttpChannel::Connect()
{
    nsresult rv;

    LOG(("nsHttpChannel::Connect [this=%p]\n", this));

    
    
    
    
    bool usingSSL = false;
    rv = mURI->SchemeIs("https", &usingSSL);
    NS_ENSURE_SUCCESS(rv,rv);

    if (!usingSSL) {
        
        nsISiteSecurityService* sss = gHttpHandler->GetSSService();
        NS_ENSURE_TRUE(sss, NS_ERROR_OUT_OF_MEMORY);

        bool isStsHost = false;
        uint32_t flags = mPrivateBrowsing ? nsISocketProvider::NO_PERMANENT_STORAGE : 0;
        rv = sss->IsSecureURI(nsISiteSecurityService::HEADER_HSTS, mURI, flags,
                              &isStsHost);

        
        
        NS_ASSERTION(NS_SUCCEEDED(rv),
                     "Something is wrong with SSS: IsSecureURI failed.");

        if (NS_SUCCEEDED(rv) && isStsHost) {
            LOG(("nsHttpChannel::Connect() STS permissions found\n"));
            return AsyncCall(&nsHttpChannel::HandleAsyncRedirectChannelToHttps);
        }
    }

    
    if (!net_IsValidHostName(nsDependentCString(mConnectionInfo->Host())))
        return NS_ERROR_UNKNOWN_HOST;

    
    mConnectionInfo->SetAnonymous((mLoadFlags & LOAD_ANONYMOUS) != 0);
    mConnectionInfo->SetPrivate(mPrivateBrowsing);

    
    RetrieveSSLOptions();
    SpeculativeConnect();

    
    if (mResuming && (mLoadFlags & LOAD_ONLY_FROM_CACHE)) {
        LOG(("Resuming from cache is not supported yet"));
        return NS_ERROR_DOCUMENT_NOT_CACHED;
    }

    
    rv = OpenCacheEntry(usingSSL);

    
    if (mCacheEntriesToWaitFor) {
        MOZ_ASSERT(NS_SUCCEEDED(rv), "Unexpected state");
        return NS_OK;
    }

    if (NS_FAILED(rv)) {
        LOG(("OpenCacheEntry failed [rv=%x]\n", rv));
        
        
        if (mLoadFlags & LOAD_ONLY_FROM_CACHE) {
            
            
            if (!mFallbackChannel && !mFallbackKey.IsEmpty()) {
                return AsyncCall(&nsHttpChannel::HandleAsyncFallback);
            }
            return NS_ERROR_DOCUMENT_NOT_CACHED;
        }
        
    }

    return ContinueConnect();
}

nsresult
nsHttpChannel::ContinueConnect()
{
    
    if (mCacheEntry) {
        
        if (mCachedContentIsValid) {
            nsRunnableMethod<nsHttpChannel> *event = nullptr;
            if (!mCachedContentIsPartial) {
                AsyncCall(&nsHttpChannel::AsyncOnExamineCachedResponse, &event);
            }
            nsresult rv = ReadFromCache(true);
            if (NS_FAILED(rv) && event) {
                event->Revoke();
            }

            AccumulateCacheHitTelemetry(mCacheEntryDeviceTelemetryID,
                                        kCacheHit);

            return rv;
        }
        else if (mLoadFlags & LOAD_ONLY_FROM_CACHE) {
            
            
            
            
            LOG(("  !mCachedContentIsValid && mLoadFlags & LOAD_ONLY_FROM_CACHE"));
            return NS_ERROR_DOCUMENT_NOT_CACHED;
        }
    }
    else if (mLoadFlags & LOAD_ONLY_FROM_CACHE) {
        
        
        if (!mFallbackChannel && !mFallbackKey.IsEmpty()) {
            return AsyncCall(&nsHttpChannel::HandleAsyncFallback);
        }
        LOG(("  !mCachedEntry && mLoadFlags & LOAD_ONLY_FROM_CACHE"));
        return NS_ERROR_DOCUMENT_NOT_CACHED;
    }

    if (mLoadFlags & LOAD_NO_NETWORK_IO) {
        LOG(("  mLoadFlags & LOAD_NO_NETWORK_IO"));
        return NS_ERROR_DOCUMENT_NOT_CACHED;
    }

    
    nsresult rv = SetupTransaction();
    if (NS_FAILED(rv)) return rv;

    rv = gHttpHandler->InitiateTransaction(mTransaction, mPriority);
    if (NS_FAILED(rv)) return rv;

    rv = mTransactionPump->AsyncRead(this, nullptr);
    if (NS_FAILED(rv)) return rv;

    uint32_t suspendCount = mSuspendCount;
    while (suspendCount--)
        mTransactionPump->Suspend();

    return NS_OK;
}

void
nsHttpChannel::SpeculativeConnect()
{
    
    

    
    
    
    
    if (mApplicationCache || gIOService->IsOffline() ||
        mUpgradeProtocolCallback || !(mCaps & NS_HTTP_ALLOW_KEEPALIVE))
        return;

    
    
    
    if (mLoadFlags & (LOAD_ONLY_FROM_CACHE | LOAD_FROM_CACHE |
                      LOAD_NO_NETWORK_IO | LOAD_CHECK_OFFLINE_CACHE))
        return;

    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    NS_NewNotificationCallbacksAggregation(mCallbacks, mLoadGroup,
                                           getter_AddRefs(callbacks));
    if (!callbacks)
        return;

    gHttpHandler->SpeculativeConnect(
        mConnectionInfo, callbacks,
        mCaps & (NS_HTTP_ALLOW_RSA_FALSESTART | NS_HTTP_ALLOW_RC4_FALSESTART | NS_HTTP_DISALLOW_SPDY));
}

void
nsHttpChannel::DoNotifyListenerCleanup()
{
    
    CleanRedirectCacheChainIfNecessary();
}

void
nsHttpChannel::HandleAsyncRedirect()
{
    NS_PRECONDITION(!mCallOnResume, "How did that happen?");

    if (mSuspendCount) {
        LOG(("Waiting until resume to do async redirect [this=%p]\n", this));
        mCallOnResume = &nsHttpChannel::HandleAsyncRedirect;
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
            mCacheEntry->AsyncDoom(nullptr);
    }
    CloseCacheEntry(false);

    mIsPending = false;

    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nullptr, mStatus);

    return NS_OK;
}

void
nsHttpChannel::HandleAsyncNotModified()
{
    NS_PRECONDITION(!mCallOnResume, "How did that happen?");

    if (mSuspendCount) {
        LOG(("Waiting until resume to do async not-modified [this=%p]\n",
             this));
        mCallOnResume = &nsHttpChannel::HandleAsyncNotModified;
        return;
    }

    LOG(("nsHttpChannel::HandleAsyncNotModified [this=%p]\n", this));

    DoNotifyListener();

    CloseCacheEntry(true);

    mIsPending = false;

    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nullptr, mStatus);
}

void
nsHttpChannel::HandleAsyncFallback()
{
    NS_PRECONDITION(!mCallOnResume, "How did that happen?");

    if (mSuspendCount) {
        LOG(("Waiting until resume to do async fallback [this=%p]\n", this));
        mCallOnResume = &nsHttpChannel::HandleAsyncFallback;
        return;
    }

    nsresult rv = NS_OK;

    LOG(("nsHttpChannel::HandleAsyncFallback [this=%p]\n", this));

    
    
    
    if (!mCanceled) {
        PushRedirectAsyncFunc(&nsHttpChannel::ContinueHandleAsyncFallback);
        bool waitingForRedirectCallback;
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

    mIsPending = false;

    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nullptr, mStatus);

    return rv;
}

void
nsHttpChannel::SetupTransactionLoadGroupInfo()
{
    
    
    
    nsCOMPtr<nsILoadGroupChild> childLoadGroup = do_QueryInterface(mLoadGroup);
    if (!childLoadGroup)
        return;

    nsCOMPtr<nsILoadGroup> rootLoadGroup;
    childLoadGroup->GetRootLoadGroup(getter_AddRefs(rootLoadGroup));
    if (!rootLoadGroup)
        return;

    
    nsCOMPtr<nsILoadGroupConnectionInfo> ci;
    rootLoadGroup->GetConnectionInfo(getter_AddRefs(ci));
    if (ci)
        mTransaction->SetLoadGroupConnectionInfo(ci);
}

void
nsHttpChannel::RetrieveSSLOptions()
{
    if (!IsHTTPS() || mPrivateBrowsing)
        return;

    nsIPrincipal *principal = GetPrincipal();
    if (!principal)
        return;

    nsCOMPtr<nsIPermissionManager> permMgr =
        do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
    if (!permMgr)
        return;

    uint32_t perm;
    nsresult rv = permMgr->TestPermissionFromPrincipal(principal,
                                                       "falsestart-rsa", &perm);
    if (NS_SUCCEEDED(rv) && perm == nsIPermissionManager::ALLOW_ACTION) {
        LOG(("nsHttpChannel::RetrieveSSLOptions [this=%p] "
             "falsestart-rsa permission found\n", this));
        mCaps |= NS_HTTP_ALLOW_RSA_FALSESTART;
    }
    rv = permMgr->TestPermissionFromPrincipal(principal, "falsestart-rc4", &perm);
    if (NS_SUCCEEDED(rv) && perm == nsIPermissionManager::ALLOW_ACTION) {
        LOG(("nsHttpChannel::RetrieveSSLOptions [this=%p] "
             "falsestart-rc4 permission found\n", this));
        mCaps |= NS_HTTP_ALLOW_RC4_FALSESTART;
    }
}

nsresult
nsHttpChannel::SetupTransaction()
{
    LOG(("nsHttpChannel::SetupTransaction [this=%p]\n", this));

    NS_ENSURE_TRUE(!mTransaction, NS_ERROR_ALREADY_INITIALIZED);

    nsresult rv;

    if (mCaps & NS_HTTP_ALLOW_PIPELINING) {
        
        
        
        
        
        
        
        
        if (!mAllowPipelining ||
           (mLoadFlags & (LOAD_INITIAL_DOCUMENT_URI | INHIBIT_PIPELINE)) ||
            !(mRequestHead.Method() == nsHttp::Get ||
              mRequestHead.Method() == nsHttp::Head ||
              mRequestHead.Method() == nsHttp::Options ||
              mRequestHead.Method() == nsHttp::Propfind ||
              mRequestHead.Method() == nsHttp::Proppatch)) {
            LOG(("  pipelining disallowed\n"));
            mCaps &= ~NS_HTTP_ALLOW_PIPELINING;
        }
    }

    if (!mAllowSpdy)
        mCaps |= NS_HTTP_DISALLOW_SPDY;

    
    
    nsAutoCString buf, path;
    nsCString* requestURI;
    if (mConnectionInfo->UsingConnect() ||
        !mConnectionInfo->UsingHttpProxy()) {
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

    
    int32_t ref = requestURI->FindChar('#');
    if (ref != kNotFound)
        requestURI->SetLength(ref);

    mRequestHead.SetRequestURI(*requestURI);

    
    mRequestTime = NowInSeconds();
    mRequestTimeInitialized = true;

    
    if (mLoadFlags & LOAD_BYPASS_CACHE) {
        
        
        
        mRequestHead.SetHeaderOnce(nsHttp::Pragma, "no-cache", true);
        
        
        if (mRequestHead.Version() >= NS_HTTP_VERSION_1_1)
            mRequestHead.SetHeaderOnce(nsHttp::Cache_Control, "no-cache", true);
    }
    else if ((mLoadFlags & VALIDATE_ALWAYS) && !mCacheEntryIsWriteOnly) {
        
        
        
        
        
        if (mRequestHead.Version() >= NS_HTTP_VERSION_1_1)
            mRequestHead.SetHeaderOnce(nsHttp::Cache_Control, "max-age=0", true);
        else
            mRequestHead.SetHeaderOnce(nsHttp::Pragma, "no-cache", true);
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
                nsAutoCString ifMatch;
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

    if (mTimingEnabled)
        mCaps |= NS_HTTP_TIMING_ENABLED;

    if (mUpgradeProtocolCallback) {
        mRequestHead.SetHeader(nsHttp::Upgrade, mUpgradeProtocol, false);
        mRequestHead.SetHeaderOnce(nsHttp::Connection,
                                   nsHttp::Upgrade.get(),
                                   true);
        mCaps |=  NS_HTTP_STICKY_CONNECTION;
        mCaps &= ~NS_HTTP_ALLOW_PIPELINING;
        mCaps &= ~NS_HTTP_ALLOW_KEEPALIVE;
        mCaps |=  NS_HTTP_DISALLOW_SPDY;
    }

    nsCOMPtr<nsIAsyncInputStream> responseStream;
    rv = mTransaction->Init(mCaps, mConnectionInfo, &mRequestHead,
                            mUploadStream, mUploadStreamHasHeaders,
                            NS_GetCurrentThread(), callbacks, this,
                            getter_AddRefs(responseStream));
    if (NS_FAILED(rv)) {
        mTransaction = nullptr;
        return rv;
    }

    SetupTransactionLoadGroupInfo();

    rv = nsInputStreamPump::Create(getter_AddRefs(mTransactionPump),
                                   responseStream);
    return rv;
}



static void
CallTypeSniffers(void *aClosure, const uint8_t *aData, uint32_t aCount)
{
  nsIChannel *chan = static_cast<nsIChannel*>(aClosure);

  nsAutoCString newType;
  NS_SniffContent(NS_CONTENT_SNIFFER_CATEGORY, chan, aData, aCount, newType);
  if (!newType.IsEmpty()) {
    chan->SetContentType(newType);
  }
}

nsresult
nsHttpChannel::CallOnStartRequest()
{
    mTracingEnabled = false;

    
    if (mLoadFlags & LOAD_CALL_CONTENT_SNIFFERS) {
        
        
        
        

        nsIChannel* thisChannel = static_cast<nsIChannel*>(this);

        bool typeSniffersCalled = false;
        if (mCachePump) {
          typeSniffersCalled =
            NS_SUCCEEDED(mCachePump->PeekStream(CallTypeSniffers, thisChannel));
        }

        if (!typeSniffersCalled && mTransactionPump) {
          mTransactionPump->PeekStream(CallTypeSniffers, thisChannel);
        }
    }

    bool shouldSniff = mResponseHead && (mResponseHead->ContentType().IsEmpty() ||
        ((mResponseHead->ContentType().EqualsLiteral(APPLICATION_OCTET_STREAM) &&
        (mLoadFlags & LOAD_TREAT_APPLICATION_OCTET_STREAM_AS_UNKNOWN))));

    if (shouldSniff) {
        MOZ_ASSERT(mConnectionInfo, "Should have connection info here");
        if (!mContentTypeHint.IsEmpty())
            mResponseHead->SetContentType(mContentTypeHint);
        else if (mResponseHead->Version() == NS_HTTP_VERSION_0_9 &&
                 mConnectionInfo->Port() != mConnectionInfo->DefaultPort())
            mResponseHead->SetContentType(NS_LITERAL_CSTRING(TEXT_PLAIN));
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

    if (mResponseHead && mCacheEntry) {
        
        
        nsresult rv = mCacheEntry->SetPredictedDataSize(
            mResponseHead->ContentLength());
        NS_ENSURE_SUCCESS(rv, rv);
    }

    LOG(("  calling mListener->OnStartRequest\n"));
    nsresult rv;
    if (mListener) {
        nsresult rv = mListener->OnStartRequest(this, mListenerContext);
        if (NS_FAILED(rv))
            return rv;
    } else {
        NS_WARNING("OnStartRequest skipped because of null listener");
    }

    
    rv = ApplyContentConversions();
    if (NS_FAILED(rv)) return rv;

    rv = EnsureAssocReq();
    if (NS_FAILED(rv))
        return rv;

    
    if (mCacheEntry && mChannelIsForDownload) {
        mCacheEntry->AsyncDoom(nullptr);
        CloseCacheEntry(false);
    }

    if (!mCanceled) {
        
        if (ShouldUpdateOfflineCacheEntry()) {
            LOG(("writing to the offline cache"));
            rv = InitOfflineCacheEntry();
            if (NS_FAILED(rv)) return rv;

            
            if (mOfflineCacheEntry) {
                rv = InstallOfflineCacheListener();
                if (NS_FAILED(rv)) return rv;
            }
        } else if (mApplicationCacheForWrite) {
            LOG(("offline cache is up to date, not updating"));
            CloseOfflineCacheEntry();
        }
    }

    return NS_OK;
}

nsresult
nsHttpChannel::ProcessFailedProxyConnect(uint32_t httpStatus)
{
    
    
    
    
    
    
    
    
    
    

    MOZ_ASSERT(mConnectionInfo->UsingConnect(),
               "proxy connect failed but not using CONNECT?");
    nsresult rv;
    switch (httpStatus)
    {
    case 300: case 301: case 302: case 303: case 307: case 308:
        
        
        
        
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
    LOG(("Cancelling failed proxy CONNECT [this=%p httpStatus=%u]\n",
         this, httpStatus));
    Cancel(rv);
    CallOnStartRequest();
    return rv;
}








nsresult
nsHttpChannel::ProcessSTSHeader()
{
    nsresult rv;
    bool isHttps = false;
    rv = mURI->SchemeIs("https", &isHttps);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    if (!isHttps)
        return NS_OK;

    nsAutoCString asciiHost;
    rv = mURI->GetAsciiHost(asciiHost);
    NS_ENSURE_SUCCESS(rv, NS_OK);

    
    
    PRNetAddr hostAddr;
    if (PR_SUCCESS == PR_StringToNetAddr(asciiHost.get(), &hostAddr))
        return NS_OK;

    nsISiteSecurityService* sss = gHttpHandler->GetSSService();
    NS_ENSURE_TRUE(sss, NS_ERROR_OUT_OF_MEMORY);

    
    
    
    NS_ENSURE_TRUE(mSecurityInfo, NS_OK);

    
    
    
    bool tlsIsBroken = false;
    rv = sss->ShouldIgnoreHeaders(mSecurityInfo, &tlsIsBroken);
    NS_ENSURE_SUCCESS(rv, NS_OK);

    
    
    
    
    
    bool wasAlreadySTSHost;
    uint32_t flags =
      NS_UsePrivateBrowsing(this) ? nsISocketProvider::NO_PERMANENT_STORAGE : 0;
    rv = sss->IsSecureURI(nsISiteSecurityService::HEADER_HSTS, mURI, flags,
                          &wasAlreadySTSHost);
    
    
    NS_ENSURE_SUCCESS(rv, NS_OK);
    MOZ_ASSERT(!(wasAlreadySTSHost && tlsIsBroken),
               "connection should have been aborted by nss-bad-cert-handler");

    
    
    
    if (tlsIsBroken) {
        LOG(("STS: Transport layer is not trustworthy, ignoring "
             "STS headers and continuing load\n"));
        return NS_OK;
    }

    
    
    const nsHttpAtom atom = nsHttp::ResolveAtom("Strict-Transport-Security");
    nsAutoCString stsHeader;
    rv = mResponseHead->GetHeader(atom, stsHeader);
    if (rv == NS_ERROR_NOT_AVAILABLE) {
        LOG(("STS: No STS header, continuing load.\n"));
        return NS_OK;
    }
    
    NS_ENSURE_SUCCESS(rv, rv);

    rv = sss->ProcessHeader(nsISiteSecurityService::HEADER_HSTS, mURI,
                            stsHeader.get(), flags, nullptr, nullptr);
    if (NS_FAILED(rv)) {
        AddSecurityMessage(NS_LITERAL_STRING("InvalidSTSHeaders"),
                NS_LITERAL_STRING("Invalid HSTS Headers"));
        LOG(("STS: Failed to parse STS header, continuing load.\n"));
    }

    return NS_OK;
}

bool
nsHttpChannel::IsHTTPS()
{
    bool isHttps;
    if (NS_FAILED(mURI->SchemeIs("https", &isHttps)) || !isHttps)
        return false;
    return true;
}

void
nsHttpChannel::ProcessSSLInformation()
{
    
    
    
    

    if (mCanceled || NS_FAILED(mStatus) || !mSecurityInfo ||
        !IsHTTPS() || mPrivateBrowsing)
        return;

    nsCOMPtr<nsISSLSocketControl> ssl = do_QueryInterface(mSecurityInfo);
    nsCOMPtr<nsISSLStatusProvider> statusProvider =
        do_QueryInterface(mSecurityInfo);
    if (!ssl || !statusProvider)
        return;
    nsCOMPtr<nsISSLStatus> sslstat;
    statusProvider->GetSSLStatus(getter_AddRefs(sslstat));
    if (!sslstat)
        return;

    
    
    bool trustCheck;
    if (NS_FAILED(sslstat->GetIsDomainMismatch(&trustCheck)) || trustCheck)
        return;
    if (NS_FAILED(sslstat->GetIsNotValidAtThisTime(&trustCheck)) || trustCheck)
        return;
    if (NS_FAILED(sslstat->GetIsUntrusted(&trustCheck)) || trustCheck)
        return;

    int16_t kea = ssl->GetKEAUsed();
    int16_t symcipher = ssl->GetSymmetricCipherUsed();

    nsIPrincipal *principal = GetPrincipal();
    if (!principal)
        return;

    
    

    nsCOMPtr<nsIPermissionManager> permMgr =
        do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
    if (!permMgr)
        return;

    
    int64_t expireTime = (PR_Now() / PR_USEC_PER_MSEC) +
        (86400 * 7 * PR_MSEC_PER_SEC);

    if (kea == ssl_kea_rsa) {
        permMgr->AddFromPrincipal(principal, "falsestart-rsa",
                                  nsIPermissionManager::ALLOW_ACTION,
                                  nsIPermissionManager::EXPIRE_TIME,
                                  expireTime);
        LOG(("nsHttpChannel::ProcessSSLInformation [this=%p] "
             "falsestart-rsa permission granted for this host\n", this));
    } else {
        permMgr->RemoveFromPrincipal(principal, "falsestart-rsa");
    }

    if (symcipher == ssl_calg_rc4) {
        permMgr->AddFromPrincipal(principal, "falsestart-rc4",
                                  nsIPermissionManager::ALLOW_ACTION,
                                  nsIPermissionManager::EXPIRE_TIME,
                                  expireTime);
        LOG(("nsHttpChannel::ProcessSSLInformation [this=%p] "
             "falsestart-rc4 permission granted for this host\n", this));
    } else {
        permMgr->RemoveFromPrincipal(principal, "falsestart-rc4");
    }
}

nsresult
nsHttpChannel::ProcessResponse()
{
    nsresult rv;
    uint32_t httpStatus = mResponseHead->Status();

    
    
    Telemetry::Accumulate(Telemetry::HTTP_TRANSACTION_IS_SSL,
                          mConnectionInfo->UsingSSL());
    if (mLoadFlags & LOAD_INITIAL_DOCUMENT_URI) {
        Telemetry::Accumulate(Telemetry::HTTP_PAGELOAD_IS_SSL,
                              mConnectionInfo->UsingSSL());
    }

    LOG(("nsHttpChannel::ProcessResponse [this=%p httpStatus=%u]\n",
        this, httpStatus));

    if (mTransaction->ProxyConnectFailed()) {
        
        if (httpStatus != 407)
            return ProcessFailedProxyConnect(httpStatus);
        
        
    } else {
        
        rv = ProcessSTSHeader();
        MOZ_ASSERT(NS_SUCCEEDED(rv), "ProcessSTSHeader failed, continuing load.");
    }

    MOZ_ASSERT(!mCachedContentIsValid);

    ProcessSSLInformation();

    
    gHttpHandler->OnExamineResponse(this);

    SetCookie(mResponseHead->PeekHeader(nsHttp::Set_Cookie));

    
    if (httpStatus != 401 && httpStatus != 407) {
        if (!mAuthRetryPending)
            mAuthProvider->CheckForSuperfluousAuth();
        if (mCanceled)
            return CallOnStartRequest();

        
        
        mAuthProvider->Disconnect(NS_ERROR_ABORT);
        mAuthProvider = nullptr;
        LOG(("  continuation state has been reset"));
    }

    bool successfulReval = false;

    
    
    
    
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
        else {
            mCacheInputStream.CloseAndRelease();
            rv = ProcessNormal();
        }
        break;
    case 300:
    case 301:
    case 302:
    case 307:
    case 308:
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
            
            if (mCacheEntry)
                mCacheEntry->AsyncDoom(nullptr);
            if (DoNotRender3xxBody(rv)) {
                mStatus = rv;
                DoNotifyListener();
            } else {
                rv = ContinueProcessResponse(rv);
            }
        }
        break;
    case 304:
        rv = ProcessNotModified();
        if (NS_FAILED(rv)) {
            LOG(("ProcessNotModified failed [rv=%x]\n", rv));
            mCacheInputStream.CloseAndRelease();
            rv = ProcessNormal();
        }
        else {
            successfulReval = true;
        }
        break;
    case 401:
    case 407:
        rv = mAuthProvider->ProcessAuthentication(
            httpStatus, mConnectionInfo->UsingSSL() &&
                        mTransaction->ProxyConnectFailed());
        if (rv == NS_ERROR_IN_PROGRESS)  {
            
            
            mAuthRetryPending = true;
            if (httpStatus == 407 || mTransaction->ProxyConnectFailed())
                mProxyAuthPending = true;

            
            
            
            
            LOG(("Suspending the transaction, asynchronously prompting for credentials"));
            mTransactionPump->Suspend();
            rv = NS_OK;
        }
        else if (NS_FAILED(rv)) {
            LOG(("ProcessAuthentication failed [rv=%x]\n", rv));
            if (mTransaction->ProxyConnectFailed())
                return ProcessFailedProxyConnect(httpStatus);
            if (!mAuthRetryPending)
                mAuthProvider->CheckForSuperfluousAuth();
            rv = ProcessNormal();
        }
        else
            mAuthRetryPending = true; 
        break;
    default:
        rv = ProcessNormal();
        MaybeInvalidateCacheEntryForSubsequentGet();
        break;
    }

    CacheDisposition cacheDisposition;
    if (!mDidReval)
        cacheDisposition = kCacheMissed;
    else if (successfulReval)
        cacheDisposition = kCacheHitViaReval;
    else
        cacheDisposition = kCacheMissedViaReval;

    AccumulateCacheHitTelemetry(mCacheEntryDeviceTelemetryID,
                                cacheDisposition);

    return rv;
}

nsresult
nsHttpChannel::ContinueProcessResponse(nsresult rv)
{
    bool doNotRender = DoNotRender3xxBody(rv);

    if (rv == NS_ERROR_DOM_BAD_URI && mRedirectURI) {
        bool isHTTP = false;
        if (NS_FAILED(mRedirectURI->SchemeIs("http", &isHTTP)))
            isHTTP = false;
        if (!isHTTP && NS_FAILED(mRedirectURI->SchemeIs("https", &isHTTP)))
            isHTTP = false;

        if (!isHTTP) {
            
            
            
            
            LOG(("ContinueProcessResponse detected rejected Non-HTTP Redirection"));
            doNotRender = true;
            rv = NS_ERROR_CORRUPTED_CONTENT;
        }
    }

    if (doNotRender) {
        Cancel(rv);
        DoNotifyListener();
        return rv;
    }

    if (NS_SUCCEEDED(rv)) {
        UpdateInhibitPersistentCachingFlag();

        InitCacheEntry();
        CloseCacheEntry(false);

        if (mApplicationCacheForWrite) {
            
            InitOfflineCacheEntry();
            CloseOfflineCacheEntry();
        }
        return NS_OK;
    }

    LOG(("ContinueProcessResponse got failure result [rv=%x]\n", rv));
    if (mTransaction->ProxyConnectFailed()) {
        return ProcessFailedProxyConnect(mRedirectType);
    }
    return ProcessNormal();
}

nsresult
nsHttpChannel::ProcessNormal()
{
    nsresult rv;

    LOG(("nsHttpChannel::ProcessNormal [this=%p]\n", this));

    bool succeeded;
    rv = GetRequestSucceeded(&succeeded);
    if (NS_SUCCEEDED(rv) && !succeeded) {
        PushRedirectAsyncFunc(&nsHttpChannel::ContinueProcessNormal);
        bool waitingForRedirectCallback;
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

    
    
    
    mCachedContentIsPartial = false;

    ClearBogusContentEncodingIfNeeded();

    UpdateInhibitPersistentCachingFlag();

    
    
    
    if (mCacheEntry) {
        rv = InitCacheEntry();
        if (NS_FAILED(rv))
            CloseCacheEntry(true);
    }

    
    if (mResuming) {
        
        nsAutoCString id;
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

    
    if (mCacheEntry && !mLoadedFromApplicationCache) {
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
        bool repost = false;

        nsCOMPtr<nsIPrompt> prompt;
        GetCallback(prompt);
        if (!prompt)
            return NS_ERROR_NO_INTERFACE;

        prompt->Confirm(nullptr, messageString, &repost);
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
nsHttpChannel::HandleAsyncRedirectChannelToHttps()
{
    NS_PRECONDITION(!mCallOnResume, "How did that happen?");

    if (mSuspendCount) {
        LOG(("Waiting until resume to do async redirect to https [this=%p]\n", this));
        mCallOnResume = &nsHttpChannel::HandleAsyncRedirectChannelToHttps;
        return;
    }

    nsresult rv = StartRedirectChannelToHttps();
    if (NS_FAILED(rv))
        ContinueAsyncRedirectChannelToURI(rv);
}

nsresult
nsHttpChannel::StartRedirectChannelToHttps()
{
    nsresult rv = NS_OK;
    LOG(("nsHttpChannel::HandleAsyncRedirectChannelToHttps() [STS]\n"));

    nsCOMPtr<nsIURI> upgradedURI;

    rv = mURI->Clone(getter_AddRefs(upgradedURI));
    NS_ENSURE_SUCCESS(rv,rv);

    upgradedURI->SetScheme(NS_LITERAL_CSTRING("https"));

    int32_t oldPort = -1;
    rv = mURI->GetPort(&oldPort);
    if (NS_FAILED(rv)) return rv;

    
    
    
    

    if (oldPort == 80 || oldPort == -1)
        upgradedURI->SetPort(-1);
    else
        upgradedURI->SetPort(oldPort);

    return StartRedirectChannelToURI(upgradedURI);
}

void
nsHttpChannel::HandleAsyncAPIRedirect()
{
    NS_PRECONDITION(!mCallOnResume, "How did that happen?");
    NS_PRECONDITION(mAPIRedirectToURI, "How did that happen?");

    if (mSuspendCount) {
        LOG(("Waiting until resume to do async API redirect [this=%p]\n", this));
        mCallOnResume = &nsHttpChannel::HandleAsyncAPIRedirect;
        return;
    }

    nsresult rv = StartRedirectChannelToURI(mAPIRedirectToURI);
    if (NS_FAILED(rv))
        ContinueAsyncRedirectChannelToURI(rv);

    return;
}

nsresult
nsHttpChannel::StartRedirectChannelToURI(nsIURI *upgradedURI)
{
    nsresult rv = NS_OK;
    LOG(("nsHttpChannel::StartRedirectChannelToURI()\n"));

    nsCOMPtr<nsIChannel> newChannel;

    nsCOMPtr<nsIIOService> ioService;
    rv = gHttpHandler->GetIOService(getter_AddRefs(ioService));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = ioService->NewChannelFromURI(upgradedURI, getter_AddRefs(newChannel));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = SetupReplacementChannel(upgradedURI, newChannel, true);
    NS_ENSURE_SUCCESS(rv, rv);

    
    mRedirectChannel = newChannel;
    uint32_t flags = nsIChannelEventSink::REDIRECT_PERMANENT;

    PushRedirectAsyncFunc(
        &nsHttpChannel::ContinueAsyncRedirectChannelToURI);
    rv = gHttpHandler->AsyncOnChannelRedirect(this, newChannel, flags);

    if (NS_SUCCEEDED(rv))
        rv = WaitForRedirectCallback();

    if (NS_FAILED(rv)) {
        AutoRedirectVetoNotifier notifier(this);

        


        PopRedirectAsyncFunc(
            &nsHttpChannel::ContinueAsyncRedirectChannelToURI);
    }

    return rv;
}

nsresult
nsHttpChannel::ContinueAsyncRedirectChannelToURI(nsresult rv)
{
    if (NS_SUCCEEDED(rv))
        rv = OpenRedirectChannel(rv);

    if (NS_FAILED(rv)) {
        
        
        
        mStatus = rv;
    }

    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nullptr, mStatus);

    if (NS_FAILED(rv)) {
        
        
        
        DoNotifyListener();
    }

    return rv;
}

nsresult
nsHttpChannel::OpenRedirectChannel(nsresult rv)
{
    AutoRedirectVetoNotifier notifier(this);

    
    mRedirectChannel->SetOriginalURI(mOriginalURI);

    
    nsCOMPtr<nsIHttpEventSink> httpEventSink;
    GetCallback(httpEventSink);
    if (httpEventSink) {
        
        
        rv = httpEventSink->OnRedirect(this, mRedirectChannel);
        if (NS_FAILED(rv)) {
            return rv;
        }
    }

    
    rv = mRedirectChannel->AsyncOpen(mListener, mListenerContext);
    if (NS_FAILED(rv)) {
        return rv;
    }

    mStatus = NS_BINDING_REDIRECTED;

    notifier.RedirectSucceeded();

    ReleaseListeners();

    return NS_OK;
}

nsresult
nsHttpChannel::AsyncDoReplaceWithProxy(nsIProxyInfo* pi)
{
    LOG(("nsHttpChannel::AsyncDoReplaceWithProxy [this=%p pi=%p]", this, pi));
    nsresult rv;

    nsCOMPtr<nsIChannel> newChannel;
    rv = gHttpHandler->NewProxiedChannel(mURI, pi, mProxyResolveFlags,
                                         mProxyURI, getter_AddRefs(newChannel));
    if (NS_FAILED(rv))
        return rv;

    rv = SetupReplacementChannel(mURI, newChannel, true);
    if (NS_FAILED(rv))
        return rv;

    
    mRedirectChannel = newChannel;
    uint32_t flags = nsIChannelEventSink::REDIRECT_INTERNAL;

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

    ReleaseListeners();

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

    
    
    
    nsCOMPtr<nsIProtocolProxyService2> pps2 = do_QueryInterface(pps);
    if (pps2) {
        rv = pps2->AsyncResolve2(mProxyURI ? mProxyURI : mURI, mProxyResolveFlags,
                                 this, getter_AddRefs(mProxyRequest));
    } else {
        rv = pps->AsyncResolve(mProxyURI ? mProxyURI : mURI, mProxyResolveFlags,
                               this, getter_AddRefs(mProxyRequest));
    }

    return rv;
}

bool
nsHttpChannel::ResponseWouldVary(nsICacheEntry* entry) const
{
    nsresult rv;
    nsAutoCString buf, metaKey;
    mCachedResponseHead->GetHeader(nsHttp::Vary, buf);
    if (!buf.IsEmpty()) {
        NS_NAMED_LITERAL_CSTRING(prefix, "request-");

        
        char *val = buf.BeginWriting(); 
        char *token = nsCRT::strtok(val, NS_HTTP_HEADER_SEPS, &val);
        while (token) {
            LOG(("nsHttpChannel::ResponseWouldVary [channel=%p] " \
                 "processing %s\n",
                 this, token));
            
            
            
            
            
            
            
            
            
            
            
            
            
            if (*token == '*')
                return true; 

            
            metaKey = prefix + nsDependentCString(token);

            
            
            nsXPIDLCString lastVal;
            entry->GetMetaDataElement(metaKey.get(), getter_Copies(lastVal));
            LOG(("nsHttpChannel::ResponseWouldVary [channel=%p] "
                     "stored value = \"%s\"\n",
                 this, lastVal.get()));

            
            nsHttpAtom atom = nsHttp::ResolveAtom(token);
            const char *newVal = mRequestHead.PeekHeader(atom);
            if (!lastVal.IsEmpty()) {
                
                if (!newVal)
                    return true; 

                
                
                
                nsAutoCString hash;
                if (atom == nsHttp::Cookie) {
                    rv = Hash(newVal, hash);
                    
                    
                    if (NS_FAILED(rv))
                        return true;
                    newVal = hash.get();

                    LOG(("nsHttpChannel::ResponseWouldVary [this=%p] " \
                            "set-cookie value hashed to %s\n",
                         this, newVal));
                }

                if (strcmp(newVal, lastVal))
                    return true; 

            } else if (newVal) { 
                return true;
            }

            
            token = nsCRT::strtok(val, NS_HTTP_HEADER_SEPS, &val);
        }
    }
    return false;
}




void
nsHttpChannel::HandleAsyncAbort()
{
    HttpAsyncAborter<nsHttpChannel>::HandleAsyncAbort();
}


nsresult
nsHttpChannel::EnsureAssocReq()
{
    
    
    
    
    
    
    

    if (!mResponseHead)
        return NS_OK;

    const char *assoc_val = mResponseHead->PeekHeader(nsHttp::Assoc_Req);
    if (!assoc_val)
        return NS_OK;

    if (!mTransaction || !mURI)
        return NS_OK;

    if (!mTransaction->PipelinePosition()) {
        
        

        const char *pragma_val = mResponseHead->PeekHeader(nsHttp::Pragma);
        if (!pragma_val ||
            !nsHttp::FindToken(pragma_val, "X-Verify-Assoc-Req",
                               HTTP_HEADER_VALUE_SEPS))
            return NS_OK;
    }

    char *method = net_FindCharNotInSet(assoc_val, HTTP_LWS);
    if (!method)
        return NS_OK;

    bool equals;
    char *endofmethod;

    assoc_val = nullptr;
    endofmethod = net_FindCharInSet(method, HTTP_LWS);
    if (endofmethod)
        assoc_val = net_FindCharNotInSet(endofmethod, HTTP_LWS);
    if (!assoc_val)
        return NS_OK;

    
    int32_t methodlen = strlen(mRequestHead.Method().get());
    if ((methodlen != (endofmethod - method)) ||
        PL_strncmp(method,
                   mRequestHead.Method().get(),
                   endofmethod - method)) {
        LOG(("  Assoc-Req failure Method %s", method));
        if (mConnectionInfo)
            gHttpHandler->ConnMgr()->
                PipelineFeedbackInfo(mConnectionInfo,
                                     nsHttpConnectionMgr::RedCorruptedContent,
                                     nullptr, 0);

        nsCOMPtr<nsIConsoleService> consoleService =
            do_GetService(NS_CONSOLESERVICE_CONTRACTID);
        if (consoleService) {
            nsAutoString message
                (NS_LITERAL_STRING("Failed Assoc-Req. Received "));
            AppendASCIItoUTF16(
                mResponseHead->PeekHeader(nsHttp::Assoc_Req),
                message);
            message += NS_LITERAL_STRING(" expected method ");
            AppendASCIItoUTF16(mRequestHead.Method().get(), message);
            consoleService->LogStringMessage(message.get());
        }

        if (gHttpHandler->EnforceAssocReq())
            return NS_ERROR_CORRUPTED_CONTENT;
        return NS_OK;
    }

    
    nsCOMPtr<nsIURI> assoc_url;
    if (NS_FAILED(NS_NewURI(getter_AddRefs(assoc_url), assoc_val)) ||
        !assoc_url)
        return NS_OK;

    mURI->Equals(assoc_url, &equals);
    if (!equals) {
        LOG(("  Assoc-Req failure URL %s", assoc_val));
        if (mConnectionInfo)
            gHttpHandler->ConnMgr()->
                PipelineFeedbackInfo(mConnectionInfo,
                                     nsHttpConnectionMgr::RedCorruptedContent,
                                     nullptr, 0);

        nsCOMPtr<nsIConsoleService> consoleService =
            do_GetService(NS_CONSOLESERVICE_CONTRACTID);
        if (consoleService) {
            nsAutoString message
                (NS_LITERAL_STRING("Failed Assoc-Req. Received "));
            AppendASCIItoUTF16(
                mResponseHead->PeekHeader(nsHttp::Assoc_Req),
                message);
            message += NS_LITERAL_STRING(" expected URL ");
            AppendASCIItoUTF16(mSpec.get(), message);
            consoleService->LogStringMessage(message.get());
        }

        if (gHttpHandler->EnforceAssocReq())
            return NS_ERROR_CORRUPTED_CONTENT;
    }
    return NS_OK;
}





nsresult
nsHttpChannel::MaybeSetupByteRangeRequest(int64_t partialLen, int64_t contentLength)
{
    nsresult rv = NS_OK;

    bool hasContentEncoding =
        mCachedResponseHead->PeekHeader(nsHttp::Content_Encoding)
        != nullptr;

    
    mIsPartialRequest = false;

    if ((partialLen < contentLength) &&
         partialLen > 0 &&
         !hasContentEncoding &&
         mCachedResponseHead->IsResumable() &&
         !mCustomConditionalRequest &&
         !mCachedResponseHead->NoStore()) {
        
        
        rv = SetupByteRangeRequest(partialLen);
        if (NS_FAILED(rv)) {
            
            mRequestHead.ClearHeader(nsHttp::Range);
            mRequestHead.ClearHeader(nsHttp::If_Range);
        }
    }

    return rv;
}

nsresult
nsHttpChannel::SetupByteRangeRequest(int64_t partialLen)
{
    
    

    
    const char *val = mCachedResponseHead->PeekHeader(nsHttp::ETag);
    if (!val)
        val = mCachedResponseHead->PeekHeader(nsHttp::Last_Modified);
    if (!val) {
        
        
        NS_NOTREACHED("no cache validator");
        mIsPartialRequest = false;
        return NS_ERROR_FAILURE;
    }

    char buf[64];
    PR_snprintf(buf, sizeof(buf), "bytes=%lld-", partialLen);

    mRequestHead.SetHeader(nsHttp::Range, nsDependentCString(buf));
    mRequestHead.SetHeader(nsHttp::If_Range, nsDependentCString(val));
    mIsPartialRequest = true;

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

    nsresult rv;

    if (mConcurentCacheAccess) {
        
        
        
        

        rv = InstallCacheListener(mLogicalOffset);
        if (NS_FAILED(rv)) return rv;

        if (mOfflineCacheEntry) {
            rv = InstallOfflineCacheListener(mLogicalOffset);
            if (NS_FAILED(rv)) return rv;
        }

        
        rv = mCachedResponseHead->UpdateHeaders(mResponseHead->Headers());
        if (NS_FAILED(rv)) return rv;

        
        nsAutoCString head;
        mCachedResponseHead->Flatten(head, true);
        rv = mCacheEntry->SetMetaDataElement("response-head", head.get());
        if (NS_FAILED(rv)) return rv;

        UpdateInhibitPersistentCachingFlag();

        rv = UpdateExpirationTime();
        if (NS_FAILED(rv)) return rv;

        mCachedContentIsPartial = false;

        
        
        gHttpHandler->OnExamineMergedResponse(this);
    }
    else {
        
        rv = mTransactionPump->Suspend();
        if (NS_FAILED(rv)) return rv;

        
        rv = mCachedResponseHead->UpdateHeaders(mResponseHead->Headers());
        if (NS_FAILED(rv)) return rv;

        
        nsAutoCString head;
        mCachedResponseHead->Flatten(head, true);
        rv = mCacheEntry->SetMetaDataElement("response-head", head.get());
        if (NS_FAILED(rv)) return rv;

        
        mResponseHead = mCachedResponseHead;

        UpdateInhibitPersistentCachingFlag();

        rv = UpdateExpirationTime();
        if (NS_FAILED(rv)) return rv;

        
        
        gHttpHandler->OnExamineMergedResponse(this);

        
        mCachedContentIsValid = true;
        rv = ReadFromCache(false);
    }

    return rv;
}

nsresult
nsHttpChannel::OnDoneReadingPartialCacheEntry(bool *streamDone)
{
    nsresult rv;

    LOG(("nsHttpChannel::OnDoneReadingPartialCacheEntry [this=%p]", this));

    
    *streamDone = true;

    
    int64_t size;
    rv = mCacheEntry->GetDataSize(&size);
    if (NS_FAILED(rv)) return rv;

    rv = InstallCacheListener(size);
    if (NS_FAILED(rv)) return rv;

    
    
    
    
    rv = mCacheEntry->SetValid();
    if (NS_FAILED(rv)) return rv;

    
    mLogicalOffset = size;

    
    
    mCachedContentIsPartial = false;

    
    
    if (mTransactionPump) {
        rv = mTransactionPump->Resume();
        if (NS_SUCCEEDED(rv))
            *streamDone = false;
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

    if (!mDidReval) {
        LOG(("Server returned a 304 response even though we did not send a "
             "conditional request"));
        return NS_ERROR_FAILURE;
    }

    MOZ_ASSERT(mCachedResponseHead);
    MOZ_ASSERT(mCacheEntry);
    NS_ENSURE_TRUE(mCachedResponseHead && mCacheEntry, NS_ERROR_UNEXPECTED);

    
    
    
    
    
    

    nsAutoCString lastModifiedCached;
    nsAutoCString lastModified304;

    rv = mCachedResponseHead->GetHeader(nsHttp::Last_Modified,
                                        lastModifiedCached);
    if (NS_SUCCEEDED(rv)) {
        rv = mResponseHead->GetHeader(nsHttp::Last_Modified,
                                      lastModified304);
    }

    if (NS_SUCCEEDED(rv) && !lastModified304.Equals(lastModifiedCached)) {
        LOG(("Cache Entry and 304 Last-Modified Headers Do Not Match "
             "[%s] and [%s]\n",
             lastModifiedCached.get(), lastModified304.get()));

        mCacheEntry->AsyncDoom(nullptr);
        if (mConnectionInfo)
            gHttpHandler->ConnMgr()->
                PipelineFeedbackInfo(mConnectionInfo,
                                     nsHttpConnectionMgr::RedCorruptedContent,
                                     nullptr, 0);
        Telemetry::Accumulate(Telemetry::CACHE_LM_INCONSISTENT, true);
    }

    
    rv = mCachedResponseHead->UpdateHeaders(mResponseHead->Headers());
    if (NS_FAILED(rv)) return rv;

    
    nsAutoCString head;
    mCachedResponseHead->Flatten(head, true);
    rv = mCacheEntry->SetMetaDataElement("response-head", head.get());
    if (NS_FAILED(rv)) return rv;

    
    mResponseHead = mCachedResponseHead;

    UpdateInhibitPersistentCachingFlag();

    rv = UpdateExpirationTime();
    if (NS_FAILED(rv)) return rv;

    rv = AddCacheEntryHeaders(mCacheEntry);
    if (NS_FAILED(rv)) return rv;

    
    
    gHttpHandler->OnExamineMergedResponse(this);

    mCachedContentIsValid = true;

    
    rv = mCacheEntry->SetValid();
    if (NS_FAILED(rv)) return rv;

    rv = ReadFromCache(false);
    if (NS_FAILED(rv)) return rv;

    mTransactionReplaced = true;
    return NS_OK;
}

nsresult
nsHttpChannel::ProcessFallback(bool *waitingForRedirectCallback)
{
    LOG(("nsHttpChannel::ProcessFallback [this=%p]\n", this));
    nsresult rv;

    *waitingForRedirectCallback = false;
    mFallingBack = false;

    
    
    
    if (!mApplicationCache || mFallbackKey.IsEmpty() || mFallbackChannel) {
        LOG(("  choosing not to fallback [%p,%s,%d]",
             mApplicationCache.get(), mFallbackKey.get(), mFallbackChannel));
        return NS_OK;
    }

    
    
    uint32_t fallbackEntryType;
    rv = mApplicationCache->GetTypes(mFallbackKey, &fallbackEntryType);
    NS_ENSURE_SUCCESS(rv, rv);

    if (fallbackEntryType & nsIApplicationCache::ITEM_FOREIGN) {
        
        
        return NS_OK;
    }

    MOZ_ASSERT(fallbackEntryType & nsIApplicationCache::ITEM_FALLBACK,
               "Fallback entry not marked correctly!");

    
    
    if (mOfflineCacheEntry) {
        mOfflineCacheEntry->AsyncDoom(nullptr);
        mOfflineCacheEntry = nullptr;
    }

    mApplicationCacheForWrite = nullptr;
    mOfflineCacheEntry = nullptr;

    
    CloseCacheEntry(true);

    
    nsRefPtr<nsIChannel> newChannel;
    rv = gHttpHandler->NewChannel(mURI, getter_AddRefs(newChannel));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = SetupReplacementChannel(mURI, newChannel, true);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIHttpChannelInternal> httpInternal =
        do_QueryInterface(newChannel, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = httpInternal->SetupFallbackChannel(mFallbackKey.get());
    NS_ENSURE_SUCCESS(rv, rv);

    
    uint32_t newLoadFlags = mLoadFlags | LOAD_REPLACE | LOAD_ONLY_FROM_CACHE;
    rv = newChannel->SetLoadFlags(newLoadFlags);

    
    mRedirectChannel = newChannel;
    uint32_t redirectFlags = nsIChannelEventSink::REDIRECT_INTERNAL;

    PushRedirectAsyncFunc(&nsHttpChannel::ContinueProcessFallback);
    rv = gHttpHandler->AsyncOnChannelRedirect(this, newChannel, redirectFlags);

    if (NS_SUCCEEDED(rv))
        rv = WaitForRedirectCallback();

    if (NS_FAILED(rv)) {
        AutoRedirectVetoNotifier notifier(this);
        PopRedirectAsyncFunc(&nsHttpChannel::ContinueProcessFallback);
        return rv;
    }

    
    
    *waitingForRedirectCallback = true;
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

    if (mLoadFlags & LOAD_INITIAL_DOCUMENT_URI) {
        mozilla::Telemetry::Accumulate(Telemetry::HTTP_OFFLINE_CACHE_DOCUMENT_LOAD,
                                       true);
    }

    
    Cancel(NS_BINDING_REDIRECTED);

    notifier.RedirectSucceeded();

    ReleaseListeners();

    mFallingBack = true;

    return NS_OK;
}



static bool
IsSubRangeRequest(nsHttpRequestHead &aRequestHead)
{
    if (!aRequestHead.PeekHeader(nsHttp::Range))
        return false;
    nsAutoCString byteRange;
    aRequestHead.GetHeader(nsHttp::Range, byteRange);
    return !byteRange.EqualsLiteral("bytes=0-");
}

nsresult
nsHttpChannel::OpenCacheEntry(bool usingSSL)
{
    MOZ_EVENT_TRACER_EXEC(this, "net::http::OpenCacheEntry");

    
    AutoCacheWaitFlags waitFlags(this);

    
    mConcurentCacheAccess = 0;

    nsresult rv;

    mLoadedFromApplicationCache = false;
    mHasQueryString = HasQueryString(mRequestHead.Method(), mURI);

    LOG(("nsHttpChannel::OpenCacheEntry [this=%p]", this));

    
    NS_PRECONDITION(!mCacheEntry, "cache entry already open");

    nsAutoCString cacheKey;

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

    
    
    if (!mApplicationCache && mInheritApplicationCache) {
        nsCOMPtr<nsIApplicationCacheContainer> appCacheContainer;
        GetCallback(appCacheContainer);

        if (appCacheContainer) {
            appCacheContainer->GetApplicationCache(getter_AddRefs(mApplicationCache));
        }
    }

    nsCOMPtr<nsICacheStorageService> cacheStorageService =
        do_GetService("@mozilla.org/netwerk/cache-storage-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<LoadContextInfo> info = GetLoadContextInfo(this);

    nsCOMPtr<nsICacheStorage> cacheStorage;
    if (mApplicationCache) {
        rv = cacheStorageService->AppCacheStorage(info, mApplicationCache,
                                                  getter_AddRefs(cacheStorage));
    }
    else if (mLoadFlags & INHIBIT_PERSISTENT_CACHING) {
        rv = cacheStorageService->MemoryCacheStorage(info, 
            getter_AddRefs(cacheStorage));
    }
    else {
        rv = cacheStorageService->DiskCacheStorage(info,
            mChooseApplicationCache || (mLoadFlags & LOAD_CHECK_OFFLINE_CACHE),
            getter_AddRefs(cacheStorage));
    }
    NS_ENSURE_SUCCESS(rv, rv);

    uint32_t cacheEntryOpenFlags;
    if (BYPASS_LOCAL_CACHE(mLoadFlags))
        cacheEntryOpenFlags = nsICacheStorage::OPEN_TRUNCATE;
    else
        cacheEntryOpenFlags = nsICacheStorage::OPEN_NORMALLY;

    if (mLoadAsBlocking || mLoadUnblocked ||
        (mLoadFlags & LOAD_INITIAL_DOCUMENT_URI)) {
        cacheEntryOpenFlags |= nsICacheStorage::OPEN_PRIORITY;
    }

    
    
    if (mLoadFlags & LOAD_BYPASS_LOCAL_CACHE_IF_BUSY)
        cacheEntryOpenFlags |= nsICacheStorage::OPEN_BYPASS_IF_BUSY;

    nsCOMPtr<nsIURI> openURI;
    if (!mFallbackKey.IsEmpty() && mFallbackChannel) {
        
        rv = NS_NewURI(getter_AddRefs(openURI), mFallbackKey);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
        openURI = mURI;
    }

    rv = cacheStorage->AsyncOpenURI(
        openURI, mPostID ? nsPrintfCString("%d", mPostID) : EmptyCString(),
        cacheEntryOpenFlags, this);
    NS_ENSURE_SUCCESS(rv, rv);

    waitFlags.Keep(WAIT_FOR_CACHE_ENTRY);

    if (!mApplicationCacheForWrite)
        return NS_OK;

    

    
    NS_PRECONDITION(!mOfflineCacheEntry, "cache entry already open");

    bool offline = gIOService->IsOffline();
    if (offline) {
        
        return NS_OK;
    }

    if (mLoadFlags & INHIBIT_CACHING) {
        
        return NS_OK;
    }

    if (mRequestHead.Method() != nsHttp::Get) {
        
        return NS_OK;
    }

    rv = cacheStorageService->AppCacheStorage(info, mApplicationCacheForWrite,
                                              getter_AddRefs(cacheStorage));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = cacheStorage->AsyncOpenURI(
      mURI, EmptyCString(), nsICacheStorage::OPEN_TRUNCATE, this);
    NS_ENSURE_SUCCESS(rv, rv);

    waitFlags.Keep(WAIT_FOR_OFFLINE_CACHE_ENTRY);

    return NS_OK;
}

nsresult
nsHttpChannel::CheckPartial(nsICacheEntry* aEntry, int64_t *aSize, int64_t *aContentLength)
{
    nsresult rv;

    rv = aEntry->GetDataSize(aSize);

    if (NS_ERROR_IN_PROGRESS == rv) {
        *aSize = -1;
        rv = NS_OK;
    }

    NS_ENSURE_SUCCESS(rv, rv);

    nsHttpResponseHead* responseHead = mCachedResponseHead
        ? mCachedResponseHead
        : mResponseHead;

    if (!responseHead)
        return NS_ERROR_UNEXPECTED;

    *aContentLength = responseHead->ContentLength();

    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::OnCacheEntryCheck(nsICacheEntry* entry, nsIApplicationCache* appCache,
                                 uint32_t* aResult)
{
    nsresult rv = NS_OK;

    LOG(("nsHttpChannel::OnCacheEntryCheck enter [channel=%p entry=%p]",
        this, entry));

    
    
    mCustomConditionalRequest =
        mRequestHead.PeekHeader(nsHttp::If_Modified_Since) ||
        mRequestHead.PeekHeader(nsHttp::If_None_Match) ||
        mRequestHead.PeekHeader(nsHttp::If_Unmodified_Since) ||
        mRequestHead.PeekHeader(nsHttp::If_Match) ||
        mRequestHead.PeekHeader(nsHttp::If_Range);

    
    *aResult = ENTRY_WANTED;
    mCachedContentIsValid = false;

    nsXPIDLCString buf;

    
    rv = entry->GetMetaDataElement("request-method", getter_Copies(buf));
    NS_ENSURE_SUCCESS(rv, rv);

    nsHttpAtom method = nsHttp::ResolveAtom(buf);
    if (method == nsHttp::Head) {
        
        
        if (mRequestHead.Method() != nsHttp::Head)
            return NS_OK;
    }
    buf.Adopt(0);

    
    uint32_t lastModifiedTime;
    rv = entry->GetLastModified(&lastModifiedTime);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    bool fromPreviousSession =
            (gHttpHandler->SessionStartTime() > lastModifiedTime);

    
    rv = entry->GetMetaDataElement("response-head", getter_Copies(buf));
    NS_ENSURE_SUCCESS(rv, rv);

    
    mCachedResponseHead = new nsHttpResponseHead();
    rv = mCachedResponseHead->Parse((char *) buf.get());
    NS_ENSURE_SUCCESS(rv, rv);
    buf.Adopt(0);

    bool isCachedRedirect = WillRedirect(mCachedResponseHead);

    
    
    NS_ENSURE_TRUE((mCachedResponseHead->Status() / 100 != 3) ||
                   isCachedRedirect, NS_ERROR_ABORT);

    
    
    
    
    if (!mApplicationCacheForWrite &&
        (appCache ||
         (mCacheEntryIsReadOnly && !(mLoadFlags & nsIRequest::INHIBIT_CACHING)) ||
         mFallbackChannel)) {
        rv = OpenCacheInputStream(entry, true);
        if (NS_SUCCEEDED(rv)) {
            mCachedContentIsValid = true;
            entry->MaybeMarkValid();
        }
        return rv;
    }

    if (method != nsHttp::Head && !isCachedRedirect) {
        
        
        
        
        
        
        
        
        int64_t size, contentLength;
        rv = CheckPartial(entry, &size, &contentLength);
        NS_ENSURE_SUCCESS(rv,rv);

        if (size == int64_t(-1)) {
            LOG(("  write is in progress"));
            if (mLoadFlags & LOAD_BYPASS_LOCAL_CACHE_IF_BUSY) {
                LOG(("  not interested in the entry, "
                     "LOAD_BYPASS_LOCAL_CACHE_IF_BUSY specified"));
                *aResult = ENTRY_NOT_WANTED;
                return NS_OK;
            }

            mConcurentCacheAccess = 1;
        }
        else if (contentLength != int64_t(-1) && contentLength != size) {
            LOG(("Cached data size does not match the Content-Length header "
                 "[content-length=%lld size=%lld]\n", contentLength, size));

            rv = MaybeSetupByteRangeRequest(size, contentLength);
            mCachedContentIsPartial = NS_SUCCEEDED(rv);
            if (mCachedContentIsPartial) {
                rv = OpenCacheInputStream(entry, false);
                *aResult = ENTRY_NEEDS_REVALIDATION;
            }
            return rv;
        }
    }

    bool usingSSL = false;
    rv = mURI->SchemeIs("https", &usingSSL);
    NS_ENSURE_SUCCESS(rv,rv);

    bool doValidation = false;
    bool canAddImsHeader = true;

    
    if (ResponseWouldVary(entry)) {
        LOG(("Validating based on Vary headers returning TRUE\n"));
        canAddImsHeader = false;
        doValidation = true;
    }
    
    else if (mLoadFlags & nsIRequest::LOAD_FROM_CACHE) {
        LOG(("NOT validating based on LOAD_FROM_CACHE load flag\n"));
        doValidation = false;
    }
    
    
    else if (mLoadFlags & nsIRequest::VALIDATE_ALWAYS) {
        LOG(("Validating based on VALIDATE_ALWAYS load flag\n"));
        doValidation = true;
    }
    
    
    else if (mLoadFlags & nsIRequest::VALIDATE_NEVER) {
        LOG(("VALIDATE_NEVER set\n"));
        
        
        if (mCachedResponseHead->NoStore() ||
           (mCachedResponseHead->NoCache() && usingSSL)) {
            LOG(("Validating based on (no-store || (no-cache && ssl)) logic\n"));
            doValidation = true;
        }
        else {
            LOG(("NOT validating based on VALIDATE_NEVER load flag\n"));
            doValidation = false;
        }
    }
    
    else if (mCachedResponseHead->MustValidate()) {
        LOG(("Validating based on MustValidate() returning TRUE\n"));
        doValidation = true;
    }

    else if (MustValidateBasedOnQueryUrl()) {
        LOG(("Validating based on RFC 2616 section 13.9 "
             "(query-url w/o explicit expiration-time)\n"));
        doValidation = true;
    }
    
    else {
        uint32_t time = 0; 

        rv = entry->GetExpirationTime(&time);
        NS_ENSURE_SUCCESS(rv, rv);

        LOG(("  NowInSeconds()=%u, time=%u", NowInSeconds(), time));
        if (NowInSeconds() <= time)
            doValidation = false;
        else if (mCachedResponseHead->MustValidateIfExpired())
            doValidation = true;
        else if (mLoadFlags & nsIRequest::VALIDATE_ONCE_PER_SESSION) {
            
            
            
            
            
            rv = mCachedResponseHead->ComputeFreshnessLifetime(&time);
            NS_ENSURE_SUCCESS(rv, rv);

            if (time == 0)
                doValidation = true;
            else
                doValidation = fromPreviousSession;
        }
        else
            doValidation = true;

        LOG(("%salidating based on expiration time\n", doValidation ? "V" : "Not v"));
    }

    if (!doValidation && mRequestHead.PeekHeader(nsHttp::If_Match) &&
        (method == nsHttp::Get || method == nsHttp::Head)) {
        const char *requestedETag, *cachedETag;
        cachedETag = mCachedResponseHead->PeekHeader(nsHttp::ETag);
        requestedETag = mRequestHead.PeekHeader(nsHttp::If_Match);
        if (cachedETag && (!strncmp(cachedETag, "W/", 2) ||
            strcmp(requestedETag, cachedETag))) {
            
            
            
            doValidation = true;
        }
    }

    if (!doValidation) {
        
        
        
        
        
        
        
        
        
        
        
        entry->GetMetaDataElement("auth", getter_Copies(buf));
        doValidation =
            (fromPreviousSession && !buf.IsEmpty()) ||
            (buf.IsEmpty() && mRequestHead.PeekHeader(nsHttp::Authorization));
    }

    
    
    
    
    
    
    if (!doValidation && isCachedRedirect) {
        nsAutoCString cacheKey;
        GenerateCacheKey(mPostID, cacheKey);

        if (!mRedirectedCachekeys)
            mRedirectedCachekeys = new nsTArray<nsCString>();
        else if (mRedirectedCachekeys->Contains(cacheKey))
            doValidation = true;

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
            mDidReval = true;
        }
    }

    if (mCachedContentIsValid || mDidReval) {
        rv = OpenCacheInputStream(entry, mCachedContentIsValid);
        if (NS_FAILED(rv)) {
            
            
            if (mDidReval) {
                
                mRequestHead.ClearHeader(nsHttp::If_Modified_Since);
                mRequestHead.ClearHeader(nsHttp::ETag);
                mDidReval = false;
            }
            mCachedContentIsValid = false;
        }
    }

    if (mDidReval)
        *aResult = ENTRY_NEEDS_REVALIDATION;
    else
        *aResult = ENTRY_WANTED;

    if (mCachedContentIsValid) {
        entry->MaybeMarkValid();
    }

    LOG(("nsHTTPChannel::OnCacheEntryCheck exit [this=%p doValidation=%d result=%d]\n",
         this, doValidation, *aResult));
    return rv;
}

NS_IMETHODIMP
nsHttpChannel::OnCacheEntryAvailable(nsICacheEntry *entry,
                                     bool aNew,
                                     nsIApplicationCache* aAppCache,
                                     nsresult status)
{
    MOZ_ASSERT(NS_IsMainThread());

    nsresult rv;

    LOG(("nsHttpChannel::OnCacheEntryAvailable [this=%p entry=%p "
         "new=%d appcache=%p status=%x]\n", this, entry, aNew, aAppCache, status));

    
    
    if (!mIsPending) {
        mCacheInputStream.CloseAndRelease();
        return NS_OK;
    }

    rv = OnCacheEntryAvailableInternal(entry, aNew, aAppCache, status);
    if (NS_FAILED(rv)) {
        CloseCacheEntry(true);
        AsyncAbort(rv);
    }

    return NS_OK;
}

nsresult
nsHttpChannel::OnCacheEntryAvailableInternal(nsICacheEntry *entry,
                                             bool aNew,
                                             nsIApplicationCache* aAppCache,
                                             nsresult status)
{
    nsresult rv;

    if (mCanceled) {
        LOG(("channel was canceled [this=%p status=%x]\n", this, mStatus));
        return mStatus;
    }

    if (aAppCache) {
        if (mApplicationCache == aAppCache && !mCacheEntry) {
            rv = OnOfflineCacheEntryAvailable(entry, aNew, aAppCache, status);
        }
        else if (mApplicationCacheForWrite == aAppCache && aNew && !mOfflineCacheEntry) {
            rv = OnOfflineCacheEntryForWritingAvailable(entry, aAppCache, status);
        }
        else {
            rv = OnOfflineCacheEntryAvailable(entry, aNew, aAppCache, status);
        }
    }
    else {
        rv = OnNormalCacheEntryAvailable(entry, aNew, status);
    }

    if (NS_FAILED(rv) && mLoadFlags & LOAD_ONLY_FROM_CACHE) {
        
        
        if (!mFallbackChannel && !mFallbackKey.IsEmpty()) {
            return AsyncCall(&nsHttpChannel::HandleAsyncFallback);
        }
        return NS_ERROR_DOCUMENT_NOT_CACHED;
    }

    if (NS_FAILED(rv))
      return rv;

    
    if (mCacheEntriesToWaitFor)
      return NS_OK;

    return ContinueConnect();
}

nsresult
nsHttpChannel::OnNormalCacheEntryAvailable(nsICacheEntry *aEntry,
                                           bool aNew,
                                           nsresult aEntryStatus)
{
    mCacheEntriesToWaitFor &= ~WAIT_FOR_CACHE_ENTRY;

    if (aNew) {
      bool offline = gIOService->IsOffline();
      if ((mLoadFlags & INHIBIT_CACHING) && !offline) {
          
          return NS_OK;
      }

      if ((mLoadFlags & LOAD_ONLY_FROM_CACHE)) {
          
          
          return NS_ERROR_DOCUMENT_NOT_CACHED;
      }
    }

    if (NS_SUCCEEDED(aEntryStatus)) {
        mCacheEntry = aEntry;
        mCacheEntryIsWriteOnly = aNew;

        if (mLoadFlags & LOAD_INITIAL_DOCUMENT_URI) {
            mozilla::Telemetry::Accumulate(Telemetry::HTTP_OFFLINE_CACHE_DOCUMENT_LOAD,
                                           false);
        }
    }

    return NS_OK;
}

nsresult
nsHttpChannel::OnOfflineCacheEntryAvailable(nsICacheEntry *aEntry,
                                            bool aNew,
                                            nsIApplicationCache* aAppCache,
                                            nsresult aEntryStatus)
{
    MOZ_ASSERT(!mApplicationCache || aAppCache == mApplicationCache);
    MOZ_ASSERT(!aNew || !aEntry || mApplicationCacheForWrite);

    mCacheEntriesToWaitFor &= ~WAIT_FOR_CACHE_ENTRY;

    nsresult rv;

    if (!mApplicationCache)
        mApplicationCache = aAppCache;

    if (NS_SUCCEEDED(aEntryStatus)) {
        
        
        mLoadedFromApplicationCache = true;
        mCacheEntryIsReadOnly = true;
        mCacheEntry = aEntry;
        mCacheEntryIsWriteOnly = false;

        if (mLoadFlags & LOAD_INITIAL_DOCUMENT_URI && !mApplicationCacheForWrite) {
            mozilla::Telemetry::Accumulate(Telemetry::HTTP_OFFLINE_CACHE_DOCUMENT_LOAD,
                                           true);
        }

        return NS_OK;
    }

    if (!mApplicationCacheForWrite && !mFallbackChannel) {
        
        nsCOMPtr<nsIApplicationCacheNamespace> namespaceEntry;
        rv = mApplicationCache->GetMatchingNamespace(mSpec,
            getter_AddRefs(namespaceEntry));
        NS_ENSURE_SUCCESS(rv, rv);

        uint32_t namespaceType = 0;
        if (!namespaceEntry ||
            NS_FAILED(namespaceEntry->GetItemType(&namespaceType)) ||
            (namespaceType &
             (nsIApplicationCacheNamespace::NAMESPACE_FALLBACK |
              nsIApplicationCacheNamespace::NAMESPACE_BYPASS)) == 0) {
            
            
            
            mLoadFlags |= LOAD_ONLY_FROM_CACHE;

            
            
            return NS_ERROR_CACHE_KEY_NOT_FOUND;
        }

        if (namespaceType &
            nsIApplicationCacheNamespace::NAMESPACE_FALLBACK) {
            rv = namespaceEntry->GetData(mFallbackKey);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    return NS_OK;
}

nsresult
nsHttpChannel::OnOfflineCacheEntryForWritingAvailable(nsICacheEntry *aEntry,
                                                      nsIApplicationCache* aAppCache,
                                                      nsresult aEntryStatus)
{
    MOZ_ASSERT(mApplicationCacheForWrite && aAppCache == mApplicationCacheForWrite);

    mCacheEntriesToWaitFor &= ~WAIT_FOR_OFFLINE_CACHE_ENTRY;

    if (NS_SUCCEEDED(aEntryStatus)) {
        mOfflineCacheEntry = aEntry;
        if (NS_FAILED(aEntry->GetLastModified(&mOfflineCacheLastModifiedTime))) {
            mOfflineCacheLastModifiedTime = 0;
        }
    }

    return aEntryStatus;
}

NS_IMETHODIMP
nsHttpChannel::GetMainThreadOnly(bool *aMainThreadOnly)
{
    NS_ENSURE_ARG(aMainThreadOnly);

    
    *aMainThreadOnly = false;
    return NS_OK;
}


nsresult
nsHttpChannel::GenerateCacheKey(uint32_t postID, nsACString &cacheKey)
{
    AssembleCacheKey(mFallbackChannel ? mFallbackKey.get() : mSpec.get(),
                     postID, cacheKey);
    return NS_OK;
}


void
nsHttpChannel::AssembleCacheKey(const char *spec, uint32_t postID,
                                nsACString &cacheKey)
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

    
    const char *p = strchr(spec, '#');
    if (p)
        cacheKey.Append(spec, p - spec);
    else
        cacheKey.Append(spec);
}










nsresult
nsHttpChannel::UpdateExpirationTime()
{
    NS_ENSURE_TRUE(mResponseHead, NS_ERROR_FAILURE);

    nsresult rv;

    uint32_t expirationTime = 0;
    if (!mResponseHead->MustValidate()) {
        uint32_t freshnessLifetime = 0;

        rv = mResponseHead->ComputeFreshnessLifetime(&freshnessLifetime);
        if (NS_FAILED(rv)) return rv;

        if (freshnessLifetime > 0) {
            uint32_t now = NowInSeconds(), currentAge = 0;

            rv = mResponseHead->ComputeCurrentAge(now, mRequestTime, &currentAge);
            if (NS_FAILED(rv)) return rv;

            LOG(("freshnessLifetime = %u, currentAge = %u\n",
                freshnessLifetime, currentAge));

            if (freshnessLifetime > currentAge) {
                uint32_t timeRemaining = freshnessLifetime - currentAge;
                
                if (now + timeRemaining < now)
                    expirationTime = uint32_t(-1);
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

 inline bool
nsHttpChannel::HasQueryString(nsHttpAtom method, nsIURI * uri)
{
    
    
    MOZ_ASSERT(NS_IsMainThread());

    if (method != nsHttp::Get && method != nsHttp::Head)
        return false;

    nsAutoCString query;
    nsCOMPtr<nsIURL> url = do_QueryInterface(uri);
    nsresult rv = url->GetQuery(query);
    return NS_SUCCEEDED(rv) && !query.IsEmpty();
}

bool
nsHttpChannel::MustValidateBasedOnQueryUrl() const
{
    
    
    
    
    if (mHasQueryString)
    {
        uint32_t tmp; 
        nsresult rv = mCachedResponseHead->GetExpiresValue(&tmp);
        if (NS_FAILED(rv)) {
            rv = mCachedResponseHead->GetMaxAgeValue(&tmp);
            if (NS_FAILED(rv)) {
                return true;
            }
        }
    }
    return false;
}


bool
nsHttpChannel::ShouldUpdateOfflineCacheEntry()
{
    if (!mApplicationCacheForWrite || !mOfflineCacheEntry) {
        return false;
    }

    
    if (mCacheEntry && mCacheEntryIsWriteOnly) {
        return true;
    }

    
    if (mOfflineCacheEntry) {
        return true;
    }

    
    uint32_t docLastModifiedTime;
    nsresult rv = mResponseHead->GetLastModifiedValue(&docLastModifiedTime);
    if (NS_FAILED(rv)) {
        return true;
    }

    if (mOfflineCacheLastModifiedTime == 0) {
        return false;
    }

    if (docLastModifiedTime > mOfflineCacheLastModifiedTime) {
        return true;
    }

    return false;
}

nsresult
nsHttpChannel::OpenCacheInputStream(nsICacheEntry* cacheEntry, bool startBuffering)
{
    nsresult rv;

    bool usingSSL = false;
    rv = mURI->SchemeIs("https", &usingSSL);
    NS_ENSURE_SUCCESS(rv,rv);

    if (usingSSL) {
        rv = cacheEntry->GetSecurityInfo(
                                      getter_AddRefs(mCachedSecurityInfo));
        if (NS_FAILED(rv)) {
            LOG(("failed to parse security-info [channel=%p, entry=%p]",
                 this, cacheEntry));
            NS_WARNING("failed to parse security-info");
            return rv;
        }

        
        
        MOZ_ASSERT(mCachedSecurityInfo || mLoadedFromApplicationCache);
        if (!mCachedSecurityInfo && !mLoadedFromApplicationCache) {
            LOG(("mCacheEntry->GetSecurityInfo returned success but did not "
                 "return the security info [channel=%p, entry=%p]",
                 this, cacheEntry));
            return NS_ERROR_UNEXPECTED; 
        }
    }

    

    rv = NS_OK;

    if (WillRedirect(mCachedResponseHead)) {
        
        
        LOG(("Will skip read of cached redirect entity\n"));
        return NS_OK;
    }

    if ((mLoadFlags & nsICachingChannel::LOAD_ONLY_IF_MODIFIED) &&
        !mCachedContentIsPartial) {
        
        
        if (!mApplicationCacheForWrite) {
            LOG(("Will skip read from cache based on LOAD_ONLY_IF_MODIFIED "
                 "load flag\n"));
            return NS_OK;
        }

        
        
        
        
        
        
        LOG(("May skip read from cache based on LOAD_ONLY_IF_MODIFIED "
              "load flag\n"));
    }

    
    
    nsCOMPtr<nsIInputStream> stream;
    rv = cacheEntry->OpenInputStream(0, getter_AddRefs(stream));

    if (NS_FAILED(rv)) {
        LOG(("Failed to open cache input stream [channel=%p, "
             "mCacheEntry=%p]", this, cacheEntry));
        return rv;
    }

    if (startBuffering) {
        bool nonBlocking;
        rv = stream->IsNonBlocking(&nonBlocking);
        if (NS_SUCCEEDED(rv) && nonBlocking)
            startBuffering = false;
    }

    if (!startBuffering) {
        
        
        
        
        
        
        
        
        
        
        LOG(("Opened cache input stream without buffering [channel=%p, "
              "mCacheEntry=%p, stream=%p]", this,
              cacheEntry, stream.get()));
        mCacheInputStream.takeOver(stream);
        return rv;
    }

    
    

    nsCOMPtr<nsITransport> transport;
    nsCOMPtr<nsIInputStream> wrapper;

    nsCOMPtr<nsIStreamTransportService> sts =
        do_GetService(kStreamTransportServiceCID, &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = sts->CreateInputTransport(stream, int64_t(-1), int64_t(-1),
                                        true, getter_AddRefs(transport));
    }
    if (NS_SUCCEEDED(rv)) {
        rv = transport->OpenInputStream(0, 0, 0, getter_AddRefs(wrapper));
    }
    if (NS_SUCCEEDED(rv)) {
        LOG(("Opened cache input stream [channel=%p, wrapper=%p, "
              "transport=%p, stream=%p]", this, wrapper.get(),
              transport.get(), stream.get()));
    } else {
        LOG(("Failed to open cache input stream [channel=%p, "
              "wrapper=%p, transport=%p, stream=%p]", this,
              wrapper.get(), transport.get(), stream.get()));

        stream->Close();
        return rv;
    }

    mCacheInputStream.takeOver(wrapper);

    return NS_OK;
}



nsresult
nsHttpChannel::ReadFromCache(bool alreadyMarkedValid)
{
    NS_ENSURE_TRUE(mCacheEntry, NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(mCachedContentIsValid, NS_ERROR_FAILURE);

    LOG(("nsHttpChannel::ReadFromCache [this=%p] "
         "Using cached copy of: %s\n", this, mSpec.get()));

    if (mCachedResponseHead)
        mResponseHead = mCachedResponseHead;

    UpdateInhibitPersistentCachingFlag();

    
    
    
    
    if (!mSecurityInfo)
        mSecurityInfo = mCachedSecurityInfo;

    if (!alreadyMarkedValid && !mCachedContentIsPartial) {
        
        
        
        
        
        
        mCacheEntry->MaybeMarkValid();
    }

    nsresult rv;

    
    

    if (WillRedirect(mResponseHead)) {
        
        
        MOZ_ASSERT(!mCacheInputStream);
        LOG(("Skipping skip read of cached redirect entity\n"));
        return AsyncCall(&nsHttpChannel::HandleAsyncRedirect);
    }

    if ((mLoadFlags & LOAD_ONLY_IF_MODIFIED) && !mCachedContentIsPartial) {
        if (!mApplicationCacheForWrite) {
            LOG(("Skipping read from cache based on LOAD_ONLY_IF_MODIFIED "
                 "load flag\n"));
            MOZ_ASSERT(!mCacheInputStream);
            
            
            return AsyncCall(&nsHttpChannel::HandleAsyncNotModified);
        }

        if (!ShouldUpdateOfflineCacheEntry()) {
            LOG(("Skipping read from cache based on LOAD_ONLY_IF_MODIFIED "
                 "load flag (mApplicationCacheForWrite not null case)\n"));
            mCacheInputStream.CloseAndRelease();
            
            
            return AsyncCall(&nsHttpChannel::HandleAsyncNotModified);
        }
    }

    MOZ_ASSERT(mCacheInputStream);
    if (!mCacheInputStream) {
        NS_ERROR("mCacheInputStream is null but we're expecting to "
                        "be able to read from it.");
        return NS_ERROR_UNEXPECTED;
    }


    nsCOMPtr<nsIInputStream> inputStream = mCacheInputStream.forget();

    rv = nsInputStreamPump::Create(getter_AddRefs(mCachePump), inputStream,
                                   int64_t(-1), int64_t(-1), 0, 0, true);
    if (NS_FAILED(rv)) {
        inputStream->Close();
        return rv;
    }

    rv = mCachePump->AsyncRead(this, mListenerContext);
    if (NS_FAILED(rv)) return rv;

    if (mTimingEnabled)
        mCacheReadStart = TimeStamp::Now();

    uint32_t suspendCount = mSuspendCount;
    while (suspendCount--)
        mCachePump->Suspend();

    return NS_OK;
}

void
nsHttpChannel::CloseCacheEntry(bool doomOnFailure)
{
    mCacheInputStream.CloseAndRelease();

    if (!mCacheEntry)
        return;

    LOG(("nsHttpChannel::CloseCacheEntry [this=%p] mStatus=%x mCacheEntryIsWriteOnly=%x",
         this, mStatus, mCacheEntryIsWriteOnly));

    
    
    
    

    bool doom = false;
    if (mInitedCacheEntry) {
        MOZ_ASSERT(mResponseHead, "oops");
        if (NS_FAILED(mStatus) && doomOnFailure &&
            mCacheEntryIsWriteOnly && !mResponseHead->IsResumable())
            doom = true;
    }
    else if (mCacheEntryIsWriteOnly)
        doom = true;

    if (doom) {
        LOG(("  dooming cache entry!!"));
        mCacheEntry->AsyncDoom(nullptr);
    }

    mCachedResponseHead = nullptr;

    mCachePump = nullptr;
    mCacheEntry = nullptr;
    mCacheEntryIsWriteOnly = false;
    mInitedCacheEntry = false;
}


void
nsHttpChannel::CloseOfflineCacheEntry()
{
    if (!mOfflineCacheEntry)
        return;

    LOG(("nsHttpChannel::CloseOfflineCacheEntry [this=%p]", this));

    if (NS_FAILED(mStatus)) {
        mOfflineCacheEntry->AsyncDoom(nullptr);
    }
    else {
        bool succeeded;
        if (NS_SUCCEEDED(GetRequestSucceeded(&succeeded)) && !succeeded)
            mOfflineCacheEntry->AsyncDoom(nullptr);
    }

    mOfflineCacheEntry = nullptr;
}







nsresult
nsHttpChannel::InitCacheEntry()
{
    nsresult rv;

    NS_ENSURE_TRUE(mCacheEntry, NS_ERROR_UNEXPECTED);
    
    if (mCacheEntryIsReadOnly)
        return NS_OK;

    
    if (mCachedContentIsValid)
        return NS_OK;

    LOG(("nsHttpChannel::InitCacheEntry [this=%p entry=%p]\n",
        this, mCacheEntry.get()));

    if (!mCacheEntryIsWriteOnly) {
        LOG(("  we have a ready entry, but reading it again from the server -> recreating cache entry\n"));
        nsCOMPtr<nsICacheEntry> currentEntry;
        currentEntry.swap(mCacheEntry);
        rv = currentEntry->Recreate(getter_AddRefs(mCacheEntry));
        if (NS_FAILED(rv)) {
          LOG(("  recreation failed, the response will not be cached"));
          return NS_OK;
        }

        mCacheEntryIsWriteOnly = true;
    }

    if (mLoadFlags & INHIBIT_PERSISTENT_CACHING) {
        rv = mCacheEntry->SetPersistToDisk(false);
        if (NS_FAILED(rv)) return rv;
    }

    
    rv = UpdateExpirationTime();
    if (NS_FAILED(rv)) return rv;

    rv = AddCacheEntryHeaders(mCacheEntry);
    if (NS_FAILED(rv)) return rv;

    mInitedCacheEntry = true;

    
    mConcurentCacheAccess = 0;

    return NS_OK;
}

void
nsHttpChannel::UpdateInhibitPersistentCachingFlag()
{
    
    
    if (mResponseHead->NoStore())
        mLoadFlags |= INHIBIT_PERSISTENT_CACHING;

    
    if (!gHttpHandler->IsPersistentHttpsCachingEnabled() &&
        mConnectionInfo->UsingSSL())
        mLoadFlags |= INHIBIT_PERSISTENT_CACHING;
}

nsresult
nsHttpChannel::InitOfflineCacheEntry()
{
    

    if (!mOfflineCacheEntry) {
        return NS_OK;
    }

    if (!mResponseHead || mResponseHead->NoStore()) {
        if (mResponseHead && mResponseHead->NoStore()) {
            mOfflineCacheEntry->AsyncDoom(nullptr);
        }

        CloseOfflineCacheEntry();

        if (mResponseHead && mResponseHead->NoStore()) {
            return NS_ERROR_NOT_AVAILABLE;
        }

        return NS_OK;
    }

    
    
    
    if (mCacheEntry) {
        uint32_t expirationTime;
        nsresult rv = mCacheEntry->GetExpirationTime(&expirationTime);
        NS_ENSURE_SUCCESS(rv, rv);

        mOfflineCacheEntry->SetExpirationTime(expirationTime);
    }

    return AddCacheEntryHeaders(mOfflineCacheEntry);
}


nsresult
nsHttpChannel::AddCacheEntryHeaders(nsICacheEntry *entry)
{
    nsresult rv;

    LOG(("nsHttpChannel::AddCacheEntryHeaders [this=%p] begin", this));
    
    if (mSecurityInfo)
        entry->SetSecurityInfo(mSecurityInfo);

    
    
    rv = entry->SetMetaDataElement("request-method",
                                   mRequestHead.Method().get());
    if (NS_FAILED(rv)) return rv;

    
    rv = StoreAuthorizationMetaData(entry);
    if (NS_FAILED(rv)) return rv;

    
    
    
    
    
    
    
    
    
    
    
    {
        nsAutoCString buf, metaKey;
        mResponseHead->GetHeader(nsHttp::Vary, buf);
        if (!buf.IsEmpty()) {
            NS_NAMED_LITERAL_CSTRING(prefix, "request-");

            char *val = buf.BeginWriting(); 
            char *token = nsCRT::strtok(val, NS_HTTP_HEADER_SEPS, &val);
            while (token) {
                LOG(("nsHttpChannel::AddCacheEntryHeaders [this=%p] " \
                        "processing %s", this, token));
                if (*token != '*') {
                    nsHttpAtom atom = nsHttp::ResolveAtom(token);
                    const char *val = mRequestHead.PeekHeader(atom);
                    nsAutoCString hash;
                    if (val) {
                        
                        if (atom == nsHttp::Cookie) {
                            LOG(("nsHttpChannel::AddCacheEntryHeaders [this=%p] " \
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
                        LOG(("nsHttpChannel::AddCacheEntryHeaders [this=%p] " \
                                "clearing metadata for %s", this, token));
                        metaKey = prefix + nsDependentCString(token);
                        entry->SetMetaDataElement(metaKey.get(), nullptr);
                    }
                }
                token = nsCRT::strtok(val, NS_HTTP_HEADER_SEPS, &val);
            }
        }
    }


    
    
    nsAutoCString head;
    mResponseHead->Flatten(head, true);
    rv = entry->SetMetaDataElement("response-head", head.get());
    if (NS_FAILED(rv)) return rv;

    
    rv = entry->MetaDataReady();

    return rv;
}

inline void
GetAuthType(const char *challenge, nsCString &authType)
{
    const char *p;

    
    if ((p = strchr(challenge, ' ')) != nullptr)
        authType.Assign(challenge, p - challenge);
    else
        authType.Assign(challenge);
}

nsresult
nsHttpChannel::StoreAuthorizationMetaData(nsICacheEntry *entry)
{
    
    const char *val = mRequestHead.PeekHeader(nsHttp::Authorization);
    if (!val)
        return NS_OK;

    
    nsAutoCString buf;
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
nsHttpChannel::InstallCacheListener(int64_t offset)
{
    nsresult rv;

    LOG(("Preparing to write data into the cache [uri=%s]\n", mSpec.get()));

    MOZ_ASSERT(mCacheEntry);
    MOZ_ASSERT(mCacheEntryIsWriteOnly || mCachedContentIsPartial);
    MOZ_ASSERT(mListener);

    
    
    if ((mResponseHead->PeekHeader(nsHttp::Content_Encoding) == nullptr) && (
         mResponseHead->ContentType().EqualsLiteral(TEXT_HTML) ||
         mResponseHead->ContentType().EqualsLiteral(TEXT_PLAIN) ||
         mResponseHead->ContentType().EqualsLiteral(TEXT_CSS) ||
         mResponseHead->ContentType().EqualsLiteral(TEXT_JAVASCRIPT) ||
         mResponseHead->ContentType().EqualsLiteral(TEXT_ECMASCRIPT) ||
         mResponseHead->ContentType().EqualsLiteral(TEXT_XML) ||
         mResponseHead->ContentType().EqualsLiteral(APPLICATION_JAVASCRIPT) ||
         mResponseHead->ContentType().EqualsLiteral(APPLICATION_ECMASCRIPT) ||
         mResponseHead->ContentType().EqualsLiteral(APPLICATION_XJAVASCRIPT) ||
         mResponseHead->ContentType().EqualsLiteral(APPLICATION_XHTML_XML))) {
        rv = mCacheEntry->SetMetaDataElement("uncompressed-len", "0");
        if (NS_FAILED(rv)) {
            LOG(("unable to mark cache entry for compression"));
        }
    }

    LOG(("Trading cache input stream for output stream [channel=%p]", this));

    
    
    
    mCacheInputStream.CloseAndRelease();

    nsCOMPtr<nsIOutputStream> out;
    rv = mCacheEntry->OpenOutputStream(offset, getter_AddRefs(out));
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      LOG(("  entry doomed, not writing it [channel=%p]", this));
      
      
      
      return NS_OK;
    }
    if (NS_FAILED(rv)) return rv;

    
#if 0
    
    rv = mCacheEntry->MarkValid();
    if (NS_FAILED(rv)) return rv;
#endif

    nsCOMPtr<nsIStreamListenerTee> tee =
        do_CreateInstance(kStreamListenerTeeCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsICacheStorageService> serv =
        do_GetService("@mozilla.org/netwerk/cache-storage-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIEventTarget> cacheIOTarget;
    serv->GetIoTarget(getter_AddRefs(cacheIOTarget));

    if (!cacheIOTarget) {
        LOG(("nsHttpChannel::InstallCacheListener sync tee %p rv=%x "
             "cacheIOTarget=%p", tee.get(), rv, cacheIOTarget.get()));
        rv = tee->Init(mListener, out, nullptr);
    } else {
        LOG(("nsHttpChannel::InstallCacheListener async tee %p", tee.get()));
        rv = tee->InitAsync(mListener, cacheIOTarget, out, nullptr);
    }

    if (NS_FAILED(rv)) return rv;
    mListener = tee;
    return NS_OK;
}

nsresult
nsHttpChannel::InstallOfflineCacheListener(int64_t offset)
{
    nsresult rv;

    LOG(("Preparing to write data into the offline cache [uri=%s]\n",
         mSpec.get()));

    MOZ_ASSERT(mOfflineCacheEntry);
    MOZ_ASSERT(mListener);

    nsCOMPtr<nsIOutputStream> out;
    rv = mOfflineCacheEntry->OpenOutputStream(offset, getter_AddRefs(out));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIStreamListenerTee> tee =
        do_CreateInstance(kStreamListenerTeeCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = tee->Init(mListener, out, nullptr);
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
                                       bool          preserveMethod)
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
nsHttpChannel::AsyncProcessRedirection(uint32_t redirectType)
{
    LOG(("nsHttpChannel::AsyncProcessRedirection [this=%p type=%u]\n",
        this, redirectType));

    
    
    MOZ_EVENT_TRACER_EXEC(this, "net::http::channel");
    MOZ_EVENT_TRACER_EXEC(this, "net::http::redirect-callbacks");

    const char *location = mResponseHead->PeekHeader(nsHttp::Location);

    
    
    if (!location)
        return NS_ERROR_FAILURE;

    
    nsAutoCString locationBuf;
    if (NS_EscapeURL(location, -1, esc_OnlyNonASCII, locationBuf))
        location = locationBuf.get();

    if (mRedirectionLimit == 0) {
        LOG(("redirection limit reached!\n"));
        return NS_ERROR_REDIRECT_LOOP;
    }

    mRedirectType = redirectType;

    LOG(("redirecting to: %s [redirection-limit=%u]\n",
        location, uint32_t(mRedirectionLimit)));

    nsresult rv = CreateNewURI(location, getter_AddRefs(mRedirectURI));

    if (NS_FAILED(rv)) {
        LOG(("Invalid URI for redirect: Location: %s\n", location));
        return NS_ERROR_CORRUPTED_CONTENT;
    }

    if (mApplicationCache) {
        
        
        
        if (!NS_SecurityCompareURIs(mURI, mRedirectURI, false)) {
            PushRedirectAsyncFunc(&nsHttpChannel::ContinueProcessRedirectionAfterFallback);
            bool waitingForRedirectCallback;
            (void)ProcessFallback(&waitingForRedirectCallback);
            if (waitingForRedirectCallback)
                return NS_OK;
            PopRedirectAsyncFunc(&nsHttpChannel::ContinueProcessRedirectionAfterFallback);
        }
    }

    return ContinueProcessRedirectionAfterFallback(NS_OK);
}


nsresult
nsHttpChannel::CreateNewURI(const char *loc, nsIURI **newURI)
{
    nsCOMPtr<nsIIOService> ioService;
    nsresult rv = gHttpHandler->GetIOService(getter_AddRefs(ioService));
    if (NS_FAILED(rv)) return rv;

    
    nsAutoCString originCharset;
    rv = mURI->GetOriginCharset(originCharset);
    if (NS_FAILED(rv))
        originCharset.Truncate();

    return ioService->NewURI(nsDependentCString(loc),
                             originCharset.get(),
                             mURI,
                             newURI);
}

nsresult
nsHttpChannel::ContinueProcessRedirectionAfterFallback(nsresult rv)
{
    if (NS_SUCCEEDED(rv) && mFallingBack) {
        
        
        return NS_OK;
    }

    
    
    bool redirectingBackToSameURI = false;
    if (mCacheEntry && mCacheEntryIsWriteOnly &&
        NS_SUCCEEDED(mURI->Equals(mRedirectURI, &redirectingBackToSameURI)) &&
        redirectingBackToSameURI)
            mCacheEntry->AsyncDoom(nullptr);

    
    
    nsAutoCString ref;
    rv = mRedirectURI->GetRef(ref);
    if (NS_SUCCEEDED(rv) && ref.IsEmpty()) {
        mURI->GetRef(ref);
        if (!ref.IsEmpty()) {
            
            
            mRedirectURI->SetRef(ref);
        }
    }

    bool rewriteToGET = nsHttp::ShouldRewriteRedirectToGET(
                                    mRedirectType, mRequestHead.Method());

    
    if (!rewriteToGET &&
        !nsHttp::IsSafeMethod(mRequestHead.Method())) {
        rv = PromptTempRedirect();
        if (NS_FAILED(rv)) return rv;
    }

    nsCOMPtr<nsIIOService> ioService;
    rv = gHttpHandler->GetIOService(getter_AddRefs(ioService));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIChannel> newChannel;
    rv = ioService->NewChannelFromURI(mRedirectURI, getter_AddRefs(newChannel));
    if (NS_FAILED(rv)) return rv;

    rv = SetupReplacementChannel(mRedirectURI, newChannel, !rewriteToGET);
    if (NS_FAILED(rv)) return rv;

    uint32_t redirectFlags;
    if (nsHttp::IsPermanentRedirect(mRedirectType))
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

    ReleaseListeners();

    return NS_OK;
}





NS_IMETHODIMP nsHttpChannel::OnAuthAvailable()
{
    LOG(("nsHttpChannel::OnAuthAvailable [this=%p]", this));

    
    
    
    mAuthRetryPending = true;
    mProxyAuthPending = false;
    LOG(("Resuming the transaction, we got credentials from user"));
    mTransactionPump->Resume();

    return NS_OK;
}

NS_IMETHODIMP nsHttpChannel::OnAuthCancelled(bool userCancel)
{
    LOG(("nsHttpChannel::OnAuthCancelled [this=%p]", this));

    if (mTransactionPump) {
        
        
        
        
        
        
        
        if (mProxyAuthPending)
            Cancel(NS_ERROR_PROXY_CONNECTION_REFUSED);

        
        
        nsresult rv = CallOnStartRequest();

        
        
        
        mAuthRetryPending = false;
        LOG(("Resuming the transaction, user cancelled the auth dialog"));
        mTransactionPump->Resume();

        if (NS_FAILED(rv))
            mTransactionPump->Cancel(rv);
    }

    mProxyAuthPending = false;
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
    NS_INTERFACE_MAP_ENTRY(nsICacheEntryOpenCallback)
    NS_INTERFACE_MAP_ENTRY(nsIHttpChannelInternal)
    NS_INTERFACE_MAP_ENTRY(nsIResumableChannel)
    NS_INTERFACE_MAP_ENTRY(nsITransportEventSink)
    NS_INTERFACE_MAP_ENTRY(nsISupportsPriority)
    NS_INTERFACE_MAP_ENTRY(nsIProtocolProxyCallback)
    NS_INTERFACE_MAP_ENTRY(nsIProxiedChannel)
    NS_INTERFACE_MAP_ENTRY(nsIHttpAuthenticableChannel)
    NS_INTERFACE_MAP_ENTRY(nsIApplicationCacheContainer)
    NS_INTERFACE_MAP_ENTRY(nsIApplicationCacheChannel)
    NS_INTERFACE_MAP_ENTRY(nsIAsyncVerifyRedirectCallback)
    NS_INTERFACE_MAP_ENTRY(nsITimedChannel)
    NS_INTERFACE_MAP_ENTRY(nsIThreadRetargetableRequest)
    NS_INTERFACE_MAP_ENTRY(nsIThreadRetargetableStreamListener)
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
    mCanceled = true;
    mStatus = status;
    if (mProxyRequest)
        mProxyRequest->Cancel(status);
    if (mTransaction)
        gHttpHandler->CancelTransaction(mTransaction, status);
    if (mTransactionPump)
        mTransactionPump->Cancel(status);
    mCacheInputStream.CloseAndRelease();
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

    if (--mSuspendCount == 0 && mCallOnResume) {
        nsresult rv = AsyncCall(mCallOnResume);
        mCallOnResume = nullptr;
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
    MOZ_EVENT_TRACER_WAIT(this, "net::http::channel");

    LOG(("nsHttpChannel::AsyncOpen [this=%p]\n", this));

    NS_ENSURE_ARG_POINTER(listener);
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);
    NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);

    nsresult rv;

    rv = NS_CheckPortSafety(mURI);
    if (NS_FAILED(rv)) {
        ReleaseListeners();
        return rv;
    }

    
    const char *cookieHeader = mRequestHead.PeekHeader(nsHttp::Cookie);
    if (cookieHeader) {
        mUserSetCookieHeader = cookieHeader;
    }

    AddCookiesToRequest();

    
    if (!(mLoadFlags & LOAD_REPLACE)) {
        gHttpHandler->OnOpeningRequest(this);
    }

    mIsPending = true;
    mWasOpened = true;

    mListener = listener;
    mListenerContext = context;

    
    
    if (mLoadGroup)
        mLoadGroup->AddRequest(this, nullptr);

    
    
    
    
    mAsyncOpenTime = TimeStamp::Now();

    
    
    if (!mProxyInfo && NS_SUCCEEDED(ResolveProxy()))
        return NS_OK;

    rv = BeginConnect();
    if (NS_FAILED(rv))
        ReleaseListeners();

    return rv;
}

nsresult
nsHttpChannel::BeginConnect()
{
    LOG(("nsHttpChannel::BeginConnect [this=%p]\n", this));
    nsresult rv;

    
    nsAutoCString host;
    int32_t port = -1;
    bool usingSSL = false;

    rv = mURI->SchemeIs("https", &usingSSL);
    if (NS_SUCCEEDED(rv))
        rv = mURI->GetAsciiHost(host);
    if (NS_SUCCEEDED(rv))
        rv = mURI->GetPort(&port);
    if (NS_SUCCEEDED(rv))
        rv = mURI->GetAsciiSpec(mSpec);
    if (NS_FAILED(rv))
        return rv;

    
    if (host.IsEmpty())
        return NS_ERROR_MALFORMED_URI;
    LOG(("host=%s port=%d\n", host.get(), port));
    LOG(("uri=%s\n", mSpec.get()));

    nsCOMPtr<nsProxyInfo> proxyInfo;
    if (mProxyInfo)
        proxyInfo = do_QueryInterface(mProxyInfo);

    mConnectionInfo = new nsHttpConnectionInfo(host, port, proxyInfo, usingSSL);

    mAuthProvider =
        do_CreateInstance("@mozilla.org/network/http-channel-auth-provider;1",
                          &rv);
    if (NS_SUCCEEDED(rv))
        rv = mAuthProvider->Init(this);
    if (NS_FAILED(rv))
        return rv;

    
    mAuthProvider->AddAuthorizationHeaders();

    
    CallOnModifyRequestObservers();

    
    
    if (mAPIRedirectToURI) {
        return AsyncCall(&nsHttpChannel::HandleAsyncAPIRedirect);
    }

    
    
    if (!mTimingEnabled)
        mAsyncOpenTime = TimeStamp();

    
    if (!mConnectionInfo->UsingConnect() && mConnectionInfo->UsingHttpProxy()) {
        mCaps &= ~NS_HTTP_ALLOW_PIPELINING;
        if (gHttpHandler->ProxyPipelining())
            mCaps |= NS_HTTP_ALLOW_PIPELINING;
    }

    
    gHttpHandler->AddConnectionHeader(&mRequestHead.Headers(), mCaps);

    if (!mConnectionInfo->UsingHttpProxy()) {
        
        
        
        
        
        
        
        
        
        
        
        mDNSPrefetch = new nsDNSPrefetch(mURI, mTimingEnabled);
        mDNSPrefetch->PrefetchHigh();
    }

    
    
    
    if (mRequestHead.HasHeaderValue(nsHttp::Connection, "close"))
        mCaps &= ~(NS_HTTP_ALLOW_KEEPALIVE | NS_HTTP_ALLOW_PIPELINING);

    if ((mLoadFlags & VALIDATE_ALWAYS) ||
        (BYPASS_LOCAL_CACHE(mLoadFlags)))
        mCaps |= NS_HTTP_REFRESH_DNS;

    if (gHttpHandler->CritialRequestPrioritization()) {
        if (mLoadAsBlocking)
            mCaps |= NS_HTTP_LOAD_AS_BLOCKING;
        if (mLoadUnblocked)
            mCaps |= NS_HTTP_LOAD_UNBLOCKED;
    }

    
    if (mLoadFlags & LOAD_FRESH_CONNECTION) {
        
        if (mLoadFlags & LOAD_INITIAL_DOCUMENT_URI) {
            gHttpHandler->ConnMgr()->DoShiftReloadConnectionCleanup(mConnectionInfo);
        }
        
        mCaps &= ~(NS_HTTP_ALLOW_KEEPALIVE | NS_HTTP_ALLOW_PIPELINING);
    }

    
    
    
    if (mCanceled)
        rv = mStatus;
    else
        rv = Connect();
    if (NS_FAILED(rv)) {
        LOG(("Calling AsyncAbort [rv=%x mCanceled=%i]\n", rv, mCanceled));
        CloseCacheEntry(true);
        AsyncAbort(rv);
    } else if (mLoadFlags & LOAD_CLASSIFY_URI) {
        nsRefPtr<nsChannelClassifier> classifier = new nsChannelClassifier();
        rv = classifier->Start(this);
        if (NS_FAILED(rv)) {
            Cancel(rv);
            return rv;
        }
    }

    return NS_OK;
}





NS_IMETHODIMP
nsHttpChannel::SetupFallbackChannel(const char *aFallbackKey)
{
    ENSURE_CALLED_BEFORE_CONNECT();

    LOG(("nsHttpChannel::SetupFallbackChannel [this=%p, key=%s]",
         this, aFallbackKey));
    mFallbackChannel = true;
    mFallbackKey = aFallbackKey;

    return NS_OK;
}





NS_IMETHODIMP
nsHttpChannel::SetPriority(int32_t value)
{
    int16_t newValue = clamped<int32_t>(value, INT16_MIN, INT16_MAX);
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
    LOG(("nsHttpChannel::OnProxyAvailable [this=%p pi=%p status=%x mStatus=%x]\n",
         this, pi, status, mStatus));
    mProxyRequest = nullptr;

    nsresult rv;

    
    
    
    

    if (NS_SUCCEEDED(status))
        mProxyInfo = pi;

    if (!gHttpHandler->Active()) {
        LOG(("nsHttpChannel::OnProxyAvailable [this=%p] "
             "Handler no longer active.\n", this));
        rv = NS_ERROR_NOT_AVAILABLE;
    }
    else {
        rv = BeginConnect();
    }

    if (NS_FAILED(rv)) {
        Cancel(rv);
        DoNotifyListener();
    }
    return NS_OK;
}





NS_IMETHODIMP
nsHttpChannel::GetProxyInfo(nsIProxyInfo **result)
{
    if (!mConnectionInfo)
        *result = mProxyInfo;
    else
        *result = mConnectionInfo->ProxyInfo();
    NS_IF_ADDREF(*result);
    return NS_OK;
}





NS_IMETHODIMP
nsHttpChannel::SetTimingEnabled(bool enabled) {
    mTimingEnabled = enabled;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetTimingEnabled(bool* _retval) {
    *_retval = mTimingEnabled;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetChannelCreation(TimeStamp* _retval) {
    *_retval = mChannelCreationTimestamp;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetAsyncOpen(TimeStamp* _retval) {
    *_retval = mAsyncOpenTime;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetDomainLookupStart(TimeStamp* _retval) {
    if (mDNSPrefetch && mDNSPrefetch->TimingsValid())
        *_retval = mDNSPrefetch->StartTimestamp();
    else if (mTransaction)
        *_retval = mTransaction->Timings().domainLookupStart;
    else
        *_retval = mTransactionTimings.domainLookupStart;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetDomainLookupEnd(TimeStamp* _retval) {
    if (mDNSPrefetch && mDNSPrefetch->TimingsValid())
        *_retval = mDNSPrefetch->EndTimestamp();
    else if (mTransaction)
        *_retval = mTransaction->Timings().domainLookupEnd;
    else
        *_retval = mTransactionTimings.domainLookupEnd;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetConnectStart(TimeStamp* _retval) {
    if (mTransaction)
        *_retval = mTransaction->Timings().connectStart;
    else
        *_retval = mTransactionTimings.connectStart;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetConnectEnd(TimeStamp* _retval) {
    if (mTransaction)
        *_retval = mTransaction->Timings().connectEnd;
    else
        *_retval = mTransactionTimings.connectEnd;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetRequestStart(TimeStamp* _retval) {
    if (mTransaction)
        *_retval = mTransaction->Timings().requestStart;
    else
        *_retval = mTransactionTimings.requestStart;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetResponseStart(TimeStamp* _retval) {
    if (mTransaction)
        *_retval = mTransaction->Timings().responseStart;
    else
        *_retval = mTransactionTimings.responseStart;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetResponseEnd(TimeStamp* _retval) {
    if (mTransaction)
        *_retval = mTransaction->Timings().responseEnd;
    else
        *_retval = mTransactionTimings.responseEnd;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetCacheReadStart(TimeStamp* _retval) {
    *_retval = mCacheReadStart;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetCacheReadEnd(TimeStamp* _retval) {
    *_retval = mCacheReadEnd;
    return NS_OK;
}

#define IMPL_TIMING_ATTR(name)                                 \
NS_IMETHODIMP                                                  \
nsHttpChannel::Get##name##Time(PRTime* _retval) {              \
    TimeStamp stamp;                                  \
    Get##name(&stamp);                                         \
    if (stamp.IsNull()) {                                      \
        *_retval = 0;                                          \
        return NS_OK;                                          \
    }                                                          \
    *_retval = mChannelCreationTime +                          \
        (PRTime) ((stamp - mChannelCreationTimestamp).ToSeconds() * 1e6); \
    return NS_OK;                                              \
}

IMPL_TIMING_ATTR(ChannelCreation)
IMPL_TIMING_ATTR(AsyncOpen)
IMPL_TIMING_ATTR(DomainLookupStart)
IMPL_TIMING_ATTR(DomainLookupEnd)
IMPL_TIMING_ATTR(ConnectStart)
IMPL_TIMING_ATTR(ConnectEnd)
IMPL_TIMING_ATTR(RequestStart)
IMPL_TIMING_ATTR(ResponseStart)
IMPL_TIMING_ATTR(ResponseEnd)
IMPL_TIMING_ATTR(CacheReadStart)
IMPL_TIMING_ATTR(CacheReadEnd)

#undef IMPL_TIMING_ATTR





NS_IMETHODIMP
nsHttpChannel::GetIsSSL(bool *aIsSSL)
{
    *aIsSSL = mConnectionInfo->UsingSSL();
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetProxyMethodIsConnect(bool *aProxyMethodIsConnect)
{
    *aProxyMethodIsConnect = mConnectionInfo->UsingConnect();
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
    PROFILER_LABEL("nsHttpChannel", "OnStartRequest");
    if (!(mCanceled || NS_FAILED(mStatus))) {
        
        
        request->GetStatus(&mStatus);
    }

    LOG(("nsHttpChannel::OnStartRequest [this=%p request=%p status=%x]\n",
        this, request, mStatus));

    
    MOZ_ASSERT(request == mCachePump || request == mTransactionPump,
               "Unexpected request");
    MOZ_ASSERT(!(mTransactionPump && mCachePump) || mCachedContentIsPartial,
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
        bool waitingForRedirectCallback;
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
    PROFILER_LABEL("network", "nsHttpChannel::OnStopRequest");
    LOG(("nsHttpChannel::OnStopRequest [this=%p request=%p status=%x]\n",
        this, request, status));

    if (mTimingEnabled && request == mCachePump) {
        mCacheReadEnd = TimeStamp::Now();
    }

    
    bool contentComplete = NS_SUCCEEDED(status);

    
    if (mCanceled || NS_FAILED(mStatus))
        status = mStatus;

    if (mCachedContentIsPartial) {
        if (NS_SUCCEEDED(status)) {
            
            MOZ_ASSERT(request != mTransactionPump,
                       "byte-range transaction finished prematurely");

            if (request == mCachePump) {
                bool streamDone;
                status = OnDoneReadingPartialCacheEntry(&streamDone);
                if (NS_SUCCEEDED(status) && !streamDone)
                    return status;
                
            }
            else if (request == mTransactionPump) {
                MOZ_ASSERT(mConcurentCacheAccess);
            }
            else
                NS_NOTREACHED("unexpected request");
        }
        
        if (NS_FAILED(status) && mTransaction)
            gHttpHandler->CancelTransaction(mTransaction, status);
    }

    if (mTransaction) {
        
        bool authRetry = mAuthRetryPending && NS_SUCCEEDED(status);

        
        
        
        
        
        
        
        
        
        nsRefPtr<nsAHttpConnection> conn;
        if (authRetry && (mCaps & NS_HTTP_STICKY_CONNECTION)) {
            conn = mTransaction->Connection();
            
            
            
            if (conn && !conn->IsPersistent())
                conn = nullptr;
        }

        nsRefPtr<nsAHttpConnection> stickyConn;
        if (mCaps & NS_HTTP_STICKY_CONNECTION)
            stickyConn = mTransaction->Connection();

        
        mTransactionTimings = mTransaction->Timings();
        mTransaction = nullptr;
        mTransactionPump = nullptr;

        
        if (mDNSPrefetch && mDNSPrefetch->TimingsValid()) {
            mTransactionTimings.domainLookupStart =
                mDNSPrefetch->StartTimestamp();
            mTransactionTimings.domainLookupEnd =
                mDNSPrefetch->EndTimestamp();
        }
        mDNSPrefetch = nullptr;

        
        if (authRetry) {
            mAuthRetryPending = false;
            status = DoAuthRetry(conn);
            if (NS_SUCCEEDED(status))
                return NS_OK;
        }

        
        
        if (authRetry || (mAuthRetryPending && NS_FAILED(status))) {
            MOZ_ASSERT(NS_FAILED(status), "should have a failure code here");
            
            
            if (mListener) {
                mListener->OnStartRequest(this, mListenerContext);
            } else {
                NS_WARNING("OnStartRequest skipped because of null listener");
            }
        }

        
        if (mTransactionReplaced)
            return NS_OK;

        if (mUpgradeProtocolCallback && stickyConn &&
            mResponseHead && mResponseHead->Status() == 101) {
            gHttpHandler->ConnMgr()->CompleteUpgrade(stickyConn,
                                                     mUpgradeProtocolCallback);
        }
    }

    mIsPending = false;

    
    if (mCacheEntry && mCachePump &&
        mConcurentCacheAccess && contentComplete) {
        int64_t size, contentLength;
        nsresult rv = CheckPartial(mCacheEntry, &size, &contentLength);
        if (NS_SUCCEEDED(rv)) {
            if (size == int64_t(-1)) {
                
                MOZ_ASSERT(false);
                LOG(("  cache entry write is still in progress, but we just "
                     "finished reading the cache entry"));
            }
            else if (contentLength != int64_t(-1) && contentLength != size) {
                LOG(("  concurrent cache entry write has been interrupted"));
                mCachedResponseHead = mResponseHead;
                rv = MaybeSetupByteRangeRequest(size, contentLength);
                if (NS_SUCCEEDED(rv) && mIsPartialRequest) {
                    
                    mCachedContentIsValid = 0;
                    mCachedContentIsPartial = 1;

                    
                    rv = ContinueConnect();
                    if (NS_SUCCEEDED(rv)) {
                        LOG(("  performing range request"));
                        mCachePump = nullptr;
                        return NS_OK;
                    } else {
                        LOG(("  but range request perform failed 0x%08x", rv));
                        status = NS_ERROR_NET_INTERRUPT;
                    }
                }
                else {
                    LOG(("  but range request setup failed rv=0x%08x, failing load", rv));
                }
            }
        }
    }

    mStatus = status;

    
    if (mCacheEntry && mRequestTimeInitialized) {
        bool writeAccess;
        
        
        mCacheEntry->HasWriteAccess(!mCacheEntryIsReadOnly, &writeAccess);
        if (writeAccess) {
            FinalizeCacheEntry();
        }
    }

    if (mListener) {
        LOG(("  calling OnStopRequest\n"));
        mListener->OnStopRequest(this, mListenerContext, status);
    }

    MOZ_EVENT_TRACER_DONE(this, "net::http::channel");

    CloseCacheEntry(!contentComplete);

    if (mOfflineCacheEntry)
        CloseOfflineCacheEntry();

    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nullptr, status);

    
    CleanRedirectCacheChainIfNecessary();

    ReleaseListeners();

    return NS_OK;
}





class OnTransportStatusAsyncEvent : public nsRunnable
{
public:
    OnTransportStatusAsyncEvent(nsITransportEventSink* aEventSink,
                                nsresult aTransportStatus,
                                uint64_t aProgress,
                                uint64_t aProgressMax)
    : mEventSink(aEventSink)
    , mTransportStatus(aTransportStatus)
    , mProgress(aProgress)
    , mProgressMax(aProgressMax)
    {
        MOZ_ASSERT(!NS_IsMainThread(), "Shouldn't be created on main thread");
    }

    NS_IMETHOD Run()
    {
        MOZ_ASSERT(NS_IsMainThread(), "Should run on main thread");
        if (mEventSink) {
            mEventSink->OnTransportStatus(nullptr, mTransportStatus,
                                          mProgress, mProgressMax);
        }
        return NS_OK;
    }
private:
    nsCOMPtr<nsITransportEventSink> mEventSink;
    nsresult mTransportStatus;
    uint64_t mProgress;
    uint64_t mProgressMax;
};

NS_IMETHODIMP
nsHttpChannel::OnDataAvailable(nsIRequest *request, nsISupports *ctxt,
                               nsIInputStream *input,
                               uint64_t offset, uint32_t count)
{
    PROFILER_LABEL("network", "nsHttpChannel::OnDataAvailable");
    LOG(("nsHttpChannel::OnDataAvailable [this=%p request=%p offset=%llu count=%u]\n",
        this, request, offset, count));

    
    if (mCanceled)
        return mStatus;

    MOZ_ASSERT(mResponseHead, "No response head in ODA!!");

    MOZ_ASSERT(!(mCachedContentIsPartial && (request == mTransactionPump)),
               "transaction pump not suspended");

    if (mAuthRetryPending || (request == mTransactionPump && mTransactionReplaced)) {
        uint32_t n;
        return input->ReadSegments(NS_DiscardSegment, nullptr, count, &n);
    }

    if (mListener) {
        
        
        
        
        
        nsresult transportStatus;
        if (request == mCachePump)
            transportStatus = NS_NET_STATUS_READING;
        else
            transportStatus = NS_NET_STATUS_RECEIVING_FROM;

        
        
        
        

        uint64_t progressMax(uint64_t(mResponseHead->ContentLength()));
        uint64_t progress = mLogicalOffset + uint64_t(count);
        MOZ_ASSERT(progress <= progressMax, "unexpected progress values");

        if (NS_IsMainThread()) {
            OnTransportStatus(nullptr, transportStatus, progress, progressMax);
        } else {
            nsresult rv = NS_DispatchToMainThread(
                new OnTransportStatusAsyncEvent(this, transportStatus,
                                                progress, progressMax));
            NS_ENSURE_SUCCESS(rv, rv);
        }

        
        
        
        
        
        
        if (!mLogicalOffset)
            MOZ_EVENT_TRACER_EXEC(this, "net::http::channel");

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
nsHttpChannel::RetargetDeliveryTo(nsIEventTarget* aNewTarget)
{
    MOZ_ASSERT(NS_IsMainThread(), "Should be called on main thread only");

    NS_ENSURE_ARG(aNewTarget);
    if (aNewTarget == NS_GetCurrentThread()) {
        NS_WARNING("Retargeting delivery to same thread");
        return NS_OK;
    }
    NS_ENSURE_TRUE(mTransactionPump || mCachePump, NS_ERROR_NOT_AVAILABLE);

    nsresult rv = NS_OK;
    
    
    nsCOMPtr<nsIThreadRetargetableRequest> retargetableCachePump;
    nsCOMPtr<nsIThreadRetargetableRequest> retargetableTransactionPump;
    if (mCachePump) {
        retargetableCachePump = do_QueryObject(mCachePump);
        
        MOZ_ASSERT(retargetableCachePump);
        rv = retargetableCachePump->RetargetDeliveryTo(aNewTarget);
    }
    if (NS_SUCCEEDED(rv) && mTransactionPump) {
        retargetableTransactionPump = do_QueryObject(mTransactionPump);
        
        MOZ_ASSERT(retargetableTransactionPump);
        rv = retargetableTransactionPump->RetargetDeliveryTo(aNewTarget);

        
        if (NS_FAILED(rv) && retargetableCachePump) {
            nsCOMPtr<nsIThread> mainThread;
            rv = NS_GetMainThread(getter_AddRefs(mainThread));
            NS_ENSURE_SUCCESS(rv, rv);
            rv = retargetableCachePump->RetargetDeliveryTo(mainThread);
        }
    }
    return rv;
}





NS_IMETHODIMP
nsHttpChannel::CheckListenerChain()
{
    NS_ASSERTION(NS_IsMainThread(), "Should be on main thread!");
    nsresult rv = NS_OK;
    nsCOMPtr<nsIThreadRetargetableStreamListener> retargetableListener =
        do_QueryInterface(mListener, &rv);
    if (retargetableListener) {
        rv = retargetableListener->CheckListenerChain();
    }
    return rv;
}





NS_IMETHODIMP
nsHttpChannel::OnTransportStatus(nsITransport *trans, nsresult status,
                                 uint64_t progress, uint64_t progressMax)
{
    MOZ_ASSERT(NS_IsMainThread(), "Should be on main thread only");
    
    if (!mProgressSink)
        GetCallback(mProgressSink);

    if (status == NS_NET_STATUS_CONNECTED_TO ||
        status == NS_NET_STATUS_WAITING_FOR) {
        nsCOMPtr<nsISocketTransport> socketTransport =
            do_QueryInterface(trans);
        if (socketTransport) {
            socketTransport->GetSelfAddr(&mSelfAddr);
            socketTransport->GetPeerAddr(&mPeerAddr);
        }
    }

    
    if (mProgressSink && NS_SUCCEEDED(mStatus) && mIsPending && !(mLoadFlags & LOAD_BACKGROUND)) {
        LOG(("sending status notification [this=%p status=%x progress=%llu/%llu]\n",
            this, status, progress, progressMax));

        nsAutoCString host;
        mURI->GetHost(host);
        mProgressSink->OnStatus(this, nullptr, status,
                                NS_ConvertUTF8toUTF16(host).get());

        if (progress > 0) {
            MOZ_ASSERT(progress <= progressMax, "unexpected progress values");
            
            if (!mProgressSink) {
                GetCallback(mProgressSink);
            }
            if (mProgressSink) {
                mProgressSink->OnProgress(this, nullptr, progress, progressMax);
            }
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
nsHttpChannel::IsFromCache(bool *value)
{
    if (!mIsPending)
        return NS_ERROR_NOT_AVAILABLE;

    
    

    *value = (mCachePump || (mLoadFlags & LOAD_ONLY_IF_MODIFIED)) &&
              mCachedContentIsValid && !mCachedContentIsPartial;

    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetCacheTokenExpirationTime(uint32_t *_retval)
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

class nsHttpChannelCacheKey MOZ_FINAL : public nsISupportsPRUint32,
                                        public nsISupportsCString
{
    NS_DECL_ISUPPORTS

    NS_DECL_NSISUPPORTSPRIMITIVE
    NS_FORWARD_NSISUPPORTSPRUINT32(mSupportsPRUint32->)

    
    
    NS_IMETHOD GetData(nsACString & aData)
    {
        return mSupportsCString->GetData(aData);
    }
    NS_IMETHOD SetData(const nsACString & aData)
    {
        return mSupportsCString->SetData(aData);
    }

public:
    nsresult SetData(uint32_t aPostID, const nsACString& aKey);

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

NS_IMETHODIMP nsHttpChannelCacheKey::GetType(uint16_t *aType)
{
    NS_ENSURE_ARG_POINTER(aType);

    *aType = TYPE_PRUINT32;
    return NS_OK;
}

nsresult nsHttpChannelCacheKey::SetData(uint32_t aPostID,
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

    *key = nullptr;

    nsRefPtr<nsHttpChannelCacheKey> container =
        new nsHttpChannelCacheKey();

    if (!container)
        return NS_ERROR_OUT_OF_MEMORY;

    nsAutoCString cacheKey;
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

    ENSURE_CALLED_BEFORE_CONNECT();

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
nsHttpChannel::ResumeAt(uint64_t aStartPos,
                        const nsACString& aEntityID)
{
    LOG(("nsHttpChannel::ResumeAt [this=%p startPos=%llu id='%s']\n",
         this, aStartPos, PromiseFlatCString(aEntityID).get()));
    mEntityID = aEntityID;
    mStartPos = aStartPos;
    mResuming = true;
    return NS_OK;
}

nsresult
nsHttpChannel::DoAuthRetry(nsAHttpConnection *conn)
{
    LOG(("nsHttpChannel::DoAuthRetry [this=%p]\n", this));

    MOZ_ASSERT(!mTransaction, "should not have a transaction");
    nsresult rv;

    
    
    mIsPending = false;

    
    
    
    
    AddCookiesToRequest();

    
    CallOnModifyRequestObservers();

    mIsPending = true;

    
    mResponseHead = nullptr;

    
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

    rv = mTransactionPump->AsyncRead(this, nullptr);
    if (NS_FAILED(rv)) return rv;

    uint32_t suspendCount = mSuspendCount;
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
    ENSURE_CALLED_BEFORE_CONNECT();

    mApplicationCache = appCache;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetApplicationCacheForWrite(nsIApplicationCache **out)
{
    NS_IF_ADDREF(*out = mApplicationCacheForWrite);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetApplicationCacheForWrite(nsIApplicationCache *appCache)
{
    ENSURE_CALLED_BEFORE_CONNECT();

    mApplicationCacheForWrite = appCache;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetLoadedFromApplicationCache(bool *aLoadedFromApplicationCache)
{
    *aLoadedFromApplicationCache = mLoadedFromApplicationCache;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetInheritApplicationCache(bool *aInherit)
{
    *aInherit = mInheritApplicationCache;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetInheritApplicationCache(bool aInherit)
{
    ENSURE_CALLED_BEFORE_CONNECT();

    mInheritApplicationCache = aInherit;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetChooseApplicationCache(bool *aChoose)
{
    *aChoose = mChooseApplicationCache;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetChooseApplicationCache(bool aChoose)
{
    ENSURE_CALLED_BEFORE_CONNECT();

    mChooseApplicationCache = aChoose;
    return NS_OK;
}

nsHttpChannel::OfflineCacheEntryAsForeignMarker*
nsHttpChannel::GetOfflineCacheEntryAsForeignMarker()
{
    if (!mApplicationCache)
        return nullptr;

    return new OfflineCacheEntryAsForeignMarker(mApplicationCache, mURI);
}

nsresult
nsHttpChannel::OfflineCacheEntryAsForeignMarker::MarkAsForeign()
{
    nsresult rv;

    nsCOMPtr<nsIURI> noRefURI;
    rv = mCacheURI->CloneIgnoringRef(getter_AddRefs(noRefURI));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoCString spec;
    rv = noRefURI->GetAsciiSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    return mApplicationCache->MarkEntry(spec,
                                        nsIApplicationCache::ITEM_FOREIGN);
}

NS_IMETHODIMP
nsHttpChannel::MarkOfflineCacheEntryAsForeign()
{
    nsresult rv;

    nsAutoPtr<OfflineCacheEntryAsForeignMarker> marker(
        GetOfflineCacheEntryAsForeignMarker());

    if (!marker)
        return NS_ERROR_NOT_AVAILABLE;

    rv = marker->MarkAsForeign();
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
            MOZ_ASSERT(NS_SUCCEEDED(resume),
                       "Failed to resume transaction pump");
        }
        NS_ENSURE_SUCCESS(rv, rv);
    }

    mWaitingForRedirectCallback = true;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::OnRedirectVerifyCallback(nsresult result)
{
    LOG(("nsHttpChannel::OnRedirectVerifyCallback [this=%p] "
         "result=%x stack=%d mWaitingForRedirectCallback=%u\n",
         this, result, mRedirectFuncStack.Length(), mWaitingForRedirectCallback));
    MOZ_ASSERT(mWaitingForRedirectCallback,
               "Someone forgot to call WaitForRedirectCallback() ?!");
    mWaitingForRedirectCallback = false;

    if (mCanceled && NS_SUCCEEDED(result))
        result = NS_BINDING_ABORTED;

    for (uint32_t i = mRedirectFuncStack.Length(); i > 0;) {
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
        
        
        mRedirectChannel = nullptr;
        MOZ_EVENT_TRACER_DONE(this, "net::http::channel");
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
    MOZ_ASSERT(func == mRedirectFuncStack[mRedirectFuncStack.Length() - 1],
               "Trying to pop wrong method from redirect async stack!");

    mRedirectFuncStack.TruncateLength(mRedirectFuncStack.Length() - 1);
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


    
#ifdef PR_LOGGING
    nsAutoCString key;
    mURI->GetAsciiSpec(key);
    LOG(("MaybeInvalidateCacheEntryForSubsequentGet [this=%p uri=%s]\n",
        this, key.get()));
#endif

    DoInvalidateCacheEntry(mURI);

    
    const char *location = mResponseHead->PeekHeader(nsHttp::Location);
    if (location) {
        LOG(("  Location-header=%s\n", location));
        InvalidateCacheEntryForLocation(location);
    }

    
    location = mResponseHead->PeekHeader(nsHttp::Content_Location);
    if (location) {
        LOG(("  Content-Location-header=%s\n", location));
        InvalidateCacheEntryForLocation(location);
    }
}

void
nsHttpChannel::InvalidateCacheEntryForLocation(const char *location)
{
    nsAutoCString tmpCacheKey, tmpSpec;
    nsCOMPtr<nsIURI> resultingURI;
    nsresult rv = CreateNewURI(location, getter_AddRefs(resultingURI));
    if (NS_SUCCEEDED(rv) && HostPartIsTheSame(resultingURI)) {
        DoInvalidateCacheEntry(resultingURI);
    } else {
        LOG(("  hosts not matching\n"));
    }
}

void
nsHttpChannel::DoInvalidateCacheEntry(nsIURI* aURI)
{
    
    
    
    
    

    nsresult rv;

#ifdef PR_LOGGING
    nsAutoCString key;
    aURI->GetAsciiSpec(key);
    LOG(("DoInvalidateCacheEntry [channel=%p key=%s]", this, key.get()));
#endif

    nsCOMPtr<nsICacheStorageService> cacheStorageService =
        do_GetService("@mozilla.org/netwerk/cache-storage-service;1", &rv);

    nsCOMPtr<nsICacheStorage> cacheStorage;
    if (NS_SUCCEEDED(rv)) {
        nsRefPtr<LoadContextInfo> info = GetLoadContextInfo(this);
        rv = cacheStorageService->DiskCacheStorage(info, false, getter_AddRefs(cacheStorage));
    }

    if (NS_SUCCEEDED(rv)) {
        rv = cacheStorage->AsyncDoomURI(aURI, EmptyCString(), nullptr);
    }

    LOG(("DoInvalidateCacheEntry [channel=%p key=%s rv=%d]", this, key.get(), int(rv)));
}

void
nsHttpChannel::AsyncOnExamineCachedResponse()
{
    gHttpHandler->OnExamineCachedResponse(this);

}

void
nsHttpChannel::UpdateAggregateCallbacks()
{
    if (!mTransaction) {
        return;
    }
    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    NS_NewNotificationCallbacksAggregation(mCallbacks, mLoadGroup,
                                           NS_GetCurrentThread(),
                                           getter_AddRefs(callbacks));
    mTransaction->SetSecurityCallbacks(callbacks);
}

nsIPrincipal *
nsHttpChannel::GetPrincipal()
{
    if (mPrincipal)
        return mPrincipal;

    nsIScriptSecurityManager *securityManager =
        nsContentUtils::GetSecurityManager();

    if (!securityManager)
        return nullptr;

    securityManager->GetChannelPrincipal(this, getter_AddRefs(mPrincipal));
    if (!mPrincipal)
        return nullptr;

    
    if (mPrincipal->GetUnknownAppId())
        mPrincipal = nullptr;

    return mPrincipal;
}


NS_IMETHODIMP
nsHttpChannel::SetLoadGroup(nsILoadGroup *aLoadGroup)
{
    MOZ_ASSERT(NS_IsMainThread(), "Wrong thread.");

    nsresult rv = HttpBaseChannel::SetLoadGroup(aLoadGroup);
    if (NS_SUCCEEDED(rv)) {
        UpdateAggregateCallbacks();
    }
    return rv;
}

NS_IMETHODIMP
nsHttpChannel::SetNotificationCallbacks(nsIInterfaceRequestor *aCallbacks)
{
    MOZ_ASSERT(NS_IsMainThread(), "Wrong thread.");

    nsresult rv = HttpBaseChannel::SetNotificationCallbacks(aCallbacks);
    if (NS_SUCCEEDED(rv)) {
        UpdateAggregateCallbacks();
    }
    return rv;
}

} } 
