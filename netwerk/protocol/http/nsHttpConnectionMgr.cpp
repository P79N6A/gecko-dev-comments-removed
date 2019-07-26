




#include "nsHttpConnectionMgr.h"
#include "nsHttpConnection.h"
#include "nsHttpPipeline.h"
#include "nsHttpHandler.h"
#include "nsIHttpChannelInternal.h"
#include "nsNetCID.h"
#include "nsCOMPtr.h"
#include "nsNetUtil.h"
#include "mozilla/net/DNS.h"

#include "nsIServiceManager.h"

#include "nsIObserverService.h"

#include "nsISSLSocketControl.h"
#include "prnetdb.h"
#include "mozilla/Telemetry.h"
#include "mozilla/VisualEventTracer.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::net;


extern PRThread *gSocketThread;

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);




NS_IMPL_THREADSAFE_ISUPPORTS1(nsHttpConnectionMgr, nsIObserver)

static void
InsertTransactionSorted(nsTArray<nsHttpTransaction*> &pendingQ, nsHttpTransaction *trans)
{
    
    
    

    for (int32_t i=pendingQ.Length()-1; i>=0; --i) {
        nsHttpTransaction *t = pendingQ[i];
        if (trans->Priority() >= t->Priority()) {
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
    , mNumHalfOpenConns(0)
    , mTimeOfNextWakeUp(UINT64_MAX)
    , mTimeoutTickArmed(false)
{
    LOG(("Creating nsHttpConnectionMgr @%x\n", this));
    mCT.Init();
    mAlternateProtocolHash.Init(16);
    mSpdyPreferredHash.Init();
}

nsHttpConnectionMgr::~nsHttpConnectionMgr()
{
    LOG(("Destroying nsHttpConnectionMgr @%x\n", this));
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
        nsRefPtr<nsIRunnable> event = new nsConnEvent(this, handler, iparam, vparam);
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
        mTimer = NULL;
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
                             const PRUnichar *data)
{
    LOG(("nsHttpConnectionMgr::Observe [topic=\"%s\"]\n", topic));

    if (0 == strcmp(topic, NS_TIMER_CALLBACK_TOPIC)) {
        nsCOMPtr<nsITimer> timer = do_QueryInterface(subject);
        if (timer == mTimer) {
            PruneDeadConnections();
        }
        else if (timer == mTimeoutTick) {
            TimeoutTick();
        }
        else {
            NS_ABORT_IF_FALSE(false, "unexpected timer-callback");
            LOG(("Unexpected timer object\n"));
            return NS_ERROR_UNEXPECTED;
        }
    }

    return NS_OK;
}




nsresult
nsHttpConnectionMgr::AddTransaction(nsHttpTransaction *trans, int32_t priority)
{
    LOG(("nsHttpConnectionMgr::AddTransaction [trans=%x %d]\n", trans, priority));

    NS_ADDREF(trans);
    nsresult rv = PostEvent(&nsHttpConnectionMgr::OnMsgNewTransaction, priority, trans);
    if (NS_FAILED(rv))
        NS_RELEASE(trans);
    return rv;
}

nsresult
nsHttpConnectionMgr::RescheduleTransaction(nsHttpTransaction *trans, int32_t priority)
{
    LOG(("nsHttpConnectionMgr::RescheduleTransaction [trans=%x %d]\n", trans, priority));

    NS_ADDREF(trans);
    nsresult rv = PostEvent(&nsHttpConnectionMgr::OnMsgReschedTransaction, priority, trans);
    if (NS_FAILED(rv))
        NS_RELEASE(trans);
    return rv;
}

nsresult
nsHttpConnectionMgr::CancelTransaction(nsHttpTransaction *trans, nsresult reason)
{
    LOG(("nsHttpConnectionMgr::CancelTransaction [trans=%x reason=%x]\n", trans, reason));

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
nsHttpConnectionMgr::ClosePersistentConnections()
{
    return PostEvent(&nsHttpConnectionMgr::OnMsgClosePersistentConnections);
}

nsresult
nsHttpConnectionMgr::SpeculativeConnect(nsHttpConnectionInfo *ci,
                                        nsIInterfaceRequestor *callbacks)
{
    LOG(("nsHttpConnectionMgr::SpeculativeConnect [ci=%s]\n",
         ci->HashKey().get()));

    
    
    nsCOMPtr<nsIInterfaceRequestor> wrappedCallbacks;
    NS_NewInterfaceRequestorAggregation(callbacks, nullptr, getter_AddRefs(wrappedCallbacks));

    uint32_t caps = ci->GetAnonymous() ? NS_HTTP_LOAD_ANONYMOUS : 0;
    nsRefPtr<NullHttpTransaction> trans =
        new NullHttpTransaction(ci, wrappedCallbacks, caps);

    nsresult rv =
        PostEvent(&nsHttpConnectionMgr::OnMsgSpeculativeConnect, 0, trans);
    if (NS_SUCCEEDED(rv))
        trans.forget();
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
    LOG(("nsHttpConnectionMgr::ReclaimConnection [conn=%x]\n", conn));

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






nsHttpConnectionMgr::nsConnectionEntry *
nsHttpConnectionMgr::LookupConnectionEntry(nsHttpConnectionInfo *ci,
                                           nsHttpConnection *conn,
                                           nsHttpTransaction *trans)
{
    if (!ci)
        return nullptr;

    nsConnectionEntry *ent = mCT.Get(ci->HashKey());
    
    
    
    if (!ent || !ent->mUsingSpdy || ent->mCoalescingKey.IsEmpty())
        return ent;

    
    
    
    nsConnectionEntry *preferred = mSpdyPreferredHash.Get(ent->mCoalescingKey);
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
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
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
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    
    nsConnectionEntry *ent = LookupConnectionEntry(conn->ConnectionInfo(),
                                                   conn, nullptr);

    if (!ent)
        return;

    ent->mTestedSpdy = true;

    if (!usingSpdy)
        return;
    
    ent->mUsingSpdy = true;

    uint32_t ttl = conn->TimeToLive();
    uint64_t timeOfExpire = NowInSeconds() + ttl;
    if (!mTimer || timeOfExpire < mTimeOfNextWakeUp)
        PruneDeadConnectionsAfter(ttl);

    
    
    
    
    
    nsConnectionEntry *preferred =
        mSpdyPreferredHash.Get(ent->mCoalescingKey);

    LOG(("ReportSpdyConnection %s %s ent=%p preferred=%p\n",
         ent->mConnInfo->Host(), ent->mCoalescingKey.get(),
         ent, preferred));
    
    if (!preferred) {
        if (!ent->mCoalescingKey.IsEmpty()) {
            mSpdyPreferredHash.Put(ent->mCoalescingKey, ent);
            ent->mSpdyPreferred = true;
            preferred = ent;
        }
    }
    else if (preferred != ent) {
        
        
        

        
        
        
        
        

        conn->DontReuse();
    }

    PostEvent(&nsHttpConnectionMgr::OnMsgProcessAllSpdyPendingQ);
}

void
nsHttpConnectionMgr::ReportSpdyCWNDSetting(nsHttpConnectionInfo *ci,
                                           uint32_t cwndValue)
{
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

bool
nsHttpConnectionMgr::GetSpdyAlternateProtocol(nsACString &hostPortKey)
{
    if (!gHttpHandler->UseAlternateProtocol())
        return false;

    
    
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    return mAlternateProtocolHash.Contains(hostPortKey);
}

void
nsHttpConnectionMgr::ReportSpdyAlternateProtocol(nsHttpConnection *conn)
{
    
    if (!gHttpHandler->UseAlternateProtocol())
        return;

    
    if (conn->ConnectionInfo()->UsingHttpProxy())
        return;

    nsCString hostPortKey(conn->ConnectionInfo()->Host());
    if (conn->ConnectionInfo()->Port() != 80) {
        hostPortKey.Append(NS_LITERAL_CSTRING(":"));
        hostPortKey.AppendInt(conn->ConnectionInfo()->Port());
    }

    
    
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    
    if (mAlternateProtocolHash.Contains(hostPortKey))
        return;
    
    if (mAlternateProtocolHash.Count() > 2000)
        mAlternateProtocolHash.EnumerateEntries(&TrimAlternateProtocolHash,
						this);
    
    mAlternateProtocolHash.PutEntry(hostPortKey);
}

void
nsHttpConnectionMgr::RemoveSpdyAlternateProtocol(nsACString &hostPortKey)
{
    
    
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    return mAlternateProtocolHash.RemoveEntry(hostPortKey);
}

PLDHashOperator
nsHttpConnectionMgr::TrimAlternateProtocolHash(nsCStringHashKey *entry,
                                               void *closure)
{
    nsHttpConnectionMgr *self = (nsHttpConnectionMgr *) closure;
    
    if (self->mAlternateProtocolHash.Count() > 2000)
        return PL_DHASH_REMOVE;
    return PL_DHASH_STOP;
}

nsHttpConnectionMgr::nsConnectionEntry *
nsHttpConnectionMgr::GetSpdyPreferredEnt(nsConnectionEntry *aOriginalEntry)
{
    if (!gHttpHandler->IsSpdyEnabled() ||
        !gHttpHandler->CoalesceSpdy() ||
        aOriginalEntry->mCoalescingKey.IsEmpty())
        return nullptr;

    nsConnectionEntry *preferred =
        mSpdyPreferredHash.Get(aOriginalEntry->mCoalescingKey);

    
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
        
        
        preferred->mSpdyPreferred = false;
        RemoveSpdyPreferredEnt(preferred->mCoalescingKey);
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

    if (gHttpHandler->SpdyInfo()->ProtocolEnabled(0))
        rv = sslSocketControl->JoinConnection(gHttpHandler->SpdyInfo()->VersionString[0],
                                              aOriginalEntry->mConnInfo->GetHost(),
                                              aOriginalEntry->mConnInfo->Port(),
                                              &isJoined);
    else
        rv = NS_OK;                               

    
    
    if (NS_SUCCEEDED(rv) && !isJoined && gHttpHandler->SpdyInfo()->ProtocolEnabled(1)) {
        rv = sslSocketControl->JoinConnection(gHttpHandler->SpdyInfo()->VersionString[1],
                                              aOriginalEntry->mConnInfo->GetHost(),
                                              aOriginalEntry->mConnInfo->Port(),
                                              &isJoined);
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

void
nsHttpConnectionMgr::RemoveSpdyPreferredEnt(nsACString &aHashKey)
{
    if (aHashKey.IsEmpty())
        return;
    
    mSpdyPreferredHash.Remove(aHashKey);
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
        self->mNumActiveConns--;

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
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    LOG(("nsHttpConnectionMgr::ProcessPendingQForEntry [ci=%s]\n",
         ent->mConnInfo->HashKey().get()));

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

        rv = TryDispatchTransaction(ent, alreadyHalfOpen, trans);
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
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    nsConnectionEntry *ent = mCT.Get(ci->HashKey());
    if (ent)
        return ProcessPendingQForEntry(ent, false);
    return false;
}

bool
nsHttpConnectionMgr::SupportsPipelining(nsHttpConnectionInfo *ci)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

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
    NS_ABORT_IF_FALSE(uri, "precondition");

    nsAutoCString host;
    int32_t port = -1;
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
    if (NS_FAILED(rv) || !isHttp || host.IsEmpty())
        return;

    
    
    nsRefPtr<nsHttpConnectionInfo> ci =
        new nsHttpConnectionInfo(host, port, nullptr, usingSSL);
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
nsHttpConnectionMgr::RestrictConnections(nsConnectionEntry *ent)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    
    
    
    
    bool doRestrict = ent->mConnInfo->UsingSSL() &&
        gHttpHandler->IsSpdyEnabled() &&
        (!ent->mTestedSpdy || ent->mUsingSpdy) &&
        (ent->mHalfOpens.Length() || ent->mActiveConns.Length());

    
    if (!doRestrict)
        return false;
    
    
    
    if (ent->UnconnectedHalfOpens())
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
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
        
    uint32_t halfOpenLength = ent->mHalfOpens.Length();
    for (uint32_t i = 0; i < halfOpenLength; i++) {
        if (ent->mHalfOpens[i]->IsSpeculative()) {
            
            
            
            
            
            LOG(("nsHttpConnectionMgr::MakeNewConnection [ci = %s]\n"
                 "Found a speculative half open connection\n",
                 ent->mConnInfo->HashKey().get()));
            ent->mHalfOpens[i]->SetSpeculative(false);

            
            
            return NS_OK;
        }
    }

    
    
    
    if (!(trans->Caps() & NS_HTTP_DISALLOW_SPDY) && RestrictConnections(ent))
        return NS_ERROR_NOT_AVAILABLE;

    
    
    
    
    

    if ((mNumIdleConns + mNumActiveConns + 1 >= mMaxConns) && mNumIdleConns)
        mCT.Enumerate(PurgeExcessIdleConnectionsCB, this);

    if ((mNumIdleConns + mNumActiveConns + 1 >= mMaxConns) &&
        mNumActiveConns && gHttpHandler->IsSpdyEnabled())
        mCT.Enumerate(PurgeExcessSpdyConnectionsCB, this);

    if (AtActiveConnectionLimit(ent, trans->Caps()))
        return NS_ERROR_NOT_AVAILABLE;

    nsresult rv = CreateTransport(ent, trans, trans->Caps(), false);
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
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    LOG(("nsHttpConnectionMgr::TryDispatchTransaction without conn "
         "[ci=%s caps=%x]\n",
         ent->mConnInfo->HashKey().get(), uint32_t(trans->Caps())));

    nsHttpTransaction::Classifier classification = trans->Classification();
    uint32_t caps = trans->Caps();

    
    if (!(caps & NS_HTTP_ALLOW_KEEPALIVE))
        caps = caps & ~NS_HTTP_ALLOW_PIPELINING;

    
    
    
    
    
    
    
    
    
    
    
    
    

    bool attemptedOptimisticPipeline = !(caps & NS_HTTP_ALLOW_PIPELINING);

    
    
    

    if (!(caps & NS_HTTP_DISALLOW_SPDY) && gHttpHandler->IsSpdyEnabled()) {
        nsRefPtr<nsHttpConnection> conn = GetSpdyPreferredConn(ent);
        if (conn) {
            LOG(("   dispatch to spdy: [conn=%x]\n", conn.get()));
            trans->RemoveDispatchedAsBlocking();  
            DispatchTransaction(ent, trans, conn);
            return NS_OK;
        }
    }

    
    
    
    
    if (!(caps & NS_HTTP_LOAD_AS_BLOCKING)) {
        if (!(caps & NS_HTTP_LOAD_UNBLOCKED)) {
            nsILoadGroupConnectionInfo *loadGroupCI = trans->LoadGroupConnectionInfo();
            if (loadGroupCI) {
                uint32_t blockers = 0;
                if (NS_SUCCEEDED(loadGroupCI->GetBlockingTransactionCount(&blockers)) &&
                    blockers) {
                    
                    LOG(("   blocked by load group: [blockers=%d]\n", blockers));
                    return NS_ERROR_NOT_AVAILABLE;
                }
            }
        }
    }
    else {
        
        
        trans->DispatchedAsBlocking();
    }

    
    
    if (IsUnderPressure(ent, classification) && !attemptedOptimisticPipeline) {
        attemptedOptimisticPipeline = true;
        if (AddToShortestPipeline(ent, trans,
                                  classification,
                                  mMaxOptimisticPipelinedRequests)) {
            return NS_OK;
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
                LOG(("   dropping stale connection: [conn=%x]\n", conn.get()));
                conn->Close(NS_ERROR_ABORT);
                conn = nullptr;
            }
            else {
                LOG(("   reusing connection [conn=%x]\n", conn.get()));
                conn->EndIdleMonitoring();
            }

            
            
            ConditionallyStopPruneDeadConnectionsTimer();
        }
        if (conn) {
            
            
            AddActiveConn(conn, ent);
            DispatchTransaction(ent, trans, conn);
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
            return NS_OK;
        }
    }

    
    if (!onlyReusedConnection) {
        nsresult rv = MakeNewConnection(ent, trans);
        if (NS_SUCCEEDED(rv)) {
            
            return NS_ERROR_NOT_AVAILABLE;
        }
        
        if (rv != NS_ERROR_NOT_AVAILABLE) {
            
            
            return rv;
        }
    }
    
    
    if (caps & NS_HTTP_ALLOW_PIPELINING) {
        if (AddToShortestPipeline(ent, trans,
                                  classification,
                                  mMaxPipelinedRequests)) {
            return NS_OK;
        }
    }
    
    
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
         "[ci=%s trans=%x caps=%x conn=%x priority=%d]\n",
         ent->mConnInfo->HashKey().get(), trans, caps, conn, priority));

    if (conn->UsingSpdy()) {
        LOG(("Spdy Dispatch Transaction via Activate(). Transaction host = %s,"
             "Connection host = %s\n",
             trans->ConnectionInfo()->Host(),
             conn->ConnectionInfo()->Host()));
        rv = conn->Activate(trans, caps, priority);
        NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), "SPDY Cannot Fail Dispatch");
        if (NS_SUCCEEDED(rv) && !trans->GetPendingTime().IsNull()) {
            AccumulateTimeDelta(Telemetry::TRANSACTION_WAIT_TIME_SPDY,
                trans->GetPendingTime(), TimeStamp::Now());
            trans->SetPendingTime(false);
        }
        return rv;
    }

    NS_ABORT_IF_FALSE(conn && !conn->Transaction(),
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
    NS_ABORT_IF_FALSE(!conn->UsingSpdy(),
                      "Spdy Must Not Use DispatchAbstractTransaction");
    LOG(("nsHttpConnectionMgr::DispatchAbstractTransaction "
         "[ci=%s trans=%x caps=%x conn=%x]\n",
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
        mNumActiveConns--;
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
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    


    

   
    nsRefPtr<nsHttpPipeline> pipeline = new nsHttpPipeline();
    pipeline->AddTransaction(firstTrans);
    NS_ADDREF(*result = pipeline);
    return NS_OK;
}

void
nsHttpConnectionMgr::ReportProxyTelemetry(nsConnectionEntry *ent)
{
    enum { PROXY_NONE = 1, PROXY_HTTP = 2, PROXY_SOCKS = 3 };

    if (!ent->mConnInfo->UsingProxy())
        Telemetry::Accumulate(Telemetry::HTTP_PROXY_TYPE, PROXY_NONE);
    else if (ent->mConnInfo->UsingHttpProxy())
        Telemetry::Accumulate(Telemetry::HTTP_PROXY_TYPE, PROXY_HTTP);
    else
        Telemetry::Accumulate(Telemetry::HTTP_PROXY_TYPE, PROXY_SOCKS);
}

nsresult
nsHttpConnectionMgr::ProcessNewTransaction(nsHttpTransaction *trans)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    
    
    
    
    if (NS_FAILED(trans->Status())) {
        LOG(("  transaction was canceled... dropping event!\n"));
        return NS_OK;
    }

    trans->SetPendingTime();

    nsresult rv = NS_OK;
    nsHttpConnectionInfo *ci = trans->ConnectionInfo();
    NS_ASSERTION(ci, "no connection info");

    nsConnectionEntry *ent = GetOrCreateConnectionEntry(ci);

    
    
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
        NS_ASSERTION(trans->Caps() & NS_HTTP_STICKY_CONNECTION,
                     "unexpected caps");
        NS_ABORT_IF_FALSE(((int32_t)ent->mActiveConns.IndexOf(conn)) != -1,
                          "Sticky Connection Not In Active List");
        trans->SetConnection(nullptr);
        rv = DispatchTransaction(ent, trans, conn);
    }
    else
        rv = TryDispatchTransaction(ent, false, trans);

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
                                     bool speculative)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    
    nsRefPtr<nsHalfOpenSocket> sock = new nsHalfOpenSocket(ent, trans, caps);
    nsresult rv = sock->SetupPrimaryStreams();
    NS_ENSURE_SUCCESS(rv, rv);

    ent->mHalfOpens.AppendElement(sock);
    mNumHalfOpenConns++;
    if (speculative)
        sock->SetSpeculative(true);
    return NS_OK;
}







void
nsHttpConnectionMgr::ProcessSpdyPendingQ(nsConnectionEntry *ent)
{
    nsHttpConnection *conn = GetSpdyPreferredConn(ent);
    if (!conn)
        return;

    for (int32_t index = ent->mPendingQ.Length() - 1;
         index >= 0 && conn->CanDirectlyActivate();
         --index) {
        nsHttpTransaction *trans = ent->mPendingQ[index];

        if (!(trans->Caps() & NS_HTTP_ALLOW_KEEPALIVE) ||
            trans->Caps() & NS_HTTP_DISALLOW_SPDY)
            continue;
 
        ent->mPendingQ.RemoveElementAt(index);

        nsresult rv = DispatchTransaction(ent, trans, conn);
        if (NS_FAILED(rv)) {
            
            
            NS_ABORT_IF_FALSE(false, "Dispatch SPDY Transaction");
            LOG(("ProcessSpdyPendingQ Dispatch Transaction failed trans=%p\n",
                    trans));
            trans->Close(rv);
        }
        NS_RELEASE(trans);
    }
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
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    LOG(("nsHttpConnectionMgr::OnMsgProcessAllSpdyPendingQ\n"));
    mCT.Enumerate(ProcessSpdyPendingQCB, this);
}

nsHttpConnection *
nsHttpConnectionMgr::GetSpdyPreferredConn(nsConnectionEntry *ent)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ABORT_IF_FALSE(ent, "no connection entry");

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
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    LOG(("nsHttpConnectionMgr::OnMsgShutdown\n"));

    mCT.Enumerate(ShutdownPassCB, this);

    if (mTimeoutTick) {
        mTimeoutTick->Cancel();
        mTimeoutTick = nullptr;
        mTimeoutTickArmed = false;
    }
    
    
    nsRefPtr<nsIRunnable> runnable = 
        new nsConnEvent(this, &nsHttpConnectionMgr::OnMsgShutdownConfirm,
                        0, param);
    NS_DispatchToMainThread(runnable);
}

void
nsHttpConnectionMgr::OnMsgShutdownConfirm(int32_t priority, void *param)
{
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "wrong thread");
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
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
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
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    LOG(("nsHttpConnectionMgr::OnMsgCancelTransaction [trans=%p]\n", param));

    nsresult closeCode = static_cast<nsresult>(reason);
    nsHttpTransaction *trans = (nsHttpTransaction *) param;
    
    
    
    
    
    nsAHttpConnection *conn = trans->Connection();
    if (conn && !trans->IsDone())
        conn->CloseTransaction(trans, closeCode);
    else {
        nsConnectionEntry *ent = LookupConnectionEntry(trans->ConnectionInfo(),
                                                       nullptr, trans);

        if (ent) {
            int32_t index = ent->mPendingQ.IndexOf(trans);
            if (index >= 0) {
                ent->mPendingQ.RemoveElementAt(index);
                nsHttpTransaction *temp = trans;
                NS_RELEASE(temp); 
            }
        }
        trans->Close(closeCode);
    }
    NS_RELEASE(trans);
}

void
nsHttpConnectionMgr::OnMsgProcessPendingQ(int32_t, void *param)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
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

void
nsHttpConnectionMgr::OnMsgPruneDeadConnections(int32_t, void *)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    LOG(("nsHttpConnectionMgr::OnMsgPruneDeadConnections\n"));

    
    mTimeOfNextWakeUp = UINT64_MAX;

    
    
    if (mNumIdleConns || (mNumActiveConns && gHttpHandler->IsSpdyEnabled()))
        mCT.Enumerate(PruneDeadConnectionsCB, this);
}

void
nsHttpConnectionMgr::OnMsgClosePersistentConnections(int32_t, void *)
{
    LOG(("nsHttpConnectionMgr::OnMsgClosePersistentConnections\n"));

    mCT.Enumerate(ClosePersistentConnectionsCB, this);
}

void
nsHttpConnectionMgr::OnMsgReclaimConnection(int32_t, void *param)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    LOG(("nsHttpConnectionMgr::OnMsgReclaimConnection [conn=%p]\n", param));

    nsHttpConnection *conn = (nsHttpConnection *) param;

    
    
    
    
    

    nsConnectionEntry *ent = LookupConnectionEntry(conn->ConnectionInfo(),
                                                   conn, nullptr);
    nsHttpConnectionInfo *ci = nullptr;

    if (!ent) {
        
        LOG(("nsHttpConnectionMgr::OnMsgReclaimConnection ent == null\n"));
        NS_ABORT_IF_FALSE(false, "no connection entry");
        NS_ADDREF(ci = conn->ConnectionInfo());
    }
    else {
        NS_ADDREF(ci = ent->mConnInfo);

        
        
        
        

        if (ent->mUsingSpdy) {
            
            
            
            
            
            
            conn->DontReuse();
        }
        
        if (ent->mActiveConns.RemoveElement(conn)) {
            if (conn == ent->mYellowConnection)
                ent->OnYellowComplete();
            nsHttpConnection *temp = conn;
            NS_RELEASE(temp);
            mNumActiveConns--;
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
        }
        else {
            LOG(("  connection cannot be reused; closing connection\n"));
            conn->Close(NS_ERROR_ABORT);
        }
    }
 
    OnMsgProcessPendingQ(0, ci); 
    NS_RELEASE(conn);
}

void
nsHttpConnectionMgr::OnMsgCompleteUpgrade(int32_t, void *param)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
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
    if (mSpdyPreferred)
        gHttpHandler->ConnMgr()->RemoveSpdyPreferredEnt(mCoalescingKey);

    NS_RELEASE(mConnInfo);
}

void
nsHttpConnectionMgr::OnMsgProcessFeedback(int32_t, void *param)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    nsHttpPipelineFeedback *fb = (nsHttpPipelineFeedback *)param;
    
    PipelineFeedbackInfo(fb->mConnInfo, fb->mInfo, fb->mConn, fb->mData);
    delete fb;
}



void
nsHttpConnectionMgr::ActivateTimeoutTick()
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    LOG(("nsHttpConnectionMgr::ActivateTimeoutTick() "
         "this=%p mTimeoutTick=%p\n"));

    
    
    

    if (mTimeoutTick && mTimeoutTickArmed)
        return;

    if (!mTimeoutTick) {
        mTimeoutTick = do_CreateInstance(NS_TIMER_CONTRACTID);
        if (!mTimeoutTick) {
            NS_WARNING("failed to create timer for http timeout management");
            return;
        }
        mTimeoutTick->SetTarget(mSocketThreadTarget);
    }

    NS_ABORT_IF_FALSE(!mTimeoutTickArmed, "timer tick armed");
    mTimeoutTickArmed = true;
    mTimeoutTick->Init(this, 1000, nsITimer::TYPE_REPEATING_SLACK);
}

void
nsHttpConnectionMgr::TimeoutTick()
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ABORT_IF_FALSE(mTimeoutTick, "no readtimeout tick");

    LOG(("nsHttpConnectionMgr::TimeoutTick active=%d\n",
         mNumActiveConns));

    mCT.Enumerate(TimeoutTickCB, this);
}

PLDHashOperator
nsHttpConnectionMgr::TimeoutTickCB(const nsACString &key,
                                       nsAutoPtr<nsConnectionEntry> &ent,
                                       void *closure)
{
    nsHttpConnectionMgr *self = (nsHttpConnectionMgr *) closure;

    LOG(("nsHttpConnectionMgr::TimeoutTickCB() this=%p host=%s\n",
         self, ent->mConnInfo->Host()));

    
    PRIntervalTime now = PR_IntervalNow();
    for (uint32_t index = 0; index < ent->mActiveConns.Length(); ++index)
        ent->mActiveConns[index]->ReadTimeoutTick(now);

    
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
                    half->SocketTransport()->Close(NS_ERROR_NET_TIMEOUT);
                if (half->BackupTransport())
                    half->BackupTransport()->Close(NS_ERROR_NET_TIMEOUT);
            }

            
            
            if (delta > maxConnectTime + 5000) {
                LOG(("Abandon half open to %s after %.2fms.\n",
                     ent->mConnInfo->HashKey().get(), delta));
                half->Abandon();
            }
        }
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

NS_IMPL_THREADSAFE_ISUPPORTS0(nsHttpConnectionMgr::nsConnectionHandle)

nsHttpConnectionMgr::nsConnectionEntry *
nsHttpConnectionMgr::GetOrCreateConnectionEntry(nsHttpConnectionInfo *ci)
{
    nsConnectionEntry *ent = mCT.Get(ci->HashKey());
    if (ent)
        return ent;

    nsHttpConnectionInfo *clone = ci->Clone();
    ent = new nsConnectionEntry(clone);
    mCT.Put(ci->HashKey(), ent);
    return ent;
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
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    nsRefPtr<NullHttpTransaction> trans =
        dont_AddRef(static_cast<NullHttpTransaction *>(param));

    LOG(("nsHttpConnectionMgr::OnMsgSpeculativeConnect [ci=%s]\n",
         trans->ConnectionInfo()->HashKey().get()));

    nsConnectionEntry *ent =
        GetOrCreateConnectionEntry(trans->ConnectionInfo());

    
    
    
    nsConnectionEntry *preferredEntry = GetSpdyPreferredEnt(ent);
    if (preferredEntry)
        ent = preferredEntry;

    if (mNumHalfOpenConns <= gHttpHandler->ParallelSpeculativeConnectLimit() &&
        !ent->mIdleConns.Length() && !RestrictConnections(ent) &&
        !AtActiveConnectionLimit(ent, trans->Caps())) {
        CreateTransport(ent, trans, trans->Caps(), true);
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





NS_IMPL_THREADSAFE_ISUPPORTS4(nsHttpConnectionMgr::nsHalfOpenSocket,
                              nsIOutputStreamCallback,
                              nsITransportEventSink,
                              nsIInterfaceRequestor,
                              nsITimerCallback)

nsHttpConnectionMgr::
nsHalfOpenSocket::nsHalfOpenSocket(nsConnectionEntry *ent,
                                   nsAHttpTransaction *trans,
                                   uint32_t caps)
    : mEnt(ent),
      mTransaction(trans),
      mCaps(caps),
      mSpeculative(false),
      mHasConnected(false)
{
    NS_ABORT_IF_FALSE(ent && trans, "constructor with null arguments");
    LOG(("Creating nsHalfOpenSocket [this=%p trans=%p ent=%s]\n",
         this, trans, ent->mConnInfo->Host()));
}

nsHttpConnectionMgr::nsHalfOpenSocket::~nsHalfOpenSocket()
{
    NS_ABORT_IF_FALSE(!mStreamOut, "streamout not null");
    NS_ABORT_IF_FALSE(!mBackupStreamOut, "backupstreamout not null");
    NS_ABORT_IF_FALSE(!mSynTimer, "syntimer not null");
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

    const char* types[1];
    types[0] = (mEnt->mConnInfo->UsingSSL()) ?
        "ssl" : gHttpHandler->DefaultSocketType();
    uint32_t typeCount = (types[0] != nullptr);

    nsCOMPtr<nsISocketTransport> socketTransport;
    nsCOMPtr<nsISocketTransportService> sts;

    sts = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = sts->CreateTransport(types, typeCount,
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

    socketTransport->SetConnectionFlags(tmpFlags);

    socketTransport->SetQoSBits(gHttpHandler->GetQoSBits());

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
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    nsresult rv;

    mPrimarySynStarted = TimeStamp::Now();
    rv = SetupStreams(getter_AddRefs(mSocketTransport),
                      getter_AddRefs(mStreamIn),
                      getter_AddRefs(mStreamOut),
                      false);
    LOG(("nsHalfOpenSocket::SetupPrimaryStreams [this=%p ent=%s rv=%x]",
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
    NS_ABORT_IF_FALSE(!mSynTimer, "timer already initd");
    
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

    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    nsRefPtr<nsHalfOpenSocket> deleteProtector(this);

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
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ABORT_IF_FALSE(timer == mSynTimer, "wrong timer");

    SetupBackupStreams();

    mSynTimer = nullptr;
    return NS_OK;
}


NS_IMETHODIMP
nsHttpConnectionMgr::
nsHalfOpenSocket::OnOutputStreamReady(nsIAsyncOutputStream *out)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ABORT_IF_FALSE(out == mStreamOut ||
                      out == mBackupStreamOut, "stream mismatch");
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

    NetAddr peeraddr;
    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    mTransaction->GetSecurityCallbacks(getter_AddRefs(callbacks));
    if (out == mStreamOut) {
        if (static_cast<uint32_t>(mPrimarySynRTT.ToMilliseconds()) <= 125)
            gHttpHandler->mCacheEffectExperimentFastConn++;
        else
            gHttpHandler->mCacheEffectExperimentSlowConn++;
        
        rv = conn->Init(mEnt->mConnInfo,
                        gHttpHandler->ConnMgr()->mMaxRequestDelay,
                        mSocketTransport, mStreamIn, mStreamOut,
                        callbacks,
                        PR_MillisecondsToInterval(
                          static_cast<uint32_t>(mPrimarySynRTT.ToMilliseconds())));

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
                        callbacks,
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
        NS_ABORT_IF_FALSE(!mSpeculative,
                          "Speculative Half Open found mTranscation");
        nsRefPtr<nsHttpTransaction> temp = dont_AddRef(mEnt->mPendingQ[index]);
        mEnt->mPendingQ.RemoveElementAt(index);
        gHttpHandler->ConnMgr()->AddActiveConn(conn, mEnt);
        rv = gHttpHandler->ConnMgr()->DispatchTransaction(mEnt, temp, conn);
    }
    else {
        
        

        
        
        const PRIntervalTime k5Sec = PR_SecondsToInterval(5);
        if (k5Sec < gHttpHandler->IdleTimeout())
            conn->SetIdleTimeout(k5Sec);
        else
            conn->SetIdleTimeout(gHttpHandler->IdleTimeout());

        
        
        
        conn->SetIsReusedAfter(950);

        
        
        
        
        
        if (mEnt->mConnInfo->UsingSSL() && !mEnt->mPendingQ.Length() &&
            !mEnt->mConnInfo->UsingHttpProxy()) {
            LOG(("nsHalfOpenSocket::OnOutputStreamReady null transaction will "
                 "be used to finish SSL handshake on conn %p\n", conn.get()));
            nsRefPtr<NullHttpTransaction>  trans =
                new NullHttpTransaction(mEnt->mConnInfo,
                                        callbacks,
                                        mCaps & ~NS_HTTP_ALLOW_PIPELINING);

            gHttpHandler->ConnMgr()->AddActiveConn(conn, mEnt);
            conn->Classify(nsAHttpTransaction::CLASS_SOLO);
            rv = gHttpHandler->ConnMgr()->
                DispatchAbstractTransaction(mEnt, trans, mCaps, conn, 0);
        }
        else {
            
            LOG(("nsHalfOpenSocket::OnOutputStreamReady no transaction match "
                 "returning conn %p to pool\n", conn.get()));
            nsRefPtr<nsHttpConnection> copy(conn);
            
            gHttpHandler->ConnMgr()->OnMsgReclaimConnection(
                0, conn.forget().get());
        }
    }

    return rv;
}


NS_IMETHODIMP
nsHttpConnectionMgr::nsHalfOpenSocket::OnTransportStatus(nsITransport *trans,
                                                         nsresult status,
                                                         uint64_t progress,
                                                         uint64_t progressMax)
{
    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (mTransaction)
        mTransaction->OnTransportStatus(trans, status, progress);

    
    if (trans != mSocketTransport)
        return NS_OK;

    
    
    
    

    if (status == NS_NET_STATUS_CONNECTED_TO &&
        gHttpHandler->IsSpdyEnabled() &&
        gHttpHandler->CoalesceSpdy() &&
        mEnt && mEnt->mConnInfo && mEnt->mConnInfo->UsingSSL() &&
        !mEnt->mConnInfo->UsingProxy() &&
        mEnt->mCoalescingKey.IsEmpty()) {

        NetAddr addr;
        nsresult rv = mSocketTransport->GetPeerAddr(&addr);
        if (NS_SUCCEEDED(rv)) {
            mEnt->mCoalescingKey.SetCapacity(kIPv6CStrBufSize + 26);
            NetAddrToString(&addr, mEnt->mCoalescingKey.BeginWriting(), kIPv6CStrBufSize);
            mEnt->mCoalescingKey.SetLength(
                strlen(mEnt->mCoalescingKey.BeginReading()));

            if (mEnt->mConnInfo->GetAnonymous())
                mEnt->mCoalescingKey.AppendLiteral("~A:");
            else
                mEnt->mCoalescingKey.AppendLiteral("~.:");
            mEnt->mCoalescingKey.AppendInt(mEnt->mConnInfo->Port());

            LOG(("nsHttpConnectionMgr::nsHalfOpenSocket::OnTransportStatus "
                 "STATUS_CONNECTED_TO Established New Coalescing Key for host "
                 "%s [%s]", mEnt->mConnInfo->Host(),
                 mEnt->mCoalescingKey.get()));

            gHttpHandler->ConnMgr()->ProcessSpdyPendingQ(mEnt);
        }
    }

    switch (status) {
    case NS_NET_STATUS_CONNECTING_TO:
        
        
        
        
        
        
        
        
        if (mEnt) {
            
            mPrimarySynStarted = TimeStamp::Now();

            if (!mBackupTransport && !mSynTimer)
                SetupBackupTimer();
        }
        break;

    case NS_NET_STATUS_CONNECTED_TO:
        
        
        CancelBackupTimer();
        if (mEnt && !mPrimarySynStarted.IsNull())
            mPrimarySynRTT = TimeStamp::Now() - mPrimarySynStarted;
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
    
    

    NS_ASSERTION(mConn, "no connection");
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
    , mSpdyPreferred(false)
    , mPreferIPv4(false)
    , mPreferIPv6(false)
{
    NS_ADDREF(mConnInfo);
    if (gHttpHandler->GetPipelineAggressive()) {
        mGreenDepth = kPipelineUnlimited;
        mPipelineState = PS_GREEN;
    }
    mInitialGreenDepth = mGreenDepth;
    memset(mPipeliningClassPenalty, 0, sizeof(int16_t) * nsAHttpTransaction::CLASS_MAX);
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
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    
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
            NS_ABORT_IF_FALSE(0, "Unknown Bad/Red Pipeline Feedback Event");
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
    NS_ABORT_IF_FALSE(!mYellowConnection && mPipelineState == PS_YELLOW,
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
    nsTArray<HttpRetParams> *args = static_cast<nsTArray<HttpRetParams> *> (aArg);
    HttpRetParams data;
    data.host = ent->mConnInfo->Host();
    data.port = ent->mConnInfo->Port();
    for (uint32_t i = 0; i < ent->mActiveConns.Length(); i++) {
        HttpConnInfo info;
        info.ttl = ent->mActiveConns[i]->TimeToLive();
        info.rtt = ent->mActiveConns[i]->Rtt();
        data.active.AppendElement(info);
    }
    for (uint32_t i = 0; i < ent->mIdleConns.Length(); i++) {
        HttpConnInfo info;
        info.ttl = ent->mIdleConns[i]->TimeToLive();
        info.rtt = ent->mIdleConns[i]->Rtt();
        data.active.AppendElement(info);
    }
    data.spdy = ent->mUsingSpdy;
    data.ssl = ent->mConnInfo->UsingSSL();
    args->AppendElement(data);
    return PL_DHASH_NEXT;
}

bool
nsHttpConnectionMgr::GetConnectionData(nsTArray<mozilla::net::HttpRetParams> *aArg)
{
    mCT.Enumerate(ReadConnectionEntry, aArg);
    return true;
}

void
nsHttpConnectionMgr::ResetIPFamillyPreference(nsHttpConnectionInfo *ci)
{
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
    
    
    
    mHalfOpens.RemoveElement(halfOpen);
    gHttpHandler->ConnMgr()->mNumHalfOpenConns--;

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
