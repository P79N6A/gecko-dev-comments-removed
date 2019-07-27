




#if defined(MOZ_LOGGING)
#define FORCE_PR_LOG
#endif

#include "nsOfflineCacheUpdate.h"

#include "nsCPrefetchService.h"
#include "nsCURILoader.h"
#include "nsIApplicationCacheContainer.h"
#include "nsIApplicationCacheChannel.h"
#include "nsIApplicationCacheService.h"
#include "nsICachingChannel.h"
#include "nsIContent.h"
#include "mozilla/dom/Element.h"
#include "nsIDocumentLoader.h"
#include "nsIDOMElement.h"
#include "nsIDOMWindow.h"
#include "nsIDOMOfflineResourceList.h"
#include "nsIDocument.h"
#include "nsIObserverService.h"
#include "nsIURL.h"
#include "nsIWebProgress.h"
#include "nsICryptoHash.h"
#include "nsICacheEntry.h"
#include "nsIPermissionManager.h"
#include "nsIPrincipal.h"
#include "nsNetCID.h"
#include "nsNetUtil.h"
#include "nsServiceManagerUtils.h"
#include "nsStreamUtils.h"
#include "nsThreadUtils.h"
#include "nsProxyRelease.h"
#include "nsIConsoleService.h"
#include "prlog.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "mozilla/Preferences.h"
#include "mozilla/Attributes.h"
#include "nsContentUtils.h"

#include "nsXULAppAPI.h"

using namespace mozilla;

static const uint32_t kRescheduleLimit = 3;

static const uint32_t kPinnedEntryRetriesLimit = 3;

static const uint32_t kParallelLoadLimit = 15;


static const int32_t  kCustomProfileQuota = 512000;

#if defined(PR_LOGGING)









extern PRLogModuleInfo *gOfflineCacheUpdateLog;
#endif

#undef LOG
#define LOG(args) PR_LOG(gOfflineCacheUpdateLog, 4, args)

#undef LOG_ENABLED
#define LOG_ENABLED() PR_LOG_TEST(gOfflineCacheUpdateLog, 4)

class AutoFreeArray {
public:
    AutoFreeArray(uint32_t count, char **values)
        : mCount(count), mValues(values) {};
    ~AutoFreeArray() { NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(mCount, mValues); }
private:
    uint32_t mCount;
    char **mValues;
};

namespace { 

nsresult
DropReferenceFromURL(nsIURI * aURI)
{
    
    
    return aURI->SetRef(EmptyCString());
}

void
LogToConsole(const char * message, nsOfflineCacheUpdateItem * item = nullptr)
{
    nsCOMPtr<nsIConsoleService> consoleService =
        do_GetService(NS_CONSOLESERVICE_CONTRACTID);
    if (consoleService)
    {
        nsAutoString messageUTF16 = NS_ConvertUTF8toUTF16(message);
        if (item && item->mURI) {
            nsAutoCString uriSpec;
            item->mURI->GetSpec(uriSpec);

            messageUTF16.AppendLiteral(", URL=");
            messageUTF16.Append(NS_ConvertUTF8toUTF16(uriSpec));
        }
        consoleService->LogStringMessage(messageUTF16.get());
    }
}

} 





class nsManifestCheck MOZ_FINAL : public nsIStreamListener
                                , public nsIChannelEventSink
                                , public nsIInterfaceRequestor
{
public:
    nsManifestCheck(nsOfflineCacheUpdate *aUpdate,
                    nsIURI *aURI,
                    nsIURI *aReferrerURI)
        : mUpdate(aUpdate)
        , mURI(aURI)
        , mReferrerURI(aReferrerURI)
        {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSICHANNELEVENTSINK
    NS_DECL_NSIINTERFACEREQUESTOR

    nsresult Begin();

private:

    ~nsManifestCheck() {}

    static NS_METHOD ReadManifest(nsIInputStream *aInputStream,
                                  void *aClosure,
                                  const char *aFromSegment,
                                  uint32_t aOffset,
                                  uint32_t aCount,
                                  uint32_t *aBytesConsumed);

    nsRefPtr<nsOfflineCacheUpdate> mUpdate;
    nsCOMPtr<nsIURI> mURI;
    nsCOMPtr<nsIURI> mReferrerURI;
    nsCOMPtr<nsICryptoHash> mManifestHash;
    nsCOMPtr<nsIChannel> mChannel;
};




NS_IMPL_ISUPPORTS(nsManifestCheck,
                  nsIRequestObserver,
                  nsIStreamListener,
                  nsIChannelEventSink,
                  nsIInterfaceRequestor)





nsresult
nsManifestCheck::Begin()
{
    nsresult rv;
    mManifestHash = do_CreateInstance("@mozilla.org/security/hash;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mManifestHash->Init(nsICryptoHash::MD5);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = NS_NewChannel(getter_AddRefs(mChannel),
                       mURI,
                       nsContentUtils::GetSystemPrincipal(),
                       nsILoadInfo::SEC_NORMAL,
                       nsIContentPolicy::TYPE_OTHER,
                       nullptr,   
                       nullptr,   
                       nullptr,   
                       nsIRequest::LOAD_BYPASS_CACHE);

    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIHttpChannel> httpChannel =
        do_QueryInterface(mChannel);
    if (httpChannel) {
        httpChannel->SetReferrer(mReferrerURI);
        httpChannel->SetRequestHeader(NS_LITERAL_CSTRING("X-Moz"),
                                      NS_LITERAL_CSTRING("offline-resource"),
                                      false);
    }

    rv = mChannel->AsyncOpen(this, nullptr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}






NS_METHOD
nsManifestCheck::ReadManifest(nsIInputStream *aInputStream,
                              void *aClosure,
                              const char *aFromSegment,
                              uint32_t aOffset,
                              uint32_t aCount,
                              uint32_t *aBytesConsumed)
{
    nsManifestCheck *manifestCheck =
        static_cast<nsManifestCheck*>(aClosure);

    nsresult rv;
    *aBytesConsumed = aCount;

    rv = manifestCheck->mManifestHash->Update(
        reinterpret_cast<const uint8_t *>(aFromSegment), aCount);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}





NS_IMETHODIMP
nsManifestCheck::OnStartRequest(nsIRequest *aRequest,
                                nsISupports *aContext)
{
    return NS_OK;
}

NS_IMETHODIMP
nsManifestCheck::OnDataAvailable(nsIRequest *aRequest,
                                 nsISupports *aContext,
                                 nsIInputStream *aStream,
                                 uint64_t aOffset,
                                 uint32_t aCount)
{
    uint32_t bytesRead;
    aStream->ReadSegments(ReadManifest, this, aCount, &bytesRead);
    return NS_OK;
}

NS_IMETHODIMP
nsManifestCheck::OnStopRequest(nsIRequest *aRequest,
                               nsISupports *aContext,
                               nsresult aStatus)
{
    nsAutoCString manifestHash;
    if (NS_SUCCEEDED(aStatus)) {
        mManifestHash->Finish(true, manifestHash);
    }

    mUpdate->ManifestCheckCompleted(aStatus, manifestHash);

    return NS_OK;
}





NS_IMETHODIMP
nsManifestCheck::GetInterface(const nsIID &aIID, void **aResult)
{
    if (aIID.Equals(NS_GET_IID(nsIChannelEventSink))) {
        NS_ADDREF_THIS();
        *aResult = static_cast<nsIChannelEventSink *>(this);
        return NS_OK;
    }

    return NS_ERROR_NO_INTERFACE;
}





NS_IMETHODIMP
nsManifestCheck::AsyncOnChannelRedirect(nsIChannel *aOldChannel,
                                        nsIChannel *aNewChannel,
                                        uint32_t aFlags,
                                        nsIAsyncVerifyRedirectCallback *callback)
{
    
    if (aFlags & nsIChannelEventSink::REDIRECT_INTERNAL) {
        callback->OnRedirectVerifyCallback(NS_OK);
        return NS_OK;
    }

    LogToConsole("Manifest check failed because its response is a redirect");

    aOldChannel->Cancel(NS_ERROR_ABORT);
    return NS_ERROR_ABORT;
}





NS_IMPL_ISUPPORTS(nsOfflineCacheUpdateItem,
                  nsIRequestObserver,
                  nsIStreamListener,
                  nsIRunnable,
                  nsIInterfaceRequestor,
                  nsIChannelEventSink)





nsOfflineCacheUpdateItem::nsOfflineCacheUpdateItem(nsIURI *aURI,
                                                   nsIURI *aReferrerURI,
                                                   nsIApplicationCache *aApplicationCache,
                                                   nsIApplicationCache *aPreviousApplicationCache,
                                                   uint32_t type)
    : mURI(aURI)
    , mReferrerURI(aReferrerURI)
    , mApplicationCache(aApplicationCache)
    , mPreviousApplicationCache(aPreviousApplicationCache)
    , mItemType(type)
    , mChannel(nullptr)
    , mState(LoadStatus::UNINITIALIZED)
    , mBytesRead(0)
{
}

nsOfflineCacheUpdateItem::~nsOfflineCacheUpdateItem()
{
}

nsresult
nsOfflineCacheUpdateItem::OpenChannel(nsOfflineCacheUpdate *aUpdate)
{
#if defined(PR_LOGGING)
    if (LOG_ENABLED()) {
        nsAutoCString spec;
        mURI->GetSpec(spec);
        LOG(("%p: Opening channel for %s", this, spec.get()));
    }
#endif

    if (mUpdate) {
        
        
        
        LOG(("  %p is already running! ignoring", this));
        return NS_ERROR_ALREADY_OPENED;
    }

    nsresult rv = nsOfflineCacheUpdate::GetCacheKey(mURI, mCacheKey);
    NS_ENSURE_SUCCESS(rv, rv);

    uint32_t flags = nsIRequest::LOAD_BACKGROUND |
                     nsICachingChannel::LOAD_ONLY_IF_MODIFIED |
                     nsICachingChannel::LOAD_CHECK_OFFLINE_CACHE;

    if (mApplicationCache == mPreviousApplicationCache) {
        
        
        
        flags |= nsIRequest::INHIBIT_CACHING;
    }

    rv = NS_NewChannel(getter_AddRefs(mChannel),
                       mURI,
                       nsContentUtils::GetSystemPrincipal(),
                       nsILoadInfo::SEC_NORMAL,
                       nsIContentPolicy::TYPE_OTHER,
                       nullptr,  
                       nullptr,  
                       this,     
                       flags);

    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIApplicationCacheChannel> appCacheChannel =
        do_QueryInterface(mChannel, &rv);

    
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = appCacheChannel->SetApplicationCache(mPreviousApplicationCache);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = appCacheChannel->SetApplicationCacheForWrite(mApplicationCache);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIHttpChannel> httpChannel =
        do_QueryInterface(mChannel);
    if (httpChannel) {
        httpChannel->SetReferrer(mReferrerURI);
        httpChannel->SetRequestHeader(NS_LITERAL_CSTRING("X-Moz"),
                                      NS_LITERAL_CSTRING("offline-resource"),
                                      false);
    }

    rv = mChannel->AsyncOpen(this, nullptr);
    NS_ENSURE_SUCCESS(rv, rv);

    mUpdate = aUpdate;

    mState = LoadStatus::REQUESTED;

    return NS_OK;
}

nsresult
nsOfflineCacheUpdateItem::Cancel()
{
    if (mChannel) {
        mChannel->Cancel(NS_ERROR_ABORT);
        mChannel = nullptr;
    }

    mState = LoadStatus::UNINITIALIZED;

    return NS_OK;
}





NS_IMETHODIMP
nsOfflineCacheUpdateItem::OnStartRequest(nsIRequest *aRequest,
                                         nsISupports *aContext)
{
    mState = LoadStatus::RECEIVING;

    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdateItem::OnDataAvailable(nsIRequest *aRequest,
                                          nsISupports *aContext,
                                          nsIInputStream *aStream,
                                          uint64_t aOffset,
                                          uint32_t aCount)
{
    uint32_t bytesRead = 0;
    aStream->ReadSegments(NS_DiscardSegment, nullptr, aCount, &bytesRead);
    mBytesRead += bytesRead;
    LOG(("loaded %u bytes into offline cache [offset=%llu]\n",
         bytesRead, aOffset));

    mUpdate->OnByteProgress(bytesRead);

    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdateItem::OnStopRequest(nsIRequest *aRequest,
                                        nsISupports *aContext,
                                        nsresult aStatus)
{
#if defined(PR_LOGGING)
    if (LOG_ENABLED()) {
        nsAutoCString spec;
        mURI->GetSpec(spec);
        LOG(("%p: Done fetching offline item %s [status=%x]\n",
            this, spec.get(), aStatus));
    }
#endif

    if (mBytesRead == 0 && aStatus == NS_OK) {
        
        
        
        mChannel->GetContentLength(&mBytesRead);
        mUpdate->OnByteProgress(mBytesRead);
    }

    if (NS_FAILED(aStatus)) {
        nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(mChannel);
        if (httpChannel) {
            bool isNoStore;
            if (NS_SUCCEEDED(httpChannel->IsNoStoreResponse(&isNoStore))
                && isNoStore) {
                LogToConsole("Offline cache manifest item has Cache-control: no-store header",
                             this);
            }
        }
    }

    
    
    NS_DispatchToCurrentThread(this);

    return NS_OK;
}





NS_IMETHODIMP
nsOfflineCacheUpdateItem::Run()
{
    
    
    
    
    
    
    mState = LoadStatus::LOADED;

    nsRefPtr<nsOfflineCacheUpdate> update;
    update.swap(mUpdate);
    update->LoadCompleted(this);

    return NS_OK;
}





NS_IMETHODIMP
nsOfflineCacheUpdateItem::GetInterface(const nsIID &aIID, void **aResult)
{
    if (aIID.Equals(NS_GET_IID(nsIChannelEventSink))) {
        NS_ADDREF_THIS();
        *aResult = static_cast<nsIChannelEventSink *>(this);
        return NS_OK;
    }

    return NS_ERROR_NO_INTERFACE;
}





NS_IMETHODIMP
nsOfflineCacheUpdateItem::AsyncOnChannelRedirect(nsIChannel *aOldChannel,
                                                 nsIChannel *aNewChannel,
                                                 uint32_t aFlags,
                                                 nsIAsyncVerifyRedirectCallback *cb)
{
    if (!(aFlags & nsIChannelEventSink::REDIRECT_INTERNAL)) {
        
        
        LogToConsole("Offline cache manifest failed because an item redirects", this);

        aOldChannel->Cancel(NS_ERROR_ABORT);
        return NS_ERROR_ABORT;
    }

    nsCOMPtr<nsIURI> newURI;
    nsresult rv = aNewChannel->GetURI(getter_AddRefs(newURI));
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIApplicationCacheChannel> appCacheChannel =
        do_QueryInterface(aNewChannel);
    if (appCacheChannel) {
        rv = appCacheChannel->SetApplicationCacheForWrite(mApplicationCache);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    nsAutoCString oldScheme;
    mURI->GetScheme(oldScheme);

    bool match;
    if (NS_FAILED(newURI->SchemeIs(oldScheme.get(), &match)) || !match) {
        LOG(("rejected: redirected to a different scheme\n"));
        return NS_ERROR_ABORT;
    }

    
    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aNewChannel);
    NS_ENSURE_STATE(httpChannel);

    httpChannel->SetRequestHeader(NS_LITERAL_CSTRING("X-Moz"),
                                  NS_LITERAL_CSTRING("offline-resource"),
                                  false);

    mChannel = aNewChannel;

    cb->OnRedirectVerifyCallback(NS_OK);
    return NS_OK;
}

nsresult
nsOfflineCacheUpdateItem::GetRequestSucceeded(bool * succeeded)
{
    *succeeded = false;

    if (!mChannel)
        return NS_OK;

    nsresult rv;
    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(mChannel, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    bool reqSucceeded;
    rv = httpChannel->GetRequestSucceeded(&reqSucceeded);
    if (NS_ERROR_NOT_AVAILABLE == rv)
        return NS_OK;
    NS_ENSURE_SUCCESS(rv, rv);

    if (!reqSucceeded) {
        LOG(("Request failed"));
        return NS_OK;
    }

    nsresult channelStatus;
    rv = httpChannel->GetStatus(&channelStatus);
    NS_ENSURE_SUCCESS(rv, rv);

    if (NS_FAILED(channelStatus)) {
        LOG(("Channel status=0x%08x", channelStatus));
        return NS_OK;
    }

    *succeeded = true;
    return NS_OK;
}

bool
nsOfflineCacheUpdateItem::IsScheduled()
{
    return mState == LoadStatus::UNINITIALIZED;
}

bool
nsOfflineCacheUpdateItem::IsInProgress()
{
    return mState == LoadStatus::REQUESTED ||
           mState == LoadStatus::RECEIVING;
}

bool
nsOfflineCacheUpdateItem::IsCompleted()
{
    return mState == LoadStatus::LOADED;
}

nsresult
nsOfflineCacheUpdateItem::GetStatus(uint16_t *aStatus)
{
    if (!mChannel) {
        *aStatus = 0;
        return NS_OK;
    }

    nsresult rv;
    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(mChannel, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    uint32_t httpStatus;
    rv = httpChannel->GetResponseStatus(&httpStatus);
    if (rv == NS_ERROR_NOT_AVAILABLE) {
        *aStatus = 0;
        return NS_OK;
    }

    NS_ENSURE_SUCCESS(rv, rv);
    *aStatus = uint16_t(httpStatus);
    return NS_OK;
}









nsOfflineManifestItem::nsOfflineManifestItem(nsIURI *aURI,
                                             nsIURI *aReferrerURI,
                                             nsIApplicationCache *aApplicationCache,
                                             nsIApplicationCache *aPreviousApplicationCache)
    : nsOfflineCacheUpdateItem(aURI, aReferrerURI,
                               aApplicationCache, aPreviousApplicationCache,
                               nsIApplicationCache::ITEM_MANIFEST)
    , mParserState(PARSE_INIT)
    , mNeedsUpdate(true)
    , mManifestHashInitialized(false)
{
    ReadStrictFileOriginPolicyPref();
}

nsOfflineManifestItem::~nsOfflineManifestItem()
{
}






NS_METHOD
nsOfflineManifestItem::ReadManifest(nsIInputStream *aInputStream,
                                    void *aClosure,
                                    const char *aFromSegment,
                                    uint32_t aOffset,
                                    uint32_t aCount,
                                    uint32_t *aBytesConsumed)
{
    nsOfflineManifestItem *manifest =
        static_cast<nsOfflineManifestItem*>(aClosure);

    nsresult rv;

    *aBytesConsumed = aCount;

    if (manifest->mParserState == PARSE_ERROR) {
        
        return NS_OK;
    }

    if (!manifest->mManifestHashInitialized) {
        
        manifest->mManifestHashInitialized = true;

        manifest->mManifestHash = do_CreateInstance("@mozilla.org/security/hash;1", &rv);
        if (NS_SUCCEEDED(rv)) {
            rv = manifest->mManifestHash->Init(nsICryptoHash::MD5);
            if (NS_FAILED(rv)) {
                manifest->mManifestHash = nullptr;
                LOG(("Could not initialize manifest hash for byte-to-byte check, rv=%08x", rv));
            }
        }
    }

    if (manifest->mManifestHash) {
        rv = manifest->mManifestHash->Update(reinterpret_cast<const uint8_t *>(aFromSegment), aCount);
        if (NS_FAILED(rv)) {
            manifest->mManifestHash = nullptr;
            LOG(("Could not update manifest hash, rv=%08x", rv));
        }
    }

    manifest->mReadBuf.Append(aFromSegment, aCount);

    nsCString::const_iterator begin, iter, end;
    manifest->mReadBuf.BeginReading(begin);
    manifest->mReadBuf.EndReading(end);

    for (iter = begin; iter != end; iter++) {
        if (*iter == '\r' || *iter == '\n') {
            nsresult rv = manifest->HandleManifestLine(begin, iter);

            if (NS_FAILED(rv)) {
                LOG(("HandleManifestLine failed with 0x%08x", rv));
                *aBytesConsumed = 0; 
                return NS_ERROR_ABORT;
            }

            begin = iter;
            begin++;
        }
    }

    
    manifest->mReadBuf = Substring(begin, end);

    return NS_OK;
}

nsresult
nsOfflineManifestItem::AddNamespace(uint32_t namespaceType,
                                    const nsCString &namespaceSpec,
                                    const nsCString &data)

{
    nsresult rv;
    if (!mNamespaces) {
        mNamespaces = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    nsCOMPtr<nsIApplicationCacheNamespace> ns =
        do_CreateInstance(NS_APPLICATIONCACHENAMESPACE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = ns->Init(namespaceType, namespaceSpec, data);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mNamespaces->AppendElement(ns, false);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
nsOfflineManifestItem::HandleManifestLine(const nsCString::const_iterator &aBegin,
                                          const nsCString::const_iterator &aEnd)
{
    nsCString::const_iterator begin = aBegin;
    nsCString::const_iterator end = aEnd;

    
    nsCString::const_iterator last = end;
    --last;
    while (end != begin && (*last == ' ' || *last == '\t')) {
        --end;
        --last;
    }

    if (mParserState == PARSE_INIT) {
        
        if (begin != end && static_cast<unsigned char>(*begin) == 0xef) {
            if (++begin == end || static_cast<unsigned char>(*begin) != 0xbb ||
                ++begin == end || static_cast<unsigned char>(*begin) != 0xbf) {
                mParserState = PARSE_ERROR;
                LogToConsole("Offline cache manifest BOM error", this);
                return NS_OK;
            }
            ++begin;
        }

        const nsCSubstring &magic = Substring(begin, end);

        if (!magic.EqualsLiteral("CACHE MANIFEST")) {
            mParserState = PARSE_ERROR;
            LogToConsole("Offline cache manifest magic incorect", this);
            return NS_OK;
        }

        mParserState = PARSE_CACHE_ENTRIES;
        return NS_OK;
    }

    
    while (begin != end && (*begin == ' ' || *begin == '\t'))
        begin++;

    
    if (begin == end || *begin == '#')
        return NS_OK;

    const nsCSubstring &line = Substring(begin, end);

    if (line.EqualsLiteral("CACHE:")) {
        mParserState = PARSE_CACHE_ENTRIES;
        return NS_OK;
    }

    if (line.EqualsLiteral("FALLBACK:")) {
        mParserState = PARSE_FALLBACK_ENTRIES;
        return NS_OK;
    }

    if (line.EqualsLiteral("NETWORK:")) {
        mParserState = PARSE_BYPASS_ENTRIES;
        return NS_OK;
    }

    
    nsCString::const_iterator lastChar = end;
    if (*(--lastChar) == ':') {
        mParserState = PARSE_UNKNOWN_SECTION;
        return NS_OK;
    }

    nsresult rv;

    switch(mParserState) {
    case PARSE_INIT:
    case PARSE_ERROR: {
        
        return NS_ERROR_FAILURE;
    }

    case PARSE_UNKNOWN_SECTION: {
        
        return NS_OK;
    }

    case PARSE_CACHE_ENTRIES: {
        nsCOMPtr<nsIURI> uri;
        rv = NS_NewURI(getter_AddRefs(uri), line, nullptr, mURI);
        if (NS_FAILED(rv))
            break;
        if (NS_FAILED(DropReferenceFromURL(uri)))
            break;

        nsAutoCString scheme;
        uri->GetScheme(scheme);

        
        bool match;
        if (NS_FAILED(mURI->SchemeIs(scheme.get(), &match)) || !match)
            break;

        mExplicitURIs.AppendObject(uri);
        break;
    }

    case PARSE_FALLBACK_ENTRIES: {
        int32_t separator = line.FindChar(' ');
        if (separator == kNotFound) {
            separator = line.FindChar('\t');
            if (separator == kNotFound)
                break;
        }

        nsCString namespaceSpec(Substring(line, 0, separator));
        nsCString fallbackSpec(Substring(line, separator + 1));
        namespaceSpec.CompressWhitespace();
        fallbackSpec.CompressWhitespace();

        nsCOMPtr<nsIURI> namespaceURI;
        rv = NS_NewURI(getter_AddRefs(namespaceURI), namespaceSpec, nullptr, mURI);
        if (NS_FAILED(rv))
            break;
        if (NS_FAILED(DropReferenceFromURL(namespaceURI)))
            break;
        rv = namespaceURI->GetAsciiSpec(namespaceSpec);
        if (NS_FAILED(rv))
            break;


        nsCOMPtr<nsIURI> fallbackURI;
        rv = NS_NewURI(getter_AddRefs(fallbackURI), fallbackSpec, nullptr, mURI);
        if (NS_FAILED(rv))
            break;
        if (NS_FAILED(DropReferenceFromURL(fallbackURI)))
            break;
        rv = fallbackURI->GetAsciiSpec(fallbackSpec);
        if (NS_FAILED(rv))
            break;

        
        if (!NS_SecurityCompareURIs(mURI, namespaceURI,
                                    mStrictFileOriginPolicy))
            break;

        
        if (!NS_SecurityCompareURIs(namespaceURI, fallbackURI,
                                    mStrictFileOriginPolicy))
            break;

        mFallbackURIs.AppendObject(fallbackURI);

        AddNamespace(nsIApplicationCacheNamespace::NAMESPACE_FALLBACK,
                     namespaceSpec, fallbackSpec);
        break;
    }

    case PARSE_BYPASS_ENTRIES: {
        if (line[0] == '*' && (line.Length() == 1 || line[1] == ' ' || line[1] == '\t'))
        {
          
          
          
          
          
          AddNamespace(nsIApplicationCacheNamespace::NAMESPACE_BYPASS,
                       EmptyCString(), EmptyCString());
          break;
        }

        nsCOMPtr<nsIURI> bypassURI;
        rv = NS_NewURI(getter_AddRefs(bypassURI), line, nullptr, mURI);
        if (NS_FAILED(rv))
            break;

        nsAutoCString scheme;
        bypassURI->GetScheme(scheme);
        bool equals;
        if (NS_FAILED(mURI->SchemeIs(scheme.get(), &equals)) || !equals)
            break;
        if (NS_FAILED(DropReferenceFromURL(bypassURI)))
            break;
        nsCString spec;
        if (NS_FAILED(bypassURI->GetAsciiSpec(spec)))
            break;

        AddNamespace(nsIApplicationCacheNamespace::NAMESPACE_BYPASS,
                     spec, EmptyCString());
        break;
    }
    }

    return NS_OK;
}

nsresult
nsOfflineManifestItem::GetOldManifestContentHash(nsIRequest *aRequest)
{
    nsresult rv;

    nsCOMPtr<nsICachingChannel> cachingChannel = do_QueryInterface(aRequest, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    nsCOMPtr<nsISupports> cacheToken;
    cachingChannel->GetCacheToken(getter_AddRefs(cacheToken));
    if (cacheToken) {
        nsCOMPtr<nsICacheEntry> cacheDescriptor(do_QueryInterface(cacheToken, &rv));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = cacheDescriptor->GetMetaDataElement("offline-manifest-hash", getter_Copies(mOldManifestHashValue));
        if (NS_FAILED(rv))
            mOldManifestHashValue.Truncate();
    }

    return NS_OK;
}

nsresult
nsOfflineManifestItem::CheckNewManifestContentHash(nsIRequest *aRequest)
{
    nsresult rv;

    if (!mManifestHash) {
        
        return NS_OK;
    }

    nsCString newManifestHashValue;
    rv = mManifestHash->Finish(true, mManifestHashValue);
    mManifestHash = nullptr;

    if (NS_FAILED(rv)) {
        LOG(("Could not finish manifest hash, rv=%08x", rv));
        
        return NS_OK;
    }

    if (!ParseSucceeded()) {
        
        return NS_OK;
    }

    if (mOldManifestHashValue == mManifestHashValue) {
        LOG(("Update not needed, downloaded manifest content is byte-for-byte identical"));
        mNeedsUpdate = false;
    }

    
    
    nsCOMPtr<nsICachingChannel> cachingChannel = do_QueryInterface(aRequest, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupports> cacheToken;
    cachingChannel->GetOfflineCacheToken(getter_AddRefs(cacheToken));
    if (cacheToken) {
        nsCOMPtr<nsICacheEntry> cacheDescriptor(do_QueryInterface(cacheToken, &rv));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = cacheDescriptor->SetMetaDataElement("offline-manifest-hash", mManifestHashValue.get());
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}

void
nsOfflineManifestItem::ReadStrictFileOriginPolicyPref()
{
    mStrictFileOriginPolicy =
        Preferences::GetBool("security.fileuri.strict_origin_policy", true);
}

NS_IMETHODIMP
nsOfflineManifestItem::OnStartRequest(nsIRequest *aRequest,
                                      nsISupports *aContext)
{
    nsresult rv;

    nsCOMPtr<nsIHttpChannel> channel = do_QueryInterface(aRequest, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    bool succeeded;
    rv = channel->GetRequestSucceeded(&succeeded);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!succeeded) {
        LOG(("HTTP request failed"));
        LogToConsole("Offline cache manifest HTTP request failed", this);
        mParserState = PARSE_ERROR;
        return NS_ERROR_ABORT;
    }

    rv = GetOldManifestContentHash(aRequest);
    NS_ENSURE_SUCCESS(rv, rv);

    return nsOfflineCacheUpdateItem::OnStartRequest(aRequest, aContext);
}

NS_IMETHODIMP
nsOfflineManifestItem::OnDataAvailable(nsIRequest *aRequest,
                                       nsISupports *aContext,
                                       nsIInputStream *aStream,
                                       uint64_t aOffset,
                                       uint32_t aCount)
{
    uint32_t bytesRead = 0;
    aStream->ReadSegments(ReadManifest, this, aCount, &bytesRead);
    mBytesRead += bytesRead;

    if (mParserState == PARSE_ERROR) {
        LOG(("OnDataAvailable is canceling the request due a parse error\n"));
        return NS_ERROR_ABORT;
    }

    LOG(("loaded %u bytes into offline cache [offset=%u]\n",
         bytesRead, aOffset));

    
    

    return NS_OK;
}

NS_IMETHODIMP
nsOfflineManifestItem::OnStopRequest(nsIRequest *aRequest,
                                     nsISupports *aContext,
                                     nsresult aStatus)
{
    if (mBytesRead == 0) {
        
        
        mNeedsUpdate = false;
    } else {
        
        nsCString::const_iterator begin, end;
        mReadBuf.BeginReading(begin);
        mReadBuf.EndReading(end);
        nsresult rv = HandleManifestLine(begin, end);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = CheckNewManifestContentHash(aRequest);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return nsOfflineCacheUpdateItem::OnStopRequest(aRequest, aContext, aStatus);
}





NS_IMPL_ISUPPORTS(nsOfflineCacheUpdate,
                  nsIOfflineCacheUpdateObserver,
                  nsIOfflineCacheUpdate,
                  nsIRunnable)





nsOfflineCacheUpdate::nsOfflineCacheUpdate()
    : mState(STATE_UNINITIALIZED)
    , mAddedItems(false)
    , mPartialUpdate(false)
    , mOnlyCheckUpdate(false)
    , mSucceeded(true)
    , mObsolete(false)
    , mAppID(NECKO_NO_APP_ID)
    , mInBrowser(false)
    , mItemsInProgress(0)
    , mRescheduleCount(0)
    , mPinnedEntryRetriesCount(0)
    , mPinned(false)
{
}

nsOfflineCacheUpdate::~nsOfflineCacheUpdate()
{
    LOG(("nsOfflineCacheUpdate::~nsOfflineCacheUpdate [%p]", this));
}


nsresult
nsOfflineCacheUpdate::GetCacheKey(nsIURI *aURI, nsACString &aKey)
{
    aKey.Truncate();

    nsCOMPtr<nsIURI> newURI;
    nsresult rv = aURI->CloneIgnoringRef(getter_AddRefs(newURI));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = newURI->GetAsciiSpec(aKey);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::InitInternal(nsIURI *aManifestURI)
{
    nsresult rv;

    
    bool match;
    rv = aManifestURI->SchemeIs("http", &match);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!match) {
        rv = aManifestURI->SchemeIs("https", &match);
        NS_ENSURE_SUCCESS(rv, rv);
        if (!match)
            return NS_ERROR_ABORT;
    }

    mManifestURI = aManifestURI;

    rv = mManifestURI->GetAsciiHost(mUpdateDomain);
    NS_ENSURE_SUCCESS(rv, rv);

    mPartialUpdate = false;

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::Init(nsIURI *aManifestURI,
                           nsIURI *aDocumentURI,
                           nsIDOMDocument *aDocument,
                           nsIFile *aCustomProfileDir,
                           uint32_t aAppID,
                           bool aInBrowser)
{
    nsresult rv;

    
    nsOfflineCacheUpdateService* service =
        nsOfflineCacheUpdateService::EnsureService();
    if (!service)
        return NS_ERROR_FAILURE;

    LOG(("nsOfflineCacheUpdate::Init [%p]", this));

    rv = InitInternal(aManifestURI);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIApplicationCacheService> cacheService =
        do_GetService(NS_APPLICATIONCACHESERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    mDocumentURI = aDocumentURI;

    if (aCustomProfileDir) {
        rv = cacheService->BuildGroupIDForApp(aManifestURI,
                                              aAppID, aInBrowser,
                                              mGroupID);
        NS_ENSURE_SUCCESS(rv, rv);

        
        

        
        
        
        mPreviousApplicationCache = nullptr;

        rv = cacheService->CreateCustomApplicationCache(mGroupID,
                                                        aCustomProfileDir,
                                                        kCustomProfileQuota,
                                                        getter_AddRefs(mApplicationCache));
        NS_ENSURE_SUCCESS(rv, rv);

        mCustomProfileDir = aCustomProfileDir;
    }
    else {
        rv = cacheService->BuildGroupIDForApp(aManifestURI,
                                              aAppID, aInBrowser,
                                              mGroupID);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = cacheService->GetActiveCache(mGroupID,
                                          getter_AddRefs(mPreviousApplicationCache));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = cacheService->CreateApplicationCache(mGroupID,
                                                  getter_AddRefs(mApplicationCache));
        NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = nsOfflineCacheUpdateService::OfflineAppPinnedForURI(aDocumentURI,
                                                             nullptr,
                                                             &mPinned);
    NS_ENSURE_SUCCESS(rv, rv);

    mAppID = aAppID;
    mInBrowser = aInBrowser;

    mState = STATE_INITIALIZED;
    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::InitForUpdateCheck(nsIURI *aManifestURI,
                                         uint32_t aAppID,
                                         bool aInBrowser,
                                         nsIObserver *aObserver)
{
    nsresult rv;

    
    nsOfflineCacheUpdateService* service =
        nsOfflineCacheUpdateService::EnsureService();
    if (!service)
        return NS_ERROR_FAILURE;

    LOG(("nsOfflineCacheUpdate::InitForUpdateCheck [%p]", this));

    rv = InitInternal(aManifestURI);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIApplicationCacheService> cacheService =
        do_GetService(NS_APPLICATIONCACHESERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = cacheService->BuildGroupIDForApp(aManifestURI,
                                          aAppID, aInBrowser,
                                          mGroupID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = cacheService->GetActiveCache(mGroupID,
                                      getter_AddRefs(mPreviousApplicationCache));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    
    
    mApplicationCache = mPreviousApplicationCache;

    rv = nsOfflineCacheUpdateService::OfflineAppPinnedForURI(aManifestURI,
                                                             nullptr,
                                                             &mPinned);
    NS_ENSURE_SUCCESS(rv, rv);

    mUpdateAvailableObserver = aObserver;
    mOnlyCheckUpdate = true;

    mState = STATE_INITIALIZED;
    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::InitPartial(nsIURI *aManifestURI,
                                  const nsACString& clientID,
                                  nsIURI *aDocumentURI)
{
    nsresult rv;

    
    nsOfflineCacheUpdateService* service =
        nsOfflineCacheUpdateService::EnsureService();
    if (!service)
        return NS_ERROR_FAILURE;

    LOG(("nsOfflineCacheUpdate::InitPartial [%p]", this));

    mPartialUpdate = true;
    mDocumentURI = aDocumentURI;

    mManifestURI = aManifestURI;
    rv = mManifestURI->GetAsciiHost(mUpdateDomain);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIApplicationCacheService> cacheService =
        do_GetService(NS_APPLICATIONCACHESERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = cacheService->GetApplicationCache(clientID,
                                           getter_AddRefs(mApplicationCache));
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mApplicationCache) {
        nsAutoCString manifestSpec;
        rv = GetCacheKey(mManifestURI, manifestSpec);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = cacheService->CreateApplicationCache
            (manifestSpec, getter_AddRefs(mApplicationCache));
        NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = mApplicationCache->GetManifestURI(getter_AddRefs(mManifestURI));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoCString groupID;
    rv = mApplicationCache->GetGroupID(groupID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = nsOfflineCacheUpdateService::OfflineAppPinnedForURI(aDocumentURI,
                                                             nullptr,
                                                             &mPinned);
    NS_ENSURE_SUCCESS(rv, rv);

    mState = STATE_INITIALIZED;
    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::HandleManifest(bool *aDoUpdate)
{
    
    *aDoUpdate = false;

    bool succeeded;
    nsresult rv = mManifestItem->GetRequestSucceeded(&succeeded);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!succeeded || !mManifestItem->ParseSucceeded()) {
        return NS_ERROR_FAILURE;
    }

    if (!mManifestItem->NeedsUpdate()) {
        return NS_OK;
    }

    
    const nsCOMArray<nsIURI> &manifestURIs = mManifestItem->GetExplicitURIs();
    for (int32_t i = 0; i < manifestURIs.Count(); i++) {
        rv = AddURI(manifestURIs[i], nsIApplicationCache::ITEM_EXPLICIT);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    const nsCOMArray<nsIURI> &fallbackURIs = mManifestItem->GetFallbackURIs();
    for (int32_t i = 0; i < fallbackURIs.Count(); i++) {
        rv = AddURI(fallbackURIs[i], nsIApplicationCache::ITEM_FALLBACK);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
    rv = AddURI(mDocumentURI, nsIApplicationCache::ITEM_IMPLICIT);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = AddExistingItems(nsIApplicationCache::ITEM_IMPLICIT);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = AddExistingItems(nsIApplicationCache::ITEM_DYNAMIC);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    rv = AddExistingItems(nsIApplicationCache::ITEM_OPPORTUNISTIC,
                          &mManifestItem->GetOpportunisticNamespaces());
    NS_ENSURE_SUCCESS(rv, rv);

    *aDoUpdate = true;

    return NS_OK;
}

bool
nsOfflineCacheUpdate::CheckUpdateAvailability()
{
    nsresult rv;

    bool succeeded;
    rv = mManifestItem->GetRequestSucceeded(&succeeded);
    NS_ENSURE_SUCCESS(rv, false);

    if (!succeeded || !mManifestItem->ParseSucceeded()) {
        return false;
    }

    if (!mPinned) {
        uint16_t status;
        rv = mManifestItem->GetStatus(&status);
        NS_ENSURE_SUCCESS(rv, false);

        
        
        
        if (status == 404 || status == 410) {
            return true;
        }
    }

    return mManifestItem->NeedsUpdate();
}

void
nsOfflineCacheUpdate::LoadCompleted(nsOfflineCacheUpdateItem *aItem)
{
    nsresult rv;

    LOG(("nsOfflineCacheUpdate::LoadCompleted [%p]", this));

    if (mState == STATE_FINISHED) {
        LOG(("  after completion, ignoring"));
        return;
    }

    
    nsCOMPtr<nsIOfflineCacheUpdate> kungFuDeathGrip(this);

    if (mState == STATE_CANCELLED) {
        NotifyState(nsIOfflineCacheUpdateObserver::STATE_ERROR);
        Finish();
        return;
    }

    if (mState == STATE_CHECKING) {
        

        if (mOnlyCheckUpdate) {
            Finish();
            NotifyUpdateAvailability(CheckUpdateAvailability());
            return;
        }

        NS_ASSERTION(mManifestItem,
                     "Must have a manifest item in STATE_CHECKING.");
        NS_ASSERTION(mManifestItem == aItem,
                     "Unexpected aItem in nsOfflineCacheUpdate::LoadCompleted");

        
        
        
        uint16_t status;
        rv = mManifestItem->GetStatus(&status);
        if (status == 404 || status == 410) {
            LogToConsole("Offline cache manifest removed, cache cleared", mManifestItem);
            mSucceeded = false;
            if (mPreviousApplicationCache) {
                if (mPinned) {
                    
                    NotifyState(nsIOfflineCacheUpdateObserver::STATE_NOUPDATE);
                } else {
                    NotifyState(nsIOfflineCacheUpdateObserver::STATE_OBSOLETE);
                    mObsolete = true;
                }
            } else {
                NotifyState(nsIOfflineCacheUpdateObserver::STATE_ERROR);
                mObsolete = true;
            }
            Finish();
            return;
        }

        bool doUpdate;
        if (NS_FAILED(HandleManifest(&doUpdate))) {
            mSucceeded = false;
            NotifyState(nsIOfflineCacheUpdateObserver::STATE_ERROR);
            Finish();
            return;
        }

        if (!doUpdate) {
            LogToConsole("Offline cache doesn't need to update", mManifestItem);

            mSucceeded = false;

            AssociateDocuments(mPreviousApplicationCache);

            ScheduleImplicit();

            
            
            if (!mImplicitUpdate) {
                NotifyState(nsIOfflineCacheUpdateObserver::STATE_NOUPDATE);
                Finish();
            }
            return;
        }

        rv = mApplicationCache->MarkEntry(mManifestItem->mCacheKey,
                                          mManifestItem->mItemType);
        if (NS_FAILED(rv)) {
            mSucceeded = false;
            NotifyState(nsIOfflineCacheUpdateObserver::STATE_ERROR);
            Finish();
            return;
        }

        mState = STATE_DOWNLOADING;
        NotifyState(nsIOfflineCacheUpdateObserver::STATE_DOWNLOADING);

        
        ProcessNextURI();

        return;
    }

    
    if (mItemsInProgress) 
      --mItemsInProgress;

    bool succeeded;
    rv = aItem->GetRequestSucceeded(&succeeded);

    if (mPinned && NS_SUCCEEDED(rv) && succeeded) {
        uint32_t dummy_cache_type;
        rv = mApplicationCache->GetTypes(aItem->mCacheKey, &dummy_cache_type);
        bool item_doomed = NS_FAILED(rv); 

        if (item_doomed &&
            mPinnedEntryRetriesCount < kPinnedEntryRetriesLimit &&
            (aItem->mItemType & (nsIApplicationCache::ITEM_EXPLICIT |
                                 nsIApplicationCache::ITEM_FALLBACK))) {
            rv = EvictOneNonPinned();
            if (NS_FAILED(rv)) {
                mSucceeded = false;
                NotifyState(nsIOfflineCacheUpdateObserver::STATE_ERROR);
                Finish();
                return;
            }

            
            
            rv = aItem->Cancel();
            if (NS_FAILED(rv)) {
                mSucceeded = false;
                NotifyState(nsIOfflineCacheUpdateObserver::STATE_ERROR);
                Finish();
                return;
            }

            mPinnedEntryRetriesCount++;

            LogToConsole("An unpinned offline cache deleted");

            
            ProcessNextURI();
            return;
        }
    }

    
    
    
    
    mPinnedEntryRetriesCount = 0;

    
    
    if (NS_FAILED(rv) || !succeeded) {
        if (aItem->mItemType &
            (nsIApplicationCache::ITEM_EXPLICIT |
             nsIApplicationCache::ITEM_FALLBACK)) {
            LogToConsole("Offline cache manifest item failed to load", aItem);
            mSucceeded = false;
        }
    } else {
        rv = mApplicationCache->MarkEntry(aItem->mCacheKey, aItem->mItemType);
        if (NS_FAILED(rv)) {
            mSucceeded = false;
        }
    }

    if (!mSucceeded) {
        NotifyState(nsIOfflineCacheUpdateObserver::STATE_ERROR);
        Finish();
        return;
    }

    NotifyState(nsIOfflineCacheUpdateObserver::STATE_ITEMCOMPLETED);

    ProcessNextURI();
}

void
nsOfflineCacheUpdate::ManifestCheckCompleted(nsresult aStatus,
                                             const nsCString &aManifestHash)
{
    
    nsCOMPtr<nsIOfflineCacheUpdate> kungFuDeathGrip(this);

    if (NS_SUCCEEDED(aStatus)) {
        nsAutoCString firstManifestHash;
        mManifestItem->GetManifestHash(firstManifestHash);
        if (aManifestHash != firstManifestHash) {
            LOG(("Manifest has changed during cache items download [%p]", this));
            LogToConsole("Offline cache manifest changed during update", mManifestItem);
            aStatus = NS_ERROR_FAILURE;
        }
    }

    if (NS_FAILED(aStatus)) {
        mSucceeded = false;
        NotifyState(nsIOfflineCacheUpdateObserver::STATE_ERROR);
    }

    if (NS_FAILED(aStatus) && mRescheduleCount < kRescheduleLimit) {
        
        
        
        
        
        FinishNoNotify();

        nsRefPtr<nsOfflineCacheUpdate> newUpdate =
            new nsOfflineCacheUpdate();
        
        
        newUpdate->Init(mManifestURI, mDocumentURI, nullptr,
                        mCustomProfileDir, mAppID, mInBrowser);

        
        
        
        for (int32_t i = 0; i < mDocumentURIs.Count(); i++) {
            newUpdate->StickDocument(mDocumentURIs[i]);
        }

        newUpdate->mRescheduleCount = mRescheduleCount + 1;
        newUpdate->AddObserver(this, false);
        newUpdate->Schedule();
    }
    else {
        LogToConsole("Offline cache update done", mManifestItem);
        Finish();
    }
}

nsresult
nsOfflineCacheUpdate::Begin()
{
    LOG(("nsOfflineCacheUpdate::Begin [%p]", this));

    
    nsCOMPtr<nsIOfflineCacheUpdate> kungFuDeathGrip(this);

    mItemsInProgress = 0;

    if (mState == STATE_CANCELLED) {
      nsRefPtr<nsRunnableMethod<nsOfflineCacheUpdate> > errorNotification =
        NS_NewRunnableMethod(this,
                             &nsOfflineCacheUpdate::AsyncFinishWithError);
      nsresult rv = NS_DispatchToMainThread(errorNotification);
      NS_ENSURE_SUCCESS(rv, rv);

      return NS_OK;
    }

    if (mPartialUpdate) {
        mState = STATE_DOWNLOADING;
        NotifyState(nsIOfflineCacheUpdateObserver::STATE_DOWNLOADING);
        ProcessNextURI();
        return NS_OK;
    }

    
    nsCOMPtr<nsIURI> uri;

    mManifestItem = new nsOfflineManifestItem(mManifestURI,
                                              mDocumentURI,
                                              mApplicationCache,
                                              mPreviousApplicationCache);
    if (!mManifestItem) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    mState = STATE_CHECKING;
    mByteProgress = 0;
    NotifyState(nsIOfflineCacheUpdateObserver::STATE_CHECKING);

    nsresult rv = mManifestItem->OpenChannel(this);
    if (NS_FAILED(rv)) {
        LoadCompleted(mManifestItem);
    }

    return NS_OK;
}





nsresult
nsOfflineCacheUpdate::AddExistingItems(uint32_t aType,
                                       nsTArray<nsCString>* namespaceFilter)
{
    if (!mPreviousApplicationCache) {
        return NS_OK;
    }

    if (namespaceFilter && namespaceFilter->Length() == 0) {
        
        
        return NS_OK;
    }

    uint32_t count = 0;
    char **keys = nullptr;
    nsresult rv = mPreviousApplicationCache->GatherEntries(aType,
                                                           &count, &keys);
    NS_ENSURE_SUCCESS(rv, rv);

    AutoFreeArray autoFree(count, keys);

    for (uint32_t i = 0; i < count; i++) {
        if (namespaceFilter) {
            bool found = false;
            for (uint32_t j = 0; j < namespaceFilter->Length() && !found; j++) {
                found = StringBeginsWith(nsDependentCString(keys[i]),
                                         namespaceFilter->ElementAt(j));
            }

            if (!found)
                continue;
        }

        nsCOMPtr<nsIURI> uri;
        if (NS_SUCCEEDED(NS_NewURI(getter_AddRefs(uri), keys[i]))) {
            rv = AddURI(uri, aType);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::ProcessNextURI()
{
    
    nsCOMPtr<nsIOfflineCacheUpdate> kungFuDeathGrip(this);

    LOG(("nsOfflineCacheUpdate::ProcessNextURI [%p, inprogress=%d, numItems=%d]",
         this, mItemsInProgress, mItems.Length()));

    if (mState != STATE_DOWNLOADING) {
        LOG(("  should only be called from the DOWNLOADING state, ignoring"));
        return NS_ERROR_UNEXPECTED;
    }

    nsOfflineCacheUpdateItem * runItem = nullptr;
    uint32_t completedItems = 0;
    for (uint32_t i = 0; i < mItems.Length(); ++i) {
        nsOfflineCacheUpdateItem * item = mItems[i];

        if (item->IsScheduled()) {
            runItem = item;
            break;
        }

        if (item->IsCompleted())
            ++completedItems;
    }

    if (completedItems == mItems.Length()) {
        LOG(("nsOfflineCacheUpdate::ProcessNextURI [%p]: all items loaded", this));

        if (mPartialUpdate) {
            return Finish();
        } else {
            
            
            
            
            nsRefPtr<nsManifestCheck> manifestCheck =
                new nsManifestCheck(this, mManifestURI, mDocumentURI);
            if (NS_FAILED(manifestCheck->Begin())) {
                mSucceeded = false;
                NotifyState(nsIOfflineCacheUpdateObserver::STATE_ERROR);
                return Finish();
            }

            return NS_OK;
        }
    }

    if (!runItem) {
        LOG(("nsOfflineCacheUpdate::ProcessNextURI [%p]:"
             " No more items to include in parallel load", this));
        return NS_OK;
    }

#if defined(PR_LOGGING)
    if (LOG_ENABLED()) {
        nsAutoCString spec;
        runItem->mURI->GetSpec(spec);
        LOG(("%p: Opening channel for %s", this, spec.get()));
    }
#endif

    ++mItemsInProgress;
    NotifyState(nsIOfflineCacheUpdateObserver::STATE_ITEMSTARTED);

    nsresult rv = runItem->OpenChannel(this);
    if (NS_FAILED(rv)) {
        LoadCompleted(runItem);
        return rv;
    }

    if (mItemsInProgress >= kParallelLoadLimit) {
        LOG(("nsOfflineCacheUpdate::ProcessNextURI [%p]:"
             " At parallel load limit", this));
        return NS_OK;
    }

    
    
    return NS_DispatchToCurrentThread(this);
}

void
nsOfflineCacheUpdate::GatherObservers(nsCOMArray<nsIOfflineCacheUpdateObserver> &aObservers)
{
    for (int32_t i = 0; i < mWeakObservers.Count(); i++) {
        nsCOMPtr<nsIOfflineCacheUpdateObserver> observer =
            do_QueryReferent(mWeakObservers[i]);
        if (observer)
            aObservers.AppendObject(observer);
        else
            mWeakObservers.RemoveObjectAt(i--);
    }

    for (int32_t i = 0; i < mObservers.Count(); i++) {
        aObservers.AppendObject(mObservers[i]);
    }
}

void
nsOfflineCacheUpdate::NotifyState(uint32_t state)
{
    LOG(("nsOfflineCacheUpdate::NotifyState [%p, %d]", this, state));

    if (state == STATE_ERROR) {
        LogToConsole("Offline cache update error", mManifestItem);
    }

    nsCOMArray<nsIOfflineCacheUpdateObserver> observers;
    GatherObservers(observers);

    for (int32_t i = 0; i < observers.Count(); i++) {
        observers[i]->UpdateStateChanged(this, state);
    }
}

void
nsOfflineCacheUpdate::NotifyUpdateAvailability(bool updateAvailable)
{
    if (!mUpdateAvailableObserver)
        return;

    LOG(("nsOfflineCacheUpdate::NotifyUpdateAvailability [this=%p, avail=%d]",
         this, updateAvailable));

    const char* topic = updateAvailable
                      ? "offline-cache-update-available"
                      : "offline-cache-update-unavailable";

    nsCOMPtr<nsIObserver> observer;
    observer.swap(mUpdateAvailableObserver);
    observer->Observe(mManifestURI, topic, nullptr);
}

void
nsOfflineCacheUpdate::AssociateDocuments(nsIApplicationCache* cache)
{
    if (!cache) {
        LOG(("nsOfflineCacheUpdate::AssociateDocuments bypassed"
             ", no cache provided [this=%p]", this));
        return;
    }

    nsCOMArray<nsIOfflineCacheUpdateObserver> observers;
    GatherObservers(observers);

    for (int32_t i = 0; i < observers.Count(); i++) {
        observers[i]->ApplicationCacheAvailable(cache);
    }
}

void
nsOfflineCacheUpdate::StickDocument(nsIURI *aDocumentURI)
{
    if (!aDocumentURI)
      return;

    mDocumentURIs.AppendObject(aDocumentURI);
}

void
nsOfflineCacheUpdate::SetOwner(nsOfflineCacheUpdateOwner *aOwner)
{
    NS_ASSERTION(!mOwner, "Tried to set cache update owner twice.");
    mOwner = aOwner;
}

bool
nsOfflineCacheUpdate::IsForGroupID(const nsCSubstring &groupID)
{
    return mGroupID == groupID;
}

bool
nsOfflineCacheUpdate::IsForProfile(nsIFile* aCustomProfileDir)
{
    if (!mCustomProfileDir && !aCustomProfileDir)
        return true;
    if (!mCustomProfileDir || !aCustomProfileDir)
        return false;

    bool equals;
    nsresult rv = mCustomProfileDir->Equals(aCustomProfileDir, &equals);

    return NS_SUCCEEDED(rv) && equals;
}

nsresult
nsOfflineCacheUpdate::UpdateFinished(nsOfflineCacheUpdate *aUpdate)
{
    
    nsCOMPtr<nsIOfflineCacheUpdate> kungFuDeathGrip(this);

    mImplicitUpdate = nullptr;

    NotifyState(nsIOfflineCacheUpdateObserver::STATE_NOUPDATE);
    Finish();

    return NS_OK;
}

void
nsOfflineCacheUpdate::OnByteProgress(uint64_t byteIncrement)
{
    mByteProgress += byteIncrement;
    NotifyState(nsIOfflineCacheUpdateObserver::STATE_ITEMPROGRESS);
}

nsresult
nsOfflineCacheUpdate::ScheduleImplicit()
{
    if (mDocumentURIs.Count() == 0)
        return NS_OK;

    nsresult rv;

    nsRefPtr<nsOfflineCacheUpdate> update = new nsOfflineCacheUpdate();
    NS_ENSURE_TRUE(update, NS_ERROR_OUT_OF_MEMORY);

    nsAutoCString clientID;
    if (mPreviousApplicationCache) {
        rv = mPreviousApplicationCache->GetClientID(clientID);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else if (mApplicationCache) {
        rv = mApplicationCache->GetClientID(clientID);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
        NS_ERROR("Offline cache update not having set mApplicationCache?");
    }

    rv = update->InitPartial(mManifestURI, clientID, mDocumentURI);
    NS_ENSURE_SUCCESS(rv, rv);

    for (int32_t i = 0; i < mDocumentURIs.Count(); i++) {
        rv = update->AddURI(mDocumentURIs[i],
              nsIApplicationCache::ITEM_IMPLICIT);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    update->SetOwner(this);
    rv = update->Begin();
    NS_ENSURE_SUCCESS(rv, rv);

    mImplicitUpdate = update;

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::FinishNoNotify()
{
    LOG(("nsOfflineCacheUpdate::Finish [%p]", this));

    mState = STATE_FINISHED;

    if (!mPartialUpdate && !mOnlyCheckUpdate) {
        if (mSucceeded) {
            nsIArray *namespaces = mManifestItem->GetNamespaces();
            nsresult rv = mApplicationCache->AddNamespaces(namespaces);
            if (NS_FAILED(rv)) {
                NotifyState(nsIOfflineCacheUpdateObserver::STATE_ERROR);
                mSucceeded = false;
            }

            rv = mApplicationCache->Activate();
            if (NS_FAILED(rv)) {
                NotifyState(nsIOfflineCacheUpdateObserver::STATE_ERROR);
                mSucceeded = false;
            }

            AssociateDocuments(mApplicationCache);
        }

        if (mObsolete) {
            nsCOMPtr<nsIApplicationCacheService> appCacheService =
                do_GetService(NS_APPLICATIONCACHESERVICE_CONTRACTID);
            if (appCacheService) {
                nsAutoCString groupID;
                mApplicationCache->GetGroupID(groupID);
                appCacheService->DeactivateGroup(groupID);
             }
        }

        if (!mSucceeded) {
            
            for (uint32_t i = 0; i < mItems.Length(); i++) {
                mItems[i]->Cancel();
            }

            mApplicationCache->Discard();
        }
    }

    nsresult rv = NS_OK;

    if (mOwner) {
        rv = mOwner->UpdateFinished(this);
        
        
        mOwner = mozilla::WeakPtr<nsOfflineCacheUpdateOwner>();
    }

    return rv;
}

nsresult
nsOfflineCacheUpdate::Finish()
{
    nsresult rv = FinishNoNotify();

    NotifyState(nsIOfflineCacheUpdateObserver::STATE_FINISHED);

    return rv;
}

void
nsOfflineCacheUpdate::AsyncFinishWithError()
{
    NotifyState(nsOfflineCacheUpdate::STATE_ERROR);
    Finish();
}

static nsresult
EvictOneOfCacheGroups(nsIApplicationCacheService *cacheService,
                      uint32_t count, const char * const *groups)
{
    nsresult rv;
    unsigned int i;

    for (i = 0; i < count; i++) {
        nsCOMPtr<nsIURI> uri;
        rv = NS_NewURI(getter_AddRefs(uri), groups[i]);
        NS_ENSURE_SUCCESS(rv, rv);

        nsDependentCString group_name(groups[i]);
        nsCOMPtr<nsIApplicationCache> cache;
        rv = cacheService->GetActiveCache(group_name, getter_AddRefs(cache));
        
        if (NS_FAILED(rv) || !cache)
            continue;

        bool pinned;
        rv = nsOfflineCacheUpdateService::OfflineAppPinnedForURI(uri,
                                                                 nullptr,
                                                                 &pinned);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!pinned) {
            rv = cache->Discard();
            return NS_OK;
        }
    }

    return NS_ERROR_FILE_NOT_FOUND;
}

nsresult
nsOfflineCacheUpdate::EvictOneNonPinned()
{
    nsresult rv;

    nsCOMPtr<nsIApplicationCacheService> cacheService =
        do_GetService(NS_APPLICATIONCACHESERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    uint32_t count;
    char **groups;
    rv = cacheService->GetGroupsTimeOrdered(&count, &groups);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = EvictOneOfCacheGroups(cacheService, count, groups);

    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(count, groups);
    return rv;
}





NS_IMETHODIMP
nsOfflineCacheUpdate::GetUpdateDomain(nsACString &aUpdateDomain)
{
    NS_ENSURE_TRUE(mState >= STATE_INITIALIZED, NS_ERROR_NOT_INITIALIZED);

    aUpdateDomain = mUpdateDomain;
    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdate::GetStatus(uint16_t *aStatus)
{
    switch (mState) {
    case STATE_CHECKING :
        *aStatus = nsIDOMOfflineResourceList::CHECKING;
        return NS_OK;
    case STATE_DOWNLOADING :
        *aStatus = nsIDOMOfflineResourceList::DOWNLOADING;
        return NS_OK;
    default :
        *aStatus = nsIDOMOfflineResourceList::IDLE;
        return NS_OK;
    }

    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsOfflineCacheUpdate::GetPartial(bool *aPartial)
{
    *aPartial = mPartialUpdate || mOnlyCheckUpdate;
    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdate::GetManifestURI(nsIURI **aManifestURI)
{
    NS_ENSURE_TRUE(mState >= STATE_INITIALIZED, NS_ERROR_NOT_INITIALIZED);

    NS_IF_ADDREF(*aManifestURI = mManifestURI);
    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdate::GetSucceeded(bool *aSucceeded)
{
    NS_ENSURE_TRUE(mState == STATE_FINISHED, NS_ERROR_NOT_AVAILABLE);

    *aSucceeded = mSucceeded;

    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdate::GetIsUpgrade(bool *aIsUpgrade)
{
    NS_ENSURE_TRUE(mState >= STATE_INITIALIZED, NS_ERROR_NOT_INITIALIZED);

    *aIsUpgrade = (mPreviousApplicationCache != nullptr);

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::AddURI(nsIURI *aURI, uint32_t aType)
{
    NS_ENSURE_TRUE(mState >= STATE_INITIALIZED, NS_ERROR_NOT_INITIALIZED);

    if (mState >= STATE_DOWNLOADING)
        return NS_ERROR_NOT_AVAILABLE;

    
    nsAutoCString scheme;
    aURI->GetScheme(scheme);

    bool match;
    if (NS_FAILED(mManifestURI->SchemeIs(scheme.get(), &match)) || !match)
        return NS_ERROR_FAILURE;

    
    for (uint32_t i = 0; i < mItems.Length(); i++) {
        bool equals;
        if (NS_SUCCEEDED(mItems[i]->mURI->Equals(aURI, &equals)) && equals) {
            
            mItems[i]->mItemType |= aType;
            return NS_OK;
        }
    }

    nsRefPtr<nsOfflineCacheUpdateItem> item =
        new nsOfflineCacheUpdateItem(aURI,
                                     mDocumentURI,
                                     mApplicationCache,
                                     mPreviousApplicationCache,
                                     aType);
    if (!item) return NS_ERROR_OUT_OF_MEMORY;

    mItems.AppendElement(item);
    mAddedItems = true;

    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdate::AddDynamicURI(nsIURI *aURI)
{
    if (GeckoProcessType_Default != XRE_GetProcessType())
        return NS_ERROR_NOT_IMPLEMENTED;

    
    
    if (mPartialUpdate) {
        nsAutoCString key;
        GetCacheKey(aURI, key);

        uint32_t types;
        nsresult rv = mApplicationCache->GetTypes(key, &types);
        if (NS_SUCCEEDED(rv)) {
            if (!(types & nsIApplicationCache::ITEM_DYNAMIC)) {
                mApplicationCache->MarkEntry
                    (key, nsIApplicationCache::ITEM_DYNAMIC);
            }
            return NS_OK;
        }
    }

    return AddURI(aURI, nsIApplicationCache::ITEM_DYNAMIC);
}

NS_IMETHODIMP
nsOfflineCacheUpdate::Cancel()
{
    LOG(("nsOfflineCacheUpdate::Cancel [%p]", this));

    if ((mState == STATE_FINISHED) || (mState == STATE_CANCELLED)) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    mState = STATE_CANCELLED;
    mSucceeded = false;

    
    for (uint32_t i = 0; i < mItems.Length(); ++i) {
        nsOfflineCacheUpdateItem * item = mItems[i];

        if (item->IsInProgress())
            item->Cancel();
    }

    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdate::AddObserver(nsIOfflineCacheUpdateObserver *aObserver,
                                  bool aHoldWeak)
{
    LOG(("nsOfflineCacheUpdate::AddObserver [%p] to update [%p]", aObserver, this));

    NS_ENSURE_TRUE(mState >= STATE_INITIALIZED, NS_ERROR_NOT_INITIALIZED);

    if (aHoldWeak) {
        nsCOMPtr<nsIWeakReference> weakRef = do_GetWeakReference(aObserver);
        mWeakObservers.AppendObject(weakRef);
    } else {
        mObservers.AppendObject(aObserver);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdate::RemoveObserver(nsIOfflineCacheUpdateObserver *aObserver)
{
    LOG(("nsOfflineCacheUpdate::RemoveObserver [%p] from update [%p]", aObserver, this));

    NS_ENSURE_TRUE(mState >= STATE_INITIALIZED, NS_ERROR_NOT_INITIALIZED);

    for (int32_t i = 0; i < mWeakObservers.Count(); i++) {
        nsCOMPtr<nsIOfflineCacheUpdateObserver> observer =
            do_QueryReferent(mWeakObservers[i]);
        if (observer == aObserver) {
            mWeakObservers.RemoveObjectAt(i);
            return NS_OK;
        }
    }

    for (int32_t i = 0; i < mObservers.Count(); i++) {
        if (mObservers[i] == aObserver) {
            mObservers.RemoveObjectAt(i);
            return NS_OK;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdate::GetByteProgress(uint64_t * _result)
{
    NS_ENSURE_ARG(_result);

    *_result = mByteProgress;
    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdate::Schedule()
{
    LOG(("nsOfflineCacheUpdate::Schedule [%p]", this));

    nsOfflineCacheUpdateService* service =
        nsOfflineCacheUpdateService::EnsureService();

    if (!service) {
        return NS_ERROR_FAILURE;
    }

    return service->ScheduleUpdate(this);
}

NS_IMETHODIMP
nsOfflineCacheUpdate::UpdateStateChanged(nsIOfflineCacheUpdate *aUpdate,
                                         uint32_t aState)
{
    if (aState == nsIOfflineCacheUpdateObserver::STATE_FINISHED) {
        
        
        
        bool succeeded;
        aUpdate->GetSucceeded(&succeeded);
        mSucceeded = succeeded;
    }

    NotifyState(aState);
    if (aState == nsIOfflineCacheUpdateObserver::STATE_FINISHED)
        aUpdate->RemoveObserver(this);

    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdate::ApplicationCacheAvailable(nsIApplicationCache *applicationCache)
{
    AssociateDocuments(applicationCache);
    return NS_OK;
}





NS_IMETHODIMP
nsOfflineCacheUpdate::Run()
{
    ProcessNextURI();
    return NS_OK;
}
