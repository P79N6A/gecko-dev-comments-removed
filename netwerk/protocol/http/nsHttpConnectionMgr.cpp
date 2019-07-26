




#include "nsHttpConnectionMgr.h"
#include "nsHttpConnection.h"
#include "nsHttpPipeline.h"
#include "nsHttpHandler.h"
#include "nsIHttpChannelInternal.h"
#include "nsNetCID.h"
#include "nsCOMPtr.h"
#include "nsNetUtil.h"

#include "nsIServiceManager.h"

#include "nsIObserverService.h"

#include "nsISSLSocketControl.h"
#include "prnetdb.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;
using namespace mozilla::net;


extern PRThread *gSocketThread;

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);




NS_IMPL_THREADSAFE_ISUPPORTS1(nsHttpConnectionMgr, nsIObserver)

static void
InsertTransactionSorted(nsTArray<nsHttpTransaction*> &pendingQ, nsHttpTransaction *trans)
{
    
    
    

    for (PRInt32 i=pendingQ.Length()-1; i>=0; --i) {
        nsHttpTransaction *t = pendingQ[i];
        if (trans->Priority() >= t->Priority()) {
            pendingQ.InsertElementAt(i+1, trans);
            return;
        }
    }
    pendingQ.InsertElementAt(0, trans);
}



nsHttpConnectionMgr::nsHttpConnectionMgr()
    : mRef(0)
    , mReentrantMonitor("nsHttpConnectionMgr.mReentrantMonitor")
    , mMaxConns(0)
    , mMaxConnsPerHost(0)
    , mMaxConnsPerProxy(0)
    , mMaxPersistConnsPerHost(0)
    , mMaxPersistConnsPerProxy(0)
    , mIsShuttingDown(false)
    , mNumActiveConns(0)
    , mNumIdleConns(0)
    , mTimeOfNextWakeUp(LL_MAXUINT)
    , mReadTimeoutTickArmed(false)
{
    LOG(("Creating nsHttpConnectionMgr @%x\n", this));
    mCT.Init();
    mAlternateProtocolHash.Init(16);
    mSpdyPreferredHash.Init();
}

nsHttpConnectionMgr::~nsHttpConnectionMgr()
{
    LOG(("Destroying nsHttpConnectionMgr @%x\n", this));
    if (mReadTimeoutTick)
        mReadTimeoutTick->Cancel();
}

nsresult
nsHttpConnectionMgr::EnsureSocketThreadTargetIfOnline()
{
    nsresult rv;
    nsCOMPtr<nsIEventTarget> sts;
    nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
    if (NS_SUCCEEDED(rv)) {
        bool offline = true;
        ioService->GetOffline(&offline);

        if (!offline) {
            sts = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
        }
    }

    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    
    if (mSocketThreadTarget || mIsShuttingDown)
        return NS_OK;

    mSocketThreadTarget = sts;

    return rv;
}

nsresult
nsHttpConnectionMgr::Init(PRUint16 maxConns,
                          PRUint16 maxConnsPerHost,
                          PRUint16 maxConnsPerProxy,
                          PRUint16 maxPersistConnsPerHost,
                          PRUint16 maxPersistConnsPerProxy,
                          PRUint16 maxRequestDelay,
                          PRUint16 maxPipelinedRequests,
                          PRUint16 maxOptimisticPipelinedRequests)
{
    LOG(("nsHttpConnectionMgr::Init\n"));

    {
        ReentrantMonitorAutoEnter mon(mReentrantMonitor);

        mMaxConns = maxConns;
        mMaxConnsPerHost = maxConnsPerHost;
        mMaxConnsPerProxy = maxConnsPerProxy;
        mMaxPersistConnsPerHost = maxPersistConnsPerHost;
        mMaxPersistConnsPerProxy = maxPersistConnsPerProxy;
        mMaxRequestDelay = maxRequestDelay;
        mMaxPipelinedRequests = maxPipelinedRequests;
        mMaxOptimisticPipelinedRequests = maxOptimisticPipelinedRequests;

        mIsShuttingDown = false;
    }

    return EnsureSocketThreadTargetIfOnline();
}

nsresult
nsHttpConnectionMgr::Shutdown()
{
    LOG(("nsHttpConnectionMgr::Shutdown\n"));

    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    
    if (!mSocketThreadTarget)
        return NS_OK;

    nsresult rv = PostEvent(&nsHttpConnectionMgr::OnMsgShutdown);

    
    
    
    mIsShuttingDown = true;
    mSocketThreadTarget = 0;

    if (NS_FAILED(rv)) {
        NS_WARNING("unable to post SHUTDOWN message");
        return rv;
    }

    
    mon.Wait();
    return NS_OK;
}

nsresult
nsHttpConnectionMgr::PostEvent(nsConnEventHandler handler, PRInt32 iparam, void *vparam)
{
    
    
    
    
    EnsureSocketThreadTargetIfOnline();

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
nsHttpConnectionMgr::PruneDeadConnectionsAfter(PRUint32 timeInSeconds)
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

    
    mTimeOfNextWakeUp = LL_MAXUINT;
    if (mTimer) {
        mTimer->Cancel();
        mTimer = NULL;
    }
}

void
nsHttpConnectionMgr::ConditionallyStopReadTimeoutTick()
{
    LOG(("nsHttpConnectionMgr::ConditionallyStopReadTimeoutTick "
         "armed=%d active=%d\n", mReadTimeoutTickArmed, mNumActiveConns));

    if (!mReadTimeoutTickArmed)
        return;

    if (mNumActiveConns)
        return;

    LOG(("nsHttpConnectionMgr::ConditionallyStopReadTimeoutTick stop==true\n"));

    mReadTimeoutTick->Cancel();
    mReadTimeoutTickArmed = false;
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
        else if (timer == mReadTimeoutTick) {
            ReadTimeoutTick();
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
nsHttpConnectionMgr::AddTransaction(nsHttpTransaction *trans, PRInt32 priority)
{
    LOG(("nsHttpConnectionMgr::AddTransaction [trans=%x %d]\n", trans, priority));

    NS_ADDREF(trans);
    nsresult rv = PostEvent(&nsHttpConnectionMgr::OnMsgNewTransaction, priority, trans);
    if (NS_FAILED(rv))
        NS_RELEASE(trans);
    return rv;
}

nsresult
nsHttpConnectionMgr::RescheduleTransaction(nsHttpTransaction *trans, PRInt32 priority)
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
    nsresult rv = PostEvent(&nsHttpConnectionMgr::OnMsgCancelTransaction, reason, trans);
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
                                        nsIInterfaceRequestor *callbacks,
                                        nsIEventTarget *target)
{
    LOG(("nsHttpConnectionMgr::SpeculativeConnect [ci=%s]\n",
         ci->HashKey().get()));

    PRUint8 caps = ci->GetAnonymous() ? NS_HTTP_LOAD_ANONYMOUS : 0;
    nsRefPtr<NullHttpTransaction> trans =
        new NullHttpTransaction(ci, callbacks, target, caps);

    nsresult rv =
        PostEvent(&nsHttpConnectionMgr::OnMsgSpeculativeConnect, 0, trans);
    if (NS_SUCCEEDED(rv))
        trans.forget();
    return rv;
}

nsresult
nsHttpConnectionMgr::GetSocketThreadTarget(nsIEventTarget **target)
{
    
    
    
    
    EnsureSocketThreadTargetIfOnline();

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
nsHttpConnectionMgr::UpdateParam(nsParamName name, PRUint16 value)
{
    PRUint32 param = (PRUint32(name) << 16) | PRUint32(value);
    return PostEvent(&nsHttpConnectionMgr::OnMsgUpdateParam, 0, (void *) param);
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






nsHttpConnectionMgr::nsConnectionEntry *
nsHttpConnectionMgr::LookupConnectionEntry(nsHttpConnectionInfo *ci,
                                           nsHttpConnection *conn,
                                           nsHttpTransaction *trans)
{
    if (!ci)
        return nsnull;

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
                                                   conn, nsnull);

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
                                                   conn, nsnull);

    if (!ent)
        return;

    ent->mTestedSpdy = true;

    if (!usingSpdy)
        return;
    
    ent->mUsingSpdy = true;

    PRUint32 ttl = conn->TimeToLive();
    PRUint64 timeOfExpire = NowInSeconds() + ttl;
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

    ProcessAllSpdyPendingQ();
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
        return nsnull;

    nsConnectionEntry *preferred =
        mSpdyPreferredHash.Get(aOriginalEntry->mCoalescingKey);

    
    if (preferred == aOriginalEntry)
        return aOriginalEntry;

    
    
    if (!preferred || !preferred->mUsingSpdy)
        return nsnull;                         

    
    
    
    

    nsHttpConnection *activeSpdy = nsnull;

    for (PRUint32 index = 0; index < preferred->mActiveConns.Length(); ++index) {
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

        return nsnull;
    }

    
    nsresult rv;
    bool isJoined = false;

    nsCOMPtr<nsISupports> securityInfo;
    nsCOMPtr<nsISSLSocketControl> sslSocketControl;
    nsCAutoString negotiatedNPN;
    
    activeSpdy->GetSecurityInfo(getter_AddRefs(securityInfo));
    if (!securityInfo) {
        NS_WARNING("cannot obtain spdy security info");
        return nsnull;
    }

    sslSocketControl = do_QueryInterface(securityInfo, &rv);
    if (NS_FAILED(rv)) {
        NS_WARNING("sslSocketControl QI Failed");
        return nsnull;
    }

    rv = sslSocketControl->JoinConnection(NS_LITERAL_CSTRING("spdy/2"),
                                          aOriginalEntry->mConnInfo->GetHost(),
                                          aOriginalEntry->mConnInfo->Port(),
                                          &isJoined);

    if (NS_FAILED(rv) || !isJoined) {
        LOG(("nsHttpConnectionMgr::GetSpdyPreferredConnection "
             "Host %s cannot be confirmed to be joined "
             "with %s connections. rv=%x isJoined=%d",
             preferred->mConnInfo->Host(), aOriginalEntry->mConnInfo->Host(),
             rv, isJoined));
        mozilla::Telemetry::Accumulate(mozilla::Telemetry::SPDY_NPN_JOIN,
                                       false);
        return nsnull;
    }

    
    LOG(("nsHttpConnectionMgr::GetSpdyPreferredConnection "
         "Host %s has cert valid for %s connections, "
         "so %s will be coalesced with %s",
         preferred->mConnInfo->Host(), aOriginalEntry->mConnInfo->Host(),
         aOriginalEntry->mConnInfo->Host(), preferred->mConnInfo->Host()));
    mozilla::Telemetry::Accumulate(mozilla::Telemetry::SPDY_NPN_JOIN, true);
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

    if (self->ProcessPendingQForEntry(ent))
        return PL_DHASH_STOP;

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
nsHttpConnectionMgr::PruneDeadConnectionsCB(const nsACString &key,
                                            nsAutoPtr<nsConnectionEntry> &ent,
                                            void *closure)
{
    nsHttpConnectionMgr *self = (nsHttpConnectionMgr *) closure;

    LOG(("  pruning [ci=%s]\n", ent->mConnInfo->HashKey().get()));

    
    
    PRUint32 timeToNextExpire = PR_UINT32_MAX;
    PRInt32 count = ent->mIdleConns.Length();
    if (count > 0) {
        for (PRInt32 i=count-1; i>=0; --i) {
            nsHttpConnection *conn = ent->mIdleConns[i];
            if (!conn->CanReuse()) {
                ent->mIdleConns.RemoveElementAt(i);
                conn->Close(NS_ERROR_ABORT);
                NS_RELEASE(conn);
                self->mNumIdleConns--;
            } else {
                timeToNextExpire = NS_MIN(timeToNextExpire, conn->TimeToLive());
            }
        }
    }

    if (ent->mUsingSpdy) {
        for (PRUint32 index = 0; index < ent->mActiveConns.Length(); ++index) {
            nsHttpConnection *conn = ent->mActiveConns[index];
            if (conn->UsingSpdy()) {
                if (!conn->CanReuse()) {
                    
                    
                    conn->DontReuse();
                }
                else {
                    timeToNextExpire = NS_MIN(timeToNextExpire,
                                              conn->TimeToLive());
                }
            }
        }
    }
    
    
    
    if (timeToNextExpire != PR_UINT32_MAX) {
        PRUint32 now = NowInSeconds();
        PRUint64 timeOfNextExpire = now + timeToNextExpire;
        
        
        
        
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

    
    for (PRInt32 i = ((PRInt32) ent->mHalfOpens.Length()) - 1; i >= 0; i--)
        ent->mHalfOpens[i]->Abandon();

    return PL_DHASH_REMOVE;
}



bool
nsHttpConnectionMgr::ProcessPendingQForEntry(nsConnectionEntry *ent)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    LOG(("nsHttpConnectionMgr::ProcessPendingQForEntry [ci=%s]\n",
         ent->mConnInfo->HashKey().get()));

    ProcessSpdyPendingQ(ent);

    PRUint32 count = ent->mPendingQ.Length();
    nsHttpTransaction *trans;
    nsresult rv;
    bool dispatchedSuccessfully = false;

    
    
    for (PRUint32 i = 0; i < count; ++i) {
        trans = ent->mPendingQ[i];

        
        
        
        
        
        bool alreadyHalfOpen = false;
        for (PRInt32 j = 0; j < ((PRInt32) ent->mHalfOpens.Length()); ++j) {
            if (ent->mHalfOpens[j]->Transaction() == trans) {
                alreadyHalfOpen = true;
                break;
            }
        }

        rv = TryDispatchTransaction(ent, alreadyHalfOpen, trans);
        if (NS_SUCCEEDED(rv)) {
            LOG(("  dispatching pending transaction...\n"));
            ent->mPendingQ.RemoveElementAt(i);
            NS_RELEASE(trans);

            
            dispatchedSuccessfully = true;
            count = ent->mPendingQ.Length();
            --i;
            continue;
        }

        if (dispatchedSuccessfully)
            return true;

        NS_ABORT_IF_FALSE(count == ((PRInt32) ent->mPendingQ.Length()),
                          "something mutated pending queue from "
                          "GetConnection()");
    }
    return false;
}

bool
nsHttpConnectionMgr::ProcessPendingQForEntry(nsHttpConnectionInfo *ci)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    nsConnectionEntry *ent = mCT.Get(ci->HashKey());
    if (ent)
        return ProcessPendingQForEntry(ent);
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
                           nsHttpConnection *conn, PRUint32 data)
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
    PRUint32 mData;
};

void
nsHttpConnectionMgr::PipelineFeedbackInfo(nsHttpConnectionInfo *ci,
                                          PipelineFeedbackInfoType info,
                                          nsHttpConnection *conn,
                                          PRUint32 data)
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

    nsCAutoString host;
    PRInt32 port = -1;
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
        new nsHttpConnectionInfo(host, port, nsnull, usingSSL);
    ci->SetAnonymous(false);
    PipelineFeedbackInfo(ci, RedCorruptedContent, nsnull, 0);

    ci = new nsHttpConnectionInfo(host, port, nsnull, usingSSL);
    ci->SetAnonymous(true);
    PipelineFeedbackInfo(ci, RedCorruptedContent, nsnull, 0);
}





bool
nsHttpConnectionMgr::AtActiveConnectionLimit(nsConnectionEntry *ent, PRUint8 caps)
{
    nsHttpConnectionInfo *ci = ent->mConnInfo;

    LOG(("nsHttpConnectionMgr::AtActiveConnectionLimit [ci=%s caps=%x]\n",
        ci->HashKey().get(), caps));

    
    
    
    PRUint32 maxSocketCount = gHttpHandler->MaxSocketCount();
    if (mMaxConns > maxSocketCount) {
        mMaxConns = maxSocketCount;
        LOG(("nsHttpConnectionMgr %p mMaxConns dynamically reduced to %u",
             this, mMaxConns));
    }

    
    
    if (mNumActiveConns >= mMaxConns) {
        LOG(("  num active conns == max conns\n"));
        return true;
    }

    nsHttpConnection *conn;
    PRInt32 i, totalCount, persistCount = 0;
    
    totalCount = ent->mActiveConns.Length();

    
    for (i=0; i<totalCount; ++i) {
        conn = ent->mActiveConns[i];
        if (conn->IsKeepAlive()) 
            persistCount++;
    }

    
    
    totalCount += ent->mHalfOpens.Length();
    persistCount += ent->mHalfOpens.Length();
    
    LOG(("   total=%d, persist=%d\n", totalCount, persistCount));

    PRUint16 maxConns;
    PRUint16 maxPersistConns;

    if (ci->UsingHttpProxy() && !ci->UsingSSL()) {
        maxConns = mMaxConnsPerProxy;
        maxPersistConns = mMaxPersistConnsPerProxy;
    }
    else {
        maxConns = mMaxConnsPerHost;
        maxPersistConns = mMaxPersistConnsPerHost;
    }

    
    return (totalCount >= maxConns) || ( (caps & NS_HTTP_ALLOW_KEEPALIVE) &&
                                         (persistCount >= maxPersistConns) );
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
    
    PRInt32 activeCount = ent->mActiveConns.Length();
    for (PRInt32 i=0; i < activeCount; i++)
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
    
    
    
    if (ent->mHalfOpens.Length())
        return true;

    
    
    
    if (ent->mUsingSpdy && ent->mActiveConns.Length()) {
        bool confirmedRestrict = false;
        for (PRUint32 index = 0; index < ent->mActiveConns.Length(); ++index) {
            nsHttpConnection *conn = ent->mActiveConns[index];
            if (!conn->ReportedNPN() || conn->EverUsedSpdy()) {
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

bool
nsHttpConnectionMgr::MakeNewConnection(nsConnectionEntry *ent,
                                       nsHttpTransaction *trans)
{
    LOG(("nsHttpConnectionMgr::MakeNewConnection %p ent=%p trans=%p",
         this, ent, trans));
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
        
    PRUint32 halfOpenLength = ent->mHalfOpens.Length();
    for (PRUint32 i = 0; i < halfOpenLength; i++) {
        if (ent->mHalfOpens[i]->IsSpeculative()) {
            
            
            
            
            
            LOG(("nsHttpConnectionMgr::MakeNewConnection [ci = %s]\n"
                 "Found a speculative half open connection\n",
                 ent->mConnInfo->HashKey().get()));
            ent->mHalfOpens[i]->SetSpeculative(false);

            
            
            return true;
        }
    }

    
    
    
    if (!(trans->Caps() & NS_HTTP_DISALLOW_SPDY) && RestrictConnections(ent))
        return false;

    
    
    
    
    

    if ((mNumIdleConns + mNumActiveConns + 1 >= mMaxConns) && mNumIdleConns)
        mCT.Enumerate(PurgeExcessIdleConnectionsCB, this);

    if (AtActiveConnectionLimit(ent, trans->Caps()))
        return false;

    nsresult rv = CreateTransport(ent, trans, trans->Caps(), false);
    if (NS_FAILED(rv))                            
        trans->Close(rv);

    return true;
}

bool
nsHttpConnectionMgr::AddToShortestPipeline(nsConnectionEntry *ent,
                                           nsHttpTransaction *trans,
                                           nsHttpTransaction::Classifier classification,
                                           PRUint16 depthLimit)
{
    if (classification == nsAHttpTransaction::CLASS_SOLO)
        return false;

    PRUint32 maxdepth = ent->MaxPipelineDepth(classification);
    if (maxdepth == 0) {
        ent->CreditPenalty();
        maxdepth = ent->MaxPipelineDepth(classification);
    }
    
    if (ent->PipelineState() == PS_RED)
        return false;

    if (ent->PipelineState() == PS_YELLOW && ent->mYellowConnection)
        return false;

    
    
    
    
    
    
    
    
    
    
    

    maxdepth = PR_MIN(maxdepth, depthLimit);

    if (maxdepth < 2)
        return false;

    nsAHttpTransaction *activeTrans;

    nsHttpConnection *bestConn = nsnull;
    PRUint32 activeCount = ent->mActiveConns.Length();
    PRUint32 bestConnLength = 0;
    PRUint32 connLength;

    for (PRUint32 i = 0; i < activeCount; ++i) {
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
    return true;
}

bool
nsHttpConnectionMgr::IsUnderPressure(nsConnectionEntry *ent,
                                   nsHttpTransaction::Classifier classification)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    PRInt32 currentConns = ent->mActiveConns.Length();
    PRInt32 maxConns =
        (ent->mConnInfo->UsingHttpProxy() && !ent->mConnInfo->UsingSSL()) ?
        mMaxPersistConnsPerProxy : mMaxPersistConnsPerHost;

    
    
    if (currentConns >= (maxConns - 2))
        return true;                           

    PRInt32 sameClass = 0;
    for (PRInt32 i = 0; i < currentConns; ++i)
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
         ent->mConnInfo->HashKey().get(), PRUint32(trans->Caps())));

    nsHttpTransaction::Classifier classification = trans->Classification();
    PRUint8 caps = trans->Caps();

    
    if (!(caps & NS_HTTP_ALLOW_KEEPALIVE))
        caps = caps & ~NS_HTTP_ALLOW_PIPELINING;

    
    
    
    
    
    
    
    
    
    
    
    
    

    bool attemptedOptimisticPipeline = !(caps & NS_HTTP_ALLOW_PIPELINING);

    
    
    

    if (!(caps & NS_HTTP_DISALLOW_SPDY) && gHttpHandler->IsSpdyEnabled()) {
        nsRefPtr<nsHttpConnection> conn = GetSpdyPreferredConn(ent);
        if (conn) {
            LOG(("   dispatch to spdy: [conn=%x]\n", conn.get()));
            DispatchTransaction(ent, trans, conn);
            return NS_OK;
        }
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
                conn = nsnull;
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

    
    if (!onlyReusedConnection && MakeNewConnection(ent, trans)) {
        return NS_ERROR_IN_PROGRESS;
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
    PRUint8 caps = trans->Caps();
    PRInt32 priority = trans->Priority();

    LOG(("nsHttpConnectionMgr::DispatchTransaction "
         "[ci=%s trans=%x caps=%x conn=%x priority=%d]\n",
         ent->mConnInfo->HashKey().get(), trans, caps, conn, priority));

    if (conn->UsingSpdy()) {
        LOG(("Spdy Dispatch Transaction via Activate(). Transaction host = %s,"
             "Connection host = %s\n",
             trans->ConnectionInfo()->Host(),
             conn->ConnectionInfo()->Host()));
        nsresult rv = conn->Activate(trans, caps, priority);
        NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), "SPDY Cannot Fail Dispatch");
        return rv;
    }

    NS_ABORT_IF_FALSE(conn && !conn->Transaction(),
                      "DispatchTranaction() on non spdy active connection");

    if (!(caps & NS_HTTP_ALLOW_PIPELINING))
        conn->Classify(nsAHttpTransaction::CLASS_SOLO);
    else
        conn->Classify(trans->Classification());

    return DispatchAbstractTransaction(ent, trans, caps, conn, priority);
}






nsresult
nsHttpConnectionMgr::DispatchAbstractTransaction(nsConnectionEntry *ent,
                                                 nsAHttpTransaction *aTrans,
                                                 PRUint8 caps,
                                                 nsHttpConnection *conn,
                                                 PRInt32 priority)
{
    NS_ABORT_IF_FALSE(!conn->UsingSpdy(),
                      "Spdy Must Not Use DispatchAbstractTransaction");
    LOG(("nsHttpConnectionMgr::DispatchAbstractTransaction "
         "[ci=%s trans=%x caps=%x conn=%x]\n",
         ent->mConnInfo->HashKey().get(), aTrans, caps, conn));

    



    nsRefPtr<nsHttpPipeline> pipeline;
    nsresult rv = BuildPipeline(ent, aTrans, getter_AddRefs(pipeline));
    if (!NS_SUCCEEDED(rv))
        return rv;

    nsRefPtr<nsConnectionHandle> handle = new nsConnectionHandle(conn);

    
    pipeline->SetConnection(handle);

    rv = conn->Activate(pipeline, caps, priority);
    if (NS_FAILED(rv)) {
        LOG(("  conn->Activate failed [rv=%x]\n", rv));
        ent->mActiveConns.RemoveElement(conn);
        if (conn == ent->mYellowConnection)
            ent->OnYellowComplete();
        mNumActiveConns--;
        ConditionallyStopReadTimeoutTick();

        
        
        pipeline->SetConnection(nsnull);
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

nsresult
nsHttpConnectionMgr::ProcessNewTransaction(nsHttpTransaction *trans)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    
    
    
    
    if (NS_FAILED(trans->Status())) {
        LOG(("  transaction was canceled... dropping event!\n"));
        return NS_OK;
    }

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

    
    
    if (trans->Caps() & NS_HTTP_CLEAR_KEEPALIVES)
        ClosePersistentConnections(ent);

    
    
    

    nsAHttpConnection *wrappedConnection = trans->Connection();
    nsRefPtr<nsHttpConnection> conn;
    if (wrappedConnection)
        conn = dont_AddRef(wrappedConnection->TakeHttpConnection());

    if (conn) {
        NS_ASSERTION(trans->Caps() & NS_HTTP_STICKY_CONNECTION,
                     "unexpected caps");
        NS_ABORT_IF_FALSE(((PRInt32)ent->mActiveConns.IndexOf(conn)) != -1,
                          "Sticky Connection Not In Active List");
        trans->SetConnection(nsnull);
        rv = DispatchTransaction(ent, trans, conn);
    }
    else
        rv = TryDispatchTransaction(ent, false, trans);

    if (NS_FAILED(rv)) {
        LOG(("  adding transaction to pending queue "
             "[trans=%p pending-count=%u]\n",
             trans, ent->mPendingQ.Length()+1));
        
        InsertTransactionSorted(ent->mPendingQ, trans);
        NS_ADDREF(trans);
    }

    return NS_OK;
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
}

void
nsHttpConnectionMgr::RecvdConnect()
{
    mNumActiveConns--;
    ConditionallyStopReadTimeoutTick();
}

nsresult
nsHttpConnectionMgr::CreateTransport(nsConnectionEntry *ent,
                                     nsAHttpTransaction *trans,
                                     PRUint8 caps,
                                     bool speculative)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    nsRefPtr<nsHalfOpenSocket> sock = new nsHalfOpenSocket(ent, trans, caps);
    nsresult rv = sock->SetupPrimaryStreams();
    NS_ENSURE_SUCCESS(rv, rv);

    ent->mHalfOpens.AppendElement(sock);
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

    for (PRInt32 index = ent->mPendingQ.Length() - 1;
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
nsHttpConnectionMgr::ProcessAllSpdyPendingQ()
{
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
    
    nsHttpConnection *conn = nsnull;
    
    if (preferred->mUsingSpdy) {
        for (PRUint32 index = 0;
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
nsHttpConnectionMgr::OnMsgShutdown(PRInt32, void *)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    LOG(("nsHttpConnectionMgr::OnMsgShutdown\n"));

    mCT.Enumerate(ShutdownPassCB, this);

    if (mReadTimeoutTick) {
        mReadTimeoutTick->Cancel();
        mReadTimeoutTick = nsnull;
        mReadTimeoutTickArmed = false;
    }
    
    
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    mon.Notify();
}

void
nsHttpConnectionMgr::OnMsgNewTransaction(PRInt32 priority, void *param)
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
nsHttpConnectionMgr::OnMsgReschedTransaction(PRInt32 priority, void *param)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    LOG(("nsHttpConnectionMgr::OnMsgReschedTransaction [trans=%p]\n", param));

    nsHttpTransaction *trans = (nsHttpTransaction *) param;
    trans->SetPriority(priority);

    nsConnectionEntry *ent = LookupConnectionEntry(trans->ConnectionInfo(),
                                                   nsnull, trans);

    if (ent) {
        PRInt32 index = ent->mPendingQ.IndexOf(trans);
        if (index >= 0) {
            ent->mPendingQ.RemoveElementAt(index);
            InsertTransactionSorted(ent->mPendingQ, trans);
        }
    }

    NS_RELEASE(trans);
}

void
nsHttpConnectionMgr::OnMsgCancelTransaction(PRInt32 reason, void *param)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    LOG(("nsHttpConnectionMgr::OnMsgCancelTransaction [trans=%p]\n", param));

    nsHttpTransaction *trans = (nsHttpTransaction *) param;
    
    
    
    
    
    nsAHttpConnection *conn = trans->Connection();
    if (conn && !trans->IsDone())
        conn->CloseTransaction(trans, reason);
    else {
        nsConnectionEntry *ent = LookupConnectionEntry(trans->ConnectionInfo(),
                                                       nsnull, trans);

        if (ent) {
            PRInt32 index = ent->mPendingQ.IndexOf(trans);
            if (index >= 0) {
                ent->mPendingQ.RemoveElementAt(index);
                nsHttpTransaction *temp = trans;
                NS_RELEASE(temp); 
            }
        }
        trans->Close(reason);
    }
    NS_RELEASE(trans);
}

void
nsHttpConnectionMgr::OnMsgProcessPendingQ(PRInt32, void *param)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    nsHttpConnectionInfo *ci = (nsHttpConnectionInfo *) param;

    LOG(("nsHttpConnectionMgr::OnMsgProcessPendingQ [ci=%s]\n", ci->HashKey().get()));

    
    nsConnectionEntry *ent = mCT.Get(ci->HashKey());
    if (!(ent && ProcessPendingQForEntry(ent))) {
        
        
        mCT.Enumerate(ProcessOneTransactionCB, this);
    }

    NS_RELEASE(ci);
}

void
nsHttpConnectionMgr::OnMsgPruneDeadConnections(PRInt32, void *)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    LOG(("nsHttpConnectionMgr::OnMsgPruneDeadConnections\n"));

    
    mTimeOfNextWakeUp = LL_MAXUINT;

    
    
    if (mNumIdleConns || (mNumActiveConns && gHttpHandler->IsSpdyEnabled()))
        mCT.Enumerate(PruneDeadConnectionsCB, this);
}

void
nsHttpConnectionMgr::OnMsgClosePersistentConnections(PRInt32, void *)
{
    LOG(("nsHttpConnectionMgr::OnMsgClosePersistentConnections\n"));

    mCT.Enumerate(ClosePersistentConnectionsCB, this);
}

void
nsHttpConnectionMgr::OnMsgReclaimConnection(PRInt32, void *param)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    LOG(("nsHttpConnectionMgr::OnMsgReclaimConnection [conn=%p]\n", param));

    nsHttpConnection *conn = (nsHttpConnection *) param;

    
    
    
    
    

    nsConnectionEntry *ent = LookupConnectionEntry(conn->ConnectionInfo(),
                                                   conn, nsnull);
    nsHttpConnectionInfo *ci = nsnull;

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
            ConditionallyStopReadTimeoutTick();
        }

        if (conn->CanReuse()) {
            LOG(("  adding connection to idle list\n"));
            
            
            

            
            

            PRUint32 idx;
            for (idx = 0; idx < ent->mIdleConns.Length(); idx++) {
                nsHttpConnection *idleConn = ent->mIdleConns[idx];
                if (idleConn->MaxBytesRead() < conn->MaxBytesRead())
                    break;
            }

            NS_ADDREF(conn);
            ent->mIdleConns.InsertElementAt(idx, conn);
            mNumIdleConns++;
            conn->BeginIdleMonitoring();

            
            
            
            PRUint32 timeToLive = conn->TimeToLive();
            if(!mTimer || NowInSeconds() + timeToLive < mTimeOfNextWakeUp)
                PruneDeadConnectionsAfter(timeToLive);
        }
        else {
            LOG(("  connection cannot be reused; closing connection\n"));
            conn->Close(NS_ERROR_ABORT);
        }
    }
 
    OnMsgProcessPendingQ(NS_OK, ci); 
    NS_RELEASE(conn);
}

void
nsHttpConnectionMgr::OnMsgCompleteUpgrade(PRInt32, void *param)
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
nsHttpConnectionMgr::OnMsgUpdateParam(PRInt32, void *param)
{
    PRUint16 name  = (NS_PTR_TO_INT32(param) & 0xFFFF0000) >> 16;
    PRUint16 value =  NS_PTR_TO_INT32(param) & 0x0000FFFF;

    switch (name) {
    case MAX_CONNECTIONS:
        mMaxConns = value;
        break;
    case MAX_CONNECTIONS_PER_HOST:
        mMaxConnsPerHost = value;
        break;
    case MAX_CONNECTIONS_PER_PROXY:
        mMaxConnsPerProxy = value;
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
nsHttpConnectionMgr::OnMsgProcessFeedback(PRInt32, void *param)
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
         "this=%p mReadTimeoutTick=%p\n"));

    
    
    

    if (mReadTimeoutTick && mReadTimeoutTickArmed)
        return;

    if (!mReadTimeoutTick) {
        mReadTimeoutTick = do_CreateInstance(NS_TIMER_CONTRACTID);
        if (!mReadTimeoutTick) {
            NS_WARNING("failed to create timer for http timeout management");
            return;
        }
        mReadTimeoutTick->SetTarget(mSocketThreadTarget);
    }

    NS_ABORT_IF_FALSE(!mReadTimeoutTickArmed, "timer tick armed");
    mReadTimeoutTickArmed = true;
    mReadTimeoutTick->Init(this, 1000, nsITimer::TYPE_REPEATING_SLACK);
}

void
nsHttpConnectionMgr::ReadTimeoutTick()
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ABORT_IF_FALSE(mReadTimeoutTick, "no readtimeout tick");

    LOG(("nsHttpConnectionMgr::ReadTimeoutTick active=%d\n",
         mNumActiveConns));

    mCT.Enumerate(ReadTimeoutTickCB, this);
}

PLDHashOperator
nsHttpConnectionMgr::ReadTimeoutTickCB(const nsACString &key,
                                       nsAutoPtr<nsConnectionEntry> &ent,
                                       void *closure)
{
    nsHttpConnectionMgr *self = (nsHttpConnectionMgr *) closure;

    LOG(("nsHttpConnectionMgr::ReadTimeoutTickCB() this=%p host=%s\n",
         self, ent->mConnInfo->Host()));

    PRIntervalTime now = PR_IntervalNow();
    for (PRUint32 index = 0; index < ent->mActiveConns.Length(); ++index)
        ent->mActiveConns[index]->ReadTimeoutTick(now);

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

nsresult
nsHttpConnectionMgr::nsConnectionHandle::ResumeSend()
{
    return mConn->ResumeSend();
}

nsresult
nsHttpConnectionMgr::nsConnectionHandle::ResumeRecv()
{
    return mConn->ResumeRecv();
}

void
nsHttpConnectionMgr::nsConnectionHandle::CloseTransaction(nsAHttpTransaction *trans, nsresult reason)
{
    mConn->CloseTransaction(trans, reason);
}

void
nsHttpConnectionMgr::nsConnectionHandle::GetConnectionInfo(nsHttpConnectionInfo **result)
{
    mConn->GetConnectionInfo(result);
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
nsHttpConnectionMgr::OnMsgSpeculativeConnect(PRInt32, void *param)
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

    if (!ent->mIdleConns.Length() && !RestrictConnections(ent) &&
        !AtActiveConnectionLimit(ent, trans->Caps())) {
        CreateTransport(ent, trans, trans->Caps(), true);
    }
}

void
nsHttpConnectionMgr::nsConnectionHandle::GetSecurityInfo(nsISupports **result)
{
    mConn->GetSecurityInfo(result);
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
nsHttpConnectionMgr::nsConnectionHandle::PushBack(const char *buf, PRUint32 bufLen)
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
                                   PRUint8 caps)
    : mEnt(ent),
      mTransaction(trans),
      mCaps(caps),
      mSpeculative(false)
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
    
    if (mEnt) {
        
        
        
        bool restrictedBeforeRelease =
            gHttpHandler->ConnMgr()->RestrictConnections(mEnt);

        
        
        
        mEnt->mHalfOpens.RemoveElement(this);

        if (restrictedBeforeRelease &&
            !gHttpHandler->ConnMgr()->RestrictConnections(mEnt)) {
            LOG(("nsHalfOpenSocket %p lifted RestrictConnections() limit.\n"));
            gHttpHandler->ConnMgr()->ProcessPendingQForEntry(mEnt);
        }
    }
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
    PRUint32 typeCount = (types[0] != nsnull);

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
    
    PRUint32 tmpFlags = 0;
    if (mCaps & NS_HTTP_REFRESH_DNS)
        tmpFlags = nsISocketTransport::BYPASS_CACHE;

    if (mCaps & NS_HTTP_LOAD_ANONYMOUS)
        tmpFlags |= nsISocketTransport::ANONYMOUS_CONNECT;

    
    
    
    
    
    if (isBackup && gHttpHandler->FastFallbackToIPv4())
        tmpFlags |= nsISocketTransport::DISABLE_IPV6;

    socketTransport->SetConnectionFlags(tmpFlags);

    socketTransport->SetQoSBits(gHttpHandler->GetQoSBits());

    rv = socketTransport->SetEventSink(this, nsnull);
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

    rv = (*outstream)->AsyncWait(this, 0, 0, nsnull);
    if (NS_SUCCEEDED(rv))
        gHttpHandler->ConnMgr()->StartedConnect();

    return rv;
}

nsresult
nsHttpConnectionMgr::nsHalfOpenSocket::SetupPrimaryStreams()
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    nsresult rv;

    mPrimarySynStarted = mozilla::TimeStamp::Now();
    rv = SetupStreams(getter_AddRefs(mSocketTransport),
                      getter_AddRefs(mStreamIn),
                      getter_AddRefs(mStreamOut),
                      false);
    LOG(("nsHalfOpenSocket::SetupPrimaryStream [this=%p ent=%s rv=%x]",
         this, mEnt->mConnInfo->Host(), rv));
    if (NS_FAILED(rv)) {
        if (mStreamOut)
            mStreamOut->AsyncWait(nsnull, 0, 0, nsnull);
        mStreamOut = nsnull;
        mStreamIn = nsnull;
        mSocketTransport = nsnull;
    }
    return rv;
}

nsresult
nsHttpConnectionMgr::nsHalfOpenSocket::SetupBackupStreams()
{
    mBackupSynStarted = mozilla::TimeStamp::Now();
    nsresult rv = SetupStreams(getter_AddRefs(mBackupTransport),
                               getter_AddRefs(mBackupStreamIn),
                               getter_AddRefs(mBackupStreamOut),
                               true);
    LOG(("nsHalfOpenSocket::SetupBackupStream [this=%p ent=%s rv=%x]",
         this, mEnt->mConnInfo->Host(), rv));
    if (NS_FAILED(rv)) {
        if (mBackupStreamOut)
            mBackupStreamOut->AsyncWait(nsnull, 0, 0, nsnull);
        mBackupStreamOut = nsnull;
        mBackupStreamIn = nsnull;
        mBackupTransport = nsnull;
    }
    return rv;
}

void
nsHttpConnectionMgr::nsHalfOpenSocket::SetupBackupTimer()
{
    PRUint16 timeout = gHttpHandler->GetIdleSynTimeout();
    NS_ABORT_IF_FALSE(!mSynTimer, "timer already initd");
    
    if (timeout && !mTransaction->IsDone()) {
        
        
        
        
        
        
        
        nsresult rv;
        mSynTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
        if (NS_SUCCEEDED(rv)) {
            mSynTimer->InitWithCallback(this, timeout, nsITimer::TYPE_ONE_SHOT);
            LOG(("nsHalfOpenSocket::SetupBackupTimer()"));
        }
    }
}

void
nsHttpConnectionMgr::nsHalfOpenSocket::CancelBackupTimer()
{
    
    
    if (!mSynTimer)
        return;

    LOG(("nsHalfOpenSocket::CancelBackupTimer()"));
    mSynTimer->Cancel();
    mSynTimer = nsnull;
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
        mStreamOut->AsyncWait(nsnull, 0, 0, nsnull);
        mStreamOut = nsnull;
    }
    if (mBackupStreamOut) {
        gHttpHandler->ConnMgr()->RecvdConnect();
        mBackupStreamOut->AsyncWait(nsnull, 0, 0, nsnull);
        mBackupStreamOut = nsnull;
    }

    CancelBackupTimer();

    mEnt = nsnull;
}

NS_IMETHODIMP 
nsHttpConnectionMgr::nsHalfOpenSocket::Notify(nsITimer *timer)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ABORT_IF_FALSE(timer == mSynTimer, "wrong timer");

    if (!gHttpHandler->ConnMgr()->
        AtActiveConnectionLimit(mEnt, mCaps)) {
        SetupBackupStreams();
    }

    mSynTimer = nsnull;
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
    PRInt32 index;
    nsresult rv;
    
    gHttpHandler->ConnMgr()->RecvdConnect();

    CancelBackupTimer();

    
    nsRefPtr<nsHttpConnection> conn = new nsHttpConnection();
    LOG(("nsHalfOpenSocket::OnOutputStreamReady "
         "Created new nshttpconnection %p\n", conn.get()));

    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    nsCOMPtr<nsIEventTarget>        callbackTarget;
    mTransaction->GetSecurityCallbacks(getter_AddRefs(callbacks),
                                       getter_AddRefs(callbackTarget));
    if (out == mStreamOut) {
        mozilla::TimeDuration rtt = 
            mozilla::TimeStamp::Now() - mPrimarySynStarted;
        rv = conn->Init(mEnt->mConnInfo,
                        gHttpHandler->ConnMgr()->mMaxRequestDelay,
                        mSocketTransport, mStreamIn, mStreamOut,
                        callbacks, callbackTarget,
                        PR_MillisecondsToInterval(rtt.ToMilliseconds()));

        
        mStreamOut = nsnull;
        mStreamIn = nsnull;
        mSocketTransport = nsnull;
    }
    else {
        mozilla::TimeDuration rtt = 
            mozilla::TimeStamp::Now() - mBackupSynStarted;
        
        rv = conn->Init(mEnt->mConnInfo,
                        gHttpHandler->ConnMgr()->mMaxRequestDelay,
                        mBackupTransport, mBackupStreamIn, mBackupStreamOut,
                        callbacks, callbackTarget,
                        PR_MillisecondsToInterval(rtt.ToMilliseconds()));

        
        mBackupStreamOut = nsnull;
        mBackupStreamIn = nsnull;
        mBackupTransport = nsnull;
    }

    if (NS_FAILED(rv)) {
        LOG(("nsHalfOpenSocket::OnOutputStreamReady "
             "conn->init (%p) failed %x\n", conn.get(), rv));
        return rv;
    }

    
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

        
        
        
        
        if (mEnt->mConnInfo->UsingSSL() && !mEnt->mPendingQ.Length()) {
            LOG(("nsHalfOpenSocket::OnOutputStreamReady null transaction will "
                 "be used to finish SSL handshake on conn %p\n", conn.get()));
            nsRefPtr<NullHttpTransaction>  trans =
                new NullHttpTransaction(mEnt->mConnInfo,
                                        callbacks, callbackTarget,
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
                NS_OK, conn.forget().get());
        }
    }

    return rv;
}


NS_IMETHODIMP
nsHttpConnectionMgr::nsHalfOpenSocket::OnTransportStatus(nsITransport *trans,
                                                         nsresult status,
                                                         PRUint64 progress,
                                                         PRUint64 progressMax)
{
    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (mTransaction)
        mTransaction->OnTransportStatus(trans, status, progress);

    if (trans != mSocketTransport)
        return NS_OK;

    
    
    
    

    if (status == nsISocketTransport::STATUS_CONNECTED_TO &&
        gHttpHandler->IsSpdyEnabled() &&
        gHttpHandler->CoalesceSpdy() &&
        mEnt && mEnt->mConnInfo && mEnt->mConnInfo->UsingSSL() &&
        !mEnt->mConnInfo->UsingHttpProxy() &&
        mEnt->mCoalescingKey.IsEmpty()) {

        PRNetAddr addr;
        nsresult rv = mSocketTransport->GetPeerAddr(&addr);
        if (NS_SUCCEEDED(rv)) {
            mEnt->mCoalescingKey.SetCapacity(72);
            PR_NetAddrToString(&addr, mEnt->mCoalescingKey.BeginWriting(), 64);
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
    case nsISocketTransport::STATUS_CONNECTING_TO:
        
        
        
        
        
        
        
        
        if (mEnt && !mBackupTransport && !mSynTimer)
            SetupBackupTimer();
        break;

    case nsISocketTransport::STATUS_CONNECTED_TO:
        
        
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
        mTransaction->GetSecurityCallbacks(getter_AddRefs(callbacks), nsnull);
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
    mConn = nsnull;
    return conn;
}

bool
nsHttpConnectionMgr::nsConnectionHandle::IsProxyConnectInProgress()
{
    return mConn->IsProxyConnectInProgress();
}

PRUint32
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

void
nsHttpConnectionMgr::
nsConnectionHandle::Classify(nsAHttpTransaction::Classifier newclass)
{
    if (mConn)
        mConn->Classify(newclass);
}



nsHttpConnectionMgr::
nsConnectionEntry::nsConnectionEntry(nsHttpConnectionInfo *ci)
    : mConnInfo(ci)
    , mPipelineState(PS_YELLOW)
    , mYellowGoodEvents(0)
    , mYellowBadEvents(0)
    , mYellowConnection(nsnull)
    , mGreenDepth(kPipelineOpen)
    , mPipeliningPenalty(0)
    , mUsingSpdy(false)
    , mTestedSpdy(false)
    , mSpdyPreferred(false)
{
    NS_ADDREF(mConnInfo);
    if (gHttpHandler->GetPipelineAggressive()) {
        mGreenDepth = kPipelineUnlimited;
        mPipelineState = PS_GREEN;
    }
    mInitialGreenDepth = mGreenDepth;
    memset(mPipeliningClassPenalty, 0, sizeof(PRInt16) * nsAHttpTransaction::CLASS_MAX);
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
    PRUint32 data)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    
    if (mPipelineState == PS_YELLOW) {
        if (info & kPipelineInfoTypeBad)
            mYellowBadEvents++;
        else if (info & (kPipelineInfoTypeNeutral | kPipelineInfoTypeGood))
            mYellowGoodEvents++;
    }
    
    if (mPipelineState == PS_GREEN && info == GoodCompletedOK) {
        PRInt32 depth = data;
        LOG(("Transaction completed at pipeline depty of %d. Host = %s\n",
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
            mLastCreditTime = mozilla::TimeStamp::Now();

        
        
        
        
        
        
        

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
        
        mPipeliningPenalty = PR_MIN(mPipeliningPenalty, 25000);
        mPipeliningClassPenalty[classification] = PR_MIN(mPipeliningClassPenalty[classification], 25000);
            
        LOG(("Assessing red penalty to %s class %d for event %d. "
             "Penalty now %d, throttle[%d] = %d\n", mConnInfo->Host(),
             classification, info, mPipeliningPenalty, classification,
             mPipeliningClassPenalty[classification]));
    }
    else {
        
        

        mPipeliningPenalty = PR_MAX(mPipeliningPenalty - 1, 0);
        mPipeliningClassPenalty[classification] = PR_MAX(mPipeliningClassPenalty[classification] - 1, 0);
    }

    if (mPipelineState == PS_RED && !mPipeliningPenalty)
    {
        LOG(("transition %s to yellow\n", mConnInfo->Host()));
        mPipelineState = PS_YELLOW;
        mYellowConnection = nsnull;
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
nsHttpConnectionMgr::nsConnectionEntry::OnYellowComplete()
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

    mYellowConnection = nsnull;
}

void
nsHttpConnectionMgr::nsConnectionEntry::CreditPenalty()
{
    if (mLastCreditTime.IsNull())
        return;
    
    
    

    mozilla::TimeStamp now = mozilla::TimeStamp::Now();
    mozilla::TimeDuration elapsedTime = now - mLastCreditTime;
    PRUint32 creditsEarned =
        static_cast<PRUint32>(elapsedTime.ToSeconds()) >> 4;
    
    bool failed = false;
    if (creditsEarned > 0) {
        mPipeliningPenalty = 
            PR_MAX(PRInt32(mPipeliningPenalty - creditsEarned), 0);
        if (mPipeliningPenalty > 0)
            failed = true;
        
        for (PRInt32 i = 0; i < nsAHttpTransaction::CLASS_MAX; ++i) {
            mPipeliningClassPenalty[i]  =
                PR_MAX(PRInt32(mPipeliningClassPenalty[i] - creditsEarned), 0);
            failed = failed || (mPipeliningClassPenalty[i] > 0);
        }

        
        mLastCreditTime +=
            mozilla::TimeDuration::FromSeconds(creditsEarned << 4);
    }
    else {
        failed = true;                         
    }

    
    
    if (!failed)
        mLastCreditTime = mozilla::TimeStamp();    

    if (mPipelineState == PS_RED && !mPipeliningPenalty)
    {
        LOG(("transition %s to yellow based on time credit\n",
             mConnInfo->Host()));
        mPipelineState = PS_YELLOW;
        mYellowConnection = nsnull;
    }    
}

PRUint32
nsHttpConnectionMgr::
nsConnectionEntry::MaxPipelineDepth(nsAHttpTransaction::Classifier aClass)
{
    
    
    if ((mPipelineState == PS_RED) || (mPipeliningClassPenalty[aClass] > 0))
        return 0;

    if (mPipelineState == PS_YELLOW)
        return kPipelineRestricted;

    return mGreenDepth;
}

bool
nsHttpConnectionMgr::nsConnectionHandle::LastTransactionExpectedNoContent()
{
    return mConn->LastTransactionExpectedNoContent();
}

void
nsHttpConnectionMgr::
nsConnectionHandle::SetLastTransactionExpectedNoContent(bool val)
{
     mConn->SetLastTransactionExpectedNoContent(val);
}

nsISocketTransport *
nsHttpConnectionMgr::nsConnectionHandle::Transport()
{
    if (!mConn)
        return nsnull;
    return mConn->Transport();
}
