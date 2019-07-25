






































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
#include "prmem.h"

#ifdef DEBUG

extern PRThread *gSocketThread;
#endif

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);





nsHttpConnection::nsHttpConnection()
    : mTransaction(nsnull)
    , mLastReadTime(0)
    , mIdleTimeout(0)
    , mConsiderReusedAfterInterval(0)
    , mConsiderReusedAfterEpoch(0)
    , mCurrentBytesRead(0)
    , mMaxBytesRead(0)
    , mKeepAlive(PR_TRUE) 
    , mKeepAliveMask(PR_TRUE)
    , mSupportsPipelining(PR_FALSE) 
    , mIsReused(PR_FALSE)
    , mCompletedProxyConnect(PR_FALSE)
    , mLastTransactionExpectedNoContent(PR_FALSE)
{
    LOG(("Creating nsHttpConnection @%x\n", this));

    
    nsHttpHandler *handler = gHttpHandler;
    NS_ADDREF(handler);
}

nsHttpConnection::~nsHttpConnection()
{
    LOG(("Destroying nsHttpConnection @%x\n", this));

    if (mCallbacks) {
        nsIInterfaceRequestor *cbs = nsnull;
        mCallbacks.swap(cbs);
        NS_ProxyRelease(mCallbackTarget, cbs);
    }

    
    nsHttpHandler *handler = gHttpHandler;
    NS_RELEASE(handler);
}

nsresult
nsHttpConnection::Init(nsHttpConnectionInfo *info,
                       PRUint16 maxHangTime,
                       nsISocketTransport *transport,
                       nsIAsyncInputStream *instream,
                       nsIAsyncOutputStream *outstream,
                       nsIInterfaceRequestor *callbacks,
                       nsIEventTarget *callbackTarget)
{
    NS_ABORT_IF_FALSE(transport && instream && outstream,
                      "invalid socket information");
    LOG(("nsHttpConnection::Init [this=%p "
         "transport=%p instream=%p outstream=%p]\n",
         this, transport, instream, outstream));

    NS_ENSURE_ARG_POINTER(info);
    NS_ENSURE_TRUE(!mConnInfo, NS_ERROR_ALREADY_INITIALIZED);

    mConnInfo = info;
    mMaxHangTime = maxHangTime;
    mLastReadTime = NowInSeconds();

    mSocketTransport = transport;
    mSocketIn = instream;
    mSocketOut = outstream;
    nsresult rv = mSocketTransport->SetEventSink(this, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    mCallbacks = callbacks;
    mCallbackTarget = callbackTarget;
    rv = mSocketTransport->SetSecurityCallbacks(this);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
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

    
    mKeepAliveMask = mKeepAlive = (caps & NS_HTTP_ALLOW_KEEPALIVE);

    
    
    if (((mConnInfo->UsingSSL() && mConnInfo->UsingHttpProxy()) ||
         mConnInfo->ShouldForceConnectMethod()) && !mCompletedProxyConnect) {
        rv = SetupProxyConnect();
        if (NS_FAILED(rv))
            goto failed_activation;
    }

    
    mCurrentBytesRead = 0;

    rv = OnOutputStreamReady(mSocketOut);
    
failed_activation:
    if (NS_FAILED(rv)) {
        mTransaction = nsnull;
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
    PRBool canReuse = IsKeepAlive() &&
        (NowInSeconds() - mLastReadTime < mIdleTimeout) &&
        IsAlive();
    
    
    
    
    
    

    PRUint32 dataSize;
    if (canReuse && mSocketIn && !mConnInfo->UsingSSL() &&
        NS_SUCCEEDED(mSocketIn->Available(&dataSize)) && dataSize) {
        LOG(("nsHttpConnection::CanReuse %p %s"
             "Socket not reusable because read data pending (%d) on it.\n",
             this, mConnInfo->Host(), dataSize));
        canReuse = PR_FALSE;
    }
    return canReuse;
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

            
            
            
            
            
            if (!mProxyConnectStream)
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

    
    
    
    
    if (mProxyConnectStream) {
        mProxyConnectStream = 0;
        if (responseHead->Status() == 200) {
            LOG(("proxy CONNECT succeeded! ssl=%s\n",
                 mConnInfo->UsingSSL() ? "true" :"false"));
            *reset = PR_TRUE;
            nsresult rv;
            if (mConnInfo->UsingSSL()) {
                rv = ProxyStartSSL();
                if (NS_FAILED(rv)) 
                    LOG(("ProxyStartSSL failed [rv=%x]\n", rv));
            }
            mCompletedProxyConnect = PR_TRUE;
            rv = mSocketOut->AsyncWait(this, 0, 0, nsnull);
            
            NS_ASSERTION(NS_SUCCEEDED(rv), "mSocketOut->AsyncWait failed");
        }
        else {
            LOG(("proxy CONNECT failed! ssl=%s\n",
                 mConnInfo->UsingSSL() ? "true" :"false"));
            mTransaction->SetSSLConnectFailed();
        }
    }

    return NS_OK;
}

PRBool
nsHttpConnection::IsReused()
{
    if (mIsReused)
        return PR_TRUE;
    if (!mConsiderReusedAfterInterval)
        return PR_FALSE;
    
    
    
    return (PR_IntervalNow() - mConsiderReusedAfterEpoch) >=
        mConsiderReusedAfterInterval;
}

void
nsHttpConnection::SetIsReusedAfter(PRUint32 afterMilliseconds)
{
    mConsiderReusedAfterEpoch = PR_IntervalNow();
    mConsiderReusedAfterInterval = PR_MillisecondsToInterval(afterMilliseconds);
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
        
        
        
        
        
        
        
        if (mProxyConnectStream) {
            LOG(("  writing CONNECT request stream\n"));
            rv = mProxyConnectStream->ReadSegments(ReadFromStream, this,
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
            
            
            
            
            
            
            mTransaction->OnTransportStatus(mSocketTransport,
                                            nsISocketTransport::STATUS_WAITING_FOR,
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
        else {
            mCurrentBytesRead += n;
            if (NS_FAILED(mSocketInCondition)) {
                
                if (mSocketInCondition == NS_BASE_STREAM_WOULD_BLOCK)
                    rv = mSocketIn->AsyncWait(this, 0, 0, nsnull);
                else
                    rv = mSocketInCondition;
                again = PR_FALSE;
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

    val = mTransaction->RequestHead()->PeekHeader(nsHttp::Host);
    if (val) {
        
        
        request.SetHeader(nsHttp::Host, nsDependentCString(val));
    }

    val = mTransaction->RequestHead()->PeekHeader(nsHttp::Proxy_Authorization);
    if (val) {
        
        
        request.SetHeader(nsHttp::Proxy_Authorization, nsDependentCString(val));
    }

    buf.Truncate();
    request.Flatten(buf, PR_FALSE);
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
                                    PRUint64 progress,
                                    PRUint64 progressMax)
{
    if (mTransaction)
        mTransaction->OnTransportStatus(trans, status, progress);
    return NS_OK;
}






NS_IMETHODIMP
nsHttpConnection::GetInterface(const nsIID &iid, void **result)
{
    
    
    
    

    
    
    

    NS_ASSERTION(PR_GetCurrentThread() != gSocketThread, "wrong thread");

    if (mCallbacks)
        return mCallbacks->GetInterface(iid, result);
    return NS_ERROR_NO_INTERFACE;
}
