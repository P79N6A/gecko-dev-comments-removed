





#include "nsHttp.h"
#include "nsHttpChannel.h"
#include "nsHttpHandler.h"
#include "nsStandardURL.h"
#include "nsIApplicationCacheService.h"
#include "nsIApplicationCacheContainer.h"
#include "nsIAuthInformation.h"
#include "nsICryptoHash.h"
#include "nsIStringBundle.h"
#include "nsIStreamListenerTee.h"
#include "nsISeekableStream.h"
#include "nsMimeTypes.h"
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
#include "mozilla/TimeStamp.h"
#include "nsError.h"
#include "nsAlgorithm.h"
#include "sampler.h"
#include "nsIConsoleService.h"
#include "base/compiler_specific.h"
#include "NullHttpTransaction.h"
#include "mozilla/Attributes.h"

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

void
MaybeMarkCacheEntryValid(const void * channel,
                         nsICacheEntryDescriptor * cacheEntry,
                         nsCacheAccessMode cacheAccess)
{
    
    
    if (cacheAccess & nsICache::ACCESS_WRITE) {
        nsresult rv = cacheEntry->MarkValid();
        LOG(("Marking cache entry valid "
             "[channel=%p, entry=%p, access=%d, result=%d]",
             channel, cacheEntry, int(cacheAccess), int(rv)));
    } else {
        LOG(("Not marking read-only cache entry valid "
             "[channel=%p, entry=%p, access=%d]", 
             channel, cacheEntry, int(cacheAccess)));
    }
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
    mChannel = nullptr;
    if (vetoHook)
        vetoHook->OnRedirectResult(succeeded);
}

class HttpCacheQuery : public nsRunnable, public nsICacheListener
{
public:
    HttpCacheQuery(nsHttpChannel * channel,
                   const nsACString & clientID,
                   nsCacheStoragePolicy storagePolicy,
                   bool usingPrivateBrowsing,
                   const nsACString & cacheKey,
                   nsCacheAccessMode accessToRequest,
                   bool noWait,
                   bool usingSSL,
                   bool loadedFromApplicationCache)
        
        : mChannel(channel)
        , mHasQueryString(HasQueryString(channel->mRequestHead.Method(),
                                         channel->mURI))
        , mLoadFlags(channel->mLoadFlags)
        , mCacheForOfflineUse(!!channel->mApplicationCacheForWrite)
        , mFallbackChannel(channel->mFallbackChannel)
        , mClientID(clientID)
        , mStoragePolicy(storagePolicy)
        , mUsingPrivateBrowsing(usingPrivateBrowsing)
        , mCacheKey(cacheKey)
        , mAccessToRequest(accessToRequest)
        , mNoWait(noWait)
        , mUsingSSL(usingSSL)
        , mLoadedFromApplicationCache(loadedFromApplicationCache)
        
        , mCacheAccess(0)
        , mStatus(NS_ERROR_NOT_INITIALIZED)
        , mRunCount(0)
        
        , mRequestHead(channel->mRequestHead)
        , mRedirectedCachekeys(channel->mRedirectedCachekeys.forget())
        
        , mCachedContentIsValid(false)
        , mCachedContentIsPartial(false)
        , mCustomConditionalRequest(false)
        , mDidReval(false)
        , mCacheEntryDeviceTelemetryID(UNKNOWN_DEVICE)
    {
        MOZ_ASSERT(NS_IsMainThread());
    }

    nsresult Dispatch();

private:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIRUNNABLE
    NS_DECL_NSICACHELISTENER

    MOZ_ALWAYS_INLINE void AssertOnCacheThread() const
    {
        MOZ_ASSERT(mCacheThread);
#ifdef DEBUG
        bool onCacheThread;
        nsresult rv = mCacheThread->IsOnCurrentThread(&onCacheThread);
        MOZ_ASSERT(NS_SUCCEEDED(rv));
        MOZ_ASSERT(onCacheThread);
#endif
    }

    static bool HasQueryString(nsHttpAtom method, nsIURI * uri);
    nsresult CheckCache();
    bool ResponseWouldVary() const;
    bool MustValidateBasedOnQueryUrl() const;
    nsresult SetupByteRangeRequest(uint32_t partialLen);
    nsresult OpenCacheInputStream(bool startBuffering);

    nsCOMPtr<nsICacheListener> mChannel;
    const bool mHasQueryString;
    const uint32_t mLoadFlags;
    const bool mCacheForOfflineUse;
    const bool mFallbackChannel;
    const InfallableCopyCString mClientID;
    const nsCacheStoragePolicy mStoragePolicy;
    const bool mUsingPrivateBrowsing;
    const InfallableCopyCString mCacheKey;
    const nsCacheAccessMode mAccessToRequest;
    const bool mNoWait;
    const bool mUsingSSL;
    const bool mLoadedFromApplicationCache;

    
    nsCOMPtr<nsIEventTarget> mCacheThread;
    nsCOMPtr<nsICacheEntryDescriptor> mCacheEntry;
    nsCacheAccessMode mCacheAccess;
    nsresult mStatus;
    uint32_t mRunCount;

    
    friend class nsHttpChannel;
     nsHttpRequestHead mRequestHead;
     nsAutoPtr<nsTArray<nsCString> > mRedirectedCachekeys;
     AutoClose<nsIInputStream> mCacheInputStream;
     nsAutoPtr<nsHttpResponseHead> mCachedResponseHead;
     nsCOMPtr<nsISupports> mCachedSecurityInfo;
     bool mCachedContentIsValid;
     bool mCachedContentIsPartial;
     bool mCustomConditionalRequest;
     bool mDidReval;
     Telemetry::ID mCacheEntryDeviceTelemetryID;
};

NS_IMPL_ISUPPORTS_INHERITED1(HttpCacheQuery, nsRunnable, nsICacheListener)





nsHttpChannel::nsHttpChannel()
    : ALLOW_THIS_IN_INITIALIZER_LIST(HttpAsyncAborter<nsHttpChannel>(this))
    , mLogicalOffset(0)
    , mCacheAccess(0)
    , mCacheEntryDeviceTelemetryID(UNKNOWN_DEVICE)
    , mPostID(0)
    , mRequestTime(0)
    , mOnCacheEntryAvailableCallback(nullptr)
    , mOfflineCacheAccess(0)
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
        
        nsIStrictTransportSecurityService* stss = gHttpHandler->GetSTSService();
        NS_ENSURE_TRUE(stss, NS_ERROR_OUT_OF_MEMORY);

        bool isStsHost = false;
        uint32_t flags = mPrivateBrowsing ? nsISocketProvider::NO_PERMANENT_STORAGE : 0;
        rv = stss->IsStsURI(mURI, flags, &isStsHost);

        
        
        NS_ASSERTION(NS_SUCCEEDED(rv),
                     "Something is wrong with STS: IsStsURI failed.");

        if (NS_SUCCEEDED(rv) && isStsHost) {
            LOG(("nsHttpChannel::Connect() STS permissions found\n"));
            return AsyncCall(&nsHttpChannel::HandleAsyncRedirectChannelToHttps);
        }

        
        if (gHttpHandler->IsSpdyEnabled() && mAllowSpdy) {
            nsAutoCString hostPort;

            if (NS_SUCCEEDED(mURI->GetHostPort(hostPort)) &&
                gHttpHandler->ConnMgr()->GetSpdyAlternateProtocol(hostPort)) {
                LOG(("nsHttpChannel::Connect() Alternate-Protocol found\n"));
                return AsyncCall(
                    &nsHttpChannel::HandleAsyncRedirectChannelToHttps);
            }
        }
    }

    
    if (!net_IsValidHostName(nsDependentCString(mConnectionInfo->Host())))
        return NS_ERROR_UNKNOWN_HOST;

    
    SpeculativeConnect();

    
    if (mResuming && (mLoadFlags & LOAD_ONLY_FROM_CACHE)) {
        LOG(("Resuming from cache is not supported yet"));
        return NS_ERROR_DOCUMENT_NOT_CACHED;
    }

    if (!gHttpHandler->UseCache())
        return ContinueConnect();

    
    rv = OpenCacheEntry(usingSSL);

    
    if (mOnCacheEntryAvailableCallback) {
        NS_ASSERTION(NS_SUCCEEDED(rv), "Unexpected state");
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

    
    
    if (mApplicationCacheForWrite) {
        rv = OpenOfflineCacheEntryForWriting();
        if (NS_FAILED(rv)) return rv;

        if (mOnCacheEntryAvailableCallback)
            return NS_OK;
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
            
            
            
            
            return NS_ERROR_DOCUMENT_NOT_CACHED;
        }
    }
    else if (mLoadFlags & LOAD_ONLY_FROM_CACHE) {
        
        
        if (!mFallbackChannel && !mFallbackKey.IsEmpty()) {
            return AsyncCall(&nsHttpChannel::HandleAsyncFallback);
        }
        return NS_ERROR_DOCUMENT_NOT_CACHED;
    }

    if (mLoadFlags & LOAD_NO_NETWORK_IO) {
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

    mConnectionInfo->SetAnonymous((mLoadFlags & LOAD_ANONYMOUS) != 0);
    mConnectionInfo->SetPrivate(mPrivateBrowsing);
    gHttpHandler->SpeculativeConnect(mConnectionInfo,
                                     callbacks);
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
    
    
    
    nsCOMPtr<nsILoadGroup> rootLoadGroup = mLoadGroup;
    while (rootLoadGroup) {
        nsCOMPtr<nsILoadGroup> tmp;
        rootLoadGroup->GetLoadGroup(getter_AddRefs(tmp));
        if (tmp)
            rootLoadGroup.swap(tmp);
        else
            break;
    }

    
    if (rootLoadGroup) {
        nsCOMPtr<nsILoadGroupConnectionInfo> ci;
        rootLoadGroup->GetConnectionInfo(getter_AddRefs(ci));
        if (ci)
            mTransaction->SetLoadGroupConnectionInfo(ci);
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
    else if ((mLoadFlags & VALIDATE_ALWAYS) && (mCacheAccess & nsICache::ACCESS_READ)) {
        
        
        
        
        
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

    mConnectionInfo->SetAnonymous((mLoadFlags & LOAD_ANONYMOUS) != 0);
    mConnectionInfo->SetPrivate(mPrivateBrowsing);

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
        NS_ASSERTION(mConnectionInfo, "Should have connection info here");
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
    nsresult rv = mListener->OnStartRequest(this, mListenerContext);
    if (NS_FAILED(rv)) return rv;

    
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
    
    
    
    
    
    
    
    
    
    

    NS_ABORT_IF_FALSE(mConnectionInfo->UsingConnect(),
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

    nsIStrictTransportSecurityService* stss = gHttpHandler->GetSTSService();
    NS_ENSURE_TRUE(stss, NS_ERROR_OUT_OF_MEMORY);

    
    
    
    NS_ENSURE_TRUE(mSecurityInfo, NS_OK);

    
    
    
    bool tlsIsBroken = false;
    rv = stss->ShouldIgnoreStsHeader(mSecurityInfo, &tlsIsBroken);
    NS_ENSURE_SUCCESS(rv, NS_OK);

    
    
    
    
    
    bool wasAlreadySTSHost;
    uint32_t flags =
      NS_UsePrivateBrowsing(this) ? nsISocketProvider::NO_PERMANENT_STORAGE : 0;
    rv = stss->IsStsURI(mURI, flags, &wasAlreadySTSHost);
    
    
    NS_ENSURE_SUCCESS(rv, NS_OK);
    NS_ASSERTION(!(wasAlreadySTSHost && tlsIsBroken),
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

    rv = stss->ProcessStsHeader(mURI, stsHeader.get(), flags, NULL, NULL);
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
        NS_ASSERTION(NS_SUCCEEDED(rv), "ProcessSTSHeader failed, continuing load.");
    }

    MOZ_ASSERT(!mCachedContentIsValid);

    
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

    return pps->AsyncResolve(mProxyURI ? mProxyURI : mURI, mProxyResolveFlags,
                             this, getter_AddRefs(mProxyRequest));
}

bool
HttpCacheQuery::ResponseWouldVary() const
{
    AssertOnCacheThread();

    nsresult rv;
    nsAutoCString buf, metaKey;
    mCachedResponseHead->GetHeader(nsHttp::Vary, buf);
    if (!buf.IsEmpty()) {
        NS_NAMED_LITERAL_CSTRING(prefix, "request-");

        
        char *val = buf.BeginWriting(); 
        char *token = nsCRT::strtok(val, NS_HTTP_HEADER_SEPS, &val);
        while (token) {
            LOG(("HttpCacheQuery::ResponseWouldVary [channel=%p] " \
                 "processing %s\n",
                 mChannel.get(), token));
            
            
            
            
            
            
            
            
            
            
            
            
            
            if (*token == '*')
                return true; 

            
            metaKey = prefix + nsDependentCString(token);

            
            
            nsXPIDLCString lastVal;
            mCacheEntry->GetMetaDataElement(metaKey.get(), getter_Copies(lastVal));
            LOG(("HttpCacheQuery::ResponseWouldVary [channel=%p] "
                     "stored value = \"%s\"\n",
                 mChannel.get(), lastVal.get()));

            
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

                    LOG(("HttpCacheQuery::ResponseWouldVary [this=%p] " \
                            "set-cookie value hashed to %s\n",
                         mChannel.get(), newVal));
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
    
    
    int32_t methodlen = PL_strlen(mRequestHead.Method().get());
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
HttpCacheQuery::SetupByteRangeRequest(uint32_t partialLen)
{
    AssertOnCacheThread();

    
    

    
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
    return ReadFromCache(false);
}

nsresult
nsHttpChannel::OnDoneReadingPartialCacheEntry(bool *streamDone)
{
    nsresult rv;

    LOG(("nsHttpChannel::OnDoneReadingPartialCacheEntry [this=%p]", this));

    
    *streamDone = true;

    
    uint32_t size;
    rv = mCacheEntry->GetDataSize(&size);
    if (NS_FAILED(rv)) return rv;

    rv = InstallCacheListener(size);
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

    NS_ASSERTION(fallbackEntryType & nsIApplicationCache::ITEM_FALLBACK,
                 "Fallback entry not marked correctly!");

    
    
    if (mOfflineCacheEntry) {
        mOfflineCacheEntry->AsyncDoom(nullptr);
        mOfflineCacheEntry = 0;
        mOfflineCacheAccess = 0;
    }

    mApplicationCacheForWrite = nullptr;
    mOfflineCacheEntry = 0;
    mOfflineCacheAccess = 0;

    
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
    nsresult rv;

    NS_ASSERTION(!mOnCacheEntryAvailableCallback, "Unexpected state");
    mLoadedFromApplicationCache = false;

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
            nsCOMPtr<nsILoadContext> loadContext;
            GetCallback(loadContext);

            if (!loadContext)
                LOG(("  no load context while choosing application cache"));

            nsresult rv = appCacheService->ChooseApplicationCache
                (cacheKey, loadContext, getter_AddRefs(mApplicationCache));
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    nsCOMPtr<nsICacheSession> session;

    
    if (mApplicationCache) {
        nsAutoCString appCacheClientID;
        rv = mApplicationCache->GetClientID(appCacheClientID);
        if (NS_SUCCEEDED(rv)) {
            
            
            
            mCacheQuery = new HttpCacheQuery(
                                this, appCacheClientID,
                                nsICache::STORE_OFFLINE, mPrivateBrowsing,
                                cacheKey, nsICache::ACCESS_READ,
                                mLoadFlags & LOAD_BYPASS_LOCAL_CACHE_IF_BUSY,
                                usingSSL, true);

            mOnCacheEntryAvailableCallback =
                &nsHttpChannel::OnOfflineCacheEntryAvailable;

            rv = mCacheQuery->Dispatch();

            if (NS_SUCCEEDED(rv))
                return NS_OK;

            mCacheQuery = nullptr;
            mOnCacheEntryAvailableCallback = nullptr;
        }

        
        return OnOfflineCacheEntryAvailable(nullptr, nsICache::ACCESS_NONE, rv);
    }

    return OpenNormalCacheEntry(usingSSL);
}

nsresult
nsHttpChannel::OnOfflineCacheEntryAvailable(nsICacheEntryDescriptor *aEntry,
                                            nsCacheAccessMode aAccess,
                                            nsresult aEntryStatus)
{
    nsresult rv;

    if (NS_SUCCEEDED(aEntryStatus)) {
        
        
        mLoadedFromApplicationCache = true;
        mCacheEntry = aEntry;
        mCacheAccess = aAccess;
    }

    
    
    if (aEntryStatus == NS_ERROR_CACHE_WAIT_FOR_VALIDATION) {
        LOG(("bypassing local cache since it is busy\n"));
        
        return NS_ERROR_NOT_AVAILABLE;
    }

    if (mCanceled && NS_FAILED(mStatus)) {
        LOG(("channel was canceled [this=%p status=%x]\n", this, mStatus));
        return mStatus;
    }

    if (NS_SUCCEEDED(aEntryStatus))
        return NS_OK;

    if (!mApplicationCacheForWrite && !mFallbackChannel) {
        nsAutoCString cacheKey;
        GenerateCacheKey(mPostID, cacheKey);

        
        nsCOMPtr<nsIApplicationCacheNamespace> namespaceEntry;
        rv = mApplicationCache->GetMatchingNamespace
            (cacheKey, getter_AddRefs(namespaceEntry));
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

    bool usingSSL = false;
    (void) mURI->SchemeIs("https", &usingSSL);
    return OpenNormalCacheEntry(usingSSL);
}

nsresult
nsHttpChannel::OpenNormalCacheEntry(bool usingSSL)
{
    NS_ASSERTION(!mCacheEntry, "We have already mCacheEntry");

    nsresult rv;

    uint32_t appId = NECKO_NO_APP_ID;
    bool isInBrowser = false;
    NS_GetAppInfo(this, &appId, &isInBrowser);

    nsCacheStoragePolicy storagePolicy = DetermineStoragePolicy();
    nsAutoCString clientID;
    nsHttpHandler::GetCacheSessionNameForStoragePolicy(storagePolicy, mPrivateBrowsing,
                                                       appId, isInBrowser, clientID);

    nsAutoCString cacheKey;
    GenerateCacheKey(mPostID, cacheKey);

    nsCacheAccessMode accessRequested;
    rv = DetermineCacheAccess(&accessRequested);
    if (NS_FAILED(rv))
        return rv;
 
    mCacheQuery = new HttpCacheQuery(
                                this, clientID, storagePolicy,
                                mPrivateBrowsing, cacheKey, accessRequested,
                                mLoadFlags & LOAD_BYPASS_LOCAL_CACHE_IF_BUSY,
                                usingSSL, false);

    mOnCacheEntryAvailableCallback =
        &nsHttpChannel::OnNormalCacheEntryAvailable;

    rv = mCacheQuery->Dispatch();
    if (NS_SUCCEEDED(rv))
        return NS_OK;

    mCacheQuery = nullptr;
    mOnCacheEntryAvailableCallback = nullptr;

    return rv;
}

nsresult
nsHttpChannel::OnNormalCacheEntryAvailable(nsICacheEntryDescriptor *aEntry,
                                           nsCacheAccessMode aAccess,
                                           nsresult aEntryStatus)
{
    if (NS_SUCCEEDED(aEntryStatus)) {
        mCacheEntry = aEntry;
        mCacheAccess = aAccess;
    }

    if (aEntryStatus == NS_ERROR_CACHE_WAIT_FOR_VALIDATION) {
        LOG(("bypassing local cache since it is busy\n"));
    }

    if (mCanceled && NS_FAILED(mStatus)) {
        LOG(("channel was canceled [this=%p status=%x]\n", this, mStatus));
        return mStatus;
    }

    if ((mLoadFlags & LOAD_ONLY_FROM_CACHE) && NS_FAILED(aEntryStatus))
        
        
        return NS_ERROR_DOCUMENT_NOT_CACHED;

    
    return NS_OK;
}


nsresult
nsHttpChannel::OpenOfflineCacheEntryForWriting()
{
    nsresult rv;

    LOG(("nsHttpChannel::OpenOfflineCacheEntryForWriting [this=%p]", this));

    
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

    
    
    if (IsSubRangeRequest(mRequestHead))
        return NS_OK;

    nsAutoCString cacheKey;
    GenerateCacheKey(mPostID, cacheKey);

    NS_ENSURE_TRUE(mApplicationCacheForWrite,
                   NS_ERROR_NOT_AVAILABLE);

    nsAutoCString offlineCacheClientID;
    rv = mApplicationCacheForWrite->GetClientID(offlineCacheClientID);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ENSURE_TRUE(!offlineCacheClientID.IsEmpty(),
                   NS_ERROR_NOT_AVAILABLE);

    nsCOMPtr<nsICacheSession> session;
    nsCOMPtr<nsICacheService> serv =
        do_GetService(NS_CACHESERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = serv->CreateSession(offlineCacheClientID.get(),
                             nsICache::STORE_OFFLINE,
                             nsICache::STREAM_BASED,
                             getter_AddRefs(session));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIFile> profileDirectory;
    rv = mApplicationCacheForWrite->GetProfileDirectory(
            getter_AddRefs(profileDirectory));
    NS_ENSURE_SUCCESS(rv, rv);

    if (profileDirectory) {
        rv = session->SetProfileDirectory(profileDirectory);
        if (NS_FAILED(rv)) return rv;
    }

    mOnCacheEntryAvailableCallback =
        &nsHttpChannel::OnOfflineCacheEntryForWritingAvailable;
    rv = session->AsyncOpenCacheEntry(cacheKey, nsICache::ACCESS_READ_WRITE,
                                      this, true);
    if (NS_SUCCEEDED(rv))
        return NS_OK;

    mOnCacheEntryAvailableCallback = nullptr;

    return rv;
}

nsresult
nsHttpChannel::OnOfflineCacheEntryForWritingAvailable(
    nsICacheEntryDescriptor *aEntry,
    nsCacheAccessMode aAccess,
    nsresult aEntryStatus)
{
    if (NS_SUCCEEDED(aEntryStatus)) {
        mOfflineCacheEntry = aEntry;
        mOfflineCacheAccess = aAccess;
        if (NS_FAILED(aEntry->GetLastModified(&mOfflineCacheLastModifiedTime))) {
            mOfflineCacheLastModifiedTime = 0;
        }
    }

    if (aEntryStatus == NS_ERROR_CACHE_WAIT_FOR_VALIDATION) {
        
        
        
        aEntryStatus = NS_OK;
    }

    if (mCanceled && NS_FAILED(mStatus)) {
        LOG(("channel was canceled [this=%p status=%x]\n", this, mStatus));
        return mStatus;
    }

    
    return aEntryStatus;
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

NS_IMETHODIMP
HttpCacheQuery::OnCacheEntryDoomed(nsresult)
{
    return NS_ERROR_UNEXPECTED;
}

nsresult
HttpCacheQuery::Dispatch()
{
    MOZ_ASSERT(NS_IsMainThread());

    nsresult rv;

    
    
    nsCOMPtr<nsICacheService> service = 
        do_GetService(NS_CACHESERVICE_CONTRACTID, &rv);

    
    if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsIStreamTransportService> sts =
            do_GetService(kStreamTransportServiceCID, &rv);
    }

    if (NS_SUCCEEDED(rv)) {
        rv = service->GetCacheIOTarget(getter_AddRefs(mCacheThread));
    }

    if (NS_SUCCEEDED(rv)) {
        rv = mCacheThread->Dispatch(this, NS_DISPATCH_NORMAL);
    }

    return rv;
}

NS_IMETHODIMP
HttpCacheQuery::Run()
{
    nsresult rv;
    if (!NS_IsMainThread()) {
        AssertOnCacheThread();

        nsCOMPtr<nsICacheService> serv =
            do_GetService(NS_CACHESERVICE_CONTRACTID, &rv);
        nsCOMPtr<nsICacheSession> session;
        if (NS_SUCCEEDED(rv)) {
            rv = serv->CreateSession(mClientID.get(), mStoragePolicy,
                                     nsICache::STREAM_BASED,
                                     getter_AddRefs(session));
        }
        if (NS_SUCCEEDED(rv)) {
            rv = session->SetIsPrivate(mUsingPrivateBrowsing);
        }
        if (NS_SUCCEEDED(rv)) {
            rv = session->SetDoomEntriesIfExpired(false);
        }
        if (NS_SUCCEEDED(rv)) {
            
            
            rv = session->AsyncOpenCacheEntry(mCacheKey, mAccessToRequest, this,
                                              mNoWait);
        }
        if (NS_FAILED(rv)) {
            LOG(("Failed to open cache entry -- calling OnCacheEntryAvailable "
                 "directly."));
            rv = OnCacheEntryAvailable(nullptr, 0, rv);
        }
    } else {
        
        nsCOMPtr<nsICacheListener> channel = mChannel.forget();
        mCacheThread = nullptr;
        nsCOMPtr<nsICacheEntryDescriptor> entry = mCacheEntry.forget();

        rv = channel->OnCacheEntryAvailable(entry, mCacheAccess, mStatus);
    }
    
    return rv;
}

NS_IMETHODIMP
HttpCacheQuery::OnCacheEntryAvailable(nsICacheEntryDescriptor *entry,
                                      nsCacheAccessMode access,
                                      nsresult status)

{
    LOG(("HttpCacheQuery::OnCacheEntryAvailable [channel=%p entry=%p "
         "access=%x status=%x, mRunConut=%d]\n", mChannel.get(), entry, access,
         status, int(mRunCount)));

    
    
    
    
    NS_ENSURE_TRUE(mRunCount == 0, NS_ERROR_UNEXPECTED);
    ++mRunCount;

    AssertOnCacheThread();

    mCacheEntry = entry;
    mCacheAccess = access;
    mStatus = status;

    if (mCacheEntry) {
        char* cacheDeviceID = nullptr;
        mCacheEntry->GetDeviceID(&cacheDeviceID);
        if (cacheDeviceID) {
            if (!strcmp(cacheDeviceID, kDiskDeviceID)) {
                mCacheEntryDeviceTelemetryID
                    = Telemetry::HTTP_DISK_CACHE_DISPOSITION_2;
            } else if (!strcmp(cacheDeviceID, kMemoryDeviceID)) {
                mCacheEntryDeviceTelemetryID
                    = Telemetry::HTTP_MEMORY_CACHE_DISPOSITION_2;
            } else if (!strcmp(cacheDeviceID, kOfflineDeviceID)) {
                mCacheEntryDeviceTelemetryID
                    = Telemetry::HTTP_OFFLINE_CACHE_DISPOSITION_2;
            } else {
                MOZ_NOT_REACHED("unknown cache device ID");
            }

            delete cacheDeviceID;
        } else {
            
            
            
        }
    }

    nsresult rv = CheckCache();
    if (NS_FAILED(rv))
        NS_WARNING("cache check failed");

    rv = NS_DispatchToMainThread(this);
    return rv;
}

nsresult
HttpCacheQuery::CheckCache()
{
    AssertOnCacheThread();

    nsresult rv = NS_OK;

    LOG(("HttpCacheQuery::CheckCache enter [channel=%p entry=%p access=%d]",
        mChannel.get(), mCacheEntry.get(), mCacheAccess));

    
    
    mCustomConditionalRequest =
        mRequestHead.PeekHeader(nsHttp::If_Modified_Since) ||
        mRequestHead.PeekHeader(nsHttp::If_None_Match) ||
        mRequestHead.PeekHeader(nsHttp::If_Unmodified_Since) ||
        mRequestHead.PeekHeader(nsHttp::If_Match) ||
        mRequestHead.PeekHeader(nsHttp::If_Range);

    
    mCachedContentIsValid = false;

    
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

    
    uint32_t lastModifiedTime;
    rv = mCacheEntry->GetLastModified(&lastModifiedTime);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    bool fromPreviousSession =
            (gHttpHandler->SessionStartTime() > lastModifiedTime);

    
    rv = mCacheEntry->GetMetaDataElement("response-head", getter_Copies(buf));
    NS_ENSURE_SUCCESS(rv, rv);

    
    mCachedResponseHead = new nsHttpResponseHead();
    rv = mCachedResponseHead->Parse((char *) buf.get());
    NS_ENSURE_SUCCESS(rv, rv);
    buf.Adopt(0);

    bool isCachedRedirect = WillRedirect(mCachedResponseHead);

    
    
    NS_ENSURE_TRUE((mCachedResponseHead->Status() / 100 != 3) ||
                   isCachedRedirect, NS_ERROR_ABORT);

    
    
    
    
    if (!mCacheForOfflineUse &&
        (mLoadedFromApplicationCache ||
         (mCacheAccess == nsICache::ACCESS_READ &&
          !(mLoadFlags & nsIRequest::INHIBIT_CACHING)) ||
         mFallbackChannel)) {
        rv = OpenCacheInputStream(true);
        if (NS_SUCCEEDED(rv)) {
            mCachedContentIsValid = true;
            
            MaybeMarkCacheEntryValid(this, mCacheEntry, mCacheAccess);
        }
        return rv;
    }

    if (method != nsHttp::Head && !isCachedRedirect) {
        
        
        
        
        
        
        
        
        int64_t contentLength = mCachedResponseHead->ContentLength();
        if (contentLength != int64_t(-1)) {
            uint32_t size;
            rv = mCacheEntry->GetDataSize(&size);
            NS_ENSURE_SUCCESS(rv, rv);

            if (int64_t(size) != contentLength) {
                LOG(("Cached data size does not match the Content-Length header "
                     "[content-length=%lld size=%u]\n", int64_t(contentLength), size));

                bool hasContentEncoding =
                    mCachedResponseHead->PeekHeader(nsHttp::Content_Encoding)
                    != nullptr;
                if ((int64_t(size) < contentLength) &&
                     size > 0 &&
                     !hasContentEncoding &&
                     mCachedResponseHead->IsResumable() &&
                     !mCustomConditionalRequest &&
                     !mCachedResponseHead->NoStore()) {
                    
                    
                    rv = SetupByteRangeRequest(size);
                    mCachedContentIsPartial = NS_SUCCEEDED(rv);
                    if (mCachedContentIsPartial) {
                        rv = OpenCacheInputStream(false);
                    } else {
                        
                        mRequestHead.ClearHeader(nsHttp::Range);
                        mRequestHead.ClearHeader(nsHttp::If_Range);
                    }
                }
                return rv;
            }
        }
    }

    bool doValidation = false;
    bool canAddImsHeader = true;

    
    if (ResponseWouldVary()) {
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
           (mCachedResponseHead->NoCache() && mUsingSSL)) {
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

        rv = mCacheEntry->GetExpirationTime(&time);
        NS_ENSURE_SUCCESS(rv, rv);

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
        
        
        
        
        
        
        
        
        
        
        
        mCacheEntry->GetMetaDataElement("auth", getter_Copies(buf));
        doValidation =
            (fromPreviousSession && !buf.IsEmpty()) ||
            (buf.IsEmpty() && mRequestHead.PeekHeader(nsHttp::Authorization));
    }

    
    
    
    
    
    
    if (!doValidation && isCachedRedirect) {
        if (!mRedirectedCachekeys)
            mRedirectedCachekeys = new nsTArray<nsCString>();
        else if (mRedirectedCachekeys->Contains(mCacheKey))
            doValidation = true;

        LOG(("Redirection-chain %s key %s\n",
             doValidation ? "contains" : "does not contain", mCacheKey.get()));

        
        if (!doValidation)
            mRedirectedCachekeys->AppendElement(mCacheKey);
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
        rv = OpenCacheInputStream(mCachedContentIsValid);
        if (NS_FAILED(rv)) {
            
            
            if (mDidReval) {
                
                mRequestHead.ClearHeader(nsHttp::If_Modified_Since);
                mRequestHead.ClearHeader(nsHttp::ETag);
                mDidReval = false;
            }
            mCachedContentIsValid = false;
        }
    }

    if (mCachedContentIsValid) {
        
        MaybeMarkCacheEntryValid(this, mCacheEntry, mCacheAccess);
    }

    LOG(("nsHTTPChannel::CheckCache exit [this=%p doValidation=%d]\n",
         this, doValidation));
    return rv;
}

 inline bool
HttpCacheQuery::HasQueryString(nsHttpAtom method, nsIURI * uri)
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
HttpCacheQuery::MustValidateBasedOnQueryUrl() const
{
    AssertOnCacheThread();

    
    
    
    
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

    
    if (mCacheEntry && (mCacheAccess & nsICache::ACCESS_WRITE)) {
        return true;
    }

    
    if (mOfflineCacheEntry && (mOfflineCacheAccess == nsICache::ACCESS_WRITE)) {
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
HttpCacheQuery::OpenCacheInputStream(bool startBuffering)
{
    AssertOnCacheThread();

    if (mUsingSSL) {
        nsresult rv = mCacheEntry->GetSecurityInfo(
                                      getter_AddRefs(mCachedSecurityInfo));
        if (NS_FAILED(rv)) {
            LOG(("failed to parse security-info [channel=%p, entry=%p]",
                 this, mCacheEntry.get()));
            NS_WARNING("failed to parse security-info");
            return rv;
        }

        
        
        MOZ_ASSERT(mCachedSecurityInfo || mLoadedFromApplicationCache);
        if (!mCachedSecurityInfo && !mLoadedFromApplicationCache) {
            LOG(("mCacheEntry->GetSecurityInfo returned success but did not "
                 "return the security info [channel=%p, entry=%p]",
                 this, mCacheEntry.get()));
            return NS_ERROR_UNEXPECTED; 
        }
    }

    nsresult rv = NS_OK;

    

    if (WillRedirect(mCachedResponseHead)) {
        
        
        LOG(("Will skip read of cached redirect entity\n"));
        return NS_OK;
    }

    if ((mLoadFlags & nsICachingChannel::LOAD_ONLY_IF_MODIFIED) &&
        !mCachedContentIsPartial) {
        
        
        if (!mCacheForOfflineUse) {
            LOG(("Will skip read from cache based on LOAD_ONLY_IF_MODIFIED "
                 "load flag\n"));
            return NS_OK;
        }

        
        
        
        
        
        
        LOG(("May skip read from cache based on LOAD_ONLY_IF_MODIFIED "
              "load flag\n"));
    }

    
    
    nsCOMPtr<nsIInputStream> stream;
    rv = mCacheEntry->OpenInputStream(0, getter_AddRefs(stream));

    if (NS_FAILED(rv)) {
        LOG(("Failed to open cache input stream [channel=%p, "
             "mCacheEntry=%p]", mChannel.get(), mCacheEntry.get()));
        return rv;
    }

    if (!startBuffering) {
        
        
        
        
        
        
        LOG(("Opened cache input stream without buffering [channel=%p, "
              "mCacheEntry=%p, stream=%p]", mChannel.get(),
              mCacheEntry.get(), stream.get()));
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
        
        
        
        
        
        
        MaybeMarkCacheEntryValid(this, mCacheEntry, mCacheAccess);
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
    mCacheQuery = nullptr;
    mCacheInputStream.CloseAndRelease();

    if (!mCacheEntry)
        return;

    LOG(("nsHttpChannel::CloseCacheEntry [this=%p] mStatus=%x mCacheAccess=%x",
         this, mStatus, mCacheAccess));

    
    
    
    

    bool doom = false;
    if (mInitedCacheEntry) {
        NS_ASSERTION(mResponseHead, "oops");
        if (NS_FAILED(mStatus) && doomOnFailure &&
            (mCacheAccess & nsICache::ACCESS_WRITE) &&
            !mResponseHead->IsResumable())
            doom = true;
    }
    else if (mCacheAccess == nsICache::ACCESS_WRITE)
        doom = true;

    if (doom) {
        LOG(("  dooming cache entry!!"));
        mCacheEntry->AsyncDoom(nullptr);
    }

    mCachedResponseHead = nullptr;

    mCachePump = 0;
    mCacheEntry = 0;
    mCacheAccess = 0;
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

    mOfflineCacheEntry = 0;
    mOfflineCacheAccess = 0;
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

    if (mLoadFlags & INHIBIT_PERSISTENT_CACHING) {
        rv = mCacheEntry->SetStoragePolicy(nsICache::STORE_IN_MEMORY);
        if (NS_FAILED(rv)) return rv;
    }

    
    rv = UpdateExpirationTime();
    if (NS_FAILED(rv)) return rv;

    rv = AddCacheEntryHeaders(mCacheEntry);
    if (NS_FAILED(rv)) return rv;

    mInitedCacheEntry = true;
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
        nsAutoCString buf, metaKey;
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
                    nsAutoCString hash;
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
nsHttpChannel::StoreAuthorizationMetaData(nsICacheEntryDescriptor *entry)
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
nsHttpChannel::InstallCacheListener(uint32_t offset)
{
    nsresult rv;

    LOG(("Preparing to write data into the cache [uri=%s]\n", mSpec.get()));

    NS_ASSERTION(mCacheEntry, "no cache entry");
    NS_ASSERTION(mListener, "no listener");

    
    
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
    if (mCacheEntry && (mCacheAccess & nsICache::ACCESS_WRITE) &&
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
    NS_INTERFACE_MAP_ENTRY(nsICacheListener)
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
    mCacheQuery = nullptr;
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
    mCacheEffectExperimentAsyncOpenTime = mAsyncOpenTime;

    
    
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

    
    CallOnModifyRequestObservers();

    
    
    if (mAPIRedirectToURI) {
        return AsyncCall(&nsHttpChannel::HandleAsyncAPIRedirect);
    }

    
    
    if (!mTimingEnabled)
        mAsyncOpenTime = TimeStamp();

    
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
            gHttpHandler->ConnMgr()->ClosePersistentConnections();
            gHttpHandler->ConnMgr()->ResetIPFamillyPreference(mConnectionInfo);
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

    LOG(("nsHttpChannel::SetupFallbackChannel [this=%x, key=%s]",
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

    
    NS_ASSERTION(request == mCachePump || request == mTransactionPump,
                 "Unexpected request");
    NS_ASSERTION(!(mTransactionPump && mCachePump) || mCachedContentIsPartial,
                 "If we have both pumps, the cache content must be partial");

    if (!mSecurityInfo && !mCachePump && mTransaction) {
        
        
        mSecurityInfo = mTransaction->SecurityInfo();
    }

    if (!mCachePump && NS_FAILED(mStatus) &&
        (mLoadFlags & LOAD_REPLACE) && mOriginalURI && mAllowSpdy) {
        
        

        nsAutoCString hostPort;
        if (NS_SUCCEEDED(mOriginalURI->GetHostPort(hostPort)))
            gHttpHandler->ConnMgr()->RemoveSpdyAlternateProtocol(hostPort);
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
            
            NS_ASSERTION(request != mTransactionPump,
                "byte-range transaction finished prematurely");

            if (request == mCachePump) {
                bool streamDone;
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
        mTransactionPump = 0;

        
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
            NS_ASSERTION(NS_FAILED(status), "should have a failure code here");
            
            
            mListener->OnStartRequest(this, mListenerContext);
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
    mStatus = status;

    
    if (mCacheEntry && (mCacheAccess & nsICache::ACCESS_WRITE) &&
        mRequestTimeInitialized){
        FinalizeCacheEntry();
    }
    
    if (mListener) {
        LOG(("  calling OnStopRequest\n"));
        mListener->OnStopRequest(this, mListenerContext, status);
    }

    CloseCacheEntry(!contentComplete);

    if (mOfflineCacheEntry)
        CloseOfflineCacheEntry();

    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nullptr, status);

    
    CleanRedirectCacheChainIfNecessary();

    ReleaseListeners();

    
    if (NS_SUCCEEDED(status) && contentComplete && !mCanceled) {
        Telemetry::ID telemID = gHttpHandler->mCacheEffectExperimentTelemetryID;
        if (telemID != nsHttpHandler::kNullTelemetryID) {
            Telemetry::AccumulateTimeDelta(telemID,
                                           mCacheEffectExperimentAsyncOpenTime);
        }
    }
    return NS_OK;
}





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

    NS_ASSERTION(mResponseHead, "No response head in ODA!!");

    NS_ASSERTION(!(mCachedContentIsPartial && (request == mTransactionPump)),
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
        NS_ASSERTION(progress <= progressMax, "unexpected progress values");

        OnTransportStatus(nullptr, transportStatus, progress, progressMax);

        
        
        
        
        
        

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
                                 uint64_t progress, uint64_t progressMax)
{
    
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
            NS_ASSERTION(progress <= progressMax, "unexpected progress values");
            mProgressSink->OnProgress(this, nullptr, progress, progressMax);
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





NS_IMETHODIMP
nsHttpChannel::OnCacheEntryAvailable(nsICacheEntryDescriptor *entry,
                                     nsCacheAccessMode access,
                                     nsresult status)
{
    MOZ_ASSERT(NS_IsMainThread());

    nsresult rv;

    LOG(("nsHttpChannel::OnCacheEntryAvailable [this=%p entry=%p "
         "access=%x status=%x]\n", this, entry, access, status));

    if (mCacheQuery) {
        mRequestHead = mCacheQuery->mRequestHead;
        mRedirectedCachekeys = mCacheQuery->mRedirectedCachekeys.forget();
        mCacheInputStream.takeOver(mCacheQuery->mCacheInputStream);
        mCachedResponseHead = mCacheQuery->mCachedResponseHead.forget();
        mCachedSecurityInfo = mCacheQuery->mCachedSecurityInfo.forget();
        mCachedContentIsValid = mCacheQuery->mCachedContentIsValid;
        mCachedContentIsPartial = mCacheQuery->mCachedContentIsPartial;
        mCustomConditionalRequest = mCacheQuery->mCustomConditionalRequest;
        mDidReval = mCacheQuery->mDidReval;
        mCacheEntryDeviceTelemetryID = mCacheQuery->mCacheEntryDeviceTelemetryID;
        mCacheQuery = nullptr;
    }

    
    
    if (!mIsPending) {
        mCacheInputStream.CloseAndRelease();
        return NS_OK;
    }

    rv = OnCacheEntryAvailableInternal(entry, access, status);

    if (NS_FAILED(rv)) {
        CloseCacheEntry(true);
        AsyncAbort(rv);
    }

    return NS_OK;
}

nsresult
nsHttpChannel::OnCacheEntryAvailableInternal(nsICacheEntryDescriptor *entry,
                                             nsCacheAccessMode access,
                                             nsresult status)
{
    nsresult rv;

    nsOnCacheEntryAvailableCallback callback = mOnCacheEntryAvailableCallback;
    mOnCacheEntryAvailableCallback = nullptr;

    NS_ASSERTION(callback,
        "nsHttpChannel::OnCacheEntryAvailable called without callback");
    rv = ((*this).*callback)(entry, access, status);

    if (mOnCacheEntryAvailableCallback) {
        
        NS_ASSERTION(NS_SUCCEEDED(rv), "Unexpected state");
        return NS_OK;
    }

    if (callback != &nsHttpChannel::OnOfflineCacheEntryForWritingAvailable) {
        if (NS_FAILED(rv)) {
            LOG(("AsyncOpenCacheEntry failed [rv=%x]\n", rv));
            if (mLoadFlags & LOAD_ONLY_FROM_CACHE) {
                
                
                if (!mFallbackChannel && !mFallbackKey.IsEmpty()) {
                    return AsyncCall(&nsHttpChannel::HandleAsyncFallback);
                }
                return NS_ERROR_DOCUMENT_NOT_CACHED;
            }
            if (mCanceled)
                
                
                return rv;

            
        }

        
        
        if (mApplicationCacheForWrite) {
            rv = OpenOfflineCacheEntryForWriting();
            if (mOnCacheEntryAvailableCallback) {
                NS_ASSERTION(NS_SUCCEEDED(rv), "Unexpected state");
                return NS_OK;
            }

            if (NS_FAILED(rv))
                return rv;
        }
    } else {
        
        if (NS_FAILED(rv))
            return rv;
    }

    return ContinueConnect();
}

NS_IMETHODIMP
nsHttpChannel::OnCacheEntryDoomed(nsresult status)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsHttpChannel::DoAuthRetry(nsAHttpConnection *conn)
{
    LOG(("nsHttpChannel::DoAuthRetry [this=%p]\n", this));

    NS_ASSERTION(!mTransaction, "should not have a transaction");
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

    nsresult rv;

    nsAutoCString cacheKey;
    rv = GenerateCacheKey(mPostID, cacheKey);
    NS_ENSURE_SUCCESS(rv, nullptr);

    return new OfflineCacheEntryAsForeignMarker(mApplicationCache, cacheKey);
}

nsresult
nsHttpChannel::OfflineCacheEntryAsForeignMarker::MarkAsForeign()
{
    return mApplicationCache->MarkEntry(mCacheKey,
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
            NS_ASSERTION(NS_SUCCEEDED(resume),
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
    NS_ASSERTION(mWaitingForRedirectCallback,
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






void
nsHttpChannel::MaybeInvalidateCacheEntryForSubsequentGet()
{
    
    
    
    
    
    if (mRequestHead.Method() == nsHttp::Options ||
       mRequestHead.Method() == nsHttp::Get ||
       mRequestHead.Method() == nsHttp::Head ||
       mRequestHead.Method() == nsHttp::Trace ||
       mRequestHead.Method() == nsHttp::Connect)
        return;


    
    
    nsAutoCString tmpCacheKey;
    GenerateCacheKey(0, tmpCacheKey);
    LOG(("MaybeInvalidateCacheEntryForSubsequentGet [this=%p uri=%s]\n", 
        this, tmpCacheKey.get()));
    DoInvalidateCacheEntry(tmpCacheKey);

    
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
        if (NS_SUCCEEDED(resultingURI->GetAsciiSpec(tmpSpec))) {
            location = tmpSpec.get();  

            
            AssembleCacheKey(location, 0, tmpCacheKey);
            DoInvalidateCacheEntry(tmpCacheKey);
        } else
            NS_WARNING(("  failed getting ascii-spec\n"));
    } else {
        LOG(("  hosts not matching\n"));
    }
}

void
nsHttpChannel::DoInvalidateCacheEntry(const nsCString &key)
{
    
    
    
    
    

    uint32_t appId = NECKO_NO_APP_ID;
    bool isInBrowser = false;
    NS_GetAppInfo(this, &appId, &isInBrowser);

    
    nsCacheStoragePolicy storagePolicy = DetermineStoragePolicy();
    nsAutoCString clientID;
    nsHttpHandler::GetCacheSessionNameForStoragePolicy(storagePolicy, mPrivateBrowsing,
                                                       appId, isInBrowser, clientID);

    LOG(("DoInvalidateCacheEntry [channel=%p session=%s policy=%d key=%s]",
         this, clientID.get(), int(storagePolicy), key.get()));

    nsresult rv;
    nsCOMPtr<nsICacheService> serv =
        do_GetService(NS_CACHESERVICE_CONTRACTID, &rv);
    nsCOMPtr<nsICacheSession> session;
    if (NS_SUCCEEDED(rv)) {
        rv = serv->CreateSession(clientID.get(), storagePolicy,
                                 nsICache::STREAM_BASED,
                                 getter_AddRefs(session));
    }
    if (NS_SUCCEEDED(rv)) {
        rv = session->SetIsPrivate(mPrivateBrowsing);
    }
    if (NS_SUCCEEDED(rv)) {
        rv = session->DoomEntry(key, nullptr);
    }

    LOG(("DoInvalidateCacheEntry [channel=%p session=%s policy=%d key=%s rv=%d]",
         this, clientID.get(), int(storagePolicy), key.get(), int(rv)));
}

nsCacheStoragePolicy
nsHttpChannel::DetermineStoragePolicy()
{
    nsCacheStoragePolicy policy = nsICache::STORE_ANYWHERE;
    if (mPrivateBrowsing)
        policy = nsICache::STORE_IN_MEMORY;
    else if (mLoadFlags & INHIBIT_PERSISTENT_CACHING)
        policy = nsICache::STORE_IN_MEMORY;

    return policy;
}

nsresult
nsHttpChannel::DetermineCacheAccess(nsCacheAccessMode *_retval)
{
    bool offline = gIOService->IsOffline();

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
