





































#include "nsHttpConnectionInfo.h"
#include "nsPrintfCString.h"

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

    mHashKey.AssignLiteral("..");
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
