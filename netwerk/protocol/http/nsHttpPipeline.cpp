





#include "HttpLog.h"

#include "nsHttpPipeline.h"
#include "nsHttpHandler.h"
#include "nsIOService.h"
#include "nsISocketTransport.h"
#include "nsIPipe.h"
#include "nsCOMPtr.h"
#include <algorithm>

#ifdef DEBUG
#include "prthread.h"

extern PRThread *gSocketThread;
#endif

namespace mozilla {
namespace net {





class nsHttpPushBackWriter : public nsAHttpSegmentWriter
{
public:
    nsHttpPushBackWriter(const char *buf, uint32_t bufLen)
        : mBuf(buf)
        , mBufLen(bufLen)
        { }
    virtual ~nsHttpPushBackWriter() {}

    nsresult OnWriteSegment(char *buf, uint32_t count, uint32_t *countWritten)
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
    uint32_t    mBufLen;
};





nsHttpPipeline::nsHttpPipeline()
    : mConnection(nullptr)
    , mStatus(NS_OK)
    , mRequestIsPartial(false)
    , mResponseIsPartial(false)
    , mClosed(false)
    , mUtilizedPipeline(false)
    , mPushBackBuf(nullptr)
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
    LOG(("nsHttpPipeline::AddTransaction [this=%p trans=%x]\n", this, trans));

    if (mRequestQ.Length() || mResponseQ.Length())
        mUtilizedPipeline = true;

    NS_ADDREF(trans);
    mRequestQ.AppendElement(trans);
    uint32_t qlen = PipelineDepth();

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

uint32_t
nsHttpPipeline::PipelineDepth()
{
    return mRequestQ.Length() + mResponseQ.Length();
}

nsresult
nsHttpPipeline::SetPipelinePosition(int32_t position)
{
    nsAHttpTransaction *trans = Response(0);
    if (trans)
        return trans->SetPipelinePosition(position);
    return NS_OK;
}

int32_t
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





NS_IMPL_ADDREF(nsHttpPipeline)
NS_IMPL_RELEASE(nsHttpPipeline)


NS_INTERFACE_MAP_BEGIN(nsHttpPipeline)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsAHttpConnection)
NS_INTERFACE_MAP_END






nsresult
nsHttpPipeline::OnHeadersAvailable(nsAHttpTransaction *trans,
                                   nsHttpRequestHead *requestHead,
                                   nsHttpResponseHead *responseHead,
                                   bool *reset)
{
    LOG(("nsHttpPipeline::OnHeadersAvailable [this=%p]\n", this));

    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    MOZ_ASSERT(mConnection, "no connection");

    nsRefPtr<nsHttpConnectionInfo> ci;
    GetConnectionInfo(getter_AddRefs(ci));
    MOZ_ASSERT(ci);

    bool pipeliningBefore = gHttpHandler->ConnMgr()->SupportsPipelining(ci);

    
    nsresult rv = mConnection->OnHeadersAvailable(trans,
                                                  requestHead,
                                                  responseHead,
                                                  reset);

    if (!pipeliningBefore && gHttpHandler->ConnMgr()->SupportsPipelining(ci))
        
        
        gHttpHandler->ConnMgr()->ProcessPendingQForEntry(ci);

    return rv;
}

void
nsHttpPipeline::CloseTransaction(nsAHttpTransaction *trans, nsresult reason)
{
    LOG(("nsHttpPipeline::CloseTransaction [this=%p trans=%x reason=%x]\n",
        this, trans, reason));

    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    MOZ_ASSERT(NS_FAILED(reason), "expecting failure code");

    

    int32_t index;
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

    
    
    if (!mRequestQ.Length() && !mResponseQ.Length() && mConnection)
        mConnection->CloseTransaction(this, reason);
}

nsresult
nsHttpPipeline::TakeTransport(nsISocketTransport  **aTransport,
                              nsIAsyncInputStream **aInputStream,
                              nsIAsyncOutputStream **aOutputStream)
{
    return mConnection->TakeTransport(aTransport, aInputStream, aOutputStream);
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
nsHttpPipeline::PushBack(const char *data, uint32_t length)
{
    LOG(("nsHttpPipeline::PushBack [this=%p len=%u]\n", this, length));

    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    MOZ_ASSERT(mPushBackLen == 0, "push back buffer already has data!");

    
    
    if (!mConnection->IsPersistent())
        return mConnection->PushBack(data, length);

    

    
    
    
    
    
    
    
    

    if (!mPushBackBuf) {
        mPushBackMax = length;
        mPushBackBuf = (char *) malloc(mPushBackMax);
        if (!mPushBackBuf)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    else if (length > mPushBackMax) {
        
        MOZ_ASSERT(length <= nsIOService::gDefaultSegmentSize, "too big");
        mPushBackMax = length;
        mPushBackBuf = (char *) realloc(mPushBackBuf, mPushBackMax);
        if (!mPushBackBuf)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    memcpy(mPushBackBuf, data, length);
    mPushBackLen = length;

    return NS_OK;
}

nsHttpConnection *
nsHttpPipeline::TakeHttpConnection()
{
    if (mConnection)
        return mConnection->TakeHttpConnection();
    return nullptr;
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
nsHttpPipeline::SetProxyConnectFailed()
{
    nsAHttpTransaction *trans = Request(0);

    if (trans)
        trans->SetProxyConnectFailed();
}

nsHttpRequestHead *
nsHttpPipeline::RequestHead()
{
    nsAHttpTransaction *trans = Request(0);

    if (trans)
        return trans->RequestHead();
    return nullptr;
}

uint32_t
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

    int32_t i, count = mRequestQ.Length();
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
    LOG(("nsHttpPipeline::SetConnection [this=%p conn=%x]\n", this, conn));

    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    MOZ_ASSERT(!mConnection, "already have a connection");

    NS_IF_ADDREF(mConnection = conn);
}

nsAHttpConnection *
nsHttpPipeline::Connection()
{
    LOG(("nsHttpPipeline::Connection [this=%p conn=%x]\n", this, mConnection));

    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    return mConnection;
}

void
nsHttpPipeline::GetSecurityCallbacks(nsIInterfaceRequestor **result)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    
    
    
    
    nsAHttpTransaction *trans = Request(0);
    if (!trans)
        trans = Response(0);
    if (trans)
        trans->GetSecurityCallbacks(result);
    else {
        *result = nullptr;
    }
}

void
nsHttpPipeline::OnTransportStatus(nsITransport* transport,
                                  nsresult status, uint64_t progress)
{
    LOG(("nsHttpPipeline::OnStatus [this=%p status=%x progress=%llu]\n",
        this, status, progress));

    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    nsAHttpTransaction *trans;
    int32_t i, count;

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

    uint32_t i, count = mRequestQ.Length();
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

uint32_t
nsHttpPipeline::Caps()
{
    nsAHttpTransaction *trans = Request(0);
    if (!trans)
        trans = Response(0);

    return trans ? trans->Caps() : 0;
}

void
nsHttpPipeline::SetDNSWasRefreshed()
{
    nsAHttpTransaction *trans = Request(0);
    if (!trans)
        trans = Response(0);

    if (trans)
      trans->SetDNSWasRefreshed();
}

uint64_t
nsHttpPipeline::Available()
{
    uint64_t result = 0;

    int32_t i, count = mRequestQ.Length();
    for (i=0; i<count; ++i)
        result += Request(i)->Available();
    return result;
}

NS_METHOD
nsHttpPipeline::ReadFromPipe(nsIInputStream *stream,
                             void *closure,
                             const char *buf,
                             uint32_t offset,
                             uint32_t count,
                             uint32_t *countRead)
{
    nsHttpPipeline *self = (nsHttpPipeline *) closure;
    return self->mReader->OnReadSegment(buf, count, countRead);
}

nsresult
nsHttpPipeline::ReadSegments(nsAHttpSegmentReader *reader,
                             uint32_t count,
                             uint32_t *countRead)
{
    LOG(("nsHttpPipeline::ReadSegments [this=%p count=%u]\n", this, count));

    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    if (mClosed) {
        *countRead = 0;
        return mStatus;
    }

    nsresult rv;
    uint64_t avail = 0;
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

    
    rv = mSendBufIn->ReadSegments(ReadFromPipe, this, (uint32_t)avail, countRead);

    mReader = nullptr;
    return rv;
}

nsresult
nsHttpPipeline::WriteSegments(nsAHttpSegmentWriter *writer,
                              uint32_t count,
                              uint32_t *countWritten)
{
    LOG(("nsHttpPipeline::WriteSegments [this=%p count=%u]\n", this, count));

    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

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
        uint32_t len = mPushBackLen, n;
        mPushBackLen = 0;

        
        
        
        nsITransport *transport = Transport();
        if (transport)
            OnTransportStatus(transport, NS_NET_STATUS_RECEIVING_FROM,
                              mReceivingFromProgress);

        
        
        
        rv = WriteSegments(&writer, len, &n);
    }

    return rv;
}

uint32_t
nsHttpPipeline::CancelPipeline(nsresult originalReason)
{
    uint32_t i, reqLen, respLen, total;
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
    LOG(("nsHttpPipeline::Close [this=%p reason=%x]\n", this, reason));

    if (mClosed) {
        LOG(("  already closed\n"));
        return;
    }

    
    mStatus = reason;
    mClosed = true;

    nsRefPtr<nsHttpConnectionInfo> ci;
    GetConnectionInfo(getter_AddRefs(ci));
    uint32_t numRescheduled = CancelPipeline(reason);

    
    
    
    
    if (ci && numRescheduled)
        gHttpHandler->ConnMgr()->PipelineFeedbackInfo(
            ci, nsHttpConnectionMgr::RedCanceledPipeline, nullptr, 0);

    nsAHttpTransaction *trans = Response(0);
    if (!trans)
        return;

    
    
    
    if (!mResponseIsPartial &&
        (reason == NS_ERROR_NET_RESET ||
         reason == NS_OK ||
         reason == NS_ERROR_NET_TIMEOUT ||
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
                              uint32_t count,
                              uint32_t *countRead)
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

    uint32_t n;
    uint64_t avail;
    nsAHttpTransaction *trans;
    nsITransport *transport = Transport();

    while ((trans = Request(0)) != nullptr) {
        avail = trans->Available();
        if (avail) {
            
            
            
            
            nsAHttpTransaction *response = Response(0);
            if (response && !response->PipelinePosition())
                response->SetPipelinePosition(1);
            rv = trans->ReadSegments(this, (uint32_t)std::min(avail, (uint64_t)UINT32_MAX), &n);
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

} 
} 
