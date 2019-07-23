





































#include "nsHttpConnectionMgr.h"
#include "nsHttpConnection.h"
#include "nsHttpPipeline.h"
#include "nsHttpHandler.h"
#include "nsAutoLock.h"
#include "nsNetCID.h"
#include "nsCOMPtr.h"

#include "nsIServiceManager.h"


extern PRThread *gSocketThread;

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);



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
    , mMonitor(nsAutoMonitor::NewMonitor("nsHttpConnectionMgr"))
    , mMaxConns(0)
    , mMaxConnsPerHost(0)
    , mMaxConnsPerProxy(0)
    , mMaxPersistConnsPerHost(0)
    , mMaxPersistConnsPerProxy(0)
    , mNumActiveConns(0)
    , mNumIdleConns(0)
{
    LOG(("Creating nsHttpConnectionMgr @%x\n", this));
}

nsHttpConnectionMgr::~nsHttpConnectionMgr()
{
    LOG(("Destroying nsHttpConnectionMgr @%x\n", this));
 
    if (mMonitor)
        nsAutoMonitor::DestroyMonitor(mMonitor);
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

    nsresult rv;
    nsCOMPtr<nsIEventTarget> sts = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsAutoMonitor mon(mMonitor);

    
    if (mSocketThreadTarget)
        return NS_OK;

    
    
    mMaxConns = maxConns;
    mMaxConnsPerHost = maxConnsPerHost;
    mMaxConnsPerProxy = maxConnsPerProxy;
    mMaxPersistConnsPerHost = maxPersistConnsPerHost;
    mMaxPersistConnsPerProxy = maxPersistConnsPerProxy;
    mMaxRequestDelay = maxRequestDelay;
    mMaxPipelinedRequests = maxPipelinedRequests;

    mSocketThreadTarget = sts;
    return rv;
}

nsresult
nsHttpConnectionMgr::Shutdown()
{
    LOG(("nsHttpConnectionMgr::Shutdown\n"));

    nsAutoMonitor mon(mMonitor);

    
    if (!mSocketThreadTarget)
        return NS_OK;

    nsresult rv = PostEvent(&nsHttpConnectionMgr::OnMsgShutdown);

    
    
    
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
    nsAutoMonitor mon(mMonitor);

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
nsHttpConnectionMgr::GetSocketThreadTarget(nsIEventTarget **target)
{
    nsAutoMonitor mon(mMonitor);
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
        nsCStringKey key(ci->HashKey());
        nsConnectionEntry *ent = (nsConnectionEntry *) mCT.Get(&key);
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




PRIntn
nsHttpConnectionMgr::ProcessOneTransactionCB(nsHashKey *key, void *data, void *closure)
{
    nsHttpConnectionMgr *self = (nsHttpConnectionMgr *) closure;
    nsConnectionEntry *ent = (nsConnectionEntry *) data;

    if (self->ProcessPendingQForEntry(ent))
        return kHashEnumerateStop;

    return kHashEnumerateNext;
}

PRIntn
nsHttpConnectionMgr::PurgeOneIdleConnectionCB(nsHashKey *key, void *data, void *closure)
{
    nsHttpConnectionMgr *self = (nsHttpConnectionMgr *) closure;
    nsConnectionEntry *ent = (nsConnectionEntry *) data;

    if (ent->mIdleConns.Length() > 0) {
        nsHttpConnection *conn = ent->mIdleConns[0];
        ent->mIdleConns.RemoveElementAt(0);
        conn->Close(NS_ERROR_ABORT);
        NS_RELEASE(conn);
        self->mNumIdleConns--;
        return kHashEnumerateStop;
    }

    return kHashEnumerateNext;
}

PRIntn
nsHttpConnectionMgr::PruneDeadConnectionsCB(nsHashKey *key, void *data, void *closure)
{
    nsHttpConnectionMgr *self = (nsHttpConnectionMgr *) closure;
    nsConnectionEntry *ent = (nsConnectionEntry *) data;

    LOG(("  pruning [ci=%s]\n", ent->mConnInfo->HashKey().get()));

    PRInt32 count = ent->mIdleConns.Length();
    if (count > 0) {
        for (PRInt32 i=count-1; i>=0; --i) {
            nsHttpConnection *conn = ent->mIdleConns[i];
            if (!conn->CanReuse()) {
                ent->mIdleConns.RemoveElementAt(i);
                conn->Close(NS_ERROR_ABORT);
                NS_RELEASE(conn);
                self->mNumIdleConns--;
            }
        }
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
        ent->mPendingQ.Length()    == 0) {
        LOG(("    removing empty connection entry\n"));
        delete ent;
        return kHashEnumerateRemove;
    }

    
    ent->mIdleConns.Compact();
    ent->mActiveConns.Compact();
    ent->mPendingQ.Compact();

    return kHashEnumerateNext;
}

PRIntn
nsHttpConnectionMgr::ShutdownPassCB(nsHashKey *key, void *data, void *closure)
{
    nsHttpConnectionMgr *self = (nsHttpConnectionMgr *) closure;
    nsConnectionEntry *ent = (nsConnectionEntry *) data;

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

    
    while (ent->mPendingQ.Length()) {
        trans = ent->mPendingQ[0];

        ent->mPendingQ.RemoveElementAt(0);

        trans->Close(NS_ERROR_ABORT);
        NS_RELEASE(trans);
    }

    delete ent;
    return kHashEnumerateRemove;
}



PRBool
nsHttpConnectionMgr::ProcessPendingQForEntry(nsConnectionEntry *ent)
{
    LOG(("nsHttpConnectionMgr::ProcessPendingQForEntry [ci=%s]\n",
        ent->mConnInfo->HashKey().get()));

    PRInt32 i, count = ent->mPendingQ.Length();
    if (count > 0) {
        LOG(("  pending-count=%u\n", count));
        nsHttpTransaction *trans = nsnull;
        nsHttpConnection *conn = nsnull;
        for (i=0; i<count; ++i) {
            trans = ent->mPendingQ[i];
            GetConnection(ent, trans->Caps(), &conn);
            if (conn)
                break;
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
            return PR_TRUE;
        }
    }
    return PR_FALSE;
}





PRBool
nsHttpConnectionMgr::AtActiveConnectionLimit(nsConnectionEntry *ent, PRUint8 caps)
{
    nsHttpConnectionInfo *ci = ent->mConnInfo;

    LOG(("nsHttpConnectionMgr::AtActiveConnectionLimit [ci=%s caps=%x]\n",
        ci->HashKey().get(), caps));

    
    if (mNumActiveConns >= mMaxConns) {
        LOG(("  num active conns == max conns\n"));
        return PR_TRUE;
    }

    nsHttpConnection *conn;
    PRInt32 i, totalCount, persistCount = 0;
    
    totalCount = ent->mActiveConns.Length();

    
    for (i=0; i<totalCount; ++i) {
        conn = ent->mActiveConns[i];
        if (conn->IsKeepAlive()) 
            persistCount++;
    }

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
nsHttpConnectionMgr::GetConnection(nsConnectionEntry *ent, PRUint8 caps,
                                   nsHttpConnection **result)
{
    LOG(("nsHttpConnectionMgr::GetConnection [ci=%s caps=%x]\n",
        ent->mConnInfo->HashKey().get(), PRUint32(caps)));

    *result = nsnull;

    if (AtActiveConnectionLimit(ent, caps)) {
        LOG(("  at active connection limit!\n"));
        return;
    }

    nsHttpConnection *conn = nsnull;

    if (caps & NS_HTTP_ALLOW_KEEPALIVE) {
        
        while (!conn && (ent->mIdleConns.Length() > 0)) {
            conn = ent->mIdleConns[0];
            
            
            if (!conn->CanReuse()) {
                LOG(("   dropping stale connection: [conn=%x]\n", conn));
                conn->Close(NS_ERROR_ABORT);
                NS_RELEASE(conn);
            }
            else
                LOG(("   reusing connection [conn=%x]\n", conn));
            ent->mIdleConns.RemoveElementAt(0);
            mNumIdleConns--;
        }
    }

    if (!conn) {
        conn = new nsHttpConnection();
        if (!conn)
            return;
        NS_ADDREF(conn);

        nsresult rv = conn->Init(ent->mConnInfo, mMaxRequestDelay);
        if (NS_FAILED(rv)) {
            NS_RELEASE(conn);
            return;
        }
        
        
        
        if (mNumIdleConns + mNumActiveConns + 1 > mMaxConns)
            mCT.Enumerate(PurgeOneIdleConnectionCB, this);

        
        
    }

    *result = conn;
}

nsresult
nsHttpConnectionMgr::DispatchTransaction(nsConnectionEntry *ent,
                                         nsAHttpTransaction *trans,
                                         PRUint8 caps,
                                         nsHttpConnection *conn)
{
    LOG(("nsHttpConnectionMgr::DispatchTransaction [ci=%s trans=%x caps=%x conn=%x]\n",
        ent->mConnInfo->HashKey().get(), trans, caps, conn));

    nsConnectionHandle *handle = new nsConnectionHandle(conn);
    if (!handle)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(handle);

    nsHttpPipeline *pipeline = nsnull;
    if (conn->SupportsPipelining() && (caps & NS_HTTP_ALLOW_PIPELINING)) {
        LOG(("  looking to build pipeline...\n"));
        if (BuildPipeline(ent, trans, &pipeline))
            trans = pipeline;
    }

    
    ent->mActiveConns.AppendElement(conn);
    mNumActiveConns++;
    NS_ADDREF(conn);

    
    trans->SetConnection(handle);

    nsresult rv = conn->Activate(trans, caps);

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

PRBool
nsHttpConnectionMgr::BuildPipeline(nsConnectionEntry *ent,
                                   nsAHttpTransaction *firstTrans,
                                   nsHttpPipeline **result)
{
    if (mMaxPipelinedRequests < 2)
        return PR_FALSE;

    nsHttpPipeline *pipeline = nsnull;
    nsHttpTransaction *trans;

    PRUint32 i = 0, numAdded = 0;
    while (i < ent->mPendingQ.Length()) {
        trans = ent->mPendingQ[i];
        if (trans->Caps() & NS_HTTP_ALLOW_PIPELINING) {
            if (numAdded == 0) {
                pipeline = new nsHttpPipeline;
                if (!pipeline)
                    return PR_FALSE;
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
        return PR_FALSE;

    LOG(("  pipelined %u transactions\n", numAdded));
    NS_ADDREF(*result = pipeline);
    return PR_TRUE;
}

nsresult
nsHttpConnectionMgr::ProcessNewTransaction(nsHttpTransaction *trans)
{
    
    
    
    
    if (NS_FAILED(trans->Status())) {
        LOG(("  transaction was canceled... dropping event!\n"));
        return NS_OK;
    }

    PRUint8 caps = trans->Caps();
    nsHttpConnectionInfo *ci = trans->ConnectionInfo();
    NS_ASSERTION(ci, "no connection info");

    nsCStringKey key(ci->HashKey());
    nsConnectionEntry *ent = (nsConnectionEntry *) mCT.Get(&key);
    if (!ent) {
        ent = new nsConnectionEntry(ci);
        if (!ent)
            return NS_ERROR_OUT_OF_MEMORY;
        mCT.Put(&key, ent);
    }

    nsHttpConnection *conn;

    
    
    
    nsConnectionHandle *handle = (nsConnectionHandle *) trans->Connection();
    if (handle) {
        NS_ASSERTION(caps & NS_HTTP_STICKY_CONNECTION, "unexpected caps");
        NS_ASSERTION(handle->mConn, "no connection");

        
        
        conn = handle->mConn;
        handle->mConn = nsnull;

        
        trans->SetConnection(nsnull);

        
        
        if (ent->mActiveConns.RemoveElement(conn))
            mNumActiveConns--;
        else {
            NS_ERROR("sticky connection not found in active list");
            return NS_ERROR_UNEXPECTED;
        }
    }
    else
        GetConnection(ent, caps, &conn);

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
nsHttpConnectionMgr::OnMsgShutdown(PRInt32, void *)
{
    LOG(("nsHttpConnectionMgr::OnMsgShutdown\n"));

    mCT.Reset(ShutdownPassCB, this);

    
    nsAutoMonitor mon(mMonitor);
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
    nsCStringKey key(ci->HashKey());
    nsConnectionEntry *ent = (nsConnectionEntry *) mCT.Get(&key);
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
        nsCStringKey key(ci->HashKey());
        nsConnectionEntry *ent = (nsConnectionEntry *) mCT.Get(&key);
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

    
    nsCStringKey key(ci->HashKey());
    nsConnectionEntry *ent = (nsConnectionEntry *) mCT.Get(&key);
    if (!(ent && ProcessPendingQForEntry(ent))) {
        
        
        mCT.Enumerate(ProcessOneTransactionCB, this);
    }

    NS_RELEASE(ci);
}

void
nsHttpConnectionMgr::OnMsgPruneDeadConnections(PRInt32, void *)
{
    LOG(("nsHttpConnectionMgr::OnMsgPruneDeadConnections\n"));

    if (mNumIdleConns > 0) 
        mCT.Enumerate(PruneDeadConnectionsCB, this);
}

void
nsHttpConnectionMgr::OnMsgReclaimConnection(PRInt32, void *param)
{
    LOG(("nsHttpConnectionMgr::OnMsgReclaimConnection [conn=%p]\n", param));

    nsHttpConnection *conn = (nsHttpConnection *) param;

    
    
    
    
    

    nsHttpConnectionInfo *ci = conn->ConnectionInfo();
    NS_ADDREF(ci);

    nsCStringKey key(ci->HashKey());
    nsConnectionEntry *ent = (nsConnectionEntry *) mCT.Get(&key);

    NS_ASSERTION(ent, "no connection entry");
    if (ent) {
        ent->mActiveConns.RemoveElement(conn);
        mNumActiveConns--;
        if (conn->CanReuse()) {
            LOG(("  adding connection to idle list\n"));
            
            
            
            ent->mIdleConns.AppendElement(conn);
            mNumIdleConns++;
        }
        else {
            LOG(("  connection cannot be reused; closing connection\n"));
            
            conn->Close(NS_ERROR_ABORT);
            nsHttpConnection *temp = conn;
            NS_RELEASE(temp);
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
                                                            PRBool *reset)
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

void
nsHttpConnectionMgr::nsConnectionHandle::GetSecurityInfo(nsISupports **result)
{
    mConn->GetSecurityInfo(result);
}

PRBool
nsHttpConnectionMgr::nsConnectionHandle::IsPersistent()
{
    return mConn->IsPersistent();
}

PRBool
nsHttpConnectionMgr::nsConnectionHandle::IsReused()
{
    return mConn->IsReused();
}

nsresult
nsHttpConnectionMgr::nsConnectionHandle::PushBack(const char *buf, PRUint32 bufLen)
{
    return mConn->PushBack(buf, bufLen);
}
