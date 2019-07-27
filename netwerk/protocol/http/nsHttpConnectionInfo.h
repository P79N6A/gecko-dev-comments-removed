





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
    nsHttpConnectionInfo(const nsACString &physicalHost,
                         int32_t physicalPort,
                         const nsACString &npnToken,
                         const nsACString &username,
                         nsProxyInfo *proxyInfo,
                         bool endToEndSSL = false);

    
    
    nsHttpConnectionInfo(const nsACString &physicalHost,
                         int32_t physicalPort,
                         const nsACString &npnToken,
                         const nsACString &username,
                         nsProxyInfo *proxyInfo,
                         const nsACString &logicalHost,
                         int32_t logicalPort);

private:
    virtual ~nsHttpConnectionInfo()
    {
        PR_LOG(gHttpLog, 4, ("Destroying nsHttpConnectionInfo @%x\n", this));
    }

    void BuildHashKey();

public:
    const nsAFlatCString &HashKey() const { return mHashKey; }

    const nsCString &GetAuthenticationHost() const { return mAuthenticationHost; }
    int32_t GetAuthenticationPort() const { return mAuthenticationPort; }

    
    
    
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
    void          SetInsecureScheme(bool insecureScheme)
                                       { mHashKey.SetCharAt(insecureScheme ? 'I' : '.', 4); }
    bool          GetInsecureScheme() const   { return mHashKey.CharAt(4) == 'I'; }

    void          SetNoSpdy(bool aNoSpdy)
                                       { mHashKey.SetCharAt(aNoSpdy ? 'X' : '.', 5); }
    bool          GetNoSpdy() const    { return mHashKey.CharAt(5) == 'X'; }

    const nsCString &GetNetworkInterfaceId() const { return mNetworkInterfaceId; }

    const nsCString &GetHost() { return mHost; }
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

    nsCString              mHashKey;
    nsCString              mHost;
    nsCString              mNetworkInterfaceId;
    int32_t                mPort;
    nsCString              mUsername;
    nsCString              mAuthenticationHost;
    int32_t                mAuthenticationPort;
    nsCOMPtr<nsProxyInfo>  mProxyInfo;
    bool                   mUsingHttpProxy;
    bool                   mUsingHttpsProxy;
    bool                   mEndToEndSSL;
    bool                   mUsingConnect;  
    nsCString              mNPNToken;


    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(nsHttpConnectionInfo)
};

}} 

#endif
