





#ifndef nsHttpConnectionInfo_h__
#define nsHttpConnectionInfo_h__

#include "nsHttp.h"
#include "nsProxyInfo.h"
#include "nsCOMPtr.h"
#include "nsStringFwd.h"

extern PRLogModuleInfo *gHttpLog;














namespace mozilla { namespace net {

class nsHttpConnectionInfo
{
public:
    nsHttpConnectionInfo(const nsACString &host, int32_t port,
                         const nsACString &username,
                         nsProxyInfo* proxyInfo,
                         bool endToEndSSL = false);

    virtual ~nsHttpConnectionInfo()
    {
        PR_LOG(gHttpLog, 4, ("Destroying nsHttpConnectionInfo @%x\n", this));
    }

    const nsAFlatCString &HashKey() const { return mHashKey; }

    void SetOriginServer(const nsACString &host, int32_t port);

    void SetOriginServer(const char *host, int32_t port)
    {
        SetOriginServer(nsDependentCString(host), port);
    }

    
    nsHttpConnectionInfo* Clone() const;
    nsresult CreateWildCard(nsHttpConnectionInfo **outParam);

    const char *ProxyHost() const { return mProxyInfo ? mProxyInfo->Host().get() : nullptr; }
    int32_t     ProxyPort() const { return mProxyInfo ? mProxyInfo->Port() : -1; }
    const char *ProxyType() const { return mProxyInfo ? mProxyInfo->Type() : nullptr; }

    
    
    
    
    
    
    
    bool Equals(const nsHttpConnectionInfo *info)
    {
        return mHashKey.Equals(info->HashKey());
    }

    const char   *Host() const           { return mHost.get(); }
    int32_t       Port() const           { return mPort; }
    const char   *Username() const       { return mUsername.get(); }
    nsProxyInfo  *ProxyInfo()            { return mProxyInfo; }
    int32_t       DefaultPort() const    { return mEndToEndSSL ? NS_HTTPS_DEFAULT_PORT : NS_HTTP_DEFAULT_PORT; }
    void          SetAnonymous(bool anon)
                                         { mHashKey.SetCharAt(anon ? 'A' : '.', 2); }
    bool          GetAnonymous() const   { return mHashKey.CharAt(2) == 'A'; }
    void          SetPrivate(bool priv)  { mHashKey.SetCharAt(priv ? 'P' : '.', 3); }
    bool          GetPrivate() const     { return mHashKey.CharAt(3) == 'P'; }

    const nsCString &GetHost() { return mHost; }

    
    bool UsingProxy();

    
    bool UsingHttpProxy() const { return mUsingHttpProxy || mUsingHttpsProxy; }

    
    bool UsingHttpsProxy() const { return mUsingHttpsProxy; }

    
    bool EndToEndSSL() const { return mEndToEndSSL; }

    
    bool FirstHopSSL() const { return mEndToEndSSL || mUsingHttpsProxy; }

    
    bool UsingConnect() const { return mUsingConnect; }

    
    bool HostIsLocalIPLiteral() const;

private:
    nsCString              mHashKey;
    nsCString              mHost;
    int32_t                mPort;
    nsCString              mUsername;
    nsCOMPtr<nsProxyInfo>  mProxyInfo;
    bool                   mUsingHttpProxy;
    bool                   mUsingHttpsProxy;
    bool                   mEndToEndSSL;
    bool                   mUsingConnect;  


    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(nsHttpConnectionInfo)
};

}} 

#endif
