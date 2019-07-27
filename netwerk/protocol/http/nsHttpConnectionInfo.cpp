






#include "HttpLog.h"


#undef LOG
#define LOG(args) LOG5(args)
#undef LOG_ENABLED
#define LOG_ENABLED() LOG5_ENABLED()

#include "nsHttpConnectionInfo.h"
#include "mozilla/net/DNS.h"
#include "prnetdb.h"

namespace mozilla {
namespace net {

nsHttpConnectionInfo::nsHttpConnectionInfo(const nsACString &host, int32_t port,
                                           const nsACString &username,
                                           nsProxyInfo* proxyInfo,
                                           bool endToEndSSL)
    : mUsername(username)
    , mProxyInfo(proxyInfo)
    , mEndToEndSSL(endToEndSSL)
    , mUsingConnect(false)
{
    LOG(("Creating nsHttpConnectionInfo @%x\n", this));

    mUsingHttpsProxy = (proxyInfo && proxyInfo->IsHTTPS());
    mUsingHttpProxy = mUsingHttpsProxy || (proxyInfo && proxyInfo->IsHTTP());

    if (mUsingHttpProxy) {
        mUsingConnect = mEndToEndSSL;  
        uint32_t resolveFlags = 0;
        if (NS_SUCCEEDED(mProxyInfo->GetResolveFlags(&resolveFlags)) &&
            resolveFlags & nsIProtocolProxyService::RESOLVE_ALWAYS_TUNNEL) {
            mUsingConnect = true;
        }
    }

    SetOriginServer(host, port);
}

void
nsHttpConnectionInfo::SetOriginServer(const nsACString &host, int32_t port)
{
    mHost = host;
    mPort = port == -1 ? DefaultPort() : port;

    
    
    
    
    
    
    
    
    

    const char *keyHost;
    int32_t keyPort;

    if (mUsingHttpProxy && !mUsingConnect) {
        keyHost = ProxyHost();
        keyPort = ProxyPort();
    } else {
        keyHost = Host();
        keyPort = Port();
    }

    
    
    
    
    
    mHashKey.AssignLiteral("....");

    mHashKey.Append(keyHost);
    mHashKey.Append(':');
    mHashKey.AppendInt(keyPort);
    if (!mUsername.IsEmpty()) {
        mHashKey.Append('[');
        mHashKey.Append(mUsername);
        mHashKey.Append(']');
    }

    if (mUsingHttpsProxy) {
        mHashKey.SetCharAt('T', 0);
    } else if (mUsingHttpProxy) {
        mHashKey.SetCharAt('P', 0);
    }
    if (mEndToEndSSL) {
        mHashKey.SetCharAt('S', 1);
    }

    
    
    
    
    
    
    
    
    

    if ((!mUsingHttpProxy && ProxyHost()) ||
        (mUsingHttpProxy && mUsingConnect)) {
        mHashKey.AppendLiteral(" (");
        mHashKey.Append(ProxyType());
        mHashKey.Append(':');
        mHashKey.Append(ProxyHost());
        mHashKey.Append(':');
        mHashKey.AppendInt(ProxyPort());
        mHashKey.Append(')');
    }
}

nsHttpConnectionInfo*
nsHttpConnectionInfo::Clone() const
{
    nsHttpConnectionInfo* clone = new nsHttpConnectionInfo(mHost, mPort, mUsername, mProxyInfo, mEndToEndSSL);

    
    clone->SetAnonymous(GetAnonymous());
    clone->SetPrivate(GetPrivate());
    MOZ_ASSERT(clone->Equals(this));
    return clone;
}

nsresult
nsHttpConnectionInfo::CreateWildCard(nsHttpConnectionInfo **outParam)
{
    
    

    if (!mUsingHttpsProxy) {
        MOZ_ASSERT(false);
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    nsRefPtr<nsHttpConnectionInfo> clone;
    clone = new nsHttpConnectionInfo(NS_LITERAL_CSTRING("*"), 0,
                                     mUsername, mProxyInfo, true);
    
    clone->SetAnonymous(GetAnonymous());
    clone->SetPrivate(GetPrivate());
    clone.forget(outParam);
    return NS_OK;
}

bool
nsHttpConnectionInfo::UsingProxy()
{
    if (!mProxyInfo)
        return false;
    return !mProxyInfo->IsDirect();
}

bool
nsHttpConnectionInfo::HostIsLocalIPLiteral() const
{
    PRNetAddr prAddr;
    
    if (ProxyHost()) {
        if (PR_StringToNetAddr(ProxyHost(), &prAddr) != PR_SUCCESS) {
          return false;
        }
    } else if (PR_StringToNetAddr(Host(), &prAddr) != PR_SUCCESS) {
        return false;
    }
    NetAddr netAddr;
    PRNetAddrToNetAddr(&prAddr, &netAddr);
    return IsIPAddrLocal(&netAddr);
}

} 
} 
