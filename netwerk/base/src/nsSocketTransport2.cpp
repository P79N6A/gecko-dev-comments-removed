







































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif

#include "nsSocketTransport2.h"
#include "nsIOService.h"
#include "nsStreamUtils.h"
#include "nsNetSegmentUtils.h"
#include "nsTransportUtils.h"
#include "nsProxyInfo.h"
#include "nsNetCID.h"
#include "nsAutoLock.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "netCore.h"
#include "nsInt64.h"
#include "prmem.h"
#include "pratom.h"
#include "plstr.h"
#include "prnetdb.h"
#include "prerror.h"
#include "prerr.h"

#include "nsIServiceManager.h"
#include "nsIProxyObjectManager.h"
#include "nsISocketProviderService.h"
#include "nsISocketProvider.h"
#include "nsISSLSocketControl.h"
#include "nsINSSErrorsService.h"
#include "nsIPipe.h"
#include "nsIProgrammingLanguage.h"
#include "nsIClassInfoImpl.h"

#if defined(XP_WIN) || defined(MOZ_ENABLE_LIBCONIC)
#include "nsNativeConnectionHelper.h"
#endif



static NS_DEFINE_CID(kSocketProviderServiceCID, NS_SOCKETPROVIDERSERVICE_CID);
static NS_DEFINE_CID(kDNSServiceCID, NS_DNSSERVICE_CID);



class nsSocketEvent : public nsRunnable
{
public:
    nsSocketEvent(nsSocketTransport *transport, PRUint32 type,
                  nsresult status = NS_OK, nsISupports *param = nsnull)
        : mTransport(transport)
        , mType(type)
        , mStatus(status)
        , mParam(param)
    {}

    NS_IMETHOD Run()
    {
        mTransport->OnSocketEvent(mType, mStatus, mParam);
        return NS_OK;
    }

private:
    nsRefPtr<nsSocketTransport> mTransport;

    PRUint32              mType;
    nsresult              mStatus;
    nsCOMPtr<nsISupports> mParam;
};




#ifdef TEST_CONNECT_ERRORS
#include <stdlib.h>
static PRErrorCode RandomizeConnectError(PRErrorCode code)
{
    
    
    
    
    int n = rand();
    if (n > RAND_MAX/2) {
        struct {
            PRErrorCode err_code;
            const char *err_name;
        } 
        errors[] = {
            
            
            
            
            { PR_CONNECT_REFUSED_ERROR, "PR_CONNECT_REFUSED_ERROR" },
            { PR_CONNECT_TIMEOUT_ERROR, "PR_CONNECT_TIMEOUT_ERROR" },
            
            
            
            
            
            { PR_CONNECT_RESET_ERROR, "PR_CONNECT_RESET_ERROR" },
        };
        n = n % (sizeof(errors)/sizeof(errors[0]));
        code = errors[n].err_code;
        LOG(("simulating NSPR error %d [%s]\n", code, errors[n].err_name));
    }
    return code;
}
#endif



static PRBool
IsNSSErrorCode(PRErrorCode code)
{
  return 
    ((code >= nsINSSErrorsService::NSS_SEC_ERROR_BASE) && 
      (code < nsINSSErrorsService::NSS_SEC_ERROR_LIMIT))
    ||
    ((code >= nsINSSErrorsService::NSS_SSL_ERROR_BASE) && 
      (code < nsINSSErrorsService::NSS_SSL_ERROR_LIMIT));
}




static nsresult
GetXPCOMFromNSSError(PRErrorCode code)
{
    return NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_SECURITY, -1 * code);
}

static nsresult
ErrorAccordingToNSPR(PRErrorCode errorCode)
{
    nsresult rv = NS_ERROR_FAILURE;
    switch (errorCode) {
    case PR_WOULD_BLOCK_ERROR:
        rv = NS_BASE_STREAM_WOULD_BLOCK;
        break;
    case PR_CONNECT_ABORTED_ERROR:
    case PR_CONNECT_RESET_ERROR:
        rv = NS_ERROR_NET_RESET;
        break;
    case PR_END_OF_FILE_ERROR: 
        rv = NS_ERROR_NET_INTERRUPT;
        break;
    case PR_CONNECT_REFUSED_ERROR:
    case PR_NETWORK_UNREACHABLE_ERROR: 
    case PR_HOST_UNREACHABLE_ERROR:    
    case PR_ADDRESS_NOT_AVAILABLE_ERROR:
    
    
    case PR_ADDRESS_NOT_SUPPORTED_ERROR:
    case PR_NO_ACCESS_RIGHTS_ERROR:
        rv = NS_ERROR_CONNECTION_REFUSED;
        break;
    case PR_IO_TIMEOUT_ERROR:
    case PR_CONNECT_TIMEOUT_ERROR:
        rv = NS_ERROR_NET_TIMEOUT;
        break;
    default:
        if (IsNSSErrorCode(errorCode))
            rv = GetXPCOMFromNSSError(errorCode);
        break;
    }
    LOG(("ErrorAccordingToNSPR [in=%d out=%x]\n", errorCode, rv));
    return rv;
}





nsSocketInputStream::nsSocketInputStream(nsSocketTransport *trans)
    : mTransport(trans)
    , mReaderRefCnt(0)
    , mCondition(NS_OK)
    , mCallbackFlags(0)
    , mByteCount(0)
{
}

nsSocketInputStream::~nsSocketInputStream()
{
}





void
nsSocketInputStream::OnSocketReady(nsresult condition)
{
    LOG(("nsSocketInputStream::OnSocketReady [this=%x cond=%x]\n",
        this, condition));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    nsCOMPtr<nsIInputStreamCallback> callback;
    {
        nsAutoLock lock(mTransport->mLock);

        
        
        if (NS_SUCCEEDED(mCondition))
            mCondition = condition;

        
        if (NS_FAILED(mCondition) || !(mCallbackFlags & WAIT_CLOSURE_ONLY)) {
            callback = mCallback;
            mCallback = nsnull;
            mCallbackFlags = 0;
        }
    }

    if (callback)
        callback->OnInputStreamReady(this);
}

NS_IMPL_QUERY_INTERFACE2(nsSocketInputStream,
                         nsIInputStream,
                         nsIAsyncInputStream)

NS_IMETHODIMP_(nsrefcnt)
nsSocketInputStream::AddRef()
{
    PR_AtomicIncrement((PRInt32*)&mReaderRefCnt);
    return mTransport->AddRef();
}

NS_IMETHODIMP_(nsrefcnt)
nsSocketInputStream::Release()
{
    if (PR_AtomicDecrement((PRInt32*)&mReaderRefCnt) == 0)
        Close();
    return mTransport->Release();
}

NS_IMETHODIMP
nsSocketInputStream::Close()
{
    return CloseWithStatus(NS_BASE_STREAM_CLOSED);
}

NS_IMETHODIMP
nsSocketInputStream::Available(PRUint32 *avail)
{
    LOG(("nsSocketInputStream::Available [this=%x]\n", this));

    *avail = 0;

    PRFileDesc *fd;
    {
        nsAutoLock lock(mTransport->mLock);

        if (NS_FAILED(mCondition))
            return mCondition;

        fd = mTransport->GetFD_Locked();
        if (!fd)
            return NS_OK;
    }

    
    
    
    PRInt32 n = PR_Available(fd);

    nsresult rv;
    {
        nsAutoLock lock(mTransport->mLock);

        mTransport->ReleaseFD_Locked(fd);

        if (n >= 0)
            *avail = n;
        else {
            PRErrorCode code = PR_GetError();
            if (code == PR_WOULD_BLOCK_ERROR)
                return NS_OK;
            mCondition = ErrorAccordingToNSPR(code);
        }
        rv = mCondition;
    }
    if (NS_FAILED(rv))
        mTransport->OnInputClosed(rv);
    return rv;
}

NS_IMETHODIMP
nsSocketInputStream::Read(char *buf, PRUint32 count, PRUint32 *countRead)
{
    LOG(("nsSocketInputStream::Read [this=%x count=%u]\n", this, count));

    *countRead = 0;

    PRFileDesc *fd;
    {
        nsAutoLock lock(mTransport->mLock);

        if (NS_FAILED(mCondition))
            return (mCondition == NS_BASE_STREAM_CLOSED) ? NS_OK : mCondition;

        fd = mTransport->GetFD_Locked();
        if (!fd)
            return NS_BASE_STREAM_WOULD_BLOCK;
    }

    LOG(("  calling PR_Read [count=%u]\n", count));

    
    
    
    PRInt32 n = PR_Read(fd, buf, count);

    LOG(("  PR_Read returned [n=%d]\n", n));

    nsresult rv;
    {
        nsAutoLock lock(mTransport->mLock);

#ifdef ENABLE_SOCKET_TRACING
        if (n > 0)
            mTransport->TraceInBuf(buf, n);
#endif

        mTransport->ReleaseFD_Locked(fd);

        if (n > 0)
            mByteCount += (*countRead = n);
        else if (n < 0) {
            PRErrorCode code = PR_GetError();
            if (code == PR_WOULD_BLOCK_ERROR)
                return NS_BASE_STREAM_WOULD_BLOCK;
            mCondition = ErrorAccordingToNSPR(code);
        }
        rv = mCondition;
    }
    if (NS_FAILED(rv))
        mTransport->OnInputClosed(rv);

    
    
    if (n > 0)
        mTransport->SendStatus(nsISocketTransport::STATUS_RECEIVING_FROM);
    return rv;
}

NS_IMETHODIMP
nsSocketInputStream::ReadSegments(nsWriteSegmentFun writer, void *closure,
                                  PRUint32 count, PRUint32 *countRead)
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsSocketInputStream::IsNonBlocking(PRBool *nonblocking)
{
    *nonblocking = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketInputStream::CloseWithStatus(nsresult reason)
{
    LOG(("nsSocketInputStream::CloseWithStatus [this=%x reason=%x]\n", this, reason));

    
 
    nsresult rv;
    {
        nsAutoLock lock(mTransport->mLock);

        if (NS_SUCCEEDED(mCondition))
            rv = mCondition = reason;
        else
            rv = NS_OK;
    }
    if (NS_FAILED(rv))
        mTransport->OnInputClosed(rv);
    return NS_OK;
}

NS_IMETHODIMP
nsSocketInputStream::AsyncWait(nsIInputStreamCallback *callback,
                               PRUint32 flags,
                               PRUint32 amount,
                               nsIEventTarget *target)
{
    LOG(("nsSocketInputStream::AsyncWait [this=%x]\n", this));

    
    
    
    nsCOMPtr<nsIInputStreamCallback> directCallback;
    {
        nsAutoLock lock(mTransport->mLock);

        if (callback && target) {
            
            
            
            
            
            
            nsCOMPtr<nsIInputStreamCallback> temp;
            nsresult rv = NS_NewInputStreamReadyEvent(getter_AddRefs(temp),
                                                      callback, target);
            if (NS_FAILED(rv)) return rv;
            mCallback = temp;
        }
        else
            mCallback = callback;

        if (NS_FAILED(mCondition))
            directCallback.swap(mCallback);
        else
            mCallbackFlags = flags;
    }
    if (directCallback)
        directCallback->OnInputStreamReady(this);
    else
        mTransport->OnInputPending();

    return NS_OK;
}





nsSocketOutputStream::nsSocketOutputStream(nsSocketTransport *trans)
    : mTransport(trans)
    , mWriterRefCnt(0)
    , mCondition(NS_OK)
    , mCallbackFlags(0)
    , mByteCount(0)
{
}

nsSocketOutputStream::~nsSocketOutputStream()
{
}





void
nsSocketOutputStream::OnSocketReady(nsresult condition)
{
    LOG(("nsSocketOutputStream::OnSocketReady [this=%x cond=%x]\n",
        this, condition));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    nsCOMPtr<nsIOutputStreamCallback> callback;
    {
        nsAutoLock lock(mTransport->mLock);

        
        
        if (NS_SUCCEEDED(mCondition))
            mCondition = condition;

        
        if (NS_FAILED(mCondition) || !(mCallbackFlags & WAIT_CLOSURE_ONLY)) {
            callback = mCallback;
            mCallback = nsnull;
            mCallbackFlags = 0;
        }
    }

    if (callback)
        callback->OnOutputStreamReady(this);
}

NS_IMPL_QUERY_INTERFACE2(nsSocketOutputStream,
                         nsIOutputStream,
                         nsIAsyncOutputStream)

NS_IMETHODIMP_(nsrefcnt)
nsSocketOutputStream::AddRef()
{
    PR_AtomicIncrement((PRInt32*)&mWriterRefCnt);
    return mTransport->AddRef();
}

NS_IMETHODIMP_(nsrefcnt)
nsSocketOutputStream::Release()
{
    if (PR_AtomicDecrement((PRInt32*)&mWriterRefCnt) == 0)
        Close();
    return mTransport->Release();
}

NS_IMETHODIMP
nsSocketOutputStream::Close()
{
    return CloseWithStatus(NS_BASE_STREAM_CLOSED);
}

NS_IMETHODIMP
nsSocketOutputStream::Flush()
{
    return NS_OK;
}

NS_IMETHODIMP
nsSocketOutputStream::Write(const char *buf, PRUint32 count, PRUint32 *countWritten)
{
    LOG(("nsSocketOutputStream::Write [this=%x count=%u]\n", this, count));

    *countWritten = 0;

    if (count == 0)
        return NS_OK;

    PRFileDesc *fd;
    {
        nsAutoLock lock(mTransport->mLock);

        if (NS_FAILED(mCondition))
            return mCondition;
        
        fd = mTransport->GetFD_Locked();
        if (!fd)
            return NS_BASE_STREAM_WOULD_BLOCK;
    }

    LOG(("  calling PR_Write [count=%u]\n", count));

    
    
    
    PRInt32 n = PR_Write(fd, buf, count);

    LOG(("  PR_Write returned [n=%d]\n", n));
    NS_ASSERTION(n != 0, "unexpected return value");

    nsresult rv;
    {
        nsAutoLock lock(mTransport->mLock);

#ifdef ENABLE_SOCKET_TRACING
    if (n > 0)
        mTransport->TraceOutBuf(buf, n);
#endif

        mTransport->ReleaseFD_Locked(fd);

        if (n > 0)
            mByteCount += (*countWritten = n);
        else if (n < 0) {
            PRErrorCode code = PR_GetError();
            if (code == PR_WOULD_BLOCK_ERROR)
                return NS_BASE_STREAM_WOULD_BLOCK;
            mCondition = ErrorAccordingToNSPR(code);
        }
        rv = mCondition;
    }
    if (NS_FAILED(rv))
        mTransport->OnOutputClosed(rv);

    
    
    if (n > 0)
        mTransport->SendStatus(nsISocketTransport::STATUS_SENDING_TO);
    return rv;
}

NS_IMETHODIMP
nsSocketOutputStream::WriteSegments(nsReadSegmentFun reader, void *closure,
                                    PRUint32 count, PRUint32 *countRead)
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD
nsSocketOutputStream::WriteFromSegments(nsIInputStream *input,
                                        void *closure,
                                        const char *fromSegment,
                                        PRUint32 offset,
                                        PRUint32 count,
                                        PRUint32 *countRead)
{
    nsSocketOutputStream *self = (nsSocketOutputStream *) closure;
    return self->Write(fromSegment, count, countRead);
}

NS_IMETHODIMP
nsSocketOutputStream::WriteFrom(nsIInputStream *stream, PRUint32 count, PRUint32 *countRead)
{
    return stream->ReadSegments(WriteFromSegments, this, count, countRead);
}

NS_IMETHODIMP
nsSocketOutputStream::IsNonBlocking(PRBool *nonblocking)
{
    *nonblocking = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketOutputStream::CloseWithStatus(nsresult reason)
{
    LOG(("nsSocketOutputStream::CloseWithStatus [this=%x reason=%x]\n", this, reason));

    
 
    nsresult rv;
    {
        nsAutoLock lock(mTransport->mLock);

        if (NS_SUCCEEDED(mCondition))
            rv = mCondition = reason;
        else
            rv = NS_OK;
    }
    if (NS_FAILED(rv))
        mTransport->OnOutputClosed(rv);
    return NS_OK;
}

NS_IMETHODIMP
nsSocketOutputStream::AsyncWait(nsIOutputStreamCallback *callback,
                                PRUint32 flags,
                                PRUint32 amount,
                                nsIEventTarget *target)
{
    LOG(("nsSocketOutputStream::AsyncWait [this=%x]\n", this));

    {
        nsAutoLock lock(mTransport->mLock);

        if (callback && target) {
            
            
            
            
            
            
            nsCOMPtr<nsIOutputStreamCallback> temp;
            nsresult rv = NS_NewOutputStreamReadyEvent(getter_AddRefs(temp),
                                                       callback, target);
            if (NS_FAILED(rv)) return rv;
            mCallback = temp;
        }
        else
            mCallback = callback;

        mCallbackFlags = flags;
    }
    mTransport->OnOutputPending();
    return NS_OK;
}





nsSocketTransport::nsSocketTransport()
    : mTypes(nsnull)
    , mTypeCount(0)
    , mPort(0)
    , mProxyPort(0)
    , mProxyTransparent(PR_FALSE)
    , mProxyTransparentResolvesHost(PR_FALSE)
    , mConnectionFlags(0)
    , mState(STATE_CLOSED)
    , mAttached(PR_FALSE)
    , mInputClosed(PR_TRUE)
    , mOutputClosed(PR_TRUE)
    , mResolving(PR_FALSE)
    , mLock(PR_NewLock())
    , mFD(nsnull)
    , mFDref(0)
    , mFDconnected(PR_FALSE)
    , mInput(this)
    , mOutput(this)
{
    LOG(("creating nsSocketTransport @%x\n", this));

    NS_ADDREF(gSocketTransportService);

    mTimeouts[TIMEOUT_CONNECT]    = PR_UINT16_MAX; 
    mTimeouts[TIMEOUT_READ_WRITE] = PR_UINT16_MAX; 
}

nsSocketTransport::~nsSocketTransport()
{
    LOG(("destroying nsSocketTransport @%x\n", this));

    
    if (mTypes) {
        PRUint32 i;
        for (i=0; i<mTypeCount; ++i)
            PL_strfree(mTypes[i]);
        free(mTypes);
    }

    if (mLock)
        PR_DestroyLock(mLock);
 
    nsSocketTransportService *serv = gSocketTransportService;
    NS_RELEASE(serv); 
}

nsresult
nsSocketTransport::Init(const char **types, PRUint32 typeCount,
                        const nsACString &host, PRUint16 port,
                        nsIProxyInfo *givenProxyInfo)
{
    if (!mLock)
        return NS_ERROR_OUT_OF_MEMORY;

    nsCOMPtr<nsProxyInfo> proxyInfo;
    if (givenProxyInfo) {
        proxyInfo = do_QueryInterface(givenProxyInfo);
        NS_ENSURE_ARG(proxyInfo);
    }

    

    mPort = port;
    mHost = host;

    const char *proxyType = nsnull;
    if (proxyInfo) {
        mProxyPort = proxyInfo->Port();
        mProxyHost = proxyInfo->Host();
        
        proxyType = proxyInfo->Type();
        if (proxyType && (strcmp(proxyType, "http") == 0 ||
                          strcmp(proxyType, "direct") == 0 ||
                          strcmp(proxyType, "unknown") == 0))
            proxyType = nsnull;
    }

    LOG(("nsSocketTransport::Init [this=%x host=%s:%hu proxy=%s:%hu]\n",
        this, mHost.get(), mPort, mProxyHost.get(), mProxyPort));

    
    mTypeCount = typeCount + (proxyType != nsnull);
    if (!mTypeCount)
        return NS_OK;

    
    
    nsresult rv;
    nsCOMPtr<nsISocketProviderService> spserv =
        do_GetService(kSocketProviderServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    mTypes = (char **) malloc(mTypeCount * sizeof(char *));
    if (!mTypes)
        return NS_ERROR_OUT_OF_MEMORY;

    
    for (PRUint32 i = 0, type = 0; i < mTypeCount; ++i) {
        
        if (i == 0 && proxyType)
            mTypes[i] = PL_strdup(proxyType);
        else
            mTypes[i] = PL_strdup(types[type++]);

        if (!mTypes[i]) {
            mTypeCount = i;
            return NS_ERROR_OUT_OF_MEMORY;
        }
        nsCOMPtr<nsISocketProvider> provider;
        rv = spserv->GetSocketProvider(mTypes[i], getter_AddRefs(provider));
        if (NS_FAILED(rv)) {
            NS_WARNING("no registered socket provider");
            return rv;
        }

        
        
        if ((strcmp(mTypes[i], "socks") == 0) ||
            (strcmp(mTypes[i], "socks4") == 0)) {
            mProxyTransparent = PR_TRUE;

            if (proxyInfo->Flags() & nsIProxyInfo::TRANSPARENT_PROXY_RESOLVES_HOST) {
                
                
                mProxyTransparentResolvesHost = PR_TRUE;
            }
        }
    }

    return NS_OK;
}

nsresult
nsSocketTransport::InitWithConnectedSocket(PRFileDesc *fd, const PRNetAddr *addr)
{
    if (!mLock)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ASSERTION(!mFD, "already initialized");

    char buf[64];
    PR_NetAddrToString(addr, buf, sizeof(buf));
    mHost.Assign(buf);

    PRUint16 port;
    if (addr->raw.family == PR_AF_INET)
        port = addr->inet.port;
    else
        port = addr->ipv6.port;
    mPort = PR_ntohs(port);

    memcpy(&mNetAddr, addr, sizeof(PRNetAddr));

    mPollFlags = (PR_POLL_READ | PR_POLL_WRITE | PR_POLL_EXCEPT);
    mPollTimeout = mTimeouts[TIMEOUT_READ_WRITE];
    mState = STATE_TRANSFERRING;

    mFD = fd;
    mFDref = 1;
    mFDconnected = 1;

    
    PRSocketOptionData opt;
    opt.option = PR_SockOpt_Nonblocking;
    opt.value.non_blocking = PR_TRUE;
    PR_SetSocketOption(mFD, &opt);

    LOG(("nsSocketTransport::InitWithConnectedSocket [this=%p addr=%s:%hu]\n",
        this, mHost.get(), mPort));

    
    return PostEvent(MSG_RETRY_INIT_SOCKET);
}

nsresult
nsSocketTransport::PostEvent(PRUint32 type, nsresult status, nsISupports *param)
{
    LOG(("nsSocketTransport::PostEvent [this=%p type=%u status=%x param=%p]\n",
        this, type, status, param));

    nsCOMPtr<nsIRunnable> event = new nsSocketEvent(this, type, status, param);
    if (!event)
        return NS_ERROR_OUT_OF_MEMORY;

    return gSocketTransportService->Dispatch(event, NS_DISPATCH_NORMAL);
}

void
nsSocketTransport::SendStatus(nsresult status)
{
    LOG(("nsSocketTransport::SendStatus [this=%x status=%x]\n", this, status));

    nsCOMPtr<nsITransportEventSink> sink;
    PRUint64 progress;
    {
        nsAutoLock lock(mLock);
        sink = mEventSink;
        switch (status) {
        case STATUS_SENDING_TO:
            progress = mOutput.ByteCount();
            break;
        case STATUS_RECEIVING_FROM:
            progress = mInput.ByteCount();
            break;
        default:
            progress = 0;
            break;
        }
    }
    if (sink)
        sink->OnTransportStatus(this, status, progress, LL_MAXUINT);
}

nsresult
nsSocketTransport::ResolveHost()
{
    LOG(("nsSocketTransport::ResolveHost [this=%x]\n", this));

    nsresult rv;

    if (!mProxyHost.IsEmpty()) {
        if (!mProxyTransparent || mProxyTransparentResolvesHost) {
            
            
            if (!net_IsValidHostName(mHost))
                return NS_ERROR_UNKNOWN_HOST;
        }
        if (mProxyTransparentResolvesHost) {
            
            
            
            
            
            
            
            mState = STATE_RESOLVING;
            PR_SetNetAddr(PR_IpAddrAny, PR_AF_INET, SocketPort(), &mNetAddr);
            return PostEvent(MSG_DNS_LOOKUP_COMPLETE, NS_OK, nsnull);
        }
    }

    nsCOMPtr<nsIDNSService> dns = do_GetService(kDNSServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    mResolving = PR_TRUE;

    PRUint32 dnsFlags = 0;
    if (mConnectionFlags & nsSocketTransport::BYPASS_CACHE)
        dnsFlags = nsIDNSService::RESOLVE_BYPASS_CACHE;

    rv = dns->AsyncResolve(SocketHost(), dnsFlags, this, nsnull,
                           getter_AddRefs(mDNSRequest));
    if (NS_SUCCEEDED(rv)) {
        LOG(("  advancing to STATE_RESOLVING\n"));
        mState = STATE_RESOLVING;
        
        if (mResolving)
            SendStatus(STATUS_RESOLVING);
    }
    return rv;
}

nsresult
nsSocketTransport::BuildSocket(PRFileDesc *&fd, PRBool &proxyTransparent, PRBool &usingSSL)
{
    LOG(("nsSocketTransport::BuildSocket [this=%x]\n", this));

    nsresult rv;

    proxyTransparent = PR_FALSE;
    usingSSL = PR_FALSE;

    if (mTypeCount == 0) {
        fd = PR_OpenTCPSocket(mNetAddr.raw.family);
        rv = fd ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
    }
    else {
        fd = nsnull;

        nsCOMPtr<nsISocketProviderService> spserv =
            do_GetService(kSocketProviderServiceCID, &rv);
        if (NS_FAILED(rv)) return rv;

        const char *host       = mHost.get();
        PRInt32     port       = (PRInt32) mPort;
        const char *proxyHost  = mProxyHost.IsEmpty() ? nsnull : mProxyHost.get();
        PRInt32     proxyPort  = (PRInt32) mProxyPort;
        PRUint32    proxyFlags = 0;

        PRUint32 i;
        for (i=0; i<mTypeCount; ++i) {
            nsCOMPtr<nsISocketProvider> provider;

            LOG(("  pushing io layer [%u:%s]\n", i, mTypes[i]));

            rv = spserv->GetSocketProvider(mTypes[i], getter_AddRefs(provider));
            if (NS_FAILED(rv))
                break;

            if (mProxyTransparentResolvesHost)
                proxyFlags |= nsISocketProvider::PROXY_RESOLVES_HOST;
            
            if (mConnectionFlags & nsISocketTransport::ANONYMOUS_CONNECT)
                proxyFlags |= nsISocketProvider::ANONYMOUS_CONNECT;

            nsCOMPtr<nsISupports> secinfo;
            if (i == 0) {
                
                
                rv = provider->NewSocket(mNetAddr.raw.family,
                                         host, port, proxyHost, proxyPort,
                                         proxyFlags, &fd,
                                         getter_AddRefs(secinfo));

                if (NS_SUCCEEDED(rv) && !fd) {
                    NS_NOTREACHED("NewSocket succeeded but failed to create a PRFileDesc");
                    rv = NS_ERROR_UNEXPECTED;
                }
            }
            else {
                
                
                
                rv = provider->AddToSocket(mNetAddr.raw.family,
                                           host, port, proxyHost, proxyPort,
                                           proxyFlags, fd,
                                           getter_AddRefs(secinfo));
            }
            
            if (NS_FAILED(rv))
                break;

            
            PRBool isSSL = (strcmp(mTypes[i], "ssl") == 0);
            if (isSSL || (strcmp(mTypes[i], "starttls") == 0)) {
                
                nsCOMPtr<nsIInterfaceRequestor> callbacks;
                {
                    nsAutoLock lock(mLock);
                    mSecInfo = secinfo;
                    callbacks = mCallbacks;
                    LOG(("  [secinfo=%x callbacks=%x]\n", mSecInfo.get(), mCallbacks.get()));
                }
                
                nsCOMPtr<nsISSLSocketControl> secCtrl(do_QueryInterface(secinfo));
                if (secCtrl)
                    secCtrl->SetNotificationCallbacks(callbacks);
                
                usingSSL = isSSL;
            }
            else if ((strcmp(mTypes[i], "socks") == 0) ||
                     (strcmp(mTypes[i], "socks4") == 0)) {
                
                
                proxyHost = nsnull;
                proxyPort = -1;
                proxyTransparent = PR_TRUE;
            }
        }

        if (NS_FAILED(rv)) {
            LOG(("  error pushing io layer [%u:%s rv=%x]\n", i, mTypes[i], rv));
            if (fd)
                PR_Close(fd);
        }
    }

    return rv;
}

nsresult
nsSocketTransport::InitiateSocket()
{
    LOG(("nsSocketTransport::InitiateSocket [this=%x]\n", this));

    nsresult rv;

    
    
    
    
    
    
    
    
    
    
    
    
    if (!gSocketTransportService->CanAttachSocket()) {
        nsCOMPtr<nsIRunnable> event =
                new nsSocketEvent(this, MSG_RETRY_INIT_SOCKET);
        if (!event)
            return NS_ERROR_OUT_OF_MEMORY;
        return gSocketTransportService->NotifyWhenCanAttachSocket(event);
    }

    
    
    
    if (mFD) {
        rv = gSocketTransportService->AttachSocket(mFD, this);
        if (NS_SUCCEEDED(rv))
            mAttached = PR_TRUE;
        return rv;
    }

    
    
    
    PRFileDesc *fd;
    PRBool proxyTransparent;
    PRBool usingSSL;

    rv = BuildSocket(fd, proxyTransparent, usingSSL);
    if (NS_FAILED(rv)) {
        LOG(("  BuildSocket failed [rv=%x]\n", rv));
        return rv;
    }

    PRStatus status;

    
    PRSocketOptionData opt;
    opt.option = PR_SockOpt_Nonblocking;
    opt.value.non_blocking = PR_TRUE;
    status = PR_SetSocketOption(fd, &opt);
    NS_ASSERTION(status == PR_SUCCESS, "unable to make socket non-blocking");

    
    
    
    PRInt32 sndBufferSize;
    gSocketTransportService->GetSendBufferSize(&sndBufferSize);
    if (sndBufferSize > 0) {
        opt.option = PR_SockOpt_SendBufferSize;
        opt.value.send_buffer_size = sndBufferSize;
        PR_SetSocketOption(fd, &opt);
    }

    
    rv = gSocketTransportService->AttachSocket(fd, this);
    if (NS_FAILED(rv)) {
        PR_Close(fd);
        return rv;
    }
    mAttached = PR_TRUE;

    
    
    {
        nsAutoLock lock(mLock);
        mFD = fd;
        mFDref = 1;
        mFDconnected = PR_FALSE;
    }

    LOG(("  advancing to STATE_CONNECTING\n"));
    mState = STATE_CONNECTING;
    mPollTimeout = mTimeouts[TIMEOUT_CONNECT];
    SendStatus(STATUS_CONNECTING_TO);

#if defined(PR_LOGGING)
    if (LOG_ENABLED()) {
        char buf[64];
        PR_NetAddrToString(&mNetAddr, buf, sizeof(buf));
        LOG(("  trying address: %s\n", buf));
    }
#endif

    
    
    
    status = PR_Connect(fd, &mNetAddr, NS_SOCKET_CONNECT_TIMEOUT);
    if (status == PR_SUCCESS) {
        
        
        
        OnSocketConnected();
    }
    else {
        PRErrorCode code = PR_GetError();
#if defined(TEST_CONNECT_ERRORS)
        code = RandomizeConnectError(code);
#endif
        
        
        
        if ((PR_WOULD_BLOCK_ERROR == code) || (PR_IN_PROGRESS_ERROR == code))
            mPollFlags = (PR_POLL_EXCEPT | PR_POLL_WRITE);
        
        
        
        else if (PR_IS_CONNECTED_ERROR == code) {
            
            
            
            OnSocketConnected();

            if (mSecInfo && !mProxyHost.IsEmpty() && proxyTransparent && usingSSL) {
                
                
                
                
                nsCOMPtr<nsISSLSocketControl> secCtrl =
                    do_QueryInterface(mSecInfo);
                if (secCtrl) {
                    LOG(("  calling ProxyStartSSL()\n"));
                    secCtrl->ProxyStartSSL();
                }
                
                
                
                
                
                
            }
        }
        
        
        
        else {
            rv = ErrorAccordingToNSPR(code);
            if ((rv == NS_ERROR_CONNECTION_REFUSED) && !mProxyHost.IsEmpty())
                rv = NS_ERROR_PROXY_CONNECTION_REFUSED;
        }
    }
    return rv;
}

PRBool
nsSocketTransport::RecoverFromError()
{
    NS_ASSERTION(NS_FAILED(mCondition), "there should be something wrong");

    LOG(("nsSocketTransport::RecoverFromError [this=%x state=%x cond=%x]\n",
        this, mState, mCondition));

    
    if (mState != STATE_RESOLVING && mState != STATE_CONNECTING)
        return PR_FALSE;

    
    NS_ASSERTION(!mFDconnected, "socket should not be connected");

    
    if (mCondition != NS_ERROR_CONNECTION_REFUSED &&
        mCondition != NS_ERROR_PROXY_CONNECTION_REFUSED &&
        mCondition != NS_ERROR_NET_TIMEOUT &&
        mCondition != NS_ERROR_UNKNOWN_HOST &&
        mCondition != NS_ERROR_UNKNOWN_PROXY_HOST)
        return PR_FALSE;

    PRBool tryAgain = PR_FALSE;

    
    if (mState == STATE_CONNECTING && mDNSRecord) {
        nsresult rv = mDNSRecord->GetNextAddr(SocketPort(), &mNetAddr);
        if (NS_SUCCEEDED(rv)) {
            LOG(("  trying again with next ip address\n"));
            tryAgain = PR_TRUE;
        }
    }

#if defined(XP_WIN) || defined(MOZ_ENABLE_LIBCONIC)
    
    
    if (!tryAgain) {
        PRBool autodialEnabled;
        gSocketTransportService->GetAutodialEnabled(&autodialEnabled);
        if (autodialEnabled) {
          tryAgain = nsNativeConnectionHelper::OnConnectionFailed(
                       NS_ConvertUTF8toUTF16(SocketHost()).get());
	    }
    }
#endif

    
    if (tryAgain) {
        nsresult rv;
        PRUint32 msg;

        if (mState == STATE_CONNECTING) {
            mState = STATE_RESOLVING;
            msg = MSG_DNS_LOOKUP_COMPLETE;
        }
        else {
            mState = STATE_CLOSED;
            msg = MSG_ENSURE_CONNECT;
        }

        rv = PostEvent(msg, NS_OK);
        if (NS_FAILED(rv))
            tryAgain = PR_FALSE;
    }

    return tryAgain;
}


void
nsSocketTransport::OnMsgInputClosed(nsresult reason)
{
    LOG(("nsSocketTransport::OnMsgInputClosed [this=%x reason=%x]\n",
        this, reason));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    mInputClosed = PR_TRUE;
    
    if (NS_FAILED(reason) && (reason != NS_BASE_STREAM_CLOSED))
        mCondition = reason;                
    else if (mOutputClosed)
        mCondition = NS_BASE_STREAM_CLOSED; 
    else {
        if (mState == STATE_TRANSFERRING)
            mPollFlags &= ~PR_POLL_READ;
        mInput.OnSocketReady(reason);
    }
}


void
nsSocketTransport::OnMsgOutputClosed(nsresult reason)
{
    LOG(("nsSocketTransport::OnMsgOutputClosed [this=%x reason=%x]\n",
        this, reason));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    mOutputClosed = PR_TRUE;
    
    if (NS_FAILED(reason) && (reason != NS_BASE_STREAM_CLOSED))
        mCondition = reason;                
    else if (mInputClosed)
        mCondition = NS_BASE_STREAM_CLOSED; 
    else {
        if (mState == STATE_TRANSFERRING)
            mPollFlags &= ~PR_POLL_WRITE;
        mOutput.OnSocketReady(reason);
    }
}

void
nsSocketTransport::OnSocketConnected()
{
    LOG(("  advancing to STATE_TRANSFERRING\n"));

    mPollFlags = (PR_POLL_READ | PR_POLL_WRITE | PR_POLL_EXCEPT);
    mPollTimeout = mTimeouts[TIMEOUT_READ_WRITE];
    mState = STATE_TRANSFERRING;

    
    
    {
        nsAutoLock lock(mLock);
        NS_ASSERTION(mFD, "no socket");
        NS_ASSERTION(mFDref == 1, "wrong socket ref count");
        mFDconnected = PR_TRUE;
    }

    SendStatus(STATUS_CONNECTED_TO);
}

PRFileDesc *
nsSocketTransport::GetFD_Locked()
{
    
    if (!mFDconnected)
        return nsnull;

    if (mFD)
        mFDref++;

    return mFD;
}

void
nsSocketTransport::ReleaseFD_Locked(PRFileDesc *fd)
{
    NS_ASSERTION(mFD == fd, "wrong fd");

    if (--mFDref == 0) {
        LOG(("nsSocketTransport: calling PR_Close [this=%x]\n", this));
        PR_Close(mFD);
        mFD = nsnull;
    }
}




void
nsSocketTransport::OnSocketEvent(PRUint32 type, nsresult status, nsISupports *param)
{
    LOG(("nsSocketTransport::OnSocketEvent [this=%p type=%u status=%x param=%p]\n",
        this, type, status, param));

    if (NS_FAILED(mCondition)) {
        
        LOG(("  blocking event [condition=%x]\n", mCondition));
        
        
        
        mInput.OnSocketReady(mCondition);
        mOutput.OnSocketReady(mCondition);
        return;
    }

    switch (type) {
    case MSG_ENSURE_CONNECT:
        LOG(("  MSG_ENSURE_CONNECT\n"));
        
        
        
        
        if (mState == STATE_CLOSED)
            mCondition = ResolveHost();
        else
            LOG(("  ignoring redundant event\n"));
        break;

    case MSG_DNS_LOOKUP_COMPLETE:
        LOG(("  MSG_DNS_LOOKUP_COMPLETE\n"));
        mDNSRequest = 0;
        if (param) {
            mDNSRecord = static_cast<nsIDNSRecord *>(param);
            mDNSRecord->GetNextAddr(SocketPort(), &mNetAddr);
        }
        
        if (NS_FAILED(status)) {
            
            
            
            
            
            if ((status == NS_ERROR_UNKNOWN_HOST) && !mProxyTransparent &&
                !mProxyHost.IsEmpty())
                mCondition = NS_ERROR_UNKNOWN_PROXY_HOST;
            else
                mCondition = status;
        }
        else if (mState == STATE_RESOLVING)
            mCondition = InitiateSocket();
        break;

    case MSG_RETRY_INIT_SOCKET:
        mCondition = InitiateSocket();
        break;

    case MSG_INPUT_CLOSED:
        LOG(("  MSG_INPUT_CLOSED\n"));
        OnMsgInputClosed(status);
        break;

    case MSG_INPUT_PENDING:
        LOG(("  MSG_INPUT_PENDING\n"));
        OnMsgInputPending();
        break;

    case MSG_OUTPUT_CLOSED:
        LOG(("  MSG_OUTPUT_CLOSED\n"));
        OnMsgOutputClosed(status);
        break;

    case MSG_OUTPUT_PENDING:
        LOG(("  MSG_OUTPUT_PENDING\n"));
        OnMsgOutputPending();
        break;
    case MSG_TIMEOUT_CHANGED:
        LOG(("  MSG_TIMEOUT_CHANGED\n"));
        mPollTimeout = mTimeouts[(mState == STATE_TRANSFERRING)
          ? TIMEOUT_READ_WRITE : TIMEOUT_CONNECT];
        break;
    default:
        LOG(("  unhandled event!\n"));
    }
    
    if (NS_FAILED(mCondition)) {
        LOG(("  after event [this=%x cond=%x]\n", this, mCondition));
        if (!mAttached) 
            OnSocketDetached(nsnull);
    }
    else if (mPollFlags == PR_POLL_EXCEPT)
        mPollFlags = 0; 
}




void
nsSocketTransport::OnSocketReady(PRFileDesc *fd, PRInt16 outFlags)
{
    LOG(("nsSocketTransport::OnSocketReady [this=%x outFlags=%hd]\n",
        this, outFlags));

    if (outFlags == -1) {
        LOG(("socket timeout expired\n"));
        mCondition = NS_ERROR_NET_TIMEOUT;
        return;
    }

    if (mState == STATE_TRANSFERRING) {
        
        if ((mPollFlags & PR_POLL_WRITE) && (outFlags & ~PR_POLL_READ)) {
            
            
            mPollFlags &= ~PR_POLL_WRITE;
            mOutput.OnSocketReady(NS_OK);
        }
        
        if ((mPollFlags & PR_POLL_READ) && (outFlags & ~PR_POLL_WRITE)) {
            
            
            mPollFlags &= ~PR_POLL_READ;
            mInput.OnSocketReady(NS_OK);
        }
        
        mPollTimeout = mTimeouts[TIMEOUT_READ_WRITE];
    }
    else if (mState == STATE_CONNECTING) {
        PRStatus status = PR_ConnectContinue(fd, outFlags);
        if (status == PR_SUCCESS) {
            
            
            
            OnSocketConnected();
        }
        else {
            PRErrorCode code = PR_GetError();
#if defined(TEST_CONNECT_ERRORS)
            code = RandomizeConnectError(code);
#endif
            
            
            
            if ((PR_WOULD_BLOCK_ERROR == code) || (PR_IN_PROGRESS_ERROR == code)) {
                
                mPollFlags = (PR_POLL_EXCEPT | PR_POLL_WRITE);
                
                mPollTimeout = mTimeouts[TIMEOUT_CONNECT];
            } 
            else {
                
                
                
                mCondition = ErrorAccordingToNSPR(code);
                if ((mCondition == NS_ERROR_CONNECTION_REFUSED) && !mProxyHost.IsEmpty())
                    mCondition = NS_ERROR_PROXY_CONNECTION_REFUSED;
                LOG(("  connection failed! [reason=%x]\n", mCondition));
            }
        }
    }
    else {
        NS_ERROR("unexpected socket state");
        mCondition = NS_ERROR_UNEXPECTED;
    }

    if (mPollFlags == PR_POLL_EXCEPT)
        mPollFlags = 0; 
}


void
nsSocketTransport::OnSocketDetached(PRFileDesc *fd)
{
    LOG(("nsSocketTransport::OnSocketDetached [this=%x cond=%x]\n",
        this, mCondition));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    
    
    if (NS_SUCCEEDED(mCondition))
        mCondition = NS_ERROR_ABORT;

    if (RecoverFromError())
        mCondition = NS_OK;
    else {
        mState = STATE_CLOSED;

        
        if (mDNSRequest) {
            mDNSRequest->Cancel(NS_ERROR_ABORT);
            mDNSRequest = 0;
        }

        
        
        
        mInput.OnSocketReady(mCondition);
        mOutput.OnSocketReady(mCondition);
    }

    
    
    
    nsCOMPtr<nsISSLSocketControl> secCtrl = do_QueryInterface(mSecInfo);
    if (secCtrl)
        secCtrl->SetNotificationCallbacks(nsnull);

    
    
    
    {
        nsAutoLock lock(mLock);
        if (mFD) {
            ReleaseFD_Locked(mFD);
            
            
            mFDconnected = PR_FALSE;
        }

        
        
        
        
        if (NS_FAILED(mCondition)) {
            mCallbacks = nsnull;
            mEventSink = nsnull;
        }
    }
}




NS_IMPL_THREADSAFE_ISUPPORTS4(nsSocketTransport,
                              nsISocketTransport,
                              nsITransport,
                              nsIDNSListener,
                              nsIClassInfo)
NS_IMPL_CI_INTERFACE_GETTER3(nsSocketTransport,
                             nsISocketTransport,
                             nsITransport,
                             nsIDNSListener)

NS_IMETHODIMP
nsSocketTransport::OpenInputStream(PRUint32 flags,
                                   PRUint32 segsize,
                                   PRUint32 segcount,
                                   nsIInputStream **result)
{
    LOG(("nsSocketTransport::OpenInputStream [this=%x flags=%x]\n",
        this, flags));

    NS_ENSURE_TRUE(!mInput.IsReferenced(), NS_ERROR_UNEXPECTED);

    nsresult rv;
    nsCOMPtr<nsIAsyncInputStream> pipeIn;

    if (!(flags & OPEN_UNBUFFERED) || (flags & OPEN_BLOCKING)) {
        
        
        PRBool openBlocking =  (flags & OPEN_BLOCKING);

        net_ResolveSegmentParams(segsize, segcount);
        nsIMemory *segalloc = net_GetSegmentAlloc(segsize);

        
        nsCOMPtr<nsIAsyncOutputStream> pipeOut;
        rv = NS_NewPipe2(getter_AddRefs(pipeIn), getter_AddRefs(pipeOut),
                         !openBlocking, PR_TRUE, segsize, segcount, segalloc);
        if (NS_FAILED(rv)) return rv;

        
        rv = NS_AsyncCopy(&mInput, pipeOut, gSocketTransportService,
                          NS_ASYNCCOPY_VIA_WRITESEGMENTS, segsize);
        if (NS_FAILED(rv)) return rv;

        *result = pipeIn;
    }
    else
        *result = &mInput;

    
    mInputClosed = PR_FALSE;

    rv = PostEvent(MSG_ENSURE_CONNECT);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*result);
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::OpenOutputStream(PRUint32 flags,
                                    PRUint32 segsize,
                                    PRUint32 segcount,
                                    nsIOutputStream **result)
{
    LOG(("nsSocketTransport::OpenOutputStream [this=%x flags=%x]\n",
        this, flags));

    NS_ENSURE_TRUE(!mOutput.IsReferenced(), NS_ERROR_UNEXPECTED);

    nsresult rv;
    nsCOMPtr<nsIAsyncOutputStream> pipeOut;
    if (!(flags & OPEN_UNBUFFERED) || (flags & OPEN_BLOCKING)) {
        
        
        PRBool openBlocking =  (flags & OPEN_BLOCKING);

        net_ResolveSegmentParams(segsize, segcount);
        nsIMemory *segalloc = net_GetSegmentAlloc(segsize);

        
        nsCOMPtr<nsIAsyncInputStream> pipeIn;
        rv = NS_NewPipe2(getter_AddRefs(pipeIn), getter_AddRefs(pipeOut),
                         PR_TRUE, !openBlocking, segsize, segcount, segalloc);
        if (NS_FAILED(rv)) return rv;

        
        rv = NS_AsyncCopy(pipeIn, &mOutput, gSocketTransportService,
                          NS_ASYNCCOPY_VIA_READSEGMENTS, segsize);
        if (NS_FAILED(rv)) return rv;

        *result = pipeOut;
    }
    else
        *result = &mOutput;

    
    mOutputClosed = PR_FALSE;

    rv = PostEvent(MSG_ENSURE_CONNECT);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*result);
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::Close(nsresult reason)
{
    if (NS_SUCCEEDED(reason))
        reason = NS_BASE_STREAM_CLOSED;

    mInput.CloseWithStatus(reason);
    mOutput.CloseWithStatus(reason);
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetSecurityInfo(nsISupports **secinfo)
{
    nsAutoLock lock(mLock);
    NS_IF_ADDREF(*secinfo = mSecInfo);
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetSecurityCallbacks(nsIInterfaceRequestor **callbacks)
{
    nsAutoLock lock(mLock);
    NS_IF_ADDREF(*callbacks = mCallbacks);
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::SetSecurityCallbacks(nsIInterfaceRequestor *callbacks)
{
    nsAutoLock lock(mLock);
    mCallbacks = callbacks;
    
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::SetEventSink(nsITransportEventSink *sink,
                                nsIEventTarget *target)
{
    nsCOMPtr<nsITransportEventSink> temp;
    if (target) {
        nsresult rv = net_NewTransportEventSinkProxy(getter_AddRefs(temp),
                                                     sink, target);
        if (NS_FAILED(rv))
            return rv;
        sink = temp.get();
    }

    nsAutoLock lock(mLock);
    mEventSink = sink;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::IsAlive(PRBool *result)
{
    *result = PR_FALSE;

    PRFileDesc *fd;
    {
        nsAutoLock lock(mLock);
        if (NS_FAILED(mCondition))
            return NS_OK;
        fd = GetFD_Locked();
        if (!fd)
            return NS_OK;
    }

    

    char c;
    PRInt32 rval = PR_Recv(fd, &c, 1, PR_MSG_PEEK, 0);

    if ((rval > 0) || (rval < 0 && PR_GetError() == PR_WOULD_BLOCK_ERROR))
        *result = PR_TRUE;

    {
        nsAutoLock lock(mLock);
        ReleaseFD_Locked(fd);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetHost(nsACString &host)
{
    host = SocketHost();
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetPort(PRInt32 *port)
{
    *port = (PRInt32) SocketPort();
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetPeerAddr(PRNetAddr *addr)
{
    
    
    
    

    NS_ENSURE_TRUE(mState == STATE_TRANSFERRING, NS_ERROR_NOT_AVAILABLE);

    memcpy(addr, &mNetAddr, sizeof(mNetAddr));
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetSelfAddr(PRNetAddr *addr)
{
    
    
    

    PRFileDesc *fd;
    {
        nsAutoLock lock(mLock);
        fd = GetFD_Locked();
    }

    if (!fd)
        return NS_ERROR_NOT_CONNECTED;

    nsresult rv =
        (PR_GetSockName(fd, addr) == PR_SUCCESS) ? NS_OK : NS_ERROR_FAILURE;

    {
        nsAutoLock lock(mLock);
        ReleaseFD_Locked(fd);
    }

    return rv;
}

NS_IMETHODIMP
nsSocketTransport::GetTimeout(PRUint32 type, PRUint32 *value)
{
    NS_ENSURE_ARG_MAX(type, nsISocketTransport::TIMEOUT_READ_WRITE);
    *value = (PRUint32) mTimeouts[type];
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::SetTimeout(PRUint32 type, PRUint32 value)
{
    NS_ENSURE_ARG_MAX(type, nsISocketTransport::TIMEOUT_READ_WRITE);
    
    mTimeouts[type] = (PRUint16) PR_MIN(value, PR_UINT16_MAX);
    PostEvent(MSG_TIMEOUT_CHANGED);
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::OnLookupComplete(nsICancelable *request,
                                    nsIDNSRecord  *rec,
                                    nsresult       status)
{
    
    mResolving = PR_FALSE;

    nsresult rv = PostEvent(MSG_DNS_LOOKUP_COMPLETE, status, rec);

    
    
    
    
    if (NS_FAILED(rv))
        NS_WARNING("unable to post DNS lookup complete message");

    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetInterfaces(PRUint32 *count, nsIID * **array)
{
    return NS_CI_INTERFACE_GETTER_NAME(nsSocketTransport)(count, array);
}

NS_IMETHODIMP
nsSocketTransport::GetHelperForLanguage(PRUint32 language, nsISupports **_retval)
{
    *_retval = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetContractID(char * *aContractID)
{
    *aContractID = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetClassDescription(char * *aClassDescription)
{
    *aClassDescription = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetClassID(nsCID * *aClassID)
{
    *aClassID = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetFlags(PRUint32 *aFlags)
{
    *aFlags = nsIClassInfo::THREADSAFE;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    return NS_ERROR_NOT_AVAILABLE;
}


NS_IMETHODIMP
nsSocketTransport::GetConnectionFlags(PRUint32 *value)
{
    *value = mConnectionFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::SetConnectionFlags(PRUint32 value)
{
    mConnectionFlags = value;
    return NS_OK;
}


#ifdef ENABLE_SOCKET_TRACING

#include <stdio.h>
#include <ctype.h>
#include "prenv.h"

static void
DumpBytesToFile(const char *path, const char *header, const char *buf, PRInt32 n)
{
    FILE *fp = fopen(path, "a");

    fprintf(fp, "\n%s [%d bytes]\n", header, n);

    const unsigned char *p;
    while (n) {
        p = (const unsigned char *) buf;

        PRInt32 i, row_max = PR_MIN(16, n);

        for (i = 0; i < row_max; ++i)
            fprintf(fp, "%02x  ", *p++);
        for (i = row_max; i < 16; ++i)
            fprintf(fp, "    ");

        p = (const unsigned char *) buf;
        for (i = 0; i < row_max; ++i, ++p) {
            if (isprint(*p))
                fprintf(fp, "%c", *p);
            else
                fprintf(fp, ".");
        }

        fprintf(fp, "\n");
        buf += row_max;
        n -= row_max;
    }

    fprintf(fp, "\n");
    fclose(fp);
}

void
nsSocketTransport::TraceInBuf(const char *buf, PRInt32 n)
{
    char *val = PR_GetEnv("NECKO_SOCKET_TRACE_LOG");
    if (!val || !*val)
        return;

    nsCAutoString header;
    header.Assign(NS_LITERAL_CSTRING("Reading from: ") + mHost);
    header.Append(':');
    header.AppendInt(mPort);

    DumpBytesToFile(val, header.get(), buf, n);
}

void
nsSocketTransport::TraceOutBuf(const char *buf, PRInt32 n)
{
    char *val = PR_GetEnv("NECKO_SOCKET_TRACE_LOG");
    if (!val || !*val)
        return;

    nsCAutoString header;
    header.Assign(NS_LITERAL_CSTRING("Writing to: ") + mHost);
    header.Append(':');
    header.AppendInt(mPort);

    DumpBytesToFile(val, header.get(), buf, n);
}

#endif
