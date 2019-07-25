





































#ifndef nsProxiedService_h__
#define nsProxiedService_h__

#include "nsServiceManagerUtils.h"
#include "nsIProxyObjectManager.h"
#include "nsXPCOMCIDInternal.h"







































#define NS_WITH_PROXIED_SERVICE(T, var, cid, Q, rvAddr)     \
    nsProxiedService _serv##var(cid, NS_GET_IID(T), Q, PR_FALSE, rvAddr);     \
    T* var = (T*)(nsISupports*)_serv##var;

#define NS_WITH_ALWAYS_PROXIED_SERVICE(T, var, cid, Q, rvAddr)     \
    nsProxiedService _serv##var(cid, NS_GET_IID(T), Q, PR_TRUE, rvAddr);       \
    T* var = (T*)(nsISupports*)_serv##var;





class NS_STACK_CLASS nsProxiedService
{
public:
    nsProxiedService(const nsCID &aClass, const nsIID &aIID, 
                     nsIEventTarget* aTarget, bool always, nsresult* rv)
    {
        nsCOMPtr<nsISupports> svc = do_GetService(aClass, rv);
        if (NS_SUCCEEDED(*rv))
            InitProxy(svc, aIID, aTarget, always, rv);
    }

    nsProxiedService(const char* aContractID, const nsIID &aIID, 
                     nsIEventTarget* aTarget, bool always, nsresult* rv)
    {
        nsCOMPtr<nsISupports> svc = do_GetService(aContractID, rv);
        if (NS_SUCCEEDED(*rv))
            InitProxy(svc, aIID, aTarget, always, rv);
    }
    
    operator nsISupports*() const
    {
        return mProxiedService;
    }

private:

    void InitProxy(nsISupports *aObj, const nsIID &aIID,
                   nsIEventTarget* aTarget, bool always, nsresult*rv)
    {
        PRInt32 proxyType = NS_PROXY_SYNC;
        if (always)
            proxyType |= NS_PROXY_ALWAYS;

        nsCOMPtr<nsIProxyObjectManager> proxyObjMgr = do_GetService(NS_XPCOMPROXY_CONTRACTID, rv);
        if (NS_FAILED(*rv))
          return;

        *rv = proxyObjMgr->GetProxyForObject(aTarget,
                                             aIID,
                                             aObj,
                                             proxyType,
                                             getter_AddRefs(mProxiedService));
    }

    nsCOMPtr<nsISupports> mProxiedService;
};

#endif 
