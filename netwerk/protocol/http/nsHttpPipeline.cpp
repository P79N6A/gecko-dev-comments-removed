





































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
    , mRequestIsPartial(PR_FALSE)
    , mResponseIsPartial(PR_FALSE)
    , mClosed(PR_FALSE)
    , mPushBackBuf(nsnull)
    , mPushBackLen(0)
    , mPushBackMax(0)
{
}

nsHttpPipeline::~nsHttpPipeline()
{
    
    Close(NS_ERROR_ABORT);

    if (mPushBackBuf)
        free(mPushBackBuf);
}

nsresult
nsHttpPipeline::AddTransaction(nsAHttpTransaction *trans)
{
    LOG(("nsHttpPipeline::AddTransaction [this=%x trans=%x]\n", this, trans));

    NS_ADDREF(trans);
    mRequestQ.AppendElement(trans);

    if (mConnection) {
        trans->SetConnection(this);

        if (mRequestQ.Length() == 1)
            mConnection->ResumeSend();
    }

    return NS_OK;
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
                                   PRBool *reset)
{
    LOG(("nsHttpPipeline::OnHeadersAvailable [this=%x]\n", this));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ASSERTION(mConnection, "no connection");

    
    return mConnection->OnHeadersAvailable(trans, requestHead, responseHead, reset);
}

nsresult
nsHttpPipeline::ResumeSend()
{
    NS_NOTREACHED("nsHttpPipeline::ResumeSend");
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

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ASSERTION(NS_FAILED(reason), "expecting failure code");

    
    
    PRInt32 index;
    PRBool killPipeline = PR_FALSE;

    index = mRequestQ.IndexOf(trans);
    if (index >= 0) {
        if (index == 0 && mRequestIsPartial) {
            
            
            killPipeline = PR_TRUE;
        }
        mRequestQ.RemoveElementAt(index);
    }
    else {
        index = mResponseQ.IndexOf(trans);
        if (index >= 0)
            mResponseQ.RemoveElementAt(index);
        
        
        
        
        killPipeline = PR_TRUE;
    }

    trans->Close(reason);
    NS_RELEASE(trans);

    if (killPipeline) {
        if (mConnection)
            mConnection->CloseTransaction(this, reason);
        else
            Close(reason);
    }
}

void
nsHttpPipeline::GetConnectionInfo(nsHttpConnectionInfo **result)
{
    NS_ASSERTION(mConnection, "no connection");
    mConnection->GetConnectionInfo(result);
}

void
nsHttpPipeline::GetSecurityInfo(nsISupports **result)
{
    NS_ASSERTION(mConnection, "no connection");
    mConnection->GetSecurityInfo(result);
}

PRBool
nsHttpPipeline::IsPersistent()
{
    return PR_TRUE; 
}

PRBool
nsHttpPipeline::IsReused()
{
    return PR_TRUE; 
}

nsresult
nsHttpPipeline::PushBack(const char *data, PRUint32 length)
{
    LOG(("nsHttpPipeline::PushBack [this=%x len=%u]\n", this, length));
    
    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ASSERTION(mPushBackLen == 0, "push back buffer already has data!");

    

    
    
    
    
    
    
    
    

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

PRBool
nsHttpPipeline::LastTransactionExpectedNoContent()
{
    NS_ABORT_IF_FALSE(mConnection, "no connection");
    return mConnection->LastTransactionExpectedNoContent();
}

void
nsHttpPipeline::SetLastTransactionExpectedNoContent(PRBool val)
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





void
nsHttpPipeline::SetConnection(nsAHttpConnection *conn)
{
    LOG(("nsHttpPipeline::SetConnection [this=%x conn=%x]\n", this, conn));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ASSERTION(!mConnection, "already have a connection");

    NS_IF_ADDREF(mConnection = conn);

    PRInt32 i, count = mRequestQ.Length();
    for (i=0; i<count; ++i)
        Request(i)->SetConnection(this);
}

void
nsHttpPipeline::GetSecurityCallbacks(nsIInterfaceRequestor **result,
                                     nsIEventTarget        **target)
{
    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    
    nsAHttpTransaction *trans = Request(0);
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
    switch (status) {
    case NS_NET_STATUS_RECEIVING_FROM:
        
        trans = Response(0);
        if (trans)
            trans->OnTransportStatus(transport, status, progress);
        break;
    default:
        
        PRInt32 i, count = mRequestQ.Length();
        for (i=0; i<count; ++i) {
            trans = Request(i);
            if (trans)
                trans->OnTransportStatus(transport, status, progress);
        }
        break;
    }
}

PRBool
nsHttpPipeline::IsDone()
{
    return (mRequestQ.Length() == 0) && (mResponseQ.Length() == 0);
}

nsresult
nsHttpPipeline::Status()
{
    return mStatus;
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
            NS_RELEASE(trans);
            mResponseQ.RemoveElementAt(0);
            mResponseIsPartial = PR_FALSE;

            
            
            gHttpHandler->ConnMgr()->AddTransactionToPipeline(this);
        }
        else
            mResponseIsPartial = PR_TRUE;
    }

    if (mPushBackLen) {
        nsHttpPushBackWriter writer(mPushBackBuf, mPushBackLen);
        PRUint32 len = mPushBackLen, n;
        mPushBackLen = 0;
        
        
        
        rv = WriteSegments(&writer, len, &n);
    }

    return rv;
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
    mClosed = PR_TRUE;

    PRUint32 i, count;
    nsAHttpTransaction *trans;

    
    count = mRequestQ.Length();
    for (i=0; i<count; ++i) {
        trans = Request(i);
        trans->Close(NS_ERROR_NET_RESET);
        NS_RELEASE(trans);
    }
    mRequestQ.Clear();

    trans = Response(0);
    if (trans) {
        
        
        if (mResponseIsPartial)
            trans->Close(reason);
        else
            trans->Close(NS_ERROR_NET_RESET);
        NS_RELEASE(trans);
        
        
        count = mResponseQ.Length();
        for (i=1; i<count; ++i) {
            trans = Response(i);
            trans->Close(NS_ERROR_NET_RESET);
            NS_RELEASE(trans);
        }
        mResponseQ.Clear();
    }

    
    
    
    NS_IF_RELEASE(mConnection);
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
                        PR_TRUE, PR_TRUE,
                        nsIOService::gBufferCache);
        if (NS_FAILED(rv)) return rv;
    }

    PRUint32 n, avail;
    nsAHttpTransaction *trans;
    while ((trans = Request(0)) != nsnull) {
        avail = trans->Available();
        if (avail) {
            rv = trans->ReadSegments(this, avail, &n);
            if (NS_FAILED(rv)) return rv;
            
            if (n == 0) {
                LOG(("send pipe is full"));
                break;
            }
        }
        avail = trans->Available();
        if (avail == 0) {
            
            mRequestQ.RemoveElementAt(0);
            mResponseQ.AppendElement(trans);
            mRequestIsPartial = PR_FALSE;
        }
        else
            mRequestIsPartial = PR_TRUE;
    }
    return NS_OK;
}
