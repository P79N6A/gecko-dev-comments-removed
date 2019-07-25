




#include <stdlib.h>
#include "nsHttp.h"
#include "nsHttpPipeline.h"
#include "nsHttpHandler.h"
#include "nsIOService.h"
#include "nsIRequest.h"
#include "nsISocketTransport.h"
#include "nsIStringStream.h"
#include "nsIPipe.h"
#include "nsCOMPtr.h"
#include "nsComponentManagerUtils.h"

#ifdef DEBUG
#include "prthread.h"

extern PRThread *gSocketThread;
#endif





class nsHttpPushBackWriter : public nsAHttpSegmentWriter
{
public:
    nsHttpPushBackWriter(const char *buf, PRUint32 bufLen)
        : mBuf(buf)
        , mBufLen(bufLen)
        { }
    virtual ~nsHttpPushBackWriter() {}

    nsresult OnWriteSegment(char *buf, PRUint32 count, PRUint32 *countWritten)
    {
        if (mBufLen == 0)
            return NS_BASE_STREAM_CLOSED;

        if (count > mBufLen)
            count = mBufLen;

        memcpy(buf, mBuf, count);

        mBuf += count;
        mBufLen -= count;
        *countWritten = count;
        return NS_OK;
    }

private:
    const char *mBuf;
    PRUint32    mBufLen;
};





nsHttpPipeline::nsHttpPipeline()
    : mConnection(nsnull)
    , mStatus(NS_OK)
    , mRequestIsPartial(false)
    , mResponseIsPartial(false)
    , mClosed(false)
    , mUtilizedPipeline(false)
    , mPushBackBuf(nsnull)
    , mPushBackLen(0)
    , mPushBackMax(0)
    , mHttp1xTransactionCount(0)
    , mReceivingFromProgress(0)
    , mSendingToProgress(0)
    , mSuppressSendEvents(true)
{
}

nsHttpPipeline::~nsHttpPipeline()
{
    
    Close(NS_ERROR_ABORT);

    NS_IF_RELEASE(mConnection);

    if (mPushBackBuf)
        free(mPushBackBuf);
}

nsresult
nsHttpPipeline::AddTransaction(nsAHttpTransaction *trans)
{
    LOG(("nsHttpPipeline::AddTransaction [this=%x trans=%x]\n", this, trans));

    if (mRequestQ.Length() || mResponseQ.Length())
        mUtilizedPipeline = true;

    NS_ADDREF(trans);
    mRequestQ.AppendElement(trans);
    PRUint32 qlen = PipelineDepth();
    
    if (qlen != 1) {
        trans->SetPipelinePosition(qlen);
    }
    else {
        
        
        trans->SetPipelinePosition(0);
    }

    
    
    trans->SetConnection(this);

    if (mConnection && !mClosed && mRequestQ.Length() == 1)
        mConnection->ResumeSend();

    return NS_OK;
}

PRUint32
nsHttpPipeline::PipelineDepth()
{
    return mRequestQ.Length() + mResponseQ.Length();
}

nsresult
nsHttpPipeline::SetPipelinePosition(PRInt32 position)
{
    nsAHttpTransaction *trans = Response(0);
    if (trans)
        return trans->SetPipelinePosition(position);
    return NS_OK;
}

PRInt32
nsHttpPipeline::PipelinePosition()
{
    nsAHttpTransaction *trans = Response(0);
    if (trans)
        return trans->PipelinePosition();

    
    if (mRequestQ.Length())
        return Request(mRequestQ.Length() - 1)->PipelinePosition();
    
    
    return 0;
}

nsHttpPipeline *
nsHttpPipeline::QueryPipeline()
{
    return this;
}





NS_IMPL_THREADSAFE_ADDREF(nsHttpPipeline)
NS_IMPL_THREADSAFE_RELEASE(nsHttpPipeline)


NS_INTERFACE_MAP_BEGIN(nsHttpPipeline)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsAHttpConnection)
NS_INTERFACE_MAP_END






nsresult
nsHttpPipeline::OnHeadersAvailable(nsAHttpTransaction *trans,
                                   nsHttpRequestHead *requestHead,
                                   nsHttpResponseHead *responseHead,
                                   bool *reset)
{
    LOG(("nsHttpPipeline::OnHeadersAvailable [this=%x]\n", this));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ASSERTION(mConnection, "no connection");
    
    nsRefPtr<nsHttpConnectionInfo> ci;
    GetConnectionInfo(getter_AddRefs(ci));

    NS_ABORT_IF_FALSE(ci, "no connection info");
    
    bool pipeliningBefore = gHttpHandler->ConnMgr()->SupportsPipelining(ci);
    
    
    nsresult rv = mConnection->OnHeadersAvailable(trans,
                                                  requestHead,
                                                  responseHead,
                                                  reset);
    
    if (!pipeliningBefore && gHttpHandler->ConnMgr()->SupportsPipelining(ci))
        
        
        gHttpHandler->ConnMgr()->ProcessPendingQForEntry(ci);

    return rv;
}

nsresult
nsHttpPipeline::ResumeSend()
{
    if (mConnection)
        return mConnection->ResumeSend();
    return NS_ERROR_UNEXPECTED;
}

nsresult
nsHttpPipeline::ResumeRecv()
{
    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ASSERTION(mConnection, "no connection");
    return mConnection->ResumeRecv();
}

void
nsHttpPipeline::CloseTransaction(nsAHttpTransaction *trans, nsresult reason)
{
    LOG(("nsHttpPipeline::CloseTransaction [this=%x trans=%x reason=%x]\n",
        this, trans, reason));

    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ASSERTION(NS_FAILED(reason), "expecting failure code");

    
    
    PRInt32 index;
    bool killPipeline = false;

    index = mRequestQ.IndexOf(trans);
    if (index >= 0) {
        if (index == 0 && mRequestIsPartial) {
            
            
            killPipeline = true;
        }
        mRequestQ.RemoveElementAt(index);
    }
    else {
        index = mResponseQ.IndexOf(trans);
        if (index >= 0)
            mResponseQ.RemoveElementAt(index);
        
        
        
        
        killPipeline = true;
    }

    
    
    
    DontReuse();

    trans->Close(reason);
    NS_RELEASE(trans);

    if (killPipeline) {
        
        CancelPipeline(reason);
    }
}

void
nsHttpPipeline::GetConnectionInfo(nsHttpConnectionInfo **result)
{
    if (!mConnection) {
        *result = nsnull;
        return;
    }

    mConnection->GetConnectionInfo(result);
}

nsresult
nsHttpPipeline::TakeTransport(nsISocketTransport  **aTransport,
                              nsIAsyncInputStream **aInputStream,
                              nsIAsyncOutputStream **aOutputStream)
{
    return mConnection->TakeTransport(aTransport, aInputStream, aOutputStream);
}

void
nsHttpPipeline::GetSecurityInfo(nsISupports **result)
{
    NS_ASSERTION(mConnection, "no connection");
    mConnection->GetSecurityInfo(result);
}

bool
nsHttpPipeline::IsPersistent()
{
    return true; 
}

bool
nsHttpPipeline::IsReused()
{
    if (!mUtilizedPipeline && mConnection)
        return mConnection->IsReused();
    return true;
}

void
nsHttpPipeline::DontReuse()
{
    if (mConnection)
        mConnection->DontReuse();
}

nsresult
nsHttpPipeline::PushBack(const char *data, PRUint32 length)
{
    LOG(("nsHttpPipeline::PushBack [this=%x len=%u]\n", this, length));
    
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ASSERTION(mPushBackLen == 0, "push back buffer already has data!");

    
    
    if (!mConnection->IsPersistent())
        return mConnection->PushBack(data, length);

    

    
    
    
    
    
    
    
    

    if (!mPushBackBuf) {
        mPushBackMax = length;
        mPushBackBuf = (char *) malloc(mPushBackMax);
        if (!mPushBackBuf)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    else if (length > mPushBackMax) {
        
        NS_ASSERTION(length <= nsIOService::gDefaultSegmentSize, "too big");
        mPushBackMax = length;
        mPushBackBuf = (char *) realloc(mPushBackBuf, mPushBackMax);
        if (!mPushBackBuf)
            return NS_ERROR_OUT_OF_MEMORY;
    }
 
    memcpy(mPushBackBuf, data, length);
    mPushBackLen = length;

    return NS_OK;
}

bool
nsHttpPipeline::IsProxyConnectInProgress()
{
    NS_ABORT_IF_FALSE(mConnection, "no connection");
    return mConnection->IsProxyConnectInProgress();
}

bool
nsHttpPipeline::LastTransactionExpectedNoContent()
{
    NS_ABORT_IF_FALSE(mConnection, "no connection");
    return mConnection->LastTransactionExpectedNoContent();
}

void
nsHttpPipeline::SetLastTransactionExpectedNoContent(bool val)
{
    NS_ABORT_IF_FALSE(mConnection, "no connection");
     mConnection->SetLastTransactionExpectedNoContent(val);
}

nsHttpConnection *
nsHttpPipeline::TakeHttpConnection()
{
    if (mConnection)
        return mConnection->TakeHttpConnection();
    return nsnull;
}

nsISocketTransport *
nsHttpPipeline::Transport()
{
    if (!mConnection)
        return nsnull;
    return mConnection->Transport();
}

nsAHttpTransaction::Classifier
nsHttpPipeline::Classification()
{
    if (mConnection)
        return mConnection->Classification();

    LOG(("nsHttpPipeline::Classification this=%p "
         "has null mConnection using CLASS_SOLO default", this));
    return nsAHttpTransaction::CLASS_SOLO;
}

void
nsHttpPipeline::Classify(nsAHttpTransaction::Classifier newclass)
{
    if (mConnection)
        mConnection->Classify(newclass);
}

void
nsHttpPipeline::SetSSLConnectFailed()
{
    nsAHttpTransaction *trans = Request(0);

    if (trans)
        trans->SetSSLConnectFailed();
}

nsHttpRequestHead *
nsHttpPipeline::RequestHead()
{
    nsAHttpTransaction *trans = Request(0);

    if (trans)
        return trans->RequestHead();
    return nsnull;
}

PRUint32
nsHttpPipeline::Http1xTransactionCount()
{
  return mHttp1xTransactionCount;
}

nsresult
nsHttpPipeline::TakeSubTransactions(
    nsTArray<nsRefPtr<nsAHttpTransaction> > &outTransactions)
{
    LOG(("nsHttpPipeline::TakeSubTransactions [this=%p]\n", this));

    if (mResponseQ.Length() || mRequestIsPartial)
        return NS_ERROR_ALREADY_OPENED;

    PRInt32 i, count = mRequestQ.Length();
    for (i = 0; i < count; ++i) {
        nsAHttpTransaction *trans = Request(i);
        
        
        trans->SetConnection(mConnection);
        outTransactions.AppendElement(trans);
        NS_RELEASE(trans);
    }
    mRequestQ.Clear();

    LOG(("   took %d\n", count));
    return NS_OK;
}





void
nsHttpPipeline::SetConnection(nsAHttpConnection *conn)
{
    LOG(("nsHttpPipeline::SetConnection [this=%x conn=%x]\n", this, conn));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ASSERTION(!mConnection, "already have a connection");

    NS_IF_ADDREF(mConnection = conn);
}

nsAHttpConnection *
nsHttpPipeline::Connection()
{
    LOG(("nsHttpPipeline::Connection [this=%x conn=%x]\n", this, mConnection));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    return mConnection;
}

void
nsHttpPipeline::GetSecurityCallbacks(nsIInterfaceRequestor **result,
                                     nsIEventTarget        **target)
{
    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    
    
    
    
    nsAHttpTransaction *trans = Request(0);
    if (!trans)
        trans = Response(0);
    if (trans)
        trans->GetSecurityCallbacks(result, target);
    else {
        *result = nsnull;
        if (target)
            *target = nsnull;
    }
}

void
nsHttpPipeline::OnTransportStatus(nsITransport* transport,
                                  nsresult status, PRUint64 progress)
{
    LOG(("nsHttpPipeline::OnStatus [this=%x status=%x progress=%llu]\n",
        this, status, progress));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    nsAHttpTransaction *trans;
    PRInt32 i, count;

    switch (status) {

    case NS_NET_STATUS_RESOLVING_HOST:
    case NS_NET_STATUS_RESOLVED_HOST:
    case NS_NET_STATUS_CONNECTING_TO:
    case NS_NET_STATUS_CONNECTED_TO:
        
        

        trans = Request(0);
        if (!trans)
            trans = Response(0);
        if (trans)
            trans->OnTransportStatus(transport, status, progress);

        break;

    case NS_NET_STATUS_SENDING_TO:
        
        
        
        
        
        
        

        if (mSuppressSendEvents) {
            mSuppressSendEvents = false;
            
            
            
            
            count = mResponseQ.Length();
            for (i = 0; i < count; ++i) {
                Response(i)->OnTransportStatus(transport,
                                               NS_NET_STATUS_SENDING_TO,
                                               progress);
                Response(i)->OnTransportStatus(transport, 
                                               NS_NET_STATUS_WAITING_FOR,
                                               progress);
            }
            if (mRequestIsPartial && Request(0))
                Request(0)->OnTransportStatus(transport,
                                              NS_NET_STATUS_SENDING_TO,
                                              progress);
            mSendingToProgress = progress;
        }
        
        break;
        
    case NS_NET_STATUS_WAITING_FOR: 
        
        
        
        
        
        break;

    case NS_NET_STATUS_RECEIVING_FROM:
        
        
        
        mReceivingFromProgress = progress;
        if (Response(0))
            Response(0)->OnTransportStatus(transport, status, progress);
        break;

    default:
        
        count = mRequestQ.Length();
        for (i = 0; i < count; ++i)
            Request(i)->OnTransportStatus(transport, status, progress);
        break;
    }
}

bool
nsHttpPipeline::IsDone()
{
    bool done = true;
    
    PRUint32 i, count = mRequestQ.Length();
    for (i = 0; done && (i < count); i++)
        done = Request(i)->IsDone();

    count = mResponseQ.Length();
    for (i = 0; done && (i < count); i++)
        done = Response(i)->IsDone();
    
    return done;
}

nsresult
nsHttpPipeline::Status()
{
    return mStatus;
}

PRUint8
nsHttpPipeline::Caps()
{
    nsAHttpTransaction *trans = Request(0);
    if (!trans)
        trans = Response(0);

    return trans ? trans->Caps() : 0;
}

PRUint32
nsHttpPipeline::Available()
{
    PRUint32 result = 0;

    PRInt32 i, count = mRequestQ.Length();
    for (i=0; i<count; ++i)
        result += Request(i)->Available();
    return result;
}

NS_METHOD
nsHttpPipeline::ReadFromPipe(nsIInputStream *stream,
                             void *closure,
                             const char *buf,
                             PRUint32 offset,
                             PRUint32 count,
                             PRUint32 *countRead)
{
    nsHttpPipeline *self = (nsHttpPipeline *) closure;
    return self->mReader->OnReadSegment(buf, count, countRead);
}

nsresult
nsHttpPipeline::ReadSegments(nsAHttpSegmentReader *reader,
                             PRUint32 count,
                             PRUint32 *countRead)
{
    LOG(("nsHttpPipeline::ReadSegments [this=%x count=%u]\n", this, count));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (mClosed) {
        *countRead = 0;
        return mStatus;
    }

    nsresult rv;
    PRUint32 avail = 0;
    if (mSendBufIn) {
        rv = mSendBufIn->Available(&avail);
        if (NS_FAILED(rv)) return rv;
    }

    if (avail == 0) {
        rv = FillSendBuf();
        if (NS_FAILED(rv)) return rv;

        rv = mSendBufIn->Available(&avail);
        if (NS_FAILED(rv)) return rv;

        
        if (avail == 0) {
            *countRead = 0;
            return NS_OK;
        }
    }

    
    if (avail > count)
        avail = count;

    mReader = reader;

    rv = mSendBufIn->ReadSegments(ReadFromPipe, this, avail, countRead);

    mReader = nsnull;
    return rv;
}

nsresult
nsHttpPipeline::WriteSegments(nsAHttpSegmentWriter *writer,
                              PRUint32 count,
                              PRUint32 *countWritten)
{
    LOG(("nsHttpPipeline::WriteSegments [this=%x count=%u]\n", this, count));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (mClosed)
        return NS_SUCCEEDED(mStatus) ? NS_BASE_STREAM_CLOSED : mStatus;

    nsAHttpTransaction *trans; 
    nsresult rv;

    trans = Response(0);
    
    
    
    
    if (!trans && mRequestQ.Length() &&
        mConnection->IsProxyConnectInProgress()) {
        LOG(("nsHttpPipeline::WriteSegments [this=%p] Forced Delegation\n",
             this));
        trans = Request(0);
    }

    if (!trans) {
        if (mRequestQ.Length() > 0)
            rv = NS_BASE_STREAM_WOULD_BLOCK;
        else
            rv = NS_BASE_STREAM_CLOSED;
    }
    else {
        
        
        
        
        rv = trans->WriteSegments(writer, count, countWritten);

        if (rv == NS_BASE_STREAM_CLOSED || trans->IsDone()) {
            trans->Close(NS_OK);

            
            if (trans == Response(0)) {
                NS_RELEASE(trans);
                mResponseQ.RemoveElementAt(0);
                mResponseIsPartial = false;
                ++mHttp1xTransactionCount;
            }

            
            
            nsRefPtr<nsHttpConnectionInfo> ci;
            GetConnectionInfo(getter_AddRefs(ci));
            if (ci)
                gHttpHandler->ConnMgr()->ProcessPendingQForEntry(ci);
        }
        else
            mResponseIsPartial = true;
    }

    if (mPushBackLen) {
        nsHttpPushBackWriter writer(mPushBackBuf, mPushBackLen);
        PRUint32 len = mPushBackLen, n;
        mPushBackLen = 0;

        
        
        
        nsITransport *transport = Transport();
        if (transport)
            OnTransportStatus(transport,
                              nsISocketTransport::STATUS_RECEIVING_FROM,
                              mReceivingFromProgress);

        
        
        
        rv = WriteSegments(&writer, len, &n);
    }

    return rv;
}

PRUint32
nsHttpPipeline::CancelPipeline(nsresult originalReason)
{
    PRUint32 i, reqLen, respLen, total;
    nsAHttpTransaction *trans;

    reqLen = mRequestQ.Length();
    respLen = mResponseQ.Length();
    total = reqLen + respLen;

    
    if (respLen)
        total--;

    if (!total)
        return 0;

    
    
    for (i = 0; i < reqLen; ++i) {
        trans = Request(i);
        if (mConnection && mConnection->IsProxyConnectInProgress())
            trans->Close(originalReason);
        else
            trans->Close(NS_ERROR_NET_RESET);
        NS_RELEASE(trans);
    }
    mRequestQ.Clear();

    
    
    
    
    for (i = 1; i < respLen; ++i) {
        trans = Response(i);
        trans->Close(NS_ERROR_NET_RESET);
        NS_RELEASE(trans);
    }

    if (respLen > 1)
        mResponseQ.TruncateLength(1);

    DontReuse();
    Classify(nsAHttpTransaction::CLASS_SOLO);

    return total;
}

void
nsHttpPipeline::Close(nsresult reason)
{
    LOG(("nsHttpPipeline::Close [this=%x reason=%x]\n", this, reason));

    if (mClosed) {
        LOG(("  already closed\n"));
        return;
    }

    
    mStatus = reason;
    mClosed = true;

    nsRefPtr<nsHttpConnectionInfo> ci;
    GetConnectionInfo(getter_AddRefs(ci));
    PRUint32 numRescheduled = CancelPipeline(reason);

    
    
    
    
    if (ci && numRescheduled)
        gHttpHandler->ConnMgr()->PipelineFeedbackInfo(
            ci, nsHttpConnectionMgr::RedCanceledPipeline, nsnull, 0);

    nsAHttpTransaction *trans = Response(0);
    if (!trans)
        return;

    
    
    
    if (!mResponseIsPartial &&
        (reason == NS_ERROR_NET_RESET ||
         reason == NS_OK ||
         reason == NS_BASE_STREAM_CLOSED)) {
        trans->Close(NS_ERROR_NET_RESET);
    }
    else {
        trans->Close(reason);
    }

    NS_RELEASE(trans);
    mResponseQ.Clear();
}

nsresult
nsHttpPipeline::OnReadSegment(const char *segment,
                              PRUint32 count,
                              PRUint32 *countRead)
{
    return mSendBufOut->Write(segment, count, countRead);
}

nsresult
nsHttpPipeline::FillSendBuf()
{
    
    

    nsresult rv;
    
    if (!mSendBufIn) {
        
        rv = NS_NewPipe(getter_AddRefs(mSendBufIn),
                        getter_AddRefs(mSendBufOut),
                        nsIOService::gDefaultSegmentSize,  
                        nsIOService::gDefaultSegmentSize,  
                        true, true);
        if (NS_FAILED(rv)) return rv;
    }

    PRUint32 n, avail;
    nsAHttpTransaction *trans;
    nsITransport *transport = Transport();

    while ((trans = Request(0)) != nsnull) {
        avail = trans->Available();
        if (avail) {
            
            
            
            
            nsAHttpTransaction *response = Response(0);
            if (response && !response->PipelinePosition())
                response->SetPipelinePosition(1);
            rv = trans->ReadSegments(this, avail, &n);
            if (NS_FAILED(rv)) return rv;
            
            if (n == 0) {
                LOG(("send pipe is full"));
                break;
            }

            mSendingToProgress += n;
            if (!mSuppressSendEvents && transport) {
                
                trans->OnTransportStatus(transport,
                                         NS_NET_STATUS_SENDING_TO,
                                         mSendingToProgress);
            }
        }

        avail = trans->Available();
        if (avail == 0) {
            
            mRequestQ.RemoveElementAt(0);
            mResponseQ.AppendElement(trans);
            mRequestIsPartial = false;

            if (!mSuppressSendEvents && transport) {
                
                trans->OnTransportStatus(transport,
                                         NS_NET_STATUS_WAITING_FOR,
                                         mSendingToProgress);
            }

            
            
            
        }
        else
            mRequestIsPartial = true;
    }
    return NS_OK;
}
