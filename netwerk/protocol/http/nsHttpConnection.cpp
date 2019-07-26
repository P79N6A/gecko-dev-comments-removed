






#include "HttpLog.h"


#undef LOG
#define LOG(args) LOG5(args)
#undef LOG_ENABLED
#define LOG_ENABLED() LOG5_ENABLED()

#include "nsHttpConnection.h"
#include "nsHttpRequestHead.h"
#include "nsHttpResponseHead.h"
#include "nsHttpHandler.h"
#include "nsIOService.h"
#include "nsISocketTransport.h"
#include "nsSocketTransportService2.h"
#include "nsISSLSocketControl.h"
#include "sslt.h"
#include "nsStringStream.h"
#include "nsProxyRelease.h"
#include "nsSocketTransport2.h"
#include "nsPreloadedStream.h"
#include "ASpdySession.h"
#include "mozilla/Telemetry.h"
#include "nsISupportsPriority.h"
#include "nsHttpPipeline.h"
#include <algorithm>
#include "mozilla/ChaosMode.h"

#ifdef DEBUG

extern PRThread *gSocketThread;
#endif

namespace mozilla {
namespace net {





nsHttpConnection::nsHttpConnection()
    : mTransaction(nullptr)
    , mHttpHandler(gHttpHandler)
    , mCallbacksLock("nsHttpConnection::mCallbacksLock")
    , mConsiderReusedAfterInterval(0)
    , mConsiderReusedAfterEpoch(0)
    , mCurrentBytesRead(0)
    , mMaxBytesRead(0)
    , mTotalBytesRead(0)
    , mTotalBytesWritten(0)
    , mConnectedTransport(false)
    , mKeepAlive(true) 
    , mKeepAliveMask(true)
    , mDontReuse(false)
    , mSupportsPipelining(false) 
    , mIsReused(false)
    , mCompletedProxyConnect(false)
    , mLastTransactionExpectedNoContent(false)
    , mIdleMonitoring(false)
    , mProxyConnectInProgress(false)
    , mExperienced(false)
    , mHttp1xTransactionCount(0)
    , mRemainingConnectionUses(0xffffffff)
    , mClassification(nsAHttpTransaction::CLASS_GENERAL)
    , mNPNComplete(false)
    , mSetupSSLCalled(false)
    , mUsingSpdyVersion(0)
    , mPriority(nsISupportsPriority::PRIORITY_NORMAL)
    , mReportedSpdy(false)
    , mEverUsedSpdy(false)
    , mLastHttpResponseVersion(NS_HTTP_VERSION_1_1)
    , mTransactionCaps(0)
    , mResponseTimeoutEnabled(false)
    , mTCPKeepaliveConfig(kTCPKeepaliveDisabled)
{
    LOG(("Creating nsHttpConnection @%p\n", this));

    
    
    static const PRIntervalTime k5Sec = PR_SecondsToInterval(5);
    mIdleTimeout =
        (k5Sec < gHttpHandler->IdleTimeout()) ? k5Sec : gHttpHandler->IdleTimeout();
}

nsHttpConnection::~nsHttpConnection()
{
    LOG(("Destroying nsHttpConnection @%p\n", this));

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
                       bool connectedTransport,
                       nsIInterfaceRequestor *callbacks,
                       PRIntervalTime rtt)
{
    MOZ_ASSERT(transport && instream && outstream,
               "invalid socket information");
    LOG(("nsHttpConnection::Init [this=%p "
         "transport=%p instream=%p outstream=%p rtt=%d]\n",
         this, transport, instream, outstream,
         PR_IntervalToMilliseconds(rtt)));

    NS_ENSURE_ARG_POINTER(info);
    NS_ENSURE_TRUE(!mConnInfo, NS_ERROR_ALREADY_INITIALIZED);

    mConnectedTransport = connectedTransport;
    mConnInfo = info;
    mLastWriteTime = mLastReadTime = PR_IntervalNow();
    mSupportsPipelining =
        gHttpHandler->ConnMgr()->SupportsPipelining(mConnInfo);
    mRtt = rtt;
    mMaxHangTime = PR_SecondsToInterval(maxHangTime);

    mSocketTransport = transport;
    mSocketIn = instream;
    mSocketOut = outstream;
    nsresult rv = mSocketTransport->SetEventSink(this, nullptr);
    NS_ENSURE_SUCCESS(rv, rv);

    
    mCallbacks = new nsMainThreadPtrHolder<nsIInterfaceRequestor>(callbacks, false);
    rv = mSocketTransport->SetSecurityCallbacks(this);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

void
nsHttpConnection::StartSpdy(uint8_t spdyVersion)
{
    LOG(("nsHttpConnection::StartSpdy [this=%p]\n", this));

    MOZ_ASSERT(!mSpdySession);

    mUsingSpdyVersion = spdyVersion;
    mEverUsedSpdy = true;

    
    
    
    
    mIsReused = true;

    
    
    

    nsTArray<nsRefPtr<nsAHttpTransaction> > list;
    nsresult rv = mTransaction->TakeSubTransactions(list);

    if (rv == NS_ERROR_ALREADY_OPENED) {
        
        LOG(("TakeSubTranscations somehow called after "
             "nsAHttpTransaction began processing\n"));
        MOZ_ASSERT(false,
                   "TakeSubTranscations somehow called after "
                   "nsAHttpTransaction began processing");
        mTransaction->Close(NS_ERROR_ABORT);
        return;
    }

    if (NS_FAILED(rv) && rv != NS_ERROR_NOT_IMPLEMENTED) {
        
        LOG(("unexpected rv from nnsAHttpTransaction::TakeSubTransactions()"));
        MOZ_ASSERT(false,
                   "unexpected result from "
                   "nsAHttpTransaction::TakeSubTransactions()");
        mTransaction->Close(NS_ERROR_ABORT);
        return;
    }

    if (NS_FAILED(rv)) { 
        MOZ_ASSERT(list.IsEmpty(), "sub transaction list not empty");

        
        
        
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
                    MOZ_ASSERT(false, "SpdySession::AddStream failed");
                    LOG(("SpdySession::AddStream failed\n"));
                    mTransaction->Close(NS_ERROR_ABORT);
                    return;
                }
            }
        }
    }

    
    rv = DisableTCPKeepalives();
    if (NS_WARN_IF(NS_FAILED(rv))) {
        LOG(("nsHttpConnection::StartSpdy [%p] DisableTCPKeepalives failed "
             "rv[0x%x]", this, rv));
    }

    mSupportsPipelining = false; 
    mTransaction = mSpdySession;
    mIdleTimeout = gHttpHandler->SpdyTimeout();
}

bool
nsHttpConnection::EnsureNPNComplete()
{
    
    

    MOZ_ASSERT(mSocketTransport);
    if (!mSocketTransport) {
        
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

    LOG(("nsHttpConnection::EnsureNPNComplete %p [%s] negotiated to '%s'\n",
         this, mConnInfo->Host(), negotiatedNPN.get()));

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
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    LOG(("nsHttpConnection::Activate [this=%p trans=%x caps=%x]\n",
         this, trans, caps));

    if (!trans->IsNullTransaction())
        mExperienced = true;

    mTransactionCaps = caps;
    mPriority = pri;
    if (mTransaction && mUsingSpdyVersion)
        return AddTransaction(trans, pri);

    NS_ENSURE_ARG_POINTER(trans);
    NS_ENSURE_TRUE(!mTransaction, NS_ERROR_IN_PROGRESS);

    
    mLastWriteTime = mLastReadTime = PR_IntervalNow();

    
    
    
    if (!mConnectedTransport) {
        uint32_t count;
        mSocketOutCondition = NS_ERROR_FAILURE;
        if (mSocketOut) {
            mSocketOutCondition = mSocketOut->Write("", 0, &count);
        }
        if (NS_FAILED(mSocketOutCondition) &&
            mSocketOutCondition != NS_BASE_STREAM_WOULD_BLOCK) {
            LOG(("nsHttpConnection::Activate [this=%p] Bad Socket %x\n",
                 this, mSocketOutCondition));
            mSocketOut->AsyncWait(nullptr, 0, 0, nullptr);
            mTransaction = trans;
            CloseTransaction(mTransaction, mSocketOutCondition);
            return mSocketOutCondition;
        }
    }

    
    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    trans->GetSecurityCallbacks(getter_AddRefs(callbacks));
    SetSecurityCallbacks(callbacks);

    SetupSSL(caps);

    
    mTransaction = trans;

    MOZ_ASSERT(!mIdleMonitoring, "Activating a connection with an Idle Monitor");
    mIdleMonitoring = false;

    
    mKeepAliveMask = mKeepAlive = (caps & NS_HTTP_ALLOW_KEEPALIVE);

    
    
    nsresult rv = NS_OK;
    if (mConnInfo->UsingConnect() && !mCompletedProxyConnect) {
        rv = SetupProxyConnect();
        if (NS_FAILED(rv))
            goto failed_activation;
        mProxyConnectInProgress = true;
    }

    
    mCurrentBytesRead = 0;

    
    mInputOverflow = nullptr;

    mResponseTimeoutEnabled = mTransaction->ResponseTimeout() > 0 &&
                              mTransaction->ResponseTimeoutEnabled();

    rv = StartShortLivedTCPKeepalives();
    if (NS_WARN_IF(NS_FAILED(rv))) {
        LOG(("nsHttpConnection::Activate [%p] "
             "StartShortLivedTCPKeepalives failed rv[0x%x]",
             this, rv));
    }

    rv = OnOutputStreamReady(mSocketOut);

failed_activation:
    if (NS_FAILED(rv)) {
        mTransaction = nullptr;
    }

    return rv;
}

void
nsHttpConnection::SetupSSL(uint32_t caps)
{
    LOG(("nsHttpConnection::SetupSSL %p caps=0x%X\n", this, caps));

    if (mSetupSSLCalled) 
        return;
    mSetupSSLCalled = true;

    if (mNPNComplete)
        return;

    
    
    mNPNComplete = true;

    if (!mConnInfo->UsingSSL())
        return;

    LOG(("nsHttpConnection::SetupSSL Setting up "
         "Next Protocol Negotiation"));
    nsCOMPtr<nsISupports> securityInfo;
    nsresult rv =
        mSocketTransport->GetSecurityInfo(getter_AddRefs(securityInfo));
    if (NS_FAILED(rv))
        return;

    nsCOMPtr<nsISSLSocketControl> ssl = do_QueryInterface(securityInfo, &rv);
    if (NS_FAILED(rv))
        return;

    if (caps & NS_HTTP_ALLOW_RSA_FALSESTART) {
        LOG(("nsHttpConnection::SetupSSL %p "
             ">= RSA Key Exchange Expected\n", this));
        ssl->SetKEAExpected(ssl_kea_rsa);
    }

    nsTArray<nsCString> protocolArray;

    
    
    
    
    protocolArray.AppendElement(NS_LITERAL_CSTRING("http/1.1"));

    if (gHttpHandler->IsSpdyEnabled() &&
        !(caps & NS_HTTP_DISALLOW_SPDY)) {
        LOG(("nsHttpConnection::SetupSSL Allow SPDY NPN selection"));
        for (uint32_t index = 0; index < SpdyInformation::kCount; ++index) {
            if (gHttpHandler->SpdyInfo()->ProtocolEnabled(index))
                protocolArray.AppendElement(
                    gHttpHandler->SpdyInfo()->VersionString[index]);
        }
    }

    if (NS_SUCCEEDED(ssl->SetNPNList(protocolArray))) {
        LOG(("nsHttpConnection::Init Setting up SPDY Negotiation OK"));
        mNPNComplete = false;
    }
}

nsresult
nsHttpConnection::AddTransaction(nsAHttpTransaction *httpTransaction,
                                 int32_t priority)
{
    LOG(("nsHttpConnection::AddTransaction for SPDY"));

    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    MOZ_ASSERT(mSpdySession && mUsingSpdyVersion,
               "AddTransaction to live http connection without spdy");
    MOZ_ASSERT(mTransaction,
               "AddTransaction to idle http connection");

    if (!mSpdySession->AddStream(httpTransaction, priority)) {
        MOZ_ASSERT(false, "AddStream should never fail due to"
                   "RoomForMore() admission check");
        return NS_ERROR_FAILURE;
    }

    ResumeSend();

    return NS_OK;
}

void
nsHttpConnection::Close(nsresult reason)
{
    LOG(("nsHttpConnection::Close [this=%p reason=%x]\n", this, reason));

    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    
    if (mTCPKeepaliveTransitionTimer) {
        mTCPKeepaliveTransitionTimer->Cancel();
        mTCPKeepaliveTransitionTimer = nullptr;
    }

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
    LOG(("nsHttpConnection::ProxyStartSSL [this=%p]\n", this));
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

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
    if (!mSocketTransport || !mConnectedTransport)
        return false;

    
    
    SetupSSL(mTransactionCaps);

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

    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    NS_ENSURE_ARG_POINTER(trans);
    MOZ_ASSERT(responseHead, "No response head?");

    
    

    
    
    

    bool explicitKeepAlive = false;
    bool explicitClose = responseHead->HasHeaderValue(nsHttp::Connection, "close") ||
        responseHead->HasHeaderValue(nsHttp::Proxy_Connection, "close");
    if (!explicitClose)
        explicitKeepAlive = responseHead->HasHeaderValue(nsHttp::Connection, "keep-alive") ||
            responseHead->HasHeaderValue(nsHttp::Proxy_Connection, "keep-alive");

    
    uint16_t responseStatus = responseHead->Status();
    static const PRIntervalTime k1000ms  = PR_MillisecondsToInterval(1000);
    if (responseStatus == 408) {
        
        
        
        
        if (mIsReused && ((PR_IntervalNow() - mLastWriteTime) < k1000ms)) {
            Close(NS_ERROR_NET_RESET);
            *reset = true;
            return NS_OK;
        }

        
        
        
        explicitClose = true;
        explicitKeepAlive = false;
    }

    
    mSupportsPipelining = false;

    if ((responseHead->Version() < NS_HTTP_VERSION_1_1) ||
        (requestHead->Version() < NS_HTTP_VERSION_1_1)) {
        
        if (explicitKeepAlive)
            mKeepAlive = true;
        else
            mKeepAlive = false;

        
        gHttpHandler->ConnMgr()->PipelineFeedbackInfo(
            mConnInfo, nsHttpConnectionMgr::RedVersionTooLow, this, 0);
    }
    else {
        
        if (explicitClose) {
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
        const char *val = responseHead->PeekHeader(nsHttp::Keep_Alive);

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

    
    
    
    
    if (mProxyConnectStream) {
        MOZ_ASSERT(!mUsingSpdyVersion,
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
            
            MOZ_ASSERT(NS_SUCCEEDED(rv), "mSocketOut->AsyncWait failed");
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

    mLastHttpResponseVersion = responseHead->Version();

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

    
    if (mTCPKeepaliveConfig == kTCPKeepaliveShortLivedConfig) {
        if (mTCPKeepaliveTransitionTimer) {
            mTCPKeepaliveTransitionTimer->Cancel();
            mTCPKeepaliveTransitionTimer = nullptr;
        }
        nsresult rv = StartLongLivedTCPKeepalives();
        LOG(("nsHttpConnection::TakeTransport [%p] calling "
             "StartLongLivedTCPKeepalives", this));
        if (NS_WARN_IF(NS_FAILED(rv))) {
            LOG(("nsHttpConnection::TakeTransport [%p] "
                 "StartLongLivedTCPKeepalives failed rv[0x%x]", this, rv));
        }
    }

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

uint32_t
nsHttpConnection::ReadTimeoutTick(PRIntervalTime now)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    
    if (!mTransaction)
        return UINT32_MAX;

    
    if (mSpdySession) {
        return mSpdySession->ReadTimeoutTick(now);
    }

    uint32_t nextTickAfter = UINT32_MAX;
    
    if (mResponseTimeoutEnabled) {
        PRIntervalTime initialResponseDelta = now - mLastWriteTime;

        if (initialResponseDelta > mTransaction->ResponseTimeout()) {
            LOG(("canceling transaction: no response for %ums: timeout is %dms\n",
                 PR_IntervalToMilliseconds(initialResponseDelta),
                 PR_IntervalToMilliseconds(mTransaction->ResponseTimeout())));

            mResponseTimeoutEnabled = false;

            
            CloseTransaction(mTransaction, NS_ERROR_NET_TIMEOUT);
            return UINT32_MAX;
        }
        nextTickAfter = PR_IntervalToSeconds(mTransaction->ResponseTimeout()) -
                        PR_IntervalToSeconds(initialResponseDelta);
        nextTickAfter = std::max(nextTickAfter, 1U);
    }

    if (!gHttpHandler->GetPipelineRescheduleOnTimeout())
        return nextTickAfter;

    PRIntervalTime delta = now - mLastReadTime;

    
    
    
    
    
    
    
    

    uint32_t pipelineDepth = mTransaction->PipelineDepth();
    if (pipelineDepth > 1) {
        
        
        nextTickAfter = 1;
    }

    if (delta >= gHttpHandler->GetPipelineRescheduleTimeout() &&
        pipelineDepth > 1) {

        
        
        LOG(("cancelling pipeline due to a %ums stall - depth %d\n",
             PR_IntervalToMilliseconds(delta), pipelineDepth));

        nsHttpPipeline *pipeline = mTransaction->QueryPipeline();
        MOZ_ASSERT(pipeline, "pipelinedepth > 1 without pipeline");
        
        
        
        if (pipeline) {
            pipeline->CancelPipeline(NS_ERROR_NET_TIMEOUT);
            LOG(("Rescheduling the head of line blocked members of a pipeline "
                 "because reschedule-timeout idle interval exceeded"));
        }
    }

    if (delta < gHttpHandler->GetPipelineTimeout())
        return nextTickAfter;

    if (pipelineDepth <= 1 && !mTransaction->PipelinePosition())
        return nextTickAfter;

    
    
    
    
    

    LOG(("canceling transaction stalled for %ums on a pipeline "
         "of depth %d and scheduled originally at pos %d\n",
         PR_IntervalToMilliseconds(delta),
         pipelineDepth, mTransaction->PipelinePosition()));

    
    CloseTransaction(mTransaction, NS_ERROR_NET_TIMEOUT);
    return UINT32_MAX;
}

void
nsHttpConnection::UpdateTCPKeepalive(nsITimer *aTimer, void *aClosure)
{
    MOZ_ASSERT(aTimer);
    MOZ_ASSERT(aClosure);

    nsHttpConnection *self = static_cast<nsHttpConnection*>(aClosure);

    if (NS_WARN_IF(self->mUsingSpdyVersion)) {
        return;
    }

    
    if (self->mIdleMonitoring) {
        return;
    }

    nsresult rv = self->StartLongLivedTCPKeepalives();
    if (NS_WARN_IF(NS_FAILED(rv))) {
        LOG(("nsHttpConnection::UpdateTCPKeepalive [%p] "
             "StartLongLivedTCPKeepalives failed rv[0x%x]",
             self, rv));
    }
}

void
nsHttpConnection::GetSecurityInfo(nsISupports **secinfo)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    if (mSocketTransport) {
        if (NS_FAILED(mSocketTransport->GetSecurityInfo(secinfo)))
            *secinfo = nullptr;
    }
}

void
nsHttpConnection::SetSecurityCallbacks(nsIInterfaceRequestor* aCallbacks)
{
    MutexAutoLock lock(mCallbacksLock);
    
    
    
    
    mCallbacks = new nsMainThreadPtrHolder<nsIInterfaceRequestor>(aCallbacks, false);
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

    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    if (mSocketOut)
        return mSocketOut->AsyncWait(this, 0, 0, nullptr);

    NS_NOTREACHED("no socket output stream");
    return NS_ERROR_UNEXPECTED;
}

nsresult
nsHttpConnection::ResumeRecv()
{
    LOG(("nsHttpConnection::ResumeRecv [this=%p]\n", this));

    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    
    
    
    
    
    
    mLastReadTime = PR_IntervalNow();

    if (mSocketIn)
        return mSocketIn->AsyncWait(this, 0, 0, nullptr);

    NS_NOTREACHED("no socket input stream");
    return NS_ERROR_UNEXPECTED;
}


class nsHttpConnectionForceRecv : public nsRunnable
{
public:
    nsHttpConnectionForceRecv(nsHttpConnection *aConn)
        : mConn(aConn) {}

    NS_IMETHOD Run()
    {
        MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

        if (!mConn->mSocketIn)
            return NS_OK;
        return mConn->OnInputStreamReady(mConn->mSocketIn);
    }
private:
    nsRefPtr<nsHttpConnection> mConn;
};


nsresult
nsHttpConnection::ForceRecv()
{
    LOG(("nsHttpConnection::ForceRecv [this=%p]\n", this));
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    return NS_DispatchToCurrentThread(new nsHttpConnectionForceRecv(this));
}

void
nsHttpConnection::BeginIdleMonitoring()
{
    LOG(("nsHttpConnection::BeginIdleMonitoring [this=%p]\n", this));
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    MOZ_ASSERT(!mTransaction, "BeginIdleMonitoring() while active");
    MOZ_ASSERT(!mUsingSpdyVersion, "Idle monitoring of spdy not allowed");

    LOG(("Entering Idle Monitoring Mode [this=%p]", this));
    mIdleMonitoring = true;
    if (mSocketIn)
        mSocketIn->AsyncWait(this, 0, 0, nullptr);
}

void
nsHttpConnection::EndIdleMonitoring()
{
    LOG(("nsHttpConnection::EndIdleMonitoring [this=%p]\n", this));
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    MOZ_ASSERT(!mTransaction, "EndIdleMonitoring() while active");

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
    LOG(("nsHttpConnection::CloseTransaction[this=%p trans=%p reason=%x]\n",
        this, trans, reason));

    MOZ_ASSERT(trans == mTransaction, "wrong transaction");
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

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
        mLastWriteTime = PR_IntervalNow();
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
                gHttpHandler->ConnMgr()->ReportSpdyConnection(this, mEverUsedSpdy);
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
            rv = NS_OK;

            if (mTransaction) { 
                
                
                
                
                
                
                mTransaction->OnTransportStatus(mSocketTransport,
                                                NS_NET_STATUS_WAITING_FOR,
                                                0);

                rv = ResumeRecv(); 
            }
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

    if (ChaosMode::isActive() && ChaosMode::randomUint32LessThan(2)) {
        
        count = ChaosMode::randomUint32LessThan(count) + 1;
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
    LOG(("nsHttpConnection::OnSocketReadable [this=%p]\n", this));

    PRIntervalTime now = PR_IntervalNow();
    PRIntervalTime delta = now - mLastReadTime;

    
    mResponseTimeoutEnabled = false;

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
            MOZ_ASSERT(pipeline, "pipelinedepth > 1 without pipeline");
            
            
            
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

    LOG(("nsHttpConnection::SetupProxyConnect [this=%p]\n", this));

    NS_ENSURE_TRUE(!mProxyConnectStream, NS_ERROR_ALREADY_INITIALIZED);
    MOZ_ASSERT(!mUsingSpdyVersion,
               "SPDY NPN Complete while using proxy connect stream");

    nsAutoCString buf;
    nsresult rv = nsHttpHandler::GenerateHostPort(
            nsDependentCString(mConnInfo->Host()), mConnInfo->Port(), buf);
    if (NS_FAILED(rv))
        return rv;

    
    nsHttpRequestHead request;
    request.SetMethod(NS_LITERAL_CSTRING("CONNECT"));
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

nsresult
nsHttpConnection::StartShortLivedTCPKeepalives()
{
    if (mUsingSpdyVersion) {
        return NS_OK;
    }
    MOZ_ASSERT(mSocketTransport);
    if (!mSocketTransport) {
        return NS_ERROR_NOT_INITIALIZED;
    }

    nsresult rv = NS_OK;
    int32_t idleTimeS = -1;
    int32_t retryIntervalS = -1;
    if (gHttpHandler->TCPKeepaliveEnabledForShortLivedConns()) {
        
        idleTimeS = gHttpHandler->GetTCPKeepaliveShortLivedIdleTime();
        LOG(("nsHttpConnection::StartShortLivedTCPKeepalives[%p] "
             "idle time[%ds].", this, idleTimeS));

        retryIntervalS =
            std::max<int32_t>((int32_t)PR_IntervalToSeconds(mRtt), 1);
        rv = mSocketTransport->SetKeepaliveVals(idleTimeS, retryIntervalS);
        if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
        }
        rv = mSocketTransport->SetKeepaliveEnabled(true);
        mTCPKeepaliveConfig = kTCPKeepaliveShortLivedConfig;
    } else {
        rv = mSocketTransport->SetKeepaliveEnabled(false);
        mTCPKeepaliveConfig = kTCPKeepaliveDisabled;
    }
    if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
    }

    
    if(!mTCPKeepaliveTransitionTimer) {
        mTCPKeepaliveTransitionTimer =
            do_CreateInstance("@mozilla.org/timer;1");
    }

    if (mTCPKeepaliveTransitionTimer) {
        int32_t time = gHttpHandler->GetTCPKeepaliveShortLivedTime();

        
        
        if (gHttpHandler->TCPKeepaliveEnabledForShortLivedConns()) {
            if (NS_WARN_IF(!gSocketTransportService)) {
                return NS_ERROR_NOT_INITIALIZED;
            }
            int32_t probeCount = -1;
            rv = gSocketTransportService->GetKeepaliveProbeCount(&probeCount);
            if (NS_WARN_IF(NS_FAILED(rv))) {
                return rv;
            }
            if (NS_WARN_IF(probeCount <= 0)) {
                return NS_ERROR_UNEXPECTED;
            }
            
            time += ((probeCount) * retryIntervalS) - (time % idleTimeS) + 2;
        }
        mTCPKeepaliveTransitionTimer->InitWithFuncCallback(
                                          nsHttpConnection::UpdateTCPKeepalive,
                                          this,
                                          (uint32_t)time*1000,
                                          nsITimer::TYPE_ONE_SHOT);
    } else {
        NS_WARNING("nsHttpConnection::StartShortLivedTCPKeepalives failed to "
                   "create timer.");
    }

    return NS_OK;
}

nsresult
nsHttpConnection::StartLongLivedTCPKeepalives()
{
    MOZ_ASSERT(!mUsingSpdyVersion, "Don't use TCP Keepalive with SPDY!");
    if (NS_WARN_IF(mUsingSpdyVersion)) {
        return NS_OK;
    }
    MOZ_ASSERT(mSocketTransport);
    if (!mSocketTransport) {
        return NS_ERROR_NOT_INITIALIZED;
    }

    nsresult rv = NS_OK;
    if (gHttpHandler->TCPKeepaliveEnabledForLongLivedConns()) {
        
        int32_t idleTimeS = gHttpHandler->GetTCPKeepaliveLongLivedIdleTime();
        LOG(("nsHttpConnection::StartLongLivedTCPKeepalives[%p] idle time[%ds]",
             this, idleTimeS));

        int32_t retryIntervalS =
            std::max<int32_t>((int32_t)PR_IntervalToSeconds(mRtt), 1);
        rv = mSocketTransport->SetKeepaliveVals(idleTimeS, retryIntervalS);
        if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
        }

        
        if (mTCPKeepaliveConfig == kTCPKeepaliveDisabled) {
            rv = mSocketTransport->SetKeepaliveEnabled(true);
            if (NS_WARN_IF(NS_FAILED(rv))) {
                return rv;
            }
        }
        mTCPKeepaliveConfig = kTCPKeepaliveLongLivedConfig;
    } else {
        rv = mSocketTransport->SetKeepaliveEnabled(false);
        mTCPKeepaliveConfig = kTCPKeepaliveDisabled;
    }

    if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
    }
    return NS_OK;
}

nsresult
nsHttpConnection::DisableTCPKeepalives()
{
    MOZ_ASSERT(mSocketTransport);
    if (!mSocketTransport) {
        return NS_ERROR_NOT_INITIALIZED;
    }
    LOG(("nsHttpConnection::DisableTCPKeepalives [%p]", this));
    if (mTCPKeepaliveConfig != kTCPKeepaliveDisabled) {
        nsresult rv = mSocketTransport->SetKeepaliveEnabled(false);
        if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
        }
        mTCPKeepaliveConfig = kTCPKeepaliveDisabled;
    }
    if (mTCPKeepaliveTransitionTimer) {
        mTCPKeepaliveTransitionTimer->Cancel();
        mTCPKeepaliveTransitionTimer = nullptr;
    }
    return NS_OK;
}





NS_IMPL_ISUPPORTS(nsHttpConnection,
                  nsIInputStreamCallback,
                  nsIOutputStreamCallback,
                  nsITransportEventSink,
                  nsIInterfaceRequestor)






NS_IMETHODIMP
nsHttpConnection::OnInputStreamReady(nsIAsyncInputStream *in)
{
    MOZ_ASSERT(in == mSocketIn, "unexpected stream");
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);

    if (mIdleMonitoring) {
        MOZ_ASSERT(!mTransaction, "Idle Input Event While Active");

        
        
        
        
        

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
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread);
    MOZ_ASSERT(out == mSocketOut, "unexpected socket");

    
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
    
    
    
    

    
    
    

    MOZ_ASSERT(PR_GetCurrentThread() != gSocketThread);

    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    {
        MutexAutoLock lock(mCallbacksLock);
        callbacks = mCallbacks;
    }
    if (callbacks)
        return callbacks->GetInterface(iid, result);
    return NS_ERROR_NO_INTERFACE;
}

} 
} 
