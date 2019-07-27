





#include "nsSocketTransport2.h"

#include "mozilla/Attributes.h"
#include "nsIOService.h"
#include "nsStreamUtils.h"
#include "nsNetSegmentUtils.h"
#include "nsNetAddr.h"
#include "nsTransportUtils.h"
#include "nsProxyInfo.h"
#include "nsNetCID.h"
#include "nsNetUtil.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "plstr.h"
#include "prerr.h"
#include "NetworkActivityMonitor.h"
#include "NSSErrorsService.h"
#include "mozilla/net/NeckoChild.h"
#include "mozilla/VisualEventTracer.h"
#include "nsThreadUtils.h"
#include "nsISocketProviderService.h"
#include "nsISocketProvider.h"
#include "nsISSLSocketControl.h"
#include "nsIPipe.h"
#include "nsIClassInfoImpl.h"
#include "nsURLHelper.h"
#include "nsIDNSService.h"
#include "nsIDNSRecord.h"
#include "nsICancelable.h"
#include <algorithm>

#include "nsPrintfCString.h"

#if defined(XP_WIN)
#include "nsNativeConnectionHelper.h"
#endif


#include "private/pprio.h"
#if defined(XP_WIN)
#include <winsock2.h>
#include <mstcpip.h>
#elif defined(XP_UNIX)
#include <errno.h>
#include <netinet/tcp.h>
#endif


using namespace mozilla;
using namespace mozilla::net;



static NS_DEFINE_CID(kSocketProviderServiceCID, NS_SOCKETPROVIDERSERVICE_CID);
static NS_DEFINE_CID(kDNSServiceCID, NS_DNSSERVICE_CID);



class nsSocketEvent : public nsRunnable
{
public:
    nsSocketEvent(nsSocketTransport *transport, uint32_t type,
                  nsresult status = NS_OK, nsISupports *param = nullptr)
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

    uint32_t              mType;
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
        SOCKET_LOG(("simulating NSPR error %d [%s]\n", code, errors[n].err_name));
    }
    return code;
}
#endif



nsresult
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
    
    
    case PR_NO_ACCESS_RIGHTS_ERROR:
        rv = NS_ERROR_CONNECTION_REFUSED;
        break;
    case PR_ADDRESS_NOT_SUPPORTED_ERROR:
        rv = NS_ERROR_SOCKET_ADDRESS_NOT_SUPPORTED;
        break;
    case PR_IO_TIMEOUT_ERROR:
    case PR_CONNECT_TIMEOUT_ERROR:
        rv = NS_ERROR_NET_TIMEOUT;
        break;
    case PR_OUT_OF_MEMORY_ERROR:
    
    
    
    case PR_PROC_DESC_TABLE_FULL_ERROR:
    case PR_SYS_DESC_TABLE_FULL_ERROR:
    case PR_INSUFFICIENT_RESOURCES_ERROR:
        rv = NS_ERROR_OUT_OF_MEMORY;
        break;
    case PR_ADDRESS_IN_USE_ERROR:
        rv = NS_ERROR_SOCKET_ADDRESS_IN_USE;
        break;
    
    case PR_FILE_NOT_FOUND_ERROR:
        rv = NS_ERROR_FILE_NOT_FOUND;
        break;
    case PR_IS_DIRECTORY_ERROR:
        rv = NS_ERROR_FILE_IS_DIRECTORY;
        break;
    case PR_LOOP_ERROR:
        rv = NS_ERROR_FILE_UNRESOLVABLE_SYMLINK;
        break;
    case PR_NAME_TOO_LONG_ERROR:
        rv = NS_ERROR_FILE_NAME_TOO_LONG;
        break;
    case PR_NO_DEVICE_SPACE_ERROR:
        rv = NS_ERROR_FILE_NO_DEVICE_SPACE;
        break;
    case PR_NOT_DIRECTORY_ERROR:
        rv = NS_ERROR_FILE_NOT_DIRECTORY;
        break;
    case PR_READ_ONLY_FILESYSTEM_ERROR:
        rv = NS_ERROR_FILE_READ_ONLY;
        break;
    default:
        if (mozilla::psm::IsNSSErrorCode(errorCode)) {
            rv = mozilla::psm::GetXPCOMFromNSSError(errorCode);
        }
        break;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    }
    SOCKET_LOG(("ErrorAccordingToNSPR [in=%d out=%x]\n", errorCode, rv));
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
    SOCKET_LOG(("nsSocketInputStream::OnSocketReady [this=%p cond=%x]\n",
        this, condition));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    nsCOMPtr<nsIInputStreamCallback> callback;
    {
        MutexAutoLock lock(mTransport->mLock);

        
        
        if (NS_SUCCEEDED(mCondition))
            mCondition = condition;

        
        if (NS_FAILED(mCondition) || !(mCallbackFlags & WAIT_CLOSURE_ONLY)) {
            callback = mCallback;
            mCallback = nullptr;
            mCallbackFlags = 0;
        }
    }

    if (callback)
        callback->OnInputStreamReady(this);
}

NS_IMPL_QUERY_INTERFACE(nsSocketInputStream,
                        nsIInputStream,
                        nsIAsyncInputStream)

NS_IMETHODIMP_(MozExternalRefCountType)
nsSocketInputStream::AddRef()
{
    ++mReaderRefCnt;
    return mTransport->AddRef();
}

NS_IMETHODIMP_(MozExternalRefCountType)
nsSocketInputStream::Release()
{
    if (--mReaderRefCnt == 0)
        Close();
    return mTransport->Release();
}

NS_IMETHODIMP
nsSocketInputStream::Close()
{
    return CloseWithStatus(NS_BASE_STREAM_CLOSED);
}

NS_IMETHODIMP
nsSocketInputStream::Available(uint64_t *avail)
{
    SOCKET_LOG(("nsSocketInputStream::Available [this=%p]\n", this));

    *avail = 0;

    PRFileDesc *fd;
    {
        MutexAutoLock lock(mTransport->mLock);

        if (NS_FAILED(mCondition))
            return mCondition;

        fd = mTransport->GetFD_Locked();
        if (!fd)
            return NS_OK;
    }

    
    
    
    int32_t n = PR_Available(fd);

    
    
    if ((n == -1) && (PR_GetError() == PR_NOT_IMPLEMENTED_ERROR)) {
        char c;

        n = PR_Recv(fd, &c, 1, PR_MSG_PEEK, 0);
        SOCKET_LOG(("nsSocketInputStream::Available [this=%p] "
                    "using PEEK backup n=%d]\n", this, n));
    }

    nsresult rv;
    {
        MutexAutoLock lock(mTransport->mLock);

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
nsSocketInputStream::Read(char *buf, uint32_t count, uint32_t *countRead)
{
    SOCKET_LOG(("nsSocketInputStream::Read [this=%p count=%u]\n", this, count));

    *countRead = 0;

    PRFileDesc* fd = nullptr;
    {
        MutexAutoLock lock(mTransport->mLock);

        if (NS_FAILED(mCondition))
            return (mCondition == NS_BASE_STREAM_CLOSED) ? NS_OK : mCondition;

        fd = mTransport->GetFD_Locked();
        if (!fd)
            return NS_BASE_STREAM_WOULD_BLOCK;
    }

    SOCKET_LOG(("  calling PR_Read [count=%u]\n", count));

    
    
    
    int32_t n = PR_Read(fd, buf, count);

    SOCKET_LOG(("  PR_Read returned [n=%d]\n", n));

    nsresult rv = NS_OK;
    {
        MutexAutoLock lock(mTransport->mLock);

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
        mTransport->SendStatus(NS_NET_STATUS_RECEIVING_FROM);
    return rv;
}

NS_IMETHODIMP
nsSocketInputStream::ReadSegments(nsWriteSegmentFun writer, void *closure,
                                  uint32_t count, uint32_t *countRead)
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsSocketInputStream::IsNonBlocking(bool *nonblocking)
{
    *nonblocking = true;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketInputStream::CloseWithStatus(nsresult reason)
{
    SOCKET_LOG(("nsSocketInputStream::CloseWithStatus [this=%p reason=%x]\n", this, reason));

    
 
    nsresult rv;
    {
        MutexAutoLock lock(mTransport->mLock);

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
                               uint32_t flags,
                               uint32_t amount,
                               nsIEventTarget *target)
{
    SOCKET_LOG(("nsSocketInputStream::AsyncWait [this=%p]\n", this));

    bool hasError = false;
    {
        MutexAutoLock lock(mTransport->mLock);

        if (callback && target) {
            
            
            
            mCallback = NS_NewInputStreamReadyEvent(callback, target);
        }
        else
            mCallback = callback;
        mCallbackFlags = flags;

        hasError = NS_FAILED(mCondition);
    } 

    if (hasError) {
        
        
        
        
        mTransport->PostEvent(nsSocketTransport::MSG_INPUT_PENDING);
    } else {
        mTransport->OnInputPending();
    }

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
    SOCKET_LOG(("nsSocketOutputStream::OnSocketReady [this=%p cond=%x]\n",
        this, condition));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    nsCOMPtr<nsIOutputStreamCallback> callback;
    {
        MutexAutoLock lock(mTransport->mLock);

        
        
        if (NS_SUCCEEDED(mCondition))
            mCondition = condition;

        
        if (NS_FAILED(mCondition) || !(mCallbackFlags & WAIT_CLOSURE_ONLY)) {
            callback = mCallback;
            mCallback = nullptr;
            mCallbackFlags = 0;
        }
    }

    if (callback)
        callback->OnOutputStreamReady(this);
}

NS_IMPL_QUERY_INTERFACE(nsSocketOutputStream,
                        nsIOutputStream,
                        nsIAsyncOutputStream)

NS_IMETHODIMP_(MozExternalRefCountType)
nsSocketOutputStream::AddRef()
{
    ++mWriterRefCnt;
    return mTransport->AddRef();
}

NS_IMETHODIMP_(MozExternalRefCountType)
nsSocketOutputStream::Release()
{
    if (--mWriterRefCnt == 0)
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
nsSocketOutputStream::Write(const char *buf, uint32_t count, uint32_t *countWritten)
{
    SOCKET_LOG(("nsSocketOutputStream::Write [this=%p count=%u]\n", this, count));

    *countWritten = 0;

    
    

    PRFileDesc* fd = nullptr;
    {
        MutexAutoLock lock(mTransport->mLock);

        if (NS_FAILED(mCondition))
            return mCondition;
        
        fd = mTransport->GetFD_Locked();
        if (!fd)
            return NS_BASE_STREAM_WOULD_BLOCK;
    }

    SOCKET_LOG(("  calling PR_Write [count=%u]\n", count));

    
    
    
    int32_t n = PR_Write(fd, buf, count);

    SOCKET_LOG(("  PR_Write returned [n=%d]\n", n));

    nsresult rv = NS_OK;
    {
        MutexAutoLock lock(mTransport->mLock);

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
        mTransport->SendStatus(NS_NET_STATUS_SENDING_TO);
    return rv;
}

NS_IMETHODIMP
nsSocketOutputStream::WriteSegments(nsReadSegmentFun reader, void *closure,
                                    uint32_t count, uint32_t *countRead)
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD
nsSocketOutputStream::WriteFromSegments(nsIInputStream *input,
                                        void *closure,
                                        const char *fromSegment,
                                        uint32_t offset,
                                        uint32_t count,
                                        uint32_t *countRead)
{
    nsSocketOutputStream *self = (nsSocketOutputStream *) closure;
    return self->Write(fromSegment, count, countRead);
}

NS_IMETHODIMP
nsSocketOutputStream::WriteFrom(nsIInputStream *stream, uint32_t count, uint32_t *countRead)
{
    return stream->ReadSegments(WriteFromSegments, this, count, countRead);
}

NS_IMETHODIMP
nsSocketOutputStream::IsNonBlocking(bool *nonblocking)
{
    *nonblocking = true;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketOutputStream::CloseWithStatus(nsresult reason)
{
    SOCKET_LOG(("nsSocketOutputStream::CloseWithStatus [this=%p reason=%x]\n", this, reason));

    
 
    nsresult rv;
    {
        MutexAutoLock lock(mTransport->mLock);

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
                                uint32_t flags,
                                uint32_t amount,
                                nsIEventTarget *target)
{
    SOCKET_LOG(("nsSocketOutputStream::AsyncWait [this=%p]\n", this));

    {
        MutexAutoLock lock(mTransport->mLock);

        if (callback && target) {
            
            
            
            mCallback = NS_NewOutputStreamReadyEvent(callback, target);
        }
        else
            mCallback = callback;

        mCallbackFlags = flags;
    }
    mTransport->OnOutputPending();
    return NS_OK;
}





nsSocketTransport::nsSocketTransport()
    : mTypes(nullptr)
    , mTypeCount(0)
    , mPort(0)
    , mProxyPort(0)
    , mProxyTransparent(false)
    , mProxyTransparentResolvesHost(false)
    , mHttpsProxy(false)
    , mConnectionFlags(0)
    , mState(STATE_CLOSED)
    , mAttached(false)
    , mInputClosed(true)
    , mOutputClosed(true)
    , mResolving(false)
    , mNetAddrIsSet(false)
    , mLock("nsSocketTransport.mLock")
    , mFD(this)
    , mFDref(0)
    , mFDconnected(false)
    , mSocketTransportService(gSocketTransportService)
    , mInput(this)
    , mOutput(this)
    , mQoSBits(0x00)
    , mKeepaliveEnabled(false)
    , mKeepaliveIdleTimeS(-1)
    , mKeepaliveRetryIntervalS(-1)
    , mKeepaliveProbeCount(-1)
{
    SOCKET_LOG(("creating nsSocketTransport @%p\n", this));

    mTimeouts[TIMEOUT_CONNECT]    = UINT16_MAX; 
    mTimeouts[TIMEOUT_READ_WRITE] = UINT16_MAX; 
}

nsSocketTransport::~nsSocketTransport()
{
    SOCKET_LOG(("destroying nsSocketTransport @%p\n", this));

    CleanupTypes();
}

void
nsSocketTransport::CleanupTypes()
{
    
    if (mTypes) {
        for (uint32_t i = 0; i < mTypeCount; ++i) {
            PL_strfree(mTypes[i]);
        }
        free(mTypes);
        mTypes = nullptr;
    }
    mTypeCount = 0;
}

nsresult
nsSocketTransport::Init(const char **types, uint32_t typeCount,
                        const nsACString &host, uint16_t port,
                        nsIProxyInfo *givenProxyInfo)
{
    MOZ_EVENT_TRACER_NAME_OBJECT(this, host.BeginReading());

    nsCOMPtr<nsProxyInfo> proxyInfo;
    if (givenProxyInfo) {
        proxyInfo = do_QueryInterface(givenProxyInfo);
        NS_ENSURE_ARG(proxyInfo);
    }

    

    mPort = port;
    mHost = host;

    if (proxyInfo) {
        mHttpsProxy = proxyInfo->IsHTTPS();
    }

    const char *proxyType = nullptr;
    if (proxyInfo) {
        mProxyPort = proxyInfo->Port();
        mProxyHost = proxyInfo->Host();
        
        proxyType = proxyInfo->Type();
        if (proxyType && (proxyInfo->IsHTTP() ||
                          proxyInfo->IsHTTPS() ||
                          proxyInfo->IsDirect() ||
                          !strcmp(proxyType, "unknown"))) {
            proxyType = nullptr;
        }
    }

    SOCKET_LOG(("nsSocketTransport::Init [this=%p host=%s:%hu proxy=%s:%hu]\n",
        this, mHost.get(), mPort, mProxyHost.get(), mProxyPort));

    
    mTypeCount = typeCount + (proxyType != nullptr);
    if (!mTypeCount)
        return NS_OK;

    
    
    nsresult rv;
    nsCOMPtr<nsISocketProviderService> spserv =
        do_GetService(kSocketProviderServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    mTypes = (char **) malloc(mTypeCount * sizeof(char *));
    if (!mTypes)
        return NS_ERROR_OUT_OF_MEMORY;

    
    for (uint32_t i = 0, type = 0; i < mTypeCount; ++i) {
        
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
            mProxyTransparent = true;

            if (proxyInfo->Flags() & nsIProxyInfo::TRANSPARENT_PROXY_RESOLVES_HOST) {
                
                
                mProxyTransparentResolvesHost = true;
            }
        }
    }

    return NS_OK;
}

nsresult
nsSocketTransport::InitWithFilename(const char *filename)
{
#if defined(XP_UNIX)
    size_t filenameLength = strlen(filename);

    if (filenameLength > sizeof(mNetAddr.local.path) - 1)
        return NS_ERROR_FILE_NAME_TOO_LONG;

    mHost.Assign(filename);
    mPort = 0;
    mTypeCount = 0;

    mNetAddr.local.family = AF_LOCAL;
    memcpy(mNetAddr.local.path, filename, filenameLength);
    mNetAddr.local.path[filenameLength] = '\0';
    mNetAddrIsSet = true;

    return NS_OK;
#else
    return NS_ERROR_SOCKET_ADDRESS_NOT_SUPPORTED;
#endif
}

nsresult
nsSocketTransport::InitWithConnectedSocket(PRFileDesc *fd, const NetAddr *addr)
{
    NS_ASSERTION(!mFD.IsInitialized(), "already initialized");

    char buf[kNetAddrMaxCStrBufSize];
    NetAddrToString(addr, buf, sizeof(buf));
    mHost.Assign(buf);

    uint16_t port;
    if (addr->raw.family == AF_INET)
        port = addr->inet.port;
    else if (addr->raw.family == AF_INET6)
        port = addr->inet6.port;
    else
        port = 0;
    mPort = ntohs(port);

    memcpy(&mNetAddr, addr, sizeof(NetAddr));

    mPollFlags = (PR_POLL_READ | PR_POLL_WRITE | PR_POLL_EXCEPT);
    mPollTimeout = mTimeouts[TIMEOUT_READ_WRITE];
    mState = STATE_TRANSFERRING;
    mNetAddrIsSet = true;

    {
        MutexAutoLock lock(mLock);

        mFD = fd;
        mFDref = 1;
        mFDconnected = 1;
    }

    
    PRSocketOptionData opt;
    opt.option = PR_SockOpt_Nonblocking;
    opt.value.non_blocking = true;
    PR_SetSocketOption(fd, &opt);

    SOCKET_LOG(("nsSocketTransport::InitWithConnectedSocket [this=%p addr=%s:%hu]\n",
        this, mHost.get(), mPort));

    
    return PostEvent(MSG_RETRY_INIT_SOCKET);
}

nsresult
nsSocketTransport::InitWithConnectedSocket(PRFileDesc* aFD,
                                           const NetAddr* aAddr,
                                           nsISupports* aSecInfo)
{
    mSecInfo = aSecInfo;
    return InitWithConnectedSocket(aFD, aAddr);
}

nsresult
nsSocketTransport::PostEvent(uint32_t type, nsresult status, nsISupports *param)
{
    SOCKET_LOG(("nsSocketTransport::PostEvent [this=%p type=%u status=%x param=%p]\n",
        this, type, status, param));

    nsCOMPtr<nsIRunnable> event = new nsSocketEvent(this, type, status, param);
    if (!event)
        return NS_ERROR_OUT_OF_MEMORY;

    return mSocketTransportService->Dispatch(event, NS_DISPATCH_NORMAL);
}

void
nsSocketTransport::SendStatus(nsresult status)
{
    SOCKET_LOG(("nsSocketTransport::SendStatus [this=%p status=%x]\n", this, status));

    nsCOMPtr<nsITransportEventSink> sink;
    uint64_t progress;
    {
        MutexAutoLock lock(mLock);
        sink = mEventSink;
        switch (status) {
        case NS_NET_STATUS_SENDING_TO:
            progress = mOutput.ByteCount();
            break;
        case NS_NET_STATUS_RECEIVING_FROM:
            progress = mInput.ByteCount();
            break;
        default:
            progress = 0;
            break;
        }
    }
    if (sink) {
        sink->OnTransportStatus(this, status, progress, -1);
    }
}

nsresult
nsSocketTransport::ResolveHost()
{
    SOCKET_LOG(("nsSocketTransport::ResolveHost [this=%p %s:%d%s]\n",
                this, SocketHost().get(), SocketPort(),
                mConnectionFlags & nsSocketTransport::BYPASS_CACHE ?
                " bypass cache" : ""));

    nsresult rv;

    if (!mProxyHost.IsEmpty()) {
        if (!mProxyTransparent || mProxyTransparentResolvesHost) {
#if defined(XP_UNIX)
            MOZ_ASSERT(!mNetAddrIsSet || mNetAddr.raw.family != AF_LOCAL,
                       "Unix domain sockets can't be used with proxies");
#endif
            
            
            
            if (!net_IsValidHostName(mHost) &&
                !mHost.Equals(NS_LITERAL_CSTRING("*"))) {
                SOCKET_LOG(("  invalid hostname %s\n", mHost.get()));
                return NS_ERROR_UNKNOWN_HOST;
            }
        }
        if (mProxyTransparentResolvesHost) {
            
            
            
            
            
            
            
            mState = STATE_RESOLVING;
            mNetAddr.raw.family = AF_INET;
            mNetAddr.inet.port = htons(SocketPort());
            mNetAddr.inet.ip = htonl(INADDR_ANY);
            return PostEvent(MSG_DNS_LOOKUP_COMPLETE, NS_OK, nullptr);
        }
    }

    nsCOMPtr<nsIDNSService> dns = do_GetService(kDNSServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    mResolving = true;

    uint32_t dnsFlags = 0;
    if (mConnectionFlags & nsSocketTransport::BYPASS_CACHE)
        dnsFlags = nsIDNSService::RESOLVE_BYPASS_CACHE;
    if (mConnectionFlags & nsSocketTransport::DISABLE_IPV6)
        dnsFlags |= nsIDNSService::RESOLVE_DISABLE_IPV6;
    if (mConnectionFlags & nsSocketTransport::DISABLE_IPV4)
        dnsFlags |= nsIDNSService::RESOLVE_DISABLE_IPV4;

    NS_ASSERTION(!(dnsFlags & nsIDNSService::RESOLVE_DISABLE_IPV6) ||
                 !(dnsFlags & nsIDNSService::RESOLVE_DISABLE_IPV4),
                 "Setting both RESOLVE_DISABLE_IPV6 and RESOLVE_DISABLE_IPV4");

    SendStatus(NS_NET_STATUS_RESOLVING_HOST);
    rv = dns->AsyncResolveExtended(SocketHost(), dnsFlags, mNetworkInterfaceId, this,
                                   nullptr, getter_AddRefs(mDNSRequest));
    if (NS_SUCCEEDED(rv)) {
        SOCKET_LOG(("  advancing to STATE_RESOLVING\n"));
        mState = STATE_RESOLVING;
    }
    return rv;
}

nsresult
nsSocketTransport::BuildSocket(PRFileDesc *&fd, bool &proxyTransparent, bool &usingSSL)
{
    SOCKET_LOG(("nsSocketTransport::BuildSocket [this=%p]\n", this));

    nsresult rv;

    proxyTransparent = false;
    usingSSL = false;

    if (mTypeCount == 0) {
        fd = PR_OpenTCPSocket(mNetAddr.raw.family);
        rv = fd ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
    }
    else {
#if defined(XP_UNIX)
        MOZ_ASSERT(!mNetAddrIsSet || mNetAddr.raw.family != AF_LOCAL,
                   "Unix domain sockets can't be used with socket types");
#endif

        fd = nullptr;

        nsCOMPtr<nsISocketProviderService> spserv =
            do_GetService(kSocketProviderServiceCID, &rv);
        if (NS_FAILED(rv)) return rv;

        const char *host       = mHost.get();
        int32_t     port       = (int32_t) mPort;
        const char *proxyHost  = mProxyHost.IsEmpty() ? nullptr : mProxyHost.get();
        int32_t     proxyPort  = (int32_t) mProxyPort;
        uint32_t    controlFlags = 0;

        uint32_t i;
        for (i=0; i<mTypeCount; ++i) {
            nsCOMPtr<nsISocketProvider> provider;

            SOCKET_LOG(("  pushing io layer [%u:%s]\n", i, mTypes[i]));

            rv = spserv->GetSocketProvider(mTypes[i], getter_AddRefs(provider));
            if (NS_FAILED(rv))
                break;

            if (mProxyTransparentResolvesHost)
                controlFlags |= nsISocketProvider::PROXY_RESOLVES_HOST;
            
            if (mConnectionFlags & nsISocketTransport::ANONYMOUS_CONNECT)
                controlFlags |= nsISocketProvider::ANONYMOUS_CONNECT;

            if (mConnectionFlags & nsISocketTransport::NO_PERMANENT_STORAGE)
                controlFlags |= nsISocketProvider::NO_PERMANENT_STORAGE;

            nsCOMPtr<nsISupports> secinfo;
            if (i == 0) {
                
                

                
                

                rv = provider->NewSocket(mNetAddr.raw.family,
                                         mHttpsProxy ? proxyHost : host,
                                         mHttpsProxy ? proxyPort : port,
                                         proxyHost, proxyPort,
                                         controlFlags, &fd,
                                         getter_AddRefs(secinfo));

                if (NS_SUCCEEDED(rv) && !fd) {
                    NS_NOTREACHED("NewSocket succeeded but failed to create a PRFileDesc");
                    rv = NS_ERROR_UNEXPECTED;
                }
            }
            else {
                
                
                
                rv = provider->AddToSocket(mNetAddr.raw.family,
                                           host, port, proxyHost, proxyPort,
                                           controlFlags, fd,
                                           getter_AddRefs(secinfo));
            }
            
            if (NS_FAILED(rv))
                break;

            
            bool isSSL = (strcmp(mTypes[i], "ssl") == 0);
            if (isSSL || (strcmp(mTypes[i], "starttls") == 0)) {
                
                nsCOMPtr<nsIInterfaceRequestor> callbacks;
                {
                    MutexAutoLock lock(mLock);
                    mSecInfo = secinfo;
                    callbacks = mCallbacks;
                    SOCKET_LOG(("  [secinfo=%x callbacks=%x]\n", mSecInfo.get(), mCallbacks.get()));
                }
                
                nsCOMPtr<nsISSLSocketControl> secCtrl(do_QueryInterface(secinfo));
                if (secCtrl)
                    secCtrl->SetNotificationCallbacks(callbacks);
                
                usingSSL = isSSL;
            }
            else if ((strcmp(mTypes[i], "socks") == 0) ||
                     (strcmp(mTypes[i], "socks4") == 0)) {
                
                
                proxyHost = nullptr;
                proxyPort = -1;
                proxyTransparent = true;
            }
        }

        if (NS_FAILED(rv)) {
            SOCKET_LOG(("  error pushing io layer [%u:%s rv=%x]\n", i, mTypes[i], rv));
            if (fd)
                PR_Close(fd);
        }
    }

    return rv;
}

nsresult
nsSocketTransport::InitiateSocket()
{
    SOCKET_LOG(("nsSocketTransport::InitiateSocket [this=%p]\n", this));

    static int crashOnNonLocalConnections = -1;
    if (crashOnNonLocalConnections == -1) {
        const char *s = getenv("MOZ_DISABLE_NONLOCAL_CONNECTIONS");
        if (s) {
            crashOnNonLocalConnections = !!strncmp(s, "0", 1);
        } else {
            crashOnNonLocalConnections = 0;
        }
    }

    nsresult rv;
    bool isLocal;
    IsLocal(&isLocal);

    if (gIOService->IsOffline()) {
        if (!isLocal)
            return NS_ERROR_OFFLINE;
    } else if (!isLocal) {

#ifdef DEBUG
        
        if (NS_SUCCEEDED(mCondition) &&
            ((mNetAddr.raw.family == AF_INET) || (mNetAddr.raw.family == AF_INET6))) {
            MOZ_ASSERT(!IsNeckoChild());
        }
#endif

        if (NS_SUCCEEDED(mCondition) &&
            crashOnNonLocalConnections &&
            !(IsIPAddrAny(&mNetAddr) || IsIPAddrLocal(&mNetAddr))) {
            nsAutoCString ipaddr;
            nsRefPtr<nsNetAddr> netaddr = new nsNetAddr(&mNetAddr);
            netaddr->GetAddress(ipaddr);
            fprintf_stderr(stderr,
                           "FATAL ERROR: Non-local network connections are disabled and a connection "
                           "attempt to %s (%s) was made.\nYou should only access hostnames "
                           "available via the test networking proxy (if running mochitests) "
                           "or from a test-specific httpd.js server (if running xpcshell tests). "
                           "Browser services should be disabled or redirected to a local server.\n",
                           mHost.get(), ipaddr.get());
            MOZ_CRASH("Attempting to connect to non-local address!");
        }
    }

    
    
    if (mConnectionFlags & nsISocketTransport::DISABLE_RFC1918 &&
        IsIPAddrLocal(&mNetAddr)) {
#ifdef PR_LOGGING
        if (SOCKET_LOG_ENABLED()) {
            nsAutoCString netAddrCString;
            netAddrCString.SetCapacity(kIPv6CStrBufSize);
            if (!NetAddrToString(&mNetAddr,
                                 netAddrCString.BeginWriting(),
                                 kIPv6CStrBufSize))
                netAddrCString = NS_LITERAL_CSTRING("<IP-to-string failed>");
            SOCKET_LOG(("nsSocketTransport::InitiateSocket skipping "
                        "speculative connection for host [%s:%d] proxy "
                        "[%s:%d] with Local IP address [%s]",
                        mHost.get(), mPort, mProxyHost.get(), mProxyPort,
                        netAddrCString.get()));
        }
#endif
        mCondition = NS_ERROR_CONNECTION_REFUSED;
        OnSocketDetached(nullptr);
        return mCondition;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    if (!mSocketTransportService->CanAttachSocket()) {
        nsCOMPtr<nsIRunnable> event =
                new nsSocketEvent(this, MSG_RETRY_INIT_SOCKET);
        if (!event)
            return NS_ERROR_OUT_OF_MEMORY;
        return mSocketTransportService->NotifyWhenCanAttachSocket(event);
    }

    
    
    
    if (mFD.IsInitialized()) {
        rv = mSocketTransportService->AttachSocket(mFD, this);
        if (NS_SUCCEEDED(rv))
            mAttached = true;
        return rv;
    }

    
    
    
    PRFileDesc *fd;
    bool proxyTransparent;
    bool usingSSL;

    rv = BuildSocket(fd, proxyTransparent, usingSSL);
    if (NS_FAILED(rv)) {
        SOCKET_LOG(("  BuildSocket failed [rv=%x]\n", rv));
        return rv;
    }

    
    mozilla::net::NetworkActivityMonitor::AttachIOLayer(fd);

    PRStatus status;

    
    PRSocketOptionData opt;
    opt.option = PR_SockOpt_Nonblocking;
    opt.value.non_blocking = true;
    status = PR_SetSocketOption(fd, &opt);
    NS_ASSERTION(status == PR_SUCCESS, "unable to make socket non-blocking");

    
    
    
    opt.option = PR_SockOpt_NoDelay;
    opt.value.no_delay = true;
    PR_SetSocketOption(fd, &opt);

    
    
    
    int32_t sndBufferSize;
    mSocketTransportService->GetSendBufferSize(&sndBufferSize);
    if (sndBufferSize > 0) {
        opt.option = PR_SockOpt_SendBufferSize;
        opt.value.send_buffer_size = sndBufferSize;
        PR_SetSocketOption(fd, &opt);
    }

    if (mQoSBits) {
        opt.option = PR_SockOpt_IpTypeOfService;
        opt.value.tos = mQoSBits;
        PR_SetSocketOption(fd, &opt);
    }

    
    rv = mSocketTransportService->AttachSocket(fd, this);
    if (NS_FAILED(rv)) {
        PR_Close(fd);
        return rv;
    }
    mAttached = true;

    
    
    {
        MutexAutoLock lock(mLock);
        mFD = fd;
        mFDref = 1;
        mFDconnected = false;
    }

    SOCKET_LOG(("  advancing to STATE_CONNECTING\n"));
    mState = STATE_CONNECTING;
    mPollTimeout = mTimeouts[TIMEOUT_CONNECT];
    SendStatus(NS_NET_STATUS_CONNECTING_TO);

#if defined(PR_LOGGING)
    if (SOCKET_LOG_ENABLED()) {
        char buf[kNetAddrMaxCStrBufSize];
        NetAddrToString(&mNetAddr, buf, sizeof(buf));
        SOCKET_LOG(("  trying address: %s\n", buf));
    }
#endif

    
    
    
    PRNetAddr prAddr;
    {
        if (mBindAddr) {
            MutexAutoLock lock(mLock);
            NetAddrToPRNetAddr(mBindAddr.get(), &prAddr);
            status = PR_Bind(fd, &prAddr);
            if (status != PR_SUCCESS) {
                return NS_ERROR_FAILURE;
            }
            mBindAddr = nullptr;
        }
    }

    NetAddrToPRNetAddr(&mNetAddr, &prAddr);

    MOZ_EVENT_TRACER_EXEC(this, "net::tcp::connect");
    status = PR_Connect(fd, &prAddr, NS_SOCKET_CONNECT_TIMEOUT);
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
                    SOCKET_LOG(("  calling ProxyStartSSL()\n"));
                    secCtrl->ProxyStartSSL();
                }
                
                
                
                
                
                
            }
        }
        
        
        
        
        else if (PR_UNKNOWN_ERROR == code &&
                 mProxyTransparent &&
                 !mProxyHost.IsEmpty()) {
            code = PR_GetOSError();
            rv = ErrorAccordingToNSPR(code);
        }
        
        
        
        else {
            rv = ErrorAccordingToNSPR(code);
            if ((rv == NS_ERROR_CONNECTION_REFUSED) && !mProxyHost.IsEmpty())
                rv = NS_ERROR_PROXY_CONNECTION_REFUSED;
        }
    }
    return rv;
}

bool
nsSocketTransport::RecoverFromError()
{
    NS_ASSERTION(NS_FAILED(mCondition), "there should be something wrong");

    SOCKET_LOG(("nsSocketTransport::RecoverFromError [this=%p state=%x cond=%x]\n",
        this, mState, mCondition));

#if defined(XP_UNIX)
    
    
    if (mNetAddrIsSet && mNetAddr.raw.family == AF_LOCAL)
        return false;
#endif

    
    if (mState != STATE_RESOLVING && mState != STATE_CONNECTING)
        return false;

    nsresult rv;

    
    NS_ASSERTION(!mFDconnected, "socket should not be connected");

    
    
    if (mState == STATE_CONNECTING && mDNSRecord) {
        mDNSRecord->ReportUnusable(SocketPort());
    }

    
    if (mCondition != NS_ERROR_CONNECTION_REFUSED &&
        mCondition != NS_ERROR_PROXY_CONNECTION_REFUSED &&
        mCondition != NS_ERROR_NET_TIMEOUT &&
        mCondition != NS_ERROR_UNKNOWN_HOST &&
        mCondition != NS_ERROR_UNKNOWN_PROXY_HOST)
        return false;

    bool tryAgain = false;

    if (mConnectionFlags & (DISABLE_IPV6 | DISABLE_IPV4) &&
        mCondition == NS_ERROR_UNKNOWN_HOST &&
        mState == STATE_RESOLVING &&
        !mProxyTransparentResolvesHost) {
        SOCKET_LOG(("  trying lookup again with both ipv4/ipv6 enabled\n"));
        mConnectionFlags &= ~(DISABLE_IPV6 | DISABLE_IPV4);
        tryAgain = true;
    }

    
    if (mState == STATE_CONNECTING && mDNSRecord) {
        nsresult rv = mDNSRecord->GetNextAddr(SocketPort(), &mNetAddr);
        if (NS_SUCCEEDED(rv)) {
            SOCKET_LOG(("  trying again with next ip address\n"));
            tryAgain = true;
        }
        else if (mConnectionFlags & (DISABLE_IPV6 | DISABLE_IPV4)) {
            
            
            
            
            SOCKET_LOG(("  failed to connect all ipv4-only or ipv6-only hosts,"
                        " trying lookup/connect again with both ipv4/ipv6\n"));
            mState = STATE_CLOSED;
            mConnectionFlags &= ~(DISABLE_IPV6 | DISABLE_IPV4);
            tryAgain = true;
        }
    }

#if defined(XP_WIN)
    
    
    if (!tryAgain) {
        bool autodialEnabled;
        mSocketTransportService->GetAutodialEnabled(&autodialEnabled);
        if (autodialEnabled) {
          tryAgain = nsNativeConnectionHelper::OnConnectionFailed(
                       NS_ConvertUTF8toUTF16(SocketHost()).get());
	    }
    }
#endif

    
    if (tryAgain) {
        uint32_t msg;

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
            tryAgain = false;
    }

    return tryAgain;
}


void
nsSocketTransport::OnMsgInputClosed(nsresult reason)
{
    SOCKET_LOG(("nsSocketTransport::OnMsgInputClosed [this=%p reason=%x]\n",
        this, reason));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    mInputClosed = true;
    
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
    SOCKET_LOG(("nsSocketTransport::OnMsgOutputClosed [this=%p reason=%x]\n",
        this, reason));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    mOutputClosed = true;
    
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
    SOCKET_LOG(("  advancing to STATE_TRANSFERRING\n"));

    mPollFlags = (PR_POLL_READ | PR_POLL_WRITE | PR_POLL_EXCEPT);
    mPollTimeout = mTimeouts[TIMEOUT_READ_WRITE];
    mState = STATE_TRANSFERRING;

    
    
    mNetAddrIsSet = true;

    
    
    {
        MutexAutoLock lock(mLock);
        NS_ASSERTION(mFD.IsInitialized(), "no socket");
        NS_ASSERTION(mFDref == 1, "wrong socket ref count");
        mFDconnected = true;
    }

    
    if (mKeepaliveEnabled) {
        nsresult rv = SetKeepaliveEnabledInternal(true);
        if (NS_WARN_IF(NS_FAILED(rv))) {
            SOCKET_LOG(("  SetKeepaliveEnabledInternal failed rv[0x%x]", rv));
        }
    }

    MOZ_EVENT_TRACER_DONE(this, "net::tcp::connect");

    SendStatus(NS_NET_STATUS_CONNECTED_TO);
}

PRFileDesc *
nsSocketTransport::GetFD_Locked()
{
    mLock.AssertCurrentThreadOwns();

    
    if (!mFDconnected)
        return nullptr;

    if (mFD.IsInitialized())
        mFDref++;

    return mFD;
}

class ThunkPRClose : public nsRunnable
{
public:
  explicit ThunkPRClose(PRFileDesc *fd) : mFD(fd) {}

  NS_IMETHOD Run()
  {
    PR_Close(mFD);
    return NS_OK;
  }
private:
  PRFileDesc *mFD;
};

void
STS_PRCloseOnSocketTransport(PRFileDesc *fd)
{
  if (gSocketTransportService) {
    
    
    
    gSocketTransportService->Dispatch(new ThunkPRClose(fd), NS_DISPATCH_NORMAL);
  } else {
    
    NS_ASSERTION(gSocketTransportService, "No STS service");
  }
}

void
nsSocketTransport::ReleaseFD_Locked(PRFileDesc *fd)
{
    mLock.AssertCurrentThreadOwns();

    NS_ASSERTION(mFD == fd, "wrong fd");
    SOCKET_LOG(("JIMB: ReleaseFD_Locked: mFDref = %d\n", mFDref));

    if (--mFDref == 0) {
        if (PR_GetCurrentThread() == gSocketThread) {
            SOCKET_LOG(("nsSocketTransport: calling PR_Close [this=%p]\n", this));
            PR_Close(mFD);
        } else {
            
            STS_PRCloseOnSocketTransport(mFD);
        }
        mFD = nullptr;
    }
}




void
nsSocketTransport::OnSocketEvent(uint32_t type, nsresult status, nsISupports *param)
{
    SOCKET_LOG(("nsSocketTransport::OnSocketEvent [this=%p type=%u status=%x param=%p]\n",
        this, type, status, param));

    if (NS_FAILED(mCondition)) {
        
        SOCKET_LOG(("  blocking event [condition=%x]\n", mCondition));
        
        
        
        mInput.OnSocketReady(mCondition);
        mOutput.OnSocketReady(mCondition);
        return;
    }

    switch (type) {
    case MSG_ENSURE_CONNECT:
        SOCKET_LOG(("  MSG_ENSURE_CONNECT\n"));
        
        
        
        
        if (mState == STATE_CLOSED) {
            
            
            
#if defined(XP_UNIX)
            if (mNetAddrIsSet && mNetAddr.raw.family == AF_LOCAL)
                mCondition = InitiateSocket();
            else
#endif
                mCondition = ResolveHost();

        } else {
            SOCKET_LOG(("  ignoring redundant event\n"));
        }
        break;

    case MSG_DNS_LOOKUP_COMPLETE:
        if (mDNSRequest)  
            SendStatus(NS_NET_STATUS_RESOLVED_HOST);

        SOCKET_LOG(("  MSG_DNS_LOOKUP_COMPLETE\n"));
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
        SOCKET_LOG(("  MSG_INPUT_CLOSED\n"));
        OnMsgInputClosed(status);
        break;

    case MSG_INPUT_PENDING:
        SOCKET_LOG(("  MSG_INPUT_PENDING\n"));
        OnMsgInputPending();
        break;

    case MSG_OUTPUT_CLOSED:
        SOCKET_LOG(("  MSG_OUTPUT_CLOSED\n"));
        OnMsgOutputClosed(status);
        break;

    case MSG_OUTPUT_PENDING:
        SOCKET_LOG(("  MSG_OUTPUT_PENDING\n"));
        OnMsgOutputPending();
        break;
    case MSG_TIMEOUT_CHANGED:
        SOCKET_LOG(("  MSG_TIMEOUT_CHANGED\n"));
        mPollTimeout = mTimeouts[(mState == STATE_TRANSFERRING)
          ? TIMEOUT_READ_WRITE : TIMEOUT_CONNECT];
        break;
    default:
        SOCKET_LOG(("  unhandled event!\n"));
    }
    
    if (NS_FAILED(mCondition)) {
        SOCKET_LOG(("  after event [this=%p cond=%x]\n", this, mCondition));
        if (!mAttached) 
            OnSocketDetached(nullptr);
    }
    else if (mPollFlags == PR_POLL_EXCEPT)
        mPollFlags = 0; 
}




void
nsSocketTransport::OnSocketReady(PRFileDesc *fd, int16_t outFlags)
{
    SOCKET_LOG(("nsSocketTransport::OnSocketReady [this=%p outFlags=%hd]\n",
        this, outFlags));

    if (outFlags == -1) {
        SOCKET_LOG(("socket timeout expired\n"));
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
            
            
            
            else if (PR_UNKNOWN_ERROR == code &&
                     mProxyTransparent &&
                     !mProxyHost.IsEmpty()) {
                code = PR_GetOSError();
                mCondition = ErrorAccordingToNSPR(code);
            }
            else {
                
                
                
                mCondition = ErrorAccordingToNSPR(code);
                if ((mCondition == NS_ERROR_CONNECTION_REFUSED) && !mProxyHost.IsEmpty())
                    mCondition = NS_ERROR_PROXY_CONNECTION_REFUSED;
                SOCKET_LOG(("  connection failed! [reason=%x]\n", mCondition));
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
    SOCKET_LOG(("nsSocketTransport::OnSocketDetached [this=%p cond=%x]\n",
        this, mCondition));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    
    
    if (NS_SUCCEEDED(mCondition)) {
        if (gIOService->IsOffline()) {
          mCondition = NS_ERROR_OFFLINE;
        }
        else {
          mCondition = NS_ERROR_ABORT;
        }
    }

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
        secCtrl->SetNotificationCallbacks(nullptr);

    
    
    

    
    
    
    
    
    nsCOMPtr<nsIInterfaceRequestor> ourCallbacks;
    nsCOMPtr<nsITransportEventSink> ourEventSink;
    {
        MutexAutoLock lock(mLock);
        if (mFD.IsInitialized()) {
            ReleaseFD_Locked(mFD);
            
            
            mFDconnected = false;
        }

        
        
        
        
        if (NS_FAILED(mCondition)) {
            mCallbacks.swap(ourCallbacks);
            mEventSink.swap(ourEventSink);
        }
    }
}

void
nsSocketTransport::IsLocal(bool *aIsLocal)
{
    {
        MutexAutoLock lock(mLock);

#if defined(XP_UNIX)
        
        if (mNetAddr.raw.family == PR_AF_LOCAL)
        {
            *aIsLocal = true;
            return;
        }
#endif

        *aIsLocal = IsLoopBackAddress(&mNetAddr);
    }
}




NS_IMPL_ISUPPORTS(nsSocketTransport,
                  nsISocketTransport,
                  nsITransport,
                  nsIDNSListener,
                  nsIClassInfo,
                  nsIInterfaceRequestor)
NS_IMPL_CI_INTERFACE_GETTER(nsSocketTransport,
                            nsISocketTransport,
                            nsITransport,
                            nsIDNSListener,
                            nsIInterfaceRequestor)

NS_IMETHODIMP
nsSocketTransport::OpenInputStream(uint32_t flags,
                                   uint32_t segsize,
                                   uint32_t segcount,
                                   nsIInputStream **result)
{
    SOCKET_LOG(("nsSocketTransport::OpenInputStream [this=%p flags=%x]\n",
        this, flags));

    NS_ENSURE_TRUE(!mInput.IsReferenced(), NS_ERROR_UNEXPECTED);

    nsresult rv;
    nsCOMPtr<nsIAsyncInputStream> pipeIn;

    if (!(flags & OPEN_UNBUFFERED) || (flags & OPEN_BLOCKING)) {
        
        
        bool openBlocking =  (flags & OPEN_BLOCKING);

        net_ResolveSegmentParams(segsize, segcount);

        
        nsCOMPtr<nsIAsyncOutputStream> pipeOut;
        rv = NS_NewPipe2(getter_AddRefs(pipeIn), getter_AddRefs(pipeOut),
                         !openBlocking, true, segsize, segcount);
        if (NS_FAILED(rv)) return rv;

        
        rv = NS_AsyncCopy(&mInput, pipeOut, mSocketTransportService,
                          NS_ASYNCCOPY_VIA_WRITESEGMENTS, segsize);
        if (NS_FAILED(rv)) return rv;

        *result = pipeIn;
    }
    else
        *result = &mInput;

    
    mInputClosed = false;

    rv = PostEvent(MSG_ENSURE_CONNECT);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*result);
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::OpenOutputStream(uint32_t flags,
                                    uint32_t segsize,
                                    uint32_t segcount,
                                    nsIOutputStream **result)
{
    SOCKET_LOG(("nsSocketTransport::OpenOutputStream [this=%p flags=%x]\n",
        this, flags));

    NS_ENSURE_TRUE(!mOutput.IsReferenced(), NS_ERROR_UNEXPECTED);

    nsresult rv;
    nsCOMPtr<nsIAsyncOutputStream> pipeOut;
    if (!(flags & OPEN_UNBUFFERED) || (flags & OPEN_BLOCKING)) {
        
        
        bool openBlocking =  (flags & OPEN_BLOCKING);

        net_ResolveSegmentParams(segsize, segcount);

        
        nsCOMPtr<nsIAsyncInputStream> pipeIn;
        rv = NS_NewPipe2(getter_AddRefs(pipeIn), getter_AddRefs(pipeOut),
                         true, !openBlocking, segsize, segcount);
        if (NS_FAILED(rv)) return rv;

        
        rv = NS_AsyncCopy(pipeIn, &mOutput, mSocketTransportService,
                          NS_ASYNCCOPY_VIA_READSEGMENTS, segsize);
        if (NS_FAILED(rv)) return rv;

        *result = pipeOut;
    }
    else
        *result = &mOutput;

    
    mOutputClosed = false;

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
    MutexAutoLock lock(mLock);
    NS_IF_ADDREF(*secinfo = mSecInfo);
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetSecurityCallbacks(nsIInterfaceRequestor **callbacks)
{
    MutexAutoLock lock(mLock);
    NS_IF_ADDREF(*callbacks = mCallbacks);
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::SetSecurityCallbacks(nsIInterfaceRequestor *callbacks)
{
    nsCOMPtr<nsIInterfaceRequestor> threadsafeCallbacks;
    NS_NewNotificationCallbacksAggregation(callbacks, nullptr,
                                           NS_GetCurrentThread(),
                                           getter_AddRefs(threadsafeCallbacks));

    nsCOMPtr<nsISupports> secinfo;
    {
        MutexAutoLock lock(mLock);
        mCallbacks = threadsafeCallbacks;
        SOCKET_LOG(("Reset callbacks for secinfo=%p callbacks=%p\n",
                    mSecInfo.get(), mCallbacks.get()));

        secinfo = mSecInfo;
    }

    
    nsCOMPtr<nsISSLSocketControl> secCtrl(do_QueryInterface(secinfo));
    if (secCtrl)
        secCtrl->SetNotificationCallbacks(threadsafeCallbacks);

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

    MutexAutoLock lock(mLock);
    mEventSink = sink;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::IsAlive(bool *result)
{
    *result = false;

    nsresult conditionWhileLocked = NS_OK;
    PRFileDescAutoLock fd(this, &conditionWhileLocked);
    if (NS_FAILED(conditionWhileLocked) || !fd.IsInitialized()) {
        return NS_OK;
    }

    

    char c;
    int32_t rval = PR_Recv(fd, &c, 1, PR_MSG_PEEK, 0);

    if ((rval > 0) || (rval < 0 && PR_GetError() == PR_WOULD_BLOCK_ERROR))
        *result = true;

    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetHost(nsACString &host)
{
    host = SocketHost();
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetPort(int32_t *port)
{
    *port = (int32_t) SocketPort();
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetNetworkInterfaceId(nsACString_internal &aNetworkInterfaceId)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    aNetworkInterfaceId = mNetworkInterfaceId;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::SetNetworkInterfaceId(const nsACString_internal &aNetworkInterfaceId)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    mNetworkInterfaceId = aNetworkInterfaceId;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetPeerAddr(NetAddr *addr)
{
    
    
    
    

    if (!mNetAddrIsSet) {
        SOCKET_LOG(("nsSocketTransport::GetPeerAddr [this=%p state=%d] "
                    "NOT_AVAILABLE because not yet connected.", this, mState));
        return NS_ERROR_NOT_AVAILABLE;
    }

    memcpy(addr, &mNetAddr, sizeof(NetAddr));
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetSelfAddr(NetAddr *addr)
{
    
    
    

    PRFileDescAutoLock fd(this);
    if (!fd.IsInitialized()) {
        return NS_ERROR_NOT_CONNECTED;
    }

    PRNetAddr prAddr;

    
    
    
    
    
    
    
    memset(&prAddr, 0, sizeof(prAddr));

    nsresult rv =
        (PR_GetSockName(fd, &prAddr) == PR_SUCCESS) ? NS_OK : NS_ERROR_FAILURE;
    PRNetAddrToNetAddr(&prAddr, addr);

    return rv;
}

NS_IMETHODIMP
nsSocketTransport::Bind(NetAddr *aLocalAddr)
{
    NS_ENSURE_ARG(aLocalAddr);

    MutexAutoLock lock(mLock);
    if (mAttached) {
        return NS_ERROR_FAILURE;
    }

    mBindAddr = new NetAddr();
    memcpy(mBindAddr.get(), aLocalAddr, sizeof(NetAddr));

    return NS_OK;
}


NS_IMETHODIMP
nsSocketTransport::GetScriptablePeerAddr(nsINetAddr * *addr)
{
    NetAddr rawAddr;

    nsresult rv;
    rv = GetPeerAddr(&rawAddr);
    if (NS_FAILED(rv))
        return rv;

    NS_ADDREF(*addr = new nsNetAddr(&rawAddr));

    return NS_OK;
}


NS_IMETHODIMP
nsSocketTransport::GetScriptableSelfAddr(nsINetAddr * *addr)
{
    NetAddr rawAddr;

    nsresult rv;
    rv = GetSelfAddr(&rawAddr);
    if (NS_FAILED(rv))
        return rv;

    NS_ADDREF(*addr = new nsNetAddr(&rawAddr));

    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetTimeout(uint32_t type, uint32_t *value)
{
    NS_ENSURE_ARG_MAX(type, nsISocketTransport::TIMEOUT_READ_WRITE);
    *value = (uint32_t) mTimeouts[type];
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::SetTimeout(uint32_t type, uint32_t value)
{
    NS_ENSURE_ARG_MAX(type, nsISocketTransport::TIMEOUT_READ_WRITE);
    
    mTimeouts[type] = (uint16_t) std::min<uint32_t>(value, UINT16_MAX);
    PostEvent(MSG_TIMEOUT_CHANGED);
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::SetQoSBits(uint8_t aQoSBits)
{
    
    
    
    
    

    mQoSBits = aQoSBits;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetQoSBits(uint8_t *aQoSBits)
{
    *aQoSBits = mQoSBits;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetRecvBufferSize(uint32_t *aSize)
{
    PRFileDescAutoLock fd(this);
    if (!fd.IsInitialized())
        return NS_ERROR_NOT_CONNECTED;

    nsresult rv = NS_OK;
    PRSocketOptionData opt;
    opt.option = PR_SockOpt_RecvBufferSize;
    if (PR_GetSocketOption(fd, &opt) == PR_SUCCESS)
        *aSize = opt.value.recv_buffer_size;
    else
        rv = NS_ERROR_FAILURE;

    return rv;
}

NS_IMETHODIMP
nsSocketTransport::GetSendBufferSize(uint32_t *aSize)
{
    PRFileDescAutoLock fd(this);
    if (!fd.IsInitialized())
        return NS_ERROR_NOT_CONNECTED;

    nsresult rv = NS_OK;
    PRSocketOptionData opt;
    opt.option = PR_SockOpt_SendBufferSize;
    if (PR_GetSocketOption(fd, &opt) == PR_SUCCESS)
        *aSize = opt.value.send_buffer_size;
    else
        rv = NS_ERROR_FAILURE;

    return rv;
}

NS_IMETHODIMP
nsSocketTransport::SetRecvBufferSize(uint32_t aSize)
{
    PRFileDescAutoLock fd(this);
    if (!fd.IsInitialized())
        return NS_ERROR_NOT_CONNECTED;

    nsresult rv = NS_OK;
    PRSocketOptionData opt;
    opt.option = PR_SockOpt_RecvBufferSize;
    opt.value.recv_buffer_size = aSize;
    if (PR_SetSocketOption(fd, &opt) != PR_SUCCESS)
        rv = NS_ERROR_FAILURE;

    return rv;
}

NS_IMETHODIMP
nsSocketTransport::SetSendBufferSize(uint32_t aSize)
{
    PRFileDescAutoLock fd(this);
    if (!fd.IsInitialized())
        return NS_ERROR_NOT_CONNECTED;

    nsresult rv = NS_OK;
    PRSocketOptionData opt;
    opt.option = PR_SockOpt_SendBufferSize;
    opt.value.send_buffer_size = aSize;
    if (PR_SetSocketOption(fd, &opt) != PR_SUCCESS)
        rv = NS_ERROR_FAILURE;

    return rv;
}

NS_IMETHODIMP
nsSocketTransport::OnLookupComplete(nsICancelable *request,
                                    nsIDNSRecord  *rec,
                                    nsresult       status)
{
    
    mResolving = false;

    MOZ_EVENT_TRACER_WAIT(this, "net::tcp::connect");
    nsresult rv = PostEvent(MSG_DNS_LOOKUP_COMPLETE, status, rec);

    
    
    
    
    if (NS_FAILED(rv))
        NS_WARNING("unable to post DNS lookup complete message");

    return NS_OK;
}


NS_IMETHODIMP
nsSocketTransport::GetInterface(const nsIID &iid, void **result)
{
    if (iid.Equals(NS_GET_IID(nsIDNSRecord))) {
        return mDNSRecord ?
            mDNSRecord->QueryInterface(iid, result) : NS_ERROR_NO_INTERFACE;
    }
    return this->QueryInterface(iid, result);
}

NS_IMETHODIMP
nsSocketTransport::GetInterfaces(uint32_t *count, nsIID * **array)
{
    return NS_CI_INTERFACE_GETTER_NAME(nsSocketTransport)(count, array);
}

NS_IMETHODIMP
nsSocketTransport::GetScriptableHelper(nsIXPCScriptable **_retval)
{
    *_retval = nullptr;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetContractID(char * *aContractID)
{
    *aContractID = nullptr;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetClassDescription(char * *aClassDescription)
{
    *aClassDescription = nullptr;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetClassID(nsCID * *aClassID)
{
    *aClassID = nullptr;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetFlags(uint32_t *aFlags)
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
nsSocketTransport::GetConnectionFlags(uint32_t *value)
{
    *value = mConnectionFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::SetConnectionFlags(uint32_t value)
{
    mConnectionFlags = value;
    mIsPrivate = value & nsISocketTransport::NO_PERMANENT_STORAGE;
    return NS_OK;
}

void
nsSocketTransport::OnKeepaliveEnabledPrefChange(bool aEnabled)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    
    
    
    if (mKeepaliveEnabled) {
        nsresult rv = SetKeepaliveEnabledInternal(aEnabled);
        if (NS_WARN_IF(NS_FAILED(rv))) {
            SOCKET_LOG(("  SetKeepaliveEnabledInternal [%s] failed rv[0x%x]",
                        aEnabled ? "enable" : "disable", rv));
        }
    }
}

nsresult
nsSocketTransport::SetKeepaliveEnabledInternal(bool aEnable)
{
    MOZ_ASSERT(mKeepaliveIdleTimeS > 0 &&
               mKeepaliveIdleTimeS <= kMaxTCPKeepIdle);
    MOZ_ASSERT(mKeepaliveRetryIntervalS > 0 &&
               mKeepaliveRetryIntervalS <= kMaxTCPKeepIntvl);
    MOZ_ASSERT(mKeepaliveProbeCount > 0 &&
               mKeepaliveProbeCount <= kMaxTCPKeepCount);

    PRFileDescAutoLock fd(this);
    if (NS_WARN_IF(!fd.IsInitialized())) {
        return NS_ERROR_NOT_INITIALIZED;
    }

    
    
    bool enable = aEnable && mSocketTransportService->IsKeepaliveEnabled();
    nsresult rv = fd.SetKeepaliveVals(enable,
                                      mKeepaliveIdleTimeS,
                                      mKeepaliveRetryIntervalS,
                                      mKeepaliveProbeCount);
    if (NS_WARN_IF(NS_FAILED(rv))) {
        SOCKET_LOG(("  SetKeepaliveVals failed rv[0x%x]", rv));
        return rv;
    }
    rv = fd.SetKeepaliveEnabled(enable);
    if (NS_WARN_IF(NS_FAILED(rv))) {
        SOCKET_LOG(("  SetKeepaliveEnabled failed rv[0x%x]", rv));
        return rv;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::GetKeepaliveEnabled(bool *aResult)
{
    MOZ_ASSERT(aResult);

    *aResult = mKeepaliveEnabled;
    return NS_OK;
}

nsresult
nsSocketTransport::EnsureKeepaliveValsAreInitialized()
{
    nsresult rv = NS_OK;
    int32_t val = -1;
    if (mKeepaliveIdleTimeS == -1) {
        rv = mSocketTransportService->GetKeepaliveIdleTime(&val);
        if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
        }
        mKeepaliveIdleTimeS = val;
    }
    if (mKeepaliveRetryIntervalS == -1) {
        rv = mSocketTransportService->GetKeepaliveRetryInterval(&val);
        if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
        }
        mKeepaliveRetryIntervalS = val;
    }
    if (mKeepaliveProbeCount == -1) {
        rv = mSocketTransportService->GetKeepaliveProbeCount(&val);
        if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
        }
        mKeepaliveProbeCount = val;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransport::SetKeepaliveEnabled(bool aEnable)
{
#if defined(XP_WIN) || defined(XP_UNIX) || defined(XP_MACOSX)
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (aEnable == mKeepaliveEnabled) {
        SOCKET_LOG(("nsSocketTransport::SetKeepaliveEnabled [%p] already %s.",
                    this, aEnable ? "enabled" : "disabled"));
        return NS_OK;
    }

    nsresult rv = NS_OK;
    if (aEnable) {
        rv = EnsureKeepaliveValsAreInitialized();
        if (NS_WARN_IF(NS_FAILED(rv))) {
            SOCKET_LOG(("  SetKeepaliveEnabled [%p] "
                        "error [0x%x] initializing keepalive vals",
                        this, rv));
            return rv;
        }
    }
    SOCKET_LOG(("nsSocketTransport::SetKeepaliveEnabled [%p] "
                "%s, idle time[%ds] retry interval[%ds] packet count[%d]: "
                "globally %s.",
                this, aEnable ? "enabled" : "disabled",
                mKeepaliveIdleTimeS, mKeepaliveRetryIntervalS,
                mKeepaliveProbeCount,
                mSocketTransportService->IsKeepaliveEnabled() ?
                "enabled" : "disabled"));

    
    
    
    mKeepaliveEnabled = aEnable;

    rv = SetKeepaliveEnabledInternal(aEnable);
    if (NS_WARN_IF(NS_FAILED(rv))) {
        SOCKET_LOG(("  SetKeepaliveEnabledInternal failed rv[0x%x]", rv));
        return rv;
    }

    return NS_OK;
#else 
    SOCKET_LOG(("nsSocketTransport::SetKeepaliveEnabled unsupported platform"));
    return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

NS_IMETHODIMP
nsSocketTransport::SetKeepaliveVals(int32_t aIdleTime,
                                    int32_t aRetryInterval)
{
#if defined(XP_WIN) || defined(XP_UNIX) || defined(XP_MACOSX)
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    if (NS_WARN_IF(aIdleTime <= 0 || kMaxTCPKeepIdle < aIdleTime)) {
        return NS_ERROR_INVALID_ARG;
    }
    if (NS_WARN_IF(aRetryInterval <= 0 ||
                   kMaxTCPKeepIntvl < aRetryInterval)) {
        return NS_ERROR_INVALID_ARG;
    }

    if (aIdleTime == mKeepaliveIdleTimeS &&
        aRetryInterval == mKeepaliveRetryIntervalS) {
        SOCKET_LOG(("nsSocketTransport::SetKeepaliveVals [%p] idle time "
                    "already %ds and retry interval already %ds.",
                    this, mKeepaliveIdleTimeS,
                    mKeepaliveRetryIntervalS));
        return NS_OK;
    }
    mKeepaliveIdleTimeS = aIdleTime;
    mKeepaliveRetryIntervalS = aRetryInterval;

    nsresult rv = NS_OK;
    if (mKeepaliveProbeCount == -1) {
        int32_t val = -1;
        nsresult rv = mSocketTransportService->GetKeepaliveProbeCount(&val);
        if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
        }
        mKeepaliveProbeCount = val;
    }

    SOCKET_LOG(("nsSocketTransport::SetKeepaliveVals [%p] "
                "keepalive %s, idle time[%ds] retry interval[%ds] "
                "packet count[%d]",
                this, mKeepaliveEnabled ? "enabled" : "disabled",
                mKeepaliveIdleTimeS, mKeepaliveRetryIntervalS,
                mKeepaliveProbeCount));

    PRFileDescAutoLock fd(this);
    if (NS_WARN_IF(!fd.IsInitialized())) {
        return NS_ERROR_NULL_POINTER;
    }

    rv = fd.SetKeepaliveVals(mKeepaliveEnabled,
                             mKeepaliveIdleTimeS,
                             mKeepaliveRetryIntervalS,
                             mKeepaliveProbeCount);
    if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
    }
    return NS_OK;
#else
    SOCKET_LOG(("nsSocketTransport::SetKeepaliveVals unsupported platform"));
    return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

#ifdef ENABLE_SOCKET_TRACING

#include <stdio.h>
#include <ctype.h>
#include "prenv.h"

static void
DumpBytesToFile(const char *path, const char *header, const char *buf, int32_t n)
{
    FILE *fp = fopen(path, "a");

    fprintf(fp, "\n%s [%d bytes]\n", header, n);

    const unsigned char *p;
    while (n) {
        p = (const unsigned char *) buf;

        int32_t i, row_max = std::min(16, n);

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
nsSocketTransport::TraceInBuf(const char *buf, int32_t n)
{
    char *val = PR_GetEnv("NECKO_SOCKET_TRACE_LOG");
    if (!val || !*val)
        return;

    nsAutoCString header;
    header.AssignLiteral("Reading from: ");
    header.Append(mHost);
    header.Append(':');
    header.AppendInt(mPort);

    DumpBytesToFile(val, header.get(), buf, n);
}

void
nsSocketTransport::TraceOutBuf(const char *buf, int32_t n)
{
    char *val = PR_GetEnv("NECKO_SOCKET_TRACE_LOG");
    if (!val || !*val)
        return;

    nsAutoCString header;
    header.AssignLiteral("Writing to: ");
    header.Append(mHost);
    header.Append(':');
    header.AppendInt(mPort);

    DumpBytesToFile(val, header.get(), buf, n);
}

#endif

static void LogNSPRError(const char* aPrefix, const void *aObjPtr)
{
#if defined(PR_LOGGING) && defined(DEBUG)
    PRErrorCode errCode = PR_GetError();
    int errLen = PR_GetErrorTextLength();
    nsAutoCString errStr;
    if (errLen > 0) {
        errStr.SetLength(errLen);
        PR_GetErrorText(errStr.BeginWriting());
    }
    NS_WARNING(nsPrintfCString(
               "%s [%p] NSPR error[0x%x] %s.",
               aPrefix ? aPrefix : "nsSocketTransport", aObjPtr, errCode,
               errLen > 0 ? errStr.BeginReading() : "<no error text>").get());
#endif
}

nsresult
nsSocketTransport::PRFileDescAutoLock::SetKeepaliveEnabled(bool aEnable)
{
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    MOZ_ASSERT(!(aEnable && !gSocketTransportService->IsKeepaliveEnabled()),
               "Cannot enable keepalive if global pref is disabled!");
    if (aEnable && !gSocketTransportService->IsKeepaliveEnabled()) {
        return NS_ERROR_ILLEGAL_VALUE;
    }

    PRSocketOptionData opt;

    opt.option = PR_SockOpt_Keepalive;
    opt.value.keep_alive = aEnable;
    PRStatus status = PR_SetSocketOption(mFd, &opt);
    if (NS_WARN_IF(status != PR_SUCCESS)) {
        LogNSPRError("nsSocketTransport::PRFileDescAutoLock::SetKeepaliveEnabled",
                     mSocketTransport);
        return ErrorAccordingToNSPR(PR_GetError());
    }
    return NS_OK;
}

static void LogOSError(const char *aPrefix, const void *aObjPtr)
{
#if defined(PR_LOGGING) && defined(DEBUG)
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread, "wrong thread");

#ifdef XP_WIN
    DWORD errCode = WSAGetLastError();
    LPVOID errMessage;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  errCode,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) &errMessage,
                  0, NULL);
#else
    int errCode = errno;
    char *errMessage = strerror(errno);
#endif
    NS_WARNING(nsPrintfCString(
               "%s [%p] OS error[0x%x] %s",
               aPrefix ? aPrefix : "nsSocketTransport", aObjPtr, errCode,
               errMessage ? errMessage : "<no error text>").get());
#ifdef XP_WIN
    LocalFree(errMessage);
#endif
#endif
}






nsresult
nsSocketTransport::PRFileDescAutoLock::SetKeepaliveVals(bool aEnabled,
                                                        int aIdleTime,
                                                        int aRetryInterval,
                                                        int aProbeCount)
{
#if defined(XP_WIN) || defined(XP_UNIX) || defined(XP_MACOSX)
    MOZ_ASSERT(PR_GetCurrentThread() == gSocketThread, "wrong thread");
    if (NS_WARN_IF(aIdleTime <= 0 || kMaxTCPKeepIdle < aIdleTime)) {
        return NS_ERROR_INVALID_ARG;
    }
    if (NS_WARN_IF(aRetryInterval <= 0 ||
                   kMaxTCPKeepIntvl < aRetryInterval)) {
        return NS_ERROR_INVALID_ARG;
    }
    if (NS_WARN_IF(aProbeCount <= 0 || kMaxTCPKeepCount < aProbeCount)) {
        return NS_ERROR_INVALID_ARG;
    }

    PROsfd sock = PR_FileDesc2NativeHandle(mFd);
    if (NS_WARN_IF(sock == -1)) {
        LogNSPRError("nsSocketTransport::PRFileDescAutoLock::SetKeepaliveVals",
                     mSocketTransport);
        return ErrorAccordingToNSPR(PR_GetError());
    }
#endif

#if defined(XP_WIN)
    
    struct tcp_keepalive keepalive_vals = {
        (int)aEnabled,
        
        aIdleTime * 1000,
        aRetryInterval * 1000
    };
    DWORD bytes_returned;
    int err = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &keepalive_vals,
                       sizeof(keepalive_vals), NULL, 0, &bytes_returned, NULL,
                       NULL);
    if (NS_WARN_IF(err)) {
        LogOSError("nsSocketTransport WSAIoctl failed", mSocketTransport);
        return NS_ERROR_UNEXPECTED;
    }
    return NS_OK;

#elif defined(XP_MACOSX)
    
    int err = setsockopt(sock, IPPROTO_TCP, TCP_KEEPALIVE,
                         &aIdleTime, sizeof(aIdleTime));
    if (NS_WARN_IF(err)) {
        LogOSError("nsSocketTransport Failed setting TCP_KEEPALIVE",
                   mSocketTransport);
        return NS_ERROR_UNEXPECTED;
    }
    return NS_OK;

#elif defined(XP_UNIX)
    
    
    
#if defined(ANDROID) || defined(TCP_KEEPIDLE)
    
    int err = setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE,
                         &aIdleTime, sizeof(aIdleTime));
    if (NS_WARN_IF(err)) {
        LogOSError("nsSocketTransport Failed setting TCP_KEEPIDLE",
                   mSocketTransport);
        return NS_ERROR_UNEXPECTED;
    }

#endif
#if defined(ANDROID) || defined(TCP_KEEPINTVL)
    
    err = setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL,
                        &aRetryInterval, sizeof(aRetryInterval));
    if (NS_WARN_IF(err)) {
        LogOSError("nsSocketTransport Failed setting TCP_KEEPINTVL",
                   mSocketTransport);
        return NS_ERROR_UNEXPECTED;
    }

#endif
#if defined(ANDROID) || defined(TCP_KEEPCNT)
    
    err = setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT,
                     &aProbeCount, sizeof(aProbeCount));
    if (NS_WARN_IF(err)) {
        LogOSError("nsSocketTransport Failed setting TCP_KEEPCNT",
                   mSocketTransport);
        return NS_ERROR_UNEXPECTED;
    }

#endif
    return NS_OK;
#else
    MOZ_ASSERT(false, "nsSocketTransport::PRFileDescAutoLock::SetKeepaliveVals "
               "called on unsupported platform!");
    return NS_ERROR_UNEXPECTED;
#endif
}
