





#ifndef nsHttpConnectionInfo_h__
#define nsHttpConnectionInfo_h__

#include "nsHttp.h"
#include "nsProxyInfo.h"
#include "nsCOMPtr.h"
#include "nsStringFwd.h"
#include "mozilla/Logging.h"

extern PRLogModuleInfo *gHttpLog;














namespace mozilla { namespace net {

class nsHttpConnectionInfo
{
public:
    nsHttpConnectionInfo(const nsACString &originHost,
                         int32_t originPort,
                         const nsACString &npnToken,
                         const nsACString &username,
                         nsProxyInfo *proxyInfo,
                         bool endToEndSSL = false);

    
    
    
    nsHttpConnectionInfo(const nsACString &originHost,
                         int32_t originPort,
                         const nsACString &npnToken,
                         const nsACString &username,
                         nsProxyInfo *proxyInfo,
                         const nsACString &routedHost,
                         int32_t routedPort);

private:
    virtual ~nsHttpConnectionInfo()
    {
        MOZ_LOG(gHttpLog, LogLevel::Debug, ("Destroying nsHttpConnectionInfo @%x\n", this));
    }

    void BuildHashKey();

public:
    const nsAFlatCString &HashKey() const { return mHashKey; }

    const nsCString &GetOrigin() const { return mOrigin; }
    const char   *Origin()       const { return mOrigin.get(); }
    int32_t       OriginPort()   const { return mOriginPort; }

    const nsCString &GetRoutedHost() const { return mRoutedHost; }
    const char      *RoutedHost() const { return mRoutedHost.get(); }
    int32_t          RoutedPort() const { return mRoutedPort; }

    
    
    
    void SetNetworkInterfaceId(const nsACString& aNetworkInterfaceId);

    
    nsHttpConnectionInfo* Clone() const;
    void CloneAsDirectRoute(nsHttpConnectionInfo **outParam);
    nsresult CreateWildCard(nsHttpConnectionInfo **outParam);

    const char *ProxyHost() const { return mProxyInfo ? mProxyInfo->Host().get() : nullptr; }
    int32_t     ProxyPort() const { return mProxyInfo ? mProxyInfo->Port() : -1; }
    const char *ProxyType() const { return mProxyInfo ? mProxyInfo->Type() : nullptr; }

    
    
    
    
    
    
    
    bool Equals(const nsHttpConnectionInfo *info)
    {
        return mHashKey.Equals(info->HashKey());
    }

    const char   *Username() const       { return mUsername.get(); }
    nsProxyInfo  *ProxyInfo() const      { return mProxyInfo; }
    int32_t       DefaultPort() const    { return mEndToEndSSL ? NS_HTTPS_DEFAULT_PORT : NS_HTTP_DEFAULT_PORT; }
    void          SetAnonymous(bool anon)
                                         { mHashKey.SetCharAt(anon ? 'A' : '.', 2); }
    bool          GetAnonymous() const   { return mHashKey.CharAt(2) == 'A'; }
    void          SetPrivate(bool priv)  { mHashKey.SetCharAt(priv ? 'P' : '.', 3); }
    bool          GetPrivate() const     { return mHashKey.CharAt(3) == 'P'; }
    void          SetInsecureScheme(bool insecureScheme)
                                       { mHashKey.SetCharAt(insecureScheme ? 'I' : '.', 4); }
    bool          GetInsecureScheme() const   { return mHashKey.CharAt(4) == 'I'; }

    void          SetNoSpdy(bool aNoSpdy)
                                       { mHashKey.SetCharAt(aNoSpdy ? 'X' : '.', 5); }
    bool          GetNoSpdy() const    { return mHashKey.CharAt(5) == 'X'; }

    const nsCString &GetNetworkInterfaceId() const { return mNetworkInterfaceId; }

    const nsCString &GetNPNToken() { return mNPNToken; }
    const nsCString &GetUsername() { return mUsername; }

    
    bool UsingProxy();

    
    bool UsingHttpProxy() const { return mUsingHttpProxy || mUsingHttpsProxy; }

    
    bool UsingHttpsProxy() const { return mUsingHttpsProxy; }

    
    bool EndToEndSSL() const { return mEndToEndSSL; }

    
    bool FirstHopSSL() const { return mEndToEndSSL || mUsingHttpsProxy; }

    
    bool UsingConnect() const { return mUsingConnect; }

    
    bool HostIsLocalIPLiteral() const;

private:
    void Init(const nsACString &host,
              int32_t port,
              const nsACString &npnToken,
              const nsACString &username,
              nsProxyInfo* proxyInfo,
              bool EndToEndSSL);
    void SetOriginServer(const nsACString &host, int32_t port);

    nsCString              mOrigin;
    int32_t                mOriginPort;
    nsCString              mRoutedHost;
    int32_t                mRoutedPort;

    nsCString              mHashKey;
    nsCString              mNetworkInterfaceId;
    nsCString              mUsername;
    nsCOMPtr<nsProxyInfo>  mProxyInfo;
    bool                   mUsingHttpProxy;
    bool                   mUsingHttpsProxy;
    bool                   mEndToEndSSL;
    bool                   mUsingConnect;  
    nsCString              mNPNToken;


    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(nsHttpConnectionInfo)
};

} 
} 

#endif
