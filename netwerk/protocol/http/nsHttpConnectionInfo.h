




#ifndef nsHttpConnectionInfo_h__
#define nsHttpConnectionInfo_h__

#include "nsHttp.h"
#include "nsProxyInfo.h"
#include "nsCOMPtr.h"
#include "nsDependentString.h"
#include "nsString.h"
#include "plstr.h"
#include "nsCRT.h"
#include "nsIProtocolProxyService.h"





class nsHttpConnectionInfo
{
public:
    nsHttpConnectionInfo(const nsACString &host, int32_t port,
                         nsProxyInfo* proxyInfo,
                         bool usingSSL=false)
        : mRef(0)
        , mProxyInfo(proxyInfo)
        , mUsingSSL(usingSSL)
        , mUsingConnect(false)
    {
        LOG(("Creating nsHttpConnectionInfo @%x\n", this));

        mUsingHttpProxy = (proxyInfo && !nsCRT::strcmp(proxyInfo->Type(), "http"));

        if (mUsingHttpProxy) {
            mUsingConnect = mUsingSSL;  
            uint32_t resolveFlags = 0;
            if (NS_SUCCEEDED(mProxyInfo->GetResolveFlags(&resolveFlags)) &&
                resolveFlags & nsIProtocolProxyService::RESOLVE_ALWAYS_TUNNEL) {
                mUsingConnect = true;
            }
        }

        SetOriginServer(host, port);
    }
    
   ~nsHttpConnectionInfo()
    {
        LOG(("Destroying nsHttpConnectionInfo @%x\n", this));
    }

    nsrefcnt AddRef()
    {
        nsrefcnt n = NS_AtomicIncrementRefcnt(mRef);
        NS_LOG_ADDREF(this, n, "nsHttpConnectionInfo", sizeof(*this));
        return n;
    }

    nsrefcnt Release()
    {
        nsrefcnt n = NS_AtomicDecrementRefcnt(mRef);
        NS_LOG_RELEASE(this, n, "nsHttpConnectionInfo");
        if (n == 0)
            delete this;
        return n;
    }

    const nsAFlatCString &HashKey() const { return mHashKey; }

    void SetOriginServer(const nsACString &host, int32_t port);

    void SetOriginServer(const char *host, int32_t port)
    {
        SetOriginServer(nsDependentCString(host), port);
    }
    
    
    nsHttpConnectionInfo* Clone() const;

    const char *ProxyHost() const { return mProxyInfo ? mProxyInfo->Host().get() : nullptr; }
    int32_t     ProxyPort() const { return mProxyInfo ? mProxyInfo->Port() : -1; }
    const char *ProxyType() const { return mProxyInfo ? mProxyInfo->Type() : nullptr; }

    
    
    
    
    
    
    
    bool Equals(const nsHttpConnectionInfo *info)
    {
        return mHashKey.Equals(info->HashKey());
    }

    const char   *Host() const           { return mHost.get(); }
    int32_t       Port() const           { return mPort; }
    nsProxyInfo  *ProxyInfo()            { return mProxyInfo; }
    bool          UsingHttpProxy() const { return mUsingHttpProxy; }
    bool          UsingSSL() const       { return mUsingSSL; }
    bool          UsingConnect() const   { return mUsingConnect; }
    int32_t       DefaultPort() const    { return mUsingSSL ? NS_HTTPS_DEFAULT_PORT : NS_HTTP_DEFAULT_PORT; }
    void          SetAnonymous(bool anon)         
                                         { mHashKey.SetCharAt(anon ? 'A' : '.', 2); }
    bool          GetAnonymous() const   { return mHashKey.CharAt(2) == 'A'; }
    void          SetPrivate(bool priv)  { mHashKey.SetCharAt(priv ? 'P' : '.', 3); }
    bool          GetPrivate() const     { return mHashKey.CharAt(3) == 'P'; }

    const nsCString &GetHost() { return mHost; }

private:
    nsrefcnt               mRef;
    nsCString              mHashKey;
    nsCString              mHost;
    int32_t                mPort;
    nsCOMPtr<nsProxyInfo>  mProxyInfo;
    bool                   mUsingHttpProxy;
    bool                   mUsingSSL;
    bool                   mUsingConnect;  
};

#endif 
