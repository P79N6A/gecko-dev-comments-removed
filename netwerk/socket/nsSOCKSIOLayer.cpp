





#include "nspr.h"
#include "private/pprio.h"
#include "nsString.h"
#include "nsCRT.h"

#include "nsIServiceManager.h"
#include "nsIDNSService.h"
#include "nsIDNSRecord.h"
#include "nsISOCKSSocketInfo.h"
#include "nsISocketProvider.h"
#include "nsSOCKSIOLayer.h"
#include "nsNetCID.h"
#include "nsIDNSListener.h"
#include "nsICancelable.h"
#include "nsThreadUtils.h"

static PRDescIdentity	nsSOCKSIOLayerIdentity;
static PRIOMethods	nsSOCKSIOLayerMethods;
static bool firstTime = true;
static bool ipv6Supported = true;

#if defined(PR_LOGGING)
static PRLogModuleInfo *gSOCKSLog;
#define LOGDEBUG(args) PR_LOG(gSOCKSLog, PR_LOG_DEBUG, args)
#define LOGERROR(args) PR_LOG(gSOCKSLog, PR_LOG_ERROR , args)

#else
#define LOGDEBUG(args)
#define LOGERROR(args)
#endif

class nsSOCKSSocketInfo : public nsISOCKSSocketInfo
                        , public nsIDNSListener
{
    enum State {
        SOCKS_INITIAL,
        SOCKS_DNS_IN_PROGRESS,
        SOCKS_DNS_COMPLETE,
        SOCKS_CONNECTING_TO_PROXY,
        SOCKS4_WRITE_CONNECT_REQUEST,
        SOCKS4_READ_CONNECT_RESPONSE,
        SOCKS5_WRITE_AUTH_REQUEST,
        SOCKS5_READ_AUTH_RESPONSE,
        SOCKS5_WRITE_CONNECT_REQUEST,
        SOCKS5_READ_CONNECT_RESPONSE_TOP,
        SOCKS5_READ_CONNECT_RESPONSE_BOTTOM,
        SOCKS_CONNECTED,
        SOCKS_FAILED
    };

    
    
    static const PRUint32 BUFFER_SIZE = 262;
    static const PRUint32 MAX_HOSTNAME_LEN = 255;

public:
    nsSOCKSSocketInfo();
    virtual ~nsSOCKSSocketInfo() { HandshakeFinished(); }

    NS_DECL_ISUPPORTS
    NS_DECL_NSISOCKSSOCKETINFO
    NS_DECL_NSIDNSLISTENER

    void Init(PRInt32 version,
              PRInt32 family,
              const char *proxyHost,
              PRInt32 proxyPort,
              const char *destinationHost,
              PRUint32 flags);

    void SetConnectTimeout(PRIntervalTime to);
    PRStatus DoHandshake(PRFileDesc *fd, PRInt16 oflags = -1);
    PRInt16 GetPollFlags() const;
    bool IsConnected() const { return mState == SOCKS_CONNECTED; }

private:
    void HandshakeFinished(PRErrorCode err = 0);
    PRStatus StartDNS(PRFileDesc *fd);
    PRStatus ConnectToProxy(PRFileDesc *fd);
    void FixupAddressFamily(PRFileDesc *fd, PRNetAddr *proxy);
    PRStatus ContinueConnectingToProxy(PRFileDesc *fd, PRInt16 oflags);
    PRStatus WriteV4ConnectRequest();
    PRStatus ReadV4ConnectResponse();
    PRStatus WriteV5AuthRequest();
    PRStatus ReadV5AuthResponse();
    PRStatus WriteV5ConnectRequest();
    PRStatus ReadV5AddrTypeAndLength(PRUint8 *type, PRUint32 *len);
    PRStatus ReadV5ConnectResponseTop();
    PRStatus ReadV5ConnectResponseBottom();

    void WriteUint8(PRUint8 d);
    void WriteUint16(PRUint16 d);
    void WriteUint32(PRUint32 d);
    void WriteNetAddr(const PRNetAddr *addr);
    void WriteNetPort(const PRNetAddr *addr);
    void WriteString(const nsACString &str);

    PRUint8 ReadUint8();
    PRUint16 ReadUint16();
    PRUint32 ReadUint32();
    void ReadNetAddr(PRNetAddr *addr, PRUint16 fam);
    void ReadNetPort(PRNetAddr *addr);

    void WantRead(PRUint32 sz);
    PRStatus ReadFromSocket(PRFileDesc *fd);
    PRStatus WriteToSocket(PRFileDesc *fd);

private:
    State     mState;
    PRUint8 * mData;
    PRUint8 * mDataIoPtr;
    PRUint32  mDataLength;
    PRUint32  mReadOffset;
    PRUint32  mAmountToRead;
    nsCOMPtr<nsIDNSRecord>  mDnsRec;
    nsCOMPtr<nsICancelable> mLookup;
    nsresult                mLookupStatus;
    PRFileDesc             *mFD;

    nsCString mDestinationHost;
    nsCString mProxyHost;
    PRInt32   mProxyPort;
    PRInt32   mVersion;   
    PRInt32   mDestinationFamily;
    PRUint32  mFlags;
    PRNetAddr mInternalProxyAddr;
    PRNetAddr mExternalProxyAddr;
    PRNetAddr mDestinationAddr;
    PRIntervalTime mTimeout;
};

nsSOCKSSocketInfo::nsSOCKSSocketInfo()
    : mState(SOCKS_INITIAL)
    , mDataIoPtr(nullptr)
    , mDataLength(0)
    , mReadOffset(0)
    , mAmountToRead(0)
    , mProxyPort(-1)
    , mVersion(-1)
    , mDestinationFamily(PR_AF_INET)
    , mFlags(0)
    , mTimeout(PR_INTERVAL_NO_TIMEOUT)
{
    mData = new PRUint8[BUFFER_SIZE];
    PR_InitializeNetAddr(PR_IpAddrAny, 0, &mInternalProxyAddr);
    PR_InitializeNetAddr(PR_IpAddrAny, 0, &mExternalProxyAddr);
    PR_InitializeNetAddr(PR_IpAddrAny, 0, &mDestinationAddr);
}

void
nsSOCKSSocketInfo::Init(PRInt32 version, PRInt32 family, const char *proxyHost, PRInt32 proxyPort, const char *host, PRUint32 flags)
{
    mVersion         = version;
    mDestinationFamily = family;
    mProxyHost       = proxyHost;
    mProxyPort       = proxyPort;
    mDestinationHost = host;
    mFlags           = flags;
}

NS_IMPL_THREADSAFE_ISUPPORTS2(nsSOCKSSocketInfo, nsISOCKSSocketInfo, nsIDNSListener)

NS_IMETHODIMP 
nsSOCKSSocketInfo::GetExternalProxyAddr(PRNetAddr * *aExternalProxyAddr)
{
    memcpy(*aExternalProxyAddr, &mExternalProxyAddr, sizeof(PRNetAddr));
    return NS_OK;
}

NS_IMETHODIMP 
nsSOCKSSocketInfo::SetExternalProxyAddr(PRNetAddr *aExternalProxyAddr)
{
    memcpy(&mExternalProxyAddr, aExternalProxyAddr, sizeof(PRNetAddr));
    return NS_OK;
}

NS_IMETHODIMP 
nsSOCKSSocketInfo::GetDestinationAddr(PRNetAddr * *aDestinationAddr)
{
    memcpy(*aDestinationAddr, &mDestinationAddr, sizeof(PRNetAddr));
    return NS_OK;
}

NS_IMETHODIMP 
nsSOCKSSocketInfo::SetDestinationAddr(PRNetAddr *aDestinationAddr)
{
    memcpy(&mDestinationAddr, aDestinationAddr, sizeof(PRNetAddr));
    return NS_OK;
}

NS_IMETHODIMP 
nsSOCKSSocketInfo::GetInternalProxyAddr(PRNetAddr * *aInternalProxyAddr)
{
    memcpy(*aInternalProxyAddr, &mInternalProxyAddr, sizeof(PRNetAddr));
    return NS_OK;
}

NS_IMETHODIMP 
nsSOCKSSocketInfo::SetInternalProxyAddr(PRNetAddr *aInternalProxyAddr)
{
    memcpy(&mInternalProxyAddr, aInternalProxyAddr, sizeof(PRNetAddr));
    return NS_OK;
}








void
nsSOCKSSocketInfo::HandshakeFinished(PRErrorCode err)
{
    if (err == 0) {
        mState = SOCKS_CONNECTED;
    } else {
        mState = SOCKS_FAILED;
        PR_SetError(PR_UNKNOWN_ERROR, err);
    }

    
    delete [] mData;
    mData = nullptr;
    mDataIoPtr = nullptr;
    mDataLength = 0;
    mReadOffset = 0;
    mAmountToRead = 0;
    if (mLookup) {
        mLookup->Cancel(NS_ERROR_FAILURE);
        mLookup = nullptr;
    }
}

PRStatus
nsSOCKSSocketInfo::StartDNS(PRFileDesc *fd)
{
    NS_ABORT_IF_FALSE(!mDnsRec && mState == SOCKS_INITIAL,
                      "Must be in initial state to make DNS Lookup");

    nsCOMPtr<nsIDNSService> dns = do_GetService(NS_DNSSERVICE_CONTRACTID);
    if (!dns)
        return PR_FAILURE;

    mFD  = fd;
    nsresult rv = dns->AsyncResolve(mProxyHost, 0, this,
                                    NS_GetCurrentThread(),
                                    getter_AddRefs(mLookup));

    if (NS_FAILED(rv)) {
        LOGERROR(("socks: DNS lookup for SOCKS proxy %s failed",
                  mProxyHost.get()));
        return PR_FAILURE;
    }
    mState = SOCKS_DNS_IN_PROGRESS;
    PR_SetError(PR_IN_PROGRESS_ERROR, 0);
    return PR_FAILURE;
}

NS_IMETHODIMP
nsSOCKSSocketInfo::OnLookupComplete(nsICancelable *aRequest,
                                    nsIDNSRecord *aRecord,
                                    nsresult aStatus)
{
    NS_ABORT_IF_FALSE(aRequest == mLookup, "wrong DNS query");
    mLookup = nullptr;
    mLookupStatus = aStatus;
    mDnsRec = aRecord;
    mState = SOCKS_DNS_COMPLETE;
    ConnectToProxy(mFD);
    mFD = nullptr;
    return NS_OK;
}

PRStatus
nsSOCKSSocketInfo::ConnectToProxy(PRFileDesc *fd)
{
    PRStatus status;
    nsresult rv;

    NS_ABORT_IF_FALSE(mState == SOCKS_DNS_COMPLETE,
                      "Must have DNS to make connection!");

    if (NS_FAILED(mLookupStatus)) {
        PR_SetError(PR_BAD_ADDRESS_ERROR, 0);
        return PR_FAILURE;
    }

    
    if (mVersion == 4 &&
        PR_NetAddrFamily(&mDestinationAddr) == PR_AF_INET6) {
        mVersion = 5;
    }

    PRInt32 addresses = 0;
    do {
        if (addresses++)
            mDnsRec->ReportUnusable(mProxyPort);
        
        rv = mDnsRec->GetNextAddr(mProxyPort, &mInternalProxyAddr);
        
        if (NS_FAILED(rv)) {
            LOGERROR(("socks: unable to connect to SOCKS proxy, %s",
                     mProxyHost.get()));
            return PR_FAILURE;
        }

#if defined(PR_LOGGING)
        char buf[64];
        PR_NetAddrToString(&mInternalProxyAddr, buf, sizeof(buf));
        LOGDEBUG(("socks: trying proxy server, %s:%hu",
                 buf, PR_ntohs(PR_NetAddrInetPort(&mInternalProxyAddr))));
#endif
        PRNetAddr proxy = mInternalProxyAddr;
        FixupAddressFamily(fd, &proxy);
        status = fd->lower->methods->connect(fd->lower, &proxy, mTimeout);
        if (status != PR_SUCCESS) {
            PRErrorCode c = PR_GetError();
            
            if (c == PR_WOULD_BLOCK_ERROR || c == PR_IN_PROGRESS_ERROR) {
                mState = SOCKS_CONNECTING_TO_PROXY;
                return status;
            }
        }
    } while (status != PR_SUCCESS);

    
    if (mVersion == 4)
        return WriteV4ConnectRequest();
    return WriteV5AuthRequest();
}

void
nsSOCKSSocketInfo::FixupAddressFamily(PRFileDesc *fd, PRNetAddr *proxy)
{
    PRInt32 proxyFamily = PR_NetAddrFamily(&mInternalProxyAddr);
    
    if (proxyFamily == mDestinationFamily) {
        return;
    }
    
    
    if (proxyFamily == PR_AF_INET6 && !ipv6Supported) {
        return;
    }
    
    
    
    if (mDestinationFamily == PR_AF_INET6 && !ipv6Supported) {
        proxy->ipv6.family = PR_AF_INET6;
        proxy->ipv6.port = mInternalProxyAddr.inet.port;
        PRUint8 *proxyp = proxy->ipv6.ip.pr_s6_addr;
        memset(proxyp, 0, 10);
        memset(proxyp + 10, 0xff, 2);
        memcpy(proxyp + 12,(char *) &mInternalProxyAddr.inet.ip, 4);
        
        return;
    }
    
    PROsfd osfd = PR_FileDesc2NativeHandle(fd);
    if (osfd == -1) {
        return;
    }
    
    PRFileDesc *tmpfd = PR_OpenTCPSocket(proxyFamily);
    if (!tmpfd) {
        return;
    }
    PROsfd newsd = PR_FileDesc2NativeHandle(tmpfd);
    if (newsd == -1) {
        PR_Close(tmpfd);
        return;
    }
    
    fd = PR_GetIdentitiesLayer(fd, PR_NSPR_IO_LAYER);
    MOZ_ASSERT(fd);
    
    PR_ChangeFileDescNativeHandle(fd, newsd);
    PR_ChangeFileDescNativeHandle(tmpfd, osfd);
    
    
    PR_Close(tmpfd);
    mDestinationFamily = proxyFamily;
}

PRStatus
nsSOCKSSocketInfo::ContinueConnectingToProxy(PRFileDesc *fd, PRInt16 oflags)
{
    PRStatus status;

    NS_ABORT_IF_FALSE(mState == SOCKS_CONNECTING_TO_PROXY,
                      "Continuing connection in wrong state!");

    LOGDEBUG(("socks: continuing connection to proxy"));

    status = fd->lower->methods->connectcontinue(fd->lower, oflags);
    if (status != PR_SUCCESS) {
        PRErrorCode c = PR_GetError();
        if (c != PR_WOULD_BLOCK_ERROR && c != PR_IN_PROGRESS_ERROR) {
            
            mState = SOCKS_DNS_COMPLETE;
            return ConnectToProxy(fd);
        }

        
        return PR_FAILURE;
    }

    
    if (mVersion == 4)
        return WriteV4ConnectRequest();
    return WriteV5AuthRequest();
}

PRStatus
nsSOCKSSocketInfo::WriteV4ConnectRequest()
{
    PRNetAddr *addr = &mDestinationAddr;
    PRInt32 proxy_resolve;

    NS_ABORT_IF_FALSE(mState == SOCKS_CONNECTING_TO_PROXY,
                      "Invalid state!");
    
    proxy_resolve = mFlags & nsISocketProvider::PROXY_RESOLVES_HOST;

    mDataLength = 0;
    mState = SOCKS4_WRITE_CONNECT_REQUEST;

    LOGDEBUG(("socks4: sending connection request (socks4a resolve? %s)",
             proxy_resolve? "yes" : "no"));

    
    WriteUint8(0x04); 
    WriteUint8(0x01); 
    WriteNetPort(addr);
    if (proxy_resolve) {
        
        
        
        
        
        WriteUint32(PR_htonl(0x00000001)); 
        WriteUint8(0x00); 
        if (mDestinationHost.Length() > MAX_HOSTNAME_LEN) {
            LOGERROR(("socks4: destination host name is too long!"));
            HandshakeFinished(PR_BAD_ADDRESS_ERROR);
            return PR_FAILURE;
        }
        WriteString(mDestinationHost); 
        WriteUint8(0x00);
    } else if (PR_NetAddrFamily(addr) == PR_AF_INET) {
        WriteNetAddr(addr); 
        WriteUint8(0x00); 
    } else if (PR_NetAddrFamily(addr) == PR_AF_INET6) {
        LOGERROR(("socks: SOCKS 4 can't handle IPv6 addresses!"));
        HandshakeFinished(PR_BAD_ADDRESS_ERROR);
        return PR_FAILURE;
    }

    return PR_SUCCESS;
}

PRStatus
nsSOCKSSocketInfo::ReadV4ConnectResponse()
{
    NS_ABORT_IF_FALSE(mState == SOCKS4_READ_CONNECT_RESPONSE,
                      "Handling SOCKS 4 connection reply in wrong state!");
    NS_ABORT_IF_FALSE(mDataLength == 8,
                      "SOCKS 4 connection reply must be 8 bytes!");

    LOGDEBUG(("socks4: checking connection reply"));

    if (ReadUint8() != 0x00) {
        LOGERROR(("socks4: wrong connection reply"));
        HandshakeFinished(PR_CONNECT_REFUSED_ERROR);
        return PR_FAILURE;
    }

    
    if (ReadUint8() == 90) {
        LOGDEBUG(("socks4: connection successful!"));
        HandshakeFinished();
        return PR_SUCCESS;
    }

    LOGERROR(("socks4: unable to connect"));
    HandshakeFinished(PR_CONNECT_REFUSED_ERROR);
    return PR_FAILURE;
}

PRStatus
nsSOCKSSocketInfo::WriteV5AuthRequest()
{
    NS_ABORT_IF_FALSE(mVersion == 5, "SOCKS version must be 5!");

    mState = SOCKS5_WRITE_AUTH_REQUEST;

    
    LOGDEBUG(("socks5: sending auth methods"));
    WriteUint8(0x05); 
    WriteUint8(0x01); 
    WriteUint8(0x00); 

    return PR_SUCCESS;
}

PRStatus
nsSOCKSSocketInfo::ReadV5AuthResponse()
{
    NS_ABORT_IF_FALSE(mState == SOCKS5_READ_AUTH_RESPONSE,
                      "Handling SOCKS 5 auth method reply in wrong state!");
    NS_ABORT_IF_FALSE(mDataLength == 2,
                      "SOCKS 5 auth method reply must be 2 bytes!");

    LOGDEBUG(("socks5: checking auth method reply"));

    
    if (ReadUint8() != 0x05) {
        LOGERROR(("socks5: unexpected version in the reply"));
        HandshakeFinished(PR_CONNECT_REFUSED_ERROR);
        return PR_FAILURE;
    }

    
    if (ReadUint8() != 0x00) {
        LOGERROR(("socks5: server did not accept our authentication method"));
        HandshakeFinished(PR_CONNECT_REFUSED_ERROR);
        return PR_FAILURE;
    }

    return WriteV5ConnectRequest();
}

PRStatus
nsSOCKSSocketInfo::WriteV5ConnectRequest()
{
    
    PRNetAddr *addr = &mDestinationAddr;
    PRInt32 proxy_resolve;
    proxy_resolve = mFlags & nsISocketProvider::PROXY_RESOLVES_HOST;

    LOGDEBUG(("socks5: sending connection request (socks5 resolve? %s)",
             proxy_resolve? "yes" : "no"));

    mDataLength = 0;
    mState = SOCKS5_WRITE_CONNECT_REQUEST;

    WriteUint8(0x05); 
    WriteUint8(0x01); 
    WriteUint8(0x00); 
   
    
    
    if (proxy_resolve) {
        
        
        if (mDestinationHost.Length() > MAX_HOSTNAME_LEN) {
            LOGERROR(("socks5: destination host name is too long!"));
            HandshakeFinished(PR_BAD_ADDRESS_ERROR);
            return PR_FAILURE;
        }
        WriteUint8(0x03); 
        WriteUint8(mDestinationHost.Length()); 
        WriteString(mDestinationHost);
    } else if (PR_NetAddrFamily(addr) == PR_AF_INET) {
        WriteUint8(0x01); 
        WriteNetAddr(addr);
    } else if (PR_NetAddrFamily(addr) == PR_AF_INET6) {
        WriteUint8(0x04); 
        WriteNetAddr(addr);
    } else {
        LOGERROR(("socks5: destination address of unknown type!"));
        HandshakeFinished(PR_BAD_ADDRESS_ERROR);
        return PR_FAILURE;
    }

    WriteNetPort(addr); 

    return PR_SUCCESS;
}

PRStatus
nsSOCKSSocketInfo::ReadV5AddrTypeAndLength(PRUint8 *type, PRUint32 *len)
{
    NS_ABORT_IF_FALSE(mState == SOCKS5_READ_CONNECT_RESPONSE_TOP ||
                      mState == SOCKS5_READ_CONNECT_RESPONSE_BOTTOM,
                      "Invalid state!");
    NS_ABORT_IF_FALSE(mDataLength >= 5,
                      "SOCKS 5 connection reply must be at least 5 bytes!");
 
    
    mReadOffset = 3;
   
    *type = ReadUint8();

    switch (*type) {
        case 0x01: 
            *len = 4 - 1;
            break;
        case 0x04: 
            *len = 16 - 1;
            break;
        case 0x03: 
            *len = ReadUint8();
            break;
        default:   
            LOGERROR(("socks5: wrong address type in connection reply!"));
            return PR_FAILURE;
    }

    return PR_SUCCESS;
}

PRStatus
nsSOCKSSocketInfo::ReadV5ConnectResponseTop()
{
    PRUint8 res;
    PRUint32 len;

    NS_ABORT_IF_FALSE(mState == SOCKS5_READ_CONNECT_RESPONSE_TOP,
                      "Invalid state!");
    NS_ABORT_IF_FALSE(mDataLength == 5,
                      "SOCKS 5 connection reply must be exactly 5 bytes!");

    LOGDEBUG(("socks5: checking connection reply"));

    
    if (ReadUint8() != 0x05) {
        LOGERROR(("socks5: unexpected version in the reply"));
        HandshakeFinished(PR_CONNECT_REFUSED_ERROR);
        return PR_FAILURE;
    }

    
    res = ReadUint8();
    if (res != 0x00) {
        PRErrorCode c = PR_CONNECT_REFUSED_ERROR;

        switch (res) {
            case 0x01:
                LOGERROR(("socks5: connect failed: "
                          "01, General SOCKS server failure."));
                break;
            case 0x02:
                LOGERROR(("socks5: connect failed: "
                          "02, Connection not allowed by ruleset."));
                break;
            case 0x03:
                LOGERROR(("socks5: connect failed: 03, Network unreachable."));
                c = PR_NETWORK_UNREACHABLE_ERROR;
                break;
            case 0x04:
                LOGERROR(("socks5: connect failed: 04, Host unreachable."));
                break;
            case 0x05:
                LOGERROR(("socks5: connect failed: 05, Connection refused."));
                break;
            case 0x06:  
                LOGERROR(("socks5: connect failed: 06, TTL expired."));
                c = PR_CONNECT_TIMEOUT_ERROR;
                break;
            case 0x07:
                LOGERROR(("socks5: connect failed: "
                          "07, Command not supported."));
                break;
            case 0x08:
                LOGERROR(("socks5: connect failed: "
                          "08, Address type not supported."));
                c = PR_BAD_ADDRESS_ERROR;
                break;
            default:
                LOGERROR(("socks5: connect failed."));
                break;
        }

        HandshakeFinished(c);
        return PR_FAILURE;
    }

    if (ReadV5AddrTypeAndLength(&res, &len) != PR_SUCCESS) {
        HandshakeFinished(PR_BAD_ADDRESS_ERROR);
        return PR_FAILURE;
    }

    mState = SOCKS5_READ_CONNECT_RESPONSE_BOTTOM;
    WantRead(len + 2);

    return PR_SUCCESS;
}

PRStatus
nsSOCKSSocketInfo::ReadV5ConnectResponseBottom()
{
    PRUint8 type;
    PRUint32 len;

    NS_ABORT_IF_FALSE(mState == SOCKS5_READ_CONNECT_RESPONSE_BOTTOM,
                      "Invalid state!");

    if (ReadV5AddrTypeAndLength(&type, &len) != PR_SUCCESS) {
        HandshakeFinished(PR_BAD_ADDRESS_ERROR);
        return PR_FAILURE;
    }

    NS_ABORT_IF_FALSE(mDataLength == 7+len,
                      "SOCKS 5 unexpected length of connection reply!");

    LOGDEBUG(("socks5: loading source addr and port"));
    
    switch (type) {
        case 0x01: 
            ReadNetAddr(&mExternalProxyAddr, PR_AF_INET);
            break;
        case 0x04: 
            ReadNetAddr(&mExternalProxyAddr, PR_AF_INET6);
            break;
        case 0x03: 
            mReadOffset += len;
            mExternalProxyAddr.raw.family = PR_AF_INET;
            break;
    }

    ReadNetPort(&mExternalProxyAddr);

    LOGDEBUG(("socks5: connected!"));
    HandshakeFinished();

    return PR_SUCCESS;
}

void
nsSOCKSSocketInfo::SetConnectTimeout(PRIntervalTime to)
{
    mTimeout = to;
}

PRStatus
nsSOCKSSocketInfo::DoHandshake(PRFileDesc *fd, PRInt16 oflags)
{
    LOGDEBUG(("socks: DoHandshake(), state = %d", mState));

    switch (mState) {
        case SOCKS_INITIAL:
            return StartDNS(fd);
        case SOCKS_DNS_IN_PROGRESS:
            PR_SetError(PR_IN_PROGRESS_ERROR, 0);
            return PR_FAILURE;
        case SOCKS_DNS_COMPLETE:
            return ConnectToProxy(fd);
        case SOCKS_CONNECTING_TO_PROXY:
            return ContinueConnectingToProxy(fd, oflags);
        case SOCKS4_WRITE_CONNECT_REQUEST:
            if (WriteToSocket(fd) != PR_SUCCESS)
                return PR_FAILURE;
            WantRead(8);
            mState = SOCKS4_READ_CONNECT_RESPONSE;
            return PR_SUCCESS;
        case SOCKS4_READ_CONNECT_RESPONSE:
            if (ReadFromSocket(fd) != PR_SUCCESS)
                return PR_FAILURE;
            return ReadV4ConnectResponse();

        case SOCKS5_WRITE_AUTH_REQUEST:
            if (WriteToSocket(fd) != PR_SUCCESS)
                return PR_FAILURE;
            WantRead(2);
            mState = SOCKS5_READ_AUTH_RESPONSE;
            return PR_SUCCESS;
        case SOCKS5_READ_AUTH_RESPONSE:
            if (ReadFromSocket(fd) != PR_SUCCESS)
                return PR_FAILURE;
            return ReadV5AuthResponse();
        case SOCKS5_WRITE_CONNECT_REQUEST:
            if (WriteToSocket(fd) != PR_SUCCESS)
                return PR_FAILURE;

            
            
            
            WantRead(5);
            mState = SOCKS5_READ_CONNECT_RESPONSE_TOP;
            return PR_SUCCESS;
        case SOCKS5_READ_CONNECT_RESPONSE_TOP:
            if (ReadFromSocket(fd) != PR_SUCCESS)
                return PR_FAILURE;
            return ReadV5ConnectResponseTop();
        case SOCKS5_READ_CONNECT_RESPONSE_BOTTOM:
            if (ReadFromSocket(fd) != PR_SUCCESS)
                return PR_FAILURE;
            return ReadV5ConnectResponseBottom();

        case SOCKS_CONNECTED:
            LOGERROR(("socks: already connected"));
            HandshakeFinished(PR_IS_CONNECTED_ERROR);
            return PR_FAILURE;
        case SOCKS_FAILED:
            LOGERROR(("socks: already failed"));
            return PR_FAILURE;
    }

    LOGERROR(("socks: executing handshake in invalid state, %d", mState));
    HandshakeFinished(PR_INVALID_STATE_ERROR);

    return PR_FAILURE;
}

PRInt16
nsSOCKSSocketInfo::GetPollFlags() const
{
    switch (mState) {
        case SOCKS_DNS_IN_PROGRESS:
        case SOCKS_DNS_COMPLETE:
        case SOCKS_CONNECTING_TO_PROXY:
            return PR_POLL_EXCEPT | PR_POLL_WRITE;
        case SOCKS4_WRITE_CONNECT_REQUEST:
        case SOCKS5_WRITE_AUTH_REQUEST:
        case SOCKS5_WRITE_CONNECT_REQUEST:
            return PR_POLL_WRITE;
        case SOCKS4_READ_CONNECT_RESPONSE:
        case SOCKS5_READ_AUTH_RESPONSE:
        case SOCKS5_READ_CONNECT_RESPONSE_TOP:
        case SOCKS5_READ_CONNECT_RESPONSE_BOTTOM:
            return PR_POLL_READ;
        default:
            break;
    }

    return 0;
}

inline void
nsSOCKSSocketInfo::WriteUint8(PRUint8 v)
{
    NS_ABORT_IF_FALSE(mDataLength + sizeof(v) <= BUFFER_SIZE,
                      "Can't write that much data!");
    mData[mDataLength] = v;
    mDataLength += sizeof(v);
}

inline void
nsSOCKSSocketInfo::WriteUint16(PRUint16 v)
{
    NS_ABORT_IF_FALSE(mDataLength + sizeof(v) <= BUFFER_SIZE,
                      "Can't write that much data!");
    memcpy(mData + mDataLength, &v, sizeof(v));
    mDataLength += sizeof(v);
}

inline void
nsSOCKSSocketInfo::WriteUint32(PRUint32 v)
{
    NS_ABORT_IF_FALSE(mDataLength + sizeof(v) <= BUFFER_SIZE,
                      "Can't write that much data!");
    memcpy(mData + mDataLength, &v, sizeof(v));
    mDataLength += sizeof(v);
}

void
nsSOCKSSocketInfo::WriteNetAddr(const PRNetAddr *addr)
{
    const char *ip = NULL;
    PRUint32 len = 0;

    if (PR_NetAddrFamily(addr) == PR_AF_INET) {
        ip = (const char*)&addr->inet.ip;
        len = sizeof(addr->inet.ip);
    } else if (PR_NetAddrFamily(addr) == PR_AF_INET6) {
        ip = (const char*)addr->ipv6.ip.pr_s6_addr;
        len = sizeof(addr->ipv6.ip.pr_s6_addr);
    }

    NS_ABORT_IF_FALSE(ip != NULL, "Unknown address");
    NS_ABORT_IF_FALSE(mDataLength + len <= BUFFER_SIZE,
                      "Can't write that much data!");
 
    memcpy(mData + mDataLength, ip, len);
    mDataLength += len;
}

void
nsSOCKSSocketInfo::WriteNetPort(const PRNetAddr *addr)
{
    WriteUint16(PR_NetAddrInetPort(addr));
}

void
nsSOCKSSocketInfo::WriteString(const nsACString &str)
{
    NS_ABORT_IF_FALSE(mDataLength + str.Length() <= BUFFER_SIZE,
                      "Can't write that much data!");
    memcpy(mData + mDataLength, str.Data(), str.Length());
    mDataLength += str.Length();
}

inline PRUint8
nsSOCKSSocketInfo::ReadUint8()
{
    PRUint8 rv;
    NS_ABORT_IF_FALSE(mReadOffset + sizeof(rv) <= mDataLength,
                      "Not enough space to pop a uint8!");
    rv = mData[mReadOffset];
    mReadOffset += sizeof(rv);
    return rv;
}

inline PRUint16
nsSOCKSSocketInfo::ReadUint16()
{
    PRUint16 rv;
    NS_ABORT_IF_FALSE(mReadOffset + sizeof(rv) <= mDataLength,
                      "Not enough space to pop a uint16!");
    memcpy(&rv, mData + mReadOffset, sizeof(rv));
    mReadOffset += sizeof(rv);
    return rv;
}

inline PRUint32
nsSOCKSSocketInfo::ReadUint32()
{
    PRUint32 rv;
    NS_ABORT_IF_FALSE(mReadOffset + sizeof(rv) <= mDataLength,
                      "Not enough space to pop a uint32!");
    memcpy(&rv, mData + mReadOffset, sizeof(rv));
    mReadOffset += sizeof(rv);
    return rv;
}

void
nsSOCKSSocketInfo::ReadNetAddr(PRNetAddr *addr, PRUint16 fam)
{
    PRUint32 amt;
    const PRUint8 *ip = mData + mReadOffset;

    addr->raw.family = fam;
    if (fam == PR_AF_INET) {
        amt = sizeof(addr->inet.ip);
        NS_ABORT_IF_FALSE(mReadOffset + amt <= mDataLength,
                          "Not enough space to pop an ipv4 addr!");
        memcpy(&addr->inet.ip, ip, amt);
    } else if (fam == PR_AF_INET6) {
        amt = sizeof(addr->ipv6.ip.pr_s6_addr);
        NS_ABORT_IF_FALSE(mReadOffset + amt <= mDataLength,
                          "Not enough space to pop an ipv6 addr!");
        memcpy(addr->ipv6.ip.pr_s6_addr, ip, amt);
    }

    mReadOffset += amt;
}

void
nsSOCKSSocketInfo::ReadNetPort(PRNetAddr *addr)
{
    addr->inet.port = ReadUint16();
}

void
nsSOCKSSocketInfo::WantRead(PRUint32 sz)
{
    NS_ABORT_IF_FALSE(mDataIoPtr == NULL,
                      "WantRead() called while I/O already in progress!");
    NS_ABORT_IF_FALSE(mDataLength + sz <= BUFFER_SIZE,
                      "Can't read that much data!");
    mAmountToRead = sz;
}

PRStatus
nsSOCKSSocketInfo::ReadFromSocket(PRFileDesc *fd)
{
    PRInt32 rc;
    const PRUint8 *end;

    if (!mAmountToRead) {
        LOGDEBUG(("socks: ReadFromSocket(), nothing to do"));
        return PR_SUCCESS;
    }

    if (!mDataIoPtr) {
        mDataIoPtr = mData + mDataLength;
        mDataLength += mAmountToRead;
    }

    end = mData + mDataLength;

    while (mDataIoPtr < end) {
        rc = PR_Read(fd, mDataIoPtr, end - mDataIoPtr);
        if (rc <= 0) {
            if (rc == 0) {
                LOGERROR(("socks: proxy server closed connection"));
                HandshakeFinished(PR_CONNECT_REFUSED_ERROR);
                return PR_FAILURE;
            } else if (PR_GetError() == PR_WOULD_BLOCK_ERROR) {
                LOGDEBUG(("socks: ReadFromSocket(), want read"));
            }
            break;
        }

        mDataIoPtr += rc;
    }

    LOGDEBUG(("socks: ReadFromSocket(), have %u bytes total",
             unsigned(mDataIoPtr - mData)));
    if (mDataIoPtr == end) {
        mDataIoPtr = nullptr;
        mAmountToRead = 0;
        mReadOffset = 0;
        return PR_SUCCESS;
    }

    return PR_FAILURE;
}

PRStatus
nsSOCKSSocketInfo::WriteToSocket(PRFileDesc *fd)
{
    PRInt32 rc;
    const PRUint8 *end;

    if (!mDataLength) {
        LOGDEBUG(("socks: WriteToSocket(), nothing to do"));
        return PR_SUCCESS;
    }

    if (!mDataIoPtr)
        mDataIoPtr = mData;

    end = mData + mDataLength;

    while (mDataIoPtr < end) {
        rc = PR_Write(fd, mDataIoPtr, end - mDataIoPtr);
        if (rc < 0) {
            if (PR_GetError() == PR_WOULD_BLOCK_ERROR) {
                LOGDEBUG(("socks: WriteToSocket(), want write"));
            }
            break;
        }
        
        mDataIoPtr += rc;
    }

    if (mDataIoPtr == end) {
        mDataIoPtr = nullptr;
        mDataLength = 0;
        mReadOffset = 0;
        return PR_SUCCESS;
    }
    
    return PR_FAILURE;
}

static PRStatus
nsSOCKSIOLayerConnect(PRFileDesc *fd, const PRNetAddr *addr, PRIntervalTime to)
{
    PRStatus status;
    PRNetAddr dst;

    nsSOCKSSocketInfo * info = (nsSOCKSSocketInfo*) fd->secret;
    if (info == NULL) return PR_FAILURE;

    if (PR_NetAddrFamily(addr) == PR_AF_INET6 &&
        PR_IsNetAddrType(addr, PR_IpAddrV4Mapped)) {
        const PRUint8 *srcp;

        LOGDEBUG(("socks: converting ipv4-mapped ipv6 address to ipv4"));

        
        PR_InitializeNetAddr(PR_IpAddrAny, 0, &dst);
        srcp = addr->ipv6.ip.pr_s6_addr;
        memcpy(&dst.inet.ip, srcp + 12, 4);
        dst.inet.family = PR_AF_INET;
        dst.inet.port = addr->ipv6.port;
    } else {
        memcpy(&dst, addr, sizeof(dst));
    }

    info->SetDestinationAddr(&dst);
    info->SetConnectTimeout(to);

    do {
        status = info->DoHandshake(fd, -1);
    } while (status == PR_SUCCESS && !info->IsConnected());

    return status;
}

static PRStatus
nsSOCKSIOLayerConnectContinue(PRFileDesc *fd, PRInt16 oflags)
{
    PRStatus status;

    nsSOCKSSocketInfo * info = (nsSOCKSSocketInfo*) fd->secret;
    if (info == NULL) return PR_FAILURE;

    do { 
        status = info->DoHandshake(fd, oflags);
    } while (status == PR_SUCCESS && !info->IsConnected());

    return status;
}

static PRInt16
nsSOCKSIOLayerPoll(PRFileDesc *fd, PRInt16 in_flags, PRInt16 *out_flags)
{
    nsSOCKSSocketInfo * info = (nsSOCKSSocketInfo*) fd->secret;
    if (info == NULL) return PR_FAILURE;

    if (!info->IsConnected()) {
        *out_flags = 0;
        return info->GetPollFlags();
    }

    return fd->lower->methods->poll(fd->lower, in_flags, out_flags);
}

static PRStatus
nsSOCKSIOLayerClose(PRFileDesc *fd)
{
    nsSOCKSSocketInfo * info = (nsSOCKSSocketInfo*) fd->secret;
    PRDescIdentity id = PR_GetLayersIdentity(fd);

    if (info && id == nsSOCKSIOLayerIdentity)
    {
        NS_RELEASE(info);
        fd->identity = PR_INVALID_IO_LAYER;
    }

    return fd->lower->methods->close(fd->lower);
}

static PRFileDesc*
nsSOCKSIOLayerAccept(PRFileDesc *fd, PRNetAddr *addr, PRIntervalTime timeout)
{
    
    return fd->lower->methods->accept(fd->lower, addr, timeout);
}

static PRInt32
nsSOCKSIOLayerAcceptRead(PRFileDesc *sd, PRFileDesc **nd, PRNetAddr **raddr, void *buf, PRInt32 amount, PRIntervalTime timeout)
{
    
    return sd->lower->methods->acceptread(sd->lower, nd, raddr, buf, amount, timeout);
}

static PRStatus
nsSOCKSIOLayerBind(PRFileDesc *fd, const PRNetAddr *addr)
{
    
    return fd->lower->methods->bind(fd->lower, addr);
}

static PRStatus
nsSOCKSIOLayerGetName(PRFileDesc *fd, PRNetAddr *addr)
{
    nsSOCKSSocketInfo * info = (nsSOCKSSocketInfo*) fd->secret;
    
    if (info != NULL && addr != NULL) {
        if (info->GetExternalProxyAddr(&addr) == NS_OK)
            return PR_SUCCESS;
    }

    return PR_FAILURE;
}

static PRStatus
nsSOCKSIOLayerGetPeerName(PRFileDesc *fd, PRNetAddr *addr)
{
    nsSOCKSSocketInfo * info = (nsSOCKSSocketInfo*) fd->secret;

    if (info != NULL && addr != NULL) {
        if (info->GetDestinationAddr(&addr) == NS_OK)
            return PR_SUCCESS;
    }

    return PR_FAILURE;
}

static PRStatus
nsSOCKSIOLayerListen(PRFileDesc *fd, PRIntn backlog)
{
    
    return fd->lower->methods->listen(fd->lower, backlog);
}


nsresult
nsSOCKSIOLayerAddToSocket(PRInt32 family,
                          const char *host, 
                          PRInt32 port,
                          const char *proxyHost,
                          PRInt32 proxyPort,
                          PRInt32 socksVersion,
                          PRUint32 flags,
                          PRFileDesc *fd, 
                          nsISupports** info)
{
    NS_ENSURE_TRUE((socksVersion == 4) || (socksVersion == 5), NS_ERROR_NOT_INITIALIZED);


    if (firstTime)
    {
        
        
        PRFileDesc *tmpfd = PR_OpenTCPSocket(PR_AF_INET6);
        if (!tmpfd) {
            ipv6Supported = false;
        } else {
            
            
            ipv6Supported = PR_GetIdentitiesLayer(tmpfd, PR_NSPR_IO_LAYER) == tmpfd;
            PR_Close(tmpfd);
        }

        nsSOCKSIOLayerIdentity		= PR_GetUniqueIdentity("SOCKS layer");
        nsSOCKSIOLayerMethods		= *PR_GetDefaultIOMethods();

        nsSOCKSIOLayerMethods.connect	= nsSOCKSIOLayerConnect;
        nsSOCKSIOLayerMethods.connectcontinue	= nsSOCKSIOLayerConnectContinue;
        nsSOCKSIOLayerMethods.poll	= nsSOCKSIOLayerPoll;
        nsSOCKSIOLayerMethods.bind	= nsSOCKSIOLayerBind;
        nsSOCKSIOLayerMethods.acceptread = nsSOCKSIOLayerAcceptRead;
        nsSOCKSIOLayerMethods.getsockname = nsSOCKSIOLayerGetName;
        nsSOCKSIOLayerMethods.getpeername = nsSOCKSIOLayerGetPeerName;
        nsSOCKSIOLayerMethods.accept	= nsSOCKSIOLayerAccept;
        nsSOCKSIOLayerMethods.listen	= nsSOCKSIOLayerListen;
        nsSOCKSIOLayerMethods.close	= nsSOCKSIOLayerClose;

        firstTime			= false;

#if defined(PR_LOGGING)
        gSOCKSLog = PR_NewLogModule("SOCKS");
#endif

    }

    LOGDEBUG(("Entering nsSOCKSIOLayerAddToSocket()."));

    PRFileDesc *	layer;
    PRStatus	rv;

    layer = PR_CreateIOLayerStub(nsSOCKSIOLayerIdentity, &nsSOCKSIOLayerMethods);
    if (! layer)
    {
        LOGERROR(("PR_CreateIOLayerStub() failed."));
        return NS_ERROR_FAILURE;
    }

    nsSOCKSSocketInfo * infoObject = new nsSOCKSSocketInfo();
    if (!infoObject)
    {
        
        LOGERROR(("Failed to create nsSOCKSSocketInfo()."));
        PR_DELETE(layer);
        return NS_ERROR_FAILURE;
    }

    NS_ADDREF(infoObject);
    infoObject->Init(socksVersion, family, proxyHost, proxyPort, host, flags);
    layer->secret = (PRFilePrivate*) infoObject;
    rv = PR_PushIOLayer(fd, PR_GetLayersIdentity(fd), layer);

    if (NS_FAILED(rv))
    {
        LOGERROR(("PR_PushIOLayer() failed. rv = %x.", rv));
        NS_RELEASE(infoObject);
        PR_DELETE(layer);
        return NS_ERROR_FAILURE;
    }

    *info = static_cast<nsISOCKSSocketInfo*>(infoObject);
    NS_ADDREF(*info);
    return NS_OK;
}
