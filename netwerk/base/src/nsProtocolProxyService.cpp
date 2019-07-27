





#include "mozilla/ArrayUtils.h"
#include "mozilla/Attributes.h"

#include "nsProtocolProxyService.h"
#include "nsProxyInfo.h"
#include "nsIClassInfoImpl.h"
#include "nsIIOService.h"
#include "nsIObserverService.h"
#include "nsIProtocolHandler.h"
#include "nsIProtocolProxyCallback.h"
#include "nsICancelable.h"
#include "nsIDNSService.h"
#include "nsPIDNSService.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsThreadUtils.h"
#include "nsString.h"
#include "nsNetUtil.h"
#include "nsNetCID.h"
#include "prnetdb.h"
#include "nsPACMan.h"
#include "nsProxyRelease.h"
#include "mozilla/Mutex.h"
#include "mozilla/CondVar.h"
#include "nsISystemProxySettings.h"
#include "nsINetworkLinkService.h"



namespace mozilla {
  extern const char kProxyType_HTTP[];
  extern const char kProxyType_HTTPS[];
  extern const char kProxyType_SOCKS[];
  extern const char kProxyType_SOCKS4[];
  extern const char kProxyType_SOCKS5[];
  extern const char kProxyType_DIRECT[];
}

using namespace mozilla;

#include "prlog.h"
#if defined(PR_LOGGING)
#endif
#undef LOG
#define LOG(args) PR_LOG(net::GetProxyLog(), PR_LOG_DEBUG, args)



#define PROXY_PREF_BRANCH  "network.proxy"
#define PROXY_PREF(x)      PROXY_PREF_BRANCH "." x

#define WPAD_URL "http://wpad/wpad.dat"




struct nsProtocolInfo {
    nsAutoCString scheme;
    uint32_t flags;
    int32_t defaultPort;
};






class nsAsyncResolveRequest MOZ_FINAL : public nsIRunnable
                                      , public nsPACManCallback
                                      , public nsICancelable
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS

    nsAsyncResolveRequest(nsProtocolProxyService *pps, nsIURI *uri,
                          uint32_t aResolveFlags,
                          nsIProtocolProxyCallback *callback)
        : mStatus(NS_OK)
        , mDispatched(false)
        , mResolveFlags(aResolveFlags)
        , mPPS(pps)
        , mXPComPPS(pps)
        , mURI(uri)
        , mCallback(callback)
    {
        NS_ASSERTION(mCallback, "null callback");
    }

private:
    ~nsAsyncResolveRequest()
    {
        if (!NS_IsMainThread()) {
            
            
            

            nsCOMPtr<nsIThread> mainThread;
            NS_GetMainThread(getter_AddRefs(mainThread));

            if (mURI) {
                nsIURI *forgettable;
                mURI.forget(&forgettable);
                NS_ProxyRelease(mainThread, forgettable, false);
            }

            if (mCallback) {
                nsIProtocolProxyCallback *forgettable;
                mCallback.forget(&forgettable);
                NS_ProxyRelease(mainThread, forgettable, false);
            }

            if (mProxyInfo) {
                nsIProxyInfo *forgettable;
                mProxyInfo.forget(&forgettable);
                NS_ProxyRelease(mainThread, forgettable, false);
            }

            if (mXPComPPS) {
                nsIProtocolProxyService *forgettable;
                mXPComPPS.forget(&forgettable);
                NS_ProxyRelease(mainThread, forgettable, false);
            }
        }
    }

public:
    void SetResult(nsresult status, nsIProxyInfo *pi)
    {
        mStatus = status;
        mProxyInfo = pi;
    }

    NS_IMETHOD Run()
    {
        if (mCallback)
            DoCallback();
        return NS_OK;
    }

    NS_IMETHOD Cancel(nsresult reason)
    {
        NS_ENSURE_ARG(NS_FAILED(reason));

        
        if (!mCallback)
            return NS_OK;

        SetResult(reason, nullptr);
        return DispatchCallback();
    }

    nsresult DispatchCallback()
    {
        if (mDispatched)  
            return NS_OK;

        nsresult rv = NS_DispatchToCurrentThread(this);
        if (NS_FAILED(rv))
            NS_WARNING("unable to dispatch callback event");
        else {
            mDispatched = true;
            return NS_OK;
        }

        mCallback = nullptr;  
        return rv;
    }

private:

    
    
    void OnQueryComplete(nsresult status,
                         const nsCString &pacString,
                         const nsCString &newPACURL)
    {
        
        if (!mCallback)
            return;

        
        if (mStatus == NS_OK) {
            mStatus = status;
            mPACString = pacString;
            mPACURL = newPACURL;
        }

        
        
        
        DoCallback();
    }

    void DoCallback()
    {
        if (mStatus == NS_ERROR_NOT_AVAILABLE && !mProxyInfo) {
            
            
            
            mPACString = NS_LITERAL_CSTRING("DIRECT;");
            mStatus = NS_OK;
        }

        
        if (NS_SUCCEEDED(mStatus) && !mProxyInfo && !mPACString.IsEmpty()) {
            mPPS->ProcessPACString(mPACString, mResolveFlags,
                                   getter_AddRefs(mProxyInfo));

            
            nsProtocolInfo info;
            mStatus = mPPS->GetProtocolInfo(mURI, &info);
            if (NS_SUCCEEDED(mStatus))
                mPPS->ApplyFilters(mURI, info, mProxyInfo);
            else
                mProxyInfo = nullptr;

            LOG(("pac thread callback %s\n", mPACString.get()));
            if (NS_SUCCEEDED(mStatus))
                mPPS->MaybeDisableDNSPrefetch(mProxyInfo);
            mCallback->OnProxyAvailable(this, mURI, mProxyInfo, mStatus);
        }
        else if (NS_SUCCEEDED(mStatus) && !mPACURL.IsEmpty()) {
            LOG(("pac thread callback indicates new pac file load\n"));

            
            nsresult rv = mPPS->ConfigureFromPAC(mPACURL, false);
            if (NS_SUCCEEDED(rv)) {
                
                nsRefPtr<nsAsyncResolveRequest> newRequest =
                    new nsAsyncResolveRequest(mPPS, mURI, mResolveFlags, mCallback);
                rv = mPPS->mPACMan->AsyncGetProxyForURI(mURI, newRequest, true);
            }

            if (NS_FAILED(rv))
                mCallback->OnProxyAvailable(this, mURI, nullptr, rv);

            
            
        }
        else {
            LOG(("pac thread callback did not provide information %X\n", mStatus));
            if (NS_SUCCEEDED(mStatus))
                mPPS->MaybeDisableDNSPrefetch(mProxyInfo);
            mCallback->OnProxyAvailable(this, mURI, mProxyInfo, mStatus);
        }

        
        
        
        mCallback = nullptr;  
        mPPS = nullptr;
        mXPComPPS = nullptr;
        mURI = nullptr;
        mProxyInfo = nullptr;
    }

private:

    nsresult  mStatus;
    nsCString mPACString;
    nsCString mPACURL;
    bool      mDispatched;
    uint32_t  mResolveFlags;

    nsProtocolProxyService            *mPPS;
    nsCOMPtr<nsIProtocolProxyService>  mXPComPPS;
    nsCOMPtr<nsIURI>                   mURI;
    nsCOMPtr<nsIProtocolProxyCallback> mCallback;
    nsCOMPtr<nsIProxyInfo>             mProxyInfo;
};

NS_IMPL_ISUPPORTS(nsAsyncResolveRequest, nsICancelable, nsIRunnable)



#define IS_ASCII_SPACE(_c) ((_c) == ' ' || (_c) == '\t')






static void
proxy_MaskIPv6Addr(PRIPv6Addr &addr, uint16_t mask_len)
{
    if (mask_len == 128)
        return;

    if (mask_len > 96) {
        addr.pr_s6_addr32[3] = PR_htonl(
                PR_ntohl(addr.pr_s6_addr32[3]) & (~0L << (128 - mask_len)));
    }
    else if (mask_len > 64) {
        addr.pr_s6_addr32[3] = 0;
        addr.pr_s6_addr32[2] = PR_htonl(
                PR_ntohl(addr.pr_s6_addr32[2]) & (~0L << (96 - mask_len)));
    }
    else if (mask_len > 32) {
        addr.pr_s6_addr32[3] = 0;
        addr.pr_s6_addr32[2] = 0;
        addr.pr_s6_addr32[1] = PR_htonl(
                PR_ntohl(addr.pr_s6_addr32[1]) & (~0L << (64 - mask_len)));
    }
    else {
        addr.pr_s6_addr32[3] = 0;
        addr.pr_s6_addr32[2] = 0;
        addr.pr_s6_addr32[1] = 0;
        addr.pr_s6_addr32[0] = PR_htonl(
                PR_ntohl(addr.pr_s6_addr32[0]) & (~0L << (32 - mask_len)));
    }
}

static void
proxy_GetStringPref(nsIPrefBranch *aPrefBranch,
                    const char    *aPref,
                    nsCString     &aResult)
{
    nsXPIDLCString temp;
    nsresult rv = aPrefBranch->GetCharPref(aPref, getter_Copies(temp));
    if (NS_FAILED(rv))
        aResult.Truncate();
    else {
        aResult.Assign(temp);
        
        
        aResult.StripWhitespace();
    }
}

static void
proxy_GetIntPref(nsIPrefBranch *aPrefBranch,
                 const char    *aPref,
                 int32_t       &aResult)
{
    int32_t temp;
    nsresult rv = aPrefBranch->GetIntPref(aPref, &temp);
    if (NS_FAILED(rv)) 
        aResult = -1;
    else
        aResult = temp;
}

static void
proxy_GetBoolPref(nsIPrefBranch *aPrefBranch,
                 const char    *aPref,
                 bool          &aResult)
{
    bool temp;
    nsresult rv = aPrefBranch->GetBoolPref(aPref, &temp);
    if (NS_FAILED(rv)) 
        aResult = false;
    else
        aResult = temp;
}



static const int32_t PROXYCONFIG_DIRECT4X = 3;
static const int32_t PROXYCONFIG_COUNT = 6;

NS_IMPL_ADDREF(nsProtocolProxyService)
NS_IMPL_RELEASE(nsProtocolProxyService)
NS_IMPL_CLASSINFO(nsProtocolProxyService, nullptr, nsIClassInfo::SINGLETON,
                  NS_PROTOCOLPROXYSERVICE_CID)
NS_IMPL_QUERY_INTERFACE_CI(nsProtocolProxyService,
                           nsIProtocolProxyService,
                           nsIProtocolProxyService2,
                           nsIObserver)
NS_IMPL_CI_INTERFACE_GETTER(nsProtocolProxyService,
                            nsIProtocolProxyService,
                            nsIProtocolProxyService2)

nsProtocolProxyService::nsProtocolProxyService()
    : mFilterLocalHosts(false)
    , mFilters(nullptr)
    , mProxyConfig(PROXYCONFIG_DIRECT)
    , mHTTPProxyPort(-1)
    , mFTPProxyPort(-1)
    , mHTTPSProxyPort(-1)
    , mSOCKSProxyPort(-1)
    , mSOCKSProxyVersion(4)
    , mSOCKSProxyRemoteDNS(false)
    , mProxyOverTLS(true)
    , mPACMan(nullptr)
    , mSessionStart(PR_Now())
    , mFailedProxyTimeout(30 * 60) 
{
}

nsProtocolProxyService::~nsProtocolProxyService()
{
    
    NS_ASSERTION(mHostFiltersArray.Length() == 0 && mFilters == nullptr &&
                 mPACMan == nullptr, "what happened to xpcom-shutdown?");
}


nsresult
nsProtocolProxyService::Init()
{
    
    nsCOMPtr<nsIPrefBranch> prefBranch =
            do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefBranch) {
        
        prefBranch->AddObserver(PROXY_PREF_BRANCH, this, false);

        
        PrefsChanged(prefBranch, nullptr);
    }

    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
        
        
        obs->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);

        obs->AddObserver(this, NS_NETWORK_LINK_TOPIC, false);
    }

    return NS_OK;
}



nsresult
nsProtocolProxyService::ReloadNetworkPAC()
{
    nsCOMPtr<nsIPrefBranch> prefs =
        do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!prefs) {
        return NS_OK;
    }

    int32_t type;
    nsresult rv = prefs->GetIntPref(PROXY_PREF("type"), &type);
    if (NS_FAILED(rv)) {
        return NS_OK;
    }

    if (type == PROXYCONFIG_PAC) {
        nsXPIDLCString pacSpec;
        prefs->GetCharPref(PROXY_PREF("autoconfig_url"),
                           getter_Copies(pacSpec));
        if (!pacSpec.IsEmpty()) {
            nsCOMPtr<nsIURI> pacURI;
            NS_NewURI(getter_AddRefs(pacURI), pacSpec);
            nsProtocolInfo pac;
            GetProtocolInfo(pacURI, &pac);

            if (!pac.scheme.EqualsLiteral("file") &&
                !pac.scheme.EqualsLiteral("data")) {
                LOG((": received network changed event, reload PAC"));
                ReloadPAC();
            }
        }
    } else if ((type == PROXYCONFIG_WPAD) || (type == PROXYCONFIG_SYSTEM)) {
        ReloadPAC();
    }

    return NS_OK;
}


NS_IMETHODIMP
nsProtocolProxyService::Observe(nsISupports     *aSubject,
                                const char      *aTopic,
                                const char16_t *aData)
{
    if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
        
        if (mHostFiltersArray.Length() > 0) {
            mHostFiltersArray.Clear();
        }
        if (mFilters) {
            delete mFilters;
            mFilters = nullptr;
        }
        if (mPACMan) {
            mPACMan->Shutdown();
            mPACMan = nullptr;
        }
    } else if (strcmp(aTopic, NS_NETWORK_LINK_TOPIC) == 0) {
        nsCString converted = NS_ConvertUTF16toUTF8(aData);
        const char *state = converted.get();
        if (!strcmp(state, NS_NETWORK_LINK_DATA_CHANGED)) {
            ReloadNetworkPAC();
        }
    }
    else {
        NS_ASSERTION(strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) == 0,
                     "what is this random observer event?");
        nsCOMPtr<nsIPrefBranch> prefs = do_QueryInterface(aSubject);
        if (prefs)
            PrefsChanged(prefs, NS_LossyConvertUTF16toASCII(aData).get());
    }
    return NS_OK;
}

void
nsProtocolProxyService::PrefsChanged(nsIPrefBranch *prefBranch,
                                     const char    *pref)
{
    nsresult rv = NS_OK;
    bool reloadPAC = false;
    nsXPIDLCString tempString;

    if (!pref || !strcmp(pref, PROXY_PREF("type"))) {
        int32_t type = -1;
        rv = prefBranch->GetIntPref(PROXY_PREF("type"), &type);
        if (NS_SUCCEEDED(rv)) {
            
            if (type == PROXYCONFIG_DIRECT4X) {
                type = PROXYCONFIG_DIRECT;
                
                
                
                
                if (!pref)
                    prefBranch->SetIntPref(PROXY_PREF("type"), type);
            } else if (type >= PROXYCONFIG_COUNT) {
                LOG(("unknown proxy type: %lu; assuming direct\n", type));
                type = PROXYCONFIG_DIRECT;
            }
            mProxyConfig = type;
            reloadPAC = true;
        }

        if (mProxyConfig == PROXYCONFIG_SYSTEM) {
            mSystemProxySettings = do_GetService(NS_SYSTEMPROXYSETTINGS_CONTRACTID);
            if (!mSystemProxySettings)
                mProxyConfig = PROXYCONFIG_DIRECT;
            ResetPACThread();
        } else {
            if (mSystemProxySettings) {
                mSystemProxySettings = nullptr;
                ResetPACThread();
            }
        }
    }

    if (!pref || !strcmp(pref, PROXY_PREF("http")))
        proxy_GetStringPref(prefBranch, PROXY_PREF("http"), mHTTPProxyHost);

    if (!pref || !strcmp(pref, PROXY_PREF("http_port")))
        proxy_GetIntPref(prefBranch, PROXY_PREF("http_port"), mHTTPProxyPort);

    if (!pref || !strcmp(pref, PROXY_PREF("ssl")))
        proxy_GetStringPref(prefBranch, PROXY_PREF("ssl"), mHTTPSProxyHost);

    if (!pref || !strcmp(pref, PROXY_PREF("ssl_port")))
        proxy_GetIntPref(prefBranch, PROXY_PREF("ssl_port"), mHTTPSProxyPort);

    if (!pref || !strcmp(pref, PROXY_PREF("ftp")))
        proxy_GetStringPref(prefBranch, PROXY_PREF("ftp"), mFTPProxyHost);

    if (!pref || !strcmp(pref, PROXY_PREF("ftp_port")))
        proxy_GetIntPref(prefBranch, PROXY_PREF("ftp_port"), mFTPProxyPort);

    if (!pref || !strcmp(pref, PROXY_PREF("socks")))
        proxy_GetStringPref(prefBranch, PROXY_PREF("socks"), mSOCKSProxyHost);
    
    if (!pref || !strcmp(pref, PROXY_PREF("socks_port")))
        proxy_GetIntPref(prefBranch, PROXY_PREF("socks_port"), mSOCKSProxyPort);

    if (!pref || !strcmp(pref, PROXY_PREF("socks_version"))) {
        int32_t version;
        proxy_GetIntPref(prefBranch, PROXY_PREF("socks_version"), version);
        
        if (version == 5)
            mSOCKSProxyVersion = 5;
        else
            mSOCKSProxyVersion = 4;
    }

    if (!pref || !strcmp(pref, PROXY_PREF("socks_remote_dns")))
        proxy_GetBoolPref(prefBranch, PROXY_PREF("socks_remote_dns"),
                          mSOCKSProxyRemoteDNS);

    if (!pref || !strcmp(pref, PROXY_PREF("proxy_over_tls"))) {
        proxy_GetBoolPref(prefBranch, PROXY_PREF("proxy_over_tls"),
                          mProxyOverTLS);
    }

    if (!pref || !strcmp(pref, PROXY_PREF("failover_timeout")))
        proxy_GetIntPref(prefBranch, PROXY_PREF("failover_timeout"),
                         mFailedProxyTimeout);

    if (!pref || !strcmp(pref, PROXY_PREF("no_proxies_on"))) {
        rv = prefBranch->GetCharPref(PROXY_PREF("no_proxies_on"),
                                     getter_Copies(tempString));
        if (NS_SUCCEEDED(rv))
            LoadHostFilters(tempString.get());
    }

    
    
    if (mProxyConfig != PROXYCONFIG_PAC && mProxyConfig != PROXYCONFIG_WPAD &&
        mProxyConfig != PROXYCONFIG_SYSTEM)
        return;

    
    
    

    if (!pref || !strcmp(pref, PROXY_PREF("autoconfig_url")))
        reloadPAC = true;

    if (reloadPAC) {
        tempString.Truncate();
        if (mProxyConfig == PROXYCONFIG_PAC) {
            prefBranch->GetCharPref(PROXY_PREF("autoconfig_url"),
                                    getter_Copies(tempString));
            if (mPACMan && !mPACMan->IsPACURI(tempString)) {
                LOG(("PAC Thread URI Changed - Reset Pac Thread"));
                ResetPACThread();
            }
        } else if (mProxyConfig == PROXYCONFIG_WPAD) {
            
            
            
            
            
            
            tempString.AssignLiteral(WPAD_URL);
        } else if (mSystemProxySettings) {
            
            mSystemProxySettings->GetPACURI(tempString);
        }
        if (!tempString.IsEmpty())
            ConfigureFromPAC(tempString, false);
    }
}

bool
nsProtocolProxyService::CanUseProxy(nsIURI *aURI, int32_t defaultPort) 
{
    if (mHostFiltersArray.Length() == 0)
        return true;

    int32_t port;
    nsAutoCString host;
 
    nsresult rv = aURI->GetAsciiHost(host);
    if (NS_FAILED(rv) || host.IsEmpty())
        return false;

    rv = aURI->GetPort(&port);
    if (NS_FAILED(rv))
        return false;
    if (port == -1)
        port = defaultPort;

    PRNetAddr addr;
    bool is_ipaddr = (PR_StringToNetAddr(host.get(), &addr) == PR_SUCCESS);

    PRIPv6Addr ipv6;
    if (is_ipaddr) {
        
        if (addr.raw.family == PR_AF_INET) {
            
            PR_ConvertIPv4AddrToIPv6(addr.inet.ip, &ipv6);
        }
        else if (addr.raw.family == PR_AF_INET6) {
            
            memcpy(&ipv6, &addr.ipv6.ip, sizeof(PRIPv6Addr));
        }
        else {
            NS_WARNING("unknown address family");
            return true; 
        }
    }
    
    
    if (!is_ipaddr && mFilterLocalHosts && (kNotFound == host.FindChar('.'))) {
        LOG(("Not using proxy for this local host [%s]!\n", host.get()));
        return false; 
    }

    int32_t index = -1;
    while (++index < int32_t(mHostFiltersArray.Length())) {
        HostInfo *hinfo = mHostFiltersArray[index];

        if (is_ipaddr != hinfo->is_ipaddr)
            continue;
        if (hinfo->port && hinfo->port != port)
            continue;

        if (is_ipaddr) {
            
            PRIPv6Addr masked;
            memcpy(&masked, &ipv6, sizeof(PRIPv6Addr));
            proxy_MaskIPv6Addr(masked, hinfo->ip.mask_len);

            
            if (memcmp(&masked, &hinfo->ip.addr, sizeof(PRIPv6Addr)) == 0)
                return false; 
        }
        else {
            uint32_t host_len = host.Length();
            uint32_t filter_host_len = hinfo->name.host_len;

            if (host_len >= filter_host_len) {
                
                
                
                const char *host_tail = host.get() + host_len - filter_host_len;
                if (!PL_strncasecmp(host_tail, hinfo->name.host, filter_host_len))
                    return false; 
            }
        }
    }
    return true;
}



namespace mozilla {
const char kProxyType_HTTP[]    = "http";
const char kProxyType_HTTPS[]   = "https";
const char kProxyType_PROXY[]   = "proxy";
const char kProxyType_SOCKS[]   = "socks";
const char kProxyType_SOCKS4[]  = "socks4";
const char kProxyType_SOCKS5[]  = "socks5";
const char kProxyType_DIRECT[]  = "direct";
}

const char *
nsProtocolProxyService::ExtractProxyInfo(const char *start,
                                         uint32_t aResolveFlags,
                                         nsProxyInfo **result)
{
    *result = nullptr;
    uint32_t flags = 0;

    

    
    const char *end = start;
    while (*end && *end != ';') ++end;

    
    const char *sp = start;
    while (sp < end && *sp != ' ' && *sp != '\t') ++sp;

    uint32_t len = sp - start;
    const char *type = nullptr;
    switch (len) {
    case 4:
        if (PL_strncasecmp(start, kProxyType_HTTP, 5) == 0) {
            type = kProxyType_HTTP;
        }
        break;
    case 5:
        if (PL_strncasecmp(start, kProxyType_PROXY, 5) == 0) {
            type = kProxyType_HTTP;
        } else if (PL_strncasecmp(start, kProxyType_SOCKS, 5) == 0) {
            type = kProxyType_SOCKS4; 
        } else if (PL_strncasecmp(start, kProxyType_HTTPS, 5) == 0) {
            type = kProxyType_HTTPS;
        }
        break;
    case 6:
        if (PL_strncasecmp(start, kProxyType_DIRECT, 6) == 0)
            type = kProxyType_DIRECT;
        else if (PL_strncasecmp(start, kProxyType_SOCKS4, 6) == 0)
            type = kProxyType_SOCKS4;
        else if (PL_strncasecmp(start, kProxyType_SOCKS5, 6) == 0)
            
            
            type = kProxyType_SOCKS;
        break;
    }
    if (type) {
        const char *host = nullptr, *hostEnd = nullptr;
        int32_t port = -1;

        
        
        
        if (type == kProxyType_SOCKS || mSOCKSProxyRemoteDNS)
            flags |= nsIProxyInfo::TRANSPARENT_PROXY_RESOLVES_HOST;

        
        start = sp;
        while ((*start == ' ' || *start == '\t') && start < end)
            start++;

        
        if (type == kProxyType_HTTP) {
            port = 80;
        } else if (type == kProxyType_HTTPS) {
            port = 443;
        } else {
            port = 1080;
        }

        nsProxyInfo *pi = new nsProxyInfo();
        pi->mType = type;
        pi->mFlags = flags;
        pi->mResolveFlags = aResolveFlags;
        pi->mTimeout = mFailedProxyTimeout;

        
        nsDependentCSubstring maybeURL(start, end - start);
        nsCOMPtr<nsIURI> pacURI;

        nsAutoCString urlHost;
        if (NS_SUCCEEDED(NS_NewURI(getter_AddRefs(pacURI), maybeURL)) &&
            NS_SUCCEEDED(pacURI->GetAsciiHost(urlHost)) &&
            !urlHost.IsEmpty()) {
            

            pi->mHost = urlHost;

            int32_t tPort;
            if (NS_SUCCEEDED(pacURI->GetPort(&tPort)) && tPort != -1) {
                port = tPort;
            }
            pi->mPort = port;
        }
        else {
            
            if (start < end) {
                host = start;
                hostEnd = strchr(host, ':');
                if (!hostEnd || hostEnd > end) {
                    hostEnd = end;
                    
                }
                else {
                    port = atoi(hostEnd + 1);
                }
            }
            
            if (host) {
                pi->mHost.Assign(host, hostEnd - host);
                pi->mPort = port;
            }
        }
        NS_ADDREF(*result = pi);
    }

    while (*end == ';' || *end == ' ' || *end == '\t')
        ++end;
    return end;
}

void
nsProtocolProxyService::GetProxyKey(nsProxyInfo *pi, nsCString &key)
{
    key.AssignASCII(pi->mType);
    if (!pi->mHost.IsEmpty()) {
        key.Append(' ');
        key.Append(pi->mHost);
        key.Append(':');
        key.AppendInt(pi->mPort);
    }
} 

uint32_t
nsProtocolProxyService::SecondsSinceSessionStart()
{
    PRTime now = PR_Now();

    
    int64_t diff = now - mSessionStart;

    
    diff /= PR_USEC_PER_SEC;

    
    return uint32_t(diff);
}

void
nsProtocolProxyService::EnableProxy(nsProxyInfo *pi)
{
    nsAutoCString key;
    GetProxyKey(pi, key);
    mFailedProxies.Remove(key);
}

void
nsProtocolProxyService::DisableProxy(nsProxyInfo *pi)
{
    nsAutoCString key;
    GetProxyKey(pi, key);

    uint32_t dsec = SecondsSinceSessionStart();

    
    
    dsec += pi->mTimeout;

    
    
    
    
    
    
    

    LOG(("DisableProxy %s %d\n", key.get(), dsec));

    
    
    mFailedProxies.Put(key, dsec);
}

bool
nsProtocolProxyService::IsProxyDisabled(nsProxyInfo *pi)
{
    nsAutoCString key;
    GetProxyKey(pi, key);

    uint32_t val;
    if (!mFailedProxies.Get(key, &val))
        return false;

    uint32_t dsec = SecondsSinceSessionStart();

    
    if (dsec > val) {
        mFailedProxies.Remove(key);
        return false;
    }

    return true;
}

nsresult
nsProtocolProxyService::SetupPACThread()
{
    if (mPACMan)
        return NS_OK;

    mPACMan = new nsPACMan();

    bool mainThreadOnly;
    nsresult rv;
    if (mSystemProxySettings &&
        NS_SUCCEEDED(mSystemProxySettings->GetMainThreadOnly(&mainThreadOnly)) &&
        !mainThreadOnly) {
        rv = mPACMan->Init(mSystemProxySettings);
    }
    else {
        rv = mPACMan->Init(nullptr);
    }

    if (NS_FAILED(rv))
        mPACMan = nullptr;
    return rv;
}

nsresult
nsProtocolProxyService::ResetPACThread()
{
    if (!mPACMan)
        return NS_OK;

    mPACMan->Shutdown();
    mPACMan = nullptr;
    return SetupPACThread();
}

nsresult
nsProtocolProxyService::ConfigureFromPAC(const nsCString &spec,
                                         bool forceReload)
{
    SetupPACThread();

    if (mPACMan->IsPACURI(spec) && !forceReload)
        return NS_OK;

    mFailedProxies.Clear();

    return mPACMan->LoadPACFromURI(spec);
}

void
nsProtocolProxyService::ProcessPACString(const nsCString &pacString,
                                         uint32_t aResolveFlags,
                                         nsIProxyInfo **result)
{
    if (pacString.IsEmpty()) {
        *result = nullptr;
        return;
    }

    const char *proxies = pacString.get();

    nsProxyInfo *pi = nullptr, *first = nullptr, *last = nullptr;
    while (*proxies) {
        proxies = ExtractProxyInfo(proxies, aResolveFlags, &pi);
        if (pi && (pi->mType == kProxyType_HTTPS) && !mProxyOverTLS) {
            delete pi;
            pi = nullptr;
        }

        if (pi) {
            if (last) {
                NS_ASSERTION(last->mNext == nullptr, "leaking nsProxyInfo");
                last->mNext = pi;
            }
            else
                first = pi;
            last = pi;
        }
    }
    *result = first;
}


NS_IMETHODIMP
nsProtocolProxyService::ReloadPAC()
{
    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!prefs)
        return NS_OK;

    int32_t type;
    nsresult rv = prefs->GetIntPref(PROXY_PREF("type"), &type);
    if (NS_FAILED(rv))
        return NS_OK;

    nsXPIDLCString pacSpec;
    if (type == PROXYCONFIG_PAC)
        prefs->GetCharPref(PROXY_PREF("autoconfig_url"), getter_Copies(pacSpec));
    else if (type == PROXYCONFIG_WPAD)
        pacSpec.AssignLiteral(WPAD_URL);

    if (!pacSpec.IsEmpty())
        ConfigureFromPAC(pacSpec, true);
    return NS_OK;
}







class nsAsyncBridgeRequest MOZ_FINAL  : public nsPACManCallback
{
    NS_DECL_THREADSAFE_ISUPPORTS

     nsAsyncBridgeRequest()
        : mMutex("nsDeprecatedCallback")
        , mCondVar(mMutex, "nsDeprecatedCallback")
        , mCompleted(false)
    {
    }

    void OnQueryComplete(nsresult status,
                         const nsCString &pacString,
                         const nsCString &newPACURL)
    {
        MutexAutoLock lock(mMutex);
        mCompleted = true;
        mStatus = status;
        mPACString = pacString;
        mPACURL = newPACURL;
        mCondVar.Notify();
    }

    void Lock()   { mMutex.Lock(); }
    void Unlock() { mMutex.Unlock(); }
    void Wait()   { mCondVar.Wait(PR_SecondsToInterval(3)); }

private:
    ~nsAsyncBridgeRequest()
    {
    }

    friend class nsProtocolProxyService;

    Mutex    mMutex;
    CondVar  mCondVar;

    nsresult  mStatus;
    nsCString mPACString;
    nsCString mPACURL;
    bool      mCompleted;
};
NS_IMPL_ISUPPORTS0(nsAsyncBridgeRequest)


NS_IMETHODIMP
nsProtocolProxyService::DeprecatedBlockingResolve(nsIURI *aURI,
                                                  uint32_t aFlags,
                                                  nsIProxyInfo **retval)
{
    NS_ENSURE_ARG_POINTER(aURI);

    nsProtocolInfo info;
    nsresult rv = GetProtocolInfo(aURI, &info);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIProxyInfo> pi;
    bool usePACThread;

    
    
    

    rv = Resolve_Internal(aURI, info, aFlags, &usePACThread, getter_AddRefs(pi));
    if (NS_FAILED(rv))
        return rv;

    if (!usePACThread || !mPACMan) {
        ApplyFilters(aURI, info, pi);
        pi.forget(retval);
        return NS_OK;
    }

    
    
    nsRefPtr<nsAsyncBridgeRequest> ctx = new nsAsyncBridgeRequest();
    ctx->Lock();
    if (NS_SUCCEEDED(mPACMan->AsyncGetProxyForURI(aURI, ctx, false))) {
        
       ctx->Wait();
    }
    ctx->Unlock();
    if (!ctx->mCompleted)
        return NS_ERROR_FAILURE;
    if (NS_FAILED(ctx->mStatus))
        return ctx->mStatus;

    

    
    if (!ctx->mPACString.IsEmpty()) {
        LOG(("sync pac thread callback %s\n", ctx->mPACString.get()));
        ProcessPACString(ctx->mPACString, 0, getter_AddRefs(pi));
        ApplyFilters(aURI, info, pi);
        pi.forget(retval);
        return NS_OK;
    }

    if (!ctx->mPACURL.IsEmpty()) {
        NS_WARNING("sync pac thread callback indicates new pac file load\n");
        
        
        
        
        

        rv = ConfigureFromPAC(ctx->mPACURL, false);
        if (NS_FAILED(rv))
            return rv;
        return NS_ERROR_NOT_AVAILABLE;
    }

    *retval = nullptr;
    return NS_OK;
}

nsresult
nsProtocolProxyService::AsyncResolveInternal(nsIURI *uri, uint32_t flags,
                                             nsIProtocolProxyCallback *callback,
                                             nsICancelable **result,
                                             bool isSyncOK)
{
    NS_ENSURE_ARG_POINTER(uri);
    NS_ENSURE_ARG_POINTER(callback);

    *result = nullptr;
    nsRefPtr<nsAsyncResolveRequest> ctx =
        new nsAsyncResolveRequest(this, uri, flags, callback);

    nsProtocolInfo info;
    nsresult rv = GetProtocolInfo(uri, &info);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIProxyInfo> pi;
    bool usePACThread;

    
    
    

    rv = Resolve_Internal(uri, info, flags, &usePACThread, getter_AddRefs(pi));
    if (NS_FAILED(rv))
        return rv;

    if (!usePACThread || !mPACMan) {
        
        ApplyFilters(uri, info, pi);
        ctx->SetResult(NS_OK, pi);
        if (isSyncOK) {
            ctx->Run();
            return NS_OK;
        }

        rv = ctx->DispatchCallback();
        if (NS_SUCCEEDED(rv))
            ctx.forget(result);
        return rv;
    }

    

    rv = mPACMan->AsyncGetProxyForURI(uri, ctx, true);
    if (NS_SUCCEEDED(rv))
        ctx.forget(result);
    return rv;
}


NS_IMETHODIMP
nsProtocolProxyService::AsyncResolve2(nsIURI *uri, uint32_t flags,
                                      nsIProtocolProxyCallback *callback,
                                      nsICancelable **result)
{
    return AsyncResolveInternal(uri, flags, callback, result, true);
}

NS_IMETHODIMP
nsProtocolProxyService::AsyncResolve(nsIURI *uri, uint32_t flags,
                                     nsIProtocolProxyCallback *callback,
                                     nsICancelable **result)
{
    return AsyncResolveInternal(uri, flags, callback, result, false);
}

NS_IMETHODIMP
nsProtocolProxyService::NewProxyInfo(const nsACString &aType,
                                     const nsACString &aHost,
                                     int32_t aPort,
                                     uint32_t aFlags,
                                     uint32_t aFailoverTimeout,
                                     nsIProxyInfo *aFailoverProxy,
                                     nsIProxyInfo **aResult)
{
    static const char *types[] = {
        kProxyType_HTTP,
        kProxyType_HTTPS,
        kProxyType_SOCKS,
        kProxyType_SOCKS4,
        kProxyType_DIRECT
    };

    
    
    const char *type = nullptr;
    for (uint32_t i = 0; i < ArrayLength(types); ++i) {
        if (aType.LowerCaseEqualsASCII(types[i])) {
            type = types[i];
            break;
        }
    }
    NS_ENSURE_TRUE(type, NS_ERROR_INVALID_ARG);

    if (aPort <= 0)
        aPort = -1;

    return NewProxyInfo_Internal(type, aHost, aPort, aFlags, aFailoverTimeout,
                                 aFailoverProxy, 0, aResult);
}

NS_IMETHODIMP
nsProtocolProxyService::GetFailoverForProxy(nsIProxyInfo  *aProxy,
                                            nsIURI        *aURI,
                                            nsresult       aStatus,
                                            nsIProxyInfo **aResult)
{
    
    
    if (mProxyConfig != PROXYCONFIG_PAC && mProxyConfig != PROXYCONFIG_WPAD &&
        mProxyConfig != PROXYCONFIG_SYSTEM)
        return NS_ERROR_NOT_AVAILABLE;

    
    nsCOMPtr<nsProxyInfo> pi = do_QueryInterface(aProxy);
    NS_ENSURE_ARG(pi);
    

    
    DisableProxy(pi);

    
    
    

    if (!pi->mNext)
        return NS_ERROR_NOT_AVAILABLE;

    LOG(("PAC failover from %s %s:%d to %s %s:%d\n",
        pi->mType, pi->mHost.get(), pi->mPort,
        pi->mNext->mType, pi->mNext->mHost.get(), pi->mNext->mPort));

    NS_ADDREF(*aResult = pi->mNext);
    return NS_OK;
}

NS_IMETHODIMP
nsProtocolProxyService::RegisterFilter(nsIProtocolProxyFilter *filter,
                                       uint32_t position)
{
    UnregisterFilter(filter);  

    FilterLink *link = new FilterLink(position, filter);
    if (!link)
        return NS_ERROR_OUT_OF_MEMORY;

    if (!mFilters) {
        mFilters = link;
        return NS_OK;
    }

    
    FilterLink *last = nullptr;
    for (FilterLink *iter = mFilters; iter; iter = iter->next) {
        if (position < iter->position) {
            if (last) {
                link->next = last->next;
                last->next = link;
            }
            else {
                link->next = mFilters;
                mFilters = link;
            }
            return NS_OK;
        }
        last = iter;
    }
    
    last->next = link;
    return NS_OK;
}

NS_IMETHODIMP
nsProtocolProxyService::UnregisterFilter(nsIProtocolProxyFilter *filter)
{
    
    nsCOMPtr<nsISupports> givenObject = do_QueryInterface(filter);

    FilterLink *last = nullptr;
    for (FilterLink *iter = mFilters; iter; iter = iter->next) {
        nsCOMPtr<nsISupports> object = do_QueryInterface(iter->filter);
        if (object == givenObject) {
            if (last)
                last->next = iter->next;
            else
                mFilters = iter->next;
            iter->next = nullptr;
            delete iter;
            return NS_OK;
        }
        last = iter;
    }

    
    return NS_OK;
}

NS_IMETHODIMP
nsProtocolProxyService::GetProxyConfigType(uint32_t* aProxyConfigType)
{
  *aProxyConfigType = mProxyConfig;
  return NS_OK;
}

void
nsProtocolProxyService::LoadHostFilters(const char *filters)
{
    
    if (mHostFiltersArray.Length() > 0) {
        mHostFiltersArray.Clear();
    }

    if (!filters)
        return; 

    
    
    
    
    
    mFilterLocalHosts = false;
    while (*filters) {
        
        while (*filters && (*filters == ',' || IS_ASCII_SPACE(*filters)))
            filters++;

        const char *starthost = filters;
        const char *endhost = filters + 1; 
        const char *portLocation = 0; 
        const char *maskLocation = 0;

        while (*endhost && (*endhost != ',' && !IS_ASCII_SPACE(*endhost))) {
            if (*endhost == ':')
                portLocation = endhost;
            else if (*endhost == '/')
                maskLocation = endhost;
            else if (*endhost == ']') 
                portLocation = 0;
            endhost++;
        }

        filters = endhost; 

        
        const char *end = maskLocation ? maskLocation :
                          portLocation ? portLocation :
                          endhost;

        nsAutoCString str(starthost, end - starthost);

        
        
        if (str.EqualsIgnoreCase("<local>")) {
            mFilterLocalHosts = true;
            LOG(("loaded filter for local hosts "
                 "(plain host names, no dots)\n"));
            
            continue;
        }

        
        HostInfo *hinfo = new HostInfo();
        hinfo->port = portLocation ? atoi(portLocation + 1) : 0;

        PRNetAddr addr;
        if (PR_StringToNetAddr(str.get(), &addr) == PR_SUCCESS) {
            hinfo->is_ipaddr   = true;
            hinfo->ip.family   = PR_AF_INET6; 
            hinfo->ip.mask_len = maskLocation ? atoi(maskLocation + 1) : 128;

            if (hinfo->ip.mask_len == 0) {
                NS_WARNING("invalid mask");
                goto loser;
            }

            if (addr.raw.family == PR_AF_INET) {
                
                PR_ConvertIPv4AddrToIPv6(addr.inet.ip, &hinfo->ip.addr);
                
                if (hinfo->ip.mask_len <= 32)
                    hinfo->ip.mask_len += 96;
            }
            else if (addr.raw.family == PR_AF_INET6) {
                
                memcpy(&hinfo->ip.addr, &addr.ipv6.ip, sizeof(PRIPv6Addr));
            }
            else {
                NS_WARNING("unknown address family");
                goto loser;
            }

            
            proxy_MaskIPv6Addr(hinfo->ip.addr, hinfo->ip.mask_len);
        }
        else {
            uint32_t startIndex, endIndex;
            if (str.First() == '*')
                startIndex = 1; 
            else
                startIndex = 0;
            endIndex = (portLocation ? portLocation : endhost) - starthost;

            hinfo->is_ipaddr = false;
            hinfo->name.host = ToNewCString(Substring(str, startIndex, endIndex));

            if (!hinfo->name.host)
                goto loser;

            hinfo->name.host_len = endIndex - startIndex;
        }


#ifdef DEBUG_DUMP_FILTERS
        printf("loaded filter[%u]:\n", mHostFiltersArray.Length());
        printf("  is_ipaddr = %u\n", hinfo->is_ipaddr);
        printf("  port = %u\n", hinfo->port);
        if (hinfo->is_ipaddr) {
            printf("  ip.family = %x\n", hinfo->ip.family);
            printf("  ip.mask_len = %u\n", hinfo->ip.mask_len);

            PRNetAddr netAddr;
            PR_SetNetAddr(PR_IpAddrNull, PR_AF_INET6, 0, &netAddr);
            memcpy(&netAddr.ipv6.ip, &hinfo->ip.addr, sizeof(hinfo->ip.addr));

            char buf[256];
            PR_NetAddrToString(&netAddr, buf, sizeof(buf));

            printf("  ip.addr = %s\n", buf);
        }
        else {
            printf("  name.host = %s\n", hinfo->name.host);
        }
#endif

        mHostFiltersArray.AppendElement(hinfo);
        hinfo = nullptr;
loser:
        delete hinfo;
    }
}

nsresult
nsProtocolProxyService::GetProtocolInfo(nsIURI *uri, nsProtocolInfo *info)
{
    NS_PRECONDITION(uri, "URI is null");
    NS_PRECONDITION(info, "info is null");

    nsresult rv;

    rv = uri->GetScheme(info->scheme);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIIOService> ios = do_GetIOService(&rv);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIProtocolHandler> handler;
    rv = ios->GetProtocolHandler(info->scheme.get(), getter_AddRefs(handler));
    if (NS_FAILED(rv))
        return rv;

    rv = handler->GetProtocolFlags(&info->flags);
    if (NS_FAILED(rv))
        return rv;

    rv = handler->GetDefaultPort(&info->defaultPort);
    return rv;
}

nsresult
nsProtocolProxyService::NewProxyInfo_Internal(const char *aType,
                                              const nsACString &aHost,
                                              int32_t aPort,
                                              uint32_t aFlags,
                                              uint32_t aFailoverTimeout,
                                              nsIProxyInfo *aFailoverProxy,
                                              uint32_t aResolveFlags,
                                              nsIProxyInfo **aResult)
{
    nsCOMPtr<nsProxyInfo> failover;
    if (aFailoverProxy) {
        failover = do_QueryInterface(aFailoverProxy);
        NS_ENSURE_ARG(failover);
    }

    nsProxyInfo *proxyInfo = new nsProxyInfo();
    if (!proxyInfo)
        return NS_ERROR_OUT_OF_MEMORY;

    proxyInfo->mType = aType;
    proxyInfo->mHost = aHost;
    proxyInfo->mPort = aPort;
    proxyInfo->mFlags = aFlags;
    proxyInfo->mResolveFlags = aResolveFlags;
    proxyInfo->mTimeout = aFailoverTimeout == UINT32_MAX
        ? mFailedProxyTimeout : aFailoverTimeout;
    failover.swap(proxyInfo->mNext);

    NS_ADDREF(*aResult = proxyInfo);
    return NS_OK;
}

nsresult
nsProtocolProxyService::Resolve_Internal(nsIURI *uri,
                                         const nsProtocolInfo &info,
                                         uint32_t flags,
                                         bool *usePACThread,
                                         nsIProxyInfo **result)
{
    NS_ENSURE_ARG_POINTER(uri);
    nsresult rv = SetupPACThread();
    if (NS_FAILED(rv))
        return rv;

    *usePACThread = false;
    *result = nullptr;

    if (!(info.flags & nsIProtocolHandler::ALLOWS_PROXY))
        return NS_OK;  

    
    
    
    if (mPACMan && mPACMan->IsPACURI(uri))
        return NS_OK;

    bool mainThreadOnly;
    if (mSystemProxySettings &&
        mProxyConfig == PROXYCONFIG_SYSTEM &&
        NS_SUCCEEDED(mSystemProxySettings->GetMainThreadOnly(&mainThreadOnly)) &&
        !mainThreadOnly) {
        *usePACThread = true;
        return NS_OK;
    }

    if (mSystemProxySettings && mProxyConfig == PROXYCONFIG_SYSTEM) {
        
        
        

        nsAutoCString PACURI;
        nsAutoCString pacString;

        if (NS_SUCCEEDED(mSystemProxySettings->GetPACURI(PACURI)) &&
            !PACURI.IsEmpty()) {
            
            
            

            if (mPACMan && mPACMan->IsPACURI(PACURI)) {
                
                *usePACThread = true;
                return NS_OK;
            }

            ConfigureFromPAC(PACURI, false);
            return NS_OK;
        }

        nsAutoCString spec;
        nsAutoCString host;
        nsAutoCString scheme;
        int32_t port = -1;

        uri->GetAsciiSpec(spec);
        uri->GetAsciiHost(host);
        uri->GetScheme(scheme);
        uri->GetPort(&port);

        
        if (NS_SUCCEEDED(mSystemProxySettings->
                         GetProxyForURI(spec, scheme, host, port,
                                        pacString))) {
            ProcessPACString(pacString, 0, result);
            return NS_OK;
        }
    }

    
    
    if (mProxyConfig == PROXYCONFIG_DIRECT ||
        (mProxyConfig == PROXYCONFIG_MANUAL &&
         !CanUseProxy(uri, info.defaultPort)))
        return NS_OK;

    
    if (mProxyConfig == PROXYCONFIG_PAC || mProxyConfig == PROXYCONFIG_WPAD) {
        
        *usePACThread = true;
        return NS_OK;
    }

    
    
    if (mProxyConfig != PROXYCONFIG_MANUAL)
        return NS_OK;

    
    const char *type = nullptr;
    const nsACString *host = nullptr;
    int32_t port = -1;

    uint32_t proxyFlags = 0;

    if ((flags & RESOLVE_PREFER_SOCKS_PROXY) &&
        !mSOCKSProxyHost.IsEmpty() && mSOCKSProxyPort > 0) {
      host = &mSOCKSProxyHost;
      if (mSOCKSProxyVersion == 4)
          type = kProxyType_SOCKS4;
      else
          type = kProxyType_SOCKS;
      port = mSOCKSProxyPort;
      if (mSOCKSProxyRemoteDNS)
          proxyFlags |= nsIProxyInfo::TRANSPARENT_PROXY_RESOLVES_HOST;
    }
    else if ((flags & RESOLVE_PREFER_HTTPS_PROXY) &&
             !mHTTPSProxyHost.IsEmpty() && mHTTPSProxyPort > 0) {
        host = &mHTTPSProxyHost;
        type = kProxyType_HTTP;
        port = mHTTPSProxyPort;
    }
    else if (!mHTTPProxyHost.IsEmpty() && mHTTPProxyPort > 0 &&
             ((flags & RESOLVE_IGNORE_URI_SCHEME) ||
              info.scheme.EqualsLiteral("http"))) {
        host = &mHTTPProxyHost;
        type = kProxyType_HTTP;
        port = mHTTPProxyPort;
    }
    else if (!mHTTPSProxyHost.IsEmpty() && mHTTPSProxyPort > 0 &&
             !(flags & RESOLVE_IGNORE_URI_SCHEME) &&
             info.scheme.EqualsLiteral("https")) {
        host = &mHTTPSProxyHost;
        type = kProxyType_HTTP;
        port = mHTTPSProxyPort;
    }
    else if (!mFTPProxyHost.IsEmpty() && mFTPProxyPort > 0 &&
             !(flags & RESOLVE_IGNORE_URI_SCHEME) &&
             info.scheme.EqualsLiteral("ftp")) {
        host = &mFTPProxyHost;
        type = kProxyType_HTTP;
        port = mFTPProxyPort;
    }
    else if (!mSOCKSProxyHost.IsEmpty() && mSOCKSProxyPort > 0) {
        host = &mSOCKSProxyHost;
        if (mSOCKSProxyVersion == 4)
            type = kProxyType_SOCKS4;
        else
            type = kProxyType_SOCKS;
        port = mSOCKSProxyPort;
        if (mSOCKSProxyRemoteDNS)
            proxyFlags |= nsIProxyInfo::TRANSPARENT_PROXY_RESOLVES_HOST;
    }

    if (type) {
        rv = NewProxyInfo_Internal(type, *host, port, proxyFlags,
                                   UINT32_MAX, nullptr, flags,
                                   result);
        if (NS_FAILED(rv))
            return rv;
    }

    return NS_OK;
}

void
nsProtocolProxyService::MaybeDisableDNSPrefetch(nsIProxyInfo *aProxy)
{
    
    if (!aProxy)
        return;

    nsCOMPtr<nsProxyInfo> pi = do_QueryInterface(aProxy);
    if (!pi ||
        !pi->mType ||
        pi->mType == kProxyType_DIRECT)
        return;

    nsCOMPtr<nsIDNSService> dns = do_GetService(NS_DNSSERVICE_CONTRACTID);
    if (!dns)
        return;
    nsCOMPtr<nsPIDNSService> pdns = do_QueryInterface(dns);
    if (!pdns)
        return;

    
    pdns->SetPrefetchEnabled(false);
}

void
nsProtocolProxyService::ApplyFilters(nsIURI *uri, const nsProtocolInfo &info,
                                     nsIProxyInfo **list)
{
    if (!(info.flags & nsIProtocolHandler::ALLOWS_PROXY))
        return;

    
    
    

    nsresult rv;
    nsCOMPtr<nsIProxyInfo> result;

    for (FilterLink *iter = mFilters; iter; iter = iter->next) {
        PruneProxyInfo(info, list);

        rv = iter->filter->ApplyFilter(this, uri, *list,
                                       getter_AddRefs(result));
        if (NS_FAILED(rv))
            continue;
        result.swap(*list);
    }

    PruneProxyInfo(info, list);
}

void
nsProtocolProxyService::PruneProxyInfo(const nsProtocolInfo &info,
                                       nsIProxyInfo **list)
{
    if (!*list)
        return;
    nsProxyInfo *head = nullptr;
    CallQueryInterface(*list, &head);
    if (!head) {
        NS_NOTREACHED("nsIProxyInfo must QI to nsProxyInfo");
        return;
    }
    NS_RELEASE(*list);

    
    
    
    
    
    

    
    if (!(info.flags & nsIProtocolHandler::ALLOWS_PROXY_HTTP)) {
        nsProxyInfo *last = nullptr, *iter = head;
        while (iter) {
            if ((iter->Type() == kProxyType_HTTP) ||
                (iter->Type() == kProxyType_HTTPS)) {
                
                if (last)
                    last->mNext = iter->mNext;
                else
                    head = iter->mNext;
                nsProxyInfo *next = iter->mNext;
                iter->mNext = nullptr;
                iter->Release();
                iter = next;
            } else {
                last = iter;
                iter = iter->mNext;
            }
        }
        if (!head)
            return;
    }

    
    
    

    bool allDisabled = true;

    nsProxyInfo *iter;
    for (iter = head; iter; iter = iter->mNext) {
        if (!IsProxyDisabled(iter)) {
            allDisabled = false;
            break;
        }
    }

    if (allDisabled)
        LOG(("All proxies are disabled, so trying all again"));
    else {
        
        nsProxyInfo *last = nullptr;
        for (iter = head; iter; ) {
            if (IsProxyDisabled(iter)) {
                
                nsProxyInfo *reject = iter;

                iter = iter->mNext;
                if (last)
                    last->mNext = iter;
                else
                    head = iter;

                reject->mNext = nullptr;
                NS_RELEASE(reject);
                continue;
            }

            
            
            
            
            
            
            EnableProxy(iter);

            last = iter;
            iter = iter->mNext;
        }
    }

    
    if (head && !head->mNext && head->mType == kProxyType_DIRECT)
        NS_RELEASE(head);

    *list = head;  
}
