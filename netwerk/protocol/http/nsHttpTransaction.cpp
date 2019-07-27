






#include "HttpLog.h"

#include "base/basictypes.h"

#include "nsHttpHandler.h"
#include "nsHttpTransaction.h"
#include "nsHttpRequestHead.h"
#include "nsHttpResponseHead.h"
#include "nsHttpChunkedDecoder.h"
#include "nsTransportUtils.h"
#include "nsNetUtil.h"
#include "nsCRT.h"

#include "nsISeekableStream.h"
#include "nsMultiplexInputStream.h"
#include "nsStringStream.h"
#include "mozilla/VisualEventTracer.h"

#include "nsComponentManagerUtils.h" 
#include "nsServiceManagerUtils.h"   
#include "nsIHttpActivityObserver.h"
#include "nsSocketTransportService2.h"
#include "nsICancelable.h"
#include "nsIEventTarget.h"
#include "nsIHttpChannelInternal.h"
#include "nsIInputStream.h"
#include "nsITransport.h"
#include "nsIOService.h"
#include <algorithm>

#ifdef MOZ_WIDGET_GONK
#include "NetStatistics.h"
#endif



#ifdef DEBUG

extern PRThread *gSocketThread;
#endif



static NS_DEFINE_CID(kMultiplexInputStream, NS_MULTIPLEXINPUTSTREAM_CID);



#define MAX_INVALID_RESPONSE_BODY_SIZE (1024 * 128)

using namespace mozilla::net;

namespace mozilla {
namespace net {





#if defined(PR_LOGGING)
static void
LogHeaders(const char *lineStart)
{
    nsAutoCString buf;
    char *endOfLine;
    while ((endOfLine = PL_strstr(lineStart, "\r\n"))) {
        buf.Assign(lineStart, endOfLine - lineStart);
        if (PL_strcasestr(buf.get(), "authorization: ") ||
            PL_strcasestr(buf.get(), "proxy-authorization: ")) {
            char *p = PL_strchr(PL_strchr(buf.get(), ' ') + 1, ' ');
            while (p && *++p)
                *p = '*';
        }
        LOG3(("  %s\n", buf.get()));
        lineStart = endOfLine + 2;
    }
}
#endif





nsHttpTransaction::nsHttpTransaction()
    : mLock("transaction lock")
    , mRequestSize(0)
    , mConnection(nullptr)
    , mRequestHead(nullptr)
    , mResponseHead(nullptr)
    , mContentLength(-1)
    , mContentRead(0)
    , mInvalidResponseBytesRead(0)
    , mPushedStream(nullptr)
    , mChunkedDecoder(nullptr)
    , mStatus(NS_OK)
    , mPriority(0)
    , mRestartCount(0)
    , mCaps(0)
    , mClassification(CLASS_GENERAL)
    , mPipelinePosition(0)
    , mCapsToClear(0)
    , mHttpVersion(NS_HTTP_VERSION_UNKNOWN)
    , mClosed(false)
    , mConnected(false)
    , mHaveStatusLine(false)
    , mHaveAllHeaders(false)
    , mTransactionDone(false)
    , mResponseIsComplete(false)
    , mDidContentStart(false)
    , mNoContent(false)
    , mSentData(false)
    , mReceivedData(false)
    , mStatusEventPending(false)
    , mHasRequestBody(false)
    , mProxyConnectFailed(false)
    , mHttpResponseMatched(false)
    , mPreserveStream(false)
    , mDispatchedAsBlocking(false)
    , mResponseTimeoutEnabled(true)
    , mForceRestart(false)
    , mReuseOnRestart(false)
    , mContentDecoding(false)
    , mContentDecodingCheck(false)
    , mReportedStart(false)
    , mReportedResponseHeader(false)
    , mForTakeResponseHead(nullptr)
    , mResponseHeadTaken(false)
    , mSubmittedRatePacing(false)
    , mPassedRatePacing(false)
    , mSynchronousRatePaceRequest(false)
    , mCountRecv(0)
    , mCountSent(0)
    , mAppId(NECKO_NO_APP_ID)
    , mIsInBrowser(false)
    , mClassOfService(0)
{
    LOG(("Creating nsHttpTransaction @%p\n", this));
    gHttpHandler->GetMaxPipelineObjectSize(&mMaxPipelineObjectSize);

#ifdef MOZ_VALGRIND
    memset(&mSelfAddr, 0, sizeof(NetAddr));
    memset(&mPeerAddr, 0, sizeof(NetAddr));
#endif
    mSelfAddr.raw.family = PR_AF_UNSPEC;
    mPeerAddr.raw.family = PR_AF_UNSPEC;
}

nsHttpTransaction::~nsHttpTransaction()
{
    LOG(("Destroying nsHttpTransaction @%p\n", this));

    if (mPushedStream) {
        mPushedStream->OnPushFailed();
        mPushedStream = nullptr;
    }

    if (mTokenBucketCancel) {
        mTokenBucketCancel->Cancel(NS_ERROR_ABORT);
        mTokenBucketCancel = nullptr;
    }

    
    mCallbacks = nullptr;
    mConnection = nullptr;

    delete mResponseHead;
    delete mForTakeResponseHead;
    delete mChunkedDecoder;
    ReleaseBlockingTransaction();
}

nsHttpTransaction::Classifier
nsHttpTransaction::Classify()
{
    if (!(mCaps & NS_HTTP_ALLOW_PIPELINING))
        return (mClassification = CLASS_SOLO);

    if (mRequestHead->PeekHeader(nsHttp::If_Modified_Since) ||
        mRequestHead->PeekHeader(nsHttp::If_None_Match))
        return (mClassification = CLASS_REVALIDATION);

    const char *accept = mRequestHead->PeekHeader(nsHttp::Accept);
    if (accept && !PL_strncmp(accept, "image/", 6))
        return (mClassification = CLASS_IMAGE);

    if (accept && !PL_strncmp(accept, "text/css", 8))
        return (mClassification = CLASS_SCRIPT);

    mClassification = CLASS_GENERAL;

    int32_t queryPos = mRequestHead->RequestURI().FindChar('?');
    if (queryPos == kNotFound) {
        if (StringEndsWith(mRequestHead->RequestURI(),
                           NS_LITERAL_CSTRING(".js")))
            mClassification = CLASS_SCRIPT;
    }
    else if (queryPos >= 3 &&
             Substring(mRequestHead->RequestURI(), queryPos - 3, 3).
             EqualsLiteral(".js")) {
        mClassification = CLASS_SCRIPT;
    }

    return mClassification;
}

nsresult
nsHttpTransaction::Init(uint32_t caps,
                        nsHttpConnectionInfo *cinfo,
                        nsHttpRequestHead *requestHead,
                        nsIInputStream *requestBody,
                        bool requestBodyHasHeaders,
                        nsIEventTarget *target,
                        nsIInterfaceRequestor *callbacks,
                        nsITransportEventSink *eventsink,
                        nsIAsyncInputStream **responseBody)
{
    MOZ_EVENT_TRACER_COMPOUND_NAME(static_cast<nsAHttpTransaction*>(this),
                                   requestHead->PeekHeader(nsHttp::Host),
                                   requestHead->RequestURI().BeginReading());

    MOZ_EVENT_TRACER_WAIT(static_cast<nsAHttpTransaction*>(this),
                          "net::http::transaction");
    nsresult rv;

    LOG(("nsHttpTransaction::Init [this=%p caps=%x]\n", this, caps));

    MOZ_ASSERT(cinfo);
    MOZ_ASSERT(requestHead);
    MOZ_ASSERT(target);

    mActivityDistributor = do_GetService(NS_HTTPACTIVITYDISTRIBUTOR_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    bool activityDistributorActive;
    rv = mActivityDistributor->GetIsActive(&activityDistributorActive);
    if (NS_SUCCEEDED(rv) && activityDistributorActive) {
        
        
        LOG(("nsHttpTransaction::Init() " \
             "mActivityDistributor is active " \
             "this=%p", this));
    } else {
        
        activityDistributorActive = false;
        mActivityDistributor = nullptr;
    }
    mChannel = do_QueryInterface(eventsink);
    nsCOMPtr<nsIChannel> channel = do_QueryInterface(eventsink);
    if (channel) {
        NS_GetAppInfo(channel, &mAppId, &mIsInBrowser);
    }

#ifdef MOZ_WIDGET_GONK
    if (mAppId != NECKO_NO_APP_ID) {
        nsCOMPtr<nsINetworkInterface> activeNetwork;
        GetActiveNetworkInterface(activeNetwork);
        mActiveNetwork =
            new nsMainThreadPtrHolder<nsINetworkInterface>(activeNetwork);
    }
#endif

    nsCOMPtr<nsIHttpChannelInternal> httpChannelInternal =
        do_QueryInterface(eventsink);
    if (httpChannelInternal) {
        rv = httpChannelInternal->GetResponseTimeoutEnabled(
            &mResponseTimeoutEnabled);
        if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
        }
    }

    
    
    
    
    rv = net_NewTransportEventSinkProxy(getter_AddRefs(mTransportSink),
                                        eventsink, target,
                                        !activityDistributorActive);
    if (NS_FAILED(rv)) return rv;

    mConnInfo = cinfo;
    mCallbacks = callbacks;
    mConsumerTarget = target;
    mCaps = caps;

    if (requestHead->IsHead()) {
        mNoContent = true;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    if ((requestHead->IsPost() || requestHead->IsPut()) &&
        !requestBody && !requestHead->PeekHeader(nsHttp::Transfer_Encoding)) {
        requestHead->SetHeader(nsHttp::Content_Length, NS_LITERAL_CSTRING("0"));
    }

    
    mRequestHead = requestHead;

    
    
    bool pruneProxyHeaders = cinfo->UsingConnect();

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
            PR_Now(), 0,
            mReqHeaderBuf);

    
    
    
    nsCOMPtr<nsIInputStream> headers;
    rv = NS_NewByteInputStream(getter_AddRefs(headers),
                               mReqHeaderBuf.get(),
                               mReqHeaderBuf.Length());
    if (NS_FAILED(rv)) return rv;

    if (requestBody) {
        mHasRequestBody = true;

        
        nsCOMPtr<nsIMultiplexInputStream> multi =
            do_CreateInstance(kMultiplexInputStream, &rv);
        if (NS_FAILED(rv)) return rv;

        rv = multi->AppendStream(headers);
        if (NS_FAILED(rv)) return rv;

        rv = multi->AppendStream(requestBody);
        if (NS_FAILED(rv)) return rv;

        
        
        
        rv = NS_NewBufferedInputStream(getter_AddRefs(mRequestStream), multi,
                                       nsIOService::gDefaultSegmentSize);
        if (NS_FAILED(rv)) return rv;
    }
    else
        mRequestStream = headers;

    uint64_t size_u64;
    rv = mRequestStream->Available(&size_u64);
    if (NS_FAILED(rv)) {
        return rv;
    }

    
    mRequestSize = InScriptableRange(size_u64) ? static_cast<int64_t>(size_u64) : -1;

    
    rv = NS_NewPipe2(getter_AddRefs(mPipeIn),
                     getter_AddRefs(mPipeOut),
                     true, true,
                     nsIOService::gDefaultSegmentSize,
                     nsIOService::gDefaultSegmentCount);
    if (NS_FAILED(rv)) return rv;

    Classify();

    nsCOMPtr<nsIAsyncInputStream> tmp(mPipeIn);
    tmp.forget(responseBody);
    return NS_OK;
}


nsAHttpConnection *
nsHttpTransaction::Connection()
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    return mConnection.get();
}

already_AddRefed<nsAHttpConnection>
nsHttpTransaction::GetConnectionReference()
{
    MutexAutoLock lock(mLock);
    nsRefPtr<nsAHttpConnection> connection(mConnection);
    return connection.forget();
}

nsHttpResponseHead *
nsHttpTransaction::TakeResponseHead()
{
    MOZ_ASSERT(!mResponseHeadTaken, "TakeResponseHead called 2x");

    
    MutexAutoLock lock(*nsHttp::GetLock());

    mResponseHeadTaken = true;

    
    
    nsHttpResponseHead *head;
    if (mForTakeResponseHead) {
        head = mForTakeResponseHead;
        mForTakeResponseHead = nullptr;
        return head;
    }

    
    
    if (!mHaveAllHeaders) {
        NS_WARNING("response headers not available or incomplete");
        return nullptr;
    }

    head = mResponseHead;
    mResponseHead = nullptr;
    return head;
}

void
nsHttpTransaction::SetProxyConnectFailed()
{
    mProxyConnectFailed = true;
}

nsHttpRequestHead *
nsHttpTransaction::RequestHead()
{
    return mRequestHead;
}

uint32_t
nsHttpTransaction::Http1xTransactionCount()
{
  return 1;
}

nsresult
nsHttpTransaction::TakeSubTransactions(
    nsTArray<nsRefPtr<nsAHttpTransaction> > &outTransactions)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}





void
nsHttpTransaction::SetConnection(nsAHttpConnection *conn)
{
    {
        MutexAutoLock lock(mLock);
        mConnection = conn;
    }

    if (conn) {
        MOZ_EVENT_TRACER_EXEC(static_cast<nsAHttpTransaction*>(this),
                              "net::http::transaction");
    }
}

void
nsHttpTransaction::GetSecurityCallbacks(nsIInterfaceRequestor **cb)
{
    MutexAutoLock lock(mLock);
    nsCOMPtr<nsIInterfaceRequestor> tmp(mCallbacks);
    tmp.forget(cb);
}

void
nsHttpTransaction::SetSecurityCallbacks(nsIInterfaceRequestor* aCallbacks)
{
    {
        MutexAutoLock lock(mLock);
        mCallbacks = aCallbacks;
    }

    if (gSocketTransportService) {
        nsRefPtr<UpdateSecurityCallbacks> event = new UpdateSecurityCallbacks(this, aCallbacks);
        gSocketTransportService->Dispatch(event, nsIEventTarget::DISPATCH_NORMAL);
    }
}

void
nsHttpTransaction::OnTransportStatus(nsITransport* transport,
                                     nsresult status, int64_t progress)
{
    LOG(("nsHttpTransaction::OnSocketStatus [this=%p status=%x progress=%lld]\n",
        this, status, progress));

    if (status == NS_NET_STATUS_CONNECTED_TO ||
        status == NS_NET_STATUS_WAITING_FOR) {
        nsISocketTransport *socketTransport =
            mConnection ? mConnection->Transport() : nullptr;
        if (socketTransport) {
            MutexAutoLock lock(mLock);
            socketTransport->GetSelfAddr(&mSelfAddr);
            socketTransport->GetPeerAddr(&mPeerAddr);
        }
    }

    
    
    
    
    
    if (TimingEnabled() && GetRequestStart().IsNull()) {
        if (status == NS_NET_STATUS_RESOLVING_HOST) {
            SetDomainLookupStart(TimeStamp::Now());
        } else if (status == NS_NET_STATUS_RESOLVED_HOST) {
            SetDomainLookupEnd(TimeStamp::Now());
        } else if (status == NS_NET_STATUS_CONNECTING_TO) {
            SetConnectStart(TimeStamp::Now());
        } else if (status == NS_NET_STATUS_CONNECTED_TO) {
            SetConnectEnd(TimeStamp::Now());
        }
    }

    if (!mTransportSink)
        return;

    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    
    
    if (mActivityDistributor) {
        
        if ((mHasRequestBody) &&
            (status == NS_NET_STATUS_WAITING_FOR))
            mActivityDistributor->ObserveActivity(
                mChannel,
                NS_HTTP_ACTIVITY_TYPE_HTTP_TRANSACTION,
                NS_HTTP_ACTIVITY_SUBTYPE_REQUEST_BODY_SENT,
                PR_Now(), 0, EmptyCString());

        
        if (!mRestartInProgressVerifier.IsDiscardingContent())
            mActivityDistributor->ObserveActivity(
                mChannel,
                NS_HTTP_ACTIVITY_TYPE_SOCKET_TRANSPORT,
                static_cast<uint32_t>(status),
                PR_Now(),
                progress,
                EmptyCString());
    }

    
    if (status == NS_NET_STATUS_RECEIVING_FROM)
        return;

    int64_t progressMax;

    if (status == NS_NET_STATUS_SENDING_TO) {
        
        if (!mHasRequestBody) {
            LOG(("nsHttpTransaction::OnTransportStatus %p "
                 "SENDING_TO without request body\n", this));
            return;
        }

        nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(mRequestStream);
        if (!seekable) {
            LOG(("nsHttpTransaction::OnTransportStatus %p "
                 "SENDING_TO without seekable request stream\n", this));
            progress = 0;
        } else {
            int64_t prog = 0;
            seekable->Tell(&prog);
            progress = prog;
        }

        
        
        progressMax = mRequestSize;
    }
    else {
        progress = 0;
        progressMax = 0;
    }

    mTransportSink->OnTransportStatus(transport, status, progress, progressMax);
}

bool
nsHttpTransaction::IsDone()
{
    return mTransactionDone;
}

nsresult
nsHttpTransaction::Status()
{
    return mStatus;
}

uint32_t
nsHttpTransaction::Caps()
{
    return mCaps & ~mCapsToClear;
}

void
nsHttpTransaction::SetDNSWasRefreshed()
{
    MOZ_ASSERT(NS_IsMainThread(), "SetDNSWasRefreshed on main thread only!");
    mCapsToClear |= NS_HTTP_REFRESH_DNS;
}

uint64_t
nsHttpTransaction::Available()
{
    uint64_t size;
    if (NS_FAILED(mRequestStream->Available(&size)))
        size = 0;
    return size;
}

NS_METHOD
nsHttpTransaction::ReadRequestSegment(nsIInputStream *stream,
                                      void *closure,
                                      const char *buf,
                                      uint32_t offset,
                                      uint32_t count,
                                      uint32_t *countRead)
{
    nsHttpTransaction *trans = (nsHttpTransaction *) closure;
    nsresult rv = trans->mReader->OnReadSegment(buf, count, countRead);
    if (NS_FAILED(rv)) return rv;

    if (trans->TimingEnabled()) {
        
        trans->SetRequestStart(TimeStamp::Now(), true);
    }

    if (!trans->mSentData) {
        MOZ_EVENT_TRACER_MARK(static_cast<nsAHttpTransaction*>(trans),
                              "net::http::first-write");
    }

    trans->CountSentBytes(*countRead);
    trans->mSentData = true;
    return NS_OK;
}

nsresult
nsHttpTransaction::ReadSegments(nsAHttpSegmentReader *reader,
                                uint32_t count, uint32_t *countRead)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    if (mTransactionDone) {
        *countRead = 0;
        return mStatus;
    }

    if (!mConnected) {
        mConnected = true;
        mConnection->GetSecurityInfo(getter_AddRefs(mSecurityInfo));
    }

    mReader = reader;

    nsresult rv = mRequestStream->ReadSegments(ReadRequestSegment, this, count, countRead);

    mReader = nullptr;

    
    
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
                                    uint32_t offset,
                                    uint32_t count,
                                    uint32_t *countWritten)
{
    nsHttpTransaction *trans = (nsHttpTransaction *) closure;

    if (trans->mTransactionDone)
        return NS_BASE_STREAM_CLOSED; 

    if (trans->TimingEnabled()) {
        
        trans->SetResponseStart(TimeStamp::Now(), true);
    }

    nsresult rv;
    
    
    
    rv = trans->mWriter->OnWriteSegment(buf, count, countWritten);
    if (NS_FAILED(rv)) return rv; 

    if (!trans->mReceivedData) {
        MOZ_EVENT_TRACER_MARK(static_cast<nsAHttpTransaction*>(trans),
                              "net::http::first-read");
    }

    MOZ_ASSERT(*countWritten > 0, "bad writer");
    trans->CountRecvBytes(*countWritten);
    trans->mReceivedData = true;

    
    
    
    
    
    
    rv = trans->ProcessData(buf, *countWritten, countWritten);
    if (NS_FAILED(rv))
        trans->Close(rv);

    return rv; 
}

nsresult
nsHttpTransaction::WriteSegments(nsAHttpSegmentWriter *writer,
                                 uint32_t count, uint32_t *countWritten)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    if (mTransactionDone)
        return NS_SUCCEEDED(mStatus) ? NS_BASE_STREAM_CLOSED : mStatus;

    mWriter = writer;

    nsresult rv = mPipeOut->WriteSegments(WritePipeSegment, this, count, countWritten);

    mWriter = nullptr;

    
    
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

nsresult
nsHttpTransaction::SaveNetworkStats(bool enforce)
{
#ifdef MOZ_WIDGET_GONK
    
    if (!mActiveNetwork || mAppId == NECKO_NO_APP_ID) {
        return NS_OK;
    }

    if (mCountRecv <= 0 && mCountSent <= 0) {
        
        return NS_OK;
    }

    
    
    
    uint64_t totalBytes = mCountRecv + mCountSent;
    if (!enforce && totalBytes < NETWORK_STATS_THRESHOLD) {
        return NS_OK;
    }

    
    
    nsRefPtr<nsRunnable> event =
        new SaveNetworkStatsEvent(mAppId, mIsInBrowser, mActiveNetwork,
                                  mCountRecv, mCountSent, false);
    NS_DispatchToMainThread(event);

    
    mCountSent = 0;
    mCountRecv = 0;

    return NS_OK;
#else
    return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

void
nsHttpTransaction::Close(nsresult reason)
{
    LOG(("nsHttpTransaction::Close [this=%p reason=%x]\n", this, reason));

    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    if (mClosed) {
        LOG(("  already closed\n"));
        return;
    }

    if (mTokenBucketCancel) {
        mTokenBucketCancel->Cancel(reason);
        mTokenBucketCancel = nullptr;
    }

    if (mActivityDistributor) {
        
        if (!mResponseIsComplete)
            mActivityDistributor->ObserveActivity(
                mChannel,
                NS_HTTP_ACTIVITY_TYPE_HTTP_TRANSACTION,
                NS_HTTP_ACTIVITY_SUBTYPE_RESPONSE_COMPLETE,
                PR_Now(),
                static_cast<uint64_t>(mContentRead),
                EmptyCString());

        
        mActivityDistributor->ObserveActivity(
            mChannel,
            NS_HTTP_ACTIVITY_TYPE_HTTP_TRANSACTION,
            NS_HTTP_ACTIVITY_SUBTYPE_TRANSACTION_CLOSE,
            PR_Now(), 0, EmptyCString());
    }

    
    
    bool connReused = false;
    if (mConnection)
        connReused = mConnection->IsReused();
    mConnected = false;
    mTunnelProvider = nullptr;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (reason == NS_ERROR_NET_RESET || reason == NS_OK) {

        if (mForceRestart && NS_SUCCEEDED(Restart())) {
            LOG(("transaction force restarted\n"));
            return;
        }

        
        
        
        bool reallySentData =
            mSentData && (!mConnection || mConnection->BytesWritten());

        if (!mReceivedData &&
            (!reallySentData || connReused || mPipelinePosition)) {
            
            

            if (mPipelinePosition) {
                gHttpHandler->ConnMgr()->PipelineFeedbackInfo(
                    mConnInfo, nsHttpConnectionMgr::RedCanceledPipeline,
                    nullptr, 0);
            }
            if (NS_SUCCEEDED(Restart()))
                return;
        }
        else if (!mResponseIsComplete && mPipelinePosition &&
                 reason == NS_ERROR_NET_RESET) {
            
            

            gHttpHandler->ConnMgr()->PipelineFeedbackInfo(
                mConnInfo, nsHttpConnectionMgr::RedCorruptedContent, nullptr, 0);
            if (NS_SUCCEEDED(RestartInProgress()))
                return;
        }
    }

    if ((mChunkedDecoder || (mContentLength >= int64_t(0))) &&
        (NS_SUCCEEDED(reason) && !mResponseIsComplete)) {

        NS_WARNING("Partial transfer, incomplete HTTP response received");

        if (mHttpVersion >= NS_HTTP_VERSION_1_1) {
            FrameCheckLevel clevel = gHttpHandler->GetEnforceH1Framing();
            if (clevel >= FRAMECHECK_BARELY) {
                if ((clevel == FRAMECHECK_STRICT) ||
                    (mChunkedDecoder && mChunkedDecoder->GetChunkRemaining()) ||
                    (!mChunkedDecoder && !mContentDecoding && mContentDecodingCheck) ) {
                    reason = NS_ERROR_NET_PARTIAL_TRANSFER;
                    LOG(("Partial transfer, incomplete HTTP response received: %s",
                         mChunkedDecoder ? "broken chunk" : "c-l underrun"));
                }
            }
        }

        if (mConnection) {
            
            
            mConnection->DontReuse();
        }
    }

    bool relConn = true;
    if (NS_SUCCEEDED(reason)) {
        if (!mResponseIsComplete) {
            
            
            
            gHttpHandler->ConnMgr()->PipelineFeedbackInfo(
                mConnInfo, nsHttpConnectionMgr::BadInsufficientFraming,
                nullptr, mClassification);
        }
        else if (mPipelinePosition) {
            
            gHttpHandler->ConnMgr()->PipelineFeedbackInfo(
                mConnInfo, nsHttpConnectionMgr::GoodCompletedOK,
                nullptr, mPipelinePosition);
        }

        
        
        
        
        if (!mHaveAllHeaders) {
            char data = '\n';
            uint32_t unused;
            ParseHead(&data, 1, &unused);

            if (mResponseHead->Version() == NS_HTTP_VERSION_0_9) {
                
                LOG(("nsHttpTransaction::Close %p 0 Byte 0.9 Response", this));
                reason = NS_ERROR_NET_RESET;
            }
        }

        
        if (mCaps & NS_HTTP_STICKY_CONNECTION)
            relConn = false;
    }

    
    
    
    if (TimingEnabled()) {
        const TimingStruct timings = Timings();
        if (timings.responseEnd.IsNull() && !timings.responseStart.IsNull()) {
            SetResponseEnd(TimeStamp::Now());
        }
    }

    if (relConn && mConnection) {
        MutexAutoLock lock(mLock);
        mConnection = nullptr;
    }

    
    SaveNetworkStats(true);

    mStatus = reason;
    mTransactionDone = true; 
    mClosed = true;
    ReleaseBlockingTransaction();

    
    mRequestStream = nullptr;
    mReqHeaderBuf.Truncate();
    mLineBuf.Truncate();
    if (mChunkedDecoder) {
        delete mChunkedDecoder;
        mChunkedDecoder = nullptr;
    }

    
    mPipeOut->CloseWithStatus(reason);

    MOZ_EVENT_TRACER_DONE(static_cast<nsAHttpTransaction*>(this),
                          "net::http::transaction");
}

nsHttpConnectionInfo *
nsHttpTransaction::ConnectionInfo()
{
    return mConnInfo.get();
}

nsresult
nsHttpTransaction::AddTransaction(nsAHttpTransaction *trans)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

uint32_t
nsHttpTransaction::PipelineDepth()
{
    return IsDone() ? 0 : 1;
}

nsresult
nsHttpTransaction::SetPipelinePosition(int32_t position)
{
    mPipelinePosition = position;
    return NS_OK;
}

int32_t
nsHttpTransaction::PipelinePosition()
{
    return mPipelinePosition;
}

bool 
nsAHttpTransaction::ResponseTimeoutEnabled() const
{
    return false;
}

PRIntervalTime 
nsAHttpTransaction::ResponseTimeout()
{
    return gHttpHandler->ResponseTimeout();
}

bool
nsHttpTransaction::ResponseTimeoutEnabled() const
{
    return mResponseTimeoutEnabled;
}





nsresult
nsHttpTransaction::RestartInProgress()
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    if ((mRestartCount + 1) >= gHttpHandler->MaxRequestAttempts()) {
        LOG(("nsHttpTransaction::RestartInProgress() "
             "reached max request attempts, failing transaction %p\n", this));
        return NS_ERROR_NET_RESET;
    }

    
    MutexAutoLock lock(*nsHttp::GetLock());

    
    
    
    if (!mHaveAllHeaders)
        return NS_ERROR_NET_RESET;

    
    if (!mRestartInProgressVerifier.IsSetup())
        return NS_ERROR_NET_RESET;

    LOG(("Will restart transaction %p and skip first %lld bytes, "
         "old Content-Length %lld",
         this, mContentRead, mContentLength));

    mRestartInProgressVerifier.SetAlreadyProcessed(
        std::max(mRestartInProgressVerifier.AlreadyProcessed(), mContentRead));

    if (!mResponseHeadTaken && !mForTakeResponseHead) {
        
        
        
        
        
        mForTakeResponseHead = mResponseHead;
        mResponseHead = nullptr;
    }

    if (mResponseHead) {
        mResponseHead->Reset();
    }

    mContentRead = 0;
    mContentLength = -1;
    delete mChunkedDecoder;
    mChunkedDecoder = nullptr;
    mHaveStatusLine = false;
    mHaveAllHeaders = false;
    mHttpResponseMatched = false;
    mResponseIsComplete = false;
    mDidContentStart = false;
    mNoContent = false;
    mSentData = false;
    mReceivedData = false;

    return Restart();
}

nsresult
nsHttpTransaction::Restart()
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    
    if (++mRestartCount >= gHttpHandler->MaxRequestAttempts()) {
        LOG(("reached max request attempts, failing transaction @%p\n", this));
        return NS_ERROR_NET_RESET;
    }

    LOG(("restarting transaction @%p\n", this));
    mTunnelProvider = nullptr;

    
    nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(mRequestStream);
    if (seekable)
        seekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);

    
    mSecurityInfo = 0;
    if (mConnection) {
        if (!mReuseOnRestart) {
            mConnection->DontReuse();
        }
        MutexAutoLock lock(mLock);
        mConnection = nullptr;
    }

    
    
    mReuseOnRestart = false;

    
    
    
    mCaps &= ~NS_HTTP_ALLOW_PIPELINING;
    SetPipelinePosition(0);

    if (!mConnInfo->GetAuthenticationHost().IsEmpty()) {
        MutexAutoLock lock(*nsHttp::GetLock());
        nsRefPtr<nsHttpConnectionInfo> ci;
         mConnInfo->CloneAsDirectRoute(getter_AddRefs(ci));
         mConnInfo = ci;
        if (mRequestHead) {
            mRequestHead->SetHeader(nsHttp::Alternate_Service_Used, NS_LITERAL_CSTRING("0"));
        }
    }
    mForceRestart = false;

    return gHttpHandler->InitiateTransaction(this, mPriority);
}

char *
nsHttpTransaction::LocateHttpStart(char *buf, uint32_t len,
                                   bool aAllowPartialMatch)
{
    MOZ_ASSERT(!aAllowPartialMatch || mLineBuf.IsEmpty());

    static const char HTTPHeader[] = "HTTP/1.";
    static const uint32_t HTTPHeaderLen = sizeof(HTTPHeader) - 1;
    static const char HTTP2Header[] = "HTTP/2.0";
    static const uint32_t HTTP2HeaderLen = sizeof(HTTP2Header) - 1;
    
    static const char ICYHeader[] = "ICY ";
    static const uint32_t ICYHeaderLen = sizeof(ICYHeader) - 1;

    if (aAllowPartialMatch && (len < HTTPHeaderLen))
        return (PL_strncasecmp(buf, HTTPHeader, len) == 0) ? buf : nullptr;

    
    if (!mLineBuf.IsEmpty()) {
        MOZ_ASSERT(mLineBuf.Length() < HTTPHeaderLen);
        int32_t checkChars = std::min(len, HTTPHeaderLen - mLineBuf.Length());
        if (PL_strncasecmp(buf, HTTPHeader + mLineBuf.Length(),
                           checkChars) == 0) {
            mLineBuf.Append(buf, checkChars);
            if (mLineBuf.Length() == HTTPHeaderLen) {
                
                
                return (buf + checkChars);
            }
            
            return 0;
        }
        
        
        mLineBuf.Truncate();
    }

    bool firstByte = true;
    while (len > 0) {
        if (PL_strncasecmp(buf, HTTPHeader, std::min<uint32_t>(len, HTTPHeaderLen)) == 0) {
            if (len < HTTPHeaderLen) {
                
                
                mLineBuf.Assign(buf, len);
                return 0;
            }

            
            return buf;
        }

        
        
        
        

        if (firstByte && !mInvalidResponseBytesRead && len >= HTTP2HeaderLen &&
            (PL_strncasecmp(buf, HTTP2Header, HTTP2HeaderLen) == 0)) {
            LOG(("nsHttpTransaction:: Identified HTTP/2.0 treating as 1.x\n"));
            return buf;
        }

        
        
        

        if (firstByte && !mInvalidResponseBytesRead && len >= ICYHeaderLen &&
            (PL_strncasecmp(buf, ICYHeader, ICYHeaderLen) == 0)) {
            LOG(("nsHttpTransaction:: Identified ICY treating as HTTP/1.0\n"));
            return buf;
        }

        if (!nsCRT::IsAsciiSpace(*buf))
            firstByte = false;
        buf++;
        len--;
    }
    return 0;
}

nsresult
nsHttpTransaction::ParseLine(char *line)
{
    LOG(("nsHttpTransaction::ParseLine [%s]\n", line));
    nsresult rv = NS_OK;

    if (!mHaveStatusLine) {
        mResponseHead->ParseStatusLine(line);
        mHaveStatusLine = true;
        
        if (mResponseHead->Version() == NS_HTTP_VERSION_0_9)
            mHaveAllHeaders = true;
    }
    else {
        rv = mResponseHead->ParseHeaderLine(line);
    }
    return rv;
}

nsresult
nsHttpTransaction::ParseLineSegment(char *segment, uint32_t len)
{
    NS_PRECONDITION(!mHaveAllHeaders, "already have all headers");

    if (!mLineBuf.IsEmpty() && mLineBuf.Last() == '\n') {
        
        
        
        
        mLineBuf.Truncate(mLineBuf.Length() - 1);
        if (!mHaveStatusLine || (*segment != ' ' && *segment != '\t')) {
            nsresult rv = ParseLine(mLineBuf.BeginWriting());
            mLineBuf.Truncate();
            if (NS_FAILED(rv)) {
                gHttpHandler->ConnMgr()->PipelineFeedbackInfo(
                    mConnInfo, nsHttpConnectionMgr::RedCorruptedContent,
                    nullptr, 0);
                return rv;
            }
        }
    }

    
    mLineBuf.Append(segment, len);

    
    if (mLineBuf.First() == '\n') {
        mLineBuf.Truncate();
        
        uint16_t status = mResponseHead->Status();
        if ((status != 101) && (status / 100 == 1)) {
            LOG(("ignoring 1xx response\n"));
            mHaveStatusLine = false;
            mHttpResponseMatched = false;
            mConnection->SetLastTransactionExpectedNoContent(true);
            mResponseHead->Reset();
            return NS_OK;
        }
        mHaveAllHeaders = true;
    }
    return NS_OK;
}

nsresult
nsHttpTransaction::ParseHead(char *buf,
                             uint32_t count,
                             uint32_t *countRead)
{
    nsresult rv;
    uint32_t len;
    char *eol;

    LOG(("nsHttpTransaction::ParseHead [count=%u]\n", count));

    *countRead = 0;

    NS_PRECONDITION(!mHaveAllHeaders, "oops");

    
    if (!mResponseHead) {
        mResponseHead = new nsHttpResponseHead();
        if (!mResponseHead)
            return NS_ERROR_OUT_OF_MEMORY;

        
        if (mActivityDistributor && !mReportedStart) {
            mReportedStart = true;
            mActivityDistributor->ObserveActivity(
                mChannel,
                NS_HTTP_ACTIVITY_TYPE_HTTP_TRANSACTION,
                NS_HTTP_ACTIVITY_SUBTYPE_RESPONSE_START,
                PR_Now(), 0, EmptyCString());
        }
    }

    if (!mHttpResponseMatched) {
        
        
        
        
        
        if (!mConnection || !mConnection->LastTransactionExpectedNoContent()) {
            
            mHttpResponseMatched = true;
            char *p = LocateHttpStart(buf, std::min<uint32_t>(count, 11), true);
            if (!p) {
                
                if (mRequestHead->IsPut())
                    return NS_ERROR_ABORT;

                mResponseHead->ParseStatusLine("");
                mHaveStatusLine = true;
                mHaveAllHeaders = true;
                return NS_OK;
            }
            if (p > buf) {
                
                mInvalidResponseBytesRead += p - buf;
                *countRead = p - buf;
                buf = p;
            }
        }
        else {
            char *p = LocateHttpStart(buf, count, false);
            if (p) {
                mInvalidResponseBytesRead += p - buf;
                *countRead = p - buf;
                buf = p;
                mHttpResponseMatched = true;
            } else {
                mInvalidResponseBytesRead += count;
                *countRead = count;
                if (mInvalidResponseBytesRead > MAX_INVALID_RESPONSE_BODY_SIZE) {
                    LOG(("nsHttpTransaction::ParseHead() "
                         "Cannot find Response Header\n"));
                    
                    
                    return NS_ERROR_ABORT;
                }
                return NS_OK;
            }
        }
    }
    

    MOZ_ASSERT (mHttpResponseMatched);
    while ((eol = static_cast<char *>(memchr(buf, '\n', count - *countRead))) != nullptr) {
        
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

        if (!mHttpResponseMatched) {
            
            
            return NS_ERROR_NET_INTERRUPT;
        }
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
    LOG(("nsHttpTransaction::HandleContentStart [this=%p]\n", this));
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    if (mResponseHead) {
#if defined(PR_LOGGING)
        if (LOG3_ENABLED()) {
            LOG3(("http response [\n"));
            nsAutoCString headers;
            mResponseHead->Flatten(headers, false);
            LogHeaders(headers.get());
            LOG3(("]\n"));
        }
#endif
        
        
        mHttpVersion = mResponseHead->Version();

        
        bool reset = false;
        if (!mRestartInProgressVerifier.IsSetup())
            mConnection->OnHeadersAvailable(this, mRequestHead, mResponseHead, &reset);

        
        if (reset) {
            LOG(("resetting transaction's response head\n"));
            mHaveAllHeaders = false;
            mHaveStatusLine = false;
            mReceivedData = false;
            mSentData = false;
            mHttpResponseMatched = false;
            mResponseHead->Reset();
            
            return NS_OK;
        }

        
        switch (mResponseHead->Status()) {
        case 101:
            mPreserveStream = true;    
        case 204:
        case 205:
        case 304:
            mNoContent = true;
            LOG(("this response should not contain a body.\n"));
            break;
        case 421:
            LOG(("Misdirected Request.\n"));
            gHttpHandler->ConnMgr()->ClearHostMapping(mConnInfo);

            
            mCaps &= ~NS_HTTP_ALLOW_KEEPALIVE;
            mForceRestart = true; 
            break;
        }

        if (mResponseHead->Status() == 200 &&
            mConnection->IsProxyConnectInProgress()) {
            
            mNoContent = true;
        }
        mConnection->SetLastTransactionExpectedNoContent(mNoContent);
        if (mInvalidResponseBytesRead)
            gHttpHandler->ConnMgr()->PipelineFeedbackInfo(
                mConnInfo, nsHttpConnectionMgr::BadInsufficientFraming,
                nullptr, mClassification);

        if (mNoContent)
            mContentLength = 0;
        else {
            
            mContentLength = mResponseHead->ContentLength();

            if ((mClassification != CLASS_SOLO) &&
                (mContentLength > mMaxPipelineObjectSize))
                CancelPipeline(nsHttpConnectionMgr::BadUnexpectedLarge);

            
            
            
            
            
            if (mResponseHead->Version() >= NS_HTTP_VERSION_1_1 &&
                mResponseHead->HasHeaderValue(nsHttp::Transfer_Encoding, "chunked")) {
                
                mChunkedDecoder = new nsHttpChunkedDecoder();
                if (!mChunkedDecoder)
                    return NS_ERROR_OUT_OF_MEMORY;
                LOG(("chunked decoder created\n"));
                
                mContentLength = -1;
            }
#if defined(PR_LOGGING)
            else if (mContentLength == int64_t(-1))
                LOG(("waiting for the server to close the connection.\n"));
#endif
        }
        if (mRestartInProgressVerifier.IsSetup() &&
            !mRestartInProgressVerifier.Verify(mContentLength, mResponseHead)) {
            LOG(("Restart in progress subsequent transaction failed to match"));
            return NS_ERROR_ABORT;
        }
    }

    mDidContentStart = true;

    
    
    if (mRequestHead->IsGet())
        mRestartInProgressVerifier.Set(mContentLength, mResponseHead);

    return NS_OK;
}


nsresult
nsHttpTransaction::HandleContent(char *buf,
                                 uint32_t count,
                                 uint32_t *contentRead,
                                 uint32_t *contentRemaining)
{
    nsresult rv;

    LOG(("nsHttpTransaction::HandleContent [this=%p count=%u]\n", this, count));

    *contentRead = 0;
    *contentRemaining = 0;

    MOZ_ASSERT(mConnection);

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
    else if (mContentLength >= int64_t(0)) {
        
        
        
        
        if (mConnection->IsPersistent() || mPreserveStream ||
            mHttpVersion >= NS_HTTP_VERSION_1_1) {
            int64_t remaining = mContentLength - mContentRead;
            *contentRead = uint32_t(std::min<int64_t>(count, remaining));
            *contentRemaining = count - *contentRead;
        }
        else {
            *contentRead = count;
            
            int64_t position = mContentRead + int64_t(count);
            if (position > mContentLength) {
                mContentLength = position;
                
            }
        }
    }
    else {
        
        
        *contentRead = count;
    }

    int64_t toReadBeforeRestart =
        mRestartInProgressVerifier.ToReadBeforeRestart();

    if (toReadBeforeRestart && *contentRead) {
        uint32_t ignore =
            static_cast<uint32_t>(std::min<int64_t>(toReadBeforeRestart, UINT32_MAX));
        ignore = std::min(*contentRead, ignore);
        LOG(("Due To Restart ignoring %d of remaining %ld",
             ignore, toReadBeforeRestart));
        *contentRead -= ignore;
        mContentRead += ignore;
        mRestartInProgressVerifier.HaveReadBeforeRestart(ignore);
        memmove(buf, buf + ignore, *contentRead + *contentRemaining);
    }

    if (*contentRead) {
        
        mContentRead += *contentRead;
    }

    LOG(("nsHttpTransaction::HandleContent [this=%p count=%u read=%u mContentRead=%lld mContentLength=%lld]\n",
        this, count, *contentRead, mContentRead, mContentLength));

    
    
    if ((mClassification != CLASS_SOLO) &&
        mChunkedDecoder &&
        ((mContentRead + mChunkedDecoder->GetChunkRemaining()) >
         mMaxPipelineObjectSize)) {
        CancelPipeline(nsHttpConnectionMgr::BadUnexpectedLarge);
    }

    
    if ((mContentRead == mContentLength) ||
        (mChunkedDecoder && mChunkedDecoder->ReachedEOF())) {
        
        mTransactionDone = true;
        mResponseIsComplete = true;
        ReleaseBlockingTransaction();

        if (TimingEnabled()) {
            SetResponseEnd(TimeStamp::Now());
        }

        
        if (mActivityDistributor)
            mActivityDistributor->ObserveActivity(
                mChannel,
                NS_HTTP_ACTIVITY_TYPE_HTTP_TRANSACTION,
                NS_HTTP_ACTIVITY_SUBTYPE_RESPONSE_COMPLETE,
                PR_Now(),
                static_cast<uint64_t>(mContentRead),
                EmptyCString());
    }

    return NS_OK;
}

nsresult
nsHttpTransaction::ProcessData(char *buf, uint32_t count, uint32_t *countRead)
{
    nsresult rv;

    LOG(("nsHttpTransaction::ProcessData [this=%p count=%u]\n", this, count));

    *countRead = 0;

    
    if (!mHaveAllHeaders) {
        uint32_t bytesConsumed = 0;

        do {
            uint32_t localBytesConsumed = 0;
            char *localBuf = buf + bytesConsumed;
            uint32_t localCount = count - bytesConsumed;

            rv = ParseHead(localBuf, localCount, &localBytesConsumed);
            if (NS_FAILED(rv) && rv != NS_ERROR_NET_INTERRUPT)
                return rv;
            bytesConsumed += localBytesConsumed;
        } while (rv == NS_ERROR_NET_INTERRUPT);

        count -= bytesConsumed;

        
        if (count && bytesConsumed)
            memmove(buf, buf + bytesConsumed, count);

        
        if (mActivityDistributor && mResponseHead && mHaveAllHeaders &&
            !mReportedResponseHeader) {
            mReportedResponseHeader = true;
            nsAutoCString completeResponseHeaders;
            mResponseHead->Flatten(completeResponseHeaders, false);
            completeResponseHeaders.AppendLiteral("\r\n");
            mActivityDistributor->ObserveActivity(
                mChannel,
                NS_HTTP_ACTIVITY_TYPE_HTTP_TRANSACTION,
                NS_HTTP_ACTIVITY_SUBTYPE_RESPONSE_HEADER,
                PR_Now(), 0,
                completeResponseHeaders);
        }
    }

    
    
    if (mHaveAllHeaders) {
        uint32_t countRemaining = 0;
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        rv = HandleContent(buf, count, countRead, &countRemaining);
        if (NS_FAILED(rv)) return rv;
        
        
        if (mResponseIsComplete && countRemaining) {
            MOZ_ASSERT(mConnection);
            mConnection->PushBack(buf + *countRead, countRemaining);
        }

        if (!mContentDecodingCheck && mResponseHead) {
            mContentDecoding =
                !!mResponseHead->PeekHeader(nsHttp::Content_Encoding);
            mContentDecodingCheck = true;
        }
    }

    return NS_OK;
}

void
nsHttpTransaction::CancelPipeline(uint32_t reason)
{
    
    gHttpHandler->ConnMgr()->PipelineFeedbackInfo(
        mConnInfo,
        static_cast<nsHttpConnectionMgr::PipelineFeedbackInfoType>(reason),
        nullptr, mClassification);

    mConnection->CancelPipeline(NS_ERROR_ABORT);

    
    
    
    mClassification = CLASS_SOLO;
}


void
nsHttpTransaction::SetLoadGroupConnectionInfo(nsILoadGroupConnectionInfo *aLoadGroupCI)
{
    LOG(("nsHttpTransaction %p SetLoadGroupConnectionInfo %p\n", this, aLoadGroupCI));
    mLoadGroupCI = aLoadGroupCI;
}





void
nsHttpTransaction::DispatchedAsBlocking()
{
    if (mDispatchedAsBlocking)
        return;

    LOG(("nsHttpTransaction %p dispatched as blocking\n", this));

    if (!mLoadGroupCI)
        return;

    LOG(("nsHttpTransaction adding blocking transaction %p from "
         "loadgroup %p\n", this, mLoadGroupCI.get()));

    mLoadGroupCI->AddBlockingTransaction();
    mDispatchedAsBlocking = true;
}

void
nsHttpTransaction::RemoveDispatchedAsBlocking()
{
    if (!mLoadGroupCI || !mDispatchedAsBlocking)
        return;

    uint32_t blockers = 0;
    nsresult rv = mLoadGroupCI->RemoveBlockingTransaction(&blockers);

    LOG(("nsHttpTransaction removing blocking transaction %p from "
         "loadgroup %p. %d blockers remain.\n", this,
         mLoadGroupCI.get(), blockers));

    if (NS_SUCCEEDED(rv) && !blockers) {
        LOG(("nsHttpTransaction %p triggering release of blocked channels "
             " with loadgroupci=%p\n", this, mLoadGroupCI.get()));
        gHttpHandler->ConnMgr()->ProcessPendingQ();
    }

    mDispatchedAsBlocking = false;
}

void
nsHttpTransaction::ReleaseBlockingTransaction()
{
    RemoveDispatchedAsBlocking();
    LOG(("nsHttpTransaction %p loadgroupci set to null "
         "in ReleaseBlockingTransaction() - was %p\n", this, mLoadGroupCI.get()));
    mLoadGroupCI = nullptr;
}

void
nsHttpTransaction::DisableSpdy()
{
    mCaps |= NS_HTTP_DISALLOW_SPDY;
    if (mConnInfo) {
        
        
        mConnInfo->SetNoSpdy(true);
    }
}

const TimingStruct
nsHttpTransaction::Timings()
{
    mozilla::MutexAutoLock lock(mLock);
    TimingStruct timings = mTimings;
    return timings;
}

void
nsHttpTransaction::SetDomainLookupStart(mozilla::TimeStamp timeStamp, bool onlyIfNull)
{
    mozilla::MutexAutoLock lock(mLock);
    if (onlyIfNull && !mTimings.domainLookupStart.IsNull()) {
        return; 
    }
    mTimings.domainLookupStart = timeStamp;
}

void
nsHttpTransaction::SetDomainLookupEnd(mozilla::TimeStamp timeStamp, bool onlyIfNull)
{
    mozilla::MutexAutoLock lock(mLock);
    if (onlyIfNull && !mTimings.domainLookupEnd.IsNull()) {
        return; 
    }
    mTimings.domainLookupEnd = timeStamp;
}

void
nsHttpTransaction::SetConnectStart(mozilla::TimeStamp timeStamp, bool onlyIfNull)
{
    mozilla::MutexAutoLock lock(mLock);
    if (onlyIfNull && !mTimings.connectStart.IsNull()) {
        return; 
    }
    mTimings.connectStart = timeStamp;
}

void
nsHttpTransaction::SetConnectEnd(mozilla::TimeStamp timeStamp, bool onlyIfNull)
{
    mozilla::MutexAutoLock lock(mLock);
    if (onlyIfNull && !mTimings.connectEnd.IsNull()) {
        return; 
    }
    mTimings.connectEnd = timeStamp;
}

void
nsHttpTransaction::SetRequestStart(mozilla::TimeStamp timeStamp, bool onlyIfNull)
{
    mozilla::MutexAutoLock lock(mLock);
    if (onlyIfNull && !mTimings.requestStart.IsNull()) {
        return; 
    }
    mTimings.requestStart = timeStamp;
}

void
nsHttpTransaction::SetResponseStart(mozilla::TimeStamp timeStamp, bool onlyIfNull)
{
    mozilla::MutexAutoLock lock(mLock);
    if (onlyIfNull && !mTimings.responseStart.IsNull()) {
        return; 
    }
    mTimings.responseStart = timeStamp;
}

void
nsHttpTransaction::SetResponseEnd(mozilla::TimeStamp timeStamp, bool onlyIfNull)
{
    mozilla::MutexAutoLock lock(mLock);
    if (onlyIfNull && !mTimings.responseEnd.IsNull()) {
        return; 
    }
    mTimings.responseEnd = timeStamp;
}

mozilla::TimeStamp
nsHttpTransaction::GetDomainLookupStart()
{
    mozilla::MutexAutoLock lock(mLock);
    return mTimings.domainLookupStart;
}

mozilla::TimeStamp
nsHttpTransaction::GetDomainLookupEnd()
{
    mozilla::MutexAutoLock lock(mLock);
    return mTimings.domainLookupEnd;
}

mozilla::TimeStamp
nsHttpTransaction::GetConnectStart()
{
    mozilla::MutexAutoLock lock(mLock);
    return mTimings.connectStart;
}

mozilla::TimeStamp
nsHttpTransaction::GetConnectEnd()
{
    mozilla::MutexAutoLock lock(mLock);
    return mTimings.connectEnd;
}

mozilla::TimeStamp
nsHttpTransaction::GetRequestStart()
{
    mozilla::MutexAutoLock lock(mLock);
    return mTimings.requestStart;
}

mozilla::TimeStamp
nsHttpTransaction::GetResponseStart()
{
    mozilla::MutexAutoLock lock(mLock);
    return mTimings.responseStart;
}

mozilla::TimeStamp
nsHttpTransaction::GetResponseEnd()
{
    mozilla::MutexAutoLock lock(mLock);
    return mTimings.responseEnd;
}





class DeleteHttpTransaction : public nsRunnable {
public:
    explicit DeleteHttpTransaction(nsHttpTransaction *trans)
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
    LOG(("nsHttpTransaction::DeleteSelfOnConsumerThread [this=%p]\n", this));

    bool val;
    if (!mConsumerTarget ||
        (NS_SUCCEEDED(mConsumerTarget->IsOnCurrentThread(&val)) && val)) {
        delete this;
    } else {
        LOG(("proxying delete to consumer thread...\n"));
        nsCOMPtr<nsIRunnable> event = new DeleteHttpTransaction(this);
        if (NS_FAILED(mConsumerTarget->Dispatch(event, NS_DISPATCH_NORMAL)))
            NS_WARNING("failed to dispatch nsHttpDeleteTransaction event");
    }
}

bool
nsHttpTransaction::TryToRunPacedRequest()
{
    if (mSubmittedRatePacing)
        return mPassedRatePacing;

    mSubmittedRatePacing = true;
    mSynchronousRatePaceRequest = true;
    gHttpHandler->SubmitPacedRequest(this, getter_AddRefs(mTokenBucketCancel));
    mSynchronousRatePaceRequest = false;
    return mPassedRatePacing;
}

void
nsHttpTransaction::OnTokenBucketAdmitted()
{
    mPassedRatePacing = true;
    mTokenBucketCancel = nullptr;

    if (!mSynchronousRatePaceRequest)
        gHttpHandler->ConnMgr()->ProcessPendingQ(mConnInfo);
}

void
nsHttpTransaction::CancelPacing(nsresult reason)
{
    if (mTokenBucketCancel) {
        mTokenBucketCancel->Cancel(reason);
        mTokenBucketCancel = nullptr;
    }
}





NS_IMPL_ADDREF(nsHttpTransaction)

NS_IMETHODIMP_(MozExternalRefCountType)
nsHttpTransaction::Release()
{
    nsrefcnt count;
    NS_PRECONDITION(0 != mRefCnt, "dup release");
    count = --mRefCnt;
    NS_LOG_RELEASE(this, count, "nsHttpTransaction");
    if (0 == count) {
        mRefCnt = 1; 
        
        
        DeleteSelfOnConsumerThread();
        return 0;
    }
    return count;
}

NS_IMPL_QUERY_INTERFACE(nsHttpTransaction,
                        nsIInputStreamCallback,
                        nsIOutputStreamCallback)






NS_IMETHODIMP
nsHttpTransaction::OnInputStreamReady(nsIAsyncInputStream *out)
{
    if (mConnection) {
        mConnection->TransactionHasDataToWrite(this);
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



static bool
matchOld(nsHttpResponseHead *newHead, nsCString &old,
         nsHttpAtom headerAtom)
{
    const char *val;

    val = newHead->PeekHeader(headerAtom);
    if (val && old.IsEmpty())
        return false;
    if (!val && !old.IsEmpty())
        return false;
    if (val && !old.Equals(val))
        return false;
    return true;
}

bool
nsHttpTransaction::RestartVerifier::Verify(int64_t contentLength,
                                           nsHttpResponseHead *newHead)
{
    if (mContentLength != contentLength)
        return false;

    if (newHead->Status() != 200)
        return false;

    if (!matchOld(newHead, mContentRange, nsHttp::Content_Range))
        return false;

    if (!matchOld(newHead, mLastModified, nsHttp::Last_Modified))
        return false;

    if (!matchOld(newHead, mETag, nsHttp::ETag))
        return false;

    if (!matchOld(newHead, mContentEncoding, nsHttp::Content_Encoding))
        return false;

    if (!matchOld(newHead, mTransferEncoding, nsHttp::Transfer_Encoding))
        return false;

    return true;
}

void
nsHttpTransaction::RestartVerifier::Set(int64_t contentLength,
                                        nsHttpResponseHead *head)
{
    if (mSetup)
        return;

    
    

    
    if (!head || (head->Status() != 200)) {
        return;
    }

    mContentLength = contentLength;

    const char *val;
    val = head->PeekHeader(nsHttp::ETag);
    if (val) {
        mETag.Assign(val);
    }
    val = head->PeekHeader(nsHttp::Last_Modified);
    if (val) {
        mLastModified.Assign(val);
    }
    val = head->PeekHeader(nsHttp::Content_Range);
    if (val) {
        mContentRange.Assign(val);
    }
    val = head->PeekHeader(nsHttp::Content_Encoding);
    if (val) {
        mContentEncoding.Assign(val);
    }
    val = head->PeekHeader(nsHttp::Transfer_Encoding);
    if (val) {
        mTransferEncoding.Assign(val);
    }

    
    
    if (mETag.IsEmpty() && mLastModified.IsEmpty()) {
        return;
    }

    mSetup = true;
}

void
nsHttpTransaction::GetNetworkAddresses(NetAddr &self, NetAddr &peer)
{
    MutexAutoLock lock(mLock);
    self = mSelfAddr;
    peer = mPeerAddr;
}

} 
} 
