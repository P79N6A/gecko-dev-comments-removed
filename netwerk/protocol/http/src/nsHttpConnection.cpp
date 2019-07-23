






































#include "nsHttpConnection.h"
#include "nsHttpTransaction.h"
#include "nsHttpRequestHead.h"
#include "nsHttpResponseHead.h"
#include "nsHttpHandler.h"
#include "nsISocketTransportService.h"
#include "nsISocketTransport.h"
#include "nsIServiceManager.h"
#include "nsISSLSocketControl.h"
#include "nsStringStream.h"
#include "netCore.h"
#include "nsNetCID.h"
#include "nsAutoLock.h"
#include "prmem.h"

#ifdef DEBUG

extern PRThread *gSocketThread;
#endif

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);





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
{
    LOG(("Creating nsHttpConnection @%x\n", this));

    
    nsHttpHandler *handler = gHttpHandler;
    NS_ADDREF(handler);
}

nsHttpConnection::~nsHttpConnection()
{
    LOG(("Destroying nsHttpConnection @%x\n", this));
 
    NS_IF_RELEASE(mConnInfo);
    NS_IF_RELEASE(mTransaction);

    if (mLock) {
        PR_DestroyLock(mLock);
        mLock = nsnull;
    }

    
    nsHttpHandler *handler = gHttpHandler;
    NS_RELEASE(handler);
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


nsresult
nsHttpConnection::Activate(nsAHttpTransaction *trans, PRUint8 caps)
{
    nsresult rv;

    LOG(("nsHttpConnection::Activate [this=%x trans=%x caps=%x]\n",
         this, trans, caps));

    NS_ENSURE_ARG_POINTER(trans);
    NS_ENSURE_TRUE(!mTransaction, NS_ERROR_IN_PROGRESS);

    
    mTransaction = trans;
    NS_ADDREF(mTransaction);

    
    mKeepAliveMask = mKeepAlive = (caps & NS_HTTP_ALLOW_KEEPALIVE);

    
    if (!mSocketTransport) {
        rv = CreateTransport();
        if (NS_FAILED(rv))
            goto loser;
    }

    
    if (mConnInfo->UsingSSL() && mConnInfo->UsingHttpProxy() && !mCompletedSSLConnect) {
        rv = SetupSSLProxyConnect();
        if (NS_FAILED(rv))
            goto loser;
    }

    
    rv = mSocketOut->AsyncWait(this, 0, 0, nsnull);
    if (NS_SUCCEEDED(rv))
        return rv;

loser:
    NS_RELEASE(mTransaction);
    return rv;
}

void
nsHttpConnection::Close(nsresult reason)
{
    LOG(("nsHttpConnection::Close [this=%x reason=%x]\n", this, reason));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

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
    return IsKeepAlive() && (NowInSeconds() - mLastReadTime < mIdleTimeout)
                         && IsAlive();
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

    
    static const char *bad_servers[] = {
        "Microsoft-IIS/4.",
        "Microsoft-IIS/5.",
        "Netscape-Enterprise/3.",
        nsnull
    };

    for (const char **server = bad_servers; *server; ++server) {
        if (PL_strcasestr(val, *server) != nsnull) {
            LOG(("looks like this server does not support pipelining"));
            return PR_FALSE;
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
                    NS_STATIC_CAST(nsHttpTransaction *, mTransaction);
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
nsHttpConnection::CreateTransport()
{
    nsresult rv;

    NS_PRECONDITION(!mSocketTransport, "unexpected");

    nsCOMPtr<nsISocketTransportService> sts =
            do_GetService(kSocketTransportServiceCID, &rv);
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

    mSocketTransport = strans;
    mSocketIn = do_QueryInterface(sin);
    mSocketOut = do_QueryInterface(sout);
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

    NS_RELEASE(mTransaction);
    mTransaction = 0;

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
                                                      NS_HTTP_SEGMENT_SIZE, &n);
        }
        else {
            LOG(("  writing transaction request stream\n"));
            rv = mTransaction->ReadSegments(this, NS_HTTP_SEGMENT_SIZE, &n);
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
        rv = mTransaction->WriteSegments(this, NS_HTTP_SEGMENT_SIZE, &n);
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
    buf.Assign(mConnInfo->Host());
    buf.Append(':');
    buf.AppendInt(mConnInfo->Port());

    
    nsHttpRequestHead request;
    request.SetMethod(nsHttp::Connect);
    request.SetVersion(gHttpHandler->HttpVersion());
    request.SetRequestURI(buf);
    request.SetHeader(nsHttp::User_Agent, gHttpHandler->UserAgent());

    
    request.SetHeader(nsHttp::Proxy_Connection, NS_LITERAL_CSTRING("keep-alive"));

    
    
    nsHttpTransaction *trans = NS_STATIC_CAST(nsHttpTransaction *, mTransaction);
    
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
    NS_ASSERTION(out == mSocketOut, "unexpected stream");
    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    
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
        mTransaction->OnTransportStatus(status, progress);
    return NS_OK;
}






NS_IMETHODIMP
nsHttpConnection::GetInterface(const nsIID &iid, void **result)
{
    
    
    
    
    NS_ASSERTION(PR_GetCurrentThread() != gSocketThread, "wrong thread");
 
    if (mTransaction) {
        nsCOMPtr<nsIInterfaceRequestor> callbacks;
        mTransaction->GetSecurityCallbacks(getter_AddRefs(callbacks));
        if (callbacks)
            return callbacks->GetInterface(iid, result);
    }

    return NS_ERROR_NO_INTERFACE;
}
