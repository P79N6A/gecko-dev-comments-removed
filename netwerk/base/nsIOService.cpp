





#include "mozilla/DebugOnly.h"

#include "nsIOService.h"
#include "nsIProtocolHandler.h"
#include "nsIFileProtocolHandler.h"
#include "nscore.h"
#include "nsIURI.h"
#include "prprf.h"
#include "nsIErrorService.h"
#include "netCore.h"
#include "nsIObserverService.h"
#include "nsIPrefService.h"
#include "nsXPCOM.h"
#include "nsIProxiedProtocolHandler.h"
#include "nsIProxyInfo.h"
#include "nsEscape.h"
#include "nsNetCID.h"
#include "nsCRT.h"
#include "nsSimpleNestedURI.h"
#include "nsNetUtil.h"
#include "nsTArray.h"
#include "nsIConsoleService.h"
#include "nsIUploadChannel2.h"
#include "nsXULAppAPI.h"
#include "nsIScriptSecurityManager.h"
#include "nsIProtocolProxyCallback.h"
#include "nsICancelable.h"
#include "nsINetworkLinkService.h"
#include "nsPISocketTransportService.h"
#include "nsAsyncRedirectVerifyHelper.h"
#include "nsURLHelper.h"
#include "nsPIDNSService.h"
#include "nsIProtocolProxyService2.h"
#include "MainThreadUtils.h"
#include "nsIWidget.h"
#include "nsThreadUtils.h"
#include "mozilla/LoadInfo.h"
#include "mozilla/net/NeckoCommon.h"
#include "mozilla/Telemetry.h"

#ifdef MOZ_WIDGET_GONK
#include "nsINetworkManager.h"
#endif

#if defined(XP_WIN)
#include "nsNativeConnectionHelper.h"
#endif

using namespace mozilla;
using mozilla::net::IsNeckoChild;

#define PORT_PREF_PREFIX           "network.security.ports."
#define PORT_PREF(x)               PORT_PREF_PREFIX x
#define AUTODIAL_PREF              "network.autodial-helper.enabled"
#define MANAGE_OFFLINE_STATUS_PREF "network.manage-offline-status"




#define NECKO_BUFFER_CACHE_COUNT_PREF "network.buffer.cache.count"
#define NECKO_BUFFER_CACHE_SIZE_PREF  "network.buffer.cache.size"
#define NETWORK_NOTIFY_CHANGED_PREF   "network.notify.changed"

#define MAX_RECURSION_COUNT 50

nsIOService* gIOService = nullptr;
static bool gHasWarnedUploadChannel2;







int16_t gBadPortList[] = { 
  1,    
  7,    
  9,    
  11,   
  13,   
  15,   
  17,   
  19,   
  20,   
  21,   
  22,   
  23,   
  25,   
  37,   
  42,   
  43,   
  53,   
  77,   
  79,   
  87,   
  95,   
  101,  
  102,  
  103,  
  104,  
  109,  
  110,  
  111,  
  113,  
  115,  
  117,  
  119,  
  123,  
  135,  
  139,  
  143,  
  179,  
  389,  
  465,  
  512,  
  513,  
  514,  
  515,  
  526,  
  530,  
  531,  
  532,  
  540,  
  556,  
  563,  
  587,  
  601,  
  636,  
  993,  
  995,  
  2049, 
  4045, 
  6000, 
  0,    
};

static const char kProfileChangeNetTeardownTopic[] = "profile-change-net-teardown";
static const char kProfileChangeNetRestoreTopic[] = "profile-change-net-restore";
static const char kProfileDoChange[] = "profile-do-change";
static const char kNetworkActiveChanged[] = "network-active-changed";


uint32_t   nsIOService::gDefaultSegmentSize = 4096;
uint32_t   nsIOService::gDefaultSegmentCount = 24;

bool nsIOService::sTelemetryEnabled = false;

NS_IMPL_ISUPPORTS(nsAppOfflineInfo, nsIAppOfflineInfo)



nsIOService::nsIOService()
    : mOffline(true)
    , mOfflineForProfileChange(false)
    , mManageOfflineStatus(false)
    , mSettingOffline(false)
    , mSetOfflineValue(false)
    , mShutdown(false)
    , mNetworkLinkServiceInitialized(false)
    , mChannelEventSinks(NS_CHANNEL_EVENT_SINK_CATEGORY)
    , mAutoDialEnabled(false)
    , mNetworkNotifyChanged(true)
    , mPreviousWifiState(-1)
{
}

nsresult
nsIOService::Init()
{
    nsresult rv;

    
    
    

    mDNSService = do_GetService(NS_DNSSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) {
        NS_WARNING("failed to get DNS service");
        return rv;
    }

    
    nsCOMPtr<nsIErrorService> errorService = do_GetService(NS_ERRORSERVICE_CONTRACTID);
    if (errorService) {
        errorService->RegisterErrorStringBundle(NS_ERROR_MODULE_NETWORK, NECKO_MSGS_URL);
    }
    else
        NS_WARNING("failed to get error service");
    
    
    for(int i=0; gBadPortList[i]; i++)
        mRestrictedPortList.AppendElement(gBadPortList[i]);

    
    nsCOMPtr<nsIPrefBranch> prefBranch;
    GetPrefBranch(getter_AddRefs(prefBranch));
    if (prefBranch) {
        prefBranch->AddObserver(PORT_PREF_PREFIX, this, true);
        prefBranch->AddObserver(AUTODIAL_PREF, this, true);
        prefBranch->AddObserver(MANAGE_OFFLINE_STATUS_PREF, this, true);
        prefBranch->AddObserver(NECKO_BUFFER_CACHE_COUNT_PREF, this, true);
        prefBranch->AddObserver(NECKO_BUFFER_CACHE_SIZE_PREF, this, true);
        prefBranch->AddObserver(NETWORK_NOTIFY_CHANGED_PREF, this, true);
        PrefsChanged(prefBranch);
    }
    
    
    nsCOMPtr<nsIObserverService> observerService =
        mozilla::services::GetObserverService();
    if (observerService) {
        observerService->AddObserver(this, kProfileChangeNetTeardownTopic, true);
        observerService->AddObserver(this, kProfileChangeNetRestoreTopic, true);
        observerService->AddObserver(this, kProfileDoChange, true);
        observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, true);
        observerService->AddObserver(this, NS_NETWORK_LINK_TOPIC, true);
        observerService->AddObserver(this, NS_WIDGET_WAKE_OBSERVER_TOPIC, true);
        observerService->AddObserver(this, kNetworkActiveChanged, true);
    }
    else
        NS_WARNING("failed to get observer service");

    Preferences::AddBoolVarCache(&sTelemetryEnabled, "toolkit.telemetry.enabled", false);

    gIOService = this;

    InitializeNetworkLinkService();

    return NS_OK;
}


nsIOService::~nsIOService()
{
    gIOService = nullptr;
}

nsresult
nsIOService::InitializeSocketTransportService()
{
    nsresult rv = NS_OK;

    if (!mSocketTransportService) {
        mSocketTransportService = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
        if (NS_FAILED(rv)) {
            NS_WARNING("failed to get socket transport service");
        }
    }

    if (mSocketTransportService) {
        rv = mSocketTransportService->Init();
        NS_ASSERTION(NS_SUCCEEDED(rv), "socket transport service init failed");
        mSocketTransportService->SetAutodialEnabled(mAutoDialEnabled);
        mSocketTransportService->SetOffline(false);
    }

    return rv;
}

nsresult
nsIOService::InitializeNetworkLinkService()
{
    nsresult rv = NS_OK;

    if (mNetworkLinkServiceInitialized)
        return rv;

    if (!NS_IsMainThread()) {
        NS_WARNING("Network link service should be created on main thread"); 
        return NS_ERROR_FAILURE; 
    }

    
    if (XRE_GetProcessType() == GeckoProcessType_Default)
    {
        mNetworkLinkService = do_GetService(NS_NETWORK_LINK_SERVICE_CONTRACTID, &rv);
    }

    if (mNetworkLinkService) {
        mNetworkLinkServiceInitialized = true;
    }
    else {
        
        
        mManageOfflineStatus = false;
    }

    if (mManageOfflineStatus)
        OnNetworkLinkEvent(NS_NETWORK_LINK_DATA_UNKNOWN);
    else
        SetOffline(false);
    
    return rv;
}

nsIOService*
nsIOService::GetInstance() {
    if (!gIOService) {
        gIOService = new nsIOService();
        if (!gIOService)
            return nullptr;
        NS_ADDREF(gIOService);

        nsresult rv = gIOService->Init();
        if (NS_FAILED(rv)) {
            NS_RELEASE(gIOService);
            return nullptr;
        }
        return gIOService;
    }
    NS_ADDREF(gIOService);
    return gIOService;
}

NS_IMPL_ISUPPORTS(nsIOService,
                  nsIIOService,
                  nsIIOService2,
                  nsINetUtil,
                  nsISpeculativeConnect,
                  nsIObserver,
                  nsISupportsWeakReference)



nsresult
nsIOService::AsyncOnChannelRedirect(nsIChannel* oldChan, nsIChannel* newChan,
                                    uint32_t flags,
                                    nsAsyncRedirectVerifyHelper *helper)
{
    nsCOMPtr<nsIChannelEventSink> sink =
        do_GetService(NS_GLOBAL_CHANNELEVENTSINK_CONTRACTID);
    if (sink) {
        nsresult rv = helper->DelegateOnChannelRedirect(sink, oldChan,
                                                        newChan, flags);
        if (NS_FAILED(rv))
            return rv;
    }

    
    nsCOMArray<nsIChannelEventSink> entries;
    mChannelEventSinks.GetEntries(entries);
    int32_t len = entries.Count();
    for (int32_t i = 0; i < len; ++i) {
        nsresult rv = helper->DelegateOnChannelRedirect(entries[i], oldChan,
                                                        newChan, flags);
        if (NS_FAILED(rv))
            return rv;
    }
    return NS_OK;
}

nsresult
nsIOService::CacheProtocolHandler(const char *scheme, nsIProtocolHandler *handler)
{
    for (unsigned int i=0; i<NS_N(gScheme); i++)
    {
        if (!nsCRT::strcasecmp(scheme, gScheme[i]))
        {
            nsresult rv;
            NS_ASSERTION(!mWeakHandler[i], "Protocol handler already cached");
            
            nsCOMPtr<nsISupportsWeakReference> factoryPtr = do_QueryInterface(handler, &rv);
            if (!factoryPtr)
            {
                
                
#ifdef DEBUG_dp
                printf("DEBUG: %s protcol handler doesn't support weak ref. Not cached.\n", scheme);
#endif 
                return NS_ERROR_FAILURE;
            }
            mWeakHandler[i] = do_GetWeakReference(handler);
            return NS_OK;
        }
    }
    return NS_ERROR_FAILURE;
}

nsresult
nsIOService::GetCachedProtocolHandler(const char *scheme, nsIProtocolHandler **result, uint32_t start, uint32_t end)
{
    uint32_t len = end - start - 1;
    for (unsigned int i=0; i<NS_N(gScheme); i++)
    {
        if (!mWeakHandler[i])
            continue;

        
        
        if (end ? (!nsCRT::strncasecmp(scheme + start, gScheme[i], len)
                   && gScheme[i][len] == '\0')
                : (!nsCRT::strcasecmp(scheme, gScheme[i])))
        {
            return CallQueryReferent(mWeakHandler[i].get(), result);
        }
    }
    return NS_ERROR_FAILURE;
}
 
NS_IMETHODIMP
nsIOService::GetProtocolHandler(const char* scheme, nsIProtocolHandler* *result)
{
    nsresult rv;

    NS_ENSURE_ARG_POINTER(scheme);
    
    
    

    rv = GetCachedProtocolHandler(scheme, result);
    if (NS_SUCCEEDED(rv))
        return rv;

    bool externalProtocol = false;
    nsCOMPtr<nsIPrefBranch> prefBranch;
    GetPrefBranch(getter_AddRefs(prefBranch));
    if (prefBranch) {
        nsAutoCString externalProtocolPref("network.protocol-handler.external.");
        externalProtocolPref += scheme;
        rv = prefBranch->GetBoolPref(externalProtocolPref.get(), &externalProtocol);
        if (NS_FAILED(rv)) {
            externalProtocol = false;
        }
    }

    if (!externalProtocol) {
        nsAutoCString contractID(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX);
        contractID += scheme;
        ToLowerCase(contractID);

        rv = CallGetService(contractID.get(), result);
        if (NS_SUCCEEDED(rv)) {
            CacheProtocolHandler(scheme, *result);
            return rv;
        }

#ifdef MOZ_ENABLE_GIO
        
        
        
        

        rv = CallGetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX"moz-gio",
                            result);
        if (NS_SUCCEEDED(rv)) {
            nsAutoCString spec(scheme);
            spec.Append(':');

            nsIURI *uri;
            rv = (*result)->NewURI(spec, nullptr, nullptr, &uri);
            if (NS_SUCCEEDED(rv)) {
                NS_RELEASE(uri);
                return rv;
            }

            NS_RELEASE(*result);
        }
#endif
    }

    
    
    
    

    rv = CallGetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX"default",
                        result);
    if (NS_FAILED(rv))
        return NS_ERROR_UNKNOWN_PROTOCOL;

    return rv;
}

NS_IMETHODIMP
nsIOService::ExtractScheme(const nsACString &inURI, nsACString &scheme)
{
    return net_ExtractURLScheme(inURI, nullptr, nullptr, &scheme);
}

NS_IMETHODIMP 
nsIOService::GetProtocolFlags(const char* scheme, uint32_t *flags)
{
    nsCOMPtr<nsIProtocolHandler> handler;
    nsresult rv = GetProtocolHandler(scheme, getter_AddRefs(handler));
    if (NS_FAILED(rv)) return rv;

    rv = handler->GetProtocolFlags(flags);
    return rv;
}

class AutoIncrement
{
    public:
        explicit AutoIncrement(uint32_t *var) : mVar(var)
        {
            ++*var;
        }
        ~AutoIncrement()
        {
            --*mVar;
        }
    private:
        uint32_t *mVar;
};

nsresult
nsIOService::NewURI(const nsACString &aSpec, const char *aCharset, nsIURI *aBaseURI, nsIURI **result)
{
    NS_ASSERTION(NS_IsMainThread(), "wrong thread");

    static uint32_t recursionCount = 0;
    if (recursionCount >= MAX_RECURSION_COUNT)
        return NS_ERROR_MALFORMED_URI;
    AutoIncrement inc(&recursionCount);

    nsAutoCString scheme;
    nsresult rv = ExtractScheme(aSpec, scheme);
    if (NS_FAILED(rv)) {
        
        if (!aBaseURI)
            return NS_ERROR_MALFORMED_URI;

        rv = aBaseURI->GetScheme(scheme);
        if (NS_FAILED(rv)) return rv;
    }

    
    nsCOMPtr<nsIProtocolHandler> handler;
    rv = GetProtocolHandler(scheme.get(), getter_AddRefs(handler));
    if (NS_FAILED(rv)) return rv;

    return handler->NewURI(aSpec, aCharset, aBaseURI, result);
}


NS_IMETHODIMP 
nsIOService::NewFileURI(nsIFile *file, nsIURI **result)
{
    nsresult rv;
    NS_ENSURE_ARG_POINTER(file);

    nsCOMPtr<nsIProtocolHandler> handler;

    rv = GetProtocolHandler("file", getter_AddRefs(handler));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIFileProtocolHandler> fileHandler( do_QueryInterface(handler, &rv) );
    if (NS_FAILED(rv)) return rv;
    
    return fileHandler->NewFileURI(file, result);
}

NS_IMETHODIMP
nsIOService::NewChannelFromURI2(nsIURI* aURI,
                                nsIDOMNode* aLoadingNode,
                                nsIPrincipal* aLoadingPrincipal,
                                nsIPrincipal* aTriggeringPrincipal,
                                uint32_t aSecurityFlags,
                                uint32_t aContentPolicyType,
                                nsIChannel** result)
{
    return NewChannelFromURIWithProxyFlags2(aURI,
                                            nullptr, 
                                            0,       
                                            aLoadingNode,
                                            aLoadingPrincipal,
                                            aTriggeringPrincipal,
                                            aSecurityFlags,
                                            aContentPolicyType,
                                            result);
}

NS_IMETHODIMP
nsIOService::NewChannelFromURIWithLoadInfo(nsIURI* aURI,
                                           nsILoadInfo* aLoadInfo,
                                           nsIChannel** result)
{
  NS_ENSURE_ARG_POINTER(aLoadInfo);
  return NewChannelFromURIWithProxyFlagsInternal(aURI,
                                                 nullptr, 
                                                 0,       
                                                 aLoadInfo,
                                                 result);
}











NS_IMETHODIMP
nsIOService::NewChannelFromURI(nsIURI *aURI, nsIChannel **result)
{
  NS_WARNING("Deprecated, use NewChannelFromURI2 providing loadInfo arguments!");
  return NewChannelFromURI2(aURI,
                            nullptr, 
                            nullptr, 
                            nullptr, 
                            nsILoadInfo::SEC_NORMAL,
                            nsIContentPolicy::TYPE_OTHER,
                            result);
}

nsresult
nsIOService::NewChannelFromURIWithProxyFlagsInternal(nsIURI* aURI,
                                                     nsIURI* aProxyURI,
                                                     uint32_t aProxyFlags,
                                                     nsILoadInfo* aLoadInfo,
                                                     nsIChannel** result)
{
    nsresult rv;
    NS_ENSURE_ARG_POINTER(aURI);

    nsAutoCString scheme;
    rv = aURI->GetScheme(scheme);
    if (NS_FAILED(rv))
        return rv;

    if (sTelemetryEnabled) {
        nsAutoCString path;
        aURI->GetPath(path);

        bool endsInExcl = StringEndsWith(path, NS_LITERAL_CSTRING("!"));
        int32_t bangSlashPos = path.Find("!/");

        bool hasBangSlash = bangSlashPos != kNotFound;
        bool hasBangDoubleSlash = false;

        if (bangSlashPos != kNotFound) {
            nsDependentCSubstring substr(path, bangSlashPos);
            hasBangDoubleSlash = StringBeginsWith(substr, NS_LITERAL_CSTRING("!//"));
        }

        Telemetry::Accumulate(Telemetry::URL_PATH_ENDS_IN_EXCLAMATION, endsInExcl);
        Telemetry::Accumulate(Telemetry::URL_PATH_CONTAINS_EXCLAMATION_SLASH,
                              hasBangSlash);
        Telemetry::Accumulate(Telemetry::URL_PATH_CONTAINS_EXCLAMATION_DOUBLE_SLASH,
                              hasBangDoubleSlash);
    }

    nsCOMPtr<nsIProtocolHandler> handler;
    rv = GetProtocolHandler(scheme.get(), getter_AddRefs(handler));
    if (NS_FAILED(rv))
        return rv;

    uint32_t protoFlags;
    rv = handler->GetProtocolFlags(&protoFlags);
    if (NS_FAILED(rv))
        return rv;

    
    
    
    
    
    

    bool newChannel2Succeeded = true;

    nsCOMPtr<nsIProxiedProtocolHandler> pph = do_QueryInterface(handler);
    if (pph) {
        rv = pph->NewProxiedChannel2(aURI, nullptr, aProxyFlags, aProxyURI,
                                     aLoadInfo, result);
        
        
        if (NS_FAILED(rv)) {
            newChannel2Succeeded = false;
            rv = pph->NewProxiedChannel(aURI, nullptr, aProxyFlags, aProxyURI,
                                        result);
        }
    }
    else {
        rv = handler->NewChannel2(aURI, aLoadInfo, result);
        
        
        if (NS_FAILED(rv)) {
            newChannel2Succeeded = false;
            rv = handler->NewChannel(aURI, result);
        }
    }
    NS_ENSURE_SUCCESS(rv, rv);

    if (aLoadInfo && newChannel2Succeeded) {
      
      
      
      
      
      nsCOMPtr<nsILoadInfo> loadInfo;
      (*result)->GetLoadInfo(getter_AddRefs(loadInfo));
      
      if (aLoadInfo != loadInfo) {
        MOZ_ASSERT(false, "newly created channel must have a loadinfo attached");
        return NS_ERROR_UNEXPECTED;
      }

      
      
      if (loadInfo->GetLoadingSandboxed()) {
        (*result)->SetOwner(nullptr);
      }
    }

    
    
    
    
    
    
    
    if (!gHasWarnedUploadChannel2 && scheme.EqualsLiteral("http")) {
        nsCOMPtr<nsIUploadChannel2> uploadChannel2 = do_QueryInterface(*result);
        if (!uploadChannel2) {
            nsCOMPtr<nsIConsoleService> consoleService =
                do_GetService(NS_CONSOLESERVICE_CONTRACTID);
            if (consoleService) {
                consoleService->LogStringMessage(NS_LITERAL_STRING(
                    "Http channel implementation doesn't support nsIUploadChannel2. An extension has supplied a non-functional http protocol handler. This will break behavior and in future releases not work at all."
                                                                   ).get());
            }
            gHasWarnedUploadChannel2 = true;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsIOService::NewChannelFromURIWithProxyFlags2(nsIURI* aURI,
                                              nsIURI* aProxyURI,
                                              uint32_t aProxyFlags,
                                              nsIDOMNode* aLoadingNode,
                                              nsIPrincipal* aLoadingPrincipal,
                                              nsIPrincipal* aTriggeringPrincipal,
                                              uint32_t aSecurityFlags,
                                              uint32_t aContentPolicyType,
                                              nsIChannel** result)
{
    
    
    
    
    
    
    
    
    
    
    nsCOMPtr<nsILoadInfo> loadInfo;

    if (aLoadingNode || aLoadingPrincipal) {
      nsCOMPtr<nsINode> loadingNode(do_QueryInterface(aLoadingNode));
      loadInfo = new mozilla::LoadInfo(aLoadingPrincipal,
                                       aTriggeringPrincipal,
                                       loadingNode,
                                       aSecurityFlags,
                                       aContentPolicyType);
      if (!loadInfo) {
        return NS_ERROR_UNEXPECTED;
      }
    }
    return NewChannelFromURIWithProxyFlagsInternal(aURI,
                                                   aProxyURI,
                                                   aProxyFlags,
                                                   loadInfo,
                                                   result);
}











NS_IMETHODIMP
nsIOService::NewChannelFromURIWithProxyFlags(nsIURI *aURI,
                                             nsIURI *aProxyURI,
                                             uint32_t aProxyFlags,
                                             nsIChannel **result)
{
  NS_WARNING("Deprecated, use NewChannelFromURIWithProxyFlags2 providing loadInfo arguments!");
  return NewChannelFromURIWithProxyFlags2(aURI,
                                          aProxyURI,
                                          aProxyFlags,
                                          nullptr, 
                                          nullptr, 
                                          nullptr, 
                                          nsILoadInfo::SEC_NORMAL,
                                          nsIContentPolicy::TYPE_OTHER,
                                          result);
}

NS_IMETHODIMP
nsIOService::NewChannel2(const nsACString& aSpec,
                         const char* aCharset,
                         nsIURI* aBaseURI,
                         nsIDOMNode* aLoadingNode,
                         nsIPrincipal* aLoadingPrincipal,
                         nsIPrincipal* aTriggeringPrincipal,
                         uint32_t aSecurityFlags,
                         uint32_t aContentPolicyType,
                         nsIChannel** result)
{
    nsresult rv;
    nsCOMPtr<nsIURI> uri;
    rv = NewURI(aSpec, aCharset, aBaseURI, getter_AddRefs(uri));
    if (NS_FAILED(rv)) return rv;

    return NewChannelFromURI2(uri,
                              aLoadingNode,
                              aLoadingPrincipal,
                              aTriggeringPrincipal,
                              aSecurityFlags,
                              aContentPolicyType,
                              result);
}











NS_IMETHODIMP
nsIOService::NewChannel(const nsACString &aSpec, const char *aCharset, nsIURI *aBaseURI, nsIChannel **result)
{
  NS_WARNING("Deprecated, use NewChannel2 providing loadInfo arguments!");
  return NewChannel2(aSpec,
                     aCharset,
                     aBaseURI,
                     nullptr, 
                     nullptr, 
                     nullptr, 
                     nsILoadInfo::SEC_NORMAL,
                     nsIContentPolicy::TYPE_OTHER,
                     result);
}

bool
nsIOService::IsLinkUp()
{
    InitializeNetworkLinkService();

    if (!mNetworkLinkService) {
        
        return true;
    }

    bool isLinkUp;
    nsresult rv;
    rv = mNetworkLinkService->GetIsLinkUp(&isLinkUp);
    if (NS_FAILED(rv)) {
        return true;
    }

    return isLinkUp;
}

NS_IMETHODIMP
nsIOService::GetOffline(bool *offline)
{
    *offline = mOffline;
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::SetOffline(bool offline)
{
    
    
    if ((mShutdown || mOfflineForProfileChange) && !offline)
        return NS_ERROR_NOT_AVAILABLE;

    
    
    
    
    mSetOfflineValue = offline;
    if (mSettingOffline) {
        return NS_OK;
    }

    mSettingOffline = true;

    nsCOMPtr<nsIObserverService> observerService =
        mozilla::services::GetObserverService();

    NS_ASSERTION(observerService, "The observer service should not be null");

    if (XRE_GetProcessType() == GeckoProcessType_Default) {
        if (observerService) {
            (void)observerService->NotifyObservers(nullptr,
                NS_IPC_IOSERVICE_SET_OFFLINE_TOPIC, offline ? 
                MOZ_UTF16("true") :
                MOZ_UTF16("false"));
        }
    }

    nsIIOService *subject = static_cast<nsIIOService *>(this);
    while (mSetOfflineValue != mOffline) {
        offline = mSetOfflineValue;

        if (offline && !mOffline) {
            NS_NAMED_LITERAL_STRING(offlineString, NS_IOSERVICE_OFFLINE);
            mOffline = true; 

            
            if (observerService)
                observerService->NotifyObservers(subject,
                                                 NS_IOSERVICE_GOING_OFFLINE_TOPIC,
                                                 offlineString.get());

            if (mDNSService)
                mDNSService->SetOffline(true);

            if (mSocketTransportService)
                mSocketTransportService->SetOffline(true);

            if (observerService)
                observerService->NotifyObservers(subject,
                                                 NS_IOSERVICE_OFFLINE_STATUS_TOPIC,
                                                 offlineString.get());
        }
        else if (!offline && mOffline) {
            
            if (mDNSService) {
                mDNSService->SetOffline(false);
                DebugOnly<nsresult> rv = mDNSService->Init();
                NS_ASSERTION(NS_SUCCEEDED(rv), "DNS service init failed");
            }
            InitializeSocketTransportService();
            mOffline = false;    
                                    

            
            if (mProxyService)
                mProxyService->ReloadPAC();

            
            if (observerService)
                observerService->NotifyObservers(subject,
                                                 NS_IOSERVICE_OFFLINE_STATUS_TOPIC,
                                                 NS_LITERAL_STRING(NS_IOSERVICE_ONLINE).get());
        }
    }

    
    if ((mShutdown || mOfflineForProfileChange) && mOffline) {
        
        
        if (mDNSService) {
            DebugOnly<nsresult> rv = mDNSService->Shutdown();
            NS_ASSERTION(NS_SUCCEEDED(rv), "DNS service shutdown failed");
        }
        if (mSocketTransportService) {
            DebugOnly<nsresult> rv = mSocketTransportService->Shutdown();
            NS_ASSERTION(NS_SUCCEEDED(rv), "socket transport service shutdown failed");
        }
    }

    mSettingOffline = false;

    return NS_OK;
}


NS_IMETHODIMP
nsIOService::AllowPort(int32_t inPort, const char *scheme, bool *_retval)
{
    int16_t port = inPort;
    if (port == -1) {
        *_retval = true;
        return NS_OK;
    }

    if (port == 0) {
        *_retval = false;
        return NS_OK;
    }
        
    
    int32_t badPortListCnt = mRestrictedPortList.Length();
    for (int i=0; i<badPortListCnt; i++)
    {
        if (port == mRestrictedPortList[i])
        {
            *_retval = false;

            
            if (!scheme)
                return NS_OK;
            
            nsCOMPtr<nsIProtocolHandler> handler;
            nsresult rv = GetProtocolHandler(scheme, getter_AddRefs(handler));
            if (NS_FAILED(rv)) return rv;

            
            return handler->AllowPort(port, scheme, _retval);
        }
    }

    *_retval = true;
    return NS_OK;
}



void
nsIOService::PrefsChanged(nsIPrefBranch *prefs, const char *pref)
{
    if (!prefs) return;

    
    if (!pref || strcmp(pref, PORT_PREF("banned")) == 0)
        ParsePortList(prefs, PORT_PREF("banned"), false);

    
    if (!pref || strcmp(pref, PORT_PREF("banned.override")) == 0)
        ParsePortList(prefs, PORT_PREF("banned.override"), true);

    if (!pref || strcmp(pref, AUTODIAL_PREF) == 0) {
        bool enableAutodial = false;
        nsresult rv = prefs->GetBoolPref(AUTODIAL_PREF, &enableAutodial);
        
        mAutoDialEnabled = enableAutodial;
        if (NS_SUCCEEDED(rv)) {
            if (mSocketTransportService)
                mSocketTransportService->SetAutodialEnabled(enableAutodial);
        }
    }

    if (!pref || strcmp(pref, MANAGE_OFFLINE_STATUS_PREF) == 0) {
        bool manage;
        if (mNetworkLinkServiceInitialized &&
            NS_SUCCEEDED(prefs->GetBoolPref(MANAGE_OFFLINE_STATUS_PREF,
                                            &manage)))
            SetManageOfflineStatus(manage);
    }

    if (!pref || strcmp(pref, NECKO_BUFFER_CACHE_COUNT_PREF) == 0) {
        int32_t count;
        if (NS_SUCCEEDED(prefs->GetIntPref(NECKO_BUFFER_CACHE_COUNT_PREF,
                                           &count)))
            
            if (count > 0)
                gDefaultSegmentCount = count;
    }
    
    if (!pref || strcmp(pref, NECKO_BUFFER_CACHE_SIZE_PREF) == 0) {
        int32_t size;
        if (NS_SUCCEEDED(prefs->GetIntPref(NECKO_BUFFER_CACHE_SIZE_PREF,
                                           &size)))
            




            if (size > 0 && size < 1024*1024)
                gDefaultSegmentSize = size;
        NS_WARN_IF_FALSE( (!(size & (size - 1))) , "network segment size is not a power of 2!");
    }

    if (!pref || strcmp(pref, NETWORK_NOTIFY_CHANGED_PREF) == 0) {
        bool allow;
        nsresult rv = prefs->GetBoolPref(NETWORK_NOTIFY_CHANGED_PREF, &allow);
        if (NS_SUCCEEDED(rv)) {
            mNetworkNotifyChanged = allow;
        }
    }
}

void
nsIOService::ParsePortList(nsIPrefBranch *prefBranch, const char *pref, bool remove)
{
    nsXPIDLCString portList;

    
    prefBranch->GetCharPref(pref, getter_Copies(portList));
    if (portList) {
        nsTArray<nsCString> portListArray;
        ParseString(portList, ',', portListArray);
        uint32_t index;
        for (index=0; index < portListArray.Length(); index++) {
            portListArray[index].StripWhitespace();
            int32_t portBegin, portEnd;

            if (PR_sscanf(portListArray[index].get(), "%d-%d", &portBegin, &portEnd) == 2) {
               if ((portBegin < 65536) && (portEnd < 65536)) {
                   int32_t curPort;
                   if (remove) {
                        for (curPort=portBegin; curPort <= portEnd; curPort++)
                            mRestrictedPortList.RemoveElement(curPort);
                   } else {
                        for (curPort=portBegin; curPort <= portEnd; curPort++)
                            mRestrictedPortList.AppendElement(curPort);
                   }
               }
            } else {
               nsresult aErrorCode;
               int32_t port = portListArray[index].ToInteger(&aErrorCode);
               if (NS_SUCCEEDED(aErrorCode) && port < 65536) {
                   if (remove)
                       mRestrictedPortList.RemoveElement(port);
                   else
                       mRestrictedPortList.AppendElement(port);
               }
            }

        }
    }
}

void
nsIOService::GetPrefBranch(nsIPrefBranch **result)
{
    *result = nullptr;
    CallGetService(NS_PREFSERVICE_CONTRACTID, result);
}



static bool
IsWifiActive()
{
    
    if (IsNeckoChild()) {
        return false;
    }
#ifdef MOZ_WIDGET_GONK
    
    nsCOMPtr<nsINetworkManager> networkManager =
        do_GetService("@mozilla.org/network/manager;1");
    if (!networkManager) {
        return false;
    }
    nsCOMPtr<nsINetworkInterface> active;
    networkManager->GetActive(getter_AddRefs(active));
    if (!active) {
        return false;
    }
    int32_t type;
    if (NS_FAILED(active->GetType(&type))) {
        return false;
    }
    switch (type) {
    case nsINetworkInterface::NETWORK_TYPE_WIFI:
    case nsINetworkInterface::NETWORK_TYPE_WIFI_P2P:
        return true;
    default:
        return false;
    }
#else
    
    
    return true;
#endif
}

struct EnumeratorParams {
    nsIOService *service;
    int32_t     status;
};

PLDHashOperator
nsIOService::EnumerateWifiAppsChangingState(const unsigned int &aKey,
                                            int32_t aValue,
                                            void *aUserArg)
{
    EnumeratorParams *params = reinterpret_cast<EnumeratorParams*>(aUserArg);
    if (aValue == nsIAppOfflineInfo::WIFI_ONLY) {
        params->service->NotifyAppOfflineStatus(aKey, params->status);
    }
    return PL_DHASH_NEXT;
}


NS_IMETHODIMP
nsIOService::Observe(nsISupports *subject,
                     const char *topic,
                     const char16_t *data)
{
    if (!strcmp(topic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
        nsCOMPtr<nsIPrefBranch> prefBranch = do_QueryInterface(subject);
        if (prefBranch)
            PrefsChanged(prefBranch, NS_ConvertUTF16toUTF8(data).get());
    } else if (!strcmp(topic, kProfileChangeNetTeardownTopic)) {
        if (!mOffline) {
            mOfflineForProfileChange = true;
            SetOffline(true);
        }
    } else if (!strcmp(topic, kProfileChangeNetRestoreTopic)) {
        if (mOfflineForProfileChange) {
            mOfflineForProfileChange = false;
            if (!mManageOfflineStatus ||
                NS_FAILED(OnNetworkLinkEvent(NS_NETWORK_LINK_DATA_UNKNOWN))) {
                SetOffline(false);
            }
        } 
    } else if (!strcmp(topic, kProfileDoChange)) { 
        if (data && NS_LITERAL_STRING("startup").Equals(data)) {
            
            InitializeNetworkLinkService();
            
            
            mNetworkLinkServiceInitialized = true;
            
            nsCOMPtr<nsIPrefBranch> prefBranch;
            GetPrefBranch(getter_AddRefs(prefBranch));
            PrefsChanged(prefBranch, MANAGE_OFFLINE_STATUS_PREF);
        }
    } else if (!strcmp(topic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
        
        
        
        mShutdown = true;

        SetOffline(true);

        
        mProxyService = nullptr;
    } else if (!strcmp(topic, NS_NETWORK_LINK_TOPIC)) {
        if (!mOfflineForProfileChange && mManageOfflineStatus) {
            OnNetworkLinkEvent(NS_ConvertUTF16toUTF8(data).get());
        }
    } else if (!strcmp(topic, NS_WIDGET_WAKE_OBSERVER_TOPIC)) {
        
        nsCOMPtr<nsIObserverService> observerService =
            mozilla::services::GetObserverService();

        NS_ASSERTION(observerService, "The observer service should not be null");

        if (observerService && mNetworkNotifyChanged) {
            (void)observerService->
                NotifyObservers(nullptr,
                                NS_NETWORK_LINK_TOPIC,
                                MOZ_UTF16(NS_NETWORK_LINK_DATA_CHANGED));
        }
    } else if (!strcmp(topic, kNetworkActiveChanged)) {
#ifdef MOZ_WIDGET_GONK
        if (IsNeckoChild()) {
          return NS_OK;
        }
        nsCOMPtr<nsINetworkInterface> interface = do_QueryInterface(subject);
        if (!interface) {
            return NS_ERROR_FAILURE;
        }
        int32_t state;
        if (NS_FAILED(interface->GetState(&state))) {
            return NS_ERROR_FAILURE;
        }

        bool wifiActive = IsWifiActive();
        int32_t newWifiState = wifiActive ?
            nsINetworkInterface::NETWORK_TYPE_WIFI :
            nsINetworkInterface::NETWORK_TYPE_MOBILE;
        if (mPreviousWifiState != newWifiState) {
            
            int32_t status = wifiActive ?
                nsIAppOfflineInfo::ONLINE : nsIAppOfflineInfo::OFFLINE;

            EnumeratorParams params = {this, status};
            mAppsOfflineStatus.EnumerateRead(EnumerateWifiAppsChangingState, &params);
        }

        mPreviousWifiState = newWifiState;
#endif
    }

    return NS_OK;
}


NS_IMETHODIMP
nsIOService::ParseContentType(const nsACString &aTypeHeader,
                              nsACString &aCharset,
                              bool *aHadCharset,
                              nsACString &aContentType)
{
    net_ParseContentType(aTypeHeader, aContentType, aCharset, aHadCharset);
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::ProtocolHasFlags(nsIURI   *uri,
                              uint32_t  flags,
                              bool     *result)
{
    NS_ENSURE_ARG(uri);

    *result = false;
    nsAutoCString scheme;
    nsresult rv = uri->GetScheme(scheme);
    NS_ENSURE_SUCCESS(rv, rv);
  
    uint32_t protocolFlags;
    rv = GetProtocolFlags(scheme.get(), &protocolFlags);

    if (NS_SUCCEEDED(rv)) {
        *result = (protocolFlags & flags) == flags;
    }
  
    return rv;
}

NS_IMETHODIMP
nsIOService::URIChainHasFlags(nsIURI   *uri,
                              uint32_t  flags,
                              bool     *result)
{
    nsresult rv = ProtocolHasFlags(uri, flags, result);
    NS_ENSURE_SUCCESS(rv, rv);

    if (*result) {
        return rv;
    }

    
    
    nsCOMPtr<nsINestedURI> nestedURI = do_QueryInterface(uri);
    while (nestedURI) {
        nsCOMPtr<nsIURI> innerURI;
        rv = nestedURI->GetInnerURI(getter_AddRefs(innerURI));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = ProtocolHasFlags(innerURI, flags, result);

        if (*result) {
            return rv;
        }

        nestedURI = do_QueryInterface(innerURI);
    }

    return rv;
}

NS_IMETHODIMP
nsIOService::ToImmutableURI(nsIURI* uri, nsIURI** result)
{
    if (!uri) {
        *result = nullptr;
        return NS_OK;
    }

    nsresult rv = NS_EnsureSafeToReturn(uri, result);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_TryToSetImmutable(*result);
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::NewSimpleNestedURI(nsIURI* aURI, nsIURI** aResult)
{
    NS_ENSURE_ARG(aURI);

    nsCOMPtr<nsIURI> safeURI;
    nsresult rv = NS_EnsureSafeToReturn(aURI, getter_AddRefs(safeURI));
    NS_ENSURE_SUCCESS(rv, rv);

    NS_IF_ADDREF(*aResult = new nsSimpleNestedURI(safeURI));
    return *aResult ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsIOService::SetManageOfflineStatus(bool aManage)
{
    nsresult rv = NS_OK;

    
    
    
    
    
    
    
    
    bool wasManaged = mManageOfflineStatus;
    mManageOfflineStatus = aManage;

    InitializeNetworkLinkService();

    if (mManageOfflineStatus && !wasManaged) {
        rv = OnNetworkLinkEvent(NS_NETWORK_LINK_DATA_UNKNOWN);
        if (NS_FAILED(rv))
            mManageOfflineStatus = false;
    }
    return rv;
}

NS_IMETHODIMP
nsIOService::GetManageOfflineStatus(bool* aManage) {
    *aManage = mManageOfflineStatus;
    return NS_OK;
}


nsresult
nsIOService::OnNetworkLinkEvent(const char *data)
{
    if (!mNetworkLinkService)
        return NS_ERROR_FAILURE;

    if (mShutdown)
        return NS_ERROR_NOT_AVAILABLE;

    if (!mManageOfflineStatus) {
      return NS_OK;
    }

    if (!strcmp(data, NS_NETWORK_LINK_DATA_DOWN)) {
        
        if (mSocketTransportService) {
            bool autodialEnabled = false;
            mSocketTransportService->GetAutodialEnabled(&autodialEnabled);
            
            
            
            if (autodialEnabled) {
#if defined(XP_WIN)
                
                
                
                if (nsNativeConnectionHelper::IsAutodialEnabled()) {
                    return SetOffline(false);
                }
#else
                return SetOffline(false);
#endif
            }
        }
    }

    bool isUp;
    if (!strcmp(data, NS_NETWORK_LINK_DATA_CHANGED)) {
        
        return NS_OK;
    } else if (!strcmp(data, NS_NETWORK_LINK_DATA_DOWN)) {
        isUp = false;
    } else if (!strcmp(data, NS_NETWORK_LINK_DATA_UP)) {
        isUp = true;
    } else if (!strcmp(data, NS_NETWORK_LINK_DATA_UNKNOWN)) {
        nsresult rv = mNetworkLinkService->GetIsLinkUp(&isUp);
        NS_ENSURE_SUCCESS(rv, rv);
    } else {
        NS_WARNING("Unhandled network event!");
        return NS_OK;
    }

    return SetOffline(!isUp);
}

NS_IMETHODIMP
nsIOService::EscapeString(const nsACString& aString,
                          uint32_t aEscapeType,
                          nsACString& aResult)
{
  NS_ENSURE_ARG_MAX(aEscapeType, 4);

  nsAutoCString stringCopy(aString);
  nsCString result;

  if (!NS_Escape(stringCopy, result, (nsEscapeMask) aEscapeType))
    return NS_ERROR_OUT_OF_MEMORY;

  aResult.Assign(result);

  return NS_OK;
}

NS_IMETHODIMP 
nsIOService::EscapeURL(const nsACString &aStr, 
                       uint32_t aFlags, nsACString &aResult)
{
  aResult.Truncate();
  NS_EscapeURL(aStr.BeginReading(), aStr.Length(), 
               aFlags | esc_AlwaysCopy, aResult);
  return NS_OK;
}

NS_IMETHODIMP 
nsIOService::UnescapeString(const nsACString &aStr, 
                            uint32_t aFlags, nsACString &aResult)
{
  aResult.Truncate();
  NS_UnescapeURL(aStr.BeginReading(), aStr.Length(), 
                 aFlags | esc_AlwaysCopy, aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsIOService::ExtractCharsetFromContentType(const nsACString &aTypeHeader,
                                           nsACString &aCharset,
                                           int32_t *aCharsetStart,
                                           int32_t *aCharsetEnd,
                                           bool *aHadCharset)
{
    nsAutoCString ignored;
    net_ParseContentType(aTypeHeader, ignored, aCharset, aHadCharset,
                         aCharsetStart, aCharsetEnd);
    if (*aHadCharset && *aCharsetStart == *aCharsetEnd) {
        *aHadCharset = false;
    }
    return NS_OK;
}


class IOServiceProxyCallback final : public nsIProtocolProxyCallback
{
    ~IOServiceProxyCallback() {}

public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLPROXYCALLBACK

    IOServiceProxyCallback(nsIInterfaceRequestor *aCallbacks,
                           nsIOService *aIOService)
        : mCallbacks(aCallbacks)
        , mIOService(aIOService)
    { }

private:
    nsRefPtr<nsIInterfaceRequestor> mCallbacks;
    nsRefPtr<nsIOService>           mIOService;
};

NS_IMPL_ISUPPORTS(IOServiceProxyCallback, nsIProtocolProxyCallback)

NS_IMETHODIMP
IOServiceProxyCallback::OnProxyAvailable(nsICancelable *request, nsIChannel *channel,
                                         nsIProxyInfo *pi, nsresult status)
{
    
    nsAutoCString type;
    if (NS_SUCCEEDED(status) && pi &&
        NS_SUCCEEDED(pi->GetType(type)) &&
        !type.EqualsLiteral("direct")) {
        
        return NS_OK;
    }

    nsCOMPtr<nsIURI> uri;
    nsresult rv = channel->GetURI(getter_AddRefs(uri));
    if (NS_FAILED(rv)) {
        return NS_OK;
    }

    nsAutoCString scheme;
    rv = uri->GetScheme(scheme);
    if (NS_FAILED(rv))
        return NS_OK;

    nsCOMPtr<nsIProtocolHandler> handler;
    rv = mIOService->GetProtocolHandler(scheme.get(),
                                        getter_AddRefs(handler));
    if (NS_FAILED(rv))
        return NS_OK;

    nsCOMPtr<nsISpeculativeConnect> speculativeHandler =
        do_QueryInterface(handler);
    if (!speculativeHandler)
        return NS_OK;

    speculativeHandler->SpeculativeConnect(uri,
                                           mCallbacks);
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::SpeculativeConnect(nsIURI *aURI,
                                nsIInterfaceRequestor *aCallbacks)
{
    
    
    
    nsresult rv;
    nsCOMPtr<nsIProtocolProxyService> pps =
            do_GetService(NS_PROTOCOLPROXYSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIScriptSecurityManager> secMan(
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIPrincipal> systemPrincipal;
    rv = secMan->GetSystemPrincipal(getter_AddRefs(systemPrincipal));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    
    
    nsCOMPtr<nsIChannel> channel;
    rv = NewChannelFromURI2(aURI,
                            nullptr, 
                            systemPrincipal,
                            nullptr, 
                            nsILoadInfo::SEC_NORMAL,
                            nsIContentPolicy::TYPE_OTHER,
                            getter_AddRefs(channel));

    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsICancelable> cancelable;
    nsRefPtr<IOServiceProxyCallback> callback =
        new IOServiceProxyCallback(aCallbacks, this);
    nsCOMPtr<nsIProtocolProxyService2> pps2 = do_QueryInterface(pps);
    if (pps2) {
        return pps2->AsyncResolve2(channel, 0, callback, getter_AddRefs(cancelable));
    }
    return pps->AsyncResolve(channel, 0, callback, getter_AddRefs(cancelable));
}

void
nsIOService::NotifyAppOfflineStatus(uint32_t appId, int32_t state)
{
    MOZ_RELEASE_ASSERT(NS_IsMainThread(),
            "Should be called on the main thread");

    nsCOMPtr<nsIObserverService> observerService =
        mozilla::services::GetObserverService();
    MOZ_ASSERT(observerService, "The observer service should not be null");

    if (observerService) {
        nsRefPtr<nsAppOfflineInfo> info = new nsAppOfflineInfo(appId, state);
        observerService->NotifyObservers(
            info,
            NS_IOSERVICE_APP_OFFLINE_STATUS_TOPIC,
            MOZ_UTF16("all data in nsIAppOfflineInfo subject argument"));
    }
}

namespace {

class SetAppOfflineMainThread : public nsRunnable
{
public:
    SetAppOfflineMainThread(uint32_t aAppId, int32_t aState)
        : mAppId(aAppId)
        , mState(aState)
    {
    }

    NS_IMETHOD Run()
    {
        MOZ_ASSERT(NS_IsMainThread());
        gIOService->SetAppOfflineInternal(mAppId, mState);
        return NS_OK;
    }
private:
    uint32_t mAppId;
    int32_t mState;
};

}

NS_IMETHODIMP
nsIOService::SetAppOffline(uint32_t aAppId, int32_t aState)
{
    NS_ENSURE_TRUE(!IsNeckoChild(),
                   NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(aAppId != nsIScriptSecurityManager::NO_APP_ID,
                   NS_ERROR_INVALID_ARG);
    NS_ENSURE_TRUE(aAppId != nsIScriptSecurityManager::UNKNOWN_APP_ID,
                   NS_ERROR_INVALID_ARG);

    if (!NS_IsMainThread()) {
        NS_DispatchToMainThread(new SetAppOfflineMainThread(aAppId, aState));
        return NS_OK;
    }

    SetAppOfflineInternal(aAppId, aState);

    return NS_OK;
}








void
nsIOService::SetAppOfflineInternal(uint32_t aAppId, int32_t aState)
{
    MOZ_ASSERT(NS_IsMainThread());
    NS_ENSURE_TRUE_VOID(NS_IsMainThread());

    int32_t state = nsIAppOfflineInfo::ONLINE;
    mAppsOfflineStatus.Get(aAppId, &state);
    if (state == aState) {
        
        return;
    }

    
    
    
    bool wifiActive = IsWifiActive();
    bool offline = (state == nsIAppOfflineInfo::OFFLINE) ||
                   (state == nsIAppOfflineInfo::WIFI_ONLY && !wifiActive);

    switch (aState) {
    case nsIAppOfflineInfo::OFFLINE:
        mAppsOfflineStatus.Put(aAppId, nsIAppOfflineInfo::OFFLINE);
        if (!offline) {
            NotifyAppOfflineStatus(aAppId, nsIAppOfflineInfo::OFFLINE);
        }
        break;
    case nsIAppOfflineInfo::WIFI_ONLY:
        MOZ_RELEASE_ASSERT(!IsNeckoChild());
        mAppsOfflineStatus.Put(aAppId, nsIAppOfflineInfo::WIFI_ONLY);
        if (offline && wifiActive) {
            NotifyAppOfflineStatus(aAppId, nsIAppOfflineInfo::ONLINE);
        } else if (!offline && !wifiActive) {
            NotifyAppOfflineStatus(aAppId, nsIAppOfflineInfo::OFFLINE);
        }
        break;
    case nsIAppOfflineInfo::ONLINE:
        mAppsOfflineStatus.Remove(aAppId);
        if (offline) {
            NotifyAppOfflineStatus(aAppId, nsIAppOfflineInfo::ONLINE);
        }
        break;
    default:
        break;
    }

}

NS_IMETHODIMP
nsIOService::GetAppOfflineState(uint32_t aAppId, int32_t *aResult)
{
    NS_ENSURE_ARG(aResult);

    if (aAppId == NECKO_NO_APP_ID ||
        aAppId == NECKO_UNKNOWN_APP_ID) {
        return NS_ERROR_NOT_AVAILABLE;
    }

    *aResult = nsIAppOfflineInfo::ONLINE;
    mAppsOfflineStatus.Get(aAppId, aResult);

    return NS_OK;
}

NS_IMETHODIMP
nsIOService::IsAppOffline(uint32_t aAppId, bool* aResult)
{
    NS_ENSURE_ARG(aResult);
    *aResult = false;

    if (aAppId == NECKO_NO_APP_ID ||
        aAppId == NECKO_UNKNOWN_APP_ID) {
        return NS_ERROR_NOT_AVAILABLE;
    }

    int32_t state;
    if (mAppsOfflineStatus.Get(aAppId, &state)) {
        switch (state) {
        case nsIAppOfflineInfo::OFFLINE:
            *aResult = true;
            break;
        case nsIAppOfflineInfo::WIFI_ONLY:
            MOZ_RELEASE_ASSERT(!IsNeckoChild());
            *aResult = !IsWifiActive();
            break;
        default:
            
            break;
        }
    }

    return NS_OK;
}
