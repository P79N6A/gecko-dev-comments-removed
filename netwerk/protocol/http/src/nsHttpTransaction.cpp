







































#include "nsHttpHandler.h"
#include "nsHttpTransaction.h"
#include "nsHttpConnection.h"
#include "nsHttpRequestHead.h"
#include "nsHttpResponseHead.h"
#include "nsHttpChunkedDecoder.h"
#include "nsNetSegmentUtils.h"
#include "nsTransportUtils.h"
#include "nsNetUtil.h"
#include "nsProxyRelease.h"
#include "nsIOService.h"
#include "nsAutoLock.h"
#include "pratom.h"

#include "nsISeekableStream.h"
#include "nsISocketTransport.h"
#include "nsMultiplexInputStream.h"
#include "nsStringStream.h"

#include "nsComponentManagerUtils.h" 
#include "nsServiceManagerUtils.h"   
#include "nsIHttpActivityObserver.h"



#ifdef DEBUG

extern PRThread *gSocketThread;
#endif



static NS_DEFINE_CID(kMultiplexInputStream, NS_MULTIPLEXINPUTSTREAM_CID);


#define MAX_LINEBUF_LENGTH (1024 * 10)





static char *
LocateHttpStart(char *buf, PRUint32 len)
{
    
    
    if (len < 4)
        return (PL_strncasecmp(buf, "HTTP", len) == 0) ? buf : 0;

    
    
    while (len >= 4) {
        if (PL_strncasecmp(buf, "HTTP", 4) == 0)
            return buf;
        buf++;
        len--;
    }
    return 0;
}

#if defined(PR_LOGGING)
static void
LogHeaders(const char *lines)
{
    nsCAutoString buf;
    char *p;
    while ((p = PL_strstr(lines, "\r\n")) != nsnull) {
        buf.Assign(lines, p - lines);
        if (PL_strcasestr(buf.get(), "authorization: ") != nsnull) {
            char *p = PL_strchr(PL_strchr(buf.get(), ' ')+1, ' ');
            while (*++p) *p = '*';
        }
        LOG3(("  %s\n", buf.get()));
        lines = p + 2;
    }
}
#endif





nsHttpTransaction::nsHttpTransaction()
    : mRequestSize(0)
    , mConnection(nsnull)
    , mConnInfo(nsnull)
    , mRequestHead(nsnull)
    , mResponseHead(nsnull)
    , mContentLength(-1)
    , mContentRead(0)
    , mChunkedDecoder(nsnull)
    , mStatus(NS_OK)
    , mPriority(0)
    , mRestartCount(0)
    , mCaps(0)
    , mClosed(PR_FALSE)
    , mConnected(PR_FALSE)
    , mHaveStatusLine(PR_FALSE)
    , mHaveAllHeaders(PR_FALSE)
    , mTransactionDone(PR_FALSE)
    , mResponseIsComplete(PR_FALSE)
    , mDidContentStart(PR_FALSE)
    , mNoContent(PR_FALSE)
    , mSentData(PR_FALSE)
    , mReceivedData(PR_FALSE)
    , mStatusEventPending(PR_FALSE)
    , mHasRequestBody(PR_FALSE)
    , mSSLConnectFailed(PR_FALSE)
{
    LOG(("Creating nsHttpTransaction @%x\n", this));
}

nsHttpTransaction::~nsHttpTransaction()
{
    LOG(("Destroying nsHttpTransaction @%x\n", this));

    NS_IF_RELEASE(mConnection);
    NS_IF_RELEASE(mConnInfo);

    delete mResponseHead;
    delete mChunkedDecoder;
}

nsresult
nsHttpTransaction::Init(PRUint8 caps,
                        nsHttpConnectionInfo *cinfo,
                        nsHttpRequestHead *requestHead,
                        nsIInputStream *requestBody,
                        PRBool requestBodyHasHeaders,
                        nsIEventTarget *target,
                        nsIInterfaceRequestor *callbacks,
                        nsITransportEventSink *eventsink,
                        nsIAsyncInputStream **responseBody)
{
    nsresult rv;

    LOG(("nsHttpTransaction::Init [this=%x caps=%x]\n", this, caps));

    NS_ASSERTION(cinfo, "ouch");
    NS_ASSERTION(requestHead, "ouch");
    NS_ASSERTION(target, "ouch");

    
    rv = net_NewTransportEventSinkProxy(getter_AddRefs(mTransportSink),
                                        eventsink, target, PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    
    mActivityDistributor = do_GetService(NS_HTTPACTIVITYDISTRIBUTOR_CONTRACTID, &rv);

    
    if (NS_SUCCEEDED(rv) && mActivityDistributor) {
        
        PRBool active;
        rv = mActivityDistributor->GetIsActive(&active);
        if (NS_SUCCEEDED(rv) && active) {
            
            
            mChannel = do_QueryInterface(eventsink);
            LOG(("nsHttpTransaction::Init() " \
                 "mActivityDistributor is active " \
                 "this=%x", this));
        } else
            
            mActivityDistributor = nsnull;
    }

    NS_ADDREF(mConnInfo = cinfo);
    mCallbacks = callbacks;
    mConsumerTarget = target;
    mCaps = caps;

    if (requestHead->Method() == nsHttp::Head)
        mNoContent = PR_TRUE;

    
    
    
    
    
    
    
    
    
    
    
    
    if ((requestHead->Method() == nsHttp::Post || requestHead->Method() == nsHttp::Put) &&
        !requestBody && !requestHead->PeekHeader(nsHttp::Transfer_Encoding)) {
        requestHead->SetHeader(nsHttp::Content_Length, NS_LITERAL_CSTRING("0"));
    }

    
    mRequestHead = requestHead;

    
    
    PRBool pruneProxyHeaders = cinfo->UsingSSL() &&
                               cinfo->UsingHttpProxy();
    mReqHeaderBuf.Truncate();
    requestHead->Flatten(mReqHeaderBuf, pruneProxyHeaders);

#if defined(PR_LOGGING)
    if (LOG3_ENABLED()) {
        LOG3(("http request [\n"));
        LogHeaders(mReqHeaderBuf.get());
        LOG3(("]\n"));
    }
#endif

    
    
    if (!requestBodyHasHeaders || !requestBody)
        mReqHeaderBuf.AppendLiteral("\r\n");

    
    if (mActivityDistributor)
        mActivityDistributor->ObserveActivity(
            mChannel,
            NS_HTTP_ACTIVITY_TYPE_HTTP_TRANSACTION,
            NS_HTTP_ACTIVITY_SUBTYPE_REQUEST_HEADER,
            LL_ZERO, LL_ZERO,
            mReqHeaderBuf);

    
    
    
    nsCOMPtr<nsIInputStream> headers;
    rv = NS_NewByteInputStream(getter_AddRefs(headers),
                               mReqHeaderBuf.get(),
                               mReqHeaderBuf.Length());
    if (NS_FAILED(rv)) return rv;

    if (requestBody) {
        mHasRequestBody = PR_TRUE;

        
        nsCOMPtr<nsIMultiplexInputStream> multi =
            do_CreateInstance(kMultiplexInputStream, &rv);
        if (NS_FAILED(rv)) return rv;

        rv = multi->AppendStream(headers);
        if (NS_FAILED(rv)) return rv;

        rv = multi->AppendStream(requestBody);
        if (NS_FAILED(rv)) return rv;

        
        
        
        rv = NS_NewBufferedInputStream(getter_AddRefs(mRequestStream), multi,
                                       NET_DEFAULT_SEGMENT_SIZE);
        if (NS_FAILED(rv)) return rv;
    }
    else
        mRequestStream = headers;

    rv = mRequestStream->Available(&mRequestSize);
    if (NS_FAILED(rv)) return rv;

    
    rv = NS_NewPipe2(getter_AddRefs(mPipeIn),
                     getter_AddRefs(mPipeOut),
                     PR_TRUE, PR_TRUE,
                     NS_HTTP_SEGMENT_SIZE,
                     NS_HTTP_SEGMENT_COUNT,
                     nsIOService::gBufferCache);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*responseBody = mPipeIn);
    return NS_OK;
}

nsHttpResponseHead *
nsHttpTransaction::TakeResponseHead()
{
    if (!mHaveAllHeaders) {
        NS_WARNING("response headers not available or incomplete");
        return nsnull;
    }

    nsHttpResponseHead *head = mResponseHead;
    mResponseHead = nsnull;
    return head;
}





void
nsHttpTransaction::SetConnection(nsAHttpConnection *conn)
{
    NS_IF_RELEASE(mConnection);
    NS_IF_ADDREF(mConnection = conn);
}

void
nsHttpTransaction::GetSecurityCallbacks(nsIInterfaceRequestor **cb)
{
    NS_IF_ADDREF(*cb = mCallbacks);
}

void
nsHttpTransaction::OnTransportStatus(nsresult status, PRUint64 progress)
{
    LOG(("nsHttpTransaction::OnSocketStatus [this=%x status=%x progress=%llu]\n",
        this, status, progress));

    if (!mTransportSink)
        return;
    
    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    
    
    if (mActivityDistributor) {
        
        if ((mHasRequestBody) &&
            (status == nsISocketTransport::STATUS_WAITING_FOR))
            mActivityDistributor->ObserveActivity(
                mChannel,
                NS_HTTP_ACTIVITY_TYPE_HTTP_TRANSACTION,
                NS_HTTP_ACTIVITY_SUBTYPE_REQUEST_BODY_SENT,
                LL_ZERO, LL_ZERO, EmptyCString());

        
        mActivityDistributor->ObserveActivity(
            mChannel,
            NS_HTTP_ACTIVITY_TYPE_SOCKET_TRANSPORT,
            static_cast<PRUint32>(status),
            LL_ZERO,
            progress,
            EmptyCString());
    }

    
    if (status == nsISocketTransport::STATUS_RECEIVING_FROM)
        return;

    PRUint64 progressMax;

    if (status == nsISocketTransport::STATUS_SENDING_TO) {
        
        if (!mHasRequestBody)
            return;

        nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(mRequestStream);
        NS_ASSERTION(seekable, "Request stream isn't seekable?!?");

        PRInt64 prog = 0;
        seekable->Tell(&prog);
        progress = prog;

        
        
        progressMax = mRequestSize; 
    }
    else {
        progress = LL_ZERO;
        progressMax = 0;
    }

    mTransportSink->OnTransportStatus(nsnull, status, progress, progressMax);
}

PRBool
nsHttpTransaction::IsDone()
{
    return mTransactionDone;
}

nsresult
nsHttpTransaction::Status()
{
    return mStatus;
}

PRUint32
nsHttpTransaction::Available()
{
    PRUint32 size;
    if (NS_FAILED(mRequestStream->Available(&size)))
        size = 0;
    return size;
}

NS_METHOD
nsHttpTransaction::ReadRequestSegment(nsIInputStream *stream,
                                      void *closure,
                                      const char *buf,
                                      PRUint32 offset,
                                      PRUint32 count,
                                      PRUint32 *countRead)
{
    nsHttpTransaction *trans = (nsHttpTransaction *) closure;
    nsresult rv = trans->mReader->OnReadSegment(buf, count, countRead);
    if (NS_FAILED(rv)) return rv;

    trans->mSentData = PR_TRUE;
    return NS_OK;
}

nsresult
nsHttpTransaction::ReadSegments(nsAHttpSegmentReader *reader,
                                PRUint32 count, PRUint32 *countRead)
{
    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (mTransactionDone) {
        *countRead = 0;
        return mStatus;
    }

    if (!mConnected) {
        mConnected = PR_TRUE;
        mConnection->GetSecurityInfo(getter_AddRefs(mSecurityInfo));
    }

    mReader = reader;

    nsresult rv = mRequestStream->ReadSegments(ReadRequestSegment, this, count, countRead);

    mReader = nsnull;

    
    
    if (rv == NS_BASE_STREAM_WOULD_BLOCK) {
        nsCOMPtr<nsIAsyncInputStream> asyncIn =
                do_QueryInterface(mRequestStream);
        if (asyncIn) {
            nsCOMPtr<nsIEventTarget> target;
            gHttpHandler->GetSocketThreadTarget(getter_AddRefs(target));
            if (target)
                asyncIn->AsyncWait(this, 0, 0, target);
            else {
                NS_ERROR("no socket thread event target");
                rv = NS_ERROR_UNEXPECTED;
            }
        }
    }

    return rv;
}

NS_METHOD
nsHttpTransaction::WritePipeSegment(nsIOutputStream *stream,
                                    void *closure,
                                    char *buf,
                                    PRUint32 offset,
                                    PRUint32 count,
                                    PRUint32 *countWritten)
{
    nsHttpTransaction *trans = (nsHttpTransaction *) closure;

    if (trans->mTransactionDone)
        return NS_BASE_STREAM_CLOSED; 

    nsresult rv;
    
    
    
    rv = trans->mWriter->OnWriteSegment(buf, count, countWritten);
    if (NS_FAILED(rv)) return rv; 

    NS_ASSERTION(*countWritten > 0, "bad writer");
    trans->mReceivedData = PR_TRUE;

    
    
    rv = trans->ProcessData(buf, *countWritten, countWritten);
    if (NS_FAILED(rv))
        trans->Close(rv);

    return rv; 
}

nsresult
nsHttpTransaction::WriteSegments(nsAHttpSegmentWriter *writer,
                                 PRUint32 count, PRUint32 *countWritten)
{
    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (mTransactionDone)
        return NS_SUCCEEDED(mStatus) ? NS_BASE_STREAM_CLOSED : mStatus;

    mWriter = writer;

    nsresult rv = mPipeOut->WriteSegments(WritePipeSegment, this, count, countWritten);

    mWriter = nsnull;

    
    
    if (rv == NS_BASE_STREAM_WOULD_BLOCK) {
        nsCOMPtr<nsIEventTarget> target;
        gHttpHandler->GetSocketThreadTarget(getter_AddRefs(target));
        if (target)
            mPipeOut->AsyncWait(this, 0, 0, target);
        else {
            NS_ERROR("no socket thread event target");
            rv = NS_ERROR_UNEXPECTED;
        }
    }

    return rv;
}

void
nsHttpTransaction::Close(nsresult reason)
{
    LOG(("nsHttpTransaction::Close [this=%x reason=%x]\n", this, reason));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (mClosed) {
        LOG(("  already closed\n"));
        return;
    }

    if (mActivityDistributor) {
        
        if (!mResponseIsComplete)
            mActivityDistributor->ObserveActivity(
                mChannel,
                NS_HTTP_ACTIVITY_TYPE_HTTP_TRANSACTION,
                NS_HTTP_ACTIVITY_SUBTYPE_RESPONSE_COMPLETE,
                LL_ZERO,
                static_cast<PRUint64>(mContentRead.mValue),
                EmptyCString());

        
        mActivityDistributor->ObserveActivity(
            mChannel,
            NS_HTTP_ACTIVITY_TYPE_HTTP_TRANSACTION,
            NS_HTTP_ACTIVITY_SUBTYPE_TRANSACTION_CLOSE,
            LL_ZERO, LL_ZERO, EmptyCString());
    }

    
    
    PRBool connReused = PR_FALSE;
    if (mConnection)
        connReused = mConnection->IsReused();
    mConnected = PR_FALSE;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (reason == NS_ERROR_NET_RESET || reason == NS_OK) {
        if (!mReceivedData && (!mSentData || connReused)) {
            
            
            if (NS_SUCCEEDED(Restart()))
                return;
        }
    }

    PRBool relConn = PR_TRUE;
    if (NS_SUCCEEDED(reason)) {
        
        
        
        
        
        
        if (!mHaveAllHeaders) {
            char data = '\n';
            PRUint32 unused;
            ParseHead(&data, 1, &unused);
        }

        
        if (mCaps & NS_HTTP_STICKY_CONNECTION)
            relConn = PR_FALSE;
    }
    if (relConn && mConnection)
        NS_RELEASE(mConnection);

    mStatus = reason;
    mTransactionDone = PR_TRUE; 
    mClosed = PR_TRUE;

    
    mRequestStream = nsnull;
    mReqHeaderBuf.Truncate();
    mLineBuf.Truncate();
    if (mChunkedDecoder) {
        delete mChunkedDecoder;
        mChunkedDecoder = nsnull;
    }

    
    mPipeOut->CloseWithStatus(reason);
}





nsresult
nsHttpTransaction::Restart()
{
    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    
    if (++mRestartCount >= gHttpHandler->MaxRequestAttempts()) {
        LOG(("reached max request attempts, failing transaction @%x\n", this));
        return NS_ERROR_NET_RESET;
    }

    LOG(("restarting transaction @%x\n", this));

    
    nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(mRequestStream);
    if (seekable)
        seekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);

    
    mSecurityInfo = 0;
    NS_IF_RELEASE(mConnection);

    
    
    
    mCaps &= ~NS_HTTP_ALLOW_PIPELINING;

    return gHttpHandler->InitiateTransaction(this, mPriority);
}

void
nsHttpTransaction::ParseLine(char *line)
{
    LOG(("nsHttpTransaction::ParseLine [%s]\n", line));

    if (!mHaveStatusLine) {
        mResponseHead->ParseStatusLine(line);
        mHaveStatusLine = PR_TRUE;
        
        if (mResponseHead->Version() == NS_HTTP_VERSION_0_9)
            mHaveAllHeaders = PR_TRUE;
    }
    else
        mResponseHead->ParseHeaderLine(line);
}

nsresult
nsHttpTransaction::ParseLineSegment(char *segment, PRUint32 len)
{
    NS_PRECONDITION(!mHaveAllHeaders, "already have all headers");

    if (!mLineBuf.IsEmpty() && mLineBuf.Last() == '\n') {
        
        
        
        
        mLineBuf.Truncate(mLineBuf.Length() - 1);
        if (!mHaveStatusLine || (*segment != ' ' && *segment != '\t')) {
            ParseLine(mLineBuf.BeginWriting());
            mLineBuf.Truncate();
        }
    }

    
    if (mLineBuf.Length() + len > MAX_LINEBUF_LENGTH) {
        LOG(("excessively long header received, canceling transaction [trans=%x]", this));
        return NS_ERROR_ABORT;
    }
    mLineBuf.Append(segment, len);
    
    
    if (mLineBuf.First() == '\n') {
        mLineBuf.Truncate();
        
        if (mResponseHead->Status() / 100 == 1) {
            LOG(("ignoring 1xx response\n"));
            mHaveStatusLine = PR_FALSE;
            mResponseHead->Reset();
            return NS_OK;
        }
        mHaveAllHeaders = PR_TRUE;
    }
    return NS_OK;
}

nsresult
nsHttpTransaction::ParseHead(char *buf,
                             PRUint32 count,
                             PRUint32 *countRead)
{
    nsresult rv;
    PRUint32 len;
    char *eol;

    LOG(("nsHttpTransaction::ParseHead [count=%u]\n", count));

    *countRead = 0;

    NS_PRECONDITION(!mHaveAllHeaders, "oops");
        
    
    if (!mResponseHead) {
        mResponseHead = new nsHttpResponseHead();
        if (!mResponseHead)
            return NS_ERROR_OUT_OF_MEMORY;

        
        if (mActivityDistributor)
            mActivityDistributor->ObserveActivity(
                mChannel,
                NS_HTTP_ACTIVITY_TYPE_HTTP_TRANSACTION,
                NS_HTTP_ACTIVITY_SUBTYPE_RESPONSE_START,
                LL_ZERO, LL_ZERO, EmptyCString());
    }

    
    
    if (!mHaveStatusLine && mLineBuf.IsEmpty()) {
        
        char *p = LocateHttpStart(buf, PR_MIN(count, 8));
        if (!p) {
            
            if (mRequestHead->Method() == nsHttp::Put)
                return NS_ERROR_ABORT;

            mResponseHead->ParseStatusLine("");
            mHaveStatusLine = PR_TRUE;
            mHaveAllHeaders = PR_TRUE;
            return NS_OK;
        }
        if (p > buf) {
            
            *countRead = p - buf;
            buf = p;
        }
    }
    

    while ((eol = static_cast<char *>(memchr(buf, '\n', count - *countRead))) != nsnull) {
        
        len = eol - buf + 1;

        *countRead += len;

        
        if ((eol > buf) && (*(eol-1) == '\r'))
            len--;

        buf[len-1] = '\n';
        rv = ParseLineSegment(buf, len);
        if (NS_FAILED(rv))
            return rv;

        if (mHaveAllHeaders)
            return NS_OK;

        
        buf = eol + 1;
    }

    
    if (!mHaveAllHeaders && (len = count - *countRead)) {
        *countRead = count;
        
        
        if ((buf[len-1] == '\r') && (--len == 0))
            return NS_OK;
        rv = ParseLineSegment(buf, len);
        if (NS_FAILED(rv))
            return rv;
    }
    return NS_OK;
}


nsresult
nsHttpTransaction::HandleContentStart()
{
    LOG(("nsHttpTransaction::HandleContentStart [this=%x]\n", this));

    if (mResponseHead) {
#if defined(PR_LOGGING)
        if (LOG3_ENABLED()) {
            LOG3(("http response [\n"));
            nsCAutoString headers;
            mResponseHead->Flatten(headers, PR_FALSE);
            LogHeaders(headers.get());
            LOG3(("]\n"));
        }
#endif
        
        PRBool reset = PR_FALSE;
        mConnection->OnHeadersAvailable(this, mRequestHead, mResponseHead, &reset);

        
        if (reset) {
            LOG(("resetting transaction's response head\n"));
            mHaveAllHeaders = PR_FALSE;
            mHaveStatusLine = PR_FALSE;
            mReceivedData = PR_FALSE;
            mSentData = PR_FALSE;
            mResponseHead->Reset();
            
            return NS_OK;
        }

        
        switch (mResponseHead->Status()) {
        case 204:
        case 205:
        case 304:
            mNoContent = PR_TRUE;
            LOG(("this response should not contain a body.\n"));
            break;
        }

        if (mNoContent)
            mContentLength = 0;
        else {
            
            mContentLength = mResponseHead->ContentLength();

            
            
            
            
            
            if (mResponseHead->Version() >= NS_HTTP_VERSION_1_1 &&
                mResponseHead->HasHeaderValue(nsHttp::Transfer_Encoding, "chunked")) {
                
                mChunkedDecoder = new nsHttpChunkedDecoder();
                if (!mChunkedDecoder)
                    return NS_ERROR_OUT_OF_MEMORY;
                LOG(("chunked decoder created\n"));
                
                mContentLength = -1;
            }
#if defined(PR_LOGGING)
            else if (mContentLength == nsInt64(-1))
                LOG(("waiting for the server to close the connection.\n"));
#endif
        }
    }

    mDidContentStart = PR_TRUE;
    return NS_OK;
}


nsresult
nsHttpTransaction::HandleContent(char *buf,
                                 PRUint32 count,
                                 PRUint32 *contentRead,
                                 PRUint32 *contentRemaining)
{
    nsresult rv;

    LOG(("nsHttpTransaction::HandleContent [this=%x count=%u]\n", this, count));

    *contentRead = 0;
    *contentRemaining = 0;

    NS_ASSERTION(mConnection, "no connection");

    if (!mDidContentStart) {
        rv = HandleContentStart();
        if (NS_FAILED(rv)) return rv;
        
        if (!mDidContentStart)
            return NS_OK;
    }

    if (mChunkedDecoder) {
        
        
        rv = mChunkedDecoder->HandleChunkedContent(buf, count, contentRead, contentRemaining);
        if (NS_FAILED(rv)) return rv;
    }
    else if (mContentLength >= nsInt64(0)) {
        
        
        
        
        if (mConnection->IsPersistent()) {
            nsInt64 remaining = mContentLength - mContentRead;
            nsInt64 count64 = count;
            *contentRead = PR_MIN(count64, remaining);
            *contentRemaining = count - *contentRead;
        }
        else {
            *contentRead = count;
            
            nsInt64 position = mContentRead + nsInt64(count);
            if (position > mContentLength) {
                mContentLength = position;
                
            }
        }
    }
    else {
        
        
        *contentRead = count;
    }

    if (*contentRead) {
        
        mContentRead += *contentRead;
        



    }

    LOG(("nsHttpTransaction::HandleContent [this=%x count=%u read=%u mContentRead=%lld mContentLength=%lld]\n",
        this, count, *contentRead, mContentRead.mValue, mContentLength.mValue));

    
    if ((mContentRead == mContentLength) ||
        (mChunkedDecoder && mChunkedDecoder->ReachedEOF())) {
        
        mTransactionDone = PR_TRUE;
        mResponseIsComplete = PR_TRUE;

        
        if (mActivityDistributor)
            mActivityDistributor->ObserveActivity(
                mChannel,
                NS_HTTP_ACTIVITY_TYPE_HTTP_TRANSACTION,
                NS_HTTP_ACTIVITY_SUBTYPE_RESPONSE_COMPLETE,
                LL_ZERO,
                static_cast<PRUint64>(mContentRead.mValue),
                EmptyCString());
    }

    return NS_OK;
}

nsresult
nsHttpTransaction::ProcessData(char *buf, PRUint32 count, PRUint32 *countRead)
{
    nsresult rv;

    LOG(("nsHttpTransaction::ProcessData [this=%x count=%u]\n", this, count));

    *countRead = 0;

    
    if (!mHaveAllHeaders) {
        PRUint32 bytesConsumed = 0;

        rv = ParseHead(buf, count, &bytesConsumed);
        if (NS_FAILED(rv)) return rv;

        count -= bytesConsumed;

        
        if (count && bytesConsumed)
            memmove(buf, buf + bytesConsumed, count);

        
        if (mActivityDistributor && mResponseHead && mHaveAllHeaders) {
            nsCAutoString completeResponseHeaders;
            mResponseHead->Flatten(completeResponseHeaders, PR_FALSE);
            completeResponseHeaders.AppendLiteral("\r\n");
            mActivityDistributor->ObserveActivity(
                mChannel,
                NS_HTTP_ACTIVITY_TYPE_HTTP_TRANSACTION,
                NS_HTTP_ACTIVITY_SUBTYPE_RESPONSE_HEADER,
                LL_ZERO, LL_ZERO,
                completeResponseHeaders);
        }
    }

    
    
    if (mHaveAllHeaders) {
        PRUint32 countRemaining = 0;
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        rv = HandleContent(buf, count, countRead, &countRemaining);
        if (NS_FAILED(rv)) return rv;
        
        
        if (mResponseIsComplete && countRemaining) {
            NS_ASSERTION(mConnection, "no connection");
            mConnection->PushBack(buf + *countRead, countRemaining);
        }
    }

    return NS_OK;
}





class nsDeleteHttpTransaction : public nsRunnable {
public:
    nsDeleteHttpTransaction(nsHttpTransaction *trans)
        : mTrans(trans)
    {}

    NS_IMETHOD Run()
    {
        delete mTrans;
        return NS_OK;
    }
private:
    nsHttpTransaction *mTrans;
};

void
nsHttpTransaction::DeleteSelfOnConsumerThread()
{
    LOG(("nsHttpTransaction::DeleteSelfOnConsumerThread [this=%x]\n", this));
    
    PRBool val;
    if (NS_SUCCEEDED(mConsumerTarget->IsOnCurrentThread(&val)) && val)
        delete this;
    else {
        LOG(("proxying delete to consumer thread...\n"));
        nsCOMPtr<nsIRunnable> event = new nsDeleteHttpTransaction(this);
        if (NS_FAILED(mConsumerTarget->Dispatch(event, NS_DISPATCH_NORMAL)))
            NS_WARNING("failed to dispatch nsHttpDeleteTransaction event");
    }
}





NS_IMPL_THREADSAFE_ADDREF(nsHttpTransaction)

NS_IMETHODIMP_(nsrefcnt)
nsHttpTransaction::Release()
{
    nsrefcnt count;
    NS_PRECONDITION(0 != mRefCnt, "dup release");
    count = PR_AtomicDecrement((PRInt32 *) &mRefCnt);
    NS_LOG_RELEASE(this, count, "nsHttpTransaction");
    if (0 == count) {
        mRefCnt = 1; 
        
        
        DeleteSelfOnConsumerThread();
        return 0;
    }
    return count;
}

NS_IMPL_THREADSAFE_QUERY_INTERFACE2(nsHttpTransaction,
                                    nsIInputStreamCallback,
                                    nsIOutputStreamCallback)






NS_IMETHODIMP
nsHttpTransaction::OnInputStreamReady(nsIAsyncInputStream *out)
{
    if (mConnection) {
        nsresult rv = mConnection->ResumeSend();
        if (NS_FAILED(rv))
            NS_ERROR("ResumeSend failed");
    }
    return NS_OK;
}






NS_IMETHODIMP
nsHttpTransaction::OnOutputStreamReady(nsIAsyncOutputStream *out)
{
    if (mConnection) {
        nsresult rv = mConnection->ResumeRecv();
        if (NS_FAILED(rv))
            NS_ERROR("ResumeRecv failed");
    }
    return NS_OK;
}
