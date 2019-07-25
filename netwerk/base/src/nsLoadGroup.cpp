






































#include "nsLoadGroup.h"
#include "nsISupportsArray.h"
#include "nsEnumeratorUtils.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "prlog.h"
#include "nsCRT.h"
#include "netCore.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#include "nsTArray.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;

#if defined(PR_LOGGING)











static PRLogModuleInfo* gLoadGroupLog = nsnull;
#endif

#define LOG(args) PR_LOG(gLoadGroupLog, PR_LOG_DEBUG, args)

#define HISTOGRAM_TIME_DELTA(start, end) \
    (PRUint32)((end - start).ToMilliseconds())



class RequestMapEntry : public PLDHashEntryHdr
{
public:
    RequestMapEntry(nsIRequest *aRequest) :
        mKey(aRequest)
    {
    }

    nsCOMPtr<nsIRequest> mKey;
};

static PRBool
RequestHashMatchEntry(PLDHashTable *table, const PLDHashEntryHdr *entry,
                      const void *key)
{
    const RequestMapEntry *e =
        static_cast<const RequestMapEntry *>(entry);
    const nsIRequest *request = static_cast<const nsIRequest *>(key);

    return e->mKey == request;
}

static void
RequestHashClearEntry(PLDHashTable *table, PLDHashEntryHdr *entry)
{
    RequestMapEntry *e = static_cast<RequestMapEntry *>(entry);

    
    e->~RequestMapEntry();
}

static PRBool
RequestHashInitEntry(PLDHashTable *table, PLDHashEntryHdr *entry,
                     const void *key)
{
    const nsIRequest *const_request = static_cast<const nsIRequest *>(key);
    nsIRequest *request = const_cast<nsIRequest *>(const_request);

    
    new (entry) RequestMapEntry(request);
    return PR_TRUE;
}


static void
RescheduleRequest(nsIRequest *aRequest, PRInt32 delta)
{
    nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(aRequest);
    if (p)
        p->AdjustPriority(delta);
}

static PLDHashOperator
RescheduleRequests(PLDHashTable *table, PLDHashEntryHdr *hdr,
                   PRUint32 number, void *arg)
{
    RequestMapEntry *e = static_cast<RequestMapEntry *>(hdr);
    PRInt32 *delta = static_cast<PRInt32 *>(arg);

    RescheduleRequest(e->mKey, *delta);
    return PL_DHASH_NEXT;
}


nsLoadGroup::nsLoadGroup(nsISupports* outer)
    : mForegroundCount(0)
    , mLoadFlags(LOAD_NORMAL)
    , mStatus(NS_OK)
    , mPriority(PRIORITY_NORMAL)
    , mIsCanceling(PR_FALSE)
    , mDefaultLoadIsTimed(false)
    , mTimedRequests(0)
    , mCachedRequests(0)
{
    NS_INIT_AGGREGATED(outer);

#if defined(PR_LOGGING)
    
    if (nsnull == gLoadGroupLog)
        gLoadGroupLog = PR_NewLogModule("LoadGroup");
#endif

    LOG(("LOADGROUP [%x]: Created.\n", this));

    
    
    
    mRequests.ops = nsnull;
}

nsLoadGroup::~nsLoadGroup()
{
    nsresult rv;

    rv = Cancel(NS_BINDING_ABORTED);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Cancel failed");

    if (mRequests.ops) {
        PL_DHashTableFinish(&mRequests);
    }

    mDefaultLoadRequest = 0;

    LOG(("LOADGROUP [%x]: Destroyed.\n", this));
}


nsresult nsLoadGroup::Init()
{
    static PLDHashTableOps hash_table_ops =
    {
        PL_DHashAllocTable,
        PL_DHashFreeTable,
        PL_DHashVoidPtrKeyStub,
        RequestHashMatchEntry,
        PL_DHashMoveEntryStub,
        RequestHashClearEntry,
        PL_DHashFinalizeStub,
        RequestHashInitEntry
    };

    if (!PL_DHashTableInit(&mRequests, &hash_table_ops, nsnull,
                           sizeof(RequestMapEntry), 16)) {
        mRequests.ops = nsnull;

        return NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_OK;
}




NS_IMPL_AGGREGATED(nsLoadGroup)
NS_INTERFACE_MAP_BEGIN_AGGREGATED(nsLoadGroup)
    NS_INTERFACE_MAP_ENTRY(nsILoadGroup)
    NS_INTERFACE_MAP_ENTRY(nsIRequest)
    NS_INTERFACE_MAP_ENTRY(nsISupportsPriority)
    NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END




NS_IMETHODIMP
nsLoadGroup::GetName(nsACString &result)
{
    

    if (!mDefaultLoadRequest) {
        result.Truncate();
        return NS_OK;
    }
    
    return mDefaultLoadRequest->GetName(result);
}

NS_IMETHODIMP
nsLoadGroup::IsPending(PRBool *aResult)
{
    *aResult = (mForegroundCount > 0) ? PR_TRUE : PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::GetStatus(nsresult *status)
{
    if (NS_SUCCEEDED(mStatus) && mDefaultLoadRequest)
        return mDefaultLoadRequest->GetStatus(status);
    
    *status = mStatus;
    return NS_OK; 
}



static PLDHashOperator
AppendRequestsToArray(PLDHashTable *table, PLDHashEntryHdr *hdr,
                      PRUint32 number, void *arg)
{
    RequestMapEntry *e = static_cast<RequestMapEntry *>(hdr);
    nsTArray<nsIRequest*> *array = static_cast<nsTArray<nsIRequest*> *>(arg);

    nsIRequest *request = e->mKey;
    NS_ASSERTION(request, "What? Null key in pldhash entry?");

    PRBool ok = array->AppendElement(request) != nsnull;

    if (!ok) {
        return PL_DHASH_STOP;
    }

    NS_ADDREF(request);

    return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsLoadGroup::Cancel(nsresult status)
{
    NS_ASSERTION(NS_FAILED(status), "shouldn't cancel with a success code");
    nsresult rv;
    PRUint32 count = mRequests.entryCount;

    nsAutoTArray<nsIRequest*, 8> requests;

    PL_DHashTableEnumerate(&mRequests, AppendRequestsToArray,
                           static_cast<nsTArray<nsIRequest*> *>(&requests));

    if (requests.Length() != count) {
        for (PRUint32 i = 0, len = requests.Length(); i < len; ++i) {
            NS_RELEASE(requests[i]);
        }

        return NS_ERROR_OUT_OF_MEMORY;
    }

    
    
    
    mStatus = status;

    
    
    
    mIsCanceling = PR_TRUE;

    nsresult firstError = NS_OK;

    while (count > 0) {
        nsIRequest* request = requests.ElementAt(--count);

        NS_ASSERTION(request, "NULL request found in list.");

        RequestMapEntry *entry =
            static_cast<RequestMapEntry *>
                       (PL_DHashTableOperate(&mRequests, request,
                                                PL_DHASH_LOOKUP));

        if (PL_DHASH_ENTRY_IS_FREE(entry)) {
            

            NS_RELEASE(request);

            continue;
        }

#if defined(PR_LOGGING)
        nsCAutoString nameStr;
        request->GetName(nameStr);
        LOG(("LOADGROUP [%x]: Canceling request %x %s.\n",
             this, request, nameStr.get()));
#endif

        
        
        
        
        
        
        (void)RemoveRequest(request, nsnull, status);

        
        rv = request->Cancel(status);

        
        if (NS_FAILED(rv) && NS_SUCCEEDED(firstError))
            firstError = rv;

        NS_RELEASE(request);
    }

#if defined(DEBUG)
    NS_ASSERTION(mRequests.entryCount == 0, "Request list is not empty.");
    NS_ASSERTION(mForegroundCount == 0, "Foreground URLs are active.");
#endif

    mStatus = NS_OK;
    mIsCanceling = PR_FALSE;

    return firstError;
}


NS_IMETHODIMP
nsLoadGroup::Suspend()
{
    nsresult rv, firstError;
    PRUint32 count = mRequests.entryCount;

    nsAutoTArray<nsIRequest*, 8> requests;

    PL_DHashTableEnumerate(&mRequests, AppendRequestsToArray,
                           static_cast<nsTArray<nsIRequest*> *>(&requests));

    if (requests.Length() != count) {
        for (PRUint32 i = 0, len = requests.Length(); i < len; ++i) {
            NS_RELEASE(requests[i]);
        }

        return NS_ERROR_OUT_OF_MEMORY;
    }

    firstError = NS_OK;
    
    
    
    
    while (count > 0) {
        nsIRequest* request = requests.ElementAt(--count);

        NS_ASSERTION(request, "NULL request found in list.");
        if (!request)
            continue;

#if defined(PR_LOGGING)
        nsCAutoString nameStr;
        request->GetName(nameStr);
        LOG(("LOADGROUP [%x]: Suspending request %x %s.\n",
            this, request, nameStr.get()));
#endif

        
        rv = request->Suspend();

        
        if (NS_FAILED(rv) && NS_SUCCEEDED(firstError))
            firstError = rv;

        NS_RELEASE(request);
    }

    return firstError;
}


NS_IMETHODIMP
nsLoadGroup::Resume()
{
    nsresult rv, firstError;
    PRUint32 count = mRequests.entryCount;

    nsAutoTArray<nsIRequest*, 8> requests;

    PL_DHashTableEnumerate(&mRequests, AppendRequestsToArray,
                           static_cast<nsTArray<nsIRequest*> *>(&requests));

    if (requests.Length() != count) {
        for (PRUint32 i = 0, len = requests.Length(); i < len; ++i) {
            NS_RELEASE(requests[i]);
        }

        return NS_ERROR_OUT_OF_MEMORY;
    }

    firstError = NS_OK;
    
    
    
    
    while (count > 0) {
        nsIRequest* request = requests.ElementAt(--count);

        NS_ASSERTION(request, "NULL request found in list.");
        if (!request)
            continue;

#if defined(PR_LOGGING)
        nsCAutoString nameStr;
        request->GetName(nameStr);
        LOG(("LOADGROUP [%x]: Resuming request %x %s.\n",
            this, request, nameStr.get()));
#endif

        
        rv = request->Resume();

        
        if (NS_FAILED(rv) && NS_SUCCEEDED(firstError))
            firstError = rv;

        NS_RELEASE(request);
    }

    return firstError;
}

NS_IMETHODIMP
nsLoadGroup::GetLoadFlags(PRUint32 *aLoadFlags)
{
    *aLoadFlags = mLoadFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::SetLoadFlags(PRUint32 aLoadFlags)
{
    mLoadFlags = aLoadFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::GetLoadGroup(nsILoadGroup **loadGroup)
{
    *loadGroup = mLoadGroup;
    NS_IF_ADDREF(*loadGroup);
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::SetLoadGroup(nsILoadGroup *loadGroup)
{
    mLoadGroup = loadGroup;
    return NS_OK;
}




NS_IMETHODIMP
nsLoadGroup::GetDefaultLoadRequest(nsIRequest * *aRequest)
{
    *aRequest = mDefaultLoadRequest;
    NS_IF_ADDREF(*aRequest);
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::SetDefaultLoadRequest(nsIRequest *aRequest)
{
    mDefaultLoadRequest = aRequest;
    
    if (mDefaultLoadRequest) {
        mDefaultLoadRequest->GetLoadFlags(&mLoadFlags);
        
        
        
        
        mLoadFlags &= nsIRequest::LOAD_REQUESTMASK;

        nsCOMPtr<nsITimedChannel> timedChannel = do_QueryInterface(aRequest);
        mDefaultLoadIsTimed = timedChannel != nsnull;
        if (mDefaultLoadIsTimed) {
            timedChannel->GetChannelCreation(&mDefaultRequestCreationTime);
            timedChannel->SetTimingEnabled(PR_TRUE);
        }
    }
    
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::AddRequest(nsIRequest *request, nsISupports* ctxt)
{
    nsresult rv;

#if defined(PR_LOGGING)
    {
        nsCAutoString nameStr;
        request->GetName(nameStr);
        LOG(("LOADGROUP [%x]: Adding request %x %s (count=%d).\n",
             this, request, nameStr.get(), mRequests.entryCount));
    }
#endif 

#ifdef DEBUG
    {
      RequestMapEntry *entry =
          static_cast<RequestMapEntry *>
                     (PL_DHashTableOperate(&mRequests, request,
                                          PL_DHASH_LOOKUP));

      NS_ASSERTION(PL_DHASH_ENTRY_IS_FREE(entry),
                   "Entry added to loadgroup twice, don't do that");
    }
#endif

    
    
    
    if (mIsCanceling) {

#if defined(PR_LOGGING)
        LOG(("LOADGROUP [%x]: AddChannel() ABORTED because LoadGroup is"
             " being canceled!!\n", this));
#endif 

        return NS_BINDING_ABORTED;
    }

    nsLoadFlags flags;
    
    
    
    if (mDefaultLoadRequest == request || !mDefaultLoadRequest)
        rv = request->GetLoadFlags(&flags);
    else
        rv = MergeLoadFlags(request, flags);
    if (NS_FAILED(rv)) return rv;
    
    
    
    

    RequestMapEntry *entry =
        static_cast<RequestMapEntry *>
                   (PL_DHashTableOperate(&mRequests, request,
                                        PL_DHASH_ADD));

    if (!entry) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (mPriority != 0)
        RescheduleRequest(request, mPriority);

    nsCOMPtr<nsITimedChannel> timedChannel = do_QueryInterface(request);
    if (timedChannel)
        timedChannel->SetTimingEnabled(PR_TRUE);

    if (!(flags & nsIRequest::LOAD_BACKGROUND)) {
        
        mForegroundCount += 1;

        
        
        
        
        
        
        nsCOMPtr<nsIRequestObserver> observer = do_QueryReferent(mObserver);
        if (observer) {
            LOG(("LOADGROUP [%x]: Firing OnStartRequest for request %x."
                 "(foreground count=%d).\n", this, request, mForegroundCount));

            rv = observer->OnStartRequest(request, ctxt);
            if (NS_FAILED(rv)) {
                LOG(("LOADGROUP [%x]: OnStartRequest for request %x FAILED.\n",
                    this, request));
                
                
                
                

                PL_DHashTableOperate(&mRequests, request, PL_DHASH_REMOVE);

                rv = NS_OK;

                mForegroundCount -= 1;
            }
        }

        
        if (mForegroundCount == 1 && mLoadGroup) {
            mLoadGroup->AddRequest(this, nsnull);
        }

    }

    return rv;
}

NS_IMETHODIMP
nsLoadGroup::RemoveRequest(nsIRequest *request, nsISupports* ctxt,
                           nsresult aStatus)
{
    NS_ENSURE_ARG_POINTER(request);
    nsresult rv;

#if defined(PR_LOGGING)
    {
        nsCAutoString nameStr;
        request->GetName(nameStr);
        LOG(("LOADGROUP [%x]: Removing request %x %s status %x (count=%d).\n",
            this, request, nameStr.get(), aStatus, mRequests.entryCount-1));
    }
#endif

    
    

    nsCOMPtr<nsIRequest> kungFuDeathGrip(request);

    
    
    
    
    
    RequestMapEntry *entry =
        static_cast<RequestMapEntry *>
                   (PL_DHashTableOperate(&mRequests, request,
                                        PL_DHASH_LOOKUP));

    if (PL_DHASH_ENTRY_IS_FREE(entry)) {
        LOG(("LOADGROUP [%x]: Unable to remove request %x. Not in group!\n",
            this, request));

        return NS_ERROR_FAILURE;
    }

    PL_DHashTableRawRemove(&mRequests, entry);

    
    
    if (mDefaultLoadIsTimed && NS_SUCCEEDED(aStatus)) {
        nsCOMPtr<nsITimedChannel> timedChannel = do_QueryInterface(request);
        if (timedChannel) {
            
            ++mTimedRequests;
            TimeStamp timeStamp;
            rv = timedChannel->GetCacheReadStart(&timeStamp);
            if (NS_SUCCEEDED(rv) && !timeStamp.IsNull())
                ++mCachedRequests;

            rv = timedChannel->GetAsyncOpen(&timeStamp);
            if (NS_SUCCEEDED(rv) && !timeStamp.IsNull()) {
                Telemetry::Accumulate(
                    Telemetry::HTTP_SUBITEM_OPEN_LATENCY_TIME,
                    HISTOGRAM_TIME_DELTA(mDefaultRequestCreationTime, timeStamp));
            }

            rv = timedChannel->GetResponseStart(&timeStamp);
            if (NS_SUCCEEDED(rv) && !timeStamp.IsNull()) {
                Telemetry::Accumulate(
                    Telemetry::HTTP_SUBITEM_FIRST_BYTE_LATENCY_TIME,
                    HISTOGRAM_TIME_DELTA(mDefaultRequestCreationTime, timeStamp));
            }

            TelemetryReportChannel(timedChannel, false);
        }
    }

    if (mRequests.entryCount == 0) {
        TelemetryReport();
    }

    
    if (mPriority != 0)
        RescheduleRequest(request, -mPriority);

    nsLoadFlags flags;
    rv = request->GetLoadFlags(&flags);
    if (NS_FAILED(rv)) return rv;

    if (!(flags & nsIRequest::LOAD_BACKGROUND)) {
        NS_ASSERTION(mForegroundCount > 0, "ForegroundCount messed up");
        mForegroundCount -= 1;

        
        nsCOMPtr<nsIRequestObserver> observer = do_QueryReferent(mObserver);
        if (observer) {
            LOG(("LOADGROUP [%x]: Firing OnStopRequest for request %x."
                 "(foreground count=%d).\n", this, request, mForegroundCount));

            rv = observer->OnStopRequest(request, ctxt, aStatus);

#if defined(PR_LOGGING)
            if (NS_FAILED(rv)) {
                LOG(("LOADGROUP [%x]: OnStopRequest for request %x FAILED.\n",
                    this, request));
            }
#endif
        }

        
        if (mForegroundCount == 0 && mLoadGroup) {
            mLoadGroup->RemoveRequest(this, nsnull, aStatus);
        }
    }

    return rv;
}



static PLDHashOperator
AppendRequestsToISupportsArray(PLDHashTable *table, PLDHashEntryHdr *hdr,
                               PRUint32 number, void *arg)
{
    RequestMapEntry *e = static_cast<RequestMapEntry *>(hdr);
    nsISupportsArray *array = static_cast<nsISupportsArray *>(arg);

    PRBool ok = array->AppendElement(e->mKey);

    if (!ok) {
        return PL_DHASH_STOP;
    }

    return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsLoadGroup::GetRequests(nsISimpleEnumerator * *aRequests)
{
    nsCOMPtr<nsISupportsArray> array;
    nsresult rv = NS_NewISupportsArray(getter_AddRefs(array));
    NS_ENSURE_SUCCESS(rv, rv);

    PL_DHashTableEnumerate(&mRequests, AppendRequestsToISupportsArray,
                           array.get());

    PRUint32 count;
    array->Count(&count);

    if (count != mRequests.entryCount) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_NewArrayEnumerator(aRequests, array);
}

NS_IMETHODIMP
nsLoadGroup::SetGroupObserver(nsIRequestObserver* aObserver)
{
    mObserver = do_GetWeakReference(aObserver);
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::GetGroupObserver(nsIRequestObserver* *aResult)
{
    nsCOMPtr<nsIRequestObserver> observer = do_QueryReferent(mObserver);
    *aResult = observer;
    NS_IF_ADDREF(*aResult);
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::GetActiveCount(PRUint32* aResult)
{
    *aResult = mForegroundCount;
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::GetNotificationCallbacks(nsIInterfaceRequestor **aCallbacks)
{
    NS_ENSURE_ARG_POINTER(aCallbacks);
    *aCallbacks = mCallbacks;
    NS_IF_ADDREF(*aCallbacks);
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::SetNotificationCallbacks(nsIInterfaceRequestor *aCallbacks)
{
    mCallbacks = aCallbacks;
    return NS_OK;
}




NS_IMETHODIMP
nsLoadGroup::GetPriority(PRInt32 *aValue)
{
    *aValue = mPriority;
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::SetPriority(PRInt32 aValue)
{
    return AdjustPriority(aValue - mPriority);
}

NS_IMETHODIMP
nsLoadGroup::AdjustPriority(PRInt32 aDelta)
{
    
    if (aDelta != 0) {
        mPriority += aDelta;
        PL_DHashTableEnumerate(&mRequests, RescheduleRequests, &aDelta);
    }
    return NS_OK;
}



void 
nsLoadGroup::TelemetryReport()
{
    if (mDefaultLoadIsTimed) {
        Telemetry::Accumulate(Telemetry::HTTP_REQUEST_PER_PAGE, mTimedRequests);
        if (mTimedRequests) {
            Telemetry::Accumulate(Telemetry::HTTP_REQUEST_PER_PAGE_FROM_CACHE,
                                  mCachedRequests * 100 / mTimedRequests);
        }

        nsCOMPtr<nsITimedChannel> timedChannel =
            do_QueryInterface(mDefaultLoadRequest);
        if (timedChannel)
            TelemetryReportChannel(timedChannel, true);
    }

    mTimedRequests = 0;
    mCachedRequests = 0;
    mDefaultLoadIsTimed = false;
}

void
nsLoadGroup::TelemetryReportChannel(nsITimedChannel *aTimedChannel,
                                    bool aDefaultRequest)
{
    nsresult rv;
    PRBool timingEnabled;
    rv = aTimedChannel->GetTimingEnabled(&timingEnabled);
    if (NS_FAILED(rv) || !timingEnabled)
        return;

    TimeStamp channelCreation;
    rv = aTimedChannel->GetChannelCreation(&channelCreation);
    if (NS_FAILED(rv))
        return;

    TimeStamp asyncOpen;
    rv = aTimedChannel->GetAsyncOpen(&asyncOpen);
    
    if (NS_FAILED(rv) || asyncOpen.IsNull())
        return;

    TimeStamp cacheReadStart;
    rv = aTimedChannel->GetCacheReadStart(&cacheReadStart);
    if (NS_FAILED(rv))
        return;

    TimeStamp cacheReadEnd;
    rv = aTimedChannel->GetCacheReadEnd(&cacheReadEnd);
    if (NS_FAILED(rv))
        return;

    TimeStamp domainLookupStart;
    rv = aTimedChannel->GetDomainLookupStart(&domainLookupStart);
    if (NS_FAILED(rv))
        return;

    TimeStamp domainLookupEnd;
    rv = aTimedChannel->GetDomainLookupEnd(&domainLookupEnd);
    if (NS_FAILED(rv))
        return;

    TimeStamp connectStart;
    rv = aTimedChannel->GetConnectStart(&connectStart);
    if (NS_FAILED(rv))
        return;

    TimeStamp connectEnd;
    rv = aTimedChannel->GetConnectEnd(&connectEnd);
    if (NS_FAILED(rv))
        return;

    TimeStamp requestStart;
    rv = aTimedChannel->GetRequestStart(&requestStart);
    if (NS_FAILED(rv))
        return;

    TimeStamp responseStart;
    rv = aTimedChannel->GetResponseStart(&responseStart);
    if (NS_FAILED(rv))
        return;

    TimeStamp responseEnd;
    rv = aTimedChannel->GetResponseEnd(&responseEnd);
    if (NS_FAILED(rv))
        return;

#define HTTP_REQUEST_HISTOGRAMS(prefix)                                        \
    if (!domainLookupStart.IsNull()) {                                         \
        Telemetry::Accumulate(                                                 \
            Telemetry::HTTP_##prefix##_DNS_ISSUE_TIME,                         \
            HISTOGRAM_TIME_DELTA(asyncOpen, domainLookupStart));               \
    }                                                                          \
                                                                               \
    if (!domainLookupStart.IsNull() && !domainLookupEnd.IsNull()) {            \
        Telemetry::Accumulate(                                                 \
            Telemetry::HTTP_##prefix##_DNS_LOOKUP_TIME,                        \
            HISTOGRAM_TIME_DELTA(domainLookupStart, domainLookupEnd));         \
    }                                                                          \
                                                                               \
    if (!connectStart.IsNull() && !connectEnd.IsNull()) {                      \
        Telemetry::Accumulate(                                                 \
            Telemetry::HTTP_##prefix##_TCP_CONNECTION,                         \
            HISTOGRAM_TIME_DELTA(connectStart, connectEnd));                   \
    }                                                                          \
                                                                               \
                                                                               \
    if (!requestStart.IsNull() && !responseEnd.IsNull()) {                     \
        Telemetry::Accumulate(                                                 \
            Telemetry::HTTP_##prefix##_OPEN_TO_FIRST_SENT,                     \
            HISTOGRAM_TIME_DELTA(asyncOpen, requestStart));                    \
                                                                               \
        Telemetry::Accumulate(                                                 \
            Telemetry::HTTP_##prefix##_FIRST_SENT_TO_LAST_RECEIVED,            \
            HISTOGRAM_TIME_DELTA(requestStart, responseEnd));                  \
                                                                               \
        if (cacheReadStart.IsNull()) {                                         \
            Telemetry::Accumulate(                                             \
                Telemetry::HTTP_##prefix##_OPEN_TO_FIRST_RECEIVED,             \
                HISTOGRAM_TIME_DELTA(asyncOpen, responseStart));               \
        }                                                                      \
    }                                                                          \
                                                                               \
    if (!cacheReadStart.IsNull() && !cacheReadEnd.IsNull()) {                  \
        Telemetry::Accumulate(                                                 \
            Telemetry::HTTP_##prefix##_OPEN_TO_FIRST_FROM_CACHE,               \
            HISTOGRAM_TIME_DELTA(asyncOpen, cacheReadStart));                  \
                                                                               \
        Telemetry::Accumulate(                                                 \
            Telemetry::HTTP_##prefix##_CACHE_READ_TIME,                        \
            HISTOGRAM_TIME_DELTA(cacheReadStart, cacheReadEnd));               \
                                                                               \
        if (!requestStart.IsNull() && !responseEnd.IsNull()) {                 \
            Telemetry::Accumulate(                                             \
                Telemetry::HTTP_##prefix##_REVALIDATION,                       \
                HISTOGRAM_TIME_DELTA(requestStart, responseEnd));              \
        }                                                                      \
    }                                                                          \
                                                                               \
    if (!cacheReadEnd.IsNull()) {                                              \
        Telemetry::Accumulate(                                                 \
            Telemetry::HTTP_##prefix##_COMPLETE_LOAD,                          \
            HISTOGRAM_TIME_DELTA(asyncOpen, cacheReadEnd));                    \
        Telemetry::Accumulate(                                                 \
            Telemetry::HTTP_##prefix##_COMPLETE_LOAD_CACHED,                   \
            HISTOGRAM_TIME_DELTA(asyncOpen, cacheReadEnd));                    \
    }                                                                          \
    else if (!responseEnd.IsNull()) {                                          \
        Telemetry::Accumulate(                                                 \
            Telemetry::HTTP_##prefix##_COMPLETE_LOAD,                          \
            HISTOGRAM_TIME_DELTA(asyncOpen, responseEnd));                     \
        Telemetry::Accumulate(                                                 \
            Telemetry::HTTP_##prefix##_COMPLETE_LOAD_NET,                      \
            HISTOGRAM_TIME_DELTA(asyncOpen, responseEnd));                     \
    }

    if (aDefaultRequest) {
        HTTP_REQUEST_HISTOGRAMS(PAGE)
    } else {
        HTTP_REQUEST_HISTOGRAMS(SUB)
    }
#undef HTTP_REQUEST_HISTOGRAMS
}

nsresult nsLoadGroup::MergeLoadFlags(nsIRequest *aRequest, nsLoadFlags& outFlags)
{
    nsresult rv;
    nsLoadFlags flags, oldFlags;

    rv = aRequest->GetLoadFlags(&flags);
    if (NS_FAILED(rv)) 
        return rv;

    oldFlags = flags;

    
    flags |= (mLoadFlags & (LOAD_BACKGROUND |
                            LOAD_BYPASS_CACHE |
                            LOAD_FROM_CACHE |
                            VALIDATE_ALWAYS |
                            VALIDATE_ONCE_PER_SESSION |
                            VALIDATE_NEVER));

    if (flags != oldFlags)
        rv = aRequest->SetLoadFlags(flags);

    outFlags = flags;
    return rv;
}
