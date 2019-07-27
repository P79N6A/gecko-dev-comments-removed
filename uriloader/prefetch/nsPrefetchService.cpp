



#include "nsPrefetchService.h"
#include "nsICacheEntry.h"
#include "nsIServiceManager.h"
#include "nsICategoryManager.h"
#include "nsIObserverService.h"
#include "nsIWebProgress.h"
#include "nsCURILoader.h"
#include "nsICachingChannel.h"
#include "nsIHttpChannel.h"
#include "nsIURL.h"
#include "nsISimpleEnumerator.h"
#include "nsNetUtil.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsStreamUtils.h"
#include "nsAutoPtr.h"
#include "prtime.h"
#include "prlog.h"
#include "plstr.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "mozilla/Preferences.h"
#include "mozilla/Attributes.h"
#include "nsIDOMNode.h"
#include "nsINode.h"
#include "nsIDocument.h"

using namespace mozilla;

#if defined(PR_LOGGING)









static PRLogModuleInfo *gPrefetchLog;
#endif

#undef LOG
#define LOG(args) PR_LOG(gPrefetchLog, 4, args)

#undef LOG_ENABLED
#define LOG_ENABLED() PR_LOG_TEST(gPrefetchLog, 4)

#define PREFETCH_PREF "network.prefetch-next"





static inline uint32_t
PRTimeToSeconds(PRTime t_usec)
{
    PRTime usec_per_sec = PR_USEC_PER_SEC;
    return uint32_t(t_usec /= usec_per_sec);
}

#define NowInSeconds() PRTimeToSeconds(PR_Now())




class nsPrefetchQueueEnumerator MOZ_FINAL : public nsISimpleEnumerator
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISIMPLEENUMERATOR
    explicit nsPrefetchQueueEnumerator(nsPrefetchService *aService);

private:
    ~nsPrefetchQueueEnumerator();

    void Increment();

    nsRefPtr<nsPrefetchService> mService;
    nsRefPtr<nsPrefetchNode> mCurrent;
    bool mStarted;
};




nsPrefetchQueueEnumerator::nsPrefetchQueueEnumerator(nsPrefetchService *aService)
    : mService(aService)
    , mStarted(false)
{
    Increment();
}

nsPrefetchQueueEnumerator::~nsPrefetchQueueEnumerator()
{
}




NS_IMETHODIMP
nsPrefetchQueueEnumerator::HasMoreElements(bool *aHasMore)
{
    *aHasMore = (mCurrent != nullptr);
    return NS_OK;
}

NS_IMETHODIMP
nsPrefetchQueueEnumerator::GetNext(nsISupports **aItem)
{
    if (!mCurrent) return NS_ERROR_FAILURE;

    NS_ADDREF(*aItem = static_cast<nsIStreamListener*>(mCurrent.get()));

    Increment();

    return NS_OK;
}





void
nsPrefetchQueueEnumerator::Increment()
{
  if (!mStarted) {
    
    
    
    mStarted = true;
    mCurrent = mService->GetCurrentNode();
    if (!mCurrent)
      mCurrent = mService->GetQueueHead();
    return;
  }

  if (mCurrent) {
    if (mCurrent == mService->GetCurrentNode()) {
      
      
      mCurrent = mService->GetQueueHead();
    }
    else {
      
      mCurrent = mCurrent->mNext;
    }
  }
}





NS_IMPL_ISUPPORTS(nsPrefetchQueueEnumerator, nsISimpleEnumerator)





nsPrefetchNode::nsPrefetchNode(nsPrefetchService *aService,
                               nsIURI *aURI,
                               nsIURI *aReferrerURI,
                               nsIDOMNode *aSource)
    : mNext(nullptr)
    , mURI(aURI)
    , mReferrerURI(aReferrerURI)
    , mService(aService)
    , mChannel(nullptr)
    , mBytesRead(0)
{
    mSource = do_GetWeakReference(aSource);
}

nsresult
nsPrefetchNode::OpenChannel()
{
    nsCOMPtr<nsINode> source = do_QueryReferent(mSource);
    if (!source) {
        
        
        return NS_ERROR_FAILURE;
    }
    nsCOMPtr<nsILoadGroup> loadGroup = source->OwnerDoc()->GetDocumentLoadGroup();
    nsresult rv = NS_NewChannel(getter_AddRefs(mChannel),
                                mURI,
                                nullptr, loadGroup, this,
                                nsIRequest::LOAD_BACKGROUND |
                                nsICachingChannel::LOAD_ONLY_IF_MODIFIED);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIHttpChannel> httpChannel =
        do_QueryInterface(mChannel);
    if (httpChannel) {
        httpChannel->SetReferrer(mReferrerURI);
        httpChannel->SetRequestHeader(
            NS_LITERAL_CSTRING("X-Moz"),
            NS_LITERAL_CSTRING("prefetch"),
            false);
    }

    rv = mChannel->AsyncOpen(this, nullptr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
nsPrefetchNode::CancelChannel(nsresult error)
{
    mChannel->Cancel(error);
    mChannel = nullptr;

    return NS_OK;
}





NS_IMPL_ISUPPORTS(nsPrefetchNode,
                  nsIRequestObserver,
                  nsIStreamListener,
                  nsIInterfaceRequestor,
                  nsIChannelEventSink,
                  nsIRedirectResultListener)





NS_IMETHODIMP
nsPrefetchNode::OnStartRequest(nsIRequest *aRequest,
                               nsISupports *aContext)
{
    nsresult rv;

    nsCOMPtr<nsICachingChannel> cachingChannel =
        do_QueryInterface(aRequest, &rv);
    if (NS_FAILED(rv)) return rv;

    
    bool fromCache;
    if (NS_SUCCEEDED(cachingChannel->IsFromCache(&fromCache)) &&
        fromCache) {
        LOG(("document is already in the cache; canceling prefetch\n"));
        return NS_BINDING_ABORTED;
    }

    
    
    
    
    nsCOMPtr<nsISupports> cacheToken;
    cachingChannel->GetCacheToken(getter_AddRefs(cacheToken));
    if (!cacheToken)
        return NS_ERROR_ABORT; 

    nsCOMPtr<nsICacheEntry> entryInfo =
        do_QueryInterface(cacheToken, &rv);
    if (NS_FAILED(rv)) return rv;

    uint32_t expTime;
    if (NS_SUCCEEDED(entryInfo->GetExpirationTime(&expTime))) {
        if (NowInSeconds() >= expTime) {
            LOG(("document cannot be reused from cache; "
                 "canceling prefetch\n"));
            return NS_BINDING_ABORTED;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsPrefetchNode::OnDataAvailable(nsIRequest *aRequest,
                                nsISupports *aContext,
                                nsIInputStream *aStream,
                                uint64_t aOffset,
                                uint32_t aCount)
{
    uint32_t bytesRead = 0;
    aStream->ReadSegments(NS_DiscardSegment, nullptr, aCount, &bytesRead);
    mBytesRead += bytesRead;
    LOG(("prefetched %u bytes [offset=%llu]\n", bytesRead, aOffset));
    return NS_OK;
}


NS_IMETHODIMP
nsPrefetchNode::OnStopRequest(nsIRequest *aRequest,
                              nsISupports *aContext,
                              nsresult aStatus)
{
    LOG(("done prefetching [status=%x]\n", aStatus));

    if (mBytesRead == 0 && aStatus == NS_OK) {
        
        
        
        mChannel->GetContentLength(&mBytesRead);
    }

    mService->NotifyLoadCompleted(this);
    mService->ProcessNextURI();
    return NS_OK;
}





NS_IMETHODIMP
nsPrefetchNode::GetInterface(const nsIID &aIID, void **aResult)
{
    if (aIID.Equals(NS_GET_IID(nsIChannelEventSink))) {
        NS_ADDREF_THIS();
        *aResult = static_cast<nsIChannelEventSink *>(this);
        return NS_OK;
    }

    if (aIID.Equals(NS_GET_IID(nsIRedirectResultListener))) {
        NS_ADDREF_THIS();
        *aResult = static_cast<nsIRedirectResultListener *>(this);
        return NS_OK;
    }

    return NS_ERROR_NO_INTERFACE;
}





NS_IMETHODIMP
nsPrefetchNode::AsyncOnChannelRedirect(nsIChannel *aOldChannel,
                                       nsIChannel *aNewChannel,
                                       uint32_t aFlags,
                                       nsIAsyncVerifyRedirectCallback *callback)
{
    nsCOMPtr<nsIURI> newURI;
    nsresult rv = aNewChannel->GetURI(getter_AddRefs(newURI));
    if (NS_FAILED(rv))
        return rv;

    bool match;
    rv = newURI->SchemeIs("http", &match); 
    if (NS_FAILED(rv) || !match) {
        rv = newURI->SchemeIs("https", &match); 
        if (NS_FAILED(rv) || !match) {
            LOG(("rejected: URL is not of type http/https\n"));
            return NS_ERROR_ABORT;
        }
    }

    
    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aNewChannel);
    NS_ENSURE_STATE(httpChannel);

    httpChannel->SetRequestHeader(NS_LITERAL_CSTRING("X-Moz"),
                                  NS_LITERAL_CSTRING("prefetch"),
                                  false);

    
    
    mRedirectChannel = aNewChannel;

    callback->OnRedirectVerifyCallback(NS_OK);
    return NS_OK;
}





NS_IMETHODIMP
nsPrefetchNode::OnRedirectResult(bool proceeding)
{
    if (proceeding && mRedirectChannel)
        mChannel = mRedirectChannel;

    mRedirectChannel = nullptr;

    return NS_OK;
}





nsPrefetchService::nsPrefetchService()
    : mQueueHead(nullptr)
    , mQueueTail(nullptr)
    , mStopCount(0)
    , mHaveProcessed(false)
    , mDisabled(true)
{
}

nsPrefetchService::~nsPrefetchService()
{
    Preferences::RemoveObserver(this, PREFETCH_PREF);
    
    
    EmptyQueue();
}

nsresult
nsPrefetchService::Init()
{
#if defined(PR_LOGGING)
    if (!gPrefetchLog)
        gPrefetchLog = PR_NewLogModule("nsPrefetch");
#endif

    nsresult rv;

    
    mDisabled = !Preferences::GetBool(PREFETCH_PREF, !mDisabled);
    Preferences::AddWeakObserver(this, PREFETCH_PREF);

    
    nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
    if (!observerService)
      return NS_ERROR_FAILURE;

    rv = observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, true);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mDisabled)
        AddProgressListener();

    return NS_OK;
}

void
nsPrefetchService::ProcessNextURI()
{
    nsresult rv;
    nsCOMPtr<nsIURI> uri, referrer;

    mCurrentNode = nullptr;

    do {
        rv = DequeueNode(getter_AddRefs(mCurrentNode));

        if (NS_FAILED(rv)) break;

#if defined(PR_LOGGING)
        if (LOG_ENABLED()) {
            nsAutoCString spec;
            mCurrentNode->mURI->GetSpec(spec);
            LOG(("ProcessNextURI [%s]\n", spec.get()));
        }
#endif

        
        
        
        nsRefPtr<nsPrefetchNode> node = mCurrentNode;
        rv = node->OpenChannel();
    }
    while (NS_FAILED(rv));
}

void
nsPrefetchService::NotifyLoadRequested(nsPrefetchNode *node)
{
    nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
    if (!observerService)
      return;

    observerService->NotifyObservers(static_cast<nsIStreamListener*>(node),
                                     "prefetch-load-requested", nullptr);
}

void
nsPrefetchService::NotifyLoadCompleted(nsPrefetchNode *node)
{
    nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
    if (!observerService)
      return;

    observerService->NotifyObservers(static_cast<nsIStreamListener*>(node),
                                     "prefetch-load-completed", nullptr);
}





void
nsPrefetchService::AddProgressListener()
{
    
    nsCOMPtr<nsIWebProgress> progress = 
        do_GetService(NS_DOCUMENTLOADER_SERVICE_CONTRACTID);
    if (progress)
        progress->AddProgressListener(this, nsIWebProgress::NOTIFY_STATE_DOCUMENT);
}

void
nsPrefetchService::RemoveProgressListener()
{
    
    nsCOMPtr<nsIWebProgress> progress =
        do_GetService(NS_DOCUMENTLOADER_SERVICE_CONTRACTID);
    if (progress)
        progress->RemoveProgressListener(this);
}

nsresult
nsPrefetchService::EnqueueNode(nsPrefetchNode *aNode)
{
    NS_ADDREF(aNode);

    if (!mQueueTail) {
        mQueueHead = aNode;
        mQueueTail = aNode;
    }
    else {
        mQueueTail->mNext = aNode;
        mQueueTail = aNode;
    }

    return NS_OK;
}

nsresult
nsPrefetchService::EnqueueURI(nsIURI *aURI,
                              nsIURI *aReferrerURI,
                              nsIDOMNode *aSource,
                              nsPrefetchNode **aNode)
{
    nsPrefetchNode *node = new nsPrefetchNode(this, aURI, aReferrerURI,
                                              aSource);
    if (!node)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aNode = node);

    return EnqueueNode(node);
}

nsresult
nsPrefetchService::DequeueNode(nsPrefetchNode **node)
{
    if (!mQueueHead)
        return NS_ERROR_NOT_AVAILABLE;

    
    *node = mQueueHead;
    mQueueHead = mQueueHead->mNext;
    (*node)->mNext = nullptr;

    if (!mQueueHead)
        mQueueTail = nullptr;

    return NS_OK;
}

void
nsPrefetchService::EmptyQueue()
{
    do {
        nsRefPtr<nsPrefetchNode> node;
        DequeueNode(getter_AddRefs(node));
    } while (mQueueHead);
}

void
nsPrefetchService::StartPrefetching()
{
    
    
    
    
    
    if (mStopCount > 0)
        mStopCount--;

    LOG(("StartPrefetching [stopcount=%d]\n", mStopCount));

    
    
    
    if (mStopCount == 0 && !mCurrentNode) {
        mHaveProcessed = true;
        ProcessNextURI();
    }
}

void
nsPrefetchService::StopPrefetching()
{
    mStopCount++;

    LOG(("StopPrefetching [stopcount=%d]\n", mStopCount));

    
    if (!mCurrentNode)
        return;

    mCurrentNode->CancelChannel(NS_BINDING_ABORTED);
    mCurrentNode = nullptr;
    EmptyQueue();
}





NS_IMPL_ISUPPORTS(nsPrefetchService,
                  nsIPrefetchService,
                  nsIWebProgressListener,
                  nsIObserver,
                  nsISupportsWeakReference)





nsresult
nsPrefetchService::Prefetch(nsIURI *aURI,
                            nsIURI *aReferrerURI,
                            nsIDOMNode *aSource,
                            bool aExplicit)
{
    nsresult rv;

    NS_ENSURE_ARG_POINTER(aURI);
    NS_ENSURE_ARG_POINTER(aReferrerURI);

#if defined(PR_LOGGING)
    if (LOG_ENABLED()) {
        nsAutoCString spec;
        aURI->GetSpec(spec);
        LOG(("PrefetchURI [%s]\n", spec.get()));
    }
#endif

    if (mDisabled) {
        LOG(("rejected: prefetch service is disabled\n"));
        return NS_ERROR_ABORT;
    }

    
    
    
    
    
    
    
    
    
    
    
    bool match;
    rv = aURI->SchemeIs("http", &match); 
    if (NS_FAILED(rv) || !match) {
        rv = aURI->SchemeIs("https", &match); 
        if (NS_FAILED(rv) || !match) {
            LOG(("rejected: URL is not of type http/https\n"));
            return NS_ERROR_ABORT;
        }
    }

    
    
    
    rv = aReferrerURI->SchemeIs("http", &match);
    if (NS_FAILED(rv) || !match) {
        rv = aReferrerURI->SchemeIs("https", &match);
        if (NS_FAILED(rv) || !match) {
            LOG(("rejected: referrer URL is neither http nor https\n"));
            return NS_ERROR_ABORT;
        }
    }

    
    
    if (!aExplicit) {
        nsCOMPtr<nsIURL> url(do_QueryInterface(aURI, &rv));
        if (NS_FAILED(rv)) return rv;
        nsAutoCString query;
        rv = url->GetQuery(query);
        if (NS_FAILED(rv) || !query.IsEmpty()) {
            LOG(("rejected: URL has a query string\n"));
            return NS_ERROR_ABORT;
        }
    }

    
    
    
    if (mCurrentNode) {
        bool equals;
        if (NS_SUCCEEDED(mCurrentNode->mURI->Equals(aURI, &equals)) && equals) {
            LOG(("rejected: URL is already being prefetched\n"));
            return NS_ERROR_ABORT;
        }
    }

    
    
    
    nsPrefetchNode *node = mQueueHead;
    for (; node; node = node->mNext) {
        bool equals;
        if (NS_SUCCEEDED(node->mURI->Equals(aURI, &equals)) && equals) {
            LOG(("rejected: URL is already on prefetch queue\n"));
            return NS_ERROR_ABORT;
        }
    }

    nsRefPtr<nsPrefetchNode> enqueuedNode;
    rv = EnqueueURI(aURI, aReferrerURI, aSource,
                    getter_AddRefs(enqueuedNode));
    NS_ENSURE_SUCCESS(rv, rv);

    NotifyLoadRequested(enqueuedNode);

    
    if (mStopCount == 0 && mHaveProcessed)
        ProcessNextURI();

    return NS_OK;
}

NS_IMETHODIMP
nsPrefetchService::PrefetchURI(nsIURI *aURI,
                               nsIURI *aReferrerURI,
                               nsIDOMNode *aSource,
                               bool aExplicit)
{
    return Prefetch(aURI, aReferrerURI, aSource, aExplicit);
}

NS_IMETHODIMP
nsPrefetchService::EnumerateQueue(nsISimpleEnumerator **aEnumerator)
{
    *aEnumerator = new nsPrefetchQueueEnumerator(this);
    if (!*aEnumerator) return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aEnumerator);

    return NS_OK;
}





NS_IMETHODIMP
nsPrefetchService::OnProgressChange(nsIWebProgress *aProgress,
                                  nsIRequest *aRequest, 
                                  int32_t curSelfProgress, 
                                  int32_t maxSelfProgress, 
                                  int32_t curTotalProgress, 
                                  int32_t maxTotalProgress)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}

NS_IMETHODIMP 
nsPrefetchService::OnStateChange(nsIWebProgress* aWebProgress, 
                                 nsIRequest *aRequest, 
                                 uint32_t progressStateFlags, 
                                 nsresult aStatus)
{
    if (progressStateFlags & STATE_IS_DOCUMENT) {
        if (progressStateFlags & STATE_STOP)
            StartPrefetching();
        else if (progressStateFlags & STATE_START)
            StopPrefetching();
    }
            
    return NS_OK;
}


NS_IMETHODIMP
nsPrefetchService::OnLocationChange(nsIWebProgress* aWebProgress,
                                    nsIRequest* aRequest,
                                    nsIURI *location,
                                    uint32_t aFlags)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}

NS_IMETHODIMP 
nsPrefetchService::OnStatusChange(nsIWebProgress* aWebProgress,
                                  nsIRequest* aRequest,
                                  nsresult aStatus,
                                  const char16_t* aMessage)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}

NS_IMETHODIMP 
nsPrefetchService::OnSecurityChange(nsIWebProgress *aWebProgress, 
                                    nsIRequest *aRequest, 
                                    uint32_t state)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}





NS_IMETHODIMP
nsPrefetchService::Observe(nsISupports     *aSubject,
                           const char      *aTopic,
                           const char16_t *aData)
{
    LOG(("nsPrefetchService::Observe [topic=%s]\n", aTopic));

    if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
        StopPrefetching();
        EmptyQueue();
        mDisabled = true;
    }
    else if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
        if (Preferences::GetBool(PREFETCH_PREF, false)) {
            if (mDisabled) {
                LOG(("enabling prefetching\n"));
                mDisabled = false;
                AddProgressListener();
            }
        } 
        else {
            if (!mDisabled) {
                LOG(("disabling prefetching\n"));
                StopPrefetching();
                EmptyQueue();
                mDisabled = true;
                RemoveProgressListener();
            }
        }
    }

    return NS_OK;
}


