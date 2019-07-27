



 
#ifndef nsProtocolProxyService_h__
#define nsProtocolProxyService_h__

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsIProtocolProxyService2.h"
#include "nsIProtocolProxyFilter.h"
#include "nsIProxyInfo.h"
#include "nsIObserver.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "prio.h"
#include "mozilla/Attributes.h"

typedef nsDataHashtable<nsCStringHashKey, uint32_t> nsFailedProxyTable;

class nsProxyInfo;
struct nsProtocolInfo;
class nsIPrefBranch;
class nsISystemProxySettings;
class nsPACMan;

class nsProtocolProxyService MOZ_FINAL : public nsIProtocolProxyService2
                                       , public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLPROXYSERVICE2
    NS_DECL_NSIPROTOCOLPROXYSERVICE
    NS_DECL_NSIOBSERVER

    nsProtocolProxyService();

    nsresult Init();

protected:
    friend class nsAsyncResolveRequest;

    ~nsProtocolProxyService();

    









    void PrefsChanged(nsIPrefBranch *prefs, const char *name);

    















    const char * ExtractProxyInfo(const char *proxy,
                                              uint32_t aResolveFlags,
                                              nsProxyInfo **result);

    





    nsresult ConfigureFromPAC(const nsCString &pacURI, bool forceReload);

    











    void ProcessPACString(const nsCString &pacString,
                                      uint32_t aResolveFlags,
                                      nsIProxyInfo **result);

    








    void GetProxyKey(nsProxyInfo *pi, nsCString &result);

    


    uint32_t SecondsSinceSessionStart();

    





    void EnableProxy(nsProxyInfo *pi);

    





    void DisableProxy(nsProxyInfo *pi);

    







    bool IsProxyDisabled(nsProxyInfo *pi);

    










    nsresult GetProtocolInfo(nsIURI *uri, nsProtocolInfo *result);

    




















    nsresult NewProxyInfo_Internal(const char *type,
                                               const nsACString &host,
                                               int32_t port,
                                               uint32_t flags,
                                               uint32_t timeout,
                                               nsIProxyInfo *next,
                                               uint32_t aResolveFlags,
                                               nsIProxyInfo **result);

    

















    nsresult Resolve_Internal(nsIURI *uri,
                                          const nsProtocolInfo &info,
                                          uint32_t flags,
                                          bool *usePAC, 
                                          nsIProxyInfo **result);

    










    void ApplyFilters(nsIURI *uri, const nsProtocolInfo &info,
                                  nsIProxyInfo **proxyInfo);

    



    inline void ApplyFilters(nsIURI *uri, const nsProtocolInfo &info,
                             nsCOMPtr<nsIProxyInfo> &proxyInfo)
    {
      nsIProxyInfo *pi = nullptr;
      proxyInfo.swap(pi);
      ApplyFilters(uri, info, &pi);
      proxyInfo.swap(pi);
    }

    








    void PruneProxyInfo(const nsProtocolInfo &info,
                                    nsIProxyInfo **proxyInfo);

    





    void LoadHostFilters(const char *hostFilters);

    









    bool CanUseProxy(nsIURI *uri, int32_t defaultPort);

    





    void MaybeDisableDNSPrefetch(nsIProxyInfo *aProxy);

private:
    nsresult SetupPACThread();
    nsresult ResetPACThread();
    nsresult ReloadNetworkPAC();

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
    bool                         mProxyOverTLS;

    nsRefPtr<nsPACMan>           mPACMan;  
    nsCOMPtr<nsISystemProxySettings> mSystemProxySettings;

    PRTime                       mSessionStart;
    nsFailedProxyTable           mFailedProxies;
    int32_t                      mFailedProxyTimeout;

private:
    nsresult AsyncResolveInternal(nsIURI *uri, uint32_t flags,
                                  nsIProtocolProxyCallback *callback,
                                  nsICancelable **result,
                                  bool isSyncOK);

};

#endif 
