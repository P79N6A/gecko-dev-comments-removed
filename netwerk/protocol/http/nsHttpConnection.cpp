





#include "nsHttpConnection.h"
#include "nsHttpTransaction.h"
#include "nsHttpRequestHead.h"
#include "nsHttpResponseHead.h"
#include "nsHttpHandler.h"
#include "nsIOService.h"
#include "nsISocketTransportService.h"
#include "nsISocketTransport.h"
#include "nsIServiceManager.h"
#include "nsISSLSocketControl.h"
#include "nsStringStream.h"
#include "netCore.h"
#include "nsNetCID.h"
#include "nsProxyRelease.h"
#include "nsPreloadedStream.h"
#include "ASpdySession.h"
#include "mozilla/Telemetry.h"
#include "nsISupportsPriority.h"
#include "nsHttpPipeline.h"

#ifdef DEBUG

extern PRThread *gSocketThread;
#endif

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);

using namespace mozilla;
using namespace mozilla::net;





nsHttpConnection::nsHttpConnection()
    : mTransaction(nullptr)
    , mCallbacksLock("nsHttpConnection::mCallbacksLock")
    , mIdleTimeout(0)
    , mConsiderReusedAfterInterval(0)
    , mConsiderReusedAfterEpoch(0)
    , mCurrentBytesRead(0)
    , mMaxBytesRead(0)
    , mTotalBytesRead(0)
    , mTotalBytesWritten(0)
    , mKeepAlive(true) 
    , mKeepAliveMask(true)
    , mDontReuse(false)
    , mSupportsPipelining(false) 
    , mIsReused(false)
    , mCompletedProxyConnect(false)
    , mLastTransactionExpectedNoContent(false)
    , mIdleMonitoring(false)
    , mProxyConnectInProgress(false)
    , mHttp1xTransactionCount(0)
    , mRemainingConnectionUses(0xffffffff)
    , mClassification(nsAHttpTransaction::CLASS_GENERAL)
    , mNPNComplete(false)
    , mSetupNPNCalled(false)
    , mUsingSpdyVersion(0)
    , mPriority(nsISupportsPriority::PRIORITY_NORMAL)
    , mReportedSpdy(false)
    , mEverUsedSpdy(false)
{
    LOG(("Creating nsHttpConnection @%x\n", this));

    
    nsHttpHandler *handler = gHttpHandler;
    NS_ADDREF(handler);
}

nsHttpConnection::~nsHttpConnection()
{
    LOG(("Destroying nsHttpConnection @%x\n", this));

    
    nsHttpHandler *handler = gHttpHandler;
    NS_RELEASE(handler);

    if (!mEverUsedSpdy) {
        LOG(("nsHttpConnection %p performed %d HTTP/1.x transactions\n",
             this, mHttp1xTransactionCount));
        Telemetry::Accumulate(Telemetry::HTTP_REQUEST_PER_CONN,
                              mHttp1xTransactionCount);
    }

    if (mTotalBytesRead) {
        uint32_t totalKBRead = static_cast<uint32_t>(mTotalBytesRead >> 10);
        LOG(("nsHttpConnection %p read %dkb on connection spdy=%d\n",
             this, totalKBRead, mEverUsedSpdy));
        Telemetry::Accumulate(mEverUsedSpdy ?
                              Telemetry::SPDY_KBREAD_PER_CONN :
                              Telemetry::HTTP_KBREAD_PER_CONN,
                              totalKBRead);
    }
}

nsresult
nsHttpConnection::Init(nsHttpConnectionInfo *info,
                       uint16_t maxHangTime,
                       nsISocketTransport *transport,
                       nsIAsyncInputStream *instream,
                       nsIAsyncOutputStream *outstream,
                       nsIInterfaceRequestor *callbacks,
                       PRIntervalTime rtt)
{
    NS_ABORT_IF_FALSE(transport && instream && outstream,
                      "invalid socket information");
    LOG(("nsHttpConnection::Init [this=%p "
         "transport=%p instream=%p outstream=%p rtt=%d]\n",
         this, transport, instream, outstream,
         PR_IntervalToMilliseconds(rtt)));

    NS_ENSURE_ARG_POINTER(info);
    NS_ENSURE_TRUE(!mConnInfo, NS_ERROR_ALREADY_INITIALIZED);

    mConnInfo = info;
    mLastReadTime = PR_IntervalNow();
    mSupportsPipelining =
        gHttpHandler->ConnMgr()->SupportsPipelining(mConnInfo);
    mRtt = rtt;
    mMaxHangTime = PR_SecondsToInterval(maxHangTime);

    mSocketTransport = transport;
    mSocketIn = instream;
    mSocketOut = outstream;
    nsresult rv = mSocketTransport->SetEventSink(this, nullptr);
    NS_ENSURE_SUCCESS(rv, rv);

    mCallbacks = callbacks;
    rv = mSocketTransport->SetSecurityCallbacks(this);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

void
nsHttpConnection::StartSpdy(uint8_t spdyVersion)
{
    LOG(("nsHttpConnection::StartSpdy [this=%p]\n", this));

    NS_ABORT_IF_FALSE(!mSpdySession, "mSpdySession should be null");

    mUsingSpdyVersion = spdyVersion;
    mEverUsedSpdy = true;

    
    
    
    
    mIsReused = true;

    
    
    

    nsTArray<nsRefPtr<nsAHttpTransaction> > list;
    nsresult rv = mTransaction->TakeSubTransactions(list);

    if (rv == NS_ERROR_ALREADY_OPENED) {
        
        LOG(("TakeSubTranscations somehow called after "
             "nsAHttpTransaction began processing\n"));
        NS_ABORT_IF_FALSE(false,
                          "TakeSubTranscations somehow called after "
                          "nsAHttpTransaction began processing");
        mTransaction->Close(NS_ERROR_ABORT);
        return;
    }

    if (NS_FAILED(rv) && rv != NS_ERROR_NOT_IMPLEMENTED) {
        
        LOG(("unexpected rv from nnsAHttpTransaction::TakeSubTransactions()"));
        NS_ABORT_IF_FALSE(false,
                          "unexpected result from "
                          "nsAHttpTransaction::TakeSubTransactions()");
        mTransaction->Close(NS_ERROR_ABORT);
        return;
    }

    if (NS_FAILED(rv)) { 
        NS_ABORT_IF_FALSE(list.IsEmpty(), "sub transaction list not empty");

        
        
        
        mSpdySession = ASpdySession::NewSpdySession(spdyVersion,
                                                    mTransaction, mSocketTransport,
                                                    mPriority);
        LOG(("nsHttpConnection::StartSpdy moves single transaction %p "
             "into SpdySession %p\n", mTransaction.get(), mSpdySession.get()));
    }
    else {
        int32_t count = list.Length();

        LOG(("nsHttpConnection::StartSpdy moving transaction list len=%d "
             "into SpdySession %p\n", count, mSpdySession.get()));

        if (!count) {
            mTransaction->Close(NS_ERROR_ABORT);
            return;
        }

        for (int32_t index = 0; index < count; ++index) {
            if (!mSpdySession) {
                mSpdySession = ASpdySession::NewSpdySession(spdyVersion,
                                                            list[index], mSocketTransport, 
                                                            mPriority);
            }
            else {
                
                if (!mSpdySession->AddStream(list[index], mPriority)) {
                    NS_ABORT_IF_FALSE(false, "SpdySession::AddStream failed");
                    LOG(("SpdySession::AddStream failed\n"));
                    mTransaction->Close(NS_ERROR_ABORT);
                    return;
                }
            }
        }
    }

    mSupportsPipelining = false; 
    mTransaction = mSpdySession;
    mIdleTimeout = gHttpHandler->SpdyTimeout();
}

bool
nsHttpConnection::EnsureNPNComplete()
{
    
    
    
    

    if (!mSocketTransport) {
        
        NS_ABORT_IF_FALSE(false,
                          "EnsureNPNComplete socket transport precondition");
        mNPNComplete = true;
        return true;
    }

    if (mNPNComplete)
        return true;
    
    nsresult rv;

    nsCOMPtr<nsISupports> securityInfo;
    nsCOMPtr<nsISSLSocketControl> ssl;
    nsAutoCString negotiatedNPN;
    
    rv = mSocketTransport->GetSecurityInfo(getter_AddRefs(securityInfo));
    if (NS_FAILED(rv))
        goto npnComplete;

    ssl = do_QueryInterface(securityInfo, &rv);
    if (NS_FAILED(rv))
        goto npnComplete;

    rv = ssl->GetNegotiatedNPN(negotiatedNPN);
    if (rv == NS_ERROR_NOT_CONNECTED) {
    
        
        
        uint32_t count = 0;
        rv = mSocketOut->Write("", 0, &count);

        if (NS_FAILED(rv) && rv != NS_BASE_STREAM_WOULD_BLOCK)
            goto npnComplete;
        return false;
    }

    if (NS_FAILED(rv))
        goto npnComplete;

    LOG(("nsHttpConnection::EnsureNPNComplete %p negotiated to '%s'",
         this, negotiatedNPN.get()));

    uint8_t spdyVersion;
    rv = gHttpHandler->SpdyInfo()->GetNPNVersionIndex(negotiatedNPN,
                                                      &spdyVersion);
    if (NS_SUCCEEDED(rv))
        StartSpdy(spdyVersion);

    Telemetry::Accumulate(Telemetry::SPDY_NPN_CONNECT, UsingSpdy());

npnComplete:
    LOG(("nsHttpConnection::EnsureNPNComplete setting complete to true"));
    mNPNComplete = true;
    return true;
}


nsresult
nsHttpConnection::Activate(nsAHttpTransaction *trans, uint32_t caps, int32_t pri)
{
    nsresult rv;

    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    LOG(("nsHttpConnection::Activate [this=%x trans=%x caps=%x]\n",
         this, trans, caps));

    mPriority = pri;
    if (mTransaction && mUsingSpdyVersion)
        return AddTransaction(trans, pri);

    NS_ENSURE_ARG_POINTER(trans);
    NS_ENSURE_TRUE(!mTransaction, NS_ERROR_IN_PROGRESS);

    
    mLastReadTime = PR_IntervalNow();

    
    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    trans->GetSecurityCallbacks(getter_AddRefs(callbacks));
    SetSecurityCallbacks(callbacks);

    SetupNPN(caps); 

    
    mTransaction = trans;

    NS_ABORT_IF_FALSE(!mIdleMonitoring,
                      "Activating a connection with an Idle Monitor");
    mIdleMonitoring = false;

    
    mKeepAliveMask = mKeepAlive = (caps & NS_HTTP_ALLOW_KEEPALIVE);

    
    
    if (mConnInfo->UsingConnect() && !mCompletedProxyConnect) {
        rv = SetupProxyConnect();
        if (NS_FAILED(rv))
            goto failed_activation;
        mProxyConnectInProgress = true;
    }

    
    mCurrentBytesRead = 0;

    
    mInputOverflow = nullptr;

    rv = OnOutputStreamReady(mSocketOut);
    
failed_activation:
    if (NS_FAILED(rv)) {
        mTransaction = nullptr;
    }

    return rv;
}

void
nsHttpConnection::SetupNPN(uint32_t caps)
{
    if (mSetupNPNCalled)                                
        return;
    mSetupNPNCalled = true;

    
    if (!mNPNComplete) {

        mNPNComplete = true;

        if (mConnInfo->UsingSSL()) {
            LOG(("nsHttpConnection::SetupNPN Setting up "
                 "Next Protocol Negotiation"));
            nsCOMPtr<nsISupports> securityInfo;
            nsresult rv =
                mSocketTransport->GetSecurityInfo(getter_AddRefs(securityInfo));
            if (NS_FAILED(rv))
                return;

            nsCOMPtr<nsISSLSocketControl> ssl =
                do_QueryInterface(securityInfo, &rv);
            if (NS_FAILED(rv))
                return;

            nsTArray<nsCString> protocolArray;
            if (gHttpHandler->IsSpdyEnabled() &&
                !(caps & NS_HTTP_DISALLOW_SPDY)) {
                LOG(("nsHttpConnection::SetupNPN Allow SPDY NPN selection"));
                if (gHttpHandler->SpdyInfo()->ProtocolEnabled(0))
                    protocolArray.AppendElement(
                        gHttpHandler->SpdyInfo()->VersionString[0]);
                if (gHttpHandler->SpdyInfo()->ProtocolEnabled(1))
                    protocolArray.AppendElement(
                        gHttpHandler->SpdyInfo()->VersionString[1]);
            }

            protocolArray.AppendElement(NS_LITERAL_CSTRING("http/1.1"));
            if (NS_SUCCEEDED(ssl->SetNPNList(protocolArray))) {
                LOG(("nsHttpConnection::Init Setting up SPDY Negotiation OK"));
                mNPNComplete = false;
            }
        }
    }
}

void
nsHttpConnection::HandleAlternateProtocol(nsHttpResponseHead *responseHead)
{
    
    
    
    

    if (!gHttpHandler->IsSpdyEnabled() || mUsingSpdyVersion)
        return;

    const char *val = responseHead->PeekHeader(nsHttp::Alternate_Protocol);
    if (!val)
        return;

    
    
    
    

    uint8_t alternateProtocolVersion;
    if (NS_SUCCEEDED(gHttpHandler->SpdyInfo()->
                     GetAlternateProtocolVersionIndex(val,
                                                      &alternateProtocolVersion))) {
         LOG(("Connection %p Transaction %p found Alternate-Protocol "
             "header %s", this, mTransaction.get(), val));
        gHttpHandler->ConnMgr()->ReportSpdyAlternateProtocol(this);
    }
}

nsresult
nsHttpConnection::AddTransaction(nsAHttpTransaction *httpTransaction,
                                 int32_t priority)
{
    LOG(("nsHttpConnection::AddTransaction for SPDY"));

    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ABORT_IF_FALSE(mSpdySession && mUsingSpdyVersion,
                      "AddTransaction to live http connection without spdy");
    NS_ABORT_IF_FALSE(mTransaction,
                      "AddTransaction to idle http connection");
    
    if (!mSpdySession->AddStream(httpTransaction, priority)) {
        NS_ABORT_IF_FALSE(0, "AddStream should never fail due to"
                          "RoomForMore() admission check");
        return NS_ERROR_FAILURE;
    }

    ResumeSend();

    return NS_OK;
}

void
nsHttpConnection::Close(nsresult reason)
{
    LOG(("nsHttpConnection::Close [this=%x reason=%x]\n", this, reason));

    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (NS_FAILED(reason)) {
        if (mIdleMonitoring)
            EndIdleMonitoring();

        if (mSocketTransport) {
            mSocketTransport->SetEventSink(nullptr, nullptr);

            
            
            
            
            
            if (mSocketIn) {
                char buffer[4000];
                uint32_t count, total = 0;
                nsresult rv;
                do {
                    rv = mSocketIn->Read(buffer, 4000, &count);
                    if (NS_SUCCEEDED(rv))
                        total += count;
                }
                while (NS_SUCCEEDED(rv) && count > 0 && total < 64000);
                LOG(("nsHttpConnection::Close drained %d bytes\n", total));
            }
            
            mSocketTransport->SetSecurityCallbacks(nullptr);
            mSocketTransport->Close(reason);
            if (mSocketOut)
                mSocketOut->AsyncWait(nullptr, 0, 0, nullptr);
        }
        mKeepAlive = false;
    }
}


nsresult
nsHttpConnection::ProxyStartSSL()
{
    LOG(("nsHttpConnection::ProxyStartSSL [this=%x]\n", this));
#ifdef DEBUG
    NS_PRECONDITION(PR_GetCurrentThread() == gSocketThread, "wrong thread");
#endif

    nsCOMPtr<nsISupports> securityInfo;
    nsresult rv = mSocketTransport->GetSecurityInfo(getter_AddRefs(securityInfo));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISSLSocketControl> ssl = do_QueryInterface(securityInfo, &rv);
    if (NS_FAILED(rv)) return rv;

    return ssl->ProxyStartSSL();
}

void
nsHttpConnection::DontReuse()
{
    mKeepAliveMask = false;
    mKeepAlive = false;
    mDontReuse = true;
    mIdleTimeout = 0;
    if (mSpdySession)
        mSpdySession->DontReuse();
}


bool
nsHttpConnection::SupportsPipelining()
{
    if (mTransaction &&
        mTransaction->PipelineDepth() >= mRemainingConnectionUses) {
        LOG(("nsHttpConnection::SupportsPipelining this=%p deny pipeline "
             "because current depth %d exceeds max remaining uses %d\n",
             this, mTransaction->PipelineDepth(), mRemainingConnectionUses));
        return false;
    }
    return mSupportsPipelining && IsKeepAlive() && !mDontReuse;
}

bool
nsHttpConnection::CanReuse()
{
    if (mDontReuse)
        return false;

    if ((mTransaction ? mTransaction->PipelineDepth() : 0) >=
        mRemainingConnectionUses) {
        return false;
    }

    bool canReuse;
    
    if (mSpdySession)
        canReuse = mSpdySession->CanReuse();
    else
        canReuse = IsKeepAlive();

    canReuse = canReuse && (IdleTime() < mIdleTimeout) && IsAlive();

    
    
    
    

    uint64_t dataSize;
    if (canReuse && mSocketIn && !mUsingSpdyVersion && mHttp1xTransactionCount &&
        NS_SUCCEEDED(mSocketIn->Available(&dataSize)) && dataSize) {
        LOG(("nsHttpConnection::CanReuse %p %s"
             "Socket not reusable because read data pending (%llu) on it.\n",
             this, mConnInfo->Host(), dataSize));
        canReuse = false;
    }
    return canReuse;
}

bool
nsHttpConnection::CanDirectlyActivate()
{
    
    
    
    
    return UsingSpdy() && CanReuse() &&
        mSpdySession && mSpdySession->RoomForMoreStreams();
}

PRIntervalTime
nsHttpConnection::IdleTime()
{
    return mSpdySession ?
        mSpdySession->IdleTime() : (PR_IntervalNow() - mLastReadTime);
}



uint32_t
nsHttpConnection::TimeToLive()
{
    if (IdleTime() >= mIdleTimeout)
        return 0;
    uint32_t timeToLive = PR_IntervalToSeconds(mIdleTimeout - IdleTime());

    
    
    if (!timeToLive)
        timeToLive = 1;
    return timeToLive;
}

bool
nsHttpConnection::IsAlive()
{
    if (!mSocketTransport)
        return false;

    
    
    SetupNPN(0);

    bool alive;
    nsresult rv = mSocketTransport->IsAlive(&alive);
    if (NS_FAILED(rv))
        alive = false;


#ifdef TEST_RESTART_LOGIC
    if (!alive) {
        LOG(("pretending socket is still alive to test restart logic\n"));
        alive = true;
    }
#endif

    return alive;
}

bool
nsHttpConnection::SupportsPipelining(nsHttpResponseHead *responseHead)
{
    
    if (mUsingSpdyVersion)
        return false;

    
    if (mConnInfo->UsingHttpProxy() && !mConnInfo->UsingConnect()) {
        
        return true;
    }

    
    const char *val = responseHead->PeekHeader(nsHttp::Server);

    
    
    if (!val)
        return true;

    
    
    

    static const char *bad_servers[26][6] = {
        { nullptr }, { nullptr }, { nullptr }, { nullptr },                 
        { "EFAServer/", nullptr },                                       
        { nullptr }, { nullptr }, { nullptr }, { nullptr },                 
        { nullptr }, { nullptr }, { nullptr },                             
        { "Microsoft-IIS/4.", "Microsoft-IIS/5.", nullptr },             
        { "Netscape-Enterprise/3.", "Netscape-Enterprise/4.", 
          "Netscape-Enterprise/5.", "Netscape-Enterprise/6.", nullptr }, 
        { nullptr }, { nullptr }, { nullptr }, { nullptr },                 
        { nullptr }, { nullptr }, { nullptr }, { nullptr },                 
        { "WebLogic 3.", "WebLogic 4.","WebLogic 5.", "WebLogic 6.",
          "Winstone Servlet Engine v0.", nullptr },                      
        { nullptr }, { nullptr }, { nullptr }                              
    };  

    int index = val[0] - 'A'; 
    if ((index >= 0) && (index <= 25))
    {
        for (int i = 0; bad_servers[index][i] != nullptr; i++) {
            if (!PL_strncmp (val, bad_servers[index][i], strlen (bad_servers[index][i]))) {
                LOG(("looks like this server does not support pipelining"));
                gHttpHandler->ConnMgr()->PipelineFeedbackInfo(
                    mConnInfo, nsHttpConnectionMgr::RedBannedServer, this , 0);
                return false;
            }
        }
    }

    
    return true;
}





nsresult
nsHttpConnection::OnHeadersAvailable(nsAHttpTransaction *trans,
                                     nsHttpRequestHead *requestHead,
                                     nsHttpResponseHead *responseHead,
                                     bool *reset)
{
    LOG(("nsHttpConnection::OnHeadersAvailable [this=%p trans=%p response-head=%p]\n",
        this, trans, responseHead));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ENSURE_ARG_POINTER(trans);
    NS_ASSERTION(responseHead, "No response head?");

    
    
    
    
    uint16_t responseStatus = responseHead->Status();
    if (responseStatus == 408) {
        Close(NS_ERROR_NET_RESET);
        *reset = true;
        return NS_OK;
    }

    
    

    
    
    const char *val = responseHead->PeekHeader(nsHttp::Connection);
    if (!val)
        val = responseHead->PeekHeader(nsHttp::Proxy_Connection);

    
    mSupportsPipelining = false;

    if ((responseHead->Version() < NS_HTTP_VERSION_1_1) ||
        (requestHead->Version() < NS_HTTP_VERSION_1_1)) {
        
        if (val && !PL_strcasecmp(val, "keep-alive"))
            mKeepAlive = true;
        else
            mKeepAlive = false;
        
        
        gHttpHandler->ConnMgr()->PipelineFeedbackInfo(
            mConnInfo, nsHttpConnectionMgr::RedVersionTooLow, this, 0);
    }
    else {
        
        if (val && !PL_strcasecmp(val, "close")) {
            mKeepAlive = false;

            
            
            
            if (mRemainingConnectionUses > 1)
                gHttpHandler->ConnMgr()->PipelineFeedbackInfo(
                    mConnInfo, nsHttpConnectionMgr::BadExplicitClose, this, 0);
        }
        else {
            mKeepAlive = true;

            
            
            
            
            
            if (!mProxyConnectStream)
              mSupportsPipelining = SupportsPipelining(responseHead);
        }
    }
    mKeepAliveMask = mKeepAlive;

    
    
    
    
    if (mSupportsPipelining) {
        
        
        

        gHttpHandler->ConnMgr()->PipelineFeedbackInfo(
            mConnInfo, nsHttpConnectionMgr::NeutralExpectedOK, this, 0);

        mSupportsPipelining =
            gHttpHandler->ConnMgr()->SupportsPipelining(mConnInfo);
    }

    
    
    
    
    if (mClassification == nsAHttpTransaction::CLASS_REVALIDATION &&
        responseStatus != 304) {
        mClassification = nsAHttpTransaction::CLASS_GENERAL;
    }
    
    
    
    
    
    
    
    
    bool foundKeepAliveMax = false;
    if (mKeepAlive) {
        val = responseHead->PeekHeader(nsHttp::Keep_Alive);

        if (!mUsingSpdyVersion) {
            const char *cp = PL_strcasestr(val, "timeout=");
            if (cp)
                mIdleTimeout = PR_SecondsToInterval((uint32_t) atoi(cp + 8));
            else
                mIdleTimeout = gHttpHandler->IdleTimeout();

            cp = PL_strcasestr(val, "max=");
            if (cp) {
                int val = atoi(cp + 4);
                if (val > 0) {
                    foundKeepAliveMax = true;
                    mRemainingConnectionUses = static_cast<uint32_t>(val);
                }
            }
        }
        else {
            mIdleTimeout = gHttpHandler->SpdyTimeout();
        }
        
        LOG(("Connection can be reused [this=%p idle-timeout=%usec]\n",
             this, PR_IntervalToSeconds(mIdleTimeout)));
    }

    if (!foundKeepAliveMax && mRemainingConnectionUses && !mUsingSpdyVersion)
        --mRemainingConnectionUses;

    if (!mProxyConnectStream)
        HandleAlternateProtocol(responseHead);

    
    
    
    
    if (mProxyConnectStream) {
        NS_ABORT_IF_FALSE(!mUsingSpdyVersion,
                          "SPDY NPN Complete while using proxy connect stream");
        mProxyConnectStream = 0;
        if (responseStatus == 200) {
            LOG(("proxy CONNECT succeeded! ssl=%s\n",
                 mConnInfo->UsingSSL() ? "true" :"false"));
            *reset = true;
            nsresult rv;
            if (mConnInfo->UsingSSL()) {
                rv = ProxyStartSSL();
                if (NS_FAILED(rv)) 
                    LOG(("ProxyStartSSL failed [rv=%x]\n", rv));
            }
            mCompletedProxyConnect = true;
            mProxyConnectInProgress = false;
            rv = mSocketOut->AsyncWait(this, 0, 0, nullptr);
            
            NS_ASSERTION(NS_SUCCEEDED(rv), "mSocketOut->AsyncWait failed");
        }
        else {
            LOG(("proxy CONNECT failed! ssl=%s\n",
                 mConnInfo->UsingSSL() ? "true" :"false"));
            mTransaction->SetProxyConnectFailed();
        }
    }
    
    const char *upgradeReq = requestHead->PeekHeader(nsHttp::Upgrade);
    
    
    if (upgradeReq && responseStatus != 401 && responseStatus != 407) {
        LOG(("HTTP Upgrade in play - disable keepalive\n"));
        DontReuse();
    }
    
    if (responseStatus == 101) {
        const char *upgradeResp = responseHead->PeekHeader(nsHttp::Upgrade);
        if (!upgradeReq || !upgradeResp ||
            !nsHttp::FindToken(upgradeResp, upgradeReq,
                               HTTP_HEADER_VALUE_SEPS)) {
            LOG(("HTTP 101 Upgrade header mismatch req = %s, resp = %s\n",
                 upgradeReq, upgradeResp));
            Close(NS_ERROR_ABORT);
        }
        else {
            LOG(("HTTP Upgrade Response to %s\n", upgradeResp));
        }
    }

    return NS_OK;
}

bool
nsHttpConnection::IsReused()
{
    if (mIsReused)
        return true;
    if (!mConsiderReusedAfterInterval)
        return false;
    
    
    
    return (PR_IntervalNow() - mConsiderReusedAfterEpoch) >=
        mConsiderReusedAfterInterval;
}

void
nsHttpConnection::SetIsReusedAfter(uint32_t afterMilliseconds)
{
    mConsiderReusedAfterEpoch = PR_IntervalNow();
    mConsiderReusedAfterInterval = PR_MillisecondsToInterval(afterMilliseconds);
}

nsresult
nsHttpConnection::TakeTransport(nsISocketTransport  **aTransport,
                                nsIAsyncInputStream **aInputStream,
                                nsIAsyncOutputStream **aOutputStream)
{
    if (mUsingSpdyVersion)
        return NS_ERROR_FAILURE;
    if (mTransaction && !mTransaction->IsDone())
        return NS_ERROR_IN_PROGRESS;
    if (!(mSocketTransport && mSocketIn && mSocketOut))
        return NS_ERROR_NOT_INITIALIZED;

    if (mInputOverflow)
        mSocketIn = mInputOverflow.forget();

    NS_IF_ADDREF(*aTransport = mSocketTransport);
    NS_IF_ADDREF(*aInputStream = mSocketIn);
    NS_IF_ADDREF(*aOutputStream = mSocketOut);

    mSocketTransport->SetSecurityCallbacks(nullptr);
    mSocketTransport->SetEventSink(nullptr, nullptr);
    mSocketTransport = nullptr;
    mSocketIn = nullptr;
    mSocketOut = nullptr;
    
    return NS_OK;
}

void
nsHttpConnection::ReadTimeoutTick(PRIntervalTime now)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    
    if (!mTransaction)
        return;

    
    if (mSpdySession) {
        mSpdySession->ReadTimeoutTick(now);
        return;
    }
    
    if (!gHttpHandler->GetPipelineRescheduleOnTimeout())
        return;

    PRIntervalTime delta = now - mLastReadTime;

    
    
    
    
    
    
    
    

    uint32_t pipelineDepth = mTransaction->PipelineDepth();

    if (delta >= gHttpHandler->GetPipelineRescheduleTimeout() &&
        pipelineDepth > 1) {

        
        
        LOG(("cancelling pipeline due to a %ums stall - depth %d\n",
             PR_IntervalToMilliseconds(delta), pipelineDepth));

        nsHttpPipeline *pipeline = mTransaction->QueryPipeline();
        NS_ABORT_IF_FALSE(pipeline, "pipelinedepth > 1 without pipeline");
        
        
        
        if (pipeline) {
            pipeline->CancelPipeline(NS_ERROR_NET_TIMEOUT);
            LOG(("Rescheduling the head of line blocked members of a pipeline "
                 "because reschedule-timeout idle interval exceeded"));
        }
    }

    if (delta < gHttpHandler->GetPipelineTimeout())
        return;

    if (pipelineDepth <= 1 && !mTransaction->PipelinePosition())
        return;
    
    
    
    
    
    

    LOG(("canceling transaction stalled for %ums on a pipeline "
         "of depth %d and scheduled originally at pos %d\n",
         PR_IntervalToMilliseconds(delta),
         pipelineDepth, mTransaction->PipelinePosition()));

    
    CloseTransaction(mTransaction, NS_ERROR_NET_TIMEOUT);
}

void
nsHttpConnection::GetSecurityInfo(nsISupports **secinfo)
{
    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (mSocketTransport) {
        if (NS_FAILED(mSocketTransport->GetSecurityInfo(secinfo)))
            *secinfo = nullptr;
    }
}

void
nsHttpConnection::SetSecurityCallbacks(nsIInterfaceRequestor* aCallbacks)
{
    MutexAutoLock lock(mCallbacksLock);
    mCallbacks = aCallbacks;
}

nsresult
nsHttpConnection::PushBack(const char *data, uint32_t length)
{
    LOG(("nsHttpConnection::PushBack [this=%p, length=%d]\n", this, length));

    if (mInputOverflow) {
        NS_ERROR("nsHttpConnection::PushBack only one buffer supported");
        return NS_ERROR_UNEXPECTED;
    }
    
    mInputOverflow = new nsPreloadedStream(mSocketIn, data, length);
    return NS_OK;
}

nsresult
nsHttpConnection::ResumeSend()
{
    LOG(("nsHttpConnection::ResumeSend [this=%p]\n", this));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (mSocketOut)
        return mSocketOut->AsyncWait(this, 0, 0, nullptr);

    NS_NOTREACHED("no socket output stream");
    return NS_ERROR_UNEXPECTED;
}

nsresult
nsHttpConnection::ResumeRecv()
{
    LOG(("nsHttpConnection::ResumeRecv [this=%p]\n", this));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    
    
    
    
    
    
    mLastReadTime = PR_IntervalNow();

    if (mSocketIn)
        return mSocketIn->AsyncWait(this, 0, 0, nullptr);

    NS_NOTREACHED("no socket input stream");
    return NS_ERROR_UNEXPECTED;
}

void
nsHttpConnection::BeginIdleMonitoring()
{
    LOG(("nsHttpConnection::BeginIdleMonitoring [this=%p]\n", this));
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ABORT_IF_FALSE(!mTransaction, "BeginIdleMonitoring() while active");
    NS_ABORT_IF_FALSE(!mUsingSpdyVersion, "Idle monitoring of spdy not allowed");

    LOG(("Entering Idle Monitoring Mode [this=%p]", this));
    mIdleMonitoring = true;
    if (mSocketIn)
        mSocketIn->AsyncWait(this, 0, 0, nullptr);
}

void
nsHttpConnection::EndIdleMonitoring()
{
    LOG(("nsHttpConnection::EndIdleMonitoring [this=%p]\n", this));
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ABORT_IF_FALSE(!mTransaction, "EndIdleMonitoring() while active");

    if (mIdleMonitoring) {
        LOG(("Leaving Idle Monitoring Mode [this=%p]", this));
        mIdleMonitoring = false;
        if (mSocketIn)
            mSocketIn->AsyncWait(nullptr, 0, 0, nullptr);
    }
}





void
nsHttpConnection::CloseTransaction(nsAHttpTransaction *trans, nsresult reason)
{
    LOG(("nsHttpConnection::CloseTransaction[this=%x trans=%x reason=%x]\n",
        this, trans, reason));

    NS_ASSERTION(trans == mTransaction, "wrong transaction");
    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (mCurrentBytesRead > mMaxBytesRead)
        mMaxBytesRead = mCurrentBytesRead;

    
    if (reason == NS_BASE_STREAM_CLOSED)
        reason = NS_OK;

    if (mUsingSpdyVersion) {
        DontReuse();
        
        mUsingSpdyVersion = 0;
        mSpdySession = nullptr;
    }

    if (mTransaction) {
        mHttp1xTransactionCount += mTransaction->Http1xTransactionCount();

        mTransaction->Close(reason);
        mTransaction = nullptr;
    }

    {
        MutexAutoLock lock(mCallbacksLock);
        mCallbacks = nullptr;
    }

    if (NS_FAILED(reason))
        Close(reason);

    
    
    mIsReused = true;
}

NS_METHOD
nsHttpConnection::ReadFromStream(nsIInputStream *input,
                                 void *closure,
                                 const char *buf,
                                 uint32_t offset,
                                 uint32_t count,
                                 uint32_t *countRead)
{
    
    nsHttpConnection *conn = (nsHttpConnection *) closure;
    return conn->OnReadSegment(buf, count, countRead);
}

nsresult
nsHttpConnection::OnReadSegment(const char *buf,
                                uint32_t count,
                                uint32_t *countRead)
{
    if (count == 0) {
        
        
        
        NS_ERROR("bad ReadSegments implementation");
        return NS_ERROR_FAILURE; 
    }

    nsresult rv = mSocketOut->Write(buf, count, countRead);
    if (NS_FAILED(rv))
        mSocketOutCondition = rv;
    else if (*countRead == 0)
        mSocketOutCondition = NS_BASE_STREAM_CLOSED;
    else {
        mSocketOutCondition = NS_OK; 
        if (!mProxyConnectInProgress)
            mTotalBytesWritten += *countRead;
    }

    return mSocketOutCondition;
}

nsresult
nsHttpConnection::OnSocketWritable()
{
    LOG(("nsHttpConnection::OnSocketWritable [this=%p] host=%s\n",
         this, mConnInfo->Host()));

    nsresult rv;
    uint32_t n;
    bool again = true;

    do {
        mSocketOutCondition = NS_OK;

        
        
        
        
        
        
        
        if (mProxyConnectStream) {
            LOG(("  writing CONNECT request stream\n"));
            rv = mProxyConnectStream->ReadSegments(ReadFromStream, this,
                                                      nsIOService::gDefaultSegmentSize,
                                                      &n);
        }
        else if (!EnsureNPNComplete()) {
            
            

            
            
            
            

            rv = NS_OK;
            mSocketOutCondition = NS_BASE_STREAM_WOULD_BLOCK;
            n = 0;
        }
        else {
            if (!mReportedSpdy) {
                mReportedSpdy = true;
                gHttpHandler->ConnMgr()->ReportSpdyConnection(this, mUsingSpdyVersion);
            }

            LOG(("  writing transaction request stream\n"));
            mProxyConnectInProgress = false;
            rv = mTransaction->ReadSegments(this, nsIOService::gDefaultSegmentSize, &n);
        }

        LOG(("  ReadSegments returned [rv=%x read=%u sock-cond=%x]\n",
            rv, n, mSocketOutCondition));

        
        if (rv == NS_BASE_STREAM_CLOSED && !mTransaction->IsDone()) {
            rv = NS_OK;
            n = 0;
        }

        if (NS_FAILED(rv)) {
            
            
            if (rv == NS_BASE_STREAM_WOULD_BLOCK)
                rv = NS_OK;
            again = false;
        }
        else if (NS_FAILED(mSocketOutCondition)) {
            if (mSocketOutCondition == NS_BASE_STREAM_WOULD_BLOCK)
                rv = mSocketOut->AsyncWait(this, 0, 0, nullptr); 
            else
                rv = mSocketOutCondition;
            again = false;
        }
        else if (n == 0) {
            
            
            
            
            
            
            mTransaction->OnTransportStatus(mSocketTransport,
                                            NS_NET_STATUS_WAITING_FOR,
                                            0);

            rv = ResumeRecv(); 
            again = false;
        }
        
    } while (again);

    return rv;
}

nsresult
nsHttpConnection::OnWriteSegment(char *buf,
                                 uint32_t count,
                                 uint32_t *countWritten)
{
    if (count == 0) {
        
        
        
        NS_ERROR("bad WriteSegments implementation");
        return NS_ERROR_FAILURE; 
    }

    nsresult rv = mSocketIn->Read(buf, count, countWritten);
    if (NS_FAILED(rv))
        mSocketInCondition = rv;
    else if (*countWritten == 0)
        mSocketInCondition = NS_BASE_STREAM_CLOSED;
    else
        mSocketInCondition = NS_OK; 

    return mSocketInCondition;
}

nsresult
nsHttpConnection::OnSocketReadable()
{
    LOG(("nsHttpConnection::OnSocketReadable [this=%x]\n", this));

    PRIntervalTime now = PR_IntervalNow();
    PRIntervalTime delta = now - mLastReadTime;

    if (mKeepAliveMask && (delta >= mMaxHangTime)) {
        LOG(("max hang time exceeded!\n"));
        
        
        mKeepAliveMask = false;
        gHttpHandler->ProcessPendingQ(mConnInfo);
    }

    
    
    
    
    
    
    

    if (delta > mRtt)
        delta -= mRtt;
    else
        delta = 0;

    static const PRIntervalTime k400ms  = PR_MillisecondsToInterval(400);

    if (delta >= (mRtt + gHttpHandler->GetPipelineRescheduleTimeout())) {
        LOG(("Read delta ms of %u causing slow read major "
             "event and pipeline cancellation",
             PR_IntervalToMilliseconds(delta)));

        gHttpHandler->ConnMgr()->PipelineFeedbackInfo(
            mConnInfo, nsHttpConnectionMgr::BadSlowReadMajor, this, 0);

        if (gHttpHandler->GetPipelineRescheduleOnTimeout() &&
            mTransaction->PipelineDepth() > 1) {
            nsHttpPipeline *pipeline = mTransaction->QueryPipeline();
            NS_ABORT_IF_FALSE(pipeline, "pipelinedepth > 1 without pipeline");
            
            
            
            if (pipeline) {
                pipeline->CancelPipeline(NS_ERROR_NET_TIMEOUT);
                LOG(("Rescheduling the head of line blocked members of a "
                     "pipeline because reschedule-timeout idle interval "
                     "exceeded"));
            }
        }
    }
    else if (delta > k400ms) {
        gHttpHandler->ConnMgr()->PipelineFeedbackInfo(
            mConnInfo, nsHttpConnectionMgr::BadSlowReadMinor, this, 0);
    }

    mLastReadTime = now;

    nsresult rv;
    uint32_t n;
    bool again = true;

    do {
        if (!mProxyConnectInProgress && !mNPNComplete) {
            
            
            
            
            
            

            LOG(("nsHttpConnection::OnSocketReadable %p return due to inactive "
                 "tunnel setup but incomplete NPN state\n", this));
            rv = NS_OK;
            break;
        }

        rv = mTransaction->WriteSegments(this, nsIOService::gDefaultSegmentSize, &n);
        if (NS_FAILED(rv)) {
            
            
            if (rv == NS_BASE_STREAM_WOULD_BLOCK)
                rv = NS_OK;
            again = false;
        }
        else {
            mCurrentBytesRead += n;
            mTotalBytesRead += n;
            if (NS_FAILED(mSocketInCondition)) {
                
                if (mSocketInCondition == NS_BASE_STREAM_WOULD_BLOCK)
                    rv = ResumeRecv();
                else
                    rv = mSocketInCondition;
                again = false;
            }
        }
        
    } while (again);

    return rv;
}

nsresult
nsHttpConnection::SetupProxyConnect()
{
    const char *val;

    LOG(("nsHttpConnection::SetupProxyConnect [this=%x]\n", this));

    NS_ENSURE_TRUE(!mProxyConnectStream, NS_ERROR_ALREADY_INITIALIZED);
    NS_ABORT_IF_FALSE(!mUsingSpdyVersion,
                      "SPDY NPN Complete while using proxy connect stream");

    nsAutoCString buf;
    nsresult rv = nsHttpHandler::GenerateHostPort(
            nsDependentCString(mConnInfo->Host()), mConnInfo->Port(), buf);
    if (NS_FAILED(rv))
        return rv;

    
    nsHttpRequestHead request;
    request.SetMethod(nsHttp::Connect);
    request.SetVersion(gHttpHandler->HttpVersion());
    request.SetRequestURI(buf);
    request.SetHeader(nsHttp::User_Agent, gHttpHandler->UserAgent());

    
    request.SetHeader(nsHttp::Proxy_Connection, NS_LITERAL_CSTRING("keep-alive"));
    request.SetHeader(nsHttp::Connection, NS_LITERAL_CSTRING("keep-alive"));

    val = mTransaction->RequestHead()->PeekHeader(nsHttp::Host);
    if (val) {
        
        
        request.SetHeader(nsHttp::Host, nsDependentCString(val));
    }

    val = mTransaction->RequestHead()->PeekHeader(nsHttp::Proxy_Authorization);
    if (val) {
        
        
        request.SetHeader(nsHttp::Proxy_Authorization, nsDependentCString(val));
    }

    buf.Truncate();
    request.Flatten(buf, false);
    buf.AppendLiteral("\r\n");

    return NS_NewCStringInputStream(getter_AddRefs(mProxyConnectStream), buf);
}





NS_IMPL_THREADSAFE_ISUPPORTS4(nsHttpConnection,
                              nsIInputStreamCallback,
                              nsIOutputStreamCallback,
                              nsITransportEventSink,
                              nsIInterfaceRequestor)






NS_IMETHODIMP
nsHttpConnection::OnInputStreamReady(nsIAsyncInputStream *in)
{
    NS_ASSERTION(in == mSocketIn, "unexpected stream");
    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (mIdleMonitoring) {
        NS_ABORT_IF_FALSE(!mTransaction, "Idle Input Event While Active");

        
        
        
        
        

        if (!CanReuse()) {
            LOG(("Server initiated close of idle conn %p\n", this));
            gHttpHandler->ConnMgr()->CloseIdleConnection(this);
            return NS_OK;
        }

        LOG(("Input data on idle conn %p, but not closing yet\n", this));
        return NS_OK;
    }

    
    if (!mTransaction) {
        LOG(("  no transaction; ignoring event\n"));
        return NS_OK;
    }

    nsresult rv = OnSocketReadable();
    if (NS_FAILED(rv))
        CloseTransaction(mTransaction, rv);

    return NS_OK;
}





NS_IMETHODIMP
nsHttpConnection::OnOutputStreamReady(nsIAsyncOutputStream *out)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ABORT_IF_FALSE(out == mSocketOut, "unexpected socket");

    
    if (!mTransaction) {
        LOG(("  no transaction; ignoring event\n"));
        return NS_OK;
    }

    nsresult rv = OnSocketWritable();
    if (NS_FAILED(rv))
        CloseTransaction(mTransaction, rv);

    return NS_OK;
}





NS_IMETHODIMP
nsHttpConnection::OnTransportStatus(nsITransport *trans,
                                    nsresult status,
                                    uint64_t progress,
                                    uint64_t progressMax)
{
    if (mTransaction)
        mTransaction->OnTransportStatus(trans, status, progress);
    return NS_OK;
}






NS_IMETHODIMP
nsHttpConnection::GetInterface(const nsIID &iid, void **result)
{
    
    
    
    

    
    
    

    NS_ASSERTION(PR_GetCurrentThread() != gSocketThread, "wrong thread");

    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    {
        MutexAutoLock lock(mCallbacksLock);
        callbacks = mCallbacks;
    }
    if (callbacks)
        return callbacks->GetInterface(iid, result);
    return NS_ERROR_NO_INTERFACE;
}
