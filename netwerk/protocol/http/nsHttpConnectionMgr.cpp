





#include "HttpLog.h"


#undef LOG
#define LOG(args) LOG5(args)
#undef LOG_ENABLED
#define LOG_ENABLED() LOG5_ENABLED()

#include "nsHttpConnectionMgr.h"
#include "nsHttpConnection.h"
#include "nsHttpPipeline.h"
#include "nsHttpHandler.h"
#include "nsIHttpChannelInternal.h"
#include "nsNetCID.h"
#include "nsCOMPtr.h"
#include "nsNetUtil.h"
#include "mozilla/net/DNS.h"
#include "nsISocketTransport.h"
#include "nsISSLSocketControl.h"
#include "mozilla/Telemetry.h"
#include "mozilla/net/DashboardTypes.h"
#include "NullHttpTransaction.h"
#include "nsIDNSRecord.h"
#include "nsITransport.h"
#include "nsISocketTransportService.h"
#include <algorithm>
#include "Http2Compression.h"
#include "mozilla/ChaosMode.h"
#include "mozilla/unused.h"

#include "mozilla/Telemetry.h"


extern PRThread *gSocketThread;

namespace mozilla {
namespace net {



NS_IMPL_ISUPPORTS(nsHttpConnectionMgr, nsIObserver)

static void
InsertTransactionSorted(nsTArray<nsHttpTransaction*> &pendingQ, nsHttpTransaction *trans)
{
    
    
    

    for (int32_t i=pendingQ.Length()-1; i>=0; --i) {
        nsHttpTransaction *t = pendingQ[i];
        if (trans->Priority() >= t->Priority()) {
	  if (ChaosMode::isActive(ChaosMode::NetworkScheduling)) {
                int32_t samePriorityCount;
                for (samePriorityCount = 0; i - samePriorityCount >= 0; ++samePriorityCount) {
                    if (pendingQ[i - samePriorityCount]->Priority() != trans->Priority()) {
                        break;
                    }
                }
                
                i -= ChaosMode::randomUint32LessThan(samePriorityCount + 1);
            }
            pendingQ.InsertElementAt(i+1, trans);
            return;
        }
    }
    pendingQ.InsertElementAt(0, trans);
}



nsHttpConnectionMgr::nsHttpConnectionMgr()
    : mReentrantMonitor("nsHttpConnectionMgr.mReentrantMonitor")
    , mMaxConns(0)
    , mMaxPersistConnsPerHost(0)
    , mMaxPersistConnsPerProxy(0)
    , mIsShuttingDown(false)
    , mNumActiveConns(0)
    , mNumIdleConns(0)
    , mNumSpdyActiveConns(0)
    , mNumHalfOpenConns(0)
    , mTimeOfNextWakeUp(UINT64_MAX)
    , mPruningNoTraffic(false)
    , mTimeoutTickArmed(false)
    , mTimeoutTickNext(1)
{
    LOG(("Creating nsHttpConnectionMgr @%p\n", this));
}

nsHttpConnectionMgr::~nsHttpConnectionMgr()
{
    LOG(("Destroying nsHttpConnectionMgr @%p\n", this));
    if (mTimeoutTick)
        mTimeoutTick->Cancel();
}

nsresult
nsHttpConnectionMgr::EnsureSocketThreadTarget()
{
    nsresult rv;
    nsCOMPtr<nsIEventTarget> sts;
    nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
    if (NS_SUCCEEDED(rv))
        sts = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);

    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    
    if (mSocketThreadTarget || mIsShuttingDown)
        return NS_OK;

    mSocketThreadTarget = sts;

    return rv;
}

nsresult
nsHttpConnectionMgr::Init(uint16_t maxConns,
                          uint16_t maxPersistConnsPerHost,
                          uint16_t maxPersistConnsPerProxy,
                          uint16_t maxRequestDelay,
                          uint16_t maxPipelinedRequests,
                          uint16_t maxOptimisticPipelinedRequests)
{
    LOG(("nsHttpConnectionMgr::Init\n"));

    {
        ReentrantMonitorAutoEnter mon(mReentrantMonitor);

        mMaxConns = maxConns;
        mMaxPersistConnsPerHost = maxPersistConnsPerHost;
        mMaxPersistConnsPerProxy = maxPersistConnsPerProxy;
        mMaxRequestDelay = maxRequestDelay;
        mMaxPipelinedRequests = maxPipelinedRequests;
        mMaxOptimisticPipelinedRequests = maxOptimisticPipelinedRequests;

        mIsShuttingDown = false;
    }

    return EnsureSocketThreadTarget();
}

nsresult
nsHttpConnectionMgr::Shutdown()
{
    LOG(("nsHttpConnectionMgr::Shutdown\n"));

    bool shutdown = false;
    {
        ReentrantMonitorAutoEnter mon(mReentrantMonitor);

        
        if (!mSocketThreadTarget)
            return NS_OK;

        nsresult rv = PostEvent(&nsHttpConnectionMgr::OnMsgShutdown,
                                0, &shutdown);

        
        
        
        mIsShuttingDown = true;
        mSocketThreadTarget = 0;

        if (NS_FAILED(rv)) {
            NS_WARNING("unable to post SHUTDOWN message");
            return rv;
        }
    }

    
    while (!shutdown)
        NS_ProcessNextEvent(NS_GetCurrentThread());
    Http2CompressionCleanup();

    return NS_OK;
}

nsresult
nsHttpConnectionMgr::PostEvent(nsConnEventHandler handler, int32_t iparam, void *vparam)
{
    EnsureSocketThreadTarget();

    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    nsresult rv;
    if (!mSocketThreadTarget) {
        NS_WARNING("cannot post event if not initialized");
        rv = NS_ERROR_NOT_INITIALIZED;
    }
    else {
        nsCOMPtr<nsIRunnable> event = new nsConnEvent(this, handler, iparam, vparam);
        rv = mSocketThreadTarget->Dispatch(event, NS_DISPATCH_NORMAL);
    }
    return rv;
}

void
nsHttpConnectionMgr::PruneDeadConnectionsAfter(uint32_t timeInSeconds)
{
    LOG(("nsHttpConnectionMgr::PruneDeadConnectionsAfter\n"));

    if(!mTimer)
        mTimer = do_CreateInstance("@mozilla.org/timer;1");

    
    
    if (mTimer) {
        mTimeOfNextWakeUp = timeInSeconds + NowInSeconds();
        mTimer->Init(this, timeInSeconds*1000, nsITimer::TYPE_ONE_SHOT);
    } else {
        NS_WARNING("failed to create: timer for pruning the dead connections!");
    }
}

void
nsHttpConnectionMgr::ConditionallyStopPruneDeadConnectionsTimer()
{
    
    
    if (mNumIdleConns || (mNumActiveConns && gHttpHandler->IsSpdyEnabled()))
        return;

    LOG(("nsHttpConnectionMgr::StopPruneDeadConnectionsTimer\n"));

    
    mTimeOfNextWakeUp = UINT64_MAX;
    if (mTimer) {
        mTimer->Cancel();
        mTimer = nullptr;
    }
}

void
nsHttpConnectionMgr::ConditionallyStopTimeoutTick()
{
    LOG(("nsHttpConnectionMgr::ConditionallyStopTimeoutTick "
         "armed=%d active=%d\n", mTimeoutTickArmed, mNumActiveConns));

    if (!mTimeoutTickArmed)
        return;

    if (mNumActiveConns)
        return;

    LOG(("nsHttpConnectionMgr::ConditionallyStopTimeoutTick stop==true\n"));

    mTimeoutTick->Cancel();
    mTimeoutTickArmed = false;
}





NS_IMETHODIMP
nsHttpConnectionMgr::Observe(nsISupports *subject,
                             const char *topic,
                             const char16_t *data)
{
    LOG(("nsHttpConnectionMgr::Observe [topic=\"%s\"]\n", topic));

    if (0 == strcmp(topic, NS_TIMER_CALLBACK_TOPIC)) {
        nsCOMPtr<nsITimer> timer = do_QueryInterface(subject);
        if (timer == mTimer) {
            PruneDeadConnections();
        }
        else if (timer == mTimeoutTick) {
            TimeoutTick();
        } else if (timer == mTrafficTimer) {
            PruneNoTraffic();
        }
        else {
            MOZ_ASSERT(false, "unexpected timer-callback");
            LOG(("Unexpected timer object\n"));
            return NS_ERROR_UNEXPECTED;
        }
    }

    return NS_OK;
}




nsresult
nsHttpConnectionMgr::AddTransaction(nsHttpTransaction *trans, int32_t priority)
{
    LOG(("nsHttpConnectionMgr::AddTransaction [trans=%p %d]\n", trans, priority));

    NS_ADDREF(trans);
    nsresult rv = PostEvent(&nsHttpConnectionMgr::OnMsgNewTransaction, priority, trans);
    if (NS_FAILED(rv))
        NS_RELEASE(trans);
    return rv;
}

nsresult
nsHttpConnectionMgr::RescheduleTransaction(nsHttpTransaction *trans, int32_t priority)
{
    LOG(("nsHttpConnectionMgr::RescheduleTransaction [trans=%p %d]\n", trans, priority));

    NS_ADDREF(trans);
    nsresult rv = PostEvent(&nsHttpConnectionMgr::OnMsgReschedTransaction, priority, trans);
    if (NS_FAILED(rv))
        NS_RELEASE(trans);
    return rv;
}

nsresult
nsHttpConnectionMgr::CancelTransaction(nsHttpTransaction *trans, nsresult reason)
{
    LOG(("nsHttpConnectionMgr::CancelTransaction [trans=%p reason=%x]\n", trans, reason));

    NS_ADDREF(trans);
    nsresult rv = PostEvent(&nsHttpConnectionMgr::OnMsgCancelTransaction,
                            static_cast<int32_t>(reason), trans);
    if (NS_FAILED(rv))
        NS_RELEASE(trans);
    return rv;
}

nsresult
nsHttpConnectionMgr::PruneDeadConnections()
{
    return PostEvent(&nsHttpConnectionMgr::OnMsgPruneDeadConnections);
}




nsresult
nsHttpConnectionMgr::PruneNoTraffic()
{
    LOG(("nsHttpConnectionMgr::PruneNoTraffic\n"));
    mPruningNoTraffic = true;
    return PostEvent(&nsHttpConnectionMgr::OnMsgPruneNoTraffic);
}

nsresult
nsHttpConnectionMgr::VerifyTraffic()
{
    LOG(("nsHttpConnectionMgr::VerifyTraffic\n"));
    return PostEvent(&nsHttpConnectionMgr::OnMsgVerifyTraffic);
}


nsresult
nsHttpConnectionMgr::DoShiftReloadConnectionCleanup(nsHttpConnectionInfo *aCI)
{
    nsRefPtr<nsHttpConnectionInfo> connInfo(aCI);

    nsresult rv = PostEvent(&nsHttpConnectionMgr::OnMsgDoShiftReloadConnectionCleanup,
                            0, connInfo);
    if (NS_SUCCEEDED(rv))
        unused << connInfo.forget();
    return rv;
}

class SpeculativeConnectArgs
{
    virtual ~SpeculativeConnectArgs() {}

public:
    SpeculativeConnectArgs() { mOverridesOK = false; }

    
    
    NS_IMETHOD_(MozExternalRefCountType) AddRef(void);
    NS_IMETHOD_(MozExternalRefCountType) Release(void);

public: 
    nsRefPtr<NullHttpTransaction> mTrans;

    bool mOverridesOK;
    uint32_t mParallelSpeculativeConnectLimit;
    bool mIgnoreIdle;
    bool mIgnorePossibleSpdyConnections;
    bool mIsFromPredictor;
    bool mAllow1918;

    
    
protected:
    ThreadSafeAutoRefCnt mRefCnt;
    NS_DECL_OWNINGTHREAD
};

NS_IMPL_ADDREF(SpeculativeConnectArgs)
NS_IMPL_RELEASE(SpeculativeConnectArgs)

nsresult
nsHttpConnectionMgr::SpeculativeConnect(nsHttpConnectionInfo *ci,
                                        nsIInterfaceRequestor *callbacks,
                                        uint32_t caps,
                                        NullHttpTransaction *nullTransaction)
{
    MOZ_ASSERT(NS_IsMainThread(), "nsHttpConnectionMgr::SpeculativeConnect called off main thread!");

    LOG(("nsHttpConnectionMgr::SpeculativeConnect [ci=%s]\n",
         ci->HashKey().get()));

    nsCOMPtr<nsISpeculativeConnectionOverrider> overrider =
        do_GetInterface(callbacks);

    bool allow1918 = false;
    if (overrider) {
        overrider->GetAllow1918(&allow1918);
    }

    
    
    if ((!allow1918) && ci && ci->HostIsLocalIPLiteral()) {
        LOG(("nsHttpConnectionMgr::SpeculativeConnect skipping RFC1918 "
             "address [%s]", ci->Host()));
        return NS_OK;
    }

    nsRefPtr<SpeculativeConnectArgs> args = new SpeculativeConnectArgs();

    
    
    nsCOMPtr<nsIInterfaceRequestor> wrappedCallbacks;
    NS_NewInterfaceRequestorAggregation(callbacks, nullptr, getter_AddRefs(wrappedCallbacks));

    caps |= ci->GetAnonymous() ? NS_HTTP_LOAD_ANONYMOUS : 0;
    args->mTrans =
        nullTransaction ? nullTransaction : new NullHttpTransaction(ci, wrappedCallbacks, caps);

    if (overrider) {
        args->mOverridesOK = true;
        overrider->GetParallelSpeculativeConnectLimit(
            &args->mParallelSpeculativeConnectLimit);
        overrider->GetIgnoreIdle(&args->mIgnoreIdle);
        overrider->GetIgnorePossibleSpdyConnections(
            &args->mIgnorePossibleSpdyConnections);
        overrider->GetIsFromPredictor(&args->mIsFromPredictor);
        overrider->GetAllow1918(&args->mAllow1918);
    }

    nsresult rv =
        PostEvent(&nsHttpConnectionMgr::OnMsgSpeculativeConnect, 0, args);
    if (NS_SUCCEEDED(rv))
        unused << args.forget();
    return rv;
}

nsresult
nsHttpConnectionMgr::GetSocketThreadTarget(nsIEventTarget **target)
{
    EnsureSocketThreadTarget();

    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    NS_IF_ADDREF(*target = mSocketThreadTarget);
    return NS_OK;
}

nsresult
nsHttpConnectionMgr::ReclaimConnection(nsHttpConnection *conn)
{
    LOG(("nsHttpConnectionMgr::ReclaimConnection [conn=%p]\n", conn));

    NS_ADDREF(conn);
    nsresult rv = PostEvent(&nsHttpConnectionMgr::OnMsgReclaimConnection, 0, conn);
    if (NS_FAILED(rv))
        NS_RELEASE(conn);
    return rv;
}



class nsCompleteUpgradeData
{
public:
nsCompleteUpgradeData(nsAHttpConnection *aConn,
                      nsIHttpUpgradeListener *aListener)
    : mConn(aConn), mUpgradeListener(aListener) {}

    nsRefPtr<nsAHttpConnection> mConn;
    nsCOMPtr<nsIHttpUpgradeListener> mUpgradeListener;
};

nsresult
nsHttpConnectionMgr::CompleteUpgrade(nsAHttpConnection *aConn,
                                     nsIHttpUpgradeListener *aUpgradeListener)
{
    nsCompleteUpgradeData *data =
        new nsCompleteUpgradeData(aConn, aUpgradeListener);
    nsresult rv;
    rv = PostEvent(&nsHttpConnectionMgr::OnMsgCompleteUpgrade, 0, data);
    if (NS_FAILED(rv))
        delete data;
    return rv;
}

nsresult
nsHttpConnectionMgr::UpdateParam(nsParamName name, uint16_t value)
{
    uint32_t param = (uint32_t(name) << 16) | uint32_t(value);
    return PostEvent(&nsHttpConnectionMgr::OnMsgUpdateParam, 0,
                     (void *)(uintptr_t) param);
}

nsresult
nsHttpConnectionMgr::ProcessPendingQ(nsHttpConnectionInfo *ci)
{
    LOG(("nsHttpConnectionMgr::ProcessPendingQ [ci=%s]\n", ci->HashKey().get()));

    NS_ADDREF(ci);
    nsresult rv = PostEvent(&nsHttpConnectionMgr::OnMsgProcessPendingQ, 0, ci);
    if (NS_FAILED(rv))
        NS_RELEASE(ci);
    return rv;
}

nsresult
nsHttpConnectionMgr::ProcessPendingQ()
{
    LOG(("nsHttpConnectionMgr::ProcessPendingQ [All CI]\n"));
    return PostEvent(&nsHttpConnectionMgr::OnMsgProcessPendingQ, 0, nullptr);
}

void
nsHttpConnectionMgr::OnMsgUpdateRequestTokenBucket(int32_t, void *param)
{
    nsRefPtr<EventTokenBucket> tokenBucket =
        dont_AddRef(static_cast<EventTokenBucket *>(param));
    gHttpHandler->SetRequestTokenBucket(tokenBucket);
}

nsresult
nsHttpConnectionMgr::UpdateRequestTokenBucket(EventTokenBucket *aBucket)
{
    nsRefPtr<EventTokenBucket> bucket(aBucket);

    
    
    nsresult rv = PostEvent(&nsHttpConnectionMgr::OnMsgUpdateRequestTokenBucket,
                            0, bucket);
    if (NS_SUCCEEDED(rv))
        unused << bucket.forget();
    return rv;
}

PLDHashOperator
nsHttpConnectionMgr::RemoveDeadConnections(const nsACString &key,
                nsAutoPtr<nsConnectionEntry> &ent,
                void *aArg)
{
    if (ent->mIdleConns.Length()   == 0 &&
        ent->mActiveConns.Length() == 0 &&
        ent->mHalfOpens.Length()   == 0 &&
        ent->mPendingQ.Length()    == 0) {
        return PL_DHASH_REMOVE;
    }
    return PL_DHASH_NEXT;
}

nsresult
nsHttpConnectionMgr::ClearConnectionHistory()
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    mCT.Enumerate(RemoveDeadConnections, nullptr);
    return NS_OK;
}


nsHttpConnectionMgr::nsConnectionEntry *
nsHttpConnectionMgr::LookupPreferredHash(nsHttpConnectionMgr::nsConnectionEntry *ent)
{
    nsConnectionEntry *preferred = nullptr;
    uint32_t len = ent->mCoalescingKeys.Length();
    for (uint32_t i = 0; !preferred && (i < len); ++i) {
        preferred = mSpdyPreferredHash.Get(ent->mCoalescingKeys[i]);
    }
    return preferred;
}

void
nsHttpConnectionMgr::StorePreferredHash(nsHttpConnectionMgr::nsConnectionEntry *ent)
{
    if (ent->mCoalescingKeys.IsEmpty()) {
        return;
    }

    ent->mInPreferredHash = true;
    uint32_t len = ent->mCoalescingKeys.Length();
    for (uint32_t i = 0; i < len; ++i) {
        mSpdyPreferredHash.Put(ent->mCoalescingKeys[i], ent);
    }
}

void
nsHttpConnectionMgr::RemovePreferredHash(nsHttpConnectionMgr::nsConnectionEntry *ent)
{
    if (!ent->mInPreferredHash || ent->mCoalescingKeys.IsEmpty()) {
        return;
    }

    ent->mInPreferredHash = false;
    uint32_t len = ent->mCoalescingKeys.Length();
    for (uint32_t i = 0; i < len; ++i) {
        mSpdyPreferredHash.Remove(ent->mCoalescingKeys[i]);
    }
}






nsHttpConnectionMgr::nsConnectionEntry *
nsHttpConnectionMgr::LookupConnectionEntry(nsHttpConnectionInfo *ci,
                                           nsHttpConnection *conn,
                                           nsHttpTransaction *trans)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    if (!ci)
        return nullptr;

    nsConnectionEntry *ent = mCT.Get(ci->HashKey());

    
    
    if (!ent || !ent->mUsingSpdy || ent->mCoalescingKeys.IsEmpty())
        return ent;

    
    
    
    nsConnectionEntry *preferred = LookupPreferredHash(ent);
    if (!preferred || (preferred == ent))
        return ent;

    if (conn) {
        
        
        if (preferred->mActiveConns.Contains(conn))
            return preferred;
        if (preferred->mIdleConns.Contains(conn))
            return preferred;
    }

    if (trans && preferred->mPendingQ.Contains(trans))
        return preferred;

    
    return ent;
}

nsresult
nsHttpConnectionMgr::CloseIdleConnection(nsHttpConnection *conn)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    LOG(("nsHttpConnectionMgr::CloseIdleConnection %p conn=%p",
         this, conn));

    if (!conn->ConnectionInfo())
        return NS_ERROR_UNEXPECTED;

    nsConnectionEntry *ent = LookupConnectionEntry(conn->ConnectionInfo(),
                                                   conn, nullptr);

    if (!ent || !ent->mIdleConns.RemoveElement(conn))
        return NS_ERROR_UNEXPECTED;

    conn->Close(NS_ERROR_ABORT);
    NS_RELEASE(conn);
    mNumIdleConns--;
    ConditionallyStopPruneDeadConnectionsTimer();
    return NS_OK;
}








void
nsHttpConnectionMgr::ReportSpdyConnection(nsHttpConnection *conn,
                                          bool usingSpdy)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    nsConnectionEntry *ent = LookupConnectionEntry(conn->ConnectionInfo(),
                                                   conn, nullptr);

    if (!ent)
        return;

    ent->mTestedSpdy = true;

    if (!usingSpdy)
        return;

    ent->mUsingSpdy = true;
    mNumSpdyActiveConns++;

    uint32_t ttl = conn->TimeToLive();
    uint64_t timeOfExpire = NowInSeconds() + ttl;
    if (!mTimer || timeOfExpire < mTimeOfNextWakeUp)
        PruneDeadConnectionsAfter(ttl);

    
    
    
    
    
    nsConnectionEntry *joinedConnection;
    nsConnectionEntry *preferred = LookupPreferredHash(ent);

    LOG(("ReportSpdyConnection %p,%s prefers %p,%s\n",
         ent, ent->mConnInfo->Host(), preferred,
         preferred ? preferred->mConnInfo->Host() : ""));

    if (!preferred) {
        
        StorePreferredHash(ent);
    } else if ((preferred != ent) &&
               (joinedConnection = GetSpdyPreferredEnt(ent)) &&
               (joinedConnection != ent)) {
        
        
        
        
        

        LOG(("ReportSpdyConnection graceful close of conn=%p ent=%p to "
                 "migrate to preferred\n", conn, ent));

        conn->DontReuse();
    } else if (preferred != ent) {
        LOG (("ReportSpdyConnection preferred host may be in false start or "
              "may have insufficient cert. Leave mapping in place but do not "
              "abandon this connection yet."));
    }

    ProcessPendingQ(ent->mConnInfo);
    PostEvent(&nsHttpConnectionMgr::OnMsgProcessAllSpdyPendingQ);
}

void
nsHttpConnectionMgr::ReportSpdyCWNDSetting(nsHttpConnectionInfo *ci,
                                           uint32_t cwndValue)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    if (!gHttpHandler->UseSpdyPersistentSettings())
        return;

    if (!ci)
        return;

    nsConnectionEntry *ent = mCT.Get(ci->HashKey());
    if (!ent)
        return;

    ent = GetSpdyPreferredEnt(ent);
    if (!ent) 
        return;

    cwndValue = std::max(2U, cwndValue);
    cwndValue = std::min(128U, cwndValue);

    ent->mSpdyCWND = cwndValue;
    ent->mSpdyCWNDTimeStamp = TimeStamp::Now();
    return;
}


uint32_t
nsHttpConnectionMgr::GetSpdyCWNDSetting(nsHttpConnectionInfo *ci)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    if (!gHttpHandler->UseSpdyPersistentSettings())
        return 0;

    if (!ci)
        return 0;

    nsConnectionEntry *ent = mCT.Get(ci->HashKey());
    if (!ent)
        return 0;

    ent = GetSpdyPreferredEnt(ent);
    if (!ent) 
        return 0;

    if (ent->mSpdyCWNDTimeStamp.IsNull())
        return 0;

    
    
    
    TimeDuration age = TimeStamp::Now() - ent->mSpdyCWNDTimeStamp;
    if (age.ToMilliseconds() > (1000 * 60 * 60 * 8))
        return 0;

    return ent->mSpdyCWND;
}

nsHttpConnectionMgr::nsConnectionEntry *
nsHttpConnectionMgr::GetSpdyPreferredEnt(nsConnectionEntry *aOriginalEntry)
{
    if (!gHttpHandler->IsSpdyEnabled() ||
        !gHttpHandler->CoalesceSpdy() ||
        aOriginalEntry->mCoalescingKeys.IsEmpty()) {
        return nullptr;
    }

    nsConnectionEntry *preferred = LookupPreferredHash(aOriginalEntry);

    
    if (preferred == aOriginalEntry)
        return aOriginalEntry;

    
    
    if (!preferred || !preferred->mUsingSpdy)
        return nullptr;

    
    
    
    

    nsHttpConnection *activeSpdy = nullptr;

    for (uint32_t index = 0; index < preferred->mActiveConns.Length(); ++index) {
        if (preferred->mActiveConns[index]->CanDirectlyActivate()) {
            activeSpdy = preferred->mActiveConns[index];
            break;
        }
    }

    if (!activeSpdy) {
        
        
        RemovePreferredHash(preferred);
        LOG(("nsHttpConnectionMgr::GetSpdyPreferredConnection "
             "preferred host mapping %s to %s removed due to inactivity.\n",
             aOriginalEntry->mConnInfo->Host(),
             preferred->mConnInfo->Host()));

        return nullptr;
    }

    
    nsresult rv;
    bool isJoined = false;

    nsCOMPtr<nsISupports> securityInfo;
    nsCOMPtr<nsISSLSocketControl> sslSocketControl;
    nsAutoCString negotiatedNPN;

    activeSpdy->GetSecurityInfo(getter_AddRefs(securityInfo));
    if (!securityInfo) {
        NS_WARNING("cannot obtain spdy security info");
        return nullptr;
    }

    sslSocketControl = do_QueryInterface(securityInfo, &rv);
    if (NS_FAILED(rv)) {
        NS_WARNING("sslSocketControl QI Failed");
        return nullptr;
    }

    
    const SpdyInformation *info = gHttpHandler->SpdyInfo();
    for (uint32_t index = SpdyInformation::kCount;
         NS_SUCCEEDED(rv) && index > 0; --index) {
        if (info->ProtocolEnabled(index - 1)) {
            rv = sslSocketControl->JoinConnection(info->VersionString[index - 1],
                                                  aOriginalEntry->mConnInfo->GetHost(),
                                                  aOriginalEntry->mConnInfo->Port(),
                                                  &isJoined);
            if (NS_SUCCEEDED(rv) && isJoined) {
                break;
            }
        }
    }

    if (NS_FAILED(rv) || !isJoined) {
        LOG(("nsHttpConnectionMgr::GetSpdyPreferredConnection "
             "Host %s cannot be confirmed to be joined "
             "with %s connections. rv=%x isJoined=%d",
             preferred->mConnInfo->Host(), aOriginalEntry->mConnInfo->Host(),
             rv, isJoined));
        Telemetry::Accumulate(Telemetry::SPDY_NPN_JOIN, false);
        return nullptr;
    }

    
    LOG(("nsHttpConnectionMgr::GetSpdyPreferredConnection "
         "Host %s has cert valid for %s connections, "
         "so %s will be coalesced with %s",
         preferred->mConnInfo->Host(), aOriginalEntry->mConnInfo->Host(),
         aOriginalEntry->mConnInfo->Host(), preferred->mConnInfo->Host()));
    Telemetry::Accumulate(Telemetry::SPDY_NPN_JOIN, true);
    return preferred;
}




PLDHashOperator
nsHttpConnectionMgr::ProcessOneTransactionCB(const nsACString &key,
                                             nsAutoPtr<nsConnectionEntry> &ent,
                                             void *closure)
{
    nsHttpConnectionMgr *self = (nsHttpConnectionMgr *) closure;

    if (self->ProcessPendingQForEntry(ent, false))
        return PL_DHASH_STOP;

    return PL_DHASH_NEXT;
}

PLDHashOperator
nsHttpConnectionMgr::ProcessAllTransactionsCB(const nsACString &key,
                                              nsAutoPtr<nsConnectionEntry> &ent,
                                              void *closure)
{
    nsHttpConnectionMgr *self = (nsHttpConnectionMgr *) closure;
    self->ProcessPendingQForEntry(ent, true);
    return PL_DHASH_NEXT;
}




PLDHashOperator
nsHttpConnectionMgr::PurgeExcessIdleConnectionsCB(const nsACString &key,
                                                  nsAutoPtr<nsConnectionEntry> &ent,
                                                  void *closure)
{
    nsHttpConnectionMgr *self = (nsHttpConnectionMgr *) closure;

    while (self->mNumIdleConns + self->mNumActiveConns + 1 >= self->mMaxConns) {
        if (!ent->mIdleConns.Length()) {
            
            return PL_DHASH_NEXT;
        }
        nsHttpConnection *conn = ent->mIdleConns[0];
        ent->mIdleConns.RemoveElementAt(0);
        conn->Close(NS_ERROR_ABORT);
        NS_RELEASE(conn);
        self->mNumIdleConns--;
        self->ConditionallyStopPruneDeadConnectionsTimer();
    }
    return PL_DHASH_STOP;
}




PLDHashOperator
nsHttpConnectionMgr::PurgeExcessSpdyConnectionsCB(const nsACString &key,
                                                  nsAutoPtr<nsConnectionEntry> &ent,
                                                  void *closure)
{
    if (!ent->mUsingSpdy)
        return PL_DHASH_NEXT;

    nsHttpConnectionMgr *self = static_cast<nsHttpConnectionMgr *>(closure);
    for (uint32_t index = 0; index < ent->mActiveConns.Length(); ++index) {
        nsHttpConnection *conn = ent->mActiveConns[index];
        if (conn->UsingSpdy() && conn->CanReuse()) {
            conn->DontReuse();
            
            if (self->mNumIdleConns + self->mNumActiveConns + 1 <= self->mMaxConns)
                return PL_DHASH_STOP;
        }
    }
    return PL_DHASH_NEXT;
}

PLDHashOperator
nsHttpConnectionMgr::PruneDeadConnectionsCB(const nsACString &key,
                                            nsAutoPtr<nsConnectionEntry> &ent,
                                            void *closure)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    nsHttpConnectionMgr *self = (nsHttpConnectionMgr *) closure;

    LOG(("  pruning [ci=%s]\n", ent->mConnInfo->HashKey().get()));

    
    
    uint32_t timeToNextExpire = UINT32_MAX;
    int32_t count = ent->mIdleConns.Length();
    if (count > 0) {
        for (int32_t i=count-1; i>=0; --i) {
            nsHttpConnection *conn = ent->mIdleConns[i];
            if (!conn->CanReuse()) {
                ent->mIdleConns.RemoveElementAt(i);
                conn->Close(NS_ERROR_ABORT);
                NS_RELEASE(conn);
                self->mNumIdleConns--;
            } else {
                timeToNextExpire = std::min(timeToNextExpire, conn->TimeToLive());
            }
        }
    }

    if (ent->mUsingSpdy) {
        for (uint32_t index = 0; index < ent->mActiveConns.Length(); ++index) {
            nsHttpConnection *conn = ent->mActiveConns[index];
            if (conn->UsingSpdy()) {
                if (!conn->CanReuse()) {
                    
                    
                    conn->DontReuse();
                }
                else {
                    timeToNextExpire = std::min(timeToNextExpire,
                                              conn->TimeToLive());
                }
            }
        }
    }

    
    
    if (timeToNextExpire != UINT32_MAX) {
        uint32_t now = NowInSeconds();
        uint64_t timeOfNextExpire = now + timeToNextExpire;
        
        
        
        
        if (!self->mTimer || timeOfNextExpire < self->mTimeOfNextWakeUp) {
            self->PruneDeadConnectionsAfter(timeToNextExpire);
        }
    } else {
        self->ConditionallyStopPruneDeadConnectionsTimer();
    }

    
    
    
    
    if (ent->PipelineState()       != PS_RED &&
        self->mCT.Count()          >  125 &&
        ent->mIdleConns.Length()   == 0 &&
        ent->mActiveConns.Length() == 0 &&
        ent->mHalfOpens.Length()   == 0 &&
        ent->mPendingQ.Length()    == 0 &&
        ((!ent->mTestedSpdy && !ent->mUsingSpdy) ||
         !gHttpHandler->IsSpdyEnabled() ||
         self->mCT.Count() > 300)) {
        LOG(("    removing empty connection entry\n"));
        return PL_DHASH_REMOVE;
    }

    
    ent->mIdleConns.Compact();
    ent->mActiveConns.Compact();
    ent->mPendingQ.Compact();

    return PL_DHASH_NEXT;
}

PLDHashOperator
nsHttpConnectionMgr::VerifyTrafficCB(const nsACString &key,
                                     nsAutoPtr<nsConnectionEntry> &ent,
                                     void *closure)
{
    
    for (uint32_t index = 0; index < ent->mActiveConns.Length(); ++index) {
        nsHttpConnection *conn = ent->mActiveConns[index];
        conn->CheckForTraffic(true);
    }
    
    for (uint32_t index = 0; index < ent->mIdleConns.Length(); ++index) {
        nsHttpConnection *conn = ent->mIdleConns[index];
        conn->CheckForTraffic(false);
    }

    return PL_DHASH_NEXT;
}

PLDHashOperator
nsHttpConnectionMgr::PruneNoTrafficCB(const nsACString &key,
                                      nsAutoPtr<nsConnectionEntry> &ent,
                                      void *closure)
{
    
    nsHttpConnectionMgr *self = (nsHttpConnectionMgr *) closure;

    LOG(("  pruning no traffic [ci=%s]\n", ent->mConnInfo->HashKey().get()));

    uint32_t numConns = ent->mActiveConns.Length();
    if (numConns) {
        
        for (int index = numConns-1; index >= 0; index--) {
            if (ent->mActiveConns[index]->NoTraffic()) {
              nsRefPtr<nsHttpConnection> conn = dont_AddRef(ent->mActiveConns[index]);
              ent->mActiveConns.RemoveElementAt(index);
              self->DecrementActiveConnCount(conn);
              conn->Close(NS_ERROR_ABORT);
              LOG(("  closed active connection due to no traffic [conn=%p]\n",
                   conn.get()));
            }
        }
    }

    return PL_DHASH_NEXT;
}

PLDHashOperator
nsHttpConnectionMgr::ShutdownPassCB(const nsACString &key,
                                    nsAutoPtr<nsConnectionEntry> &ent,
                                    void *closure)
{
    nsHttpConnectionMgr *self = (nsHttpConnectionMgr *) closure;

    nsHttpTransaction *trans;
    nsHttpConnection *conn;

    
    while (ent->mActiveConns.Length()) {
        conn = ent->mActiveConns[0];

        ent->mActiveConns.RemoveElementAt(0);
        self->DecrementActiveConnCount(conn);

        conn->Close(NS_ERROR_ABORT);
        NS_RELEASE(conn);
    }

    
    while (ent->mIdleConns.Length()) {
        conn = ent->mIdleConns[0];

        ent->mIdleConns.RemoveElementAt(0);
        self->mNumIdleConns--;

        conn->Close(NS_ERROR_ABORT);
        NS_RELEASE(conn);
    }
    
    
    self->ConditionallyStopPruneDeadConnectionsTimer();

    
    while (ent->mPendingQ.Length()) {
        trans = ent->mPendingQ[0];

        ent->mPendingQ.RemoveElementAt(0);

        trans->Close(NS_ERROR_ABORT);
        NS_RELEASE(trans);
    }

    
    for (int32_t i = ((int32_t) ent->mHalfOpens.Length()) - 1; i >= 0; i--)
        ent->mHalfOpens[i]->Abandon();

    return PL_DHASH_REMOVE;
}



bool
nsHttpConnectionMgr::ProcessPendingQForEntry(nsConnectionEntry *ent, bool considerAll)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    LOG(("nsHttpConnectionMgr::ProcessPendingQForEntry "
         "[ci=%s ent=%p active=%d idle=%d queued=%d]\n",
         ent->mConnInfo->HashKey().get(), ent, ent->mActiveConns.Length(),
         ent->mIdleConns.Length(), ent->mPendingQ.Length()));

    ProcessSpdyPendingQ(ent);

    nsHttpTransaction *trans;
    nsresult rv;
    bool dispatchedSuccessfully = false;

    
    
    
    for (uint32_t i = 0; i < ent->mPendingQ.Length(); ) {
        trans = ent->mPendingQ[i];

        
        
        
        
        
        bool alreadyHalfOpen = false;
        for (int32_t j = 0; j < ((int32_t) ent->mHalfOpens.Length()); ++j) {
            if (ent->mHalfOpens[j]->Transaction() == trans) {
                alreadyHalfOpen = true;
                break;
            }
        }

        rv = TryDispatchTransaction(ent,
                                    alreadyHalfOpen || !!trans->TunnelProvider(),
                                    trans);
        if (NS_SUCCEEDED(rv) || (rv != NS_ERROR_NOT_AVAILABLE)) {
            if (NS_SUCCEEDED(rv))
                LOG(("  dispatching pending transaction...\n"));
            else
                LOG(("  removing pending transaction based on "
                     "TryDispatchTransaction returning hard error %x\n", rv));

            if (ent->mPendingQ.RemoveElement(trans)) {
                dispatchedSuccessfully = true;
                NS_RELEASE(trans);
                continue; 
            }

            LOG(("  transaction not found in pending queue\n"));
        }

        if (dispatchedSuccessfully && !considerAll)
            break;

        ++i;
    }
    return dispatchedSuccessfully;
}

bool
nsHttpConnectionMgr::ProcessPendingQForEntry(nsHttpConnectionInfo *ci)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    nsConnectionEntry *ent = mCT.Get(ci->HashKey());
    if (ent)
        return ProcessPendingQForEntry(ent, false);
    return false;
}

bool
nsHttpConnectionMgr::SupportsPipelining(nsHttpConnectionInfo *ci)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    nsConnectionEntry *ent = mCT.Get(ci->HashKey());
    if (ent)
        return ent->SupportsPipelining();
    return false;
}



class nsHttpPipelineFeedback
{
public:
    nsHttpPipelineFeedback(nsHttpConnectionInfo *ci,
                           nsHttpConnectionMgr::PipelineFeedbackInfoType info,
                           nsHttpConnection *conn, uint32_t data)
        : mConnInfo(ci)
        , mConn(conn)
        , mInfo(info)
        , mData(data)
        {
        }

    ~nsHttpPipelineFeedback()
    {
    }

    nsRefPtr<nsHttpConnectionInfo> mConnInfo;
    nsRefPtr<nsHttpConnection> mConn;
    nsHttpConnectionMgr::PipelineFeedbackInfoType mInfo;
    uint32_t mData;
};

void
nsHttpConnectionMgr::PipelineFeedbackInfo(nsHttpConnectionInfo *ci,
                                          PipelineFeedbackInfoType info,
                                          nsHttpConnection *conn,
                                          uint32_t data)
{
    if (!ci)
        return;

    
    if (PR_GetCurrentThread() != gSocketThread) {
        nsHttpPipelineFeedback *fb = new nsHttpPipelineFeedback(ci, info,
                                                                conn, data);

        nsresult rv = PostEvent(&nsHttpConnectionMgr::OnMsgProcessFeedback,
                                0, fb);
        if (NS_FAILED(rv))
            delete fb;
        return;
    }

    nsConnectionEntry *ent = mCT.Get(ci->HashKey());

    if (ent)
        ent->OnPipelineFeedbackInfo(info, conn, data);
}

void
nsHttpConnectionMgr::ReportFailedToProcess(nsIURI *uri)
{
    MOZ_ASSERT(uri);

    nsAutoCString host;
    int32_t port = -1;
    nsAutoCString username;
    bool usingSSL = false;
    bool isHttp = false;

    nsresult rv = uri->SchemeIs("https", &usingSSL);
    if (NS_SUCCEEDED(rv) && usingSSL)
        isHttp = true;
    if (NS_SUCCEEDED(rv) && !isHttp)
        rv = uri->SchemeIs("http", &isHttp);
    if (NS_SUCCEEDED(rv))
        rv = uri->GetAsciiHost(host);
    if (NS_SUCCEEDED(rv))
        rv = uri->GetPort(&port);
    if (NS_SUCCEEDED(rv))
        uri->GetUsername(username);
    if (NS_FAILED(rv) || !isHttp || host.IsEmpty())
        return;

    
    
    nsRefPtr<nsHttpConnectionInfo> ci =
        new nsHttpConnectionInfo(host, port, EmptyCString(), username, nullptr, usingSSL);
    ci->SetAnonymous(false);
    ci->SetPrivate(false);
    PipelineFeedbackInfo(ci, RedCorruptedContent, nullptr, 0);

    ci = ci->Clone();
    ci->SetAnonymous(false);
    ci->SetPrivate(true);
    PipelineFeedbackInfo(ci, RedCorruptedContent, nullptr, 0);

    ci = ci->Clone();
    ci->SetAnonymous(true);
    ci->SetPrivate(false);
    PipelineFeedbackInfo(ci, RedCorruptedContent, nullptr, 0);

    ci = ci->Clone();
    ci->SetAnonymous(true);
    ci->SetPrivate(true);
    PipelineFeedbackInfo(ci, RedCorruptedContent, nullptr, 0);
}





bool
nsHttpConnectionMgr::AtActiveConnectionLimit(nsConnectionEntry *ent, uint32_t caps)
{
    nsHttpConnectionInfo *ci = ent->mConnInfo;

    LOG(("nsHttpConnectionMgr::AtActiveConnectionLimit [ci=%s caps=%x]\n",
        ci->HashKey().get(), caps));

    
    
    
    uint32_t maxSocketCount = gHttpHandler->MaxSocketCount();
    if (mMaxConns > maxSocketCount) {
        mMaxConns = maxSocketCount;
        LOG(("nsHttpConnectionMgr %p mMaxConns dynamically reduced to %u",
             this, mMaxConns));
    }

    
    
    if (mNumActiveConns >= mMaxConns) {
        LOG(("  num active conns == max conns\n"));
        return true;
    }

    
    
    
    
    
    uint32_t totalCount =
        ent->mActiveConns.Length() + ent->UnconnectedHalfOpens();

    uint16_t maxPersistConns;

    if (ci->UsingHttpProxy() && !ci->UsingConnect())
        maxPersistConns = mMaxPersistConnsPerProxy;
    else
        maxPersistConns = mMaxPersistConnsPerHost;

    LOG(("   connection count = %d, limit %d\n", totalCount, maxPersistConns));

    
    bool result = (totalCount >= maxPersistConns);
    LOG(("  result: %s", result ? "true" : "false"));
    return result;
}

void
nsHttpConnectionMgr::ClosePersistentConnections(nsConnectionEntry *ent)
{
    LOG(("nsHttpConnectionMgr::ClosePersistentConnections [ci=%s]\n",
         ent->mConnInfo->HashKey().get()));
    while (ent->mIdleConns.Length()) {
        nsHttpConnection *conn = ent->mIdleConns[0];
        ent->mIdleConns.RemoveElementAt(0);
        mNumIdleConns--;
        conn->Close(NS_ERROR_ABORT);
        NS_RELEASE(conn);
    }

    int32_t activeCount = ent->mActiveConns.Length();
    for (int32_t i=0; i < activeCount; i++)
        ent->mActiveConns[i]->DontReuse();
}

PLDHashOperator
nsHttpConnectionMgr::ClosePersistentConnectionsCB(const nsACString &key,
                                                  nsAutoPtr<nsConnectionEntry> &ent,
                                                  void *closure)
{
    nsHttpConnectionMgr *self = static_cast<nsHttpConnectionMgr *>(closure);
    self->ClosePersistentConnections(ent);
    return PL_DHASH_NEXT;
}

bool
nsHttpConnectionMgr::RestrictConnections(nsConnectionEntry *ent,
                                         bool ignorePossibleSpdyConnections)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    
    
    

    bool doRestrict = ent->mConnInfo->FirstHopSSL() &&
        gHttpHandler->IsSpdyEnabled() &&
        ((!ent->mTestedSpdy && !ignorePossibleSpdyConnections) ||
         ent->mUsingSpdy) &&
        (ent->mHalfOpens.Length() || ent->mActiveConns.Length());

    
    if (!doRestrict)
        return false;

    
    
    if (ent->UnconnectedHalfOpens() && !ignorePossibleSpdyConnections)
        return true;

    
    
    
    if (ent->mUsingSpdy && ent->mActiveConns.Length()) {
        bool confirmedRestrict = false;
        for (uint32_t index = 0; index < ent->mActiveConns.Length(); ++index) {
            nsHttpConnection *conn = ent->mActiveConns[index];
            if (!conn->ReportedNPN() || conn->CanDirectlyActivate()) {
                confirmedRestrict = true;
                break;
            }
        }
        doRestrict = confirmedRestrict;
        if (!confirmedRestrict) {
            LOG(("nsHttpConnectionMgr spdy connection restriction to "
                 "%s bypassed.\n", ent->mConnInfo->Host()));
        }
    }
    return doRestrict;
}





nsresult
nsHttpConnectionMgr::MakeNewConnection(nsConnectionEntry *ent,
                                       nsHttpTransaction *trans)
{
    LOG(("nsHttpConnectionMgr::MakeNewConnection %p ent=%p trans=%p",
         this, ent, trans));
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    uint32_t halfOpenLength = ent->mHalfOpens.Length();
    for (uint32_t i = 0; i < halfOpenLength; i++) {
        if (ent->mHalfOpens[i]->IsSpeculative()) {
            
            
            
            
            
            LOG(("nsHttpConnectionMgr::MakeNewConnection [ci = %s]\n"
                 "Found a speculative half open connection\n",
                 ent->mConnInfo->HashKey().get()));

            uint32_t flags;
            ent->mHalfOpens[i]->SetSpeculative(false);
            nsISocketTransport *transport = ent->mHalfOpens[i]->SocketTransport();
            if (transport && NS_SUCCEEDED(transport->GetConnectionFlags(&flags))) {
                flags &= ~nsISocketTransport::DISABLE_RFC1918;
                transport->SetConnectionFlags(flags);
            }

            Telemetry::AutoCounter<Telemetry::HTTPCONNMGR_USED_SPECULATIVE_CONN> usedSpeculativeConn;
            ++usedSpeculativeConn;

            if (ent->mHalfOpens[i]->IsFromPredictor()) {
              Telemetry::AutoCounter<Telemetry::PREDICTOR_TOTAL_PRECONNECTS_USED> totalPreconnectsUsed;
              ++totalPreconnectsUsed;
            }

            
            
            return NS_OK;
        }
    }

    
    
    if (trans->Caps() & NS_HTTP_ALLOW_KEEPALIVE) {
        uint32_t activeLength = ent->mActiveConns.Length();
        for (uint32_t i = 0; i < activeLength; i++) {
            nsAHttpTransaction *activeTrans = ent->mActiveConns[i]->Transaction();
            NullHttpTransaction *nullTrans = activeTrans ? activeTrans->QueryNullTransaction() : nullptr;
            if (nullTrans && nullTrans->Claim()) {
                LOG(("nsHttpConnectionMgr::MakeNewConnection [ci = %s] "
                     "Claiming a null transaction for later use\n",
                     ent->mConnInfo->HashKey().get()));
                return NS_OK;
            }
        }
    }

    
    
    
    if (!(trans->Caps() & NS_HTTP_DISALLOW_SPDY) &&
        (trans->Caps() & NS_HTTP_ALLOW_KEEPALIVE) &&
        RestrictConnections(ent)) {
        LOG(("nsHttpConnectionMgr::MakeNewConnection [ci = %s] "
             "Not Available Due to RestrictConnections()\n",
             ent->mConnInfo->HashKey().get()));
        return NS_ERROR_NOT_AVAILABLE;
    }

    
    
    
    
    

    if ((mNumIdleConns + mNumActiveConns + 1 >= mMaxConns) && mNumIdleConns)
        mCT.Enumerate(PurgeExcessIdleConnectionsCB, this);

    if ((mNumIdleConns + mNumActiveConns + 1 >= mMaxConns) &&
        mNumActiveConns && gHttpHandler->IsSpdyEnabled())
        mCT.Enumerate(PurgeExcessSpdyConnectionsCB, this);

    if (AtActiveConnectionLimit(ent, trans->Caps()))
        return NS_ERROR_NOT_AVAILABLE;

    nsresult rv = CreateTransport(ent, trans, trans->Caps(), false, false, true);
    if (NS_FAILED(rv)) {
        
        LOG(("nsHttpConnectionMgr::MakeNewConnection [ci = %s trans = %p] "
             "CreateTransport() hard failure.\n",
             ent->mConnInfo->HashKey().get(), trans));
        trans->Close(rv);
        if (rv == NS_ERROR_NOT_AVAILABLE)
            rv = NS_ERROR_FAILURE;
        return rv;
    }

    return NS_OK;
}

bool
nsHttpConnectionMgr::AddToShortestPipeline(nsConnectionEntry *ent,
                                           nsHttpTransaction *trans,
                                           nsHttpTransaction::Classifier classification,
                                           uint16_t depthLimit)
{
    if (classification == nsAHttpTransaction::CLASS_SOLO)
        return false;

    uint32_t maxdepth = ent->MaxPipelineDepth(classification);
    if (maxdepth == 0) {
        ent->CreditPenalty();
        maxdepth = ent->MaxPipelineDepth(classification);
    }

    if (ent->PipelineState() == PS_RED)
        return false;

    if (ent->PipelineState() == PS_YELLOW && ent->mYellowConnection)
        return false;

    
    
    
    
    
    
    
    
    
    
    

    maxdepth = std::min<uint32_t>(maxdepth, depthLimit);

    if (maxdepth < 2)
        return false;

    nsAHttpTransaction *activeTrans;

    nsHttpConnection *bestConn = nullptr;
    uint32_t activeCount = ent->mActiveConns.Length();
    uint32_t bestConnLength = 0;
    uint32_t connLength;

    for (uint32_t i = 0; i < activeCount; ++i) {
        nsHttpConnection *conn = ent->mActiveConns[i];
        if (!conn->SupportsPipelining())
            continue;

        if (conn->Classification() != classification)
            continue;

        activeTrans = conn->Transaction();
        if (!activeTrans ||
            activeTrans->IsDone() ||
            NS_FAILED(activeTrans->Status()))
            continue;

        connLength = activeTrans->PipelineDepth();

        if (maxdepth <= connLength)
            continue;

        if (!bestConn || (connLength < bestConnLength)) {
            bestConn = conn;
            bestConnLength = connLength;
        }
    }

    if (!bestConn)
        return false;

    activeTrans = bestConn->Transaction();
    nsresult rv = activeTrans->AddTransaction(trans);
    if (NS_FAILED(rv))
        return false;

    LOG(("   scheduling trans %p on pipeline at position %d\n",
         trans, trans->PipelinePosition()));

    if ((ent->PipelineState() == PS_YELLOW) && (trans->PipelinePosition() > 1))
        ent->SetYellowConnection(bestConn);

    if (!trans->GetPendingTime().IsNull()) {
        if (trans->UsesPipelining())
            AccumulateTimeDelta(
                Telemetry::TRANSACTION_WAIT_TIME_HTTP_PIPELINES,
                trans->GetPendingTime(), TimeStamp::Now());
        else
            AccumulateTimeDelta(
                Telemetry::TRANSACTION_WAIT_TIME_HTTP,
                trans->GetPendingTime(), TimeStamp::Now());
        trans->SetPendingTime(false);
    }
    return true;
}

bool
nsHttpConnectionMgr::IsUnderPressure(nsConnectionEntry *ent,
                                   nsHttpTransaction::Classifier classification)
{
    
    
    
    
    
    
    
    
    
    
    
    

    int32_t currentConns = ent->mActiveConns.Length();
    int32_t maxConns =
        (ent->mConnInfo->UsingHttpProxy() && !ent->mConnInfo->UsingConnect()) ?
        mMaxPersistConnsPerProxy : mMaxPersistConnsPerHost;

    
    
    if (currentConns >= (maxConns - 2))
        return true;                           

    int32_t sameClass = 0;
    for (int32_t i = 0; i < currentConns; ++i)
        if (classification == ent->mActiveConns[i]->Classification())
            if (++sameClass == 3)
                return true;                   

    return false;                              
}







nsresult
nsHttpConnectionMgr::TryDispatchTransaction(nsConnectionEntry *ent,
                                            bool onlyReusedConnection,
                                            nsHttpTransaction *trans)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    LOG(("nsHttpConnectionMgr::TryDispatchTransaction without conn "
         "[trans=%p ci=%p ci=%s caps=%x tunnelprovider=%p onlyreused=%d "
         "active=%d idle=%d]\n", trans,
         ent->mConnInfo.get(), ent->mConnInfo->HashKey().get(),
         uint32_t(trans->Caps()), trans->TunnelProvider(),
         onlyReusedConnection, ent->mActiveConns.Length(),
         ent->mIdleConns.Length()));

    nsHttpTransaction::Classifier classification = trans->Classification();
    uint32_t caps = trans->Caps();

    
    if (!(caps & NS_HTTP_ALLOW_KEEPALIVE))
        caps = caps & ~NS_HTTP_ALLOW_PIPELINING;

    
    
    
    
    
    
    
    
    
    
    
    
    

    bool attemptedOptimisticPipeline = !(caps & NS_HTTP_ALLOW_PIPELINING);
    nsRefPtr<nsHttpConnection> unusedSpdyPersistentConnection;

    
    
    

    if (!(caps & NS_HTTP_DISALLOW_SPDY) && gHttpHandler->IsSpdyEnabled()) {
        nsRefPtr<nsHttpConnection> conn = GetSpdyPreferredConn(ent);
        if (conn) {
            if ((caps & NS_HTTP_ALLOW_KEEPALIVE) || !conn->IsExperienced()) {
                LOG(("   dispatch to spdy: [conn=%p]\n", conn.get()));
                trans->RemoveDispatchedAsBlocking();  
                DispatchTransaction(ent, trans, conn);
                return NS_OK;
            }
            unusedSpdyPersistentConnection = conn;
        }
    }

    
    
    
    
    if (!(caps & NS_HTTP_LOAD_AS_BLOCKING)) {
        if (!(caps & NS_HTTP_LOAD_UNBLOCKED)) {
            nsILoadGroupConnectionInfo *loadGroupCI = trans->LoadGroupConnectionInfo();
            if (loadGroupCI) {
                uint32_t blockers = 0;
                if (NS_SUCCEEDED(loadGroupCI->GetBlockingTransactionCount(&blockers)) &&
                    blockers) {
                    
                    LOG(("   blocked by load group: [lgci=%p trans=%p blockers=%d]\n",
                         loadGroupCI, trans, blockers));
                    return NS_ERROR_NOT_AVAILABLE;
                }
            }
        }
    } else {
        
        
        trans->DispatchedAsBlocking();
    }

    
    
    if (IsUnderPressure(ent, classification) && !attemptedOptimisticPipeline) {
        attemptedOptimisticPipeline = true;
        if (AddToShortestPipeline(ent, trans,
                                  classification,
                                  mMaxOptimisticPipelinedRequests)) {
            LOG(("   dispatched step 1 trans=%p\n", trans));
            return NS_OK;
        }
    }

    
    
    
    
    
    
    
    if (gHttpHandler->UseRequestTokenBucket() &&
        (mNumActiveConns >= mNumSpdyActiveConns) && 
        ((mNumActiveConns - mNumSpdyActiveConns) >= gHttpHandler->RequestTokenBucketMinParallelism()) &&
        !(caps & (NS_HTTP_LOAD_AS_BLOCKING | NS_HTTP_LOAD_UNBLOCKED))) {
        if (!trans->TryToRunPacedRequest()) {
            LOG(("   blocked due to rate pacing trans=%p\n", trans));
            return NS_ERROR_NOT_AVAILABLE;
        }
    }

    
    
    if (caps & NS_HTTP_ALLOW_KEEPALIVE) {
        nsRefPtr<nsHttpConnection> conn;
        while (!conn && (ent->mIdleConns.Length() > 0)) {
            conn = ent->mIdleConns[0];
            ent->mIdleConns.RemoveElementAt(0);
            mNumIdleConns--;
            nsHttpConnection *temp = conn;
            NS_RELEASE(temp);

            
            
            if (!conn->CanReuse()) {
                LOG(("   dropping stale connection: [conn=%p]\n", conn.get()));
                conn->Close(NS_ERROR_ABORT);
                conn = nullptr;
            }
            else {
                LOG(("   reusing connection [conn=%p]\n", conn.get()));
                conn->EndIdleMonitoring();
            }

            
            
            ConditionallyStopPruneDeadConnectionsTimer();
        }
        if (conn) {
            
            
            AddActiveConn(conn, ent);
            DispatchTransaction(ent, trans, conn);
            LOG(("   dispatched step 2 (idle) trans=%p\n", trans));
            return NS_OK;
        }
    }

    
    
    if (!attemptedOptimisticPipeline &&
        (classification == nsHttpTransaction::CLASS_REVALIDATION ||
         classification == nsHttpTransaction::CLASS_SCRIPT)) {
        attemptedOptimisticPipeline = true;
        if (AddToShortestPipeline(ent, trans,
                                  classification,
                                  mMaxOptimisticPipelinedRequests)) {
            LOG(("   dispatched step 3 (pipeline) trans=%p\n", trans));
            return NS_OK;
        }
    }

    
    if (!onlyReusedConnection) {
        nsresult rv = MakeNewConnection(ent, trans);
        if (NS_SUCCEEDED(rv)) {
            
            LOG(("   dispatched step 4 (async new conn) trans=%p\n", trans));
            return NS_ERROR_NOT_AVAILABLE;
        }

        if (rv != NS_ERROR_NOT_AVAILABLE) {
            
            
            LOG(("   failed step 4 (%x) trans=%p\n", rv, trans));
            return rv;
        }
    } else if (trans->TunnelProvider() && trans->TunnelProvider()->MaybeReTunnel(trans)) {
        LOG(("   sort of dispatched step 4a tunnel requeue trans=%p\n", trans));
        
        return NS_OK;
    }

    
    if (caps & NS_HTTP_ALLOW_PIPELINING) {
        if (AddToShortestPipeline(ent, trans,
                                  classification,
                                  mMaxPipelinedRequests)) {
            LOG(("   dispatched step 5 trans=%p\n", trans));
            return NS_OK;
        }
    }

    
    if (unusedSpdyPersistentConnection) {
        
        
        
        unusedSpdyPersistentConnection->DontReuse();
    }

    LOG(("   not dispatched (queued) trans=%p\n", trans));
    return NS_ERROR_NOT_AVAILABLE;                
}

nsresult
nsHttpConnectionMgr::DispatchTransaction(nsConnectionEntry *ent,
                                         nsHttpTransaction *trans,
                                         nsHttpConnection *conn)
{
    uint32_t caps = trans->Caps();
    int32_t priority = trans->Priority();
    nsresult rv;

    LOG(("nsHttpConnectionMgr::DispatchTransaction "
         "[ent-ci=%s %p trans=%p caps=%x conn=%p priority=%d]\n",
         ent->mConnInfo->HashKey().get(), ent, trans, caps, conn, priority));

    
    
    
    trans->CancelPacing(NS_OK);

    if (conn->UsingSpdy()) {
        LOG(("Spdy Dispatch Transaction via Activate(). Transaction host = %s, "
             "Connection host = %s\n",
             trans->ConnectionInfo()->Host(),
             conn->ConnectionInfo()->Host()));
        rv = conn->Activate(trans, caps, priority);
        MOZ_ASSERT(NS_SUCCEEDED(rv), "SPDY Cannot Fail Dispatch");
        if (NS_SUCCEEDED(rv) && !trans->GetPendingTime().IsNull()) {
            AccumulateTimeDelta(Telemetry::TRANSACTION_WAIT_TIME_SPDY,
                trans->GetPendingTime(), TimeStamp::Now());
            trans->SetPendingTime(false);
        }
        return rv;
    }

    MOZ_ASSERT(conn && !conn->Transaction(),
               "DispatchTranaction() on non spdy active connection");

    if (!(caps & NS_HTTP_ALLOW_PIPELINING))
        conn->Classify(nsAHttpTransaction::CLASS_SOLO);
    else
        conn->Classify(trans->Classification());

    rv = DispatchAbstractTransaction(ent, trans, caps, conn, priority);

    if (NS_SUCCEEDED(rv) && !trans->GetPendingTime().IsNull()) {
        if (trans->UsesPipelining())
            AccumulateTimeDelta(Telemetry::TRANSACTION_WAIT_TIME_HTTP_PIPELINES,
                trans->GetPendingTime(), TimeStamp::Now());
        else
            AccumulateTimeDelta(Telemetry::TRANSACTION_WAIT_TIME_HTTP,
                trans->GetPendingTime(), TimeStamp::Now());
        trans->SetPendingTime(false);
    }
    return rv;
}






nsresult
nsHttpConnectionMgr::DispatchAbstractTransaction(nsConnectionEntry *ent,
                                                 nsAHttpTransaction *aTrans,
                                                 uint32_t caps,
                                                 nsHttpConnection *conn,
                                                 int32_t priority)
{
    MOZ_ASSERT(!conn->UsingSpdy(),
               "Spdy Must Not Use DispatchAbstractTransaction");
    LOG(("nsHttpConnectionMgr::DispatchAbstractTransaction "
         "[ci=%s trans=%p caps=%x conn=%p]\n",
         ent->mConnInfo->HashKey().get(), aTrans, caps, conn));

    





    nsRefPtr<nsAHttpTransaction> transaction;
    nsresult rv;
    if (conn->Classification() != nsAHttpTransaction::CLASS_SOLO) {
        LOG(("   using pipeline datastructure.\n"));
        nsRefPtr<nsHttpPipeline> pipeline;
        rv = BuildPipeline(ent, aTrans, getter_AddRefs(pipeline));
        if (!NS_SUCCEEDED(rv))
            return rv;
        transaction = pipeline;
    }
    else {
        LOG(("   not using pipeline datastructure due to class solo.\n"));
        transaction = aTrans;
    }

    nsRefPtr<nsConnectionHandle> handle = new nsConnectionHandle(conn);

    
    transaction->SetConnection(handle);

    rv = conn->Activate(transaction, caps, priority);
    if (NS_FAILED(rv)) {
        LOG(("  conn->Activate failed [rv=%x]\n", rv));
        ent->mActiveConns.RemoveElement(conn);
        if (conn == ent->mYellowConnection)
            ent->OnYellowComplete();
        DecrementActiveConnCount(conn);
        ConditionallyStopTimeoutTick();

        
        
        transaction->SetConnection(nullptr);
        NS_RELEASE(handle->mConn);
        
        NS_RELEASE(conn);
    }

    
    
    
    

    return rv;
}

nsresult
nsHttpConnectionMgr::BuildPipeline(nsConnectionEntry *ent,
                                   nsAHttpTransaction *firstTrans,
                                   nsHttpPipeline **result)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    


    


    nsRefPtr<nsHttpPipeline> pipeline = new nsHttpPipeline();
    pipeline->AddTransaction(firstTrans);
    NS_ADDREF(*result = pipeline);
    return NS_OK;
}

void
nsHttpConnectionMgr::ReportProxyTelemetry(nsConnectionEntry *ent)
{
    enum { PROXY_NONE = 1, PROXY_HTTP = 2, PROXY_SOCKS = 3, PROXY_HTTPS = 4 };

    if (!ent->mConnInfo->UsingProxy())
        Telemetry::Accumulate(Telemetry::HTTP_PROXY_TYPE, PROXY_NONE);
    else if (ent->mConnInfo->UsingHttpsProxy())
        Telemetry::Accumulate(Telemetry::HTTP_PROXY_TYPE, PROXY_HTTPS);
    else if (ent->mConnInfo->UsingHttpProxy())
        Telemetry::Accumulate(Telemetry::HTTP_PROXY_TYPE, PROXY_HTTP);
    else
        Telemetry::Accumulate(Telemetry::HTTP_PROXY_TYPE, PROXY_SOCKS);
}

nsresult
nsHttpConnectionMgr::ProcessNewTransaction(nsHttpTransaction *trans)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    
    
    
    
    if (NS_FAILED(trans->Status())) {
        LOG(("  transaction was canceled... dropping event!\n"));
        return NS_OK;
    }

    trans->SetPendingTime();

    Http2PushedStream *pushedStream = trans->GetPushedStream();
    if (pushedStream) {
        return pushedStream->Session()->
            AddStream(trans, trans->Priority(), false, nullptr) ?
            NS_OK : NS_ERROR_UNEXPECTED;
    }

    nsresult rv = NS_OK;
    nsHttpConnectionInfo *ci = trans->ConnectionInfo();
    MOZ_ASSERT(ci);

    nsConnectionEntry *ent =
        GetOrCreateConnectionEntry(ci, !!trans->TunnelProvider());

    
    
    nsConnectionEntry *preferredEntry = GetSpdyPreferredEnt(ent);
    if (preferredEntry && (preferredEntry != ent)) {
        LOG(("nsHttpConnectionMgr::ProcessNewTransaction trans=%p "
             "redirected via coalescing from %s to %s\n", trans,
             ent->mConnInfo->Host(), preferredEntry->mConnInfo->Host()));

        ent = preferredEntry;
    }

    ReportProxyTelemetry(ent);

    
    
    

    nsAHttpConnection *wrappedConnection = trans->Connection();
    nsRefPtr<nsHttpConnection> conn;
    if (wrappedConnection)
        conn = dont_AddRef(wrappedConnection->TakeHttpConnection());

    if (conn) {
        MOZ_ASSERT(trans->Caps() & NS_HTTP_STICKY_CONNECTION);
        LOG(("nsHttpConnectionMgr::ProcessNewTransaction trans=%p "
             "sticky connection=%p\n", trans, conn.get()));

        if (static_cast<int32_t>(ent->mActiveConns.IndexOf(conn)) == -1) {
            LOG(("nsHttpConnectionMgr::ProcessNewTransaction trans=%p "
                 "sticky connection=%p needs to go on the active list\n", trans, conn.get()));

            
            
            MOZ_ASSERT(static_cast<int32_t>(ent->mIdleConns.IndexOf(conn)) == -1);
            MOZ_ASSERT(!conn->IsExperienced());

            AddActiveConn(conn, ent); 
        }

        trans->SetConnection(nullptr);
        rv = DispatchTransaction(ent, trans, conn);
    } else {
        rv = TryDispatchTransaction(ent, !!trans->TunnelProvider(), trans);
    }

    if (NS_SUCCEEDED(rv)) {
        LOG(("  ProcessNewTransaction Dispatch Immediately trans=%p\n", trans));
        return rv;
    }

    if (rv == NS_ERROR_NOT_AVAILABLE) {
        LOG(("  adding transaction to pending queue "
             "[trans=%p pending-count=%u]\n",
             trans, ent->mPendingQ.Length()+1));
        
        InsertTransactionSorted(ent->mPendingQ, trans);
        NS_ADDREF(trans);
        return NS_OK;
    }

    LOG(("  ProcessNewTransaction Hard Error trans=%p rv=%x\n", trans, rv));
    return rv;
}


void
nsHttpConnectionMgr::AddActiveConn(nsHttpConnection *conn,
                                   nsConnectionEntry *ent)
{
    NS_ADDREF(conn);
    ent->mActiveConns.AppendElement(conn);
    mNumActiveConns++;
    ActivateTimeoutTick();
}

void
nsHttpConnectionMgr::DecrementActiveConnCount(nsHttpConnection *conn)
{
    mNumActiveConns--;
    if (conn->EverUsedSpdy())
        mNumSpdyActiveConns--;
}

void
nsHttpConnectionMgr::StartedConnect()
{
    mNumActiveConns++;
    ActivateTimeoutTick(); 
}

void
nsHttpConnectionMgr::RecvdConnect()
{
    mNumActiveConns--;
    ConditionallyStopTimeoutTick();
}

nsresult
nsHttpConnectionMgr::CreateTransport(nsConnectionEntry *ent,
                                     nsAHttpTransaction *trans,
                                     uint32_t caps,
                                     bool speculative,
                                     bool isFromPredictor,
                                     bool allow1918)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    nsRefPtr<nsHalfOpenSocket> sock = new nsHalfOpenSocket(ent, trans, caps);
    if (speculative) {
        sock->SetSpeculative(true);
        sock->SetAllow1918(allow1918);
        Telemetry::AutoCounter<Telemetry::HTTPCONNMGR_TOTAL_SPECULATIVE_CONN> totalSpeculativeConn;
        ++totalSpeculativeConn;

        if (isFromPredictor) {
          sock->SetIsFromPredictor(true);
          Telemetry::AutoCounter<Telemetry::PREDICTOR_TOTAL_PRECONNECTS_CREATED> totalPreconnectsCreated;
          ++totalPreconnectsCreated;
        }
    }

    
    
    
    nsresult rv = sock->SetupPrimaryStreams();
    NS_ENSURE_SUCCESS(rv, rv);

    ent->mHalfOpens.AppendElement(sock);
    mNumHalfOpenConns++;
    return NS_OK;
}







void
nsHttpConnectionMgr::ProcessSpdyPendingQ(nsConnectionEntry *ent)
{
    nsHttpConnection *conn = GetSpdyPreferredConn(ent);
    if (!conn || !conn->CanDirectlyActivate())
        return;

    nsTArray<nsHttpTransaction*> leftovers;
    uint32_t index;

    
    for (index = 0;
         index < ent->mPendingQ.Length() && conn->CanDirectlyActivate();
         ++index) {
        nsHttpTransaction *trans = ent->mPendingQ[index];

        if (!(trans->Caps() & NS_HTTP_ALLOW_KEEPALIVE) ||
            trans->Caps() & NS_HTTP_DISALLOW_SPDY) {
            leftovers.AppendElement(trans);
            continue;
        }

        nsresult rv = DispatchTransaction(ent, trans, conn);
        if (NS_FAILED(rv)) {
            
            
            MOZ_ASSERT(false, "Dispatch SPDY Transaction");
            LOG(("ProcessSpdyPendingQ Dispatch Transaction failed trans=%p\n",
                    trans));
            trans->Close(rv);
        }
        NS_RELEASE(trans);
    }

    
    
    for (; index < ent->mPendingQ.Length(); ++index) {
        nsHttpTransaction *trans = ent->mPendingQ[index];
        leftovers.AppendElement(trans);
    }

    
    
    leftovers.SwapElements(ent->mPendingQ);
    leftovers.Clear();
}

PLDHashOperator
nsHttpConnectionMgr::ProcessSpdyPendingQCB(const nsACString &key,
                                           nsAutoPtr<nsConnectionEntry> &ent,
                                           void *closure)
{
    nsHttpConnectionMgr *self = (nsHttpConnectionMgr *) closure;
    self->ProcessSpdyPendingQ(ent);
    return PL_DHASH_NEXT;
}

void
nsHttpConnectionMgr::OnMsgProcessAllSpdyPendingQ(int32_t, void *)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    LOG(("nsHttpConnectionMgr::OnMsgProcessAllSpdyPendingQ\n"));
    mCT.Enumerate(ProcessSpdyPendingQCB, this);
}

nsHttpConnection *
nsHttpConnectionMgr::GetSpdyPreferredConn(nsConnectionEntry *ent)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    MOZ_ASSERT(ent);

    nsConnectionEntry *preferred = GetSpdyPreferredEnt(ent);

    
    if (preferred)
        
        ent->mUsingSpdy = true;
    else
        preferred = ent;

    nsHttpConnection *conn = nullptr;

    if (preferred->mUsingSpdy) {
        for (uint32_t index = 0;
             index < preferred->mActiveConns.Length();
             ++index) {
            if (preferred->mActiveConns[index]->CanDirectlyActivate()) {
                conn = preferred->mActiveConns[index];
                break;
            }
        }
    }

    return conn;
}



void
nsHttpConnectionMgr::OnMsgShutdown(int32_t, void *param)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    LOG(("nsHttpConnectionMgr::OnMsgShutdown\n"));

    mCT.Enumerate(ShutdownPassCB, this);

    if (mTimeoutTick) {
        mTimeoutTick->Cancel();
        mTimeoutTick = nullptr;
        mTimeoutTickArmed = false;
    }
    if (mTimer) {
      mTimer->Cancel();
      mTimer = nullptr;
    }
    if (mTrafficTimer) {
      mTrafficTimer->Cancel();
      mTrafficTimer = nullptr;
    }

    
    nsCOMPtr<nsIRunnable> runnable =
        new nsConnEvent(this, &nsHttpConnectionMgr::OnMsgShutdownConfirm,
                        0, param);
    NS_DispatchToMainThread(runnable);
}

void
nsHttpConnectionMgr::OnMsgShutdownConfirm(int32_t priority, void *param)
{
    MOZ_ASSERT(NS_IsMainThread());
    LOG(("nsHttpConnectionMgr::OnMsgShutdownConfirm\n"));

    bool *shutdown = static_cast<bool*>(param);
    *shutdown = true;
}

void
nsHttpConnectionMgr::OnMsgNewTransaction(int32_t priority, void *param)
{
    LOG(("nsHttpConnectionMgr::OnMsgNewTransaction [trans=%p]\n", param));

    nsHttpTransaction *trans = (nsHttpTransaction *) param;
    trans->SetPriority(priority);
    nsresult rv = ProcessNewTransaction(trans);
    if (NS_FAILED(rv))
        trans->Close(rv); 
    NS_RELEASE(trans);
}

void
nsHttpConnectionMgr::OnMsgReschedTransaction(int32_t priority, void *param)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    LOG(("nsHttpConnectionMgr::OnMsgReschedTransaction [trans=%p]\n", param));

    nsHttpTransaction *trans = (nsHttpTransaction *) param;
    trans->SetPriority(priority);

    nsConnectionEntry *ent = LookupConnectionEntry(trans->ConnectionInfo(),
                                                   nullptr, trans);

    if (ent) {
        int32_t index = ent->mPendingQ.IndexOf(trans);
        if (index >= 0) {
            ent->mPendingQ.RemoveElementAt(index);
            InsertTransactionSorted(ent->mPendingQ, trans);
        }
    }

    NS_RELEASE(trans);
}

void
nsHttpConnectionMgr::OnMsgCancelTransaction(int32_t reason, void *param)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    LOG(("nsHttpConnectionMgr::OnMsgCancelTransaction [trans=%p]\n", param));

    nsresult closeCode = static_cast<nsresult>(reason);
    nsRefPtr<nsHttpTransaction> trans =
        dont_AddRef(static_cast<nsHttpTransaction *>(param));

    
    
    
    
    
    nsRefPtr<nsAHttpConnection> conn(trans->Connection());
    if (conn && !trans->IsDone()) {
        conn->CloseTransaction(trans, closeCode);
    } else {
        nsConnectionEntry *ent =
            LookupConnectionEntry(trans->ConnectionInfo(), nullptr, trans);

        if (ent) {
            int32_t index = ent->mPendingQ.IndexOf(trans);
            if (index >= 0) {
                LOG(("nsHttpConnectionMgr::OnMsgCancelTransaction [trans=%p]"
                     " found in pending queue\n", trans.get()));
                ent->mPendingQ.RemoveElementAt(index);
                nsHttpTransaction *temp = trans;
                NS_RELEASE(temp); 
            }

            
            for (uint32_t index = 0;
                 index < ent->mHalfOpens.Length();
                 ++index) {
                nsHalfOpenSocket *half = ent->mHalfOpens[index];
                if (trans == half->Transaction()) {
                    half->Abandon();
                    
                    break;
                }
            }
        }

        trans->Close(closeCode);

        
        
        
        
        
        
        for (uint32_t index = 0;
             ent && (index < ent->mActiveConns.Length());
             ++index) {
            nsHttpConnection *activeConn = ent->mActiveConns[index];
            nsAHttpTransaction *liveTransaction = activeConn->Transaction();
            if (liveTransaction && liveTransaction->IsNullTransaction()) {
                LOG(("nsHttpConnectionMgr::OnMsgCancelTransaction [trans=%p] "
                     "also canceling Null Transaction %p on conn %p\n",
                     trans.get(), liveTransaction, activeConn));
                activeConn->CloseTransaction(liveTransaction, closeCode);
            }
        }
    }
}

void
nsHttpConnectionMgr::OnMsgProcessPendingQ(int32_t, void *param)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    nsHttpConnectionInfo *ci = (nsHttpConnectionInfo *) param;

    if (!ci) {
        LOG(("nsHttpConnectionMgr::OnMsgProcessPendingQ [ci=nullptr]\n"));
        
        mCT.Enumerate(ProcessAllTransactionsCB, this);
        return;
    }

    LOG(("nsHttpConnectionMgr::OnMsgProcessPendingQ [ci=%s]\n",
         ci->HashKey().get()));

    
    nsConnectionEntry *ent = mCT.Get(ci->HashKey());
    if (!(ent && ProcessPendingQForEntry(ent, false))) {
        
        
        mCT.Enumerate(ProcessOneTransactionCB, this);
    }

    NS_RELEASE(ci);
}

nsresult
nsHttpConnectionMgr::CancelTransactions(nsHttpConnectionInfo *aCI, nsresult code)
{
    nsRefPtr<nsHttpConnectionInfo> ci(aCI);
    LOG(("nsHttpConnectionMgr::CancelTransactions %s\n",ci->HashKey().get()));

    int32_t intReason = static_cast<int32_t>(code);
    nsresult rv = PostEvent(&nsHttpConnectionMgr::OnMsgCancelTransactions, intReason, ci);
    if (NS_SUCCEEDED(rv)) {
        unused << ci.forget();
    }
    return rv;
}

void
nsHttpConnectionMgr::OnMsgCancelTransactions(int32_t code, void *param)
{
    nsresult reason = static_cast<nsresult>(code);
    nsRefPtr<nsHttpConnectionInfo> ci =
        dont_AddRef(static_cast<nsHttpConnectionInfo *>(param));
    nsConnectionEntry *ent = mCT.Get(ci->HashKey());
    LOG(("nsHttpConnectionMgr::OnMsgCancelTransactions %s %p\n",
         ci->HashKey().get(), ent));
    if (!ent) {
        return;
    }

    nsRefPtr<nsHttpTransaction> trans;
    for (int32_t i = ent->mPendingQ.Length() - 1; i >= 0; --i) {
        trans = dont_AddRef(ent->mPendingQ[i]);
        LOG(("nsHttpConnectionMgr::OnMsgCancelTransactions %s %p %p\n",
             ci->HashKey().get(), ent, trans.get()));
        ent->mPendingQ.RemoveElementAt(i);
        trans->Close(reason);
        trans = nullptr;
    }
}

void
nsHttpConnectionMgr::OnMsgPruneDeadConnections(int32_t, void *)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    LOG(("nsHttpConnectionMgr::OnMsgPruneDeadConnections\n"));

    
    mTimeOfNextWakeUp = UINT64_MAX;

    
    
    if (mNumIdleConns || (mNumActiveConns && gHttpHandler->IsSpdyEnabled()))
        mCT.Enumerate(PruneDeadConnectionsCB, this);
}

void
nsHttpConnectionMgr::OnMsgPruneNoTraffic(int32_t, void *)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    LOG(("nsHttpConnectionMgr::OnMsgPruneNoTraffic\n"));

    
    mCT.Enumerate(PruneNoTrafficCB, this);

    mPruningNoTraffic = false; 
}

void
nsHttpConnectionMgr::OnMsgVerifyTraffic(int32_t, void *)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    LOG(("nsHttpConnectionMgr::OnMsgVerifyTraffic\n"));

    if (mPruningNoTraffic) {
      
      
      return;
    }

    
    mCT.Enumerate(VerifyTrafficCB, this);

    
    if(!mTrafficTimer) {
        mTrafficTimer = do_CreateInstance("@mozilla.org/timer;1");
    }

    
    
    if (mTrafficTimer) {
        
        
        mTrafficTimer->Init(this, gHttpHandler->NetworkChangedTimeout(),
                            nsITimer::TYPE_ONE_SHOT);
    } else {
        NS_WARNING("failed to create timer for VerifyTraffic!");
    }
}

void
nsHttpConnectionMgr::OnMsgDoShiftReloadConnectionCleanup(int32_t, void *param)
{
    LOG(("nsHttpConnectionMgr::OnMsgDoShiftReloadConnectionCleanup\n"));
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    nsRefPtr<nsHttpConnectionInfo> ci =
        dont_AddRef(static_cast<nsHttpConnectionInfo *>(param));

    mCT.Enumerate(ClosePersistentConnectionsCB, this);
    if (ci)
        ResetIPFamilyPreference(ci);
}

void
nsHttpConnectionMgr::OnMsgReclaimConnection(int32_t, void *param)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    LOG(("nsHttpConnectionMgr::OnMsgReclaimConnection [conn=%p]\n", param));

    nsHttpConnection *conn = (nsHttpConnection *) param;

    
    
    
    
    

    nsConnectionEntry *ent = LookupConnectionEntry(conn->ConnectionInfo(),
                                                   conn, nullptr);
    if (!ent) {
        
        
        
        ent = GetOrCreateConnectionEntry(conn->ConnectionInfo(), true);
        LOG(("nsHttpConnectionMgr::OnMsgReclaimConnection conn %p "
             "forced new hash entry %s\n",
             conn, conn->ConnectionInfo()->HashKey().get()));
    }

    MOZ_ASSERT(ent);
    nsHttpConnectionInfo *ci = nullptr;
    NS_ADDREF(ci = ent->mConnInfo);

    
    
    
    

    if (conn->EverUsedSpdy()) {
        
        
        
        
        
        
        conn->DontReuse();
    }

    
    
    
    if (conn->Transaction()) {
        conn->DontReuse();
    }

    if (ent->mActiveConns.RemoveElement(conn)) {
        if (conn == ent->mYellowConnection) {
            ent->OnYellowComplete();
        }
        nsHttpConnection *temp = conn;
        NS_RELEASE(temp);
        DecrementActiveConnCount(conn);
        ConditionallyStopTimeoutTick();
    }

    if (conn->CanReuse()) {
        LOG(("  adding connection to idle list\n"));
        
        
        

        
        

        uint32_t idx;
        for (idx = 0; idx < ent->mIdleConns.Length(); idx++) {
            nsHttpConnection *idleConn = ent->mIdleConns[idx];
            if (idleConn->MaxBytesRead() < conn->MaxBytesRead())
                break;
        }

        NS_ADDREF(conn);
        ent->mIdleConns.InsertElementAt(idx, conn);
        mNumIdleConns++;
        conn->BeginIdleMonitoring();

        
        
        
        uint32_t timeToLive = conn->TimeToLive();
        if(!mTimer || NowInSeconds() + timeToLive < mTimeOfNextWakeUp)
            PruneDeadConnectionsAfter(timeToLive);
    } else {
        LOG(("  connection cannot be reused; closing connection\n"));
        conn->Close(NS_ERROR_ABORT);
    }

    OnMsgProcessPendingQ(0, ci); 
    NS_RELEASE(conn);
}

void
nsHttpConnectionMgr::OnMsgCompleteUpgrade(int32_t, void *param)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    nsCompleteUpgradeData *data = (nsCompleteUpgradeData *) param;
    LOG(("nsHttpConnectionMgr::OnMsgCompleteUpgrade "
         "this=%p conn=%p listener=%p\n", this, data->mConn.get(),
         data->mUpgradeListener.get()));

    nsCOMPtr<nsISocketTransport> socketTransport;
    nsCOMPtr<nsIAsyncInputStream> socketIn;
    nsCOMPtr<nsIAsyncOutputStream> socketOut;

    nsresult rv;
    rv = data->mConn->TakeTransport(getter_AddRefs(socketTransport),
                                    getter_AddRefs(socketIn),
                                    getter_AddRefs(socketOut));

    if (NS_SUCCEEDED(rv))
        data->mUpgradeListener->OnTransportAvailable(socketTransport,
                                                     socketIn,
                                                     socketOut);
    delete data;
}

void
nsHttpConnectionMgr::OnMsgUpdateParam(int32_t, void *param)
{
    uint16_t name  = (NS_PTR_TO_INT32(param) & 0xFFFF0000) >> 16;
    uint16_t value =  NS_PTR_TO_INT32(param) & 0x0000FFFF;

    switch (name) {
    case MAX_CONNECTIONS:
        mMaxConns = value;
        break;
    case MAX_PERSISTENT_CONNECTIONS_PER_HOST:
        mMaxPersistConnsPerHost = value;
        break;
    case MAX_PERSISTENT_CONNECTIONS_PER_PROXY:
        mMaxPersistConnsPerProxy = value;
        break;
    case MAX_REQUEST_DELAY:
        mMaxRequestDelay = value;
        break;
    case MAX_PIPELINED_REQUESTS:
        mMaxPipelinedRequests = value;
        break;
    case MAX_OPTIMISTIC_PIPELINED_REQUESTS:
        mMaxOptimisticPipelinedRequests = value;
        break;
    default:
        NS_NOTREACHED("unexpected parameter name");
    }
}


nsHttpConnectionMgr::nsConnectionEntry::~nsConnectionEntry()
{
    MOZ_COUNT_DTOR(nsConnectionEntry);
    gHttpHandler->ConnMgr()->RemovePreferredHash(this);
}

void
nsHttpConnectionMgr::OnMsgProcessFeedback(int32_t, void *param)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    nsHttpPipelineFeedback *fb = (nsHttpPipelineFeedback *)param;

    PipelineFeedbackInfo(fb->mConnInfo, fb->mInfo, fb->mConn, fb->mData);
    delete fb;
}



void
nsHttpConnectionMgr::ActivateTimeoutTick()
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    LOG(("nsHttpConnectionMgr::ActivateTimeoutTick() "
         "this=%p mTimeoutTick=%p\n", this, mTimeoutTick.get()));

    
    
    

    if (mTimeoutTick && mTimeoutTickArmed) {
        
        if (mTimeoutTickNext > 1) {
            mTimeoutTickNext = 1;
            mTimeoutTick->SetDelay(1000);
        }
        return;
    }

    if (!mTimeoutTick) {
        mTimeoutTick = do_CreateInstance(NS_TIMER_CONTRACTID);
        if (!mTimeoutTick) {
            NS_WARNING("failed to create timer for http timeout management");
            return;
        }
        mTimeoutTick->SetTarget(mSocketThreadTarget);
    }

    MOZ_ASSERT(!mTimeoutTickArmed, "timer tick armed");
    mTimeoutTickArmed = true;
    mTimeoutTick->Init(this, 1000, nsITimer::TYPE_REPEATING_SLACK);
}

void
nsHttpConnectionMgr::TimeoutTick()
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    MOZ_ASSERT(mTimeoutTick, "no readtimeout tick");

    LOG(("nsHttpConnectionMgr::TimeoutTick active=%d\n", mNumActiveConns));
    
    
    
    mTimeoutTickNext = 3600; 
    mCT.Enumerate(TimeoutTickCB, this);
    if (mTimeoutTick) {
        mTimeoutTickNext = std::max(mTimeoutTickNext, 1U);
        mTimeoutTick->SetDelay(mTimeoutTickNext * 1000);
    }
}

PLDHashOperator
nsHttpConnectionMgr::TimeoutTickCB(const nsACString &key,
                                       nsAutoPtr<nsConnectionEntry> &ent,
                                       void *closure)
{
    nsHttpConnectionMgr *self = (nsHttpConnectionMgr *) closure;

    LOG(("nsHttpConnectionMgr::TimeoutTickCB() this=%p host=%s "
         "idle=%d active=%d half-len=%d pending=%d\n",
         self, ent->mConnInfo->Host(), ent->mIdleConns.Length(),
         ent->mActiveConns.Length(), ent->mHalfOpens.Length(),
         ent->mPendingQ.Length()));

    
    PRIntervalTime now = PR_IntervalNow();
    for (uint32_t index = 0; index < ent->mActiveConns.Length(); ++index) {
        uint32_t connNextTimeout =  ent->mActiveConns[index]->ReadTimeoutTick(now);
        self->mTimeoutTickNext = std::min(self->mTimeoutTickNext, connNextTimeout);
    }

    
    if (ent->mHalfOpens.Length()) {
        TimeStamp now = TimeStamp::Now();
        double maxConnectTime = gHttpHandler->ConnectTimeout();  

        for (uint32_t index = ent->mHalfOpens.Length(); index > 0; ) {
            index--;

            nsHalfOpenSocket *half = ent->mHalfOpens[index];
            double delta = half->Duration(now);
            
            
            if (delta > maxConnectTime) {
                LOG(("Force timeout of half open to %s after %.2fms.\n",
                     ent->mConnInfo->HashKey().get(), delta));
                if (half->SocketTransport())
                    half->SocketTransport()->Close(NS_ERROR_ABORT);
                if (half->BackupTransport())
                    half->BackupTransport()->Close(NS_ERROR_ABORT);
            }

            
            
            if (delta > maxConnectTime + 5000) {
                LOG(("Abandon half open to %s after %.2fms.\n",
                     ent->mConnInfo->HashKey().get(), delta));
                half->Abandon();
            }
        }
    }
    if (ent->mHalfOpens.Length()) {
        self->mTimeoutTickNext = 1;
    }
    return PL_DHASH_NEXT;
}




nsHttpConnectionMgr::nsConnectionHandle::~nsConnectionHandle()
{
    if (mConn) {
        gHttpHandler->ReclaimConnection(mConn);
        NS_RELEASE(mConn);
    }
}

NS_IMPL_ISUPPORTS0(nsHttpConnectionMgr::nsConnectionHandle)







nsHttpConnectionMgr::nsConnectionEntry *
nsHttpConnectionMgr::GetOrCreateConnectionEntry(nsHttpConnectionInfo *specificCI,
                                                bool prohibitWildCard)
{
    
    nsConnectionEntry *specificEnt = mCT.Get(specificCI->HashKey());
    if (specificEnt && specificEnt->AvailableForDispatchNow()) {
        return specificEnt;
    }

    if (!specificCI->UsingHttpsProxy()) {
        prohibitWildCard = true;
    }

    
    if (!prohibitWildCard) {
        nsRefPtr<nsHttpConnectionInfo> wildCardProxyCI;
        specificCI->CreateWildCard(getter_AddRefs(wildCardProxyCI));
        nsConnectionEntry *wildCardEnt = mCT.Get(wildCardProxyCI->HashKey());
        if (wildCardEnt && wildCardEnt->AvailableForDispatchNow()) {
            return wildCardEnt;
        }
    }

    
    if (!specificEnt) {
        nsRefPtr<nsHttpConnectionInfo> clone(specificCI->Clone());
        specificEnt = new nsConnectionEntry(clone);
        mCT.Put(clone->HashKey(), specificEnt);
    }
    return specificEnt;
}

nsresult
nsHttpConnectionMgr::nsConnectionHandle::OnHeadersAvailable(nsAHttpTransaction *trans,
                                                            nsHttpRequestHead *req,
                                                            nsHttpResponseHead *resp,
                                                            bool *reset)
{
    return mConn->OnHeadersAvailable(trans, req, resp, reset);
}

void
nsHttpConnectionMgr::nsConnectionHandle::CloseTransaction(nsAHttpTransaction *trans, nsresult reason)
{
    mConn->CloseTransaction(trans, reason);
}

nsresult
nsHttpConnectionMgr::
nsConnectionHandle::TakeTransport(nsISocketTransport  **aTransport,
                                  nsIAsyncInputStream **aInputStream,
                                  nsIAsyncOutputStream **aOutputStream)
{
    return mConn->TakeTransport(aTransport, aInputStream, aOutputStream);
}

void
nsHttpConnectionMgr::OnMsgSpeculativeConnect(int32_t, void *param)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    nsRefPtr<SpeculativeConnectArgs> args =
        dont_AddRef(static_cast<SpeculativeConnectArgs *>(param));

    LOG(("nsHttpConnectionMgr::OnMsgSpeculativeConnect [ci=%s]\n",
         args->mTrans->ConnectionInfo()->HashKey().get()));

    nsConnectionEntry *ent =
        GetOrCreateConnectionEntry(args->mTrans->ConnectionInfo(), false);

    
    
    
    nsConnectionEntry *preferredEntry = GetSpdyPreferredEnt(ent);
    if (preferredEntry)
        ent = preferredEntry;

    uint32_t parallelSpeculativeConnectLimit =
        gHttpHandler->ParallelSpeculativeConnectLimit();
    bool ignorePossibleSpdyConnections = false;
    bool ignoreIdle = false;
    bool isFromPredictor = false;
    bool allow1918 = false;

    if (args->mOverridesOK) {
        parallelSpeculativeConnectLimit = args->mParallelSpeculativeConnectLimit;
        ignorePossibleSpdyConnections = args->mIgnorePossibleSpdyConnections;
        ignoreIdle = args->mIgnoreIdle;
        isFromPredictor = args->mIsFromPredictor;
        allow1918 = args->mAllow1918;
    }

    bool keepAlive = args->mTrans->Caps() & NS_HTTP_ALLOW_KEEPALIVE;
    if (mNumHalfOpenConns < parallelSpeculativeConnectLimit &&
        ((ignoreIdle && (ent->mIdleConns.Length() < parallelSpeculativeConnectLimit)) ||
         !ent->mIdleConns.Length()) &&
        !(keepAlive && RestrictConnections(ent, ignorePossibleSpdyConnections)) &&
        !AtActiveConnectionLimit(ent, args->mTrans->Caps())) {
        CreateTransport(ent, args->mTrans, args->mTrans->Caps(), true, isFromPredictor, allow1918);
    }
    else {
        LOG(("  Transport not created due to existing connection count\n"));
    }
}

bool
nsHttpConnectionMgr::nsConnectionHandle::IsPersistent()
{
    return mConn->IsPersistent();
}

bool
nsHttpConnectionMgr::nsConnectionHandle::IsReused()
{
    return mConn->IsReused();
}

void
nsHttpConnectionMgr::nsConnectionHandle::DontReuse()
{
    mConn->DontReuse();
}

nsresult
nsHttpConnectionMgr::nsConnectionHandle::PushBack(const char *buf, uint32_t bufLen)
{
    return mConn->PushBack(buf, bufLen);
}




NS_IMPL_ISUPPORTS(nsHttpConnectionMgr::nsHalfOpenSocket,
                  nsIOutputStreamCallback,
                  nsITransportEventSink,
                  nsIInterfaceRequestor,
                  nsITimerCallback)

nsHttpConnectionMgr::
nsHalfOpenSocket::nsHalfOpenSocket(nsConnectionEntry *ent,
                                   nsAHttpTransaction *trans,
                                   uint32_t caps)
    : mEnt(ent)
    , mTransaction(trans)
    , mDispatchedMTransaction(false)
    , mCaps(caps)
    , mSpeculative(false)
    , mIsFromPredictor(false)
    , mAllow1918(true)
    , mHasConnected(false)
    , mPrimaryConnectedOK(false)
    , mBackupConnectedOK(false)
{
    MOZ_ASSERT(ent && trans, "constructor with null arguments");
    LOG(("Creating nsHalfOpenSocket [this=%p trans=%p ent=%s key=%s]\n",
         this, trans, ent->mConnInfo->Host(), ent->mConnInfo->HashKey().get()));
}

nsHttpConnectionMgr::nsHalfOpenSocket::~nsHalfOpenSocket()
{
    MOZ_ASSERT(!mStreamOut);
    MOZ_ASSERT(!mBackupStreamOut);
    MOZ_ASSERT(!mSynTimer);
    LOG(("Destroying nsHalfOpenSocket [this=%p]\n", this));

    if (mEnt)
        mEnt->RemoveHalfOpen(this);
}

nsresult
nsHttpConnectionMgr::
nsHalfOpenSocket::SetupStreams(nsISocketTransport **transport,
                               nsIAsyncInputStream **instream,
                               nsIAsyncOutputStream **outstream,
                               bool isBackup)
{
    nsresult rv;
    const char *socketTypes[1];
    uint32_t typeCount = 0;
    if (mEnt->mConnInfo->FirstHopSSL()) {
        socketTypes[typeCount++] = "ssl";
    } else {
        socketTypes[typeCount] = gHttpHandler->DefaultSocketType();
        if (socketTypes[typeCount]) {
            typeCount++;
        }
    }

    nsCOMPtr<nsISocketTransport> socketTransport;
    nsCOMPtr<nsISocketTransportService> sts;

    sts = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = sts->CreateTransport(socketTypes, typeCount,
                              nsDependentCString(mEnt->mConnInfo->Host()),
                              mEnt->mConnInfo->Port(),
                              mEnt->mConnInfo->ProxyInfo(),
                              getter_AddRefs(socketTransport));
    NS_ENSURE_SUCCESS(rv, rv);

    uint32_t tmpFlags = 0;
    if (mCaps & NS_HTTP_REFRESH_DNS)
        tmpFlags = nsISocketTransport::BYPASS_CACHE;

    if (mCaps & NS_HTTP_LOAD_ANONYMOUS)
        tmpFlags |= nsISocketTransport::ANONYMOUS_CONNECT;

    if (mEnt->mConnInfo->GetPrivate())
        tmpFlags |= nsISocketTransport::NO_PERMANENT_STORAGE;

    
    
    
    
    
    if (mEnt->mPreferIPv6) {
        tmpFlags |= nsISocketTransport::DISABLE_IPV4;
    }
    else if (mEnt->mPreferIPv4 ||
             (isBackup && gHttpHandler->FastFallbackToIPv4())) {
        tmpFlags |= nsISocketTransport::DISABLE_IPV6;
    }

    if (!Allow1918()) {
        tmpFlags |= nsISocketTransport::DISABLE_RFC1918;
    }

    socketTransport->SetConnectionFlags(tmpFlags);

    socketTransport->SetQoSBits(gHttpHandler->GetQoSBits());

    if (!mEnt->mConnInfo->GetNetworkInterfaceId().IsEmpty()) {
        socketTransport->SetNetworkInterfaceId(mEnt->mConnInfo->GetNetworkInterfaceId());
    }

    rv = socketTransport->SetEventSink(this, nullptr);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = socketTransport->SetSecurityCallbacks(this);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIOutputStream> sout;
    rv = socketTransport->OpenOutputStream(nsITransport::OPEN_UNBUFFERED,
                                            0, 0,
                                            getter_AddRefs(sout));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIInputStream> sin;
    rv = socketTransport->OpenInputStream(nsITransport::OPEN_UNBUFFERED,
                                           0, 0,
                                           getter_AddRefs(sin));
    NS_ENSURE_SUCCESS(rv, rv);

    socketTransport.forget(transport);
    CallQueryInterface(sin, instream);
    CallQueryInterface(sout, outstream);

    rv = (*outstream)->AsyncWait(this, 0, 0, nullptr);
    if (NS_SUCCEEDED(rv))
        gHttpHandler->ConnMgr()->StartedConnect();

    return rv;
}

nsresult
nsHttpConnectionMgr::nsHalfOpenSocket::SetupPrimaryStreams()
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    nsresult rv;

    mPrimarySynStarted = TimeStamp::Now();
    rv = SetupStreams(getter_AddRefs(mSocketTransport),
                      getter_AddRefs(mStreamIn),
                      getter_AddRefs(mStreamOut),
                      false);
    LOG(("nsHalfOpenSocket::SetupPrimaryStream [this=%p ent=%s rv=%x]",
         this, mEnt->mConnInfo->Host(), rv));
    if (NS_FAILED(rv)) {
        if (mStreamOut)
            mStreamOut->AsyncWait(nullptr, 0, 0, nullptr);
        mStreamOut = nullptr;
        mStreamIn = nullptr;
        mSocketTransport = nullptr;
    }
    return rv;
}

nsresult
nsHttpConnectionMgr::nsHalfOpenSocket::SetupBackupStreams()
{
    MOZ_ASSERT(mTransaction);

    mBackupSynStarted = TimeStamp::Now();
    nsresult rv = SetupStreams(getter_AddRefs(mBackupTransport),
                               getter_AddRefs(mBackupStreamIn),
                               getter_AddRefs(mBackupStreamOut),
                               true);
    LOG(("nsHalfOpenSocket::SetupBackupStream [this=%p ent=%s rv=%x]",
         this, mEnt->mConnInfo->Host(), rv));
    if (NS_FAILED(rv)) {
        if (mBackupStreamOut)
            mBackupStreamOut->AsyncWait(nullptr, 0, 0, nullptr);
        mBackupStreamOut = nullptr;
        mBackupStreamIn = nullptr;
        mBackupTransport = nullptr;
    }
    return rv;
}

void
nsHttpConnectionMgr::nsHalfOpenSocket::SetupBackupTimer()
{
    uint16_t timeout = gHttpHandler->GetIdleSynTimeout();
    MOZ_ASSERT(!mSynTimer, "timer already initd");
    if (timeout && !mTransaction->IsDone()) {
        
        
        
        
        
        
        
        nsresult rv;
        mSynTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
        if (NS_SUCCEEDED(rv)) {
            mSynTimer->InitWithCallback(this, timeout, nsITimer::TYPE_ONE_SHOT);
            LOG(("nsHalfOpenSocket::SetupBackupTimer() [this=%p]", this));
        }
    }
    else if (timeout) {
        LOG(("nsHalfOpenSocket::SetupBackupTimer() [this=%p],"
             " transaction already done!", this));
    }
}

void
nsHttpConnectionMgr::nsHalfOpenSocket::CancelBackupTimer()
{
    
    
    if (!mSynTimer)
        return;

    LOG(("nsHalfOpenSocket::CancelBackupTimer()"));
    mSynTimer->Cancel();
    mSynTimer = nullptr;
}

void
nsHttpConnectionMgr::nsHalfOpenSocket::Abandon()
{
    LOG(("nsHalfOpenSocket::Abandon [this=%p ent=%s]",
         this, mEnt->mConnInfo->Host()));

    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    nsRefPtr<nsHalfOpenSocket> deleteProtector(this);

    
    if (mSocketTransport) {
        mSocketTransport->SetEventSink(nullptr, nullptr);
        mSocketTransport->SetSecurityCallbacks(nullptr);
        mSocketTransport = nullptr;
    }
    if (mBackupTransport) {
        mBackupTransport->SetEventSink(nullptr, nullptr);
        mBackupTransport->SetSecurityCallbacks(nullptr);
        mBackupTransport = nullptr;
    }

    
    if (mStreamOut) {
        gHttpHandler->ConnMgr()->RecvdConnect();
        mStreamOut->AsyncWait(nullptr, 0, 0, nullptr);
        mStreamOut = nullptr;
    }
    if (mBackupStreamOut) {
        gHttpHandler->ConnMgr()->RecvdConnect();
        mBackupStreamOut->AsyncWait(nullptr, 0, 0, nullptr);
        mBackupStreamOut = nullptr;
    }

    
    mStreamIn = mBackupStreamIn = nullptr;

    
    CancelBackupTimer();

    
    if (mEnt)
        mEnt->RemoveHalfOpen(this);
    mEnt = nullptr;
}

double
nsHttpConnectionMgr::nsHalfOpenSocket::Duration(TimeStamp epoch)
{
    if (mPrimarySynStarted.IsNull())
        return 0;

    return (epoch - mPrimarySynStarted).ToMilliseconds();
}


NS_IMETHODIMP 
nsHttpConnectionMgr::nsHalfOpenSocket::Notify(nsITimer *timer)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    MOZ_ASSERT(timer == mSynTimer, "wrong timer");

    SetupBackupStreams();

    mSynTimer = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsHttpConnectionMgr::
nsHalfOpenSocket::OnOutputStreamReady(nsIAsyncOutputStream *out)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    MOZ_ASSERT(out == mStreamOut || out == mBackupStreamOut,
               "stream mismatch");
    LOG(("nsHalfOpenSocket::OnOutputStreamReady [this=%p ent=%s %s]\n",
         this, mEnt->mConnInfo->Host(),
         out == mStreamOut ? "primary" : "backup"));
    int32_t index;
    nsresult rv;

    gHttpHandler->ConnMgr()->RecvdConnect();

    CancelBackupTimer();

    
    nsRefPtr<nsHttpConnection> conn = new nsHttpConnection();
    LOG(("nsHalfOpenSocket::OnOutputStreamReady "
         "Created new nshttpconnection %p\n", conn.get()));

    
    
    conn->SetTransactionCaps(mTransaction->Caps());

    NetAddr peeraddr;
    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    mTransaction->GetSecurityCallbacks(getter_AddRefs(callbacks));
    if (out == mStreamOut) {
        TimeDuration rtt = TimeStamp::Now() - mPrimarySynStarted;
        rv = conn->Init(mEnt->mConnInfo,
                        gHttpHandler->ConnMgr()->mMaxRequestDelay,
                        mSocketTransport, mStreamIn, mStreamOut,
                        mPrimaryConnectedOK, callbacks,
                        PR_MillisecondsToInterval(
                          static_cast<uint32_t>(rtt.ToMilliseconds())));

        if (NS_SUCCEEDED(mSocketTransport->GetPeerAddr(&peeraddr)))
            mEnt->RecordIPFamilyPreference(peeraddr.raw.family);

        
        mStreamOut = nullptr;
        mStreamIn = nullptr;
        mSocketTransport = nullptr;
    }
    else {
        TimeDuration rtt = TimeStamp::Now() - mBackupSynStarted;
        rv = conn->Init(mEnt->mConnInfo,
                        gHttpHandler->ConnMgr()->mMaxRequestDelay,
                        mBackupTransport, mBackupStreamIn, mBackupStreamOut,
                        mBackupConnectedOK, callbacks,
                        PR_MillisecondsToInterval(
                          static_cast<uint32_t>(rtt.ToMilliseconds())));

        if (NS_SUCCEEDED(mBackupTransport->GetPeerAddr(&peeraddr)))
            mEnt->RecordIPFamilyPreference(peeraddr.raw.family);

        
        mBackupStreamOut = nullptr;
        mBackupStreamIn = nullptr;
        mBackupTransport = nullptr;
    }

    if (NS_FAILED(rv)) {
        LOG(("nsHalfOpenSocket::OnOutputStreamReady "
             "conn->init (%p) failed %x\n", conn.get(), rv));
        return rv;
    }

    
    
    mHasConnected = true;

    
    index = mEnt->mPendingQ.IndexOf(mTransaction);
    if (index != -1) {
        MOZ_ASSERT(!mSpeculative,
                   "Speculative Half Open found mTransaction");
        nsRefPtr<nsHttpTransaction> temp = dont_AddRef(mEnt->mPendingQ[index]);
        mEnt->mPendingQ.RemoveElementAt(index);
        gHttpHandler->ConnMgr()->AddActiveConn(conn, mEnt);
        rv = gHttpHandler->ConnMgr()->DispatchTransaction(mEnt, temp, conn);
    } else {
        
        

        
        
        
        conn->SetIsReusedAfter(950);

        
        
        
        
        
        if (mEnt->mConnInfo->FirstHopSSL() && !mEnt->mPendingQ.Length() &&
            !mEnt->mConnInfo->UsingConnect()) {
            LOG(("nsHalfOpenSocket::OnOutputStreamReady null transaction will "
                 "be used to finish SSL handshake on conn %p\n", conn.get()));
            nsRefPtr<nsAHttpTransaction> trans;
            if (mTransaction->IsNullTransaction() && !mDispatchedMTransaction) {
                
                
                mDispatchedMTransaction = true;
                trans = mTransaction;
            } else {
                trans = new NullHttpTransaction(mEnt->mConnInfo,
                                                callbacks,
                                                mCaps & ~NS_HTTP_ALLOW_PIPELINING);
            }

            gHttpHandler->ConnMgr()->AddActiveConn(conn, mEnt);
            conn->Classify(nsAHttpTransaction::CLASS_SOLO);
            rv = gHttpHandler->ConnMgr()->
                DispatchAbstractTransaction(mEnt, trans, mCaps, conn, 0);
        } else {
            
            LOG(("nsHalfOpenSocket::OnOutputStreamReady no transaction match "
                 "returning conn %p to pool\n", conn.get()));
            nsRefPtr<nsHttpConnection> copy(conn);
            
            gHttpHandler->ConnMgr()->OnMsgReclaimConnection(
                0, conn.forget().take());
        }
    }

    return rv;
}


NS_IMETHODIMP
nsHttpConnectionMgr::nsHalfOpenSocket::OnTransportStatus(nsITransport *trans,
                                                         nsresult status,
                                                         int64_t progress,
                                                         int64_t progressMax)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    if (mTransaction)
        mTransaction->OnTransportStatus(trans, status, progress);

    MOZ_ASSERT(trans == mSocketTransport || trans == mBackupTransport);
    if (status == NS_NET_STATUS_CONNECTED_TO) {
        if (trans == mSocketTransport) {
            mPrimaryConnectedOK = true;
        } else {
            mBackupConnectedOK = true;
        }
    }

    
    if (trans != mSocketTransport) {
        return NS_OK;
    }

    
    
    
    

    if (status == NS_NET_STATUS_CONNECTING_TO &&
        gHttpHandler->IsSpdyEnabled() &&
        gHttpHandler->CoalesceSpdy() &&
        mEnt && mEnt->mConnInfo && mEnt->mConnInfo->EndToEndSSL() &&
        !mEnt->mConnInfo->UsingProxy() &&
        mEnt->mCoalescingKeys.IsEmpty()) {

        nsCOMPtr<nsIDNSRecord> dnsRecord(do_GetInterface(mSocketTransport));
        nsTArray<NetAddr> addressSet;
        nsresult rv = NS_ERROR_NOT_AVAILABLE;
        if (dnsRecord) {
            rv = dnsRecord->GetAddresses(addressSet);
        }

        if (NS_SUCCEEDED(rv) && !addressSet.IsEmpty()) {
            for (uint32_t i = 0; i < addressSet.Length(); ++i) {
                nsCString *newKey = mEnt->mCoalescingKeys.AppendElement(nsCString());
                newKey->SetCapacity(kIPv6CStrBufSize + 26);
                NetAddrToString(&addressSet[i], newKey->BeginWriting(), kIPv6CStrBufSize);
                newKey->SetLength(strlen(newKey->BeginReading()));
                if (mEnt->mConnInfo->GetAnonymous()) {
                    newKey->AppendLiteral("~A:");
                } else {
                    newKey->AppendLiteral("~.:");
                }
                newKey->AppendInt(mEnt->mConnInfo->Port());
                LOG(("nsHttpConnectionMgr::nsHalfOpenSocket::OnTransportStatus "
                     "STATUS_CONNECTING_TO Established New Coalescing Key # %d for host "
                     "%s [%s]", i, mEnt->mConnInfo->Host(), newKey->get()));
            }
            gHttpHandler->ConnMgr()->ProcessSpdyPendingQ(mEnt);
        }
    }

    switch (status) {
    case NS_NET_STATUS_CONNECTING_TO:
        
        
        
        
        
        
        
        
        if (mEnt && !mBackupTransport && !mSynTimer)
            SetupBackupTimer();
        break;

    case NS_NET_STATUS_CONNECTED_TO:
        
        
        CancelBackupTimer();
        break;

    default:
        break;
    }

    return NS_OK;
}


NS_IMETHODIMP
nsHttpConnectionMgr::nsHalfOpenSocket::GetInterface(const nsIID &iid,
                                                    void **result)
{
    if (mTransaction) {
        nsCOMPtr<nsIInterfaceRequestor> callbacks;
        mTransaction->GetSecurityCallbacks(getter_AddRefs(callbacks));
        if (callbacks)
            return callbacks->GetInterface(iid, result);
    }
    return NS_ERROR_NO_INTERFACE;
}


nsHttpConnection *
nsHttpConnectionMgr::nsConnectionHandle::TakeHttpConnection()
{
    
    

    MOZ_ASSERT(mConn);
    nsHttpConnection *conn = mConn;
    mConn = nullptr;
    return conn;
}

uint32_t
nsHttpConnectionMgr::nsConnectionHandle::CancelPipeline(nsresult reason)
{
    
    return 0;
}

nsAHttpTransaction::Classifier
nsHttpConnectionMgr::nsConnectionHandle::Classification()
{
    if (mConn)
        return mConn->Classification();

    LOG(("nsConnectionHandle::Classification this=%p "
         "has null mConn using CLASS_SOLO default", this));
    return nsAHttpTransaction::CLASS_SOLO;
}



nsHttpConnectionMgr::
nsConnectionEntry::nsConnectionEntry(nsHttpConnectionInfo *ci)
    : mConnInfo(ci)
    , mPipelineState(PS_YELLOW)
    , mYellowGoodEvents(0)
    , mYellowBadEvents(0)
    , mYellowConnection(nullptr)
    , mGreenDepth(kPipelineOpen)
    , mPipeliningPenalty(0)
    , mSpdyCWND(0)
    , mUsingSpdy(false)
    , mTestedSpdy(false)
    , mInPreferredHash(false)
    , mPreferIPv4(false)
    , mPreferIPv6(false)
{
    MOZ_COUNT_CTOR(nsConnectionEntry);
    if (gHttpHandler->GetPipelineAggressive()) {
        mGreenDepth = kPipelineUnlimited;
        mPipelineState = PS_GREEN;
    }
    mInitialGreenDepth = mGreenDepth;
    memset(mPipeliningClassPenalty, 0, sizeof(int16_t) * nsAHttpTransaction::CLASS_MAX);
}

bool
nsHttpConnectionMgr::nsConnectionEntry::AvailableForDispatchNow()
{
    if (mIdleConns.Length() && mIdleConns[0]->CanReuse()) {
        return true;
    }

    return gHttpHandler->ConnMgr()->
        GetSpdyPreferredConn(this) ? true : false;
}

bool
nsHttpConnectionMgr::nsConnectionEntry::SupportsPipelining()
{
    return mPipelineState != nsHttpConnectionMgr::PS_RED;
}

nsHttpConnectionMgr::PipeliningState
nsHttpConnectionMgr::nsConnectionEntry::PipelineState()
{
    return mPipelineState;
}

void
nsHttpConnectionMgr::
nsConnectionEntry::OnPipelineFeedbackInfo(
    nsHttpConnectionMgr::PipelineFeedbackInfoType info,
    nsHttpConnection *conn,
    uint32_t data)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    if (mPipelineState == PS_YELLOW) {
        if (info & kPipelineInfoTypeBad)
            mYellowBadEvents++;
        else if (info & (kPipelineInfoTypeNeutral | kPipelineInfoTypeGood))
            mYellowGoodEvents++;
    }

    if (mPipelineState == PS_GREEN && info == GoodCompletedOK) {
        int32_t depth = data;
        LOG(("Transaction completed at pipeline depth of %d. Host = %s\n",
             depth, mConnInfo->Host()));

        if (depth >= 3)
            mGreenDepth = kPipelineUnlimited;
    }

    nsAHttpTransaction::Classifier classification;
    if (conn)
        classification = conn->Classification();
    else if (info == BadInsufficientFraming ||
             info == BadUnexpectedLarge)
        classification = (nsAHttpTransaction::Classifier) data;
    else
        classification = nsAHttpTransaction::CLASS_SOLO;

    if (gHttpHandler->GetPipelineAggressive() &&
        info & kPipelineInfoTypeBad &&
        info != BadExplicitClose &&
        info != RedVersionTooLow &&
        info != RedBannedServer &&
        info != RedCorruptedContent &&
        info != BadInsufficientFraming) {
        LOG(("minor negative feedback ignored "
             "because of pipeline aggressive mode"));
    }
    else if (info & kPipelineInfoTypeBad) {
        if ((info & kPipelineInfoTypeRed) && (mPipelineState != PS_RED)) {
            LOG(("transition to red from %d. Host = %s.\n",
                 mPipelineState, mConnInfo->Host()));
            mPipelineState = PS_RED;
            mPipeliningPenalty = 0;
        }

        if (mLastCreditTime.IsNull())
            mLastCreditTime = TimeStamp::Now();

        
        

        
        
        
        

        switch (info) {
        case RedVersionTooLow:
            mPipeliningPenalty += 1000;
            break;
        case RedBannedServer:
            mPipeliningPenalty += 7000;
            break;
        case RedCorruptedContent:
            mPipeliningPenalty += 7000;
            break;
        case RedCanceledPipeline:
            mPipeliningPenalty += 60;
            break;
        case BadExplicitClose:
            mPipeliningClassPenalty[classification] += 250;
            break;
        case BadSlowReadMinor:
            mPipeliningClassPenalty[classification] += 5;
            break;
        case BadSlowReadMajor:
            mPipeliningClassPenalty[classification] += 25;
            break;
        case BadInsufficientFraming:
            mPipeliningClassPenalty[classification] += 7000;
            break;
        case BadUnexpectedLarge:
            mPipeliningClassPenalty[classification] += 120;
            break;

        default:
            MOZ_ASSERT(false, "Unknown Bad/Red Pipeline Feedback Event");
        }

        const int16_t kPenalty = 25000;
        mPipeliningPenalty = std::min(mPipeliningPenalty, kPenalty);
        mPipeliningClassPenalty[classification] =
          std::min(mPipeliningClassPenalty[classification], kPenalty);

        LOG(("Assessing red penalty to %s class %d for event %d. "
             "Penalty now %d, throttle[%d] = %d\n", mConnInfo->Host(),
             classification, info, mPipeliningPenalty, classification,
             mPipeliningClassPenalty[classification]));
    }
    else {
        
        

        mPipeliningPenalty = std::max(mPipeliningPenalty - 1, 0);
        mPipeliningClassPenalty[classification] = std::max(mPipeliningClassPenalty[classification] - 1, 0);
    }

    if (mPipelineState == PS_RED && !mPipeliningPenalty)
    {
        LOG(("transition %s to yellow\n", mConnInfo->Host()));
        mPipelineState = PS_YELLOW;
        mYellowConnection = nullptr;
    }
}

void
nsHttpConnectionMgr::
nsConnectionEntry::SetYellowConnection(nsHttpConnection *conn)
{
    MOZ_ASSERT(!mYellowConnection && mPipelineState == PS_YELLOW,
               "yellow connection already set or state is not yellow");
    mYellowConnection = conn;
    mYellowGoodEvents = mYellowBadEvents = 0;
}

void
nsHttpConnectionMgr::
nsConnectionEntry::OnYellowComplete()
{
    if (mPipelineState == PS_YELLOW) {
        if (mYellowGoodEvents && !mYellowBadEvents) {
            LOG(("transition %s to green\n", mConnInfo->Host()));
            mPipelineState = PS_GREEN;
            mGreenDepth = mInitialGreenDepth;
        }
        else {
            
            
            
            
            LOG(("transition %s to red from yellow return\n",
                 mConnInfo->Host()));
            mPipelineState = PS_RED;
        }
    }

    mYellowConnection = nullptr;
}

void
nsHttpConnectionMgr::
nsConnectionEntry::CreditPenalty()
{
    if (mLastCreditTime.IsNull())
        return;

    
    

    TimeStamp now = TimeStamp::Now();
    TimeDuration elapsedTime = now - mLastCreditTime;
    uint32_t creditsEarned =
        static_cast<uint32_t>(elapsedTime.ToSeconds()) >> 4;

    bool failed = false;
    if (creditsEarned > 0) {
        mPipeliningPenalty =
            std::max(int32_t(mPipeliningPenalty - creditsEarned), 0);
        if (mPipeliningPenalty > 0)
            failed = true;

        for (int32_t i = 0; i < nsAHttpTransaction::CLASS_MAX; ++i) {
            mPipeliningClassPenalty[i]  =
                std::max(int32_t(mPipeliningClassPenalty[i] - creditsEarned), 0);
            failed = failed || (mPipeliningClassPenalty[i] > 0);
        }

        
        mLastCreditTime += TimeDuration::FromSeconds(creditsEarned << 4);
    }
    else {
        failed = true;                         
    }

    
    
    if (!failed)
        mLastCreditTime = TimeStamp();    

    if (mPipelineState == PS_RED && !mPipeliningPenalty)
    {
        LOG(("transition %s to yellow based on time credit\n",
             mConnInfo->Host()));
        mPipelineState = PS_YELLOW;
        mYellowConnection = nullptr;
    }
}

uint32_t
nsHttpConnectionMgr::
nsConnectionEntry::MaxPipelineDepth(nsAHttpTransaction::Classifier aClass)
{
    

    if ((mPipelineState == PS_RED) || (mPipeliningClassPenalty[aClass] > 0))
        return 0;

    if (mPipelineState == PS_YELLOW)
        return kPipelineRestricted;

    return mGreenDepth;
}

PLDHashOperator
nsHttpConnectionMgr::ReadConnectionEntry(const nsACString &key,
                nsAutoPtr<nsConnectionEntry> &ent,
                void *aArg)
{
    if (ent->mConnInfo->GetPrivate())
        return PL_DHASH_NEXT;

    nsTArray<HttpRetParams> *args = static_cast<nsTArray<HttpRetParams> *> (aArg);
    HttpRetParams data;
    data.host = ent->mConnInfo->Host();
    data.port = ent->mConnInfo->Port();
    for (uint32_t i = 0; i < ent->mActiveConns.Length(); i++) {
        HttpConnInfo info;
        info.ttl = ent->mActiveConns[i]->TimeToLive();
        info.rtt = ent->mActiveConns[i]->Rtt();
        if (ent->mActiveConns[i]->UsingSpdy())
            info.SetHTTP2ProtocolVersion(ent->mActiveConns[i]->GetSpdyVersion());
        else
            info.SetHTTP1ProtocolVersion(ent->mActiveConns[i]->GetLastHttpResponseVersion());

        data.active.AppendElement(info);
    }
    for (uint32_t i = 0; i < ent->mIdleConns.Length(); i++) {
        HttpConnInfo info;
        info.ttl = ent->mIdleConns[i]->TimeToLive();
        info.rtt = ent->mIdleConns[i]->Rtt();
        info.SetHTTP1ProtocolVersion(ent->mIdleConns[i]->GetLastHttpResponseVersion());
        data.idle.AppendElement(info);
    }
    for(uint32_t i = 0; i < ent->mHalfOpens.Length(); i++) {
        HalfOpenSockets hSocket;
        hSocket.speculative = ent->mHalfOpens[i]->IsSpeculative();
        data.halfOpens.AppendElement(hSocket);
    }
    data.spdy = ent->mUsingSpdy;
    data.ssl = ent->mConnInfo->EndToEndSSL();
    args->AppendElement(data);
    return PL_DHASH_NEXT;
}

bool
nsHttpConnectionMgr::GetConnectionData(nsTArray<HttpRetParams> *aArg)
{
    mCT.Enumerate(ReadConnectionEntry, aArg);
    return true;
}

void
nsHttpConnectionMgr::ResetIPFamilyPreference(nsHttpConnectionInfo *ci)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    nsConnectionEntry *ent = LookupConnectionEntry(ci, nullptr, nullptr);
    if (ent)
        ent->ResetIPFamilyPreference();
}

uint32_t
nsHttpConnectionMgr::
nsConnectionEntry::UnconnectedHalfOpens()
{
    uint32_t unconnectedHalfOpens = 0;
    for (uint32_t i = 0; i < mHalfOpens.Length(); ++i) {
        if (!mHalfOpens[i]->HasConnected())
            ++unconnectedHalfOpens;
    }
    return unconnectedHalfOpens;
}

void
nsHttpConnectionMgr::
nsConnectionEntry::RemoveHalfOpen(nsHalfOpenSocket *halfOpen)
{
    
    
    if (mHalfOpens.RemoveElement(halfOpen)) {

        if (halfOpen->IsSpeculative()) {
            Telemetry::AutoCounter<Telemetry::HTTPCONNMGR_UNUSED_SPECULATIVE_CONN> unusedSpeculativeConn;
            ++unusedSpeculativeConn;

            if (halfOpen->IsFromPredictor()) {
                Telemetry::AutoCounter<Telemetry::PREDICTOR_TOTAL_PRECONNECTS_UNUSED> totalPreconnectsUnused;
                ++totalPreconnectsUnused;
            }
        }

        MOZ_ASSERT(gHttpHandler->ConnMgr()->mNumHalfOpenConns);
        if (gHttpHandler->ConnMgr()->mNumHalfOpenConns) { 
            gHttpHandler->ConnMgr()->mNumHalfOpenConns--;
        }
    }

    if (!UnconnectedHalfOpens())
        
        
        
        gHttpHandler->ConnMgr()->ProcessPendingQ(mConnInfo);
}

void
nsHttpConnectionMgr::
nsConnectionEntry::RecordIPFamilyPreference(uint16_t family)
{
  if (family == PR_AF_INET && !mPreferIPv6)
    mPreferIPv4 = true;

  if (family == PR_AF_INET6 && !mPreferIPv4)
    mPreferIPv6 = true;
}

void
nsHttpConnectionMgr::
nsConnectionEntry::ResetIPFamilyPreference()
{
  mPreferIPv4 = false;
  mPreferIPv6 = false;
}

void
nsHttpConnectionMgr::MoveToWildCardConnEntry(nsHttpConnectionInfo *specificCI,
                                             nsHttpConnectionInfo *wildCardCI,
                                             nsHttpConnection *proxyConn)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    MOZ_ASSERT(specificCI->UsingHttpsProxy());

    LOG(("nsHttpConnectionMgr::MakeConnEntryWildCard conn %p has requested to "
         "change CI from %s to %s\n", proxyConn, specificCI->HashKey().get(),
         wildCardCI->HashKey().get()));

    nsConnectionEntry *ent = LookupConnectionEntry(specificCI, proxyConn, nullptr);
    LOG(("nsHttpConnectionMgr::MakeConnEntryWildCard conn %p using ent %p (spdy %d)\n",
         proxyConn, ent, ent ? ent->mUsingSpdy : 0));

    if (!ent || !ent->mUsingSpdy) {
        return;
    }

    nsConnectionEntry *wcEnt = GetOrCreateConnectionEntry(wildCardCI, true);
    if (wcEnt == ent) {
        
        return;
    }
    wcEnt->mUsingSpdy = true;
    wcEnt->mTestedSpdy = true;

    LOG(("nsHttpConnectionMgr::MakeConnEntryWildCard ent %p "
         "idle=%d active=%d half=%d pending=%d\n", ent,
         ent->mIdleConns.Length(), ent->mActiveConns.Length(),
         ent->mHalfOpens.Length(), ent->mPendingQ.Length()));

    LOG(("nsHttpConnectionMgr::MakeConnEntryWildCard wc-ent %p "
         "idle=%d active=%d half=%d pending=%d\n", wcEnt,
         wcEnt->mIdleConns.Length(), wcEnt->mActiveConns.Length(),
         wcEnt->mHalfOpens.Length(), wcEnt->mPendingQ.Length()));

    int32_t count = ent->mActiveConns.Length();
    for (int32_t i = 0; i < count; ++i) {
        if (ent->mActiveConns[i] == proxyConn) {
            ent->mActiveConns.RemoveElementAt(i);
            wcEnt->mActiveConns.InsertElementAt(0, proxyConn);
            return;
        }
    }

    count = ent->mIdleConns.Length();
    for (int32_t i = 0; i < count; ++i) {
        if (ent->mIdleConns[i] == proxyConn) {
            ent->mIdleConns.RemoveElementAt(i);
            wcEnt->mIdleConns.InsertElementAt(0, proxyConn);
            return;
        }
    }
}

} 
} 
