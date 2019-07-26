




#ifndef nsIOService_h__
#define nsIOService_h__

#include "necko-config.h"

#include "nsString.h"
#include "nsIIOService2.h"
#include "nsTArray.h"
#include "nsPISocketTransportService.h" 
#include "nsPIDNSService.h" 
#include "nsIProtocolProxyService2.h"
#include "nsCOMPtr.h"
#include "nsURLHelper.h"
#include "nsWeakPtr.h"
#include "nsIURLParser.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsINetUtil.h"
#include "nsIChannelEventSink.h"
#include "nsIContentSniffer.h"
#include "nsCategoryCache.h"
#include "nsINetworkLinkService.h"
#include "nsAsyncRedirectVerifyHelper.h"
#include "nsISpeculativeConnect.h"

#define NS_N(x) (sizeof(x)/sizeof(*x))




#define NS_IPC_IOSERVICE_SET_OFFLINE_TOPIC "ipc:network:set-offline"

static const char gScheme[][sizeof("resource")] =
    {"chrome", "file", "http", "jar", "resource"};

class nsIPrefBranch;

class nsIOService : public nsIIOService2
                  , public nsIObserver
                  , public nsINetUtil
                  , public nsISpeculativeConnect
                  , public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIIOSERVICE
    NS_DECL_NSIIOSERVICE2
    NS_DECL_NSIOBSERVER
    NS_DECL_NSINETUTIL
    NS_DECL_NSISPECULATIVECONNECT

    
    
    
    static nsIOService* GetInstance();

    NS_HIDDEN_(nsresult) Init();
    NS_HIDDEN_(nsresult) NewURI(const char* aSpec, nsIURI* aBaseURI,
                                nsIURI* *result,
                                nsIProtocolHandler* *hdlrResult);

    
    
    nsresult AsyncOnChannelRedirect(nsIChannel* oldChan, nsIChannel* newChan,
                                    PRUint32 flags,
                                    nsAsyncRedirectVerifyHelper *helper);

    
    const nsCOMArray<nsIContentSniffer>& GetContentSniffers() {
      return mContentSniffers.GetEntries();
    }

    bool IsOffline() { return mOffline; }
    bool IsLinkUp();

    bool IsComingOnline() const {
      return mOffline && mSettingOffline && !mSetOfflineValue;
    }

private:
    
    
    
    nsIOService() NS_HIDDEN;
    ~nsIOService() NS_HIDDEN;

    NS_HIDDEN_(nsresult) TrackNetworkLinkStatusForOffline();

    NS_HIDDEN_(nsresult) GetCachedProtocolHandler(const char *scheme,
                                                  nsIProtocolHandler* *hdlrResult,
                                                  PRUint32 start=0,
                                                  PRUint32 end=0);
    NS_HIDDEN_(nsresult) CacheProtocolHandler(const char *scheme,
                                              nsIProtocolHandler* hdlr);

    
    NS_HIDDEN_(void) PrefsChanged(nsIPrefBranch *prefs, const char *pref = nsnull);
    NS_HIDDEN_(void) GetPrefBranch(nsIPrefBranch **);
    NS_HIDDEN_(void) ParsePortList(nsIPrefBranch *prefBranch, const char *pref, bool remove);

    nsresult InitializeSocketTransportService();
    nsresult InitializeNetworkLinkService();

    
    void LookupProxyInfo(nsIURI *aURI, nsIURI *aProxyURI, PRUint32 aProxyFlags,
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
    nsCategoryCache<nsIContentSniffer>   mContentSniffers;

    nsTArray<PRInt32>                    mRestrictedPortList;

    bool                                 mAutoDialEnabled;
public:
    
    static PRUint32   gDefaultSegmentSize;
    static PRUint32   gDefaultSegmentCount;
};




extern nsIOService* gIOService;

#endif 
