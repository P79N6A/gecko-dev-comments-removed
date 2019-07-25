





































#include "nsHttpConnectionInfo.h"
#include "nsPrintfCString.h"
#include "nsIProtocolProxyService.h"

void
nsHttpConnectionInfo::SetOriginServer(const nsACString &host, PRInt32 port)
{
    mHost = host;
    mPort = port == -1 ? DefaultPort() : port;

    
    
    
    
    
    
    
    
    

    const char *keyHost;
    PRInt32 keyPort;

    if (mUsingHttpProxy && !mUsingSSL) {
        keyHost = ProxyHost();
        keyPort = ProxyPort();
    }
    else {
        keyHost = Host();
        keyPort = Port();
    }

    mHashKey.AssignLiteral("...");
    mHashKey.Append(keyHost);
    mHashKey.Append(':');
    mHashKey.AppendInt(keyPort);

    if (mUsingHttpProxy)
        mHashKey.SetCharAt('P', 0);
    if (mUsingSSL)
        mHashKey.SetCharAt('S', 1);

    
    
    
    if (!mUsingHttpProxy && ProxyHost()) {
        mHashKey.AppendLiteral(" (");
        mHashKey.Append(ProxyType());
        mHashKey.Append(')');
    }
}

nsHttpConnectionInfo*
nsHttpConnectionInfo::Clone() const
{
    nsHttpConnectionInfo* clone = new nsHttpConnectionInfo(mHost, mPort, mProxyInfo, mUsingSSL);
    if (!clone)
        return nsnull;

    
    clone->SetAnonymous(mHashKey.CharAt(2) == 'A');
    
    return clone;
}

PRBool
nsHttpConnectionInfo::ShouldForceConnectMethod()
{
    if (!mProxyInfo)
        return PR_FALSE;
    
    PRUint32 resolveFlags;
    nsresult rv;
    
    rv = mProxyInfo->GetResolveFlags(&resolveFlags);
    if (NS_FAILED(rv))
        return PR_FALSE;

    return resolveFlags & nsIProtocolProxyService::RESOLVE_ALWAYS_TUNNEL;
}
