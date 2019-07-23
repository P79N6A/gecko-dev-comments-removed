





































#include "nsOfflineCacheUpdate.h"

#include "nsCPrefetchService.h"
#include "nsCURILoader.h"
#include "nsICache.h"
#include "nsICacheService.h"
#include "nsICacheSession.h"
#include "nsICachingChannel.h"
#include "nsIDOMWindow.h"
#include "nsIDOMOfflineResourceList.h"
#include "nsIObserverService.h"
#include "nsIOfflineCacheSession.h"
#include "nsIWebProgress.h"
#include "nsNetCID.h"
#include "nsNetUtil.h"
#include "nsServiceManagerUtils.h"
#include "nsStreamUtils.h"
#include "nsThreadUtils.h"
#include "prlog.h"

static nsOfflineCacheUpdateService *gOfflineCacheUpdateService = nsnull;

#if defined(PR_LOGGING)









static PRLogModuleInfo *gOfflineCacheUpdateLog;
#endif
#define LOG(args) PR_LOG(gOfflineCacheUpdateLog, 4, args)
#define LOG_ENABLED() PR_LOG_TEST(gOfflineCacheUpdateLog, 4)

class AutoFreeArray {
public:
    AutoFreeArray(PRUint32 count, char **values)
        : mCount(count), mValues(values) {};
    ~AutoFreeArray() { NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(mCount, mValues); }
private:
    PRUint32 mCount;
    char **mValues;
};





NS_IMPL_ISUPPORTS6(nsOfflineCacheUpdateItem,
                   nsIDOMLoadStatus,
                   nsIRequestObserver,
                   nsIStreamListener,
                   nsIRunnable,
                   nsIInterfaceRequestor,
                   nsIChannelEventSink)





nsOfflineCacheUpdateItem::nsOfflineCacheUpdateItem(nsOfflineCacheUpdate *aUpdate,
                                                   nsIURI *aURI,
                                                   nsIURI *aReferrerURI,
                                                   const nsACString &aClientID)
    : mURI(aURI)
    , mReferrerURI(aReferrerURI)
    , mClientID(aClientID)
    , mUpdate(aUpdate)
    , mChannel(nsnull)
    , mState(nsIDOMLoadStatus::UNINITIALIZED)
    , mBytesRead(0)
{
}

nsOfflineCacheUpdateItem::~nsOfflineCacheUpdateItem()
{
}

nsresult
nsOfflineCacheUpdateItem::OpenChannel()
{
    nsresult rv = NS_NewChannel(getter_AddRefs(mChannel),
                                mURI,
                                nsnull, nsnull, this,
                                nsIRequest::LOAD_BACKGROUND |
                                nsICachingChannel::LOAD_ONLY_IF_MODIFIED);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIHttpChannel> httpChannel =
        do_QueryInterface(mChannel);
    if (httpChannel) {
        httpChannel->SetReferrer(mReferrerURI);
        httpChannel->SetRequestHeader(NS_LITERAL_CSTRING("X-Moz"),
                                      NS_LITERAL_CSTRING("offline-resource"),
                                      PR_FALSE);
    }

    nsCOMPtr<nsICachingChannel> cachingChannel =
        do_QueryInterface(mChannel);
    if (cachingChannel) {
        rv = cachingChannel->SetCacheForOfflineUse(PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!mClientID.IsEmpty()) {
            rv = cachingChannel->SetOfflineCacheClientID(mClientID);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    rv = mChannel->AsyncOpen(this, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    mState = nsIDOMLoadStatus::REQUESTED;

    return NS_OK;
}

nsresult
nsOfflineCacheUpdateItem::Cancel()
{
    if (mChannel) {
        mChannel->Cancel(NS_ERROR_ABORT);
        mChannel = nsnull;
    }

    mState = nsIDOMLoadStatus::UNINITIALIZED;

    return NS_OK;
}





NS_IMETHODIMP
nsOfflineCacheUpdateItem::OnStartRequest(nsIRequest *aRequest,
                                         nsISupports *aContext)
{
    mState = nsIDOMLoadStatus::RECEIVING;

    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdateItem::OnDataAvailable(nsIRequest *aRequest,
                                          nsISupports *aContext,
                                          nsIInputStream *aStream,
                                          PRUint32 aOffset,
                                          PRUint32 aCount)
{
    PRUint32 bytesRead = 0;
    aStream->ReadSegments(NS_DiscardSegment, nsnull, aCount, &bytesRead);
    mBytesRead += bytesRead;
    LOG(("loaded %u bytes into offline cache [offset=%u]\n",
         bytesRead, aOffset));
    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdateItem::OnStopRequest(nsIRequest *aRequest,
                                        nsISupports *aContext,
                                        nsresult aStatus)
{
    LOG(("done fetching offline item [status=%x]\n", aStatus));

    mState = nsIDOMLoadStatus::LOADED;

    if (mBytesRead == 0 && aStatus == NS_OK) {
        
        
        
        mChannel->GetContentLength(&mBytesRead);
    }

    
    
    NS_DispatchToCurrentThread(this);

    return NS_OK;
}





NS_IMETHODIMP
nsOfflineCacheUpdateItem::Run()
{
    mUpdate->LoadCompleted();

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
nsOfflineCacheUpdateItem::OnChannelRedirect(nsIChannel *aOldChannel,
                                            nsIChannel *aNewChannel,
                                            PRUint32 aFlags)
{
    nsCOMPtr<nsIURI> newURI;
    nsresult rv = aNewChannel->GetURI(getter_AddRefs(newURI));
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsICachingChannel> oldCachingChannel =
        do_QueryInterface(aOldChannel);
    nsCOMPtr<nsICachingChannel> newCachingChannel =
      do_QueryInterface(aOldChannel);
    if (newCachingChannel) {
        rv = newCachingChannel->SetCacheForOfflineUse(PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
        if (!mClientID.IsEmpty()) {
            rv = newCachingChannel->SetOfflineCacheClientID(mClientID);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    nsCAutoString oldScheme;
    mURI->GetScheme(oldScheme);

    PRBool match;
    if (NS_FAILED(newURI->SchemeIs(oldScheme.get(), &match)) || !match) {
        LOG(("rejected: redirected to a different scheme\n"));
        return NS_ERROR_ABORT;
    }

    
    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aNewChannel);
    NS_ENSURE_STATE(httpChannel);

    httpChannel->SetRequestHeader(NS_LITERAL_CSTRING("X-Moz"),
                                  NS_LITERAL_CSTRING("offline-resource"),
                                  PR_FALSE);

    mChannel = aNewChannel;

    return NS_OK;
}





NS_IMETHODIMP
nsOfflineCacheUpdateItem::GetSource(nsIDOMNode **aSource)
{
    *aSource = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdateItem::GetUri(nsAString &aURI)
{
    nsCAutoString spec;
    nsresult rv = mURI->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    CopyUTF8toUTF16(spec, aURI);
    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdateItem::GetTotalSize(PRInt32 *aTotalSize)
{
    if (mChannel) {
        return mChannel->GetContentLength(aTotalSize);
    }

    *aTotalSize = -1;
    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdateItem::GetLoadedSize(PRInt32 *aLoadedSize)
{
    *aLoadedSize = mBytesRead;
    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdateItem::GetReadyState(PRUint16 *aReadyState)
{
    *aReadyState = mState;
    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdateItem::GetStatus(PRUint16 *aStatus)
{
    if (!mChannel) {
        *aStatus = 0;
        return NS_OK;
    }

    nsresult rv;
    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(mChannel, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 httpStatus;
    rv = httpChannel->GetResponseStatus(&httpStatus);
    if (rv == NS_ERROR_NOT_AVAILABLE) {
        
        
        
        
        if (mState >= nsIDOMLoadStatus::RECEIVING) {
            *aStatus = NS_ERROR_NOT_AVAILABLE;
            return NS_OK;
        }

        *aStatus = 0;
        return NS_OK;
    }

    NS_ENSURE_SUCCESS(rv, rv);
    *aStatus = PRUint16(httpStatus);
    return NS_OK;
}









nsOfflineManifestItem::nsOfflineManifestItem(nsOfflineCacheUpdate *aUpdate,
                                             nsIURI *aURI,
                                             nsIURI *aReferrerURI,
                                             const nsACString &aClientID)
    : nsOfflineCacheUpdateItem(aUpdate, aURI, aReferrerURI, aClientID)
    , mParserState(PARSE_INIT)
    , mNeedsUpdate(PR_TRUE)
{
}

nsOfflineManifestItem::~nsOfflineManifestItem()
{
}






NS_METHOD
nsOfflineManifestItem::ReadManifest(nsIInputStream *aInputStream,
                                    void *aClosure,
                                    const char *aFromSegment,
                                    PRUint32 aOffset,
                                    PRUint32 aCount,
                                    PRUint32 *aBytesConsumed)
{
    nsOfflineManifestItem *manifest =
        static_cast<nsOfflineManifestItem*>(aClosure);

    *aBytesConsumed = aCount;

    if (manifest->mParserState == PARSE_ERROR) {
        
        return NS_OK;
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
                return NS_OK;
            }
            ++begin;
        }

        const nsCSubstring &magic = Substring(begin, end);

        if (!magic.EqualsLiteral("CACHE MANIFEST")) {
            mParserState = PARSE_ERROR;
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
        mParserState = PARSE_NETWORK_ENTRIES;
        return NS_OK;
    }

    nsresult rv;

    switch(mParserState) {
    case PARSE_INIT:
    case PARSE_ERROR: {
        
        return NS_ERROR_FAILURE;
    }
    case PARSE_CACHE_ENTRIES: {
        nsCOMPtr<nsIURI> uri;
        rv = NS_NewURI(getter_AddRefs(uri), line, nsnull, mURI);
        if (NS_FAILED(rv))
            break;

        nsCAutoString scheme;
        uri->GetScheme(scheme);

        
        PRBool match;
        if (NS_FAILED(mURI->SchemeIs(scheme.get(), &match)) || !match)
            break;

        mExplicitURIs.AppendObject(uri);
        break;
    }
    case PARSE_FALLBACK_ENTRIES:
    case PARSE_NETWORK_ENTRIES: {
        
        
        break;
    }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsOfflineManifestItem::OnStartRequest(nsIRequest *aRequest,
                                      nsISupports *aContext)
{
    nsresult rv;

    nsCOMPtr<nsIHttpChannel> channel = do_QueryInterface(aRequest, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool succeeded;
    rv = channel->GetRequestSucceeded(&succeeded);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!succeeded) {
        LOG(("HTTP request failed"));
        mParserState = PARSE_ERROR;
        return NS_ERROR_ABORT;
    }

    nsCAutoString contentType;
    rv = channel->GetContentType(contentType);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!contentType.EqualsLiteral("text/cache-manifest")) {
        LOG(("Rejected cache manifest with Content-Type %s (expecting text/cache-manifest)",
             contentType.get()));
        mParserState = PARSE_ERROR;
        return NS_ERROR_ABORT;
    }

    return nsOfflineCacheUpdateItem::OnStartRequest(aRequest, aContext);
}

NS_IMETHODIMP
nsOfflineManifestItem::OnDataAvailable(nsIRequest *aRequest,
                                       nsISupports *aContext,
                                       nsIInputStream *aStream,
                                       PRUint32 aOffset,
                                       PRUint32 aCount)
{
    PRUint32 bytesRead = 0;
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
    
    nsCString::const_iterator begin, end;
    mReadBuf.BeginReading(begin);
    mReadBuf.EndReading(end);
    nsresult rv = HandleManifestLine(begin, end);
    NS_ENSURE_SUCCESS(rv, rv);

    if (mBytesRead == 0) {
        
        
        mNeedsUpdate = PR_FALSE;

        
        
        
    }

    return nsOfflineCacheUpdateItem::OnStopRequest(aRequest, aContext, aStatus);
}





NS_IMPL_ISUPPORTS1(nsOfflineCacheUpdate,
                   nsIOfflineCacheUpdate)





nsOfflineCacheUpdate::nsOfflineCacheUpdate()
    : mState(STATE_UNINITIALIZED)
    , mAddedItems(PR_FALSE)
    , mPartialUpdate(PR_FALSE)
    , mSucceeded(PR_TRUE)
    , mCurrentItem(-1)
{
}

nsOfflineCacheUpdate::~nsOfflineCacheUpdate()
{
    LOG(("nsOfflineCacheUpdate::~nsOfflineCacheUpdate [%p]", this));
}

nsresult
nsOfflineCacheUpdate::Init(PRBool aPartialUpdate,
                           nsIURI *aManifestURI,
                           nsIURI *aDocumentURI)
{
    nsresult rv;

    
    nsOfflineCacheUpdateService* service =
        nsOfflineCacheUpdateService::EnsureService();
    if (!service)
        return NS_ERROR_FAILURE;

    LOG(("nsOfflineCacheUpdate::Init [%p]", this));

    mPartialUpdate = aPartialUpdate;

    
    PRBool match;
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

    nsCAutoString manifestSpec;

    rv = mManifestURI->GetAsciiSpec(manifestSpec);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 ref = manifestSpec.FindChar('#');
    if (ref != kNotFound)
        manifestSpec.Truncate(ref);

    mManifestOwnerSpec = manifestSpec;
    mManifestOwnerSpec.AppendLiteral("#manifest");

    mDynamicOwnerSpec = manifestSpec;
    mDynamicOwnerSpec.AppendLiteral("#dynamic");

    mDocumentURI = aDocumentURI;

    nsCOMPtr<nsICacheService> cacheService =
        do_GetService(NS_CACHESERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsICacheSession> session;
    rv = cacheService->CreateSession("HTTP-offline",
                                     nsICache::STORE_OFFLINE,
                                     nsICache::STREAM_BASED,
                                     getter_AddRefs(session));
    NS_ENSURE_SUCCESS(rv, rv);

    mMainCacheSession = do_QueryInterface(session, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (aPartialUpdate) {
        mCacheSession = mMainCacheSession;
    } else {
        rv = cacheService->CreateTemporaryClientID(nsICache::STORE_OFFLINE,
                                                   mClientID);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = cacheService->CreateSession(mClientID.get(),
                                         nsICache::STORE_OFFLINE,
                                         nsICache::STREAM_BASED,
                                         getter_AddRefs(session));
        NS_ENSURE_SUCCESS(rv, rv);

        mCacheSession = do_QueryInterface(session, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        
        rv = mCacheSession->AddOwnedKey(mUpdateDomain, mManifestOwnerSpec,
                                        manifestSpec);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    mState = STATE_INITIALIZED;

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::HandleManifest(PRBool *aDoUpdate)
{
    
    *aDoUpdate = PR_FALSE;

    PRUint16 status;
    nsresult rv = mManifestItem->GetStatus(&status);
    NS_ENSURE_SUCCESS(rv, rv);

    if (status == 0 || status >= 400 || !mManifestItem->ParseSucceeded()) {
        return NS_ERROR_FAILURE;
    }

    if (!mManifestItem->NeedsUpdate()) {
        return NS_OK;
    }

    
    const nsCOMArray<nsIURI> &manifestURIs = mManifestItem->GetExplicitURIs();
    for (PRInt32 i = 0; i < manifestURIs.Count(); i++) {
        rv = AddURI(manifestURIs[i], mManifestOwnerSpec);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
    rv = AddURI(mDocumentURI, mManifestOwnerSpec);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = AddOwnedItems(mDynamicOwnerSpec);
    NS_ENSURE_SUCCESS(rv, rv);

    *aDoUpdate = PR_TRUE;

    return NS_OK;
}

void
nsOfflineCacheUpdate::LoadCompleted()
{
    nsresult rv;

    LOG(("nsOfflineCacheUpdate::LoadCompleted [%p]", this));

    if (mState == STATE_CANCELLED) {
        Finish();
        return;
    }

    if (mState == STATE_CHECKING) {
        

        NS_ASSERTION(mManifestItem,
                     "Must have a manifest item in STATE_CHECKING.");

        PRBool doUpdate;
        if (NS_FAILED(HandleManifest(&doUpdate))) {
            mSucceeded = PR_FALSE;
            NotifyError();
            Finish();
            return;
        }

        if (!doUpdate) {
            mSucceeded = PR_FALSE;
            NotifyNoUpdate();
            Finish();
            return;
        }

        mState = STATE_DOWNLOADING;
        NotifyDownloading();

        
        ProcessNextURI();

        return;
    }

    

    nsRefPtr<nsOfflineCacheUpdateItem> item = mItems[mCurrentItem];
    mCurrentItem++;

    PRUint16 status;
    rv = item->GetStatus(&status);

    
    if (NS_FAILED(rv) || status == 0 || status >= 400) {
        mSucceeded = PR_FALSE;
        NotifyError();
        Finish();
        return;
    }

    rv = NotifyCompleted(item);
    if (NS_FAILED(rv)) return;

    ProcessNextURI();
}

nsresult
nsOfflineCacheUpdate::Begin()
{
    LOG(("nsOfflineCacheUpdate::Begin [%p]", this));

    mCurrentItem = 0;

    if (mPartialUpdate) {
        mState = STATE_DOWNLOADING;
        NotifyDownloading();
        ProcessNextURI();
        return NS_OK;
    }

    
    nsCOMPtr<nsIURI> uri;

    mManifestItem = new nsOfflineManifestItem(this, mManifestURI,
                                              mDocumentURI, mClientID);
    if (!mManifestItem) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    mState = STATE_CHECKING;
    NotifyChecking();

    nsresult rv = mManifestItem->OpenChannel();
    if (NS_FAILED(rv)) {
        LoadCompleted();
    }

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::Cancel()
{
    LOG(("nsOfflineCacheUpdate::Cancel [%p]", this));

    mState = STATE_CANCELLED;
    mSucceeded = PR_FALSE;

    if (mCurrentItem >= 0 &&
        mCurrentItem < static_cast<PRInt32>(mItems.Length())) {
        
        mItems[mCurrentItem]->Cancel();
    }

    return NS_OK;
}





nsresult
nsOfflineCacheUpdate::AddOwnedItems(const nsACString &aOwnerURI)
{
    PRUint32 count;
    char **keys;
    nsresult rv = mMainCacheSession->GetOwnedKeys(mUpdateDomain, aOwnerURI,
                                                  &count, &keys);
    NS_ENSURE_SUCCESS(rv, rv);

    AutoFreeArray autoFree(count, keys);

    for (PRUint32 i = 0; i < count; i++) {
        nsCOMPtr<nsIURI> uri;
        if (NS_SUCCEEDED(NS_NewURI(getter_AddRefs(uri), keys[i]))) {
            nsRefPtr<nsOfflineCacheUpdateItem> item =
                new nsOfflineCacheUpdateItem(this, uri, mDocumentURI,
                                             mClientID);
            if (!item) return NS_ERROR_OUT_OF_MEMORY;

            mItems.AppendElement(item);
        }
    }

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::ProcessNextURI()
{
    LOG(("nsOfflineCacheUpdate::ProcessNextURI [%p, current=%d, numItems=%d]",
         this, mCurrentItem, mItems.Length()));

    NS_ASSERTION(mState == STATE_DOWNLOADING,
                 "ProcessNextURI should only be called from the DOWNLOADING state");

    if (mCurrentItem >= static_cast<PRInt32>(mItems.Length())) {
        return Finish();
    }

#if defined(PR_LOGGING)
    if (LOG_ENABLED()) {
        nsCAutoString spec;
        mItems[mCurrentItem]->mURI->GetSpec(spec);
        LOG(("%p: Opening channel for %s", this, spec.get()));
    }
#endif

    NotifyStarted(mItems[mCurrentItem]);

    nsresult rv = mItems[mCurrentItem]->OpenChannel();
    if (NS_FAILED(rv)) {
        LoadCompleted();
        return rv;
    }

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::GatherObservers(nsCOMArray<nsIOfflineCacheUpdateObserver> &aObservers)
{
    for (PRInt32 i = 0; i < mWeakObservers.Count(); i++) {
        nsCOMPtr<nsIOfflineCacheUpdateObserver> observer =
            do_QueryReferent(mWeakObservers[i]);
        if (observer)
            aObservers.AppendObject(observer);
        else
            mWeakObservers.RemoveObjectAt(i--);
    }

    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        aObservers.AppendObject(mObservers[i]);
    }

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::NotifyError()
{
    LOG(("nsOfflineCacheUpdate::NotifyError [%p]", this));

    nsCOMArray<nsIOfflineCacheUpdateObserver> observers;
    nsresult rv = GatherObservers(observers);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRInt32 i = 0; i < observers.Count(); i++) {
        observers[i]->Error(this);
    }

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::NotifyChecking()
{
    LOG(("nsOfflineCacheUpdate::NotifyChecking [%p]", this));

    nsCOMArray<nsIOfflineCacheUpdateObserver> observers;
    nsresult rv = GatherObservers(observers);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRInt32 i = 0; i < observers.Count(); i++) {
        observers[i]->Checking(this);
    }

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::NotifyNoUpdate()
{
    LOG(("nsOfflineCacheUpdate::NotifyNoUpdate [%p]", this));

    nsCOMArray<nsIOfflineCacheUpdateObserver> observers;
    nsresult rv = GatherObservers(observers);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRInt32 i = 0; i < observers.Count(); i++) {
        observers[i]->NoUpdate(this);
    }

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::NotifyDownloading()
{
    LOG(("nsOfflineCacheUpdate::NotifyDownloading [%p]", this));

    nsCOMArray<nsIOfflineCacheUpdateObserver> observers;
    nsresult rv = GatherObservers(observers);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRInt32 i = 0; i < observers.Count(); i++) {
        observers[i]->Downloading(this);
    }

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::NotifyStarted(nsOfflineCacheUpdateItem *aItem)
{
    LOG(("nsOfflineCacheUpdate::NotifyStarted [%p, %p]", this, aItem));

    nsCOMArray<nsIOfflineCacheUpdateObserver> observers;
    nsresult rv = GatherObservers(observers);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRInt32 i = 0; i < observers.Count(); i++) {
        observers[i]->ItemStarted(this, aItem);
    }

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::NotifyCompleted(nsOfflineCacheUpdateItem *aItem)
{
    LOG(("nsOfflineCacheUpdate::NotifyCompleted [%p, %p]", this, aItem));

    nsCOMArray<nsIOfflineCacheUpdateObserver> observers;
    nsresult rv = GatherObservers(observers);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRInt32 i = 0; i < observers.Count(); i++) {
        observers[i]->ItemCompleted(this, aItem);
    }

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::Finish()
{
    LOG(("nsOfflineCacheUpdate::Finish [%p]", this));

    mState = STATE_FINISHED;

    nsOfflineCacheUpdateService* service =
        nsOfflineCacheUpdateService::EnsureService();

    if (!service)
        return NS_ERROR_FAILURE;

    if (!mPartialUpdate) {
        if (mSucceeded) {
            nsresult rv = mMainCacheSession->MergeTemporaryClientID(mClientID);
            if (NS_FAILED(rv)) {
                NotifyError();
                mSucceeded = PR_FALSE;
            }
        }

        if (!mSucceeded) {
            
            for (PRUint32 i = 0; i < mItems.Length(); i++) {
                mItems[i]->Cancel();
            }
        }
    }

    return service->UpdateFinished(this);
}





NS_IMETHODIMP
nsOfflineCacheUpdate::GetUpdateDomain(nsACString &aUpdateDomain)
{
    NS_ENSURE_TRUE(mState >= STATE_INITIALIZED, NS_ERROR_NOT_INITIALIZED);

    aUpdateDomain = mUpdateDomain;
    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdate::GetStatus(PRUint16 *aStatus)
{
    switch (mState) {
    case STATE_CHECKING :
        return nsIDOMOfflineResourceList::CHECKING;
    case STATE_DOWNLOADING :
        return nsIDOMOfflineResourceList::DOWNLOADING;
    default :
        return nsIDOMOfflineResourceList::IDLE;
    }

    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsOfflineCacheUpdate::GetPartial(PRBool *aPartial)
{
    *aPartial = mPartialUpdate;
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
nsOfflineCacheUpdate::GetSucceeded(PRBool *aSucceeded)
{
    NS_ENSURE_TRUE(mState == STATE_FINISHED, NS_ERROR_NOT_AVAILABLE);

    *aSucceeded = mSucceeded;

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::AddURI(nsIURI *aURI, const nsACString &aOwnerSpec)
{
    NS_ENSURE_TRUE(mState >= STATE_INITIALIZED, NS_ERROR_NOT_INITIALIZED);

    if (mState >= STATE_DOWNLOADING)
        return NS_ERROR_NOT_AVAILABLE;

    
    nsCAutoString scheme;
    aURI->GetScheme(scheme);

    PRBool match;
    if (NS_FAILED(mManifestURI->SchemeIs(scheme.get(), &match)) || !match)
        return NS_ERROR_FAILURE;

    
    nsCAutoString spec;
    nsresult rv = aURI->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCAutoString::const_iterator specStart, specEnd;
    spec.BeginReading(specStart);
    spec.EndReading(specEnd);
    if (FindCharInReadable('#', specStart, specEnd)) {
        spec.BeginReading(specEnd);
        rv = mCacheSession->AddOwnedKey(mUpdateDomain, aOwnerSpec,
                                        Substring(specEnd, specStart));
        NS_ENSURE_SUCCESS(rv, rv);
    } else {
        rv = mCacheSession->AddOwnedKey(mUpdateDomain, aOwnerSpec, spec);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    for (PRUint32 i = 0; i < mItems.Length(); i++) {
        PRBool equals;
        if (NS_SUCCEEDED(mItems[i]->mURI->Equals(aURI, &equals)) && equals) {
            return NS_OK;
        }
    }

    nsRefPtr<nsOfflineCacheUpdateItem> item =
        new nsOfflineCacheUpdateItem(this, aURI, mDocumentURI, mClientID);
    if (!item) return NS_ERROR_OUT_OF_MEMORY;

    mItems.AppendElement(item);
    mAddedItems = PR_TRUE;

    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdate::AddDynamicURI(nsIURI *aURI)
{
    return AddURI(aURI, mDynamicOwnerSpec);
}

NS_IMETHODIMP
nsOfflineCacheUpdate::GetCount(PRUint32 *aNumItems)
{
    LOG(("nsOfflineCacheUpdate::GetNumItems [%p, num=%d]",
         this, mItems.Length()));

    NS_ENSURE_TRUE(mState >= STATE_INITIALIZED, NS_ERROR_NOT_INITIALIZED);

    *aNumItems = mItems.Length();
    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdate::Item(PRUint32 aIndex, nsIDOMLoadStatus **aItem)
{
    LOG(("nsOfflineCacheUpdate::GetItems [%p, index=%d]", this, aIndex));

    NS_ENSURE_TRUE(mState >= STATE_INITIALIZED, NS_ERROR_NOT_INITIALIZED);

    if (aIndex < mItems.Length())
        NS_IF_ADDREF(*aItem = mItems.ElementAt(aIndex));
    else
        *aItem = nsnull;

    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdate::AddObserver(nsIOfflineCacheUpdateObserver *aObserver,
                                  PRBool aHoldWeak)
{
    LOG(("nsOfflineCacheUpdate::AddObserver [%p]", this));

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
    LOG(("nsOfflineCacheUpdate::RemoveObserver [%p]", this));

    NS_ENSURE_TRUE(mState >= STATE_INITIALIZED, NS_ERROR_NOT_INITIALIZED);

    for (PRInt32 i = 0; i < mWeakObservers.Count(); i++) {
        nsCOMPtr<nsIOfflineCacheUpdateObserver> observer =
            do_QueryReferent(mWeakObservers[i]);
        if (observer == aObserver) {
            mWeakObservers.RemoveObjectAt(i);
            return NS_OK;
        }
    }

    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        if (mObservers[i] == aObserver) {
            mObservers.RemoveObjectAt(i);
            return NS_OK;
        }
    }

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

    return service->Schedule(this);
}





NS_IMPL_ISUPPORTS4(nsOfflineCacheUpdateService,
                   nsIOfflineCacheUpdateService,
                   nsIWebProgressListener,
                   nsIObserver,
                   nsISupportsWeakReference)





nsOfflineCacheUpdateService::nsOfflineCacheUpdateService()
    : mDisabled(PR_FALSE)
    , mUpdateRunning(PR_FALSE)
{
}

nsOfflineCacheUpdateService::~nsOfflineCacheUpdateService()
{
    gOfflineCacheUpdateService = nsnull;
}

nsresult
nsOfflineCacheUpdateService::Init()
{
    nsresult rv;

#if defined(PR_LOGGING)
    if (!gOfflineCacheUpdateLog)
        gOfflineCacheUpdateLog = PR_NewLogModule("nsOfflineCacheUpdate");
#endif

    if (!mDocUpdates.Init())
        return NS_ERROR_FAILURE;

    
    nsCOMPtr<nsIObserverService> observerService =
        do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->AddObserver(this,
                                      NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                      PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIWebProgress> progress =
        do_GetService(NS_DOCUMENTLOADER_SERVICE_CONTRACTID);
    if (progress) {
        nsresult rv = progress->AddProgressListener
                          (this, nsIWebProgress::NOTIFY_STATE_DOCUMENT);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    gOfflineCacheUpdateService = this;

    return NS_OK;
}


nsOfflineCacheUpdateService *
nsOfflineCacheUpdateService::GetInstance()
{
    if (!gOfflineCacheUpdateService) {
        gOfflineCacheUpdateService = new nsOfflineCacheUpdateService();
        if (!gOfflineCacheUpdateService)
            return nsnull;
        NS_ADDREF(gOfflineCacheUpdateService);
        nsresult rv = gOfflineCacheUpdateService->Init();
        if (NS_FAILED(rv)) {
            NS_RELEASE(gOfflineCacheUpdateService);
            return nsnull;
        }
        return gOfflineCacheUpdateService;
    }

    NS_ADDREF(gOfflineCacheUpdateService);

    return gOfflineCacheUpdateService;
}


nsOfflineCacheUpdateService *
nsOfflineCacheUpdateService::EnsureService()
{
    if (!gOfflineCacheUpdateService) {
        
        nsCOMPtr<nsIOfflineCacheUpdateService> service =
            do_GetService(NS_OFFLINECACHEUPDATESERVICE_CONTRACTID);
    }

    return gOfflineCacheUpdateService;
}

nsresult
nsOfflineCacheUpdateService::Schedule(nsOfflineCacheUpdate *aUpdate)
{
    LOG(("nsOfflineCacheUpdateService::Schedule [%p, update=%p]",
         this, aUpdate));

    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService =
        do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    observerService->NotifyObservers(static_cast<nsIOfflineCacheUpdate*>(aUpdate),
                                     "offline-cache-update-added",
                                     nsnull);

    mUpdates.AppendElement(aUpdate);

    ProcessNextUpdate();

    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdateService::ScheduleOnDocumentStop(nsIURI *aManifestURI,
                                                    nsIURI *aDocumentURI,
                                                    nsIDOMDocument *aDocument)
{
    LOG(("nsOfflineCacheUpdateService::ScheduleOnDocumentStop [%p, manifestURI=%p, documentURI=%p doc=%p]",
         this, aManifestURI, aDocumentURI, aDocument));

    PendingUpdate *update = new PendingUpdate();
    update->mManifestURI = aManifestURI;
    update->mDocumentURI = aDocumentURI;
    if (!mDocUpdates.Put(aDocument, update))
        return NS_ERROR_FAILURE;

    return NS_OK;
}

nsresult
nsOfflineCacheUpdateService::UpdateFinished(nsOfflineCacheUpdate *aUpdate)
{
    LOG(("nsOfflineCacheUpdateService::UpdateFinished [%p, update=%p]",
         this, aUpdate));

    NS_ASSERTION(mUpdates.Length() > 0 &&
                 mUpdates[0] == aUpdate, "Unknown update completed");

    
    nsRefPtr<nsOfflineCacheUpdate> update = mUpdates[0];
    mUpdates.RemoveElementAt(0);
    mUpdateRunning = PR_FALSE;

    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService =
        do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    observerService->NotifyObservers(static_cast<nsIOfflineCacheUpdate*>(aUpdate),
                                     "offline-cache-update-completed",
                                     nsnull);

    ProcessNextUpdate();

    return NS_OK;
}





nsresult
nsOfflineCacheUpdateService::ProcessNextUpdate()
{
    LOG(("nsOfflineCacheUpdateService::ProcessNextUpdate [%p, num=%d]",
         this, mUpdates.Length()));

    if (mDisabled)
        return NS_ERROR_ABORT;

    if (mUpdateRunning)
        return NS_OK;

    if (mUpdates.Length() > 0) {
        mUpdateRunning = PR_TRUE;
        return mUpdates[0]->Begin();
    }

    return NS_OK;
}





NS_IMETHODIMP
nsOfflineCacheUpdateService::GetNumUpdates(PRUint32 *aNumUpdates)
{
    LOG(("nsOfflineCacheUpdateService::GetNumUpdates [%p]", this));

    *aNumUpdates = mUpdates.Length();
    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdateService::GetUpdate(PRUint32 aIndex,
                                       nsIOfflineCacheUpdate **aUpdate)
{
    LOG(("nsOfflineCacheUpdateService::GetUpdate [%p, %d]", this, aIndex));

    if (aIndex < mUpdates.Length()) {
        NS_ADDREF(*aUpdate = mUpdates[aIndex]);
    } else {
        *aUpdate = nsnull;
    }

    return NS_OK;
}


NS_IMETHODIMP
nsOfflineCacheUpdateService::ScheduleUpdate(nsIURI *aManifestURI,
                                            nsIURI *aDocumentURI,
                                            nsIOfflineCacheUpdate **aUpdate)
{
    
    nsresult rv;
    for (PRUint32 i = 0; i < mUpdates.Length(); i++) {
        nsRefPtr<nsOfflineCacheUpdate> update = mUpdates[i];

        PRBool partial;
        rv = update->GetPartial(&partial);
        NS_ENSURE_SUCCESS(rv, rv);

        if (partial) {
            
            continue;
        }

        nsCOMPtr<nsIURI> manifestURI;
        update->GetManifestURI(getter_AddRefs(manifestURI));
        if (manifestURI) {
            PRBool equals;
            rv = manifestURI->Equals(aManifestURI, &equals);
            if (equals) {
                NS_ADDREF(*aUpdate = update);
                return NS_OK;
            }
        }
    }

    

    nsRefPtr<nsOfflineCacheUpdate> update = new nsOfflineCacheUpdate();
    if (!update)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = update->Init(PR_FALSE, aManifestURI, aDocumentURI);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = update->Schedule();
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ADDREF(*aUpdate = update);

    return NS_OK;
}





NS_IMETHODIMP
nsOfflineCacheUpdateService::Observe(nsISupports     *aSubject,
                                     const char      *aTopic,
                                     const PRUnichar *aData)
{
    if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
        if (mUpdates.Length() > 0)
            mUpdates[0]->Cancel();
        mDisabled = PR_TRUE;
    }

    return NS_OK;
}





NS_IMETHODIMP
nsOfflineCacheUpdateService::OnProgressChange(nsIWebProgress *aProgress,
                                              nsIRequest *aRequest,
                                              PRInt32 curSelfProgress,
                                              PRInt32 maxSelfProgress,
                                              PRInt32 curTotalProgress,
                                              PRInt32 maxTotalProgress)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdateService::OnStateChange(nsIWebProgress* aWebProgress,
                                           nsIRequest *aRequest,
                                           PRUint32 progressStateFlags,
                                           nsresult aStatus)
{
    if ((progressStateFlags & STATE_IS_DOCUMENT) &&
        (progressStateFlags & STATE_STOP)) {
        if (mDocUpdates.Count() == 0)
            return NS_OK;

        nsCOMPtr<nsIDOMWindow> window;
        aWebProgress->GetDOMWindow(getter_AddRefs(window));
        if (!window) return NS_OK;

        nsCOMPtr<nsIDOMDocument> doc;
        window->GetDocument(getter_AddRefs(doc));
        if (!doc) return NS_OK;

        LOG(("nsOfflineCacheUpdateService::OnStateChange [%p, doc=%p]",
             this, doc.get()));


        PendingUpdate *pendingUpdate;
        if (mDocUpdates.Get(doc, &pendingUpdate)) {
            
            if (NS_SUCCEEDED(aStatus)) {
                nsCOMPtr<nsIOfflineCacheUpdate> update;
                ScheduleUpdate(pendingUpdate->mManifestURI,
                               pendingUpdate->mDocumentURI,
                               getter_AddRefs(update));
            }
            mDocUpdates.Remove(doc);
        }

        return NS_OK;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdateService::OnLocationChange(nsIWebProgress* aWebProgress,
                                              nsIRequest* aRequest,
                                              nsIURI *location)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdateService::OnStatusChange(nsIWebProgress* aWebProgress,
                                            nsIRequest* aRequest,
                                            nsresult aStatus,
                                            const PRUnichar* aMessage)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdateService::OnSecurityChange(nsIWebProgress *aWebProgress,
                                              nsIRequest *aRequest,
                                              PRUint32 state)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}
