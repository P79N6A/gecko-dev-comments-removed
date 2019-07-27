





#include "mozilla/DebugOnly.h"

#include "nsLoadGroup.h"

#include "nsArrayEnumerator.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "mozilla/Logging.h"
#include "nsString.h"
#include "nsTArray.h"
#include "mozilla/Atomics.h"
#include "mozilla/Telemetry.h"
#include "nsAutoPtr.h"
#include "mozilla/net/PSpdyPush.h"
#include "nsITimedChannel.h"
#include "nsIInterfaceRequestor.h"
#include "nsIRequestObserver.h"
#include "CacheObserver.h"
#include "MainThreadUtils.h"

using namespace mozilla;
using namespace mozilla::net;












static PRLogModuleInfo* gLoadGroupLog = nullptr;

#undef LOG
#define LOG(args) MOZ_LOG(gLoadGroupLog, mozilla::LogLevel::Debug, args)




class nsLoadGroupConnectionInfo final : public nsILoadGroupConnectionInfo
{
    ~nsLoadGroupConnectionInfo() {}

public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSILOADGROUPCONNECTIONINFO

    nsLoadGroupConnectionInfo();
private:
    Atomic<uint32_t>       mBlockingTransactionCount;
    nsAutoPtr<mozilla::net::SpdyPushCache> mSpdyCache;
};

NS_IMPL_ISUPPORTS(nsLoadGroupConnectionInfo, nsILoadGroupConnectionInfo)

nsLoadGroupConnectionInfo::nsLoadGroupConnectionInfo()
    : mBlockingTransactionCount(0)
{
}

NS_IMETHODIMP
nsLoadGroupConnectionInfo::GetBlockingTransactionCount(uint32_t *aBlockingTransactionCount)
{
    NS_ENSURE_ARG_POINTER(aBlockingTransactionCount);
    *aBlockingTransactionCount = mBlockingTransactionCount;
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroupConnectionInfo::AddBlockingTransaction()
{
    mBlockingTransactionCount++;
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroupConnectionInfo::RemoveBlockingTransaction(uint32_t *_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
        mBlockingTransactionCount--;
        *_retval = mBlockingTransactionCount;
    return NS_OK;
}


NS_IMETHODIMP
nsLoadGroupConnectionInfo::GetSpdyPushCache(mozilla::net::SpdyPushCache **aSpdyPushCache)
{
    *aSpdyPushCache = mSpdyCache.get();
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroupConnectionInfo::SetSpdyPushCache(mozilla::net::SpdyPushCache *aSpdyPushCache)
{
    mSpdyCache = aSpdyPushCache;
    return NS_OK;
}



class RequestMapEntry : public PLDHashEntryHdr
{
public:
    explicit RequestMapEntry(nsIRequest *aRequest) :
        mKey(aRequest)
    {
    }

    nsCOMPtr<nsIRequest> mKey;
};

static bool
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

static void
RequestHashInitEntry(PLDHashEntryHdr *entry, const void *key)
{
    const nsIRequest *const_request = static_cast<const nsIRequest *>(key);
    nsIRequest *request = const_cast<nsIRequest *>(const_request);

    
    new (entry) RequestMapEntry(request);
}

static const PLDHashTableOps sRequestHashOps =
{
    PL_DHashVoidPtrKeyStub,
    RequestHashMatchEntry,
    PL_DHashMoveEntryStub,
    RequestHashClearEntry,
    RequestHashInitEntry
};

static void
RescheduleRequest(nsIRequest *aRequest, int32_t delta)
{
    nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(aRequest);
    if (p)
        p->AdjustPriority(delta);
}

nsLoadGroup::nsLoadGroup(nsISupports* outer)
    : mForegroundCount(0)
    , mLoadFlags(LOAD_NORMAL)
    , mDefaultLoadFlags(0)
    , mConnectionInfo(new nsLoadGroupConnectionInfo())
    , mRequests(&sRequestHashOps, sizeof(RequestMapEntry))
    , mStatus(NS_OK)
    , mPriority(PRIORITY_NORMAL)
    , mIsCanceling(false)
    , mDefaultLoadIsTimed(false)
    , mTimedRequests(0)
    , mCachedRequests(0)
    , mTimedNonCachedRequestsUntilOnEndPageLoad(0)
{
    NS_INIT_AGGREGATED(outer);

    
    if (nullptr == gLoadGroupLog)
        gLoadGroupLog = PR_NewLogModule("LoadGroup");

    LOG(("LOADGROUP [%x]: Created.\n", this));
}

nsLoadGroup::~nsLoadGroup()
{
    DebugOnly<nsresult> rv = Cancel(NS_BINDING_ABORTED);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Cancel failed");

    mDefaultLoadRequest = 0;

    LOG(("LOADGROUP [%x]: Destroyed.\n", this));
}





NS_IMPL_AGGREGATED(nsLoadGroup)
NS_INTERFACE_MAP_BEGIN_AGGREGATED(nsLoadGroup)
    NS_INTERFACE_MAP_ENTRY(nsILoadGroup)
    NS_INTERFACE_MAP_ENTRY(nsPILoadGroupInternal)
    NS_INTERFACE_MAP_ENTRY(nsILoadGroupChild)
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
nsLoadGroup::IsPending(bool *aResult)
{
    *aResult = (mForegroundCount > 0) ? true : false;
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

static bool
AppendRequestsToArray(PLDHashTable* aTable, nsTArray<nsIRequest*> *aArray)
{
    for (auto iter = aTable->Iter(); !iter.Done(); iter.Next()) {
        auto e = static_cast<RequestMapEntry*>(iter.Get());
        nsIRequest *request = e->mKey;
        NS_ASSERTION(request, "What? Null key in pldhash entry?");

        bool ok = !!aArray->AppendElement(request);
        if (!ok) {
           break;
        }
        NS_ADDREF(request);
    }

    if (aArray->Length() != aTable->EntryCount()) {
        for (uint32_t i = 0, len = aArray->Length(); i < len; ++i) {
            NS_RELEASE((*aArray)[i]);
        }
        return false;
    }
    return true;
}

NS_IMETHODIMP
nsLoadGroup::Cancel(nsresult status)
{
    MOZ_ASSERT(NS_IsMainThread());

    NS_ASSERTION(NS_FAILED(status), "shouldn't cancel with a success code");
    nsresult rv;
    uint32_t count = mRequests.EntryCount();

    nsAutoTArray<nsIRequest*, 8> requests;

    if (!AppendRequestsToArray(&mRequests, &requests)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    
    
    
    mStatus = status;

    
    
    
    mIsCanceling = true;

    nsresult firstError = NS_OK;

    while (count > 0) {
        nsIRequest* request = requests.ElementAt(--count);

        NS_ASSERTION(request, "NULL request found in list.");

        if (!PL_DHashTableSearch(&mRequests, request)) {
            
            NS_RELEASE(request);
            continue;
        }

        if (MOZ_LOG_TEST(gLoadGroupLog, LogLevel::Debug)) {
            nsAutoCString nameStr;
            request->GetName(nameStr);
            LOG(("LOADGROUP [%x]: Canceling request %x %s.\n",
                 this, request, nameStr.get()));
        }

        
        
        
        
        
        
        (void)RemoveRequest(request, nullptr, status);

        
        rv = request->Cancel(status);

        
        if (NS_FAILED(rv) && NS_SUCCEEDED(firstError))
            firstError = rv;

        NS_RELEASE(request);
    }

#if defined(DEBUG)
    NS_ASSERTION(mRequests.EntryCount() == 0, "Request list is not empty.");
    NS_ASSERTION(mForegroundCount == 0, "Foreground URLs are active.");
#endif

    mStatus = NS_OK;
    mIsCanceling = false;

    return firstError;
}


NS_IMETHODIMP
nsLoadGroup::Suspend()
{
    nsresult rv, firstError;
    uint32_t count = mRequests.EntryCount();

    nsAutoTArray<nsIRequest*, 8> requests;

    if (!AppendRequestsToArray(&mRequests, &requests)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    firstError = NS_OK;
    
    
    
    
    while (count > 0) {
        nsIRequest* request = requests.ElementAt(--count);

        NS_ASSERTION(request, "NULL request found in list.");
        if (!request)
            continue;

        if (MOZ_LOG_TEST(gLoadGroupLog, LogLevel::Debug)) {
            nsAutoCString nameStr;
            request->GetName(nameStr);
            LOG(("LOADGROUP [%x]: Suspending request %x %s.\n",
                this, request, nameStr.get()));
        }

        
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
    uint32_t count = mRequests.EntryCount();

    nsAutoTArray<nsIRequest*, 8> requests;

    if (!AppendRequestsToArray(&mRequests, &requests)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    firstError = NS_OK;
    
    
    
    
    while (count > 0) {
        nsIRequest* request = requests.ElementAt(--count);

        NS_ASSERTION(request, "NULL request found in list.");
        if (!request)
            continue;

        if (MOZ_LOG_TEST(gLoadGroupLog, LogLevel::Debug)) {
            nsAutoCString nameStr;
            request->GetName(nameStr);
            LOG(("LOADGROUP [%x]: Resuming request %x %s.\n",
                this, request, nameStr.get()));
        }

        
        rv = request->Resume();

        
        if (NS_FAILED(rv) && NS_SUCCEEDED(firstError))
            firstError = rv;

        NS_RELEASE(request);
    }

    return firstError;
}

NS_IMETHODIMP
nsLoadGroup::GetLoadFlags(uint32_t *aLoadFlags)
{
    *aLoadFlags = mLoadFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::SetLoadFlags(uint32_t aLoadFlags)
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
        mDefaultLoadIsTimed = timedChannel != nullptr;
        if (mDefaultLoadIsTimed) {
            timedChannel->GetChannelCreation(&mDefaultRequestCreationTime);
            timedChannel->SetTimingEnabled(true);
        }
    }
    
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::AddRequest(nsIRequest *request, nsISupports* ctxt)
{
    nsresult rv;

    if (MOZ_LOG_TEST(gLoadGroupLog, LogLevel::Debug)) {
        nsAutoCString nameStr;
        request->GetName(nameStr);
        LOG(("LOADGROUP [%x]: Adding request %x %s (count=%d).\n",
             this, request, nameStr.get(), mRequests.EntryCount()));
    }

    NS_ASSERTION(!PL_DHashTableSearch(&mRequests, request),
                 "Entry added to loadgroup twice, don't do that");

    
    
    
    if (mIsCanceling) {
        LOG(("LOADGROUP [%x]: AddChannel() ABORTED because LoadGroup is"
             " being canceled!!\n", this));

        return NS_BINDING_ABORTED;
    }

    nsLoadFlags flags;
    
    
    
    if (mDefaultLoadRequest == request || !mDefaultLoadRequest)
        rv = request->GetLoadFlags(&flags);
    else
        rv = MergeLoadFlags(request, flags);
    if (NS_FAILED(rv)) return rv;
    
    
    
    

    RequestMapEntry *entry = static_cast<RequestMapEntry *>
        (PL_DHashTableAdd(&mRequests, request, fallible));

    if (!entry) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (mPriority != 0)
        RescheduleRequest(request, mPriority);

    nsCOMPtr<nsITimedChannel> timedChannel = do_QueryInterface(request);
    if (timedChannel)
        timedChannel->SetTimingEnabled(true);

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
                
                
                
                

                PL_DHashTableRemove(&mRequests, request);

                rv = NS_OK;

                mForegroundCount -= 1;
            }
        }

        
        if (mForegroundCount == 1 && mLoadGroup) {
            mLoadGroup->AddRequest(this, nullptr);
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

    if (MOZ_LOG_TEST(gLoadGroupLog, LogLevel::Debug)) {
        nsAutoCString nameStr;
        request->GetName(nameStr);
        LOG(("LOADGROUP [%x]: Removing request %x %s status %x (count=%d).\n",
            this, request, nameStr.get(), aStatus, mRequests.EntryCount() - 1));
    }

    
    

    nsCOMPtr<nsIRequest> kungFuDeathGrip(request);

    
    
    
    
    
    RequestMapEntry *entry =
        static_cast<RequestMapEntry *>
                   (PL_DHashTableSearch(&mRequests, request));

    if (!entry) {
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
            if (NS_SUCCEEDED(rv) && !timeStamp.IsNull()) {
                ++mCachedRequests;
            }
            else {
                mTimedNonCachedRequestsUntilOnEndPageLoad++;
            }

            rv = timedChannel->GetAsyncOpen(&timeStamp);
            if (NS_SUCCEEDED(rv) && !timeStamp.IsNull()) {
                Telemetry::AccumulateTimeDelta(
                    Telemetry::HTTP_SUBITEM_OPEN_LATENCY_TIME,
                    mDefaultRequestCreationTime, timeStamp);
            }

            rv = timedChannel->GetResponseStart(&timeStamp);
            if (NS_SUCCEEDED(rv) && !timeStamp.IsNull()) {
                Telemetry::AccumulateTimeDelta(
                    Telemetry::HTTP_SUBITEM_FIRST_BYTE_LATENCY_TIME,
                    mDefaultRequestCreationTime, timeStamp);
            }

            TelemetryReportChannel(timedChannel, false);
        }
    }

    if (mRequests.EntryCount() == 0) {
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

            if (NS_FAILED(rv)) {
                LOG(("LOADGROUP [%x]: OnStopRequest for request %x FAILED.\n",
                    this, request));
            }
        }

        
        if (mForegroundCount == 0 && mLoadGroup) {
            mLoadGroup->RemoveRequest(this, nullptr, aStatus);
        }
    }

    return rv;
}

NS_IMETHODIMP
nsLoadGroup::GetRequests(nsISimpleEnumerator * *aRequests)
{
    nsCOMArray<nsIRequest> requests;
    requests.SetCapacity(mRequests.EntryCount());

    for (auto iter = mRequests.Iter(); !iter.Done(); iter.Next()) {
      auto e = static_cast<RequestMapEntry*>(iter.Get());
      requests.AppendObject(e->mKey);
    }

    return NS_NewArrayEnumerator(aRequests, requests);
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
nsLoadGroup::GetActiveCount(uint32_t* aResult)
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
nsLoadGroup::GetConnectionInfo(nsILoadGroupConnectionInfo **aCI)
{
    NS_ENSURE_ARG_POINTER(aCI);
    *aCI = mConnectionInfo;
    NS_IF_ADDREF(*aCI);
    return NS_OK;
}





NS_IMETHODIMP
nsLoadGroup::GetParentLoadGroup(nsILoadGroup * *aParentLoadGroup)
{
    *aParentLoadGroup = nullptr;
    nsCOMPtr<nsILoadGroup> parent = do_QueryReferent(mParentLoadGroup);
    if (!parent)
        return NS_OK;
    parent.forget(aParentLoadGroup);
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::SetParentLoadGroup(nsILoadGroup *aParentLoadGroup)
{
    mParentLoadGroup = do_GetWeakReference(aParentLoadGroup);
    return NS_OK;
}


NS_IMETHODIMP
nsLoadGroup::GetChildLoadGroup(nsILoadGroup * *aChildLoadGroup)
{
    NS_ADDREF(*aChildLoadGroup = this);
    return NS_OK;
}


NS_IMETHODIMP
nsLoadGroup::GetRootLoadGroup(nsILoadGroup * *aRootLoadGroup)
{
    
    nsCOMPtr<nsILoadGroupChild> ancestor = do_QueryReferent(mParentLoadGroup);
    if (ancestor)
        return ancestor->GetRootLoadGroup(aRootLoadGroup);

    
    ancestor = do_QueryInterface(mLoadGroup);
    if (ancestor)
        return ancestor->GetRootLoadGroup(aRootLoadGroup);

    
    NS_ADDREF(*aRootLoadGroup = this);
    return NS_OK;
}




NS_IMETHODIMP
nsLoadGroup::OnEndPageLoad(nsIChannel *aDefaultChannel)
{
    
    return NS_OK;
}




NS_IMETHODIMP
nsLoadGroup::GetPriority(int32_t *aValue)
{
    *aValue = mPriority;
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::SetPriority(int32_t aValue)
{
    return AdjustPriority(aValue - mPriority);
}

NS_IMETHODIMP
nsLoadGroup::AdjustPriority(int32_t aDelta)
{
    
    if (aDelta != 0) {
        mPriority += aDelta;
        for (auto iter = mRequests.Iter(); !iter.Done(); iter.Next()) {
          auto e = static_cast<RequestMapEntry*>(iter.Get());
          RescheduleRequest(e->mKey, aDelta);
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::GetDefaultLoadFlags(uint32_t *aFlags)
{
    *aFlags = mDefaultLoadFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsLoadGroup::SetDefaultLoadFlags(uint32_t aFlags)
{
    mDefaultLoadFlags = aFlags;
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
    bool timingEnabled;
    rv = aTimedChannel->GetTimingEnabled(&timingEnabled);
    if (NS_FAILED(rv) || !timingEnabled)
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
        Telemetry::AccumulateTimeDelta(                                        \
            Telemetry::HTTP_##prefix##_DNS_ISSUE_TIME,                         \
            asyncOpen, domainLookupStart);                                     \
    }                                                                          \
                                                                               \
    if (!domainLookupStart.IsNull() && !domainLookupEnd.IsNull()) {            \
        Telemetry::AccumulateTimeDelta(                                        \
            Telemetry::HTTP_##prefix##_DNS_LOOKUP_TIME,                        \
            domainLookupStart, domainLookupEnd);                               \
    }                                                                          \
                                                                               \
    if (!connectStart.IsNull() && !connectEnd.IsNull()) {                      \
        Telemetry::AccumulateTimeDelta(                                        \
            Telemetry::HTTP_##prefix##_TCP_CONNECTION,                         \
            connectStart, connectEnd);                                         \
    }                                                                          \
                                                                               \
                                                                               \
    if (!requestStart.IsNull() && !responseEnd.IsNull()) {                     \
        Telemetry::AccumulateTimeDelta(                                        \
            Telemetry::HTTP_##prefix##_OPEN_TO_FIRST_SENT,                     \
            asyncOpen, requestStart);                                          \
                                                                               \
        Telemetry::AccumulateTimeDelta(                                        \
            Telemetry::HTTP_##prefix##_FIRST_SENT_TO_LAST_RECEIVED,            \
            requestStart, responseEnd);                                        \
                                                                               \
        if (cacheReadStart.IsNull() && !responseStart.IsNull()) {              \
            Telemetry::AccumulateTimeDelta(                                    \
                Telemetry::HTTP_##prefix##_OPEN_TO_FIRST_RECEIVED,             \
                asyncOpen, responseStart);                                     \
        }                                                                      \
    }                                                                          \
                                                                               \
    if (!cacheReadStart.IsNull() && !cacheReadEnd.IsNull()) {                  \
        if (!CacheObserver::UseNewCache()) {                                   \
            Telemetry::AccumulateTimeDelta(                                    \
                Telemetry::HTTP_##prefix##_OPEN_TO_FIRST_FROM_CACHE,           \
                asyncOpen, cacheReadStart);                                    \
        } else {                                                               \
            Telemetry::AccumulateTimeDelta(                                    \
                Telemetry::HTTP_##prefix##_OPEN_TO_FIRST_FROM_CACHE_V2,        \
                asyncOpen, cacheReadStart);                                    \
        }                                                                      \
                                                                               \
        if (!CacheObserver::UseNewCache()) {                                   \
            Telemetry::AccumulateTimeDelta(                                    \
                Telemetry::HTTP_##prefix##_CACHE_READ_TIME,                    \
                cacheReadStart, cacheReadEnd);                                 \
        } else {                                                               \
            Telemetry::AccumulateTimeDelta(                                    \
                Telemetry::HTTP_##prefix##_CACHE_READ_TIME_V2,                 \
                cacheReadStart, cacheReadEnd);                                 \
        }                                                                      \
                                                                               \
        if (!requestStart.IsNull() && !responseEnd.IsNull()) {                 \
            Telemetry::AccumulateTimeDelta(                                    \
                Telemetry::HTTP_##prefix##_REVALIDATION,                       \
                requestStart, responseEnd);                                    \
        }                                                                      \
    }                                                                          \
                                                                               \
    if (!cacheReadEnd.IsNull()) {                                              \
        Telemetry::AccumulateTimeDelta(                                        \
            Telemetry::HTTP_##prefix##_COMPLETE_LOAD,                          \
            asyncOpen, cacheReadEnd);                                          \
                                                                               \
        if (!CacheObserver::UseNewCache()) {                                   \
            Telemetry::AccumulateTimeDelta(                                    \
                Telemetry::HTTP_##prefix##_COMPLETE_LOAD_CACHED,               \
                asyncOpen, cacheReadEnd);                                      \
        } else {                                                               \
            Telemetry::AccumulateTimeDelta(                                    \
                Telemetry::HTTP_##prefix##_COMPLETE_LOAD_CACHED_V2,            \
                asyncOpen, cacheReadEnd);                                      \
        }                                                                      \
    }                                                                          \
    else if (!responseEnd.IsNull()) {                                          \
        if (!CacheObserver::UseNewCache()) {                                   \
            Telemetry::AccumulateTimeDelta(                                    \
                Telemetry::HTTP_##prefix##_COMPLETE_LOAD,                      \
                asyncOpen, responseEnd);                                       \
            Telemetry::AccumulateTimeDelta(                                    \
                Telemetry::HTTP_##prefix##_COMPLETE_LOAD_NET,                  \
                asyncOpen, responseEnd);                                       \
        } else {                                                               \
            Telemetry::AccumulateTimeDelta(                                    \
                Telemetry::HTTP_##prefix##_COMPLETE_LOAD_V2,                   \
                asyncOpen, responseEnd);                                       \
            Telemetry::AccumulateTimeDelta(                                    \
                Telemetry::HTTP_##prefix##_COMPLETE_LOAD_NET_V2,               \
                asyncOpen, responseEnd);                                       \
        }                                                                      \
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

    
    flags |= mDefaultLoadFlags;

    if (flags != oldFlags)
        rv = aRequest->SetLoadFlags(flags);

    outFlags = flags;
    return rv;
}

#undef LOG
