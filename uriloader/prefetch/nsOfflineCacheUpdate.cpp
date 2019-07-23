





































#include "nsOfflineCacheUpdate.h"

#include "nsCPrefetchService.h"
#include "nsCURILoader.h"
#include "nsICache.h"
#include "nsICacheService.h"
#include "nsICacheSession.h"
#include "nsICachingChannel.h"
#include "nsIDOMWindow.h"
#include "nsIObserverService.h"
#include "nsIOfflineCacheSession.h"
#include "nsIWebProgress.h"
#include "nsNetCID.h"
#include "nsNetUtil.h"
#include "nsServiceManagerUtils.h"
#include "nsStreamUtils.h"
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





NS_IMPL_ISUPPORTS5(nsOfflineCacheUpdateItem,
                   nsIDOMLoadStatus,
                   nsIRequestObserver,
                   nsIStreamListener,
                   nsIInterfaceRequestor,
                   nsIChannelEventSink)





nsOfflineCacheUpdateItem::nsOfflineCacheUpdateItem(nsOfflineCacheUpdate *aUpdate,
                                                   nsIURI *aURI,
                                                   nsIURI *aReferrerURI,
                                                   nsIDOMNode *aSource)
    : mURI(aURI)
    , mReferrerURI(aReferrerURI)
    , mUpdate(aUpdate)
    , mChannel(nsnull)
    , mState(nsIDOMLoadStatus::UNINITIALIZED)
    , mBytesRead(0)
{
    mSource = do_GetWeakReference(aSource);
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
    if (newCachingChannel)
      newCachingChannel->SetCacheForOfflineUse(PR_TRUE);

    PRBool match;
    rv = newURI->SchemeIs("http", &match);
    if (NS_FAILED(rv) || !match) {
        if (NS_FAILED(newURI->SchemeIs("https", &match)) ||
            !match) {
            LOG(("rejected: URL is not of type http\n"));
            return NS_ERROR_ABORT;
        }
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
    if (mSource) {
        return CallQueryReferent(mSource.get(), aSource);
    } else {
        *aSource = nsnull;
        return NS_OK;
    }
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





NS_IMPL_ISUPPORTS1(nsOfflineCacheUpdate,
                   nsIOfflineCacheUpdate);





nsOfflineCacheUpdate::nsOfflineCacheUpdate()
    : mState(STATE_UNINITIALIZED)
    , mAddedItems(PR_FALSE)
    , mPartialUpdate(PR_FALSE)
{
}

nsOfflineCacheUpdate::~nsOfflineCacheUpdate()
{
    LOG(("nsOfflineCacheUpdate::~nsOfflineCacheUpdate [%p]", this));
}

nsresult
nsOfflineCacheUpdate::Init(PRBool aPartialUpdate,
                           const nsACString &aUpdateDomain,
                           const nsACString &aOwnerURI,
                           nsIURI *aReferrerURI)
{
    nsresult rv;

    
    if (!nsOfflineCacheUpdateService::GetInstance()) {
        return NS_ERROR_FAILURE;
    }

    LOG(("nsOfflineCacheUpdate::Init [%p]", this));

    mPartialUpdate = aPartialUpdate;
    mUpdateDomain = aUpdateDomain;
    mOwnerURI = aOwnerURI;
    mReferrerURI = aReferrerURI;

    nsCOMPtr<nsICacheService> cacheService =
        do_GetService(NS_CACHESERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsICacheSession> session;
    rv = cacheService->CreateSession("HTTP-offline",
                                     nsICache::STORE_OFFLINE,
                                     nsICache::STREAM_BASED,
                                     getter_AddRefs(session));
    NS_ENSURE_SUCCESS(rv, rv);

    mCacheSession = do_QueryInterface(session, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    mState = STATE_INITIALIZED;

    return NS_OK;
}

void
nsOfflineCacheUpdate::LoadCompleted()
{
    nsresult rv;

    LOG(("nsOfflineCacheUpdate::LoadCompleted [%p]", this));

    NS_ASSERTION(mItems.Length() >= 1, "Unknown load completed");

    nsRefPtr<nsOfflineCacheUpdateItem> item = mItems[0];
    mItems.RemoveElementAt(0);

    rv = NotifyCompleted(item);
    if (NS_FAILED(rv)) return;

    ProcessNextURI();
}

nsresult
nsOfflineCacheUpdate::Begin()
{
    LOG(("nsOfflineCacheUpdate::Begin [%p]", this));

    if (!mPartialUpdate) {
        
        
        nsresult rv = AddDomainItems();
        NS_ENSURE_SUCCESS(rv, rv);
    }

    mState = STATE_RUNNING;

    ProcessNextURI();

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::Cancel()
{
    LOG(("nsOfflineCacheUpdate::Cancel [%p]", this));

    mState = STATE_CANCELLED;

    if (mItems.Length() > 0) {
        
        mItems[0]->Cancel();
    }

    return NS_OK;
}





nsresult
nsOfflineCacheUpdate::AddOwnedItems(const nsACString &aOwnerURI)
{
    PRUint32 count;
    char **keys;
    nsresult rv = mCacheSession->GetOwnedKeys(mUpdateDomain, aOwnerURI,
                                              &count, &keys);
    NS_ENSURE_SUCCESS(rv, rv);

    AutoFreeArray autoFree(count, keys);

    for (PRUint32 i = 0; i < count; i++) {
        nsCOMPtr<nsIURI> uri;
        if (NS_SUCCEEDED(NS_NewURI(getter_AddRefs(uri), keys[i]))) {
            nsRefPtr<nsOfflineCacheUpdateItem> item =
                new nsOfflineCacheUpdateItem(this, uri, mReferrerURI, nsnull);
            if (!item) return NS_ERROR_OUT_OF_MEMORY;

            mItems.AppendElement(item);
        }
    }

    return NS_OK;
}


nsresult
nsOfflineCacheUpdate::AddDomainItems()
{
    PRUint32 count;
    char **uris;
    nsresult rv = mCacheSession->GetOwnerURIs(mUpdateDomain, &count, &uris);
    NS_ENSURE_SUCCESS(rv, rv);

    AutoFreeArray autoFree(count, uris);

    for (PRUint32 i = 0; i < count; i++) {
        const char *ownerURI = uris[i];
        
        
        if (!mAddedItems || !mOwnerURI.Equals(ownerURI)) {
            rv = AddOwnedItems(nsDependentCString(ownerURI));
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::ProcessNextURI()
{
    LOG(("nsOfflineCacheUpdate::ProcessNextURI [%p, numItems=%d]",
         this, mItems.Length()));

    if (mState == STATE_CANCELLED || mItems.Length() == 0) {
        return Finish();
    }

#if defined(PR_LOGGING)
    if (LOG_ENABLED()) {
        nsCAutoString spec;
        mItems[0]->mURI->GetSpec(spec);
        LOG(("%p: Opening channel for %s", this, spec.get()));
    }
#endif

    nsresult rv = mItems[0]->OpenChannel();
    if (NS_FAILED(rv)) {
        LoadCompleted();
        return rv;
    }

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::NotifyCompleted(nsOfflineCacheUpdateItem *aItem)
{
    nsCOMArray<nsIOfflineCacheUpdateObserver> observers;

    for (PRInt32 i = 0; i < mWeakObservers.Count(); i++) {
        nsCOMPtr<nsIOfflineCacheUpdateObserver> observer =
            do_QueryReferent(mWeakObservers[i]);
        if (observer)
            observers.AppendObject(observer);
        else
            mWeakObservers.RemoveObjectAt(i--);
    }

    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        observers.AppendObject(mObservers[i]);
    }

    for (PRInt32 i = 0; i < observers.Count(); i++) {
        observers[i]->ItemCompleted(aItem);
    }

    return NS_OK;
}

nsresult
nsOfflineCacheUpdate::Finish()
{
    LOG(("nsOfflineCacheUpdate::Finish [%p]", this));

    mState = STATE_FINISHED;

    nsOfflineCacheUpdateService *service =
        nsOfflineCacheUpdateService::GetInstance();

    if (!service)
        return NS_ERROR_FAILURE;

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
nsOfflineCacheUpdate::GetOwnerURI(nsACString &aOwnerURI)
{
    NS_ENSURE_TRUE(mState >= STATE_INITIALIZED, NS_ERROR_NOT_INITIALIZED);

    aOwnerURI = mOwnerURI;
    return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheUpdate::AddURI(nsIURI *aURI, nsIDOMNode *aSource)
{
    NS_ENSURE_TRUE(mState >= STATE_INITIALIZED, NS_ERROR_NOT_INITIALIZED);

    if (mState >= STATE_RUNNING)
        return NS_ERROR_NOT_AVAILABLE;

    
    PRBool match;
    nsresult rv = aURI->SchemeIs("http", &match);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!match) {
        rv = aURI->SchemeIs("https", &match);
        NS_ENSURE_SUCCESS(rv, rv);
        if (!match)
            return NS_ERROR_ABORT;
    }

    
    nsCAutoString spec;
    rv = aURI->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCAutoString::const_iterator specStart, specEnd;
    spec.BeginReading(specStart);
    spec.EndReading(specEnd);
    if (FindCharInReadable('#', specStart, specEnd)) {
        spec.BeginReading(specEnd);
        rv = mCacheSession->AddOwnedKey(mUpdateDomain, mOwnerURI,
                                        Substring(specEnd, specStart));
        NS_ENSURE_SUCCESS(rv, rv);
    } else {
        rv = mCacheSession->AddOwnedKey(mUpdateDomain, mOwnerURI, spec);
        NS_ENSURE_SUCCESS(rv, rv);
    }

   nsRefPtr<nsOfflineCacheUpdateItem> item =
        new nsOfflineCacheUpdateItem(this, aURI, mReferrerURI, aSource);
    if (!item) return NS_ERROR_OUT_OF_MEMORY;

    mItems.AppendElement(item);
    mAddedItems = PR_TRUE;

    return NS_OK;
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

    nsOfflineCacheUpdateService *service =
        nsOfflineCacheUpdateService::GetInstance();

    if (!service) {
        return NS_ERROR_FAILURE;
    }

    return service->Schedule(this);
}

NS_IMETHODIMP
nsOfflineCacheUpdate::ScheduleOnDocumentStop(nsIDOMDocument *aDocument)
{
    LOG(("nsOfflineCacheUpdate::ScheduleOnDocumentStop [%p]", this));

    nsOfflineCacheUpdateService *service =
        nsOfflineCacheUpdateService::GetInstance();

    if (!service) {
        return NS_ERROR_FAILURE;
    }

    return service->ScheduleOnDocumentStop(this, aDocument);
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

nsresult
nsOfflineCacheUpdateService::ScheduleOnDocumentStop(nsOfflineCacheUpdate *aUpdate,
                                                    nsIDOMDocument *aDocument)
{
    LOG(("nsOfflineCacheUpdateService::ScheduleOnDocumentStop [%p, update=%p, doc=%p]",
         this, aUpdate, aDocument));

    if (!mDocUpdates.Put(aDocument, aUpdate))
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

        nsRefPtr<nsOfflineCacheUpdate> update;
        if (mDocUpdates.Get(doc, getter_AddRefs(update))) {
            Schedule(update);
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
