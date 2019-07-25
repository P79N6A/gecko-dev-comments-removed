





































#ifndef nsHttpHandler_h__
#define nsHttpHandler_h__

#include "nsHttp.h"
#include "nsHttpAuthCache.h"
#include "nsHttpConnection.h"
#include "nsHttpConnectionMgr.h"

#include "nsXPIDLString.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsWeakReference.h"

#include "nsIHttpProtocolHandler.h"
#include "nsIProtocolProxyService.h"
#include "nsIIOService.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIPrivateBrowsingService.h"
#include "nsIStreamConverterService.h"
#include "nsICacheSession.h"
#include "nsICookieService.h"
#include "nsIIDNService.h"
#include "nsITimer.h"
#include "nsIStrictTransportSecurityService.h"

class nsHttpConnectionInfo;
class nsHttpHeaderArray;
class nsHttpTransaction;
class nsAHttpTransaction;
class nsIHttpChannel;
class nsIPrefBranch;





class nsHttpHandler : public nsIHttpProtocolHandler
                    , public nsIObserver
                    , public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    NS_DECL_NSIPROXIEDPROTOCOLHANDLER
    NS_DECL_NSIHTTPPROTOCOLHANDLER
    NS_DECL_NSIOBSERVER

    nsHttpHandler();
    virtual ~nsHttpHandler();

    nsresult Init();
    nsresult AddStandardRequestHeaders(nsHttpHeaderArray *,
                                       PRUint8 capabilities,
                                       bool useProxy);
    bool     IsAcceptableEncoding(const char *encoding);

    const nsAFlatCString &UserAgent();

    nsHttpVersion  HttpVersion()             { return mHttpVersion; }
    nsHttpVersion  ProxyHttpVersion()        { return mProxyHttpVersion; }
    PRUint8        ReferrerLevel()           { return mReferrerLevel; }
    bool           SendSecureXSiteReferrer() { return mSendSecureXSiteReferrer; }
    PRUint8        RedirectionLimit()        { return mRedirectionLimit; }
    PRUint16       IdleTimeout()             { return mIdleTimeout; }
    PRUint16       SpdyTimeout()             { return mSpdyTimeout; }
    PRUint16       MaxRequestAttempts()      { return mMaxRequestAttempts; }
    const char    *DefaultSocketType()       { return mDefaultSocketType.get();  }
    nsIIDNService *IDNConverter()            { return mIDNConverter; }
    PRUint32       PhishyUserPassLength()    { return mPhishyUserPassLength; }
    PRUint8        GetQoSBits()              { return mQoSBits; }
    PRUint16       GetIdleSynTimeout()       { return mIdleSynTimeout; }
    bool           FastFallbackToIPv4()      { return mFastFallbackToIPv4; }
    PRUint32       MaxSocketCount();

    bool           IsPersistentHttpsCachingEnabled() { return mEnablePersistentHttpsCaching; }

    bool           IsSpdyEnabled() { return mEnableSpdy; }
    bool           CoalesceSpdy() { return mCoalesceSpdy; }
    bool           UseAlternateProtocol() { return mUseAlternateProtocol; }

    bool           PromptTempRedirect()      { return mPromptTempRedirect; }

    nsHttpAuthCache     *AuthCache() { return &mAuthCache; }
    nsHttpConnectionMgr *ConnMgr()   { return mConnMgr; }

    
    nsresult GetCacheSession(nsCacheStoragePolicy, nsICacheSession **);
    PRUint32 GenerateUniqueID() { return ++mLastUniqueID; }
    PRUint32 SessionStartTime() { return mSessionStartTime; }

    
    
    
    
    
    
    
    
    

    
    
    
    nsresult InitiateTransaction(nsHttpTransaction *trans, PRInt32 priority)
    {
        return mConnMgr->AddTransaction(trans, priority);
    }

    
    
    nsresult RescheduleTransaction(nsHttpTransaction *trans, PRInt32 priority)
    {
        return mConnMgr->RescheduleTransaction(trans, priority);
    }

    
    
    nsresult CancelTransaction(nsHttpTransaction *trans, nsresult reason)
    {
        return mConnMgr->CancelTransaction(trans, reason);
    }

    
    
    nsresult ReclaimConnection(nsHttpConnection *conn)
    {
        return mConnMgr->ReclaimConnection(conn);
    }

    nsresult ProcessPendingQ(nsHttpConnectionInfo *cinfo)
    {
        return mConnMgr->ProcessPendingQ(cinfo);
    }

    nsresult GetSocketThreadTarget(nsIEventTarget **target)
    {
        return mConnMgr->GetSocketThreadTarget(target);
    }

    
    bool InPrivateBrowsingMode();

    
    
    
    
    nsresult GetStreamConverterService(nsIStreamConverterService **);
    nsresult GetIOService(nsIIOService** service);
    nsICookieService * GetCookieService(); 
    nsIStrictTransportSecurityService * GetSTSService();

    
    void OnModifyRequest(nsIHttpChannel *chan)
    {
        NotifyObservers(chan, NS_HTTP_ON_MODIFY_REQUEST_TOPIC);
    }

    
    void OnExamineResponse(nsIHttpChannel *chan)
    {
        NotifyObservers(chan, NS_HTTP_ON_EXAMINE_RESPONSE_TOPIC);
    }

    
    void OnExamineMergedResponse(nsIHttpChannel *chan)
    {
        NotifyObservers(chan, NS_HTTP_ON_EXAMINE_MERGED_RESPONSE_TOPIC);
    }

    
    
    nsresult AsyncOnChannelRedirect(nsIChannel* oldChan, nsIChannel* newChan,
                               PRUint32 flags);

    
    
    void OnExamineCachedResponse(nsIHttpChannel *chan)
    {
        NotifyObservers(chan, NS_HTTP_ON_EXAMINE_CACHED_RESPONSE_TOPIC);
    }

    
    
    static nsresult GenerateHostPort(const nsCString& host, PRInt32 port,
                                     nsCString& hostLine);

private:

    
    
    
    void     BuildUserAgent();
    void     InitUserAgentComponents();
    void     PrefsChanged(nsIPrefBranch *prefs, const char *pref);

    nsresult SetAccept(const char *);
    nsresult SetAcceptLanguages(const char *);
    nsresult SetAcceptEncodings(const char *);

    nsresult InitConnectionMgr();

    void     NotifyObservers(nsIHttpChannel *chan, const char *event);

private:

    
    nsCOMPtr<nsIIOService>              mIOService;
    nsCOMPtr<nsIStreamConverterService> mStreamConvSvc;
    nsCOMPtr<nsIObserverService>        mObserverService;
    nsCOMPtr<nsICookieService>          mCookieService;
    nsCOMPtr<nsIIDNService>             mIDNConverter;
    nsCOMPtr<nsIStrictTransportSecurityService> mSTSService;

    
    nsHttpAuthCache mAuthCache;

    
    nsHttpConnectionMgr *mConnMgr;

    
    
    

    PRUint8  mHttpVersion;
    PRUint8  mProxyHttpVersion;
    PRUint8  mCapabilities;
    PRUint8  mProxyCapabilities;
    PRUint8  mReferrerLevel;

    bool mFastFallbackToIPv4;

    PRUint16 mIdleTimeout;
    PRUint16 mSpdyTimeout;
    PRUint16 mMaxRequestAttempts;
    PRUint16 mMaxRequestDelay;
    PRUint16 mIdleSynTimeout;

    PRUint16 mMaxConnections;
    PRUint8  mMaxConnectionsPerServer;
    PRUint8  mMaxPersistentConnectionsPerServer;
    PRUint8  mMaxPersistentConnectionsPerProxy;
    PRUint8  mMaxPipelinedRequests;

    PRUint8  mRedirectionLimit;

    
    
    
    
    PRUint8  mPhishyUserPassLength;

    PRUint8  mQoSBits;

    bool mPipeliningOverSSL;

    
    enum {
        PRIVATE_BROWSING_OFF = false,
        PRIVATE_BROWSING_ON = true,
        PRIVATE_BROWSING_UNKNOWN = 2
    } mInPrivateBrowsingMode;

    nsCString mAccept;
    nsCString mAcceptLanguages;
    nsCString mAcceptEncodings;

    nsXPIDLCString mDefaultSocketType;

    
    PRUint32                  mLastUniqueID;
    PRUint32                  mSessionStartTime;

    
    nsCString      mLegacyAppName;
    nsCString      mLegacyAppVersion;
    nsCString      mPlatform;
    nsCString      mOscpu;
    nsCString      mMisc;
    nsCString      mProduct;
    nsXPIDLCString mProductSub;
    nsXPIDLCString mAppName;
    nsXPIDLCString mAppVersion;
    nsCString      mCompatFirefox;

    nsCString      mUserAgent;
    nsXPIDLCString mUserAgentOverride;
    bool           mUserAgentIsDirty; 

    bool           mUseCache;

    bool           mPromptTempRedirect;
    
    
    bool           mSendSecureXSiteReferrer;

    
    bool           mEnablePersistentHttpsCaching;

    
    bool           mDoNotTrackEnabled;
    
    
    bool           mEnableSpdy;
    bool           mCoalesceSpdy;
    bool           mUseAlternateProtocol;
};



extern nsHttpHandler *gHttpHandler;






class nsHttpsHandler : public nsIHttpProtocolHandler
                     , public nsSupportsWeakReference
{
public:
    
    
    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    NS_FORWARD_NSIPROXIEDPROTOCOLHANDLER (gHttpHandler->)
    NS_FORWARD_NSIHTTPPROTOCOLHANDLER    (gHttpHandler->)

    nsHttpsHandler() { }
    virtual ~nsHttpsHandler() { }

    nsresult Init();
};

#endif 
