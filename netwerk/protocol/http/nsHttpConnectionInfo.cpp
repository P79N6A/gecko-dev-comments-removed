






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

nsHttpConnectionInfo::nsHttpConnectionInfo(const nsACString &originHost,
                                           int32_t originPort,
                                           const nsACString &npnToken,
                                           const nsACString &username,
                                           nsProxyInfo *proxyInfo,
                                           bool endToEndSSL)
    : mRoutedPort(443)
{
    Init(originHost, originPort, npnToken, username, proxyInfo, endToEndSSL);
}

nsHttpConnectionInfo::nsHttpConnectionInfo(const nsACString &originHost,
                                           int32_t originPort,
                                           const nsACString &npnToken,
                                           const nsACString &username,
                                           nsProxyInfo *proxyInfo,
                                           const nsACString &routedHost,
                                           int32_t routedPort)

{
    mEndToEndSSL = true; 
    mRoutedPort = routedPort == -1 ? DefaultPort() : routedPort;

    if (!originHost.Equals(routedHost) || (originPort != routedPort)) {
        mRoutedHost = routedHost;
    }
    Init(originHost, originPort, npnToken, username, proxyInfo, true);
}

void
nsHttpConnectionInfo::Init(const nsACString &host, int32_t port,
                           const nsACString &npnToken,
                           const nsACString &username,
                           nsProxyInfo* proxyInfo,
                           bool e2eSSL)
{
    LOG(("Init nsHttpConnectionInfo @%p\n", this));

    mUsername = username;
    mProxyInfo = proxyInfo;
    mEndToEndSSL = e2eSSL;
    mUsingConnect = false;
    mNPNToken = npnToken;

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
nsHttpConnectionInfo::SetNetworkInterfaceId(const nsACString& aNetworkInterfaceId)
{
    mNetworkInterfaceId = aNetworkInterfaceId;
    BuildHashKey();
}

void nsHttpConnectionInfo::BuildHashKey()
{
    
    
    
    
    
    
    
    
    

    const char *keyHost;
    int32_t keyPort;

    if (mUsingHttpProxy && !mUsingConnect) {
        keyHost = ProxyHost();
        keyPort = ProxyPort();
    } else {
        keyHost = Origin();
        keyPort = OriginPort();
    }

    
    
    
    
    
    
    

    mHashKey.AssignLiteral("......");
    mHashKey.Append(keyHost);
    if (!mNetworkInterfaceId.IsEmpty()) {
        mHashKey.Append('(');
        mHashKey.Append(mNetworkInterfaceId);
        mHashKey.Append(')');
    }
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

    if(!mRoutedHost.IsEmpty()) {
        mHashKey.AppendLiteral(" <ROUTE-via ");
        mHashKey.Append(mRoutedHost);
        mHashKey.Append(':');
        mHashKey.AppendInt(mRoutedPort);
        mHashKey.Append('>');
    }

    if (!mNPNToken.IsEmpty()) {
        mHashKey.AppendLiteral(" {NPN-TOKEN ");
        mHashKey.Append(mNPNToken);
        mHashKey.AppendLiteral("}");
    }
}

void
nsHttpConnectionInfo::SetOriginServer(const nsACString &host, int32_t port)
{
    mOrigin = host;
    mOriginPort = port == -1 ? DefaultPort() : port;
    BuildHashKey();
}

nsHttpConnectionInfo*
nsHttpConnectionInfo::Clone() const
{
    nsHttpConnectionInfo *clone;
    if (mRoutedHost.IsEmpty()) {
        clone = new nsHttpConnectionInfo(mOrigin, mOriginPort, mNPNToken, mUsername, mProxyInfo, mEndToEndSSL);
    } else {
        MOZ_ASSERT(mEndToEndSSL);
        clone = new nsHttpConnectionInfo(mOrigin, mOriginPort, mNPNToken, mUsername, mProxyInfo,
                                         mRoutedHost, mRoutedPort);
    }

    if (!mNetworkInterfaceId.IsEmpty()) {
        clone->SetNetworkInterfaceId(mNetworkInterfaceId);
    }

    
    clone->SetAnonymous(GetAnonymous());
    clone->SetPrivate(GetPrivate());
    clone->SetInsecureScheme(GetInsecureScheme());
    clone->SetNoSpdy(GetNoSpdy());
    MOZ_ASSERT(clone->Equals(this));

    return clone;
}

void
nsHttpConnectionInfo::CloneAsDirectRoute(nsHttpConnectionInfo **outCI)
{
    if (mRoutedHost.IsEmpty()) {
        *outCI = Clone();
        return;
    }

    nsRefPtr<nsHttpConnectionInfo> clone =
        new nsHttpConnectionInfo(mOrigin, mOriginPort,
                                 EmptyCString(), mUsername, mProxyInfo, mEndToEndSSL);
    
    clone->SetAnonymous(GetAnonymous());
    clone->SetPrivate(GetPrivate());
    clone->SetInsecureScheme(GetInsecureScheme());
    clone->SetNoSpdy(GetNoSpdy());
    if (!mNetworkInterfaceId.IsEmpty()) {
        clone->SetNetworkInterfaceId(mNetworkInterfaceId);
    }
    clone.forget(outCI);
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
                                     mNPNToken, mUsername, mProxyInfo, true);
    
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
    } else if (PR_StringToNetAddr(Origin(), &prAddr) != PR_SUCCESS) {
        return false;
    }
    NetAddr netAddr;
    PRNetAddrToNetAddr(&prAddr, &netAddr);
    return IsIPAddrLocal(&netAddr);
}

} 
} 
