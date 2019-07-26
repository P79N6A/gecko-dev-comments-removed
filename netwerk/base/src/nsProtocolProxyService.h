



 
#ifndef nsProtocolProxyService_h__
#define nsProtocolProxyService_h__

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsIPrefBranch.h"
#include "nsIProtocolProxyService2.h"
#include "nsIProtocolProxyFilter.h"
#include "nsIProxyAutoConfig.h"
#include "nsISystemProxySettings.h"
#include "nsIProxyInfo.h"
#include "nsIObserver.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsPACMan.h"
#include "prtime.h"
#include "prmem.h"
#include "prio.h"
#include "mozilla/Attributes.h"

typedef nsDataHashtable<nsCStringHashKey, uint32_t> nsFailedProxyTable;

class nsProxyInfo;
struct nsProtocolInfo;

class nsProtocolProxyService MOZ_FINAL : public nsIProtocolProxyService2
                                       , public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLPROXYSERVICE2
    NS_DECL_NSIPROTOCOLPROXYSERVICE
    NS_DECL_NSIOBSERVER

    nsProtocolProxyService() NS_HIDDEN;

    NS_HIDDEN_(nsresult) Init();

protected:
    friend class nsAsyncResolveRequest;

    ~nsProtocolProxyService() NS_HIDDEN;

    









    NS_HIDDEN_(void) PrefsChanged(nsIPrefBranch *prefs, const char *name);

    















    NS_HIDDEN_(const char *) ExtractProxyInfo(const char *proxy,
                                              uint32_t aResolveFlags,
                                              nsProxyInfo **result);

    





    NS_HIDDEN_(nsresult) ConfigureFromPAC(const nsCString &pacURI, bool forceReload);

    











    NS_HIDDEN_(void) ProcessPACString(const nsCString &pacString,
                                      uint32_t aResolveFlags,
                                      nsIProxyInfo **result);

    








    NS_HIDDEN_(void) GetProxyKey(nsProxyInfo *pi, nsCString &result);

    


    NS_HIDDEN_(uint32_t) SecondsSinceSessionStart();

    





    NS_HIDDEN_(void) EnableProxy(nsProxyInfo *pi);

    





    NS_HIDDEN_(void) DisableProxy(nsProxyInfo *pi);

    







    NS_HIDDEN_(bool) IsProxyDisabled(nsProxyInfo *pi);

    










    NS_HIDDEN_(nsresult) GetProtocolInfo(nsIURI *uri, nsProtocolInfo *result);

    




















    NS_HIDDEN_(nsresult) NewProxyInfo_Internal(const char *type,
                                               const nsACString &host,
                                               int32_t port,
                                               uint32_t flags,
                                               uint32_t timeout,
                                               nsIProxyInfo *next,
                                               uint32_t aResolveFlags,
                                               nsIProxyInfo **result);

    

















    NS_HIDDEN_(nsresult) Resolve_Internal(nsIURI *uri,
                                          const nsProtocolInfo &info,
                                          uint32_t flags,
                                          bool *usePAC, 
                                          nsIProxyInfo **result);

    










    NS_HIDDEN_(void) ApplyFilters(nsIURI *uri, const nsProtocolInfo &info,
                                  nsIProxyInfo **proxyInfo);

    



    inline void ApplyFilters(nsIURI *uri, const nsProtocolInfo &info,
                             nsCOMPtr<nsIProxyInfo> &proxyInfo)
    {
      nsIProxyInfo *pi = nullptr;
      proxyInfo.swap(pi);
      ApplyFilters(uri, info, &pi);
      proxyInfo.swap(pi);
    }

    








    NS_HIDDEN_(void) PruneProxyInfo(const nsProtocolInfo &info,
                                    nsIProxyInfo **proxyInfo);

    





    NS_HIDDEN_(void) LoadHostFilters(const char *hostFilters);

    









    NS_HIDDEN_(bool) CanUseProxy(nsIURI *uri, int32_t defaultPort);

public:
    
    
    

    struct HostInfoIP {
        uint16_t   family;
        uint16_t   mask_len;
        PRIPv6Addr addr; 
    };

    struct HostInfoName {
        char    *host;
        uint32_t host_len;
    };

protected:

    
    struct HostInfo {
        bool    is_ipaddr;
        int32_t port;
        union {
            HostInfoIP   ip;
            HostInfoName name;
        };

        HostInfo()
            : is_ipaddr(false)
            {  }
       ~HostInfo() {
            if (!is_ipaddr && name.host)
                nsMemory::Free(name.host);
        }
    };

    
    struct FilterLink {
      struct FilterLink                *next;
      uint32_t                          position;
      nsCOMPtr<nsIProtocolProxyFilter>  filter;

      FilterLink(uint32_t p, nsIProtocolProxyFilter *f)
        : next(nullptr), position(p), filter(f) {}

      
      ~FilterLink() { if (next) delete next; }
    };

    
    bool mFilterLocalHosts;

    
    nsTArray<nsAutoPtr<HostInfo> > mHostFiltersArray;

    
    
    FilterLink                  *mFilters;

    uint32_t                     mProxyConfig;

    nsCString                    mHTTPProxyHost;
    int32_t                      mHTTPProxyPort;

    nsCString                    mFTPProxyHost;
    int32_t                      mFTPProxyPort;

    nsCString                    mHTTPSProxyHost;
    int32_t                      mHTTPSProxyPort;
    
    nsCString                    mSOCKSProxyHost;
    int32_t                      mSOCKSProxyPort;
    int32_t                      mSOCKSProxyVersion;
    bool                         mSOCKSProxyRemoteDNS;

    nsRefPtr<nsPACMan>           mPACMan;  
    nsCOMPtr<nsISystemProxySettings> mSystemProxySettings;

    PRTime                       mSessionStart;
    nsFailedProxyTable           mFailedProxies;
    int32_t                      mFailedProxyTimeout;
};

#endif 
