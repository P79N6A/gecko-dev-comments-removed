




































#ifndef nsIOService_h__
#define nsIOService_h__

#include "necko-config.h"

#include "nsString.h"
#include "nsIIOService2.h"
#include "nsVoidArray.h"
#include "nsPISocketTransportService.h" 
#include "nsPIDNSService.h" 
#include "nsIProtocolProxyService2.h"
#include "nsCOMPtr.h"
#include "nsURLHelper.h"
#include "nsWeakPtr.h"
#include "nsIURLParser.h"
#include "nsSupportsArray.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsINetUtil.h"
#include "nsIChannelEventSink.h"
#include "nsIContentSniffer.h"
#include "nsCategoryCache.h"
#include "nsINetworkLinkService.h"

#define NS_N(x) (sizeof(x)/sizeof(*x))

#ifdef NECKO_SMALL_BUFFERS
#define NS_NECKO_BUFFER_CACHE_COUNT (10)  // Max holdings: 10 * 2k = 20k
#else
#define NS_NECKO_BUFFER_CACHE_COUNT (24)  // Max holdings: 24 * 4k = 96k
#endif
#define NS_NECKO_15_MINS (15 * 60)

static const char gScheme[][sizeof("resource")] =
    {"chrome", "file", "http", "jar", "resource"};

class nsIPrefBranch;
class nsIPrefBranch2;

class nsIOService : public nsIIOService2
                  , public nsIObserver
                  , public nsINetUtil
                  , public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIIOSERVICE
    NS_DECL_NSIIOSERVICE2
    NS_DECL_NSIOBSERVER
    NS_DECL_NSINETUTIL

    
    
    
    static nsIOService* GetInstance();

    NS_HIDDEN_(nsresult) Init();
    NS_HIDDEN_(nsresult) NewURI(const char* aSpec, nsIURI* aBaseURI,
                                nsIURI* *result,
                                nsIProtocolHandler* *hdlrResult);

    
    
    nsresult OnChannelRedirect(nsIChannel* oldChan, nsIChannel* newChan,
                               PRUint32 flags);

    
    const nsCOMArray<nsIContentSniffer>& GetContentSniffers() {
      return mContentSniffers.GetEntries();
    }

    PRBool IsOffline() { return mOffline; }

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
    NS_HIDDEN_(void) GetPrefBranch(nsIPrefBranch2 **);
    NS_HIDDEN_(void) ParsePortList(nsIPrefBranch *prefBranch, const char *pref, PRBool remove);

private:
    PRPackedBool                         mOffline;
    PRPackedBool                         mOfflineForProfileChange;
    PRPackedBool                         mManageOfflineStatus;
    nsCOMPtr<nsPISocketTransportService> mSocketTransportService;
    nsCOMPtr<nsPIDNSService>             mDNSService;
    nsCOMPtr<nsIProtocolProxyService2>   mProxyService;
    nsCOMPtr<nsINetworkLinkService>      mNetworkLinkService;
    
    
    nsWeakPtr                            mWeakHandler[NS_N(gScheme)];

    
    nsCategoryCache<nsIChannelEventSink> mChannelEventSinks;
    nsCategoryCache<nsIContentSniffer>   mContentSniffers;

    nsVoidArray                          mRestrictedPortList;

public:
    
    
    static nsIMemory *gBufferCache;
};




extern nsIOService* gIOService;

#endif 
