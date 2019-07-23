






































#include "nsProtocolProxyService.h"
#include "nsProxyInfo.h"
#include "nsIClassInfoImpl.h"
#include "nsIServiceManager.h"
#include "nsXPIDLString.h"
#include "nsIProxyAutoConfig.h"
#include "nsAutoLock.h"
#include "nsIIOService.h"
#include "nsIObserverService.h"
#include "nsIProtocolHandler.h"
#include "nsIProtocolProxyCallback.h"
#include "nsICancelable.h"
#include "nsIDNSService.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"
#include "nsReadableUtils.h"
#include "nsThreadUtils.h"
#include "nsString.h"
#include "nsNetUtil.h"
#include "nsCRT.h"
#include "prnetdb.h"
#include "nsPACMan.h"



#include "prlog.h"
#if defined(PR_LOGGING)
static PRLogModuleInfo *sLog = PR_NewLogModule("proxy");
#endif
#define LOG(args) PR_LOG(sLog, PR_LOG_DEBUG, args)



#define PROXY_PREF_BRANCH  "network.proxy"
#define PROXY_PREF(x)      PROXY_PREF_BRANCH "." x

#define WPAD_URL "http://wpad/wpad.dat"




struct nsProtocolInfo {
    nsCAutoString scheme;
    PRUint32 flags;
    PRInt32 defaultPort;
};



class nsAsyncResolveRequest : public nsIRunnable
                            , public nsPACManCallback 
                            , public nsICancelable
{
public:
    NS_DECL_ISUPPORTS

    nsAsyncResolveRequest(nsProtocolProxyService *pps, nsIURI *uri,
                          nsIProtocolProxyCallback *callback)
        : mStatus(NS_OK)
        , mDispatched(PR_FALSE)
        , mPPS(pps)
        , mURI(uri)
        , mCallback(callback)
    {
        NS_ASSERTION(mCallback, "null callback");
    }

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

        SetResult(reason, nsnull);
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
            mDispatched = PR_TRUE;
            return NS_OK;
        }

        mCallback = nsnull;  
        return rv;
    }

private:

    
    
    void OnQueryComplete(nsresult status, const nsCString &pacString)
    {
        
        if (!mCallback)
            return;

        
        if (mStatus == NS_OK) {
            mStatus = status;
            mPACString = pacString;
        }

        
        
        
        DoCallback();
    }

    void DoCallback()
    {
        
        if (NS_SUCCEEDED(mStatus) && !mProxyInfo && !mPACString.IsEmpty())
            mPPS->ProcessPACString(mPACString, getter_AddRefs(mProxyInfo));

        
        if (NS_SUCCEEDED(mStatus)) {
            nsProtocolInfo info;
            mStatus = mPPS->GetProtocolInfo(mURI, &info);
            if (NS_SUCCEEDED(mStatus))
                mPPS->ApplyFilters(mURI, info, mProxyInfo);
            else
                mProxyInfo = nsnull;
        }

        mCallback->OnProxyAvailable(this, mURI, mProxyInfo, mStatus);
        mCallback = nsnull;  
    }

private:

    nsresult  mStatus;
    nsCString mPACString;
    PRBool    mDispatched;

    nsRefPtr<nsProtocolProxyService>   mPPS;
    nsCOMPtr<nsIURI>                   mURI;
    nsCOMPtr<nsIProtocolProxyCallback> mCallback;
    nsCOMPtr<nsIProxyInfo>             mProxyInfo;
};

NS_IMPL_ISUPPORTS2(nsAsyncResolveRequest, nsICancelable, nsIRunnable)



#define IS_ASCII_SPACE(_c) ((_c) == ' ' || (_c) == '\t')






static void
proxy_MaskIPv6Addr(PRIPv6Addr &addr, PRUint16 mask_len)
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
                 PRInt32       &aResult)
{
    PRInt32 temp;
    nsresult rv = aPrefBranch->GetIntPref(aPref, &temp);
    if (NS_FAILED(rv)) 
        aResult = -1;
    else
        aResult = temp;
}

static void
proxy_GetBoolPref(nsIPrefBranch *aPrefBranch,
                 const char    *aPref,
                 PRBool        &aResult)
{
    PRBool temp;
    nsresult rv = aPrefBranch->GetBoolPref(aPref, &temp);
    if (NS_FAILED(rv)) 
        aResult = PR_FALSE;
    else
        aResult = temp;
}



NS_IMPL_ADDREF(nsProtocolProxyService)
NS_IMPL_RELEASE(nsProtocolProxyService)
NS_IMPL_QUERY_INTERFACE3_CI(nsProtocolProxyService,
                            nsIProtocolProxyService,
                            nsIProtocolProxyService2,
                            nsIObserver)
NS_IMPL_CI_INTERFACE_GETTER2(nsProtocolProxyService,
                             nsIProtocolProxyService,
                             nsIProtocolProxyService2)

nsProtocolProxyService::nsProtocolProxyService()
    : mFilters(nsnull)
    , mProxyConfig(eProxyConfig_Direct)
    , mHTTPProxyPort(-1)
    , mFTPProxyPort(-1)
    , mGopherProxyPort(-1)
    , mHTTPSProxyPort(-1)
    , mSOCKSProxyPort(-1)
    , mSOCKSProxyVersion(4)
    , mSOCKSProxyRemoteDNS(PR_FALSE)
    , mPACMan(nsnull)
    , mSessionStart(PR_Now())
    , mFailedProxyTimeout(30 * 60) 
{
}

nsProtocolProxyService::~nsProtocolProxyService()
{
    
    NS_ASSERTION(mHostFiltersArray.Length() == 0 && mFilters == nsnull &&
                 mPACMan == nsnull, "what happened to xpcom-shutdown?");
}


nsresult
nsProtocolProxyService::Init()
{
    if (!mFailedProxies.Init())
        return NS_ERROR_OUT_OF_MEMORY;

    
    nsCOMPtr<nsIPrefBranch2> prefBranch =
            do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefBranch) {
        
        prefBranch->AddObserver(PROXY_PREF_BRANCH, this, PR_FALSE);

        
        PrefsChanged(prefBranch, nsnull);
    }

    
    nsCOMPtr<nsIObserverService> obs =
            do_GetService("@mozilla.org/observer-service;1");
    if (obs)
        obs->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);

    return NS_OK;
}

NS_IMETHODIMP
nsProtocolProxyService::Observe(nsISupports     *aSubject,
                                const char      *aTopic,
                                const PRUnichar *aData)
{
    if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
        
        if (mHostFiltersArray.Length() > 0) {
            mHostFiltersArray.Clear();
        }
        if (mFilters) {
            delete mFilters;
            mFilters = nsnull;
        }
        if (mPACMan) {
            mPACMan->Shutdown();
            mPACMan = nsnull;
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
    PRBool reloadPAC = PR_FALSE;
    nsXPIDLCString tempString;

    if (!pref || !strcmp(pref, PROXY_PREF("type"))) {
        PRInt32 type = -1;
        rv = prefBranch->GetIntPref(PROXY_PREF("type"), &type);
        if (NS_SUCCEEDED(rv)) {
            
            if (type == eProxyConfig_Direct4x) {
                type = eProxyConfig_Direct;
                
                
                
                
                if (!pref)
                    prefBranch->SetIntPref(PROXY_PREF("type"), type);
            } else if (type >= eProxyConfig_Last) {
                LOG(("unknown proxy type: %lu; assuming direct\n", type));
                type = eProxyConfig_Direct;
            }
            mProxyConfig = static_cast<ProxyConfig>(type);
            reloadPAC = PR_TRUE;
        }

        if (mProxyConfig == eProxyConfig_System) {
            mSystemProxySettings = do_GetService(NS_SYSTEMPROXYSETTINGS_CONTRACTID);
        } else {
            mSystemProxySettings = nsnull;
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

    if (!pref || !strcmp(pref, PROXY_PREF("gopher")))
        proxy_GetStringPref(prefBranch, PROXY_PREF("gopher"), mGopherProxyHost);

    if (!pref || !strcmp(pref, PROXY_PREF("gopher_port")))
        proxy_GetIntPref(prefBranch, PROXY_PREF("gopher_port"), mGopherProxyPort);

    if (!pref || !strcmp(pref, PROXY_PREF("socks")))
        proxy_GetStringPref(prefBranch, PROXY_PREF("socks"), mSOCKSProxyHost);
    
    if (!pref || !strcmp(pref, PROXY_PREF("socks_port")))
        proxy_GetIntPref(prefBranch, PROXY_PREF("socks_port"), mSOCKSProxyPort);

    if (!pref || !strcmp(pref, PROXY_PREF("socks_version"))) {
        PRInt32 version;
        proxy_GetIntPref(prefBranch, PROXY_PREF("socks_version"), version);
        
        if (version == 5)
            mSOCKSProxyVersion = 5;
        else
            mSOCKSProxyVersion = 4;
    }

    if (!pref || !strcmp(pref, PROXY_PREF("socks_remote_dns")))
        proxy_GetBoolPref(prefBranch, PROXY_PREF("socks_remote_dns"),
                          mSOCKSProxyRemoteDNS);

    if (!pref || !strcmp(pref, PROXY_PREF("failover_timeout")))
        proxy_GetIntPref(prefBranch, PROXY_PREF("failover_timeout"),
                         mFailedProxyTimeout);

    if (!pref || !strcmp(pref, PROXY_PREF("no_proxies_on"))) {
        rv = prefBranch->GetCharPref(PROXY_PREF("no_proxies_on"),
                                     getter_Copies(tempString));
        if (NS_SUCCEEDED(rv))
            LoadHostFilters(tempString.get());
    }

    
    
    if (mProxyConfig != eProxyConfig_PAC && mProxyConfig != eProxyConfig_WPAD &&
        mProxyConfig != eProxyConfig_System)
        return;

    
    
    

    if (!pref || !strcmp(pref, PROXY_PREF("autoconfig_url")))
        reloadPAC = PR_TRUE;

    if (reloadPAC) {
        tempString.Truncate();
        if (mProxyConfig == eProxyConfig_PAC) {
            prefBranch->GetCharPref(PROXY_PREF("autoconfig_url"),
                                    getter_Copies(tempString));
        } else if (mProxyConfig == eProxyConfig_WPAD) {
            
            
            
            
            
            
            tempString.AssignLiteral(WPAD_URL);
        } else if (mSystemProxySettings) {
            
            mSystemProxySettings->GetPACURI(tempString);
        }
        if (!tempString.IsEmpty())
            ConfigureFromPAC(tempString, PR_FALSE);
    }
}

PRBool
nsProtocolProxyService::CanUseProxy(nsIURI *aURI, PRInt32 defaultPort) 
{
    if (mHostFiltersArray.Length() == 0)
        return PR_TRUE;

    PRInt32 port;
    nsCAutoString host;
 
    nsresult rv = aURI->GetAsciiHost(host);
    if (NS_FAILED(rv) || host.IsEmpty())
        return PR_FALSE;

    rv = aURI->GetPort(&port);
    if (NS_FAILED(rv))
        return PR_FALSE;
    if (port == -1)
        port = defaultPort;

    PRNetAddr addr;
    PRBool is_ipaddr = (PR_StringToNetAddr(host.get(), &addr) == PR_SUCCESS);

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
            return PR_TRUE; 
        }
    }
    
    PRInt32 index = -1;
    while (++index < PRInt32(mHostFiltersArray.Length())) {
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
                return PR_FALSE; 
        }
        else {
            PRUint32 host_len = host.Length();
            PRUint32 filter_host_len = hinfo->name.host_len;

            if (host_len >= filter_host_len) {
                
                
                
                const char *host_tail = host.get() + host_len - filter_host_len;
                if (!PL_strncasecmp(host_tail, hinfo->name.host, filter_host_len))
                    return PR_FALSE; 
            }
        }
    }
    return PR_TRUE;
}

static const char kProxyType_HTTP[]    = "http";
static const char kProxyType_PROXY[]   = "proxy";
static const char kProxyType_SOCKS[]   = "socks";
static const char kProxyType_SOCKS4[]  = "socks4";
static const char kProxyType_SOCKS5[]  = "socks5";
static const char kProxyType_DIRECT[]  = "direct";
static const char kProxyType_UNKNOWN[] = "unknown";

const char *
nsProtocolProxyService::ExtractProxyInfo(const char *start, nsProxyInfo **result)
{
    *result = nsnull;
    PRUint32 flags = 0;

    

    
    const char *end = start;
    while (*end && *end != ';') ++end;

    
    const char *sp = start;
    while (sp < end && *sp != ' ' && *sp != '\t') ++sp;

    PRUint32 len = sp - start;
    const char *type = nsnull;
    switch (len) {
    case 5:
        if (PL_strncasecmp(start, kProxyType_PROXY, 5) == 0)
            type = kProxyType_HTTP;
        else if (PL_strncasecmp(start, kProxyType_SOCKS, 5) == 0)
            type = kProxyType_SOCKS4; 
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
        const char *host = nsnull, *hostEnd = nsnull;
        PRInt32 port = -1;

        
        
        
        if (type == kProxyType_SOCKS)
            flags |= nsIProxyInfo::TRANSPARENT_PROXY_RESOLVES_HOST;

        
        start = sp;
        while ((*start == ' ' || *start == '\t') && start < end)
            start++;
        if (start < end) {
            host = start;
            hostEnd = strchr(host, ':');
            if (!hostEnd || hostEnd > end) {
                hostEnd = end;
                
                if (type == kProxyType_HTTP)
                    port = 80;
                else
                    port = 1080;
            }
            else
                port = atoi(hostEnd + 1);
        }
        nsProxyInfo *pi = new nsProxyInfo;
        if (pi) {
            pi->mType = type;
            pi->mFlags = flags;
            pi->mTimeout = mFailedProxyTimeout;
            
            if (host) {
                pi->mHost.Assign(host, hostEnd - host);
                pi->mPort = port;
            }
            NS_ADDREF(*result = pi);
        }
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

PRUint32
nsProtocolProxyService::SecondsSinceSessionStart()
{
    PRTime now = PR_Now();

    
    PRInt64 diff;
    LL_SUB(diff, now, mSessionStart);

    
    PRTime ups;
    LL_I2L(ups, PR_USEC_PER_SEC);
    LL_DIV(diff, diff, ups);

    
    PRUint32 dsec;
    LL_L2UI(dsec, diff);

    return dsec;
}

void
nsProtocolProxyService::EnableProxy(nsProxyInfo *pi)
{
    nsCAutoString key;
    GetProxyKey(pi, key);
    mFailedProxies.Remove(key);
}

void
nsProtocolProxyService::DisableProxy(nsProxyInfo *pi)
{
    nsCAutoString key;
    GetProxyKey(pi, key);

    PRUint32 dsec = SecondsSinceSessionStart();

    
    
    dsec += pi->mTimeout;

    
    
    
    
    
    
    

    LOG(("DisableProxy %s %d\n", key.get(), dsec));

    
    
    mFailedProxies.Put(key, dsec);
}

PRBool
nsProtocolProxyService::IsProxyDisabled(nsProxyInfo *pi)
{
    nsCAutoString key;
    GetProxyKey(pi, key);

    PRUint32 val;
    if (!mFailedProxies.Get(key, &val))
        return PR_FALSE;

    PRUint32 dsec = SecondsSinceSessionStart();

    
    if (dsec > val) {
        mFailedProxies.Remove(key);
        return PR_FALSE;
    }

    return PR_TRUE;
}

nsresult
nsProtocolProxyService::ConfigureFromPAC(const nsCString &spec,
                                         PRBool forceReload)
{
    if (!mPACMan) {
        mPACMan = new nsPACMan();
        if (!mPACMan)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    nsCOMPtr<nsIURI> pacURI;
    nsresult rv = NS_NewURI(getter_AddRefs(pacURI), spec);
    if (NS_FAILED(rv))
        return rv;

    if (mPACMan->IsPACURI(pacURI) && !forceReload)
        return NS_OK;

    mFailedProxies.Clear();

    return mPACMan->LoadPACFromURI(pacURI);
}

void
nsProtocolProxyService::ProcessPACString(const nsCString &pacString,
                                         nsIProxyInfo **result)
{
    if (pacString.IsEmpty()) {
        *result = nsnull;
        return;
    }

    const char *proxies = pacString.get();

    nsProxyInfo *pi = nsnull, *first = nsnull, *last = nsnull;
    while (*proxies) {
        proxies = ExtractProxyInfo(proxies, &pi);
        if (pi) {
            if (last) {
                NS_ASSERTION(last->mNext == nsnull, "leaking nsProxyInfo");
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

    PRInt32 type;
    nsresult rv = prefs->GetIntPref(PROXY_PREF("type"), &type);
    if (NS_FAILED(rv))
        return NS_OK;

    nsXPIDLCString pacSpec;
    if (type == eProxyConfig_PAC)
        prefs->GetCharPref(PROXY_PREF("autoconfig_url"), getter_Copies(pacSpec));
    else if (type == eProxyConfig_WPAD)
        pacSpec.AssignLiteral(WPAD_URL);

    if (!pacSpec.IsEmpty())
        ConfigureFromPAC(pacSpec, PR_TRUE);
    return NS_OK;
}


NS_IMETHODIMP
nsProtocolProxyService::Resolve(nsIURI *uri, PRUint32 flags,
                                nsIProxyInfo **result)
{
    nsProtocolInfo info;
    nsresult rv = GetProtocolInfo(uri, &info);
    if (NS_FAILED(rv))
        return rv;

    PRBool usePAC;
    rv = Resolve_Internal(uri, info, &usePAC, result);
    if (NS_FAILED(rv))
        return rv;

    if (usePAC && mPACMan) {
        NS_ASSERTION(*result == nsnull, "we should not have a result yet");

        
        if (flags & RESOLVE_NON_BLOCKING)
            return NS_BASE_STREAM_WOULD_BLOCK;

        
        nsCString pacString;
        rv = mPACMan->GetProxyForURI(uri, pacString);
        if (NS_SUCCEEDED(rv))
            ProcessPACString(pacString, result);
        else if (rv == NS_ERROR_IN_PROGRESS) {
            
            
            rv = NewProxyInfo_Internal(kProxyType_UNKNOWN, EmptyCString(), -1,
                                       0, 0, nsnull, result);
            if (NS_FAILED(rv))
                return rv;
        }
        else
            NS_WARNING("failed querying PAC file; trying DIRECT");
    }

    ApplyFilters(uri, info, result);
    return NS_OK;
}

NS_IMETHODIMP
nsProtocolProxyService::AsyncResolve(nsIURI *uri, PRUint32 flags,
                                     nsIProtocolProxyCallback *callback,
                                     nsICancelable **result)
{
    nsRefPtr<nsAsyncResolveRequest> ctx =
            new nsAsyncResolveRequest(this, uri, callback);
    if (!ctx)
        return NS_ERROR_OUT_OF_MEMORY;

    nsProtocolInfo info;
    nsresult rv = GetProtocolInfo(uri, &info);
    if (NS_FAILED(rv))
        return rv;

    PRBool usePAC;
    nsCOMPtr<nsIProxyInfo> pi;
    rv = Resolve_Internal(uri, info, &usePAC, getter_AddRefs(pi));
    if (NS_FAILED(rv))
        return rv;

    if (!usePAC || !mPACMan) {
        ApplyFilters(uri, info, pi);

        ctx->SetResult(NS_OK, pi);
        return ctx->DispatchCallback();
    }

    
    rv = mPACMan->AsyncGetProxyForURI(uri, ctx);
    if (NS_SUCCEEDED(rv)) {
        *result = ctx;
        NS_ADDREF(*result);
    }
    return rv;
}

NS_IMETHODIMP
nsProtocolProxyService::NewProxyInfo(const nsACString &aType,
                                     const nsACString &aHost,
                                     PRInt32 aPort,
                                     PRUint32 aFlags,
                                     PRUint32 aFailoverTimeout,
                                     nsIProxyInfo *aFailoverProxy,
                                     nsIProxyInfo **aResult)
{
    static const char *types[] = {
        kProxyType_HTTP,
        kProxyType_SOCKS,
        kProxyType_SOCKS4,
        kProxyType_DIRECT
    };

    
    
    const char *type = nsnull;
    for (PRUint32 i=0; i<NS_ARRAY_LENGTH(types); ++i) {
        if (aType.LowerCaseEqualsASCII(types[i])) {
            type = types[i];
            break;
        }
    }
    NS_ENSURE_TRUE(type, NS_ERROR_INVALID_ARG);

    if (aPort <= 0)
        aPort = -1;

    return NewProxyInfo_Internal(type, aHost, aPort, aFlags, aFailoverTimeout,
                                 aFailoverProxy, aResult);
}

NS_IMETHODIMP
nsProtocolProxyService::GetFailoverForProxy(nsIProxyInfo  *aProxy,
                                            nsIURI        *aURI,
                                            nsresult       aStatus,
                                            nsIProxyInfo **aResult)
{
    
    
    if (mProxyConfig != eProxyConfig_PAC && mProxyConfig != eProxyConfig_WPAD &&
        mProxyConfig != eProxyConfig_System)
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
                                       PRUint32 position)
{
    UnregisterFilter(filter);  

    FilterLink *link = new FilterLink(position, filter);
    if (!link)
        return NS_ERROR_OUT_OF_MEMORY;

    if (!mFilters) {
        mFilters = link;
        return NS_OK;
    }

    
    FilterLink *last = nsnull;
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

    FilterLink *last = nsnull;
    for (FilterLink *iter = mFilters; iter; iter = iter->next) {
        nsCOMPtr<nsISupports> object = do_QueryInterface(iter->filter);
        if (object == givenObject) {
            if (last)
                last->next = iter->next;
            else
                mFilters = iter->next;
            iter->next = nsnull;
            delete iter;
            return NS_OK;
        }
        last = iter;
    }

    
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

        HostInfo *hinfo = new HostInfo();
        if (!hinfo)
            return; 
        hinfo->port = portLocation ? atoi(portLocation + 1) : 0;

        
        const char *end = maskLocation ? maskLocation :
                          portLocation ? portLocation :
                          endhost;

        nsCAutoString str(starthost, end - starthost);

        PRNetAddr addr;
        if (PR_StringToNetAddr(str.get(), &addr) == PR_SUCCESS) {
            hinfo->is_ipaddr   = PR_TRUE;
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
            PRUint32 startIndex, endIndex;
            if (str.First() == '*')
                startIndex = 1; 
            else
                startIndex = 0;
            endIndex = (portLocation ? portLocation : endhost) - starthost;

            hinfo->is_ipaddr = PR_FALSE;
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
        hinfo = nsnull;
loser:
        if (hinfo)
            delete hinfo;
    }
}

nsresult
nsProtocolProxyService::GetProtocolInfo(nsIURI *uri, nsProtocolInfo *info)
{
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
                                              PRInt32 aPort,
                                              PRUint32 aFlags,
                                              PRUint32 aFailoverTimeout,
                                              nsIProxyInfo *aFailoverProxy,
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
    proxyInfo->mTimeout = aFailoverTimeout == PR_UINT32_MAX
        ? mFailedProxyTimeout : aFailoverTimeout;
    failover.swap(proxyInfo->mNext);

    NS_ADDREF(*aResult = proxyInfo);
    return NS_OK;
}

nsresult
nsProtocolProxyService::Resolve_Internal(nsIURI *uri,
                                         const nsProtocolInfo &info,
                                         PRBool *usePAC,
                                         nsIProxyInfo **result)
{
    NS_ENSURE_ARG_POINTER(uri);

    *usePAC = PR_FALSE;
    *result = nsnull;

    if (!(info.flags & nsIProtocolHandler::ALLOWS_PROXY))
        return NS_OK;  

    if (mSystemProxySettings) {
        nsCAutoString PACURI;
        if (NS_SUCCEEDED(mSystemProxySettings->GetPACURI(PACURI)) &&
            !PACURI.IsEmpty()) {
            
            
            nsresult rv = ConfigureFromPAC(PACURI, PR_FALSE);
            if (NS_FAILED(rv))
                return rv;
        } else {
            nsCAutoString proxy;
            nsresult rv = mSystemProxySettings->GetProxyForURI(uri, proxy);
            if (NS_SUCCEEDED(rv)) {
                ProcessPACString(proxy, result);
                return NS_OK;
            }
            
            return NS_OK;
        }
    }

    
    
    if (mProxyConfig == eProxyConfig_Direct ||
            (mProxyConfig == eProxyConfig_Manual &&
             !CanUseProxy(uri, info.defaultPort)))
        return NS_OK;

    
    if (mProxyConfig == eProxyConfig_PAC || mProxyConfig == eProxyConfig_WPAD ||
        mProxyConfig == eProxyConfig_System) {
        
        *usePAC = PR_TRUE;
        return NS_OK;
    }

    
    const char *type = nsnull;
    const nsACString *host = nsnull;
    PRInt32 port = -1;

    PRUint32 proxyFlags = 0;

    if (!mHTTPProxyHost.IsEmpty() && mHTTPProxyPort > 0 &&
        info.scheme.EqualsLiteral("http")) {
        host = &mHTTPProxyHost;
        type = kProxyType_HTTP;
        port = mHTTPProxyPort;
    }
    else if (!mHTTPSProxyHost.IsEmpty() && mHTTPSProxyPort > 0 &&
             info.scheme.EqualsLiteral("https")) {
        host = &mHTTPSProxyHost;
        type = kProxyType_HTTP;
        port = mHTTPSProxyPort;
    }
    else if (!mFTPProxyHost.IsEmpty() && mFTPProxyPort > 0 &&
             info.scheme.EqualsLiteral("ftp")) {
        host = &mFTPProxyHost;
        type = kProxyType_HTTP;
        port = mFTPProxyPort;
    }
    else if (!mGopherProxyHost.IsEmpty() && mGopherProxyPort > 0 &&
             info.scheme.EqualsLiteral("gopher")) {
        host = &mGopherProxyHost;
        type = kProxyType_HTTP;
        port = mGopherProxyPort;
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
        nsresult rv = NewProxyInfo_Internal(type, *host, port, proxyFlags,
                                            PR_UINT32_MAX, nsnull, result);
        if (NS_FAILED(rv))
            return rv;
    }

    return NS_OK;
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
    nsProxyInfo *head = nsnull;
    CallQueryInterface(*list, &head);
    if (!head) {
        NS_NOTREACHED("nsIProxyInfo must QI to nsProxyInfo");
        return;
    }
    NS_RELEASE(*list);

    
    
    
    
    
    

    
    if (!(info.flags & nsIProtocolHandler::ALLOWS_PROXY_HTTP)) {
        nsProxyInfo *last = nsnull, *iter = head; 
        while (iter) {
            if (iter->Type() == kProxyType_HTTP) {
                
                if (last)
                    last->mNext = iter->mNext;
                else
                    head = iter->mNext;
                nsProxyInfo *next = iter->mNext;
                iter->mNext = nsnull;
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

    
    
    
    
    PRBool allDisabled = PR_TRUE;

    nsProxyInfo *iter;
    for (iter = head; iter; iter = iter->mNext) {
        if (!IsProxyDisabled(iter)) {
            allDisabled = PR_FALSE;
            break;
        }
    }

    if (allDisabled)
        LOG(("All proxies are disabled, so trying all again"));
    else {
        
        nsProxyInfo *last = nsnull; 
        for (iter = head; iter; ) {
            if (IsProxyDisabled(iter)) {
                
                nsProxyInfo *reject = iter;

                iter = iter->mNext;
                if (last)
                    last->mNext = iter;
                else
                    head = iter;

                reject->mNext = nsnull;
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
