





































#include "nsHttpConnectionMgr.h"
#include "nsHttpConnection.h"
#include "nsHttpPipeline.h"
#include "nsHttpHandler.h"
#include "nsNetCID.h"
#include "nsCOMPtr.h"
#include "nsNetUtil.h"

#include "nsIServiceManager.h"

#include "nsIObserverService.h"
#include "nsIDNSRecord.h"
#include "nsIDNSService.h"
#include "nsICancelable.h"

using namespace mozilla;


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
{
    LOG(("Creating nsHttpConnectionMgr @%x\n", this));
    mCT.Init();
    mAlternateProtocolHash.Init(16);
}

nsHttpConnectionMgr::~nsHttpConnectionMgr()
{
    LOG(("Destroying nsHttpConnectionMgr @%x\n", this));
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
                          PRUint16 maxPipelinedRequests)
{
    LOG(("nsHttpConnectionMgr::Init\n"));

    {
        ReentrantMonitorAutoEnter mon(mReentrantMonitor);
        mSpdyPreferredHash.Init();

        mMaxConns = maxConns;
        mMaxConnsPerHost = maxConnsPerHost;
        mMaxConnsPerProxy = maxConnsPerProxy;
        mMaxPersistConnsPerHost = maxPersistConnsPerHost;
        mMaxPersistConnsPerProxy = maxPersistConnsPerProxy;
        mMaxRequestDelay = maxRequestDelay;
        mMaxPipelinedRequests = maxPipelinedRequests;

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
        if (!event)
            rv = NS_ERROR_OUT_OF_MEMORY;
        else
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





NS_IMETHODIMP
nsHttpConnectionMgr::Observe(nsISupports *subject,
                             const char *topic,
                             const PRUnichar *data)
{
    LOG(("nsHttpConnectionMgr::Observe [topic=\"%s\"]\n", topic));

    if (0 == strcmp(topic, "timer-callback")) {
        
        PruneDeadConnections();
#ifdef DEBUG
        nsCOMPtr<nsITimer> timer = do_QueryInterface(subject);
        NS_ASSERTION(timer == mTimer, "unexpected timer-callback");
#endif
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
nsHttpConnectionMgr::GetSocketThreadTarget(nsIEventTarget **target)
{
    
    
    
    
    EnsureSocketThreadTargetIfOnline();

    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    NS_IF_ADDREF(*target = mSocketThreadTarget);
    return NS_OK;
}

void
nsHttpConnectionMgr::AddTransactionToPipeline(nsHttpPipeline *pipeline)
{
    LOG(("nsHttpConnectionMgr::AddTransactionToPipeline [pipeline=%x]\n", pipeline));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    nsRefPtr<nsHttpConnectionInfo> ci;
    pipeline->GetConnectionInfo(getter_AddRefs(ci));
    if (ci) {
        nsConnectionEntry *ent = mCT.Get(ci->HashKey());
        if (ent) {
            
            PRInt32 i, count = ent->mPendingQ.Length();
            for (i=0; i<count; ++i) {
                nsHttpTransaction *trans = ent->mPendingQ[i];
                if (trans->Caps() & NS_HTTP_ALLOW_PIPELINING) {
                    pipeline->AddTransaction(trans);

                    
                    ent->mPendingQ.RemoveElementAt(i);
                    NS_RELEASE(trans);
                    break;
                }
            }
        }
    }
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

nsresult
nsHttpConnectionMgr::CloseIdleConnection(nsHttpConnection *conn)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    LOG(("nsHttpConnectionMgr::CloseIdleConnection %p conn=%p",
         this, conn));

    nsHttpConnectionInfo *ci = conn->ConnectionInfo();
    if (!ci)
        return NS_ERROR_UNEXPECTED;

    nsConnectionEntry *ent = mCT.Get(ci->HashKey());

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
    
    nsConnectionEntry *ent = mCT.Get(conn->ConnectionInfo()->HashKey());
    NS_ABORT_IF_FALSE(ent, "no connection entry");
    if (!ent)
        return;

    ent->mTestedSpdy = true;

    if (!usingSpdy) {
        if (ent->mUsingSpdy)
            conn->DontReuse();
        return;
    }
    
    ent->mUsingSpdy = true;

    PRUint32 ttl = conn->TimeToLive();
    PRUint64 timeOfExpire = NowInSeconds() + ttl;
    if (!mTimer || timeOfExpire < mTimeOfNextWakeUp)
        PruneDeadConnectionsAfter(ttl);
        
    nsConnectionEntry *preferred = GetSpdyPreferred(ent->mDottedDecimalAddress);
    LOG(("ReportSpdyConnection %s %s ent=%p ispreferred=%d\n",
         ent->mConnInfo->Host(), ent->mDottedDecimalAddress.get(),
         ent, preferred));
    
    if (!preferred) {
        ent->mSpdyPreferred = true;
        SetSpdyPreferred(ent->mDottedDecimalAddress, ent);
        ent->mSpdyRedir = false;
    }
    else if (preferred != ent) {
        
        
        ent->mSpdyRedir = true;
        conn->DontReuse();
    }
    ProcessSpdyPendingQ();
}

bool
nsHttpConnectionMgr::GetSpdyAlternateProtocol(nsACString &hostPortKey)
{
    
    
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    return mAlternateProtocolHash.Contains(hostPortKey);
}

void
nsHttpConnectionMgr::ReportSpdyAlternateProtocol(nsHttpConnection *conn)
{
    
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
    
    if (mAlternateProtocolHash.mHashTable.entryCount > 2000)
        PL_DHashTableEnumerate(&mAlternateProtocolHash.mHashTable,
                               &nsHttpConnectionMgr::TrimAlternateProtocolHash,
                               this);
    
    mAlternateProtocolHash.Put(hostPortKey);
}

void
nsHttpConnectionMgr::RemoveSpdyAlternateProtocol(nsACString &hostPortKey)
{
    
    
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);

    return mAlternateProtocolHash.Remove(hostPortKey);
}

PLDHashOperator
nsHttpConnectionMgr::TrimAlternateProtocolHash(PLDHashTable *table,
                                               PLDHashEntryHdr *hdr,
                                               PRUint32 number,
                                               void *closure)
{
    nsHttpConnectionMgr *self = (nsHttpConnectionMgr *) closure;
    
    if (self->mAlternateProtocolHash.mHashTable.entryCount > 2000)
        return PL_DHASH_REMOVE;
    return PL_DHASH_STOP;
}

nsHttpConnectionMgr::nsConnectionEntry *
nsHttpConnectionMgr::GetSpdyPreferred(nsACString &aDottedDecimal)
{
    if (!gHttpHandler->IsSpdyEnabled() ||
        !gHttpHandler->CoalesceSpdy() ||
        aDottedDecimal.IsEmpty())
        return nsnull;

    return mSpdyPreferredHash.Get(aDottedDecimal);
}

void
nsHttpConnectionMgr::SetSpdyPreferred(nsACString &aDottedDecimal,
                                      nsConnectionEntry *ent)
{
    if (!gHttpHandler->CoalesceSpdy())
        return;

    if (aDottedDecimal.IsEmpty())
        return;
    
    mSpdyPreferredHash.Put(aDottedDecimal, ent);
}

void
nsHttpConnectionMgr::RemoveSpdyPreferred(nsACString &aDottedDecimal)
{
    if (!gHttpHandler->CoalesceSpdy())
        return;

    if (aDottedDecimal.IsEmpty())
        return;
    
    mSpdyPreferredHash.Remove(aDottedDecimal);
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

    
    
    bool liveConnections = false;
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
                liveConnections = true;
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
                    liveConnections = true;
                }
            }
        }
    }
    
    
    
    if (liveConnections) {
        PRUint32 now = NowInSeconds();
        PRUint64 timeOfNextExpire = now + timeToNextExpire;
        
        
        
        
        if (!self->mTimer || timeOfNextExpire < self->mTimeOfNextWakeUp) {
            self->PruneDeadConnectionsAfter(timeToNextExpire);
        }
    } else {
        self->ConditionallyStopPruneDeadConnectionsTimer();
    }
#ifdef DEBUG
    count = ent->mActiveConns.Length();
    if (count > 0) {
        for (PRInt32 i=count-1; i>=0; --i) {
            nsHttpConnection *conn = ent->mActiveConns[i];
            LOG(("    active conn [%x] with trans [%x]\n", conn, conn->Transaction()));
        }
    }
#endif

    
    if (ent->mIdleConns.Length()   == 0 &&
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

    if (gHttpHandler->IsSpdyEnabled())
        ProcessSpdyPendingQ(ent);

    PRUint32 i, count = ent->mPendingQ.Length();
    if (count > 0) {
        LOG(("  pending-count=%u\n", count));
        nsHttpTransaction *trans = nsnull;
        nsHttpConnection *conn = nsnull;
        for (i = 0; i < count; ++i) {
            trans = ent->mPendingQ[i];

            
            
            
            
            
            bool alreadyHalfOpen = false;
            for (PRInt32 j = 0; j < ((PRInt32) ent->mHalfOpens.Length()); j++) {
                if (ent->mHalfOpens[j]->Transaction() == trans) {
                    alreadyHalfOpen = true;
                    break;
                }
            }

            GetConnection(ent, trans, alreadyHalfOpen, &conn);
            if (conn)
                break;

            
            
            if (count != ent->mPendingQ.Length()) {
                count = ent->mPendingQ.Length();
                i = 0;
            }
        }
        if (conn) {
            LOG(("  dispatching pending transaction...\n"));

            
            ent->mPendingQ.RemoveElementAt(i);

            nsresult rv = DispatchTransaction(ent, trans, trans->Caps(), conn);
            if (NS_SUCCEEDED(rv))
                NS_RELEASE(trans);
            else {
                LOG(("  DispatchTransaction failed [rv=%x]\n", rv));
                
                ent->mPendingQ.InsertElementAt(i, trans);
                
                conn->Close(rv);
            }

            NS_RELEASE(conn);
            return true;
        }
    }
    return false;
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

void
nsHttpConnectionMgr::GetConnection(nsConnectionEntry *ent,
                                   nsHttpTransaction *trans,
                                   bool onlyReusedConnection,
                                   nsHttpConnection **result)
{
    LOG(("nsHttpConnectionMgr::GetConnection [ci=%s caps=%x]\n",
        ent->mConnInfo->HashKey().get(), PRUint32(trans->Caps())));

    
    
    
    
    

    *result = nsnull;

    nsHttpConnection *conn = nsnull;
    bool addConnToActiveList = true;

    if (trans->Caps() & NS_HTTP_ALLOW_KEEPALIVE) {

        conn = GetSpdyPreferredConn(ent);
        if (conn)
            addConnToActiveList = false;

        
        
        
        while (!conn && (ent->mIdleConns.Length() > 0)) {
            conn = ent->mIdleConns[0];
            
            
            if (!conn->CanReuse()) {
                LOG(("   dropping stale connection: [conn=%x]\n", conn));
                conn->Close(NS_ERROR_ABORT);
                NS_RELEASE(conn);
            }
            else {
                LOG(("   reusing connection [conn=%x]\n", conn));
                conn->EndIdleMonitoring();
            }

            ent->mIdleConns.RemoveElementAt(0);
            mNumIdleConns--;

            
            
            ConditionallyStopPruneDeadConnectionsTimer();
        }
    }

    if (!conn) {

        
        
        if (onlyReusedConnection)
            return;
        
        
        
        
    
        if (gHttpHandler->IsSpdyEnabled() &&
            ent->mConnInfo->UsingSSL() &&
            !ent->mConnInfo->UsingHttpProxy())
        {
            nsConnectionEntry *preferred =
                GetSpdyPreferred(ent->mDottedDecimalAddress);
            if (preferred)
                ent = preferred;

            if ((!ent->mTestedSpdy || ent->mUsingSpdy) &&
                (ent->mSpdyRedir || ent->mHalfOpens.Length() ||
                 ent->mActiveConns.Length()))
                return;
        }
        
        
        
        
        
        
        
        if (mNumIdleConns && mNumIdleConns + mNumActiveConns + 1 >= mMaxConns)
            mCT.Enumerate(PurgeExcessIdleConnectionsCB, this);

        
        
        
        if (AtActiveConnectionLimit(ent, trans->Caps())) {
            LOG(("nsHttpConnectionMgr::GetConnection [ci = %s]"
                 "at active connection limit - will queue\n",
                 ent->mConnInfo->HashKey().get()));
            return;
        }

        LOG(("nsHttpConnectionMgr::GetConnection Open Connection "
             "%s %s ent=%p spdy=%d",
             ent->mConnInfo->Host(), ent->mDottedDecimalAddress.get(),
             ent, ent->mUsingSpdy));
        
        nsresult rv = CreateTransport(ent, trans);
        if (NS_FAILED(rv))
            trans->Close(rv);
        return;
    }

    if (addConnToActiveList) {
        
        ent->mActiveConns.AppendElement(conn);
        mNumActiveConns++;
    }
    
    NS_ADDREF(conn);
    *result = conn;
}

void
nsHttpConnectionMgr::AddActiveConn(nsHttpConnection *conn,
                                   nsConnectionEntry *ent)
{
    NS_ADDREF(conn);
    ent->mActiveConns.AppendElement(conn);
    mNumActiveConns++;
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
}

nsresult
nsHttpConnectionMgr::CreateTransport(nsConnectionEntry *ent,
                                     nsHttpTransaction *trans)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    nsRefPtr<nsHalfOpenSocket> sock = new nsHalfOpenSocket(ent, trans);
    nsresult rv = sock->SetupPrimaryStreams();
    NS_ENSURE_SUCCESS(rv, rv);

    ent->mHalfOpens.AppendElement(sock);
    return NS_OK;
}

nsresult
nsHttpConnectionMgr::DispatchTransaction(nsConnectionEntry *ent,
                                         nsHttpTransaction *aTrans,
                                         PRUint8 caps,
                                         nsHttpConnection *conn)
{
    LOG(("nsHttpConnectionMgr::DispatchTransaction [ci=%s trans=%x caps=%x conn=%x]\n",
        ent->mConnInfo->HashKey().get(), aTrans, caps, conn));
    nsresult rv;
    
    PRInt32 priority = aTrans->Priority();

    if (conn->UsingSpdy()) {
        rv = conn->Activate(aTrans, caps, priority);
        NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), "SPDY Cannot Fail Dispatch");
        return rv;
    }

    nsConnectionHandle *handle = new nsConnectionHandle(conn);
    if (!handle)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(handle);

    nsHttpPipeline *pipeline = nsnull;
    nsAHttpTransaction *trans = aTrans;

    if (conn->SupportsPipelining() && (caps & NS_HTTP_ALLOW_PIPELINING)) {
        LOG(("  looking to build pipeline...\n"));
        if (BuildPipeline(ent, trans, &pipeline))
            trans = pipeline;
    }

    
    trans->SetConnection(handle);

    rv = conn->Activate(trans, caps, priority);

    if (NS_FAILED(rv)) {
        LOG(("  conn->Activate failed [rv=%x]\n", rv));
        ent->mActiveConns.RemoveElement(conn);
        mNumActiveConns--;
        
        
        trans->SetConnection(nsnull);
        NS_RELEASE(handle->mConn);
        
        NS_RELEASE(conn);
    }

    
    
    
    NS_IF_RELEASE(pipeline);

    NS_RELEASE(handle);
    return rv;
}

bool
nsHttpConnectionMgr::BuildPipeline(nsConnectionEntry *ent,
                                   nsAHttpTransaction *firstTrans,
                                   nsHttpPipeline **result)
{
    if (mMaxPipelinedRequests < 2)
        return false;

    nsHttpPipeline *pipeline = nsnull;
    nsHttpTransaction *trans;

    PRUint32 i = 0, numAdded = 0;
    while (i < ent->mPendingQ.Length()) {
        trans = ent->mPendingQ[i];
        if (trans->Caps() & NS_HTTP_ALLOW_PIPELINING) {
            if (numAdded == 0) {
                pipeline = new nsHttpPipeline;
                if (!pipeline)
                    return false;
                pipeline->AddTransaction(firstTrans);
                numAdded = 1;
            }
            pipeline->AddTransaction(trans);

            
            ent->mPendingQ.RemoveElementAt(i);
            NS_RELEASE(trans);

            if (++numAdded == mMaxPipelinedRequests)
                break;
        }
        else
            ++i; 
    }

    if (numAdded == 0)
        return false;

    LOG(("  pipelined %u transactions\n", numAdded));
    NS_ADDREF(*result = pipeline);
    return true;
}

nsresult
nsHttpConnectionMgr::ProcessNewTransaction(nsHttpTransaction *trans)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    
    
    
    
    if (NS_FAILED(trans->Status())) {
        LOG(("  transaction was canceled... dropping event!\n"));
        return NS_OK;
    }

    PRUint8 caps = trans->Caps();
    nsHttpConnectionInfo *ci = trans->ConnectionInfo();
    NS_ASSERTION(ci, "no connection info");

    nsConnectionEntry *ent = mCT.Get(ci->HashKey());
    if (!ent) {
        nsHttpConnectionInfo *clone = ci->Clone();
        if (!clone)
            return NS_ERROR_OUT_OF_MEMORY;
        ent = new nsConnectionEntry(clone);
        if (!ent)
            return NS_ERROR_OUT_OF_MEMORY;
        mCT.Put(ci->HashKey(), ent);
    }

    
    
    nsConnectionEntry *preferredEntry =
        GetSpdyPreferred(ent->mDottedDecimalAddress);
    if (preferredEntry) {
        LOG(("nsHttpConnectionMgr::ProcessNewTransaction trans=%p "
             "redirected via coalescing from %s to %s\n", trans,
             ent->mConnInfo->Host(), preferredEntry->mConnInfo->Host()));
        ent = preferredEntry;
    }

    
    
    if (caps & NS_HTTP_CLEAR_KEEPALIVES)
        ClosePersistentConnections(ent);

    
    
    
    

    nsAHttpConnection *wrappedConnection = trans->Connection();
    nsHttpConnection  *conn;
    conn = wrappedConnection ? wrappedConnection->TakeHttpConnection() : nsnull;

    if (conn) {
        NS_ASSERTION(caps & NS_HTTP_STICKY_CONNECTION, "unexpected caps");

        trans->SetConnection(nsnull);
    }
    else
        GetConnection(ent, trans, false, &conn);

    nsresult rv;
    if (!conn) {
        LOG(("  adding transaction to pending queue [trans=%x pending-count=%u]\n",
            trans, ent->mPendingQ.Length()+1));
        
        InsertTransactionSorted(ent->mPendingQ, trans);
        NS_ADDREF(trans);
        rv = NS_OK;
    }
    else {
        rv = DispatchTransaction(ent, trans, caps, conn);
        NS_RELEASE(conn);
    }

    return rv;
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

        nsresult rv2 = DispatchTransaction(ent, trans, trans->Caps(), conn);
        NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv2), "Dispatch SPDY Transaction");
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
nsHttpConnectionMgr::ProcessSpdyPendingQ()
{
    mCT.Enumerate(ProcessSpdyPendingQCB, this);
}

nsHttpConnection *
nsHttpConnectionMgr::GetSpdyPreferredConn(nsConnectionEntry *ent)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ABORT_IF_FALSE(ent, "no connection entry");

    nsConnectionEntry *preferred = GetSpdyPreferred(ent->mDottedDecimalAddress);

    
    if (preferred && preferred != ent) {
        ent->mUsingSpdy = true;
        ent->mSpdyRedir = true;
    }
    else {
        ent->mSpdyRedir = false;
        
        
    }

    if (!preferred)
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
    LOG(("nsHttpConnectionMgr::OnMsgShutdown\n"));

    mCT.Enumerate(ShutdownPassCB, this);

    
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
    LOG(("nsHttpConnectionMgr::OnMsgNewTransaction [trans=%p]\n", param));

    nsHttpTransaction *trans = (nsHttpTransaction *) param;
    trans->SetPriority(priority);

    nsHttpConnectionInfo *ci = trans->ConnectionInfo();
    nsConnectionEntry *ent = mCT.Get(ci->HashKey());
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
    LOG(("nsHttpConnectionMgr::OnMsgCancelTransaction [trans=%p]\n", param));

    nsHttpTransaction *trans = (nsHttpTransaction *) param;
    
    
    
    
    
    nsAHttpConnection *conn = trans->Connection();
    if (conn && !trans->IsDone())
        conn->CloseTransaction(trans, reason);
    else {
        nsHttpConnectionInfo *ci = trans->ConnectionInfo();
        nsConnectionEntry *ent = mCT.Get(ci->HashKey());
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
    LOG(("nsHttpConnectionMgr::OnMsgReclaimConnection [conn=%p]\n", param));

    nsHttpConnection *conn = (nsHttpConnection *) param;

    
    
    
    
    

    nsHttpConnectionInfo *ci = conn->ConnectionInfo();
    NS_ADDREF(ci);

    nsConnectionEntry *ent = mCT.Get(ci->HashKey());

    NS_ASSERTION(ent, "no connection entry");
    if (ent) {
        
        
        
        

        if (ent->mUsingSpdy)
            conn->DontReuse();

        if (ent->mActiveConns.RemoveElement(conn)) {
            nsHttpConnection *temp = conn;
            NS_RELEASE(temp);
            mNumActiveConns--;
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
    default:
        NS_NOTREACHED("unexpected parameter name");
    }
}


nsHttpConnectionMgr::nsConnectionEntry::~nsConnectionEntry()
{
    if (mSpdyPreferred)
        gHttpHandler->ConnMgr()->RemoveSpdyPreferred(mDottedDecimalAddress);

    NS_RELEASE(mConnInfo);
}




nsHttpConnectionMgr::nsConnectionHandle::~nsConnectionHandle()
{
    if (mConn) {
        gHttpHandler->ReclaimConnection(mConn);
        NS_RELEASE(mConn);
    }
}

NS_IMPL_THREADSAFE_ISUPPORTS0(nsHttpConnectionMgr::nsConnectionHandle)

nsresult
nsHttpConnectionMgr::nsConnectionHandle::OnHeadersAvailable(nsAHttpTransaction *trans,
                                                            nsHttpRequestHead *req,
                                                            nsHttpResponseHead *resp,
                                                            bool *reset)
{
    return mConn->OnHeadersAvailable(trans, req, resp, reset);
}

nsresult
nsHttpConnectionMgr::nsConnectionHandle::ResumeSend(nsAHttpTransaction *caller)
{
    return mConn->ResumeSend(caller);
}

nsresult
nsHttpConnectionMgr::nsConnectionHandle::ResumeRecv(nsAHttpTransaction *caller)
{
    return mConn->ResumeRecv(caller);
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
                                   nsHttpTransaction *trans)
    : mEnt(ent),
      mTransaction(trans)
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
        
        
        
        mEnt->mHalfOpens.RemoveElement(this);
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
    if (mTransaction->Caps() & NS_HTTP_REFRESH_DNS)
        tmpFlags = nsISocketTransport::BYPASS_CACHE;

    if (mTransaction->Caps() & NS_HTTP_LOAD_ANONYMOUS)
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
    if (gHttpHandler->IsSpdyEnabled() &&
        gHttpHandler->CoalesceSpdy() &&
        mEnt->mConnInfo->UsingSSL() &&
        !mEnt->mConnInfo->UsingHttpProxy() &&
        !mEnt->mDidDNS) {

        
        
        
        
        nsCOMPtr<nsIDNSService> dns = do_GetService(NS_DNSSERVICE_CONTRACTID,
                                                    &rv);
        mEnt->mDidDNS = true;

        if (NS_SUCCEEDED(rv)) {
            nsCOMPtr<nsICancelable> cancelable;
            dns->AsyncResolve(mEnt->mConnInfo->GetHost(), 0, this,
                              NS_GetCurrentThread(),
                              getter_AddRefs(cancelable));
        }
    }

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


NS_IMETHODIMP 
nsHttpConnectionMgr::
nsHalfOpenSocket::OnLookupComplete(nsICancelable *aRequest,
                                   nsIDNSRecord *aRecord,
                                   nsresult aStatus)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (NS_SUCCEEDED(aStatus) && mEnt) {
        aRecord->GetNextAddrAsString(mEnt->mDottedDecimalAddress);
        gHttpHandler->ConnMgr()->ProcessSpdyPendingQ(mEnt);
    }
    return NS_OK;
}

void
nsHttpConnectionMgr::nsHalfOpenSocket::SetupBackupTimer()
{
    PRUint16 timeout = gHttpHandler->GetIdleSynTimeout();
    NS_ABORT_IF_FALSE(!mSynTimer, "timer already initd");
    
    if (timeout) {
        
        
        
        
        
        
        
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
        AtActiveConnectionLimit(mEnt, mTransaction->Caps())) {
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
        rv = conn->Init(mEnt->mConnInfo,
                        gHttpHandler->ConnMgr()->mMaxRequestDelay,
                        mSocketTransport, mStreamIn, mStreamOut,
                        callbacks, callbackTarget);

        
        mStreamOut = nsnull;
        mStreamIn = nsnull;
        mSocketTransport = nsnull;
    }
    else {
        rv = conn->Init(mEnt->mConnInfo,
                        gHttpHandler->ConnMgr()->mMaxRequestDelay,
                        mBackupTransport, mBackupStreamIn, mBackupStreamOut,
                        callbacks, callbackTarget);

        
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
        mEnt->mPendingQ.RemoveElementAt(index);
        nsHttpTransaction *temp = mTransaction;
        NS_RELEASE(temp);
        gHttpHandler->ConnMgr()->AddActiveConn(conn, mEnt);
        rv = gHttpHandler->ConnMgr()->DispatchTransaction(mEnt, mTransaction,
                                                          mTransaction->Caps(),
                                                          conn);
    }
    else {
        
        

        
        
        conn->SetIdleTimeout(NS_MIN((PRUint16) 5, gHttpHandler->IdleTimeout()));

        
        
        
        conn->SetIsReusedAfter(950);

        nsRefPtr<nsHttpConnection> copy(conn);  
        gHttpHandler->ConnMgr()->OnMsgReclaimConnection(NS_OK, conn.forget().get());
    }

    return rv;
}


NS_IMETHODIMP
nsHttpConnectionMgr::nsHalfOpenSocket::OnTransportStatus(nsITransport *trans,
                                                         nsresult status,
                                                         PRUint64 progress,
                                                         PRUint64 progressMax)
{
    if (mTransaction)
        mTransaction->OnTransportStatus(trans, status, progress);

    if (trans != mSocketTransport)
        return NS_OK;

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
