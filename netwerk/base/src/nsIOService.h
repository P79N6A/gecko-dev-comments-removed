




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
#include "mozilla/Attributes.h"

#define NS_N(x) (sizeof(x)/sizeof(*x))




#define NS_IPC_IOSERVICE_SET_OFFLINE_TOPIC "ipc:network:set-offline"

static const char gScheme[][sizeof("resource")] =
    {"chrome", "file", "http", "jar", "resource"};

class nsAsyncRedirectVerifyHelper;
class nsINetworkLinkService;
class nsIPrefBranch;
class nsIProtocolProxyService2;
class nsIProxyInfo;
class nsPIDNSService;
class nsPISocketTransportService;

class nsIOService MOZ_FINAL : public nsIIOService2
                            , public nsIObserver
                            , public nsINetUtil
                            , public nsISpeculativeConnect
                            , public nsSupportsWeakReference
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIIOSERVICE
    NS_DECL_NSIIOSERVICE2
    NS_DECL_NSIOBSERVER
    NS_DECL_NSINETUTIL
    NS_DECL_NSISPECULATIVECONNECT

    
    
    
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

    bool IsComingOnline() const {
      return mOffline && mSettingOffline && !mSetOfflineValue;
    }

private:
    
    
    
    nsIOService();
    ~nsIOService();

    nsresult TrackNetworkLinkStatusForOffline();

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

private:
    bool                                 mOffline;
    bool                                 mOfflineForProfileChange;
    bool                                 mManageOfflineStatus;

    
    
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
public:
    
    static uint32_t   gDefaultSegmentSize;
    static uint32_t   gDefaultSegmentCount;
};




extern nsIOService* gIOService;

#endif 
