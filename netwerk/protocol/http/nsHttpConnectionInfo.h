





































#ifndef nsHttpConnectionInfo_h__
#define nsHttpConnectionInfo_h__

#include "nsHttp.h"
#include "nsProxyInfo.h"
#include "nsCOMPtr.h"
#include "nsDependentString.h"
#include "nsString.h"
#include "plstr.h"
#include "nsCRT.h"





class nsHttpConnectionInfo
{
public:
    nsHttpConnectionInfo(const nsACString &host, PRInt32 port,
                         nsProxyInfo* proxyInfo,
                         PRBool usingSSL=PR_FALSE)
        : mRef(0)
        , mProxyInfo(proxyInfo)
        , mUsingSSL(usingSSL)
    {
        LOG(("Creating nsHttpConnectionInfo @%x\n", this));

        mUsingHttpProxy = (proxyInfo && !nsCRT::strcmp(proxyInfo->Type(), "http"));

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

    void SetOriginServer(const nsACString &host, PRInt32 port);

    void SetOriginServer(const char *host, PRInt32 port)
    {
        SetOriginServer(nsDependentCString(host), port);
    }
    
    nsHttpConnectionInfo* Clone() const;

    const char *ProxyHost() const { return mProxyInfo ? mProxyInfo->Host().get() : nsnull; }
    PRInt32     ProxyPort() const { return mProxyInfo ? mProxyInfo->Port() : -1; }
    const char *ProxyType() const { return mProxyInfo ? mProxyInfo->Type() : nsnull; }

    
    
    
    
    
    
    
    PRBool Equals(const nsHttpConnectionInfo *info)
    {
        return mHashKey.Equals(info->HashKey());
    }

    const char   *Host() const           { return mHost.get(); }
    PRInt32       Port() const           { return mPort; }
    nsProxyInfo  *ProxyInfo()            { return mProxyInfo; }
    PRBool        UsingHttpProxy() const { return mUsingHttpProxy; }
    PRBool        UsingSSL() const       { return mUsingSSL; }
    PRInt32       DefaultPort() const    { return mUsingSSL ? NS_HTTPS_DEFAULT_PORT : NS_HTTP_DEFAULT_PORT; }
    void          SetAnonymous(PRBool anon)         
                                         { mHashKey.SetCharAt(anon ? 'A' : '.', 2); }
    PRBool        ShouldForceConnectMethod();

private:
    nsrefcnt               mRef;
    nsCString              mHashKey;
    nsCString              mHost;
    PRInt32                mPort;
    nsCOMPtr<nsProxyInfo>  mProxyInfo;
    PRPackedBool           mUsingHttpProxy;
    PRPackedBool           mUsingSSL;
};

#endif 
