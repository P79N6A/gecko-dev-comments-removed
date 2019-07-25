




































 
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

typedef nsDataHashtable<nsCStringHashKey, PRUint32> nsFailedProxyTable;

class nsProxyInfo;
struct nsProtocolInfo;

class nsProtocolProxyService : public nsIProtocolProxyService2
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
                                              PRUint32 aResolveFlags,
                                              nsProxyInfo **result);

    





    NS_HIDDEN_(nsresult) ConfigureFromPAC(const nsCString &pacURI, PRBool forceReload);

    











    NS_HIDDEN_(void) ProcessPACString(const nsCString &pacString,
                                      PRUint32 aResolveFlags,
                                      nsIProxyInfo **result);

    








    NS_HIDDEN_(void) GetProxyKey(nsProxyInfo *pi, nsCString &result);

    


    NS_HIDDEN_(PRUint32) SecondsSinceSessionStart();

    





    NS_HIDDEN_(void) EnableProxy(nsProxyInfo *pi);

    





    NS_HIDDEN_(void) DisableProxy(nsProxyInfo *pi);

    







    NS_HIDDEN_(PRBool) IsProxyDisabled(nsProxyInfo *pi);

    










    NS_HIDDEN_(nsresult) GetProtocolInfo(nsIURI *uri, nsProtocolInfo *result);

    




















    NS_HIDDEN_(nsresult) NewProxyInfo_Internal(const char *type,
                                               const nsACString &host,
                                               PRInt32 port,
                                               PRUint32 flags,
                                               PRUint32 timeout,
                                               nsIProxyInfo *next,
                                               PRUint32 aResolveFlags,
                                               nsIProxyInfo **result);

    

















    NS_HIDDEN_(nsresult) Resolve_Internal(nsIURI *uri,
                                          const nsProtocolInfo &info,
                                          PRUint32 flags,
                                          PRBool *usePAC, 
                                          nsIProxyInfo **result);

    










    NS_HIDDEN_(void) ApplyFilters(nsIURI *uri, const nsProtocolInfo &info,
                                  nsIProxyInfo **proxyInfo);

    



    inline void ApplyFilters(nsIURI *uri, const nsProtocolInfo &info,
                             nsCOMPtr<nsIProxyInfo> &proxyInfo)
    {
      nsIProxyInfo *pi = nsnull;
      proxyInfo.swap(pi);
      ApplyFilters(uri, info, &pi);
      proxyInfo.swap(pi);
    }

    








    NS_HIDDEN_(void) PruneProxyInfo(const nsProtocolInfo &info,
                                    nsIProxyInfo **proxyInfo);

    





    NS_HIDDEN_(void) LoadHostFilters(const char *hostFilters);

    









    NS_HIDDEN_(PRBool) CanUseProxy(nsIURI *uri, PRInt32 defaultPort);

public:
    
    
    

    struct HostInfoIP {
        PRUint16   family;
        PRUint16   mask_len;
        PRIPv6Addr addr; 
    };

    struct HostInfoName {
        char    *host;
        PRUint32 host_len;
    };

protected:

    
    struct HostInfo {
        PRBool  is_ipaddr;
        PRInt32 port;
        union {
            HostInfoIP   ip;
            HostInfoName name;
        };

        HostInfo()
            : is_ipaddr(PR_FALSE)
            {  }
       ~HostInfo() {
            if (!is_ipaddr && name.host)
                nsMemory::Free(name.host);
        }
    };

    
    struct FilterLink {
      struct FilterLink                *next;
      PRUint32                          position;
      nsCOMPtr<nsIProtocolProxyFilter>  filter;

      FilterLink(PRUint32 p, nsIProtocolProxyFilter *f)
        : next(nsnull), position(p), filter(f) {}

      
      ~FilterLink() { if (next) delete next; }
    };

    
    nsTArray<nsAutoPtr<HostInfo> > mHostFiltersArray;

    
    
    FilterLink                  *mFilters;

    PRUint32                     mProxyConfig;

    nsCString                    mHTTPProxyHost;
    PRInt32                      mHTTPProxyPort;

    nsCString                    mFTPProxyHost;
    PRInt32                      mFTPProxyPort;

    nsCString                    mHTTPSProxyHost;
    PRInt32                      mHTTPSProxyPort;
    
    nsCString                    mSOCKSProxyHost;
    PRInt32                      mSOCKSProxyPort;
    PRInt32                      mSOCKSProxyVersion;
    PRBool                       mSOCKSProxyRemoteDNS;

    nsRefPtr<nsPACMan>           mPACMan;  
    nsCOMPtr<nsISystemProxySettings> mSystemProxySettings;

    PRTime                       mSessionStart;
    nsFailedProxyTable           mFailedProxies;
    PRInt32                      mFailedProxyTimeout;
};

#endif 
