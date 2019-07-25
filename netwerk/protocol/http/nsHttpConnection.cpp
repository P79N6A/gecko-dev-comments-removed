






































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
#include "nsAutoLock.h"
#include "nsProxyRelease.h"
#include "prmem.h"

#ifdef DEBUG

extern PRThread *gSocketThread;
#endif

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);





static PRUint32 sCreateTransport1 = 0;
static PRUint32 sCreateTransport2 = 0;
static PRUint32 sSuccessTransport1 = 0;
static PRUint32 sSuccessTransport2 = 0;
static PRUint32 sUnNecessaryTransport2 = 0;
static PRUint32 sWastedReuseCount = 0;





nsHttpConnection::nsHttpConnection()
    : mTransaction(nsnull)
    , mConnInfo(nsnull)
    , mLock(nsnull)
    , mLastReadTime(0)
    , mIdleTimeout(0)
    , mKeepAlive(PR_TRUE) 
    , mKeepAliveMask(PR_TRUE)
    , mSupportsPipelining(PR_FALSE) 
    , mIsReused(PR_FALSE)
    , mCompletedSSLConnect(PR_FALSE)
    , mActivationCount(0)
{
    LOG(("Creating nsHttpConnection @%x\n", this));

    
    nsHttpHandler *handler = gHttpHandler;
    NS_ADDREF(handler);
}

nsHttpConnection::~nsHttpConnection()
{
    LOG(("Destroying nsHttpConnection @%x\n", this));

    CancelSynTimer();
    if (mBackupConnection) {
        gHttpHandler->ReclaimConnection(mBackupConnection);
        mBackupConnection = nsnull;
    }

    ReleaseCallbacks();
    NS_IF_RELEASE(mConnInfo);

    if (mLock) {
        PR_DestroyLock(mLock);
        mLock = nsnull;
    }

    
    nsHttpHandler *handler = gHttpHandler;
    NS_RELEASE(handler);
}

void
nsHttpConnection::ReleaseCallbacks()
{
    if (mCallbacks) {
        nsIInterfaceRequestor *cbs = nsnull;
        mCallbacks.swap(cbs);
        NS_ProxyRelease(mCallbackTarget, cbs);
    }
}

void
nsHttpConnection::CancelSynTimer()
{
    if (mIdleSynTimer) {
        mIdleSynTimer->Cancel();
        mIdleSynTimer = nsnull;
    }
}

nsresult
nsHttpConnection::Init(nsHttpConnectionInfo *info, PRUint16 maxHangTime)
{
    LOG(("nsHttpConnection::Init [this=%x]\n", this));

    NS_ENSURE_ARG_POINTER(info);
    NS_ENSURE_TRUE(!mConnInfo, NS_ERROR_ALREADY_INITIALIZED);

    mLock = PR_NewLock();
    if (!mLock)
        return NS_ERROR_OUT_OF_MEMORY;

    mConnInfo = info;
    NS_ADDREF(mConnInfo);

    mMaxHangTime = maxHangTime;
    mLastReadTime = NowInSeconds();
    return NS_OK;
}

void
nsHttpConnection::IdleSynTimeout(nsITimer *timer, void *closure)
{
    
    
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    nsHttpConnection *self = (nsHttpConnection *)closure;
    NS_ABORT_IF_FALSE(timer == self->mIdleSynTimer, "wrong timer");
    self->mIdleSynTimer = nsnull;

    if (!self->mSocketTransport) {
        NS_ABORT_IF_FALSE(self->mSocketTransport1 && !self->mSocketTransport2,
                          "establishing backup tranport");

        LOG(("SocketTransport hit idle timer - starting backup socket"));

        
        
        if (!self->mTransaction)
            return;

        gHttpHandler->ConnMgr()->GetConnection(self->mConnInfo,
                                               self->mSocketCaps,
                                               getter_AddRefs(
                                                   self->mBackupConnection));
        if (!self->mBackupConnection)
            return;
        nsresult rv =
            self->CreateTransport(self->mSocketCaps,
                                  getter_AddRefs(self->mSocketTransport2),
                                  getter_AddRefs(self->mSocketIn2),
                                  getter_AddRefs(self->mSocketOut2));
        if (NS_SUCCEEDED(rv)) {
            sCreateTransport2++;
            self->mTransaction->
                GetSecurityCallbacks(
                    getter_AddRefs(self->mCallbacks),
                    getter_AddRefs(self->mCallbackTarget));
            self->mSocketOut2->AsyncWait(self, 0, 0, nsnull);
        }
    }

    return;
}


nsresult
nsHttpConnection::Activate(nsAHttpTransaction *trans, PRUint8 caps)
{
    nsresult rv;

    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    LOG(("nsHttpConnection::Activate [this=%x trans=%x caps=%x]\n",
         this, trans, caps));

    NS_ENSURE_ARG_POINTER(trans);
    NS_ENSURE_TRUE(!mTransaction, NS_ERROR_IN_PROGRESS);

    
    mTransaction = trans;
    mActivationCount++;
    ReleaseCallbacks();

    
    mKeepAliveMask = mKeepAlive = (caps & NS_HTTP_ALLOW_KEEPALIVE);

    
    if (mConnInfo->UsingSSL() && mConnInfo->UsingHttpProxy() && !mCompletedSSLConnect) {
        rv = SetupSSLProxyConnect();
        if (NS_FAILED(rv))
            goto failed_activation;
    }

    
    if (!mSocketTransport) {
        rv = CreateTransport(caps);
    }
    else {
        NS_ABORT_IF_FALSE(mSocketOut && mSocketIn,
                          "Socket Transport and SocketOut mismatch");
        
        
        
        
        
        if (mActivationCount == 1) {
            sWastedReuseCount++;
            rv = mSocketTransport->SetEventSink(this, nsnull);
            NS_ENSURE_SUCCESS(rv, rv);
            rv = mSocketTransport->SetSecurityCallbacks(this);
            NS_ENSURE_SUCCESS(rv, rv);
        }
        rv = mSocketOut->AsyncWait(this, 0, 0, nsnull);
    }
    
failed_activation:
    if (NS_FAILED(rv)) {
        mTransaction = nsnull;
        CancelSynTimer();
    }

    return rv;
}

void
nsHttpConnection::Close(nsresult reason)
{
    LOG(("nsHttpConnection::Close [this=%x reason=%x]\n", this, reason));

    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (NS_FAILED(reason)) {
        if (mSocketTransport) {
            mSocketTransport->SetSecurityCallbacks(nsnull);
            mSocketTransport->SetEventSink(nsnull, nsnull);
            mSocketTransport->Close(reason);
        }
        
        if (mSocketTransport1) {
            mSocketTransport1->SetSecurityCallbacks(nsnull);
            mSocketTransport1->SetEventSink(nsnull, nsnull);
            mSocketTransport1->Close(reason);
        }
        
        if (mSocketTransport2) {
            mSocketTransport2->SetSecurityCallbacks(nsnull);
            mSocketTransport2->SetEventSink(nsnull, nsnull);
            mSocketTransport2->Close(reason);
        }
        mKeepAlive = PR_FALSE;
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

PRBool
nsHttpConnection::CanReuse()
{
    return IsKeepAlive() && (NowInSeconds() - mLastReadTime < mIdleTimeout)
                         && IsAlive();
}

PRUint32 nsHttpConnection::TimeToLive()
{
    PRInt32 tmp = mIdleTimeout - (NowInSeconds() - mLastReadTime);
    if (0 > tmp)
        tmp = 0;

    return tmp;
}

PRBool
nsHttpConnection::IsAlive()
{
    if (!mSocketTransport)
        return PR_FALSE;

    PRBool alive;
    nsresult rv = mSocketTransport->IsAlive(&alive);
    if (NS_FAILED(rv))
        alive = PR_FALSE;


#ifdef TEST_RESTART_LOGIC
    if (!alive) {
        LOG(("pretending socket is still alive to test restart logic\n"));
        alive = PR_TRUE;
    }
#endif

    return alive;
}

PRBool
nsHttpConnection::SupportsPipelining(nsHttpResponseHead *responseHead)
{
    
    

    
    if (mConnInfo->UsingHttpProxy() && !mConnInfo->UsingSSL()) {
        
        return PR_TRUE;
    }

    

    
    const char *val = responseHead->PeekHeader(nsHttp::Server);
    if (!val)
        return PR_FALSE; 

    
    
    

    static const char *bad_servers[26][5] = {
        { nsnull }, { nsnull }, { nsnull }, { nsnull },                 
        { "EFAServer/", nsnull },                                       
        { nsnull }, { nsnull }, { nsnull }, { nsnull },                 
        { nsnull }, { nsnull }, { nsnull },                             
        { "Microsoft-IIS/4.", "Microsoft-IIS/5.", nsnull },             
        { "Netscape-Enterprise/3.", "Netscape-Enterprise/4.", 
          "Netscape-Enterprise/5.", "Netscape-Enterprise/6.", nsnull }, 
        { nsnull }, { nsnull }, { nsnull }, { nsnull },                 
        { nsnull }, { nsnull }, { nsnull }, { nsnull },                 
        { "WebLogic 3.", "WebLogic 4.","WebLogic 5.", "WebLogic 6.", nsnull }, 
        { nsnull }, { nsnull }, { nsnull }                              
    };  

    int index = val[0] - 'A'; 
    if ((index >= 0) && (index <= 25))
    {
        for (int i = 0; bad_servers[index][i] != nsnull; i++) {
            if (!PL_strncmp (val, bad_servers[index][i], strlen (bad_servers[index][i]))) {
                LOG(("looks like this server does not support pipelining"));
                return PR_FALSE;
            }
        }
    }

    
    return PR_TRUE;
}





nsresult
nsHttpConnection::OnHeadersAvailable(nsAHttpTransaction *trans,
                                     nsHttpRequestHead *requestHead,
                                     nsHttpResponseHead *responseHead,
                                     PRBool *reset)
{
    LOG(("nsHttpConnection::OnHeadersAvailable [this=%p trans=%p response-head=%p]\n",
        this, trans, responseHead));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ENSURE_ARG_POINTER(trans);
    NS_ASSERTION(responseHead, "No response head?");

    
    
    
    
    if (responseHead->Status() == 408) {
        Close(NS_ERROR_NET_RESET);
        *reset = PR_TRUE;
        return NS_OK;
    }

    
    

    
    
    const char *val = responseHead->PeekHeader(nsHttp::Connection);
    if (!val)
        val = responseHead->PeekHeader(nsHttp::Proxy_Connection);

    
    mSupportsPipelining = PR_FALSE;

    
    
    if (!mSSLProxyConnectStream) {
        if ((responseHead->Version() > NS_HTTP_VERSION_0_9) &&
            (requestHead->Version() > NS_HTTP_VERSION_0_9))
        {
            mConnInfo->DisallowHttp09();
        }
    }

    if ((responseHead->Version() < NS_HTTP_VERSION_1_1) ||
        (requestHead->Version() < NS_HTTP_VERSION_1_1)) {
        
        if (val && !PL_strcasecmp(val, "keep-alive"))
            mKeepAlive = PR_TRUE;
        else
            mKeepAlive = PR_FALSE;
    }
    else {
        
        if (val && !PL_strcasecmp(val, "close")) 
            mKeepAlive = PR_FALSE;
        else {
            mKeepAlive = PR_TRUE;

            
            
            
            
            
            if (!mSSLProxyConnectStream)
              mSupportsPipelining = SupportsPipelining(responseHead);
        }
    }
    mKeepAliveMask = mKeepAlive;

    
    
    
    
    
    
    
    if (mKeepAlive) {
        val = responseHead->PeekHeader(nsHttp::Keep_Alive);

        const char *cp = PL_strcasestr(val, "timeout=");
        if (cp)
            mIdleTimeout = (PRUint32) atoi(cp + 8);
        else
            mIdleTimeout = gHttpHandler->IdleTimeout();
        
        LOG(("Connection can be reused [this=%x idle-timeout=%u]\n", this, mIdleTimeout));
    }

    
    
    
    
    if (mSSLProxyConnectStream) {
        mSSLProxyConnectStream = 0;
        if (responseHead->Status() == 200) {
            LOG(("SSL proxy CONNECT succeeded!\n"));
            *reset = PR_TRUE;
            nsresult rv = ProxyStartSSL();
            if (NS_FAILED(rv)) 
                LOG(("ProxyStartSSL failed [rv=%x]\n", rv));
            mCompletedSSLConnect = PR_TRUE;
            rv = mSocketOut->AsyncWait(this, 0, 0, nsnull);
            
            NS_ASSERTION(NS_SUCCEEDED(rv), "mSocketOut->AsyncWait failed");
        }
        else {
            LOG(("SSL proxy CONNECT failed!\n"));
            
            
            
            nsHttpTransaction *trans =
                     static_cast<nsHttpTransaction *>(mTransaction.get());
            trans->SetSSLConnectFailed();
        }
    }

    return NS_OK;
}

void
nsHttpConnection::GetSecurityInfo(nsISupports **secinfo)
{
    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (mSocketTransport) {
        if (NS_FAILED(mSocketTransport->GetSecurityInfo(secinfo)))
            *secinfo = nsnull;
    }
}

nsresult
nsHttpConnection::ResumeSend()
{
    LOG(("nsHttpConnection::ResumeSend [this=%p]\n", this));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (mSocketOut)
        return mSocketOut->AsyncWait(this, 0, 0, nsnull);

    NS_NOTREACHED("no socket output stream");
    return NS_ERROR_UNEXPECTED;
}

nsresult
nsHttpConnection::ResumeRecv()
{
    LOG(("nsHttpConnection::ResumeRecv [this=%p]\n", this));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (mSocketIn)
        return mSocketIn->AsyncWait(this, 0, 0, nsnull);

    NS_NOTREACHED("no socket input stream");
    return NS_ERROR_UNEXPECTED;
}





nsresult
nsHttpConnection::CreateTransport(PRUint8 caps)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    NS_ABORT_IF_FALSE(!mSocketTransport, "unexpected");

    nsresult rv;
    mSocketCaps = caps;
    sCreateTransport1++;
    
    PRUint16 timeout = gHttpHandler->GetIdleSynTimeout();
    if (timeout) {

        
        
        
        
        
        
        

        mIdleSynTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
        if (NS_SUCCEEDED(rv))
            mIdleSynTimer->InitWithFuncCallback(IdleSynTimeout, this,
                                                timeout,
                                                nsITimer::TYPE_ONE_SHOT);
    }
    
    rv = CreateTransport(mSocketCaps,
                         getter_AddRefs(mSocketTransport1),
                         getter_AddRefs(mSocketIn1),
                         getter_AddRefs(mSocketOut1));
    if (NS_FAILED(rv))
        return rv;

    
    return mSocketOut1->AsyncWait(this, 0, 0, nsnull);
}

nsresult
nsHttpConnection::CreateTransport(PRUint8 caps,
                                  nsISocketTransport **sock,
                                  nsIAsyncInputStream **instream,
                                  nsIAsyncOutputStream **outstream)
{
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    nsresult rv;

    nsCOMPtr<nsISocketTransportService> sts =
            do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    
    const char* types[1];

    if (mConnInfo->UsingSSL())
        types[0] = "ssl";
    else
        types[0] = gHttpHandler->DefaultSocketType();

    nsCOMPtr<nsISocketTransport> strans;
    PRUint32 typeCount = (types[0] != nsnull);

    rv = sts->CreateTransport(types, typeCount,
                              nsDependentCString(mConnInfo->Host()),
                              mConnInfo->Port(),
                              mConnInfo->ProxyInfo(),
                              getter_AddRefs(strans));
    if (NS_FAILED(rv)) return rv;

    PRUint32 tmpFlags = 0;
    if (caps & NS_HTTP_REFRESH_DNS)
        tmpFlags = nsISocketTransport::BYPASS_CACHE;
    
    if (caps & NS_HTTP_LOAD_ANONYMOUS)
        tmpFlags |= nsISocketTransport::ANONYMOUS_CONNECT;
    
    strans->SetConnectionFlags(tmpFlags); 

    strans->SetQoSBits(gHttpHandler->GetQoSBits());

    
    
    rv = strans->SetEventSink(this, nsnull);
    if (NS_FAILED(rv)) return rv;
    rv = strans->SetSecurityCallbacks(this);
    if (NS_FAILED(rv)) return rv;

    
    nsCOMPtr<nsIOutputStream> sout;
    rv = strans->OpenOutputStream(nsITransport::OPEN_UNBUFFERED, 0, 0,
                                  getter_AddRefs(sout));
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIInputStream> sin;
    rv = strans->OpenInputStream(nsITransport::OPEN_UNBUFFERED, 0, 0,
                                 getter_AddRefs(sin));
    if (NS_FAILED(rv)) return rv;

    strans.forget(sock);
    CallQueryInterface(sin, instream);
    CallQueryInterface(sout, outstream);

    return NS_OK;
}

nsresult
nsHttpConnection::AssignTransport(nsISocketTransport *sock,
                                  nsIAsyncOutputStream *outs,
                                  nsIAsyncInputStream *ins)
{
    mSocketTransport = sock;
    mSocketOut = outs;
    mSocketIn = ins;
    return NS_OK;
}

void
nsHttpConnection::CloseTransaction(nsAHttpTransaction *trans, nsresult reason)
{
    LOG(("nsHttpConnection::CloseTransaction[this=%x trans=%x reason=%x]\n",
        this, trans, reason));

    NS_ASSERTION(trans == mTransaction, "wrong transaction");
    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    
    if (reason == NS_BASE_STREAM_CLOSED)
        reason = NS_OK;

    mTransaction->Close(reason);
    mTransaction = nsnull;

    if (NS_FAILED(reason))
        Close(reason);

    
    
    mIsReused = PR_TRUE;
}

NS_METHOD
nsHttpConnection::ReadFromStream(nsIInputStream *input,
                                 void *closure,
                                 const char *buf,
                                 PRUint32 offset,
                                 PRUint32 count,
                                 PRUint32 *countRead)
{
    
    nsHttpConnection *conn = (nsHttpConnection *) closure;
    return conn->OnReadSegment(buf, count, countRead);
}

nsresult
nsHttpConnection::OnReadSegment(const char *buf,
                                PRUint32 count,
                                PRUint32 *countRead)
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
    else
        mSocketOutCondition = NS_OK; 

    return mSocketOutCondition;
}

nsresult
nsHttpConnection::OnSocketWritable()
{
    LOG(("nsHttpConnection::OnSocketWritable [this=%x]\n", this));

    nsresult rv;
    PRUint32 n;
    PRBool again = PR_TRUE;

    do {
        
        
        
        
        
        
        
        if (mSSLProxyConnectStream) {
            LOG(("  writing CONNECT request stream\n"));
            rv = mSSLProxyConnectStream->ReadSegments(ReadFromStream, this,
                                                      nsIOService::gDefaultSegmentSize,
                                                      &n);
        }
        else {
            LOG(("  writing transaction request stream\n"));
            rv = mTransaction->ReadSegments(this, nsIOService::gDefaultSegmentSize, &n);
        }

        LOG(("  ReadSegments returned [rv=%x read=%u sock-cond=%x]\n",
            rv, n, mSocketOutCondition));

        
        if (rv == NS_BASE_STREAM_CLOSED) {
            rv = NS_OK;
            n = 0;
        }

        if (NS_FAILED(rv)) {
            
            
            if (rv == NS_BASE_STREAM_WOULD_BLOCK)
                rv = NS_OK;
            again = PR_FALSE;
        }
        else if (NS_FAILED(mSocketOutCondition)) {
            if (mSocketOutCondition == NS_BASE_STREAM_WOULD_BLOCK)
                rv = mSocketOut->AsyncWait(this, 0, 0, nsnull); 
            else
                rv = mSocketOutCondition;
            again = PR_FALSE;
        }
        else if (n == 0) {
            
            
            
            
            
            
            mTransaction->OnTransportStatus(nsISocketTransport::STATUS_WAITING_FOR,
                                            LL_ZERO);

            rv = mSocketIn->AsyncWait(this, 0, 0, nsnull); 
            again = PR_FALSE;
        }
        
    } while (again);

    return rv;
}

nsresult
nsHttpConnection::OnWriteSegment(char *buf,
                                 PRUint32 count,
                                 PRUint32 *countWritten)
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

    PRUint32 now = NowInSeconds();

    if (mKeepAliveMask && (now - mLastReadTime >= PRUint32(mMaxHangTime))) {
        LOG(("max hang time exceeded!\n"));
        
        
        mKeepAliveMask = PR_FALSE;
        gHttpHandler->ProcessPendingQ(mConnInfo);
    }
    mLastReadTime = now;

    nsresult rv;
    PRUint32 n;
    PRBool again = PR_TRUE;

    do {
        rv = mTransaction->WriteSegments(this, nsIOService::gDefaultSegmentSize, &n);
        if (NS_FAILED(rv)) {
            
            
            if (rv == NS_BASE_STREAM_WOULD_BLOCK)
                rv = NS_OK;
            again = PR_FALSE;
        }
        else if (NS_FAILED(mSocketInCondition)) {
            
            if (mSocketInCondition == NS_BASE_STREAM_WOULD_BLOCK)
                rv = mSocketIn->AsyncWait(this, 0, 0, nsnull);
            else
                rv = mSocketInCondition;
            again = PR_FALSE;
        }
        
    } while (again);

    return rv;
}

nsresult
nsHttpConnection::SetupSSLProxyConnect()
{
    const char *val;

    LOG(("nsHttpConnection::SetupSSLProxyConnect [this=%x]\n", this));

    NS_ENSURE_TRUE(!mSSLProxyConnectStream, NS_ERROR_ALREADY_INITIALIZED);

    nsCAutoString buf;
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

    
    
    nsHttpTransaction *trans =
        static_cast<nsHttpTransaction *>(mTransaction.get());
    
    val = trans->RequestHead()->PeekHeader(nsHttp::Host);
    if (val) {
        
        
        request.SetHeader(nsHttp::Host, nsDependentCString(val));
    }

    val = trans->RequestHead()->PeekHeader(nsHttp::Proxy_Authorization);
    if (val) {
        
        
        request.SetHeader(nsHttp::Proxy_Authorization, nsDependentCString(val));
    }

    buf.Truncate();
    request.Flatten(buf, PR_FALSE);
    buf.AppendLiteral("\r\n");

    return NS_NewCStringInputStream(getter_AddRefs(mSSLProxyConnectStream), buf);
}

void
nsHttpConnection::ReleaseBackupTransport(nsISocketTransport *sock,
                                         nsIAsyncOutputStream *outs,
                                         nsIAsyncInputStream *ins)
{
    
    
    NS_ABORT_IF_FALSE(sock && outs && ins, "release Backup precond");
    mBackupConnection->mIdleTimeout = NS_MIN((PRUint16) 5,
                                             gHttpHandler->IdleTimeout());
    mBackupConnection->mIsReused = PR_TRUE;
    nsresult rv = mBackupConnection->AssignTransport(sock, outs, ins);
    if (NS_SUCCEEDED(rv))
        rv = gHttpHandler->ReclaimConnection(mBackupConnection);
    if (NS_FAILED(rv))
        NS_WARNING("Backup nsHttpConnection could not be reclaimed");
    mBackupConnection = nsnull;
}

void
nsHttpConnection::SelectPrimaryTransport(nsIAsyncOutputStream *out)
{
    LOG(("nsHttpConnection::SelectPrimaryTransport(out=%p), mSocketOut1=%p, mSocketOut2=%p, mSocketOut=%p",
         out, mSocketOut1.get(), mSocketOut2.get(), mSocketOut.get()));

    if (!mSocketOut) {
        

        CancelSynTimer();

        if (out == mSocketOut1) {
            sSuccessTransport1++;
            mSocketTransport.swap(mSocketTransport1);
            mSocketOut.swap(mSocketOut1);
            mSocketIn.swap(mSocketIn1);

            if (mSocketTransport2)
                sUnNecessaryTransport2++;
        }
        else if (out == mSocketOut2) {
            NS_ABORT_IF_FALSE(mSocketOut1,
                              "backup socket without primary being tested");

            sSuccessTransport2++;
            mSocketTransport.swap(mSocketTransport2);
            mSocketOut.swap(mSocketOut2);
            mSocketIn.swap(mSocketIn2);
        }
        else {
            NS_ABORT_IF_FALSE(0, "setup on unexpected socket");
            return;
        }
    }
    else if (out == mSocketOut1) {
        
        

        ReleaseBackupTransport(mSocketTransport1,
                               mSocketOut1,
                               mSocketIn1);
        sSuccessTransport1++;
        mSocketTransport1 = nsnull;
        mSocketOut1 = nsnull;
        mSocketIn1 = nsnull;
    }
    else if (out == mSocketOut2) {
        
        

        ReleaseBackupTransport(mSocketTransport2,
                               mSocketOut2,
                               mSocketIn2);
        sSuccessTransport2++;
        mSocketTransport2 = nsnull;
        mSocketOut2 = nsnull;
        mSocketIn2 = nsnull;
    }
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

    NS_ABORT_IF_FALSE(out == mSocketOut  ||
                      out == mSocketOut1 ||
                      out == mSocketOut2    , "unexpected socket");
    if (out != mSocketOut)
        SelectPrimaryTransport(out);

    
    if (!mTransaction) {
        LOG(("  no transaction; ignoring event\n"));
        return NS_OK;
    }

    if (mSocketOut == out) {
        NS_ABORT_IF_FALSE(!mIdleSynTimer,"IdleSynTimer should not be set");
        nsresult rv = OnSocketWritable();
        if (NS_FAILED(rv))
            CloseTransaction(mTransaction, rv);
    }

    return NS_OK;
}





NS_IMETHODIMP
nsHttpConnection::OnTransportStatus(nsITransport *trans,
                                    nsresult status,
                                    PRUint64 progress,
                                    PRUint64 progressMax)
{
    if (mTransaction)
        mTransaction->OnTransportStatus(status, progress);
    return NS_OK;
}






NS_IMETHODIMP
nsHttpConnection::GetInterface(const nsIID &iid, void **result)
{
    
    
    
    
    NS_ASSERTION(PR_GetCurrentThread() != gSocketThread, "wrong thread");
        
    nsCOMPtr<nsIInterfaceRequestor> callbacks = mCallbacks;
    if (!callbacks && mTransaction)
        mTransaction->GetSecurityCallbacks(getter_AddRefs(callbacks), nsnull);
    if (callbacks)
        return callbacks->GetInterface(iid, result);

    return NS_ERROR_NO_INTERFACE;
}
