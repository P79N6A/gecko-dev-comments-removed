



 
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



#define NS_PROTOCOL_PROXY_SERVICE_IMPL_CID        \
{ 0x091eedd8, 0x8bae, 0x4fe3, \
        { 0xad, 0x62, 0x0c, 0x87, 0x35, 0x1e, 0x64, 0x0d } }

class nsProtocolProxyService final : public nsIProtocolProxyService2
                                   , public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLPROXYSERVICE2
    NS_DECL_NSIPROTOCOLPROXYSERVICE
    NS_DECL_NSIOBSERVER

    NS_DECLARE_STATIC_IID_ACCESSOR(NS_PROTOCOL_PROXY_SERVICE_IMPL_CID)

    nsProtocolProxyService();

    nsresult Init();
    nsresult DeprecatedBlockingResolve(nsIChannel *aChannel,
                                       uint32_t aFlags,
                                       nsIProxyInfo **retval);

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

    





















    nsresult Resolve_Internal(nsIChannel *channel,
                              uint32_t appId,
                              bool isInBrowser,
                              const nsProtocolInfo &info,
                              uint32_t flags,
                              bool *usePAC,
                              nsIProxyInfo **result);

    










    void ApplyFilters(nsIChannel *channel, const nsProtocolInfo &info,
                                  nsIProxyInfo **proxyInfo);

    



    inline void ApplyFilters(nsIChannel *channel, const nsProtocolInfo &info,
                             nsCOMPtr<nsIProxyInfo> &proxyInfo)
    {
      nsIProxyInfo *pi = nullptr;
      proxyInfo.swap(pi);
      ApplyFilters(channel, info, &pi);
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
                free(name.host);
        }
    };

    
    
    struct FilterLink {
      struct FilterLink                *next;
      uint32_t                          position;
      nsCOMPtr<nsIProtocolProxyFilter> filter;
      nsCOMPtr<nsIProtocolProxyChannelFilter> channelFilter;
      FilterLink(uint32_t p, nsIProtocolProxyFilter *f)
        : next(nullptr), position(p), filter(f), channelFilter(nullptr) {}
      FilterLink(uint32_t p, nsIProtocolProxyChannelFilter *cf)
        : next(nullptr), position(p), filter(nullptr), channelFilter(cf) {}
      
      ~FilterLink() { if (next) delete next; }
    };

private:
    
    nsresult InsertFilterLink(FilterLink *link, uint32_t position);
    nsresult RemoveFilterLink(nsISupports *givenObject);

protected:
    
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
    nsresult AsyncResolveInternal(nsIChannel *channel, uint32_t flags,
                                  nsIProtocolProxyCallback *callback,
                                  nsICancelable **result,
                                  bool isSyncOK);

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsProtocolProxyService, NS_PROTOCOL_PROXY_SERVICE_IMPL_CID)

#endif 
