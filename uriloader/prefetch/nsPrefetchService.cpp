




































#include "nsPrefetchService.h"
#include "nsICacheSession.h"
#include "nsICacheService.h"
#include "nsIServiceManager.h"
#include "nsICategoryManager.h"
#include "nsIObserverService.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"
#include "nsIDocCharset.h"
#include "nsIWebProgress.h"
#include "nsCURILoader.h"
#include "nsICachingChannel.h"
#include "nsICacheVisitor.h"
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

#if defined(PR_LOGGING)









static PRLogModuleInfo *gPrefetchLog;
#endif
#define LOG(args) PR_LOG(gPrefetchLog, 4, args)
#define LOG_ENABLED() PR_LOG_TEST(gPrefetchLog, 4)

#define PREFETCH_PREF "network.prefetch-next"





static inline PRUint32
PRTimeToSeconds(PRTime t_usec)
{
    PRTime usec_per_sec;
    PRUint32 t_sec;
    LL_I2L(usec_per_sec, PR_USEC_PER_SEC);
    LL_DIV(t_usec, t_usec, usec_per_sec);
    LL_L2I(t_sec, t_usec);
    return t_sec;
}

#define NowInSeconds() PRTimeToSeconds(PR_Now())




class nsPrefetchQueueEnumerator : public nsISimpleEnumerator
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISIMPLEENUMERATOR
    nsPrefetchQueueEnumerator(nsPrefetchService *aService);
    ~nsPrefetchQueueEnumerator();

private:
    void Increment();

    nsRefPtr<nsPrefetchService> mService;
    nsRefPtr<nsPrefetchNode> mCurrent;
    PRBool mStarted;
};




nsPrefetchQueueEnumerator::nsPrefetchQueueEnumerator(nsPrefetchService *aService)
    : mService(aService)
    , mStarted(PR_FALSE)
{
    Increment();
}

nsPrefetchQueueEnumerator::~nsPrefetchQueueEnumerator()
{
}




NS_IMETHODIMP
nsPrefetchQueueEnumerator::HasMoreElements(PRBool *aHasMore)
{
    *aHasMore = (mCurrent != nsnull);
    return NS_OK;
}

NS_IMETHODIMP
nsPrefetchQueueEnumerator::GetNext(nsISupports **aItem)
{
    if (!mCurrent) return NS_ERROR_FAILURE;

    NS_ADDREF(*aItem = static_cast<nsIDOMLoadStatus*>(mCurrent.get()));

    Increment();

    return NS_OK;
}





void
nsPrefetchQueueEnumerator::Increment()
{
  if (!mStarted) {
    
    
    
    mStarted = PR_TRUE;
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





NS_IMPL_ISUPPORTS1(nsPrefetchQueueEnumerator, nsISimpleEnumerator)





nsPrefetchNode::nsPrefetchNode(nsPrefetchService *aService,
                               nsIURI *aURI,
                               nsIURI *aReferrerURI,
                               nsIDOMNode *aSource)
    : mNext(nsnull)
    , mURI(aURI)
    , mReferrerURI(aReferrerURI)
    , mService(aService)
    , mChannel(nsnull)
    , mState(nsIDOMLoadStatus::UNINITIALIZED)
    , mBytesRead(0)
{
    mSource = do_GetWeakReference(aSource);
}

nsresult
nsPrefetchNode::OpenChannel()
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
        httpChannel->SetRequestHeader(
            NS_LITERAL_CSTRING("X-Moz"),
            NS_LITERAL_CSTRING("prefetch"),
            PR_FALSE);
    }

    rv = mChannel->AsyncOpen(this, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    mState = nsIDOMLoadStatus::REQUESTED;

    return NS_OK;
}

nsresult
nsPrefetchNode::CancelChannel(nsresult error)
{
    mChannel->Cancel(error);
    mChannel = nsnull;

    mState = nsIDOMLoadStatus::UNINITIALIZED;

    return NS_OK;
}





NS_IMPL_ISUPPORTS5(nsPrefetchNode,
                   nsIDOMLoadStatus,
                   nsIRequestObserver,
                   nsIStreamListener,
                   nsIInterfaceRequestor,
                   nsIChannelEventSink)





NS_IMETHODIMP
nsPrefetchNode::OnStartRequest(nsIRequest *aRequest,
                               nsISupports *aContext)
{
    nsresult rv;

    nsCOMPtr<nsICachingChannel> cachingChannel =
        do_QueryInterface(aRequest, &rv);
    if (NS_FAILED(rv)) return rv;

    
    PRBool fromCache;
    if (NS_SUCCEEDED(cachingChannel->IsFromCache(&fromCache)) &&
        fromCache) {
        LOG(("document is already in the cache; canceling prefetch\n"));
        return NS_BINDING_ABORTED;
    }

    
    
    
    
    nsCOMPtr<nsISupports> cacheToken;
    cachingChannel->GetCacheToken(getter_AddRefs(cacheToken));
    if (!cacheToken)
        return NS_ERROR_ABORT; 

    nsCOMPtr<nsICacheEntryInfo> entryInfo =
        do_QueryInterface(cacheToken, &rv);
    if (NS_FAILED(rv)) return rv;

    PRUint32 expTime;
    if (NS_SUCCEEDED(entryInfo->GetExpirationTime(&expTime))) {
        if (NowInSeconds() >= expTime) {
            LOG(("document cannot be reused from cache; "
                 "canceling prefetch\n"));
            return NS_BINDING_ABORTED;
        }
    }

    mState = nsIDOMLoadStatus::RECEIVING;

    return NS_OK;
}

NS_IMETHODIMP
nsPrefetchNode::OnDataAvailable(nsIRequest *aRequest,
                                nsISupports *aContext,
                                nsIInputStream *aStream,
                                PRUint32 aOffset,
                                PRUint32 aCount)
{
    PRUint32 bytesRead = 0;
    aStream->ReadSegments(NS_DiscardSegment, nsnull, aCount, &bytesRead);
    mBytesRead += bytesRead;
    LOG(("prefetched %u bytes [offset=%u]\n", bytesRead, aOffset));
    return NS_OK;
}


NS_IMETHODIMP
nsPrefetchNode::OnStopRequest(nsIRequest *aRequest,
                              nsISupports *aContext,
                              nsresult aStatus)
{
    LOG(("done prefetching [status=%x]\n", aStatus));

    mState = nsIDOMLoadStatus::LOADED;

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

    return NS_ERROR_NO_INTERFACE;
}





NS_IMETHODIMP
nsPrefetchNode::AsyncOnChannelRedirect(nsIChannel *aOldChannel,
                                       nsIChannel *aNewChannel,
                                       PRUint32 aFlags,
                                       nsIAsyncVerifyRedirectCallback *callback)
{
    nsCOMPtr<nsIURI> newURI;
    nsresult rv = aNewChannel->GetURI(getter_AddRefs(newURI));
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsICachingChannel> oldCachingChannel =
        do_QueryInterface(aOldChannel);

    PRBool match;
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
                                  PR_FALSE);

    mChannel = aNewChannel;

    callback->OnRedirectVerifyCallback(NS_OK);
    return NS_OK;
}






nsPrefetchService::nsPrefetchService()
    : mQueueHead(nsnull)
    , mQueueTail(nsnull)
    , mStopCount(0)
    , mHaveProcessed(PR_FALSE)
    , mDisabled(PR_TRUE)
{
}

nsPrefetchService::~nsPrefetchService()
{
    
    
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

    
    nsCOMPtr<nsIPrefBranch2> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
    if (NS_SUCCEEDED(rv)) {
      PRBool enabled;
      rv = prefs->GetBoolPref(PREFETCH_PREF, &enabled);
      if (NS_SUCCEEDED(rv) && enabled)
        mDisabled = PR_FALSE;

      prefs->AddObserver(PREFETCH_PREF, this, PR_TRUE);
    }

    
    nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
    if (!observerService)
      return NS_ERROR_FAILURE;

    rv = observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_TRUE);
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

    mCurrentNode = nsnull;

    do {
        rv = DequeueNode(getter_AddRefs(mCurrentNode));

        if (NS_FAILED(rv)) break;

#if defined(PR_LOGGING)
        if (LOG_ENABLED()) {
            nsCAutoString spec;
            mCurrentNode->mURI->GetSpec(spec);
            LOG(("ProcessNextURI [%s]\n", spec.get()));
        }
#endif

        
        
        
        rv = mCurrentNode->OpenChannel();
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

    observerService->NotifyObservers(static_cast<nsIDOMLoadStatus*>(node),
                                     "prefetch-load-requested", nsnull);
}

void
nsPrefetchService::NotifyLoadCompleted(nsPrefetchNode *node)
{
    nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
    if (!observerService)
      return;

    observerService->NotifyObservers(static_cast<nsIDOMLoadStatus*>(node),
                                     "prefetch-load-completed", nsnull);
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
    (*node)->mNext = nsnull;

    if (!mQueueHead)
        mQueueTail = nsnull;

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
        mHaveProcessed = PR_TRUE;
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
    mCurrentNode = nsnull;
    EmptyQueue();
}





NS_IMPL_ISUPPORTS4(nsPrefetchService,
                   nsIPrefetchService,
                   nsIWebProgressListener,
                   nsIObserver,
                   nsISupportsWeakReference)





nsresult
nsPrefetchService::Prefetch(nsIURI *aURI,
                            nsIURI *aReferrerURI,
                            nsIDOMNode *aSource,
                            PRBool aExplicit)
{
    nsresult rv;

    NS_ENSURE_ARG_POINTER(aURI);
    NS_ENSURE_ARG_POINTER(aReferrerURI);

#if defined(PR_LOGGING)
    if (LOG_ENABLED()) {
        nsCAutoString spec;
        aURI->GetSpec(spec);
        LOG(("PrefetchURI [%s]\n", spec.get()));
    }
#endif

    if (mDisabled) {
        LOG(("rejected: prefetch service is disabled\n"));
        return NS_ERROR_ABORT;
    }

    
    
    
    
    
    
    
    
    
    
    
    PRBool match;
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
        nsCAutoString query;
        rv = url->GetQuery(query);
        if (NS_FAILED(rv) || !query.IsEmpty()) {
            LOG(("rejected: URL has a query string\n"));
            return NS_ERROR_ABORT;
        }
    }

    
    
    
    if (mCurrentNode) {
        PRBool equals;
        if (NS_SUCCEEDED(mCurrentNode->mURI->Equals(aURI, &equals)) && equals) {
            LOG(("rejected: URL is already being prefetched\n"));
            return NS_ERROR_ABORT;
        }
    }

    
    
    
    nsPrefetchNode *node = mQueueHead;
    for (; node; node = node->mNext) {
        PRBool equals;
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
                               PRBool aExplicit)
{
    return Prefetch(aURI, aReferrerURI, aSource, aExplicit);
}

NS_IMETHODIMP
nsPrefetchService::PrefetchURIForOfflineUse(nsIURI *aURI,
                                            nsIURI *aReferrerURI,
                                            nsIDOMNode *aSource,
                                            PRBool aExplicit)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPrefetchService::EnumerateQueue(PRBool aIncludeNormalItems,
                                  PRBool aIncludeOfflineItems,
                                  nsISimpleEnumerator **aEnumerator)
{
    NS_ENSURE_TRUE(aIncludeNormalItems && !aIncludeOfflineItems,
                   NS_ERROR_NOT_IMPLEMENTED);

    *aEnumerator = new nsPrefetchQueueEnumerator(this);
    if (!*aEnumerator) return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aEnumerator);

    return NS_OK;
}




NS_IMETHODIMP
nsPrefetchNode::GetSource(nsIDOMNode **aSource)
{
    *aSource = nsnull;
    nsCOMPtr<nsIDOMNode> source = do_QueryReferent(mSource);
    if (source)
        source.swap(*aSource);

    return NS_OK;
}

NS_IMETHODIMP
nsPrefetchNode::GetUri(nsAString &aURI)
{
    nsCAutoString spec;
    nsresult rv = mURI->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    CopyUTF8toUTF16(spec, aURI);
    return NS_OK;
}

NS_IMETHODIMP
nsPrefetchNode::GetTotalSize(PRInt32 *aTotalSize)
{
    if (mChannel) {
        return mChannel->GetContentLength(aTotalSize);
    }

    *aTotalSize = -1;
    return NS_OK;
}

NS_IMETHODIMP
nsPrefetchNode::GetLoadedSize(PRInt32 *aLoadedSize)
{
    *aLoadedSize = mBytesRead;
    return NS_OK;
}

NS_IMETHODIMP
nsPrefetchNode::GetReadyState(PRUint16 *aReadyState)
{
    *aReadyState = mState;
    return NS_OK;
}

NS_IMETHODIMP
nsPrefetchNode::GetStatus(PRUint16 *aStatus)
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





NS_IMETHODIMP
nsPrefetchService::OnProgressChange(nsIWebProgress *aProgress,
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
nsPrefetchService::OnStateChange(nsIWebProgress* aWebProgress, 
                                 nsIRequest *aRequest, 
                                 PRUint32 progressStateFlags, 
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
                                    nsIURI *location)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}

NS_IMETHODIMP 
nsPrefetchService::OnStatusChange(nsIWebProgress* aWebProgress,
                                  nsIRequest* aRequest,
                                  nsresult aStatus,
                                  const PRUnichar* aMessage)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}

NS_IMETHODIMP 
nsPrefetchService::OnSecurityChange(nsIWebProgress *aWebProgress, 
                                    nsIRequest *aRequest, 
                                    PRUint32 state)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}





NS_IMETHODIMP
nsPrefetchService::Observe(nsISupports     *aSubject,
                           const char      *aTopic,
                           const PRUnichar *aData)
{
    LOG(("nsPrefetchService::Observe [topic=%s]\n", aTopic));

    if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
        StopPrefetching();
        EmptyQueue();
        mDisabled = PR_TRUE;
    }
    else if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
        nsCOMPtr<nsIPrefBranch> prefs(do_QueryInterface(aSubject));
        PRBool enabled;
        nsresult rv = prefs->GetBoolPref(PREFETCH_PREF, &enabled);
        if (NS_SUCCEEDED(rv) && enabled) {
            if (mDisabled) {
                LOG(("enabling prefetching\n"));
                mDisabled = PR_FALSE;
                AddProgressListener();
            }
        } 
        else {
            if (!mDisabled) {
                LOG(("disabling prefetching\n"));
                StopPrefetching();
                EmptyQueue();
                mDisabled = PR_TRUE;
                RemoveProgressListener();
            }
        }
    }

    return NS_OK;
}


