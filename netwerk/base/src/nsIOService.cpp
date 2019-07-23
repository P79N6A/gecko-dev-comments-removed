






































#include "nsIOService.h"
#include "nsIProtocolHandler.h"
#include "nsIFileProtocolHandler.h"
#include "nscore.h"
#include "nsIServiceManager.h"
#include "nsIURI.h"
#include "nsIStreamListener.h"
#include "prprf.h"
#include "prlog.h"
#include "nsLoadGroup.h"
#include "nsInputStreamChannel.h"
#include "nsXPIDLString.h" 
#include "nsReadableUtils.h"
#include "nsIErrorService.h" 
#include "netCore.h"
#include "nsIObserverService.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"
#include "nsIPrefLocalizedString.h"
#include "nsICategoryManager.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIProxiedProtocolHandler.h"
#include "nsIProxyInfo.h"
#include "nsITimelineService.h"
#include "nsEscape.h"
#include "nsNetCID.h"
#include "nsIRecyclingAllocator.h"
#include "nsISocketTransport.h"
#include "nsCRT.h"
#include "nsSimpleNestedURI.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"
#include "nsIPermissionManager.h"
#include "nsTArray.h"

#if defined(XP_WIN)
#include "nsNativeConnectionHelper.h"
#endif

#define PORT_PREF_PREFIX           "network.security.ports."
#define PORT_PREF(x)               PORT_PREF_PREFIX x
#define AUTODIAL_PREF              "network.autodial-helper.enabled"
#define MANAGE_OFFLINE_STATUS_PREF "network.manage-offline-status"

#define MAX_RECURSION_COUNT 50

nsIOService* gIOService = nsnull;







PRInt16 gBadPortList[] = { 
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


nsIMemory* nsIOService::gBufferCache = nsnull;



nsIOService::nsIOService()
    : mOffline(PR_FALSE)
    , mOfflineForProfileChange(PR_FALSE)
    , mManageOfflineStatus(PR_TRUE)
    , mSettingOffline(PR_FALSE)
    , mSetOfflineValue(PR_FALSE)
    , mShutdown(PR_FALSE)
    , mChannelEventSinks(NS_CHANNEL_EVENT_SINK_CATEGORY)
    , mContentSniffers(NS_CONTENT_SNIFFER_CATEGORY)
{
    
    if (!gBufferCache)
    {
        nsresult rv = NS_OK;
        nsCOMPtr<nsIRecyclingAllocator> recyclingAllocator =
            do_CreateInstance(NS_RECYCLINGALLOCATOR_CONTRACTID, &rv);
        if (NS_FAILED(rv))
            return;
        rv = recyclingAllocator->Init(NS_NECKO_BUFFER_CACHE_COUNT,
                                      NS_NECKO_15_MINS, "necko");
        if (NS_FAILED(rv))
            return;

        nsCOMPtr<nsIMemory> eyeMemory = do_QueryInterface(recyclingAllocator);
        gBufferCache = eyeMemory.get();
        NS_IF_ADDREF(gBufferCache);
    }
}

nsresult
nsIOService::Init()
{
    nsresult rv;
    
    
    
    

    

    mSocketTransportService = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) {
        NS_WARNING("failed to get socket transport service");
        return rv;
    }

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

    
    nsCOMPtr<nsIPrefBranch2> prefBranch;
    GetPrefBranch(getter_AddRefs(prefBranch));
    if (prefBranch) {
        prefBranch->AddObserver(PORT_PREF_PREFIX, this, PR_TRUE);
        prefBranch->AddObserver(AUTODIAL_PREF, this, PR_TRUE);
        prefBranch->AddObserver(MANAGE_OFFLINE_STATUS_PREF, this, PR_TRUE);
        PrefsChanged(prefBranch);
    }
    
    
    nsCOMPtr<nsIObserverService> observerService =
        do_GetService("@mozilla.org/observer-service;1");
    if (observerService) {
        observerService->AddObserver(this, kProfileChangeNetTeardownTopic, PR_TRUE);
        observerService->AddObserver(this, kProfileChangeNetRestoreTopic, PR_TRUE);
        observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_TRUE);
        observerService->AddObserver(this, NS_NETWORK_LINK_TOPIC, PR_TRUE);
    }
    else
        NS_WARNING("failed to get observer service");
        
    gIOService = this;
    
    
    mNetworkLinkService = do_GetService(NS_NETWORK_LINK_SERVICE_CONTRACTID);
    if (!mNetworkLinkService)
        mManageOfflineStatus = PR_FALSE;

    if (mManageOfflineStatus)
        TrackNetworkLinkStatusForOffline();

    return NS_OK;
}


nsIOService::~nsIOService()
{
    gIOService = nsnull;
}   

nsIOService*
nsIOService::GetInstance() {
    if (!gIOService) {
        gIOService = new nsIOService();
        if (!gIOService)
            return nsnull;
        NS_ADDREF(gIOService);

        nsresult rv = gIOService->Init();
        if (NS_FAILED(rv)) {
            NS_RELEASE(gIOService);
            return nsnull;
        }
        return gIOService;
    }
    NS_ADDREF(gIOService);
    return gIOService;
}

NS_IMPL_THREADSAFE_ISUPPORTS5(nsIOService,
                              nsIIOService,
                              nsIIOService2,
                              nsINetUtil,
                              nsIObserver,
                              nsISupportsWeakReference)



nsresult
nsIOService::OnChannelRedirect(nsIChannel* oldChan, nsIChannel* newChan,
                               PRUint32 flags)
{
    nsCOMPtr<nsIChannelEventSink> sink =
        do_GetService(NS_GLOBAL_CHANNELEVENTSINK_CONTRACTID);
    if (sink) {
        nsresult rv = sink->OnChannelRedirect(oldChan, newChan, flags);
        if (NS_FAILED(rv))
            return rv;
    }

    
    const nsCOMArray<nsIChannelEventSink>& entries =
        mChannelEventSinks.GetEntries();
    PRInt32 len = entries.Count();
    for (PRInt32 i = 0; i < len; ++i) {
        nsresult rv = entries[i]->OnChannelRedirect(oldChan, newChan, flags);
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
nsIOService::GetCachedProtocolHandler(const char *scheme, nsIProtocolHandler **result, PRUint32 start, PRUint32 end)
{
    PRUint32 len = end - start - 1;
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

    PRBool externalProtocol = PR_FALSE;
    PRBool listedProtocol   = PR_TRUE;
    nsCOMPtr<nsIPrefBranch2> prefBranch;
    GetPrefBranch(getter_AddRefs(prefBranch));
    if (prefBranch) {
        nsCAutoString externalProtocolPref("network.protocol-handler.external.");
        externalProtocolPref += scheme;
        rv = prefBranch->GetBoolPref(externalProtocolPref.get(), &externalProtocol);
        if (NS_FAILED(rv)) {
            externalProtocol = PR_FALSE;
            listedProtocol   = PR_FALSE;
        }
    }

    if (!externalProtocol) {
        nsCAutoString contractID(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX);
        contractID += scheme;
        ToLowerCase(contractID);

        rv = CallGetService(contractID.get(), result);
        if (NS_SUCCEEDED(rv)) {
            CacheProtocolHandler(scheme, *result);
            return rv;
        }

#ifdef MOZ_X11
        
        
        
        

        
        

        rv = CallGetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX"moz-gnomevfs",
                            result);
        if (NS_SUCCEEDED(rv)) {
            nsCAutoString spec(scheme);
            spec.Append(':');

            nsIURI *uri;
            rv = (*result)->NewURI(spec, nsnull, nsnull, &uri);
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
    return net_ExtractURLScheme(inURI, nsnull, nsnull, &scheme);
}

NS_IMETHODIMP 
nsIOService::GetProtocolFlags(const char* scheme, PRUint32 *flags)
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
        AutoIncrement(PRUint32 *var) : mVar(var)
        {
            ++*var;
        }
        ~AutoIncrement()
        {
            --*mVar;
        }
    private:
        PRUint32 *mVar;
};

nsresult
nsIOService::NewURI(const nsACString &aSpec, const char *aCharset, nsIURI *aBaseURI, nsIURI **result)
{
    NS_ASSERTION(NS_IsMainThread(), "wrong thread");

    static PRUint32 recursionCount = 0;
    if (recursionCount >= MAX_RECURSION_COUNT)
        return NS_ERROR_MALFORMED_URI;
    AutoIncrement inc(&recursionCount);

    nsCAutoString scheme;
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
nsIOService::NewChannelFromURI(nsIURI *aURI, nsIChannel **result)
{
    nsresult rv;
    NS_ENSURE_ARG_POINTER(aURI);
    NS_TIMELINE_MARK_URI("nsIOService::NewChannelFromURI(%s)", aURI);

    nsCAutoString scheme;
    rv = aURI->GetScheme(scheme);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIProtocolHandler> handler;
    rv = GetProtocolHandler(scheme.get(), getter_AddRefs(handler));
    if (NS_FAILED(rv))
        return rv;

    PRUint32 protoFlags;
    rv = handler->GetProtocolFlags(&protoFlags);
    if (NS_FAILED(rv))
        return rv;

    
    
    if (protoFlags & nsIProtocolHandler::ALLOWS_PROXY) {
        nsCOMPtr<nsIProxyInfo> pi;
        if (!mProxyService) {
            mProxyService = do_GetService(NS_PROTOCOLPROXYSERVICE_CONTRACTID);
            if (!mProxyService)
                NS_WARNING("failed to get protocol proxy service");
        }
        if (mProxyService) {
            rv = mProxyService->Resolve(aURI, 0, getter_AddRefs(pi));
            if (NS_FAILED(rv))
                pi = nsnull;
        }
        if (pi) {
            nsCAutoString type;
            if (NS_SUCCEEDED(pi->GetType(type)) && type.EqualsLiteral("http")) {
                
                rv = GetProtocolHandler("http", getter_AddRefs(handler));
                if (NS_FAILED(rv))
                    return rv;
            }
            nsCOMPtr<nsIProxiedProtocolHandler> pph = do_QueryInterface(handler);
            if (pph)
                return pph->NewProxiedChannel(aURI, pi, result);
        }
    }

    return handler->NewChannel(aURI, result);
}

NS_IMETHODIMP
nsIOService::NewChannel(const nsACString &aSpec, const char *aCharset, nsIURI *aBaseURI, nsIChannel **result)
{
    nsresult rv;
    nsCOMPtr<nsIURI> uri;
    rv = NewURI(aSpec, aCharset, aBaseURI, getter_AddRefs(uri));
    if (NS_FAILED(rv)) return rv;

    return NewChannelFromURI(uri, result);
}

PRBool
nsIOService::IsLinkUp()
{
    if (!mNetworkLinkService) {
        
        return PR_TRUE;
    }

    PRBool isLinkUp;
    nsresult rv;
    rv = mNetworkLinkService->GetIsLinkUp(&isLinkUp);
    if (NS_FAILED(rv)) {
        return PR_TRUE;
    }

    return isLinkUp;
}

NS_IMETHODIMP
nsIOService::GetOffline(PRBool *offline)
{
    *offline = mOffline;
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::SetOffline(PRBool offline)
{
    
    
    if (mShutdown && !offline)
        return NS_ERROR_NOT_AVAILABLE;

    
    
    
    
    mSetOfflineValue = offline;
    if (mSettingOffline) {
        return NS_OK;
    }
    mSettingOffline = PR_TRUE;

    nsCOMPtr<nsIObserverService> observerService =
        do_GetService("@mozilla.org/observer-service;1");

    while (mSetOfflineValue != mOffline) {
        offline = mSetOfflineValue;

        nsresult rv;
        if (offline && !mOffline) {
            NS_NAMED_LITERAL_STRING(offlineString, NS_IOSERVICE_OFFLINE);
            mOffline = PR_TRUE; 

            
            
            if (observerService)
                observerService->NotifyObservers(static_cast<nsIIOService *>(this),
                                                 NS_IOSERVICE_GOING_OFFLINE_TOPIC,
                                                 offlineString.get());

            
            
            if (mDNSService) {
                rv = mDNSService->Shutdown();
                NS_ASSERTION(NS_SUCCEEDED(rv), "DNS service shutdown failed");
            }
            if (mSocketTransportService) {
                rv = mSocketTransportService->Shutdown();
                NS_ASSERTION(NS_SUCCEEDED(rv), "socket transport service shutdown failed");
            }

            
            if (observerService)
                observerService->NotifyObservers(static_cast<nsIIOService *>(this),
                                                 NS_IOSERVICE_OFFLINE_STATUS_TOPIC,
                                                 offlineString.get());
        }
        else if (!offline && mOffline) {
            
            if (mDNSService) {
                rv = mDNSService->Init();
                NS_ASSERTION(NS_SUCCEEDED(rv), "DNS service init failed");
            }
            if (mSocketTransportService) {
                rv = mSocketTransportService->Init();
                NS_ASSERTION(NS_SUCCEEDED(rv), "socket transport service init failed");
            }
            mOffline = PR_FALSE;    
                                    

            
            if (mProxyService)
                mProxyService->ReloadPAC();

            
            if (observerService)
                observerService->NotifyObservers(static_cast<nsIIOService *>(this),
                                                 NS_IOSERVICE_OFFLINE_STATUS_TOPIC,
                                                 NS_LITERAL_STRING(NS_IOSERVICE_ONLINE).get());
        }
    }

    mSettingOffline = PR_FALSE;

    return NS_OK;
}


NS_IMETHODIMP
nsIOService::AllowPort(PRInt32 inPort, const char *scheme, PRBool *_retval)
{
    PRInt16 port = inPort;
    if (port == -1) {
        *_retval = PR_TRUE;
        return NS_OK;
    }
        
    
    PRInt32 badPortListCnt = mRestrictedPortList.Length();
    for (int i=0; i<badPortListCnt; i++)
    {
        if (port == mRestrictedPortList[i])
        {
            *_retval = PR_FALSE;

            
            if (!scheme)
                return NS_OK;
            
            nsCOMPtr<nsIProtocolHandler> handler;
            nsresult rv = GetProtocolHandler(scheme, getter_AddRefs(handler));
            if (NS_FAILED(rv)) return rv;

            
            return handler->AllowPort(port, scheme, _retval);
        }
    }

    *_retval = PR_TRUE;
    return NS_OK;
}



void
nsIOService::PrefsChanged(nsIPrefBranch *prefs, const char *pref)
{
    if (!prefs) return;

    
    if (!pref || strcmp(pref, PORT_PREF("banned")) == 0)
        ParsePortList(prefs, PORT_PREF("banned"), PR_FALSE);

    
    if (!pref || strcmp(pref, PORT_PREF("banned.override")) == 0)
        ParsePortList(prefs, PORT_PREF("banned.override"), PR_TRUE);

    if (!pref || strcmp(pref, AUTODIAL_PREF) == 0) {
        PRBool enableAutodial = PR_FALSE;
        nsresult rv = prefs->GetBoolPref(AUTODIAL_PREF, &enableAutodial);
        
        if (NS_SUCCEEDED(rv)) {
            if (mSocketTransportService)
                mSocketTransportService->SetAutodialEnabled(enableAutodial);
        }
    }

    if (!pref || strcmp(pref, MANAGE_OFFLINE_STATUS_PREF) == 0) {
        PRBool manage;
        if (NS_SUCCEEDED(prefs->GetBoolPref(MANAGE_OFFLINE_STATUS_PREF,
                                            &manage)))
            SetManageOfflineStatus(manage);
    }
}

void
nsIOService::ParsePortList(nsIPrefBranch *prefBranch, const char *pref, PRBool remove)
{
    nsXPIDLCString portList;

    
    prefBranch->GetCharPref(pref, getter_Copies(portList));
    if (portList) {
        nsTArray<nsCString> portListArray;
        ParseString(portList, ',', portListArray);
        PRUint32 index;
        for (index=0; index < portListArray.Length(); index++) {
            portListArray[index].StripWhitespace();
            PRInt32 aErrorCode, portBegin, portEnd;

            if (PR_sscanf(portListArray[index].get(), "%d-%d", &portBegin, &portEnd) == 2) {
               if ((portBegin < 65536) && (portEnd < 65536)) {
                   PRInt32 curPort;
                   if (remove) {
                        for (curPort=portBegin; curPort <= portEnd; curPort++)
                            mRestrictedPortList.RemoveElement(curPort);
                   } else {
                        for (curPort=portBegin; curPort <= portEnd; curPort++)
                            mRestrictedPortList.AppendElement(curPort);
                   }
               }
            } else {
               PRInt32 port = portListArray[index].ToInteger(&aErrorCode);
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
nsIOService::GetPrefBranch(nsIPrefBranch2 **result)
{
    *result = nsnull;
    CallGetService(NS_PREFSERVICE_CONTRACTID, result);
}


NS_IMETHODIMP
nsIOService::Observe(nsISupports *subject,
                     const char *topic,
                     const PRUnichar *data)
{
    if (!strcmp(topic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
        nsCOMPtr<nsIPrefBranch> prefBranch = do_QueryInterface(subject);
        if (prefBranch)
            PrefsChanged(prefBranch, NS_ConvertUTF16toUTF8(data).get());
    }
    else if (!strcmp(topic, kProfileChangeNetTeardownTopic)) {
        if (!mOffline) {
            SetOffline(PR_TRUE);
            mOfflineForProfileChange = PR_TRUE;
        }
    }
    else if (!strcmp(topic, kProfileChangeNetRestoreTopic)) {
        if (mOfflineForProfileChange) {
            mOfflineForProfileChange = PR_FALSE;
            if (!mManageOfflineStatus ||
                NS_FAILED(TrackNetworkLinkStatusForOffline())) {
                SetOffline(PR_FALSE);
            }
        } 
    }
    else if (!strcmp(topic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
        
        
        
        mShutdown = PR_TRUE;

        SetOffline(PR_TRUE);

        
        mProxyService = nsnull;
    }
    else if (!strcmp(topic, NS_NETWORK_LINK_TOPIC)) {
        if (!mOfflineForProfileChange && mManageOfflineStatus) {
            TrackNetworkLinkStatusForOffline();
        }
    }
    
    return NS_OK;
}


NS_IMETHODIMP
nsIOService::ParseContentType(const nsACString &aTypeHeader,
                              nsACString &aCharset,
                              PRBool *aHadCharset,
                              nsACString &aContentType)
{
    net_ParseContentType(aTypeHeader, aContentType, aCharset, aHadCharset);
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::ProtocolHasFlags(nsIURI   *uri,
                              PRUint32  flags,
                              PRBool   *result)
{
    NS_ENSURE_ARG(uri);

    *result = PR_FALSE;
    nsCAutoString scheme;
    nsresult rv = uri->GetScheme(scheme);
    NS_ENSURE_SUCCESS(rv, rv);
  
    PRUint32 protocolFlags;
    rv = GetProtocolFlags(scheme.get(), &protocolFlags);

    if (NS_SUCCEEDED(rv)) {
        *result = (protocolFlags & flags) == flags;
    }
  
    return rv;
}

NS_IMETHODIMP
nsIOService::URIChainHasFlags(nsIURI   *uri,
                              PRUint32  flags,
                              PRBool   *result)
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
        *result = nsnull;
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
nsIOService::SetManageOfflineStatus(PRBool aManage) {
    PRBool wasManaged = mManageOfflineStatus;
    mManageOfflineStatus = aManage;
    if (mManageOfflineStatus && !wasManaged)
        return TrackNetworkLinkStatusForOffline();
    return NS_OK;
}

NS_IMETHODIMP
nsIOService::GetManageOfflineStatus(PRBool* aManage) {
    *aManage = mManageOfflineStatus;
    return NS_OK;
}

nsresult
nsIOService::TrackNetworkLinkStatusForOffline()
{
    NS_ASSERTION(mManageOfflineStatus,
                 "Don't call this unless we're managing the offline status");
    if (!mNetworkLinkService)
        return NS_ERROR_FAILURE;

    if (mShutdown)
        return NS_ERROR_NOT_AVAILABLE;
  
    
    if (mSocketTransportService) {
        PRBool autodialEnabled = PR_FALSE;
        mSocketTransportService->GetAutodialEnabled(&autodialEnabled);
        
        
        
        if (autodialEnabled) {
#if defined(XP_WIN)
            
            
            
            
            if(nsNativeConnectionHelper::IsAutodialEnabled()) 
                return SetOffline(PR_FALSE);
#else
            return SetOffline(PR_FALSE);
#endif
        }
    }
  
    PRBool isUp;
    nsresult rv = mNetworkLinkService->GetIsLinkUp(&isUp);
    NS_ENSURE_SUCCESS(rv, rv);
    return SetOffline(!isUp);
}

NS_IMETHODIMP
nsIOService::EscapeString(const nsACString& aString,
                          PRUint32 aEscapeType,
                          nsACString& aResult)
{
  NS_ENSURE_ARG_RANGE(aEscapeType, 0, 4);

  nsCAutoString stringCopy(aString);
  nsCString result;

  if (!NS_Escape(stringCopy, result, (nsEscapeMask) aEscapeType))
    return NS_ERROR_OUT_OF_MEMORY;

  aResult.Assign(result);

  return NS_OK;
}

NS_IMETHODIMP 
nsIOService::EscapeURL(const nsACString &aStr, 
                       PRUint32 aFlags, nsACString &aResult)
{
  aResult.Truncate();
  NS_EscapeURL(aStr.BeginReading(), aStr.Length(), 
               aFlags | esc_AlwaysCopy, aResult);
  return NS_OK;
}

NS_IMETHODIMP 
nsIOService::UnescapeString(const nsACString &aStr, 
                            PRUint32 aFlags, nsACString &aResult)
{
  aResult.Truncate();
  NS_UnescapeURL(aStr.BeginReading(), aStr.Length(), 
                 aFlags | esc_AlwaysCopy, aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsIOService::ExtractCharsetFromContentType(const nsACString &aTypeHeader,
                                           nsACString &aCharset,
                                           PRInt32 *aCharsetStart,
                                           PRInt32 *aCharsetEnd,
                                           PRBool *aHadCharset)
{
    nsCAutoString ignored;
    net_ParseContentType(aTypeHeader, ignored, aCharset, aHadCharset,
                         aCharsetStart, aCharsetEnd);
    if (*aHadCharset && *aCharsetStart == *aCharsetEnd) {
        *aHadCharset = PR_FALSE;
    }
    return NS_OK;
}
