




#ifndef nsIOService_h__
#define nsIOService_h__

#include "nsStringFwd.h"
#include "nsIIOService2.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsWeakPtr.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsINetUtil.h"
#include "nsIChannelEventSink.h"
#include "nsCategoryCache.h"
#include "nsISpeculativeConnect.h"
#include "nsDataHashtable.h"
#include "mozilla/Attributes.h"

#define NS_N(x) (sizeof(x)/sizeof(*x))




#define NS_IPC_IOSERVICE_SET_OFFLINE_TOPIC "ipc:network:set-offline"
#define NS_IPC_IOSERVICE_SET_CONNECTIVITY_TOPIC "ipc:network:set-connectivity"

static const char gScheme[][sizeof("resource")] =
    {"chrome", "file", "http", "https", "jar", "data", "resource"};

class nsAsyncRedirectVerifyHelper;
class nsINetworkLinkService;
class nsIPrefBranch;
class nsIProtocolProxyService2;
class nsIProxyInfo;
class nsPIDNSService;
class nsPISocketTransportService;

namespace mozilla {
namespace net {
    class NeckoChild;
} 
} 

class nsIOService final : public nsIIOService2
                        , public nsIObserver
                        , public nsINetUtil
                        , public nsISpeculativeConnect
                        , public nsSupportsWeakReference
                        , public nsIIOServiceInternal
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIIOSERVICE
    NS_DECL_NSIIOSERVICE2
    NS_DECL_NSIOBSERVER
    NS_DECL_NSINETUTIL
    NS_DECL_NSISPECULATIVECONNECT
    NS_DECL_NSIIOSERVICEINTERNAL

    
    
    
    static nsIOService* GetInstance();

    nsresult Init();
    nsresult NewURI(const char* aSpec, nsIURI* aBaseURI,
                                nsIURI* *result,
                                nsIProtocolHandler* *hdlrResult);

    
    
    nsresult AsyncOnChannelRedirect(nsIChannel* oldChan, nsIChannel* newChan,
                                    uint32_t flags,
                                    nsAsyncRedirectVerifyHelper *helper);

    bool IsOffline() { return mOffline; }
    bool IsLinkUp();

    
    void SetAppOfflineInternal(uint32_t appId, int32_t status);

private:
    
    
    
    nsIOService();
    ~nsIOService();
    nsresult SetConnectivityInternal(bool aConnectivity);

    nsresult OnNetworkLinkEvent(const char *data);

    nsresult GetCachedProtocolHandler(const char *scheme,
                                                  nsIProtocolHandler* *hdlrResult,
                                                  uint32_t start=0,
                                                  uint32_t end=0);
    nsresult CacheProtocolHandler(const char *scheme,
                                              nsIProtocolHandler* hdlr);

    
    void PrefsChanged(nsIPrefBranch *prefs, const char *pref = nullptr);
    void GetPrefBranch(nsIPrefBranch **);
    void ParsePortList(nsIPrefBranch *prefBranch, const char *pref, bool remove);

    nsresult InitializeSocketTransportService();
    nsresult InitializeNetworkLinkService();

    
    void LookupProxyInfo(nsIURI *aURI, nsIURI *aProxyURI, uint32_t aProxyFlags,
                         nsCString *aScheme, nsIProxyInfo **outPI);

    
    
    void NotifyAppOfflineStatus(uint32_t appId, int32_t status);
    static PLDHashOperator EnumerateWifiAppsChangingState(const unsigned int &, int32_t, void*);

    nsresult NewChannelFromURIWithProxyFlagsInternal(nsIURI* aURI,
                                                     nsIURI* aProxyURI,
                                                     uint32_t aProxyFlags,
                                                     nsILoadInfo* aLoadInfo,
                                                     nsIChannel** result);
private:
    bool                                 mOffline;
    bool                                 mOfflineForProfileChange;
    bool                                 mManageLinkStatus;
    bool                                 mConnectivity;

    
    
    bool                                 mSettingOffline;
    bool                                 mSetOfflineValue;

    bool                                 mShutdown;

    nsCOMPtr<nsPISocketTransportService> mSocketTransportService;
    nsCOMPtr<nsPIDNSService>             mDNSService;
    nsCOMPtr<nsIProtocolProxyService2>   mProxyService;
    nsCOMPtr<nsINetworkLinkService>      mNetworkLinkService;
    bool                                 mNetworkLinkServiceInitialized;

    
    nsWeakPtr                            mWeakHandler[NS_N(gScheme)];

    
    nsCategoryCache<nsIChannelEventSink> mChannelEventSinks;

    nsTArray<int32_t>                    mRestrictedPortList;

    bool                                 mAutoDialEnabled;
    bool                                 mNetworkNotifyChanged;
    int32_t                              mPreviousWifiState;
    
    
    nsDataHashtable<nsUint32HashKey, int32_t> mAppsOfflineStatus;

    static bool                          sTelemetryEnabled;
public:
    
    static uint32_t   gDefaultSegmentSize;
    static uint32_t   gDefaultSegmentCount;
};






class nsAppOfflineInfo : public nsIAppOfflineInfo
{
    NS_DECL_THREADSAFE_ISUPPORTS
public:
    nsAppOfflineInfo(uint32_t aAppId, int32_t aMode)
        : mAppId(aAppId), mMode(aMode)
    {
    }

    NS_IMETHODIMP GetMode(int32_t *aMode) override
    {
        *aMode = mMode;
        return NS_OK;
    }

    NS_IMETHODIMP GetAppId(uint32_t *aAppId) override
    {
        *aAppId = mAppId;
        return NS_OK;
    }

private:
    virtual ~nsAppOfflineInfo() {}

    uint32_t mAppId;
    int32_t mMode;
};




extern nsIOService* gIOService;

#endif 
