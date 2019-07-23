





































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
#include "nsIProxyObjectManager.h"
#include "nsIStreamConverterService.h"
#include "nsICacheSession.h"
#include "nsICookieService.h"
#include "nsIIDNService.h"
#include "nsITimer.h"

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
                                       PRBool useProxy);
    PRBool   IsAcceptableEncoding(const char *encoding);

    const nsAFlatCString &UserAgent();

    nsHttpVersion  HttpVersion()             { return mHttpVersion; }
    nsHttpVersion  ProxyHttpVersion()        { return mProxyHttpVersion; }
    PRUint8        ReferrerLevel()           { return mReferrerLevel; }
    PRBool         SendSecureXSiteReferrer() { return mSendSecureXSiteReferrer; }
    PRUint8        RedirectionLimit()        { return mRedirectionLimit; }
    PRUint16       IdleTimeout()             { return mIdleTimeout; }
    PRUint16       MaxRequestAttempts()      { return mMaxRequestAttempts; }
    const char    *DefaultSocketType()       { return mDefaultSocketType.get();  }
    nsIIDNService *IDNConverter()            { return mIDNConverter; }
    PRUint32       PhishyUserPassLength()    { return mPhishyUserPassLength; }
    
    PRBool         CanCacheAllSSLContent()   { return mEnablePersistentHttpsCaching; }

    PRBool         PromptTempRedirect()      { return mPromptTempRedirect; }

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

    
    
    
    
    nsresult GetStreamConverterService(nsIStreamConverterService **);
    nsresult GetIOService(nsIIOService** service);
    nsICookieService * GetCookieService(); 

    
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

    
    
    nsresult OnChannelRedirect(nsIChannel* oldChan, nsIChannel* newChan,
                               PRUint32 flags);

    
    
    void OnExamineCachedResponse(nsIHttpChannel *chan)
    {
        NotifyObservers(chan, NS_HTTP_ON_EXAMINE_CACHED_RESPONSE_TOPIC);
    }
private:

    
    
    
    void     BuildUserAgent();
    void     InitUserAgentComponents();
    void     PrefsChanged(nsIPrefBranch *prefs, const char *pref);

    nsresult SetAccept(const char *);
    nsresult SetAcceptLanguages(const char *);
    nsresult SetAcceptEncodings(const char *);
    nsresult SetAcceptCharsets(const char *);

    nsresult InitConnectionMgr();
    void     StartPruneDeadConnectionsTimer();
    void     StopPruneDeadConnectionsTimer();

    void     NotifyObservers(nsIHttpChannel *chan, const char *event);

private:

    
    nsCOMPtr<nsIIOService>              mIOService;
    nsCOMPtr<nsIStreamConverterService> mStreamConvSvc;
    nsCOMPtr<nsIObserverService>        mObserverService;
    nsCOMPtr<nsICookieService>          mCookieService;
    nsCOMPtr<nsIIDNService>             mIDNConverter;
    nsCOMPtr<nsITimer>                  mTimer;

    
    nsHttpAuthCache mAuthCache;

    
    nsHttpConnectionMgr *mConnMgr;

    
    
    

    PRUint8  mHttpVersion;
    PRUint8  mProxyHttpVersion;
    PRUint8  mCapabilities;
    PRUint8  mProxyCapabilities;
    PRUint8  mReferrerLevel;

    PRUint16 mIdleTimeout;
    PRUint16 mMaxRequestAttempts;
    PRUint16 mMaxRequestDelay;

    PRUint16 mMaxConnections;
    PRUint8  mMaxConnectionsPerServer;
    PRUint8  mMaxPersistentConnectionsPerServer;
    PRUint8  mMaxPersistentConnectionsPerProxy;
    PRUint8  mMaxPipelinedRequests;

    PRUint8  mRedirectionLimit;

    
    
    
    
    PRUint8  mPhishyUserPassLength;

    PRPackedBool mPipeliningOverSSL;

    nsCString mAccept;
    nsCString mAcceptLanguages;
    nsCString mAcceptEncodings;
    nsCString mAcceptCharsets;

    nsXPIDLCString mDefaultSocketType;

    
    PRUint32                  mLastUniqueID;
    PRUint32                  mSessionStartTime;

    
    nsXPIDLCString mAppName;
    nsXPIDLCString mAppVersion;
    nsCString      mPlatform;
    nsCString      mOscpu;
    nsCString      mDeviceType;
    nsXPIDLCString mSecurity;
    nsCString      mLanguage;
    nsCString      mMisc;
    nsXPIDLCString mVendor;
    nsXPIDLCString mVendorSub;
    nsXPIDLCString mVendorComment;
    nsCString      mProduct;
    nsXPIDLCString mProductSub;
    nsXPIDLCString mProductComment;
    nsCString      mExtraUA;

    nsCString      mUserAgent;
    nsXPIDLCString mUserAgentOverride;
    PRPackedBool   mUserAgentIsDirty; 

    PRPackedBool   mUseCache;

    PRPackedBool   mPromptTempRedirect;
    
    
    PRPackedBool   mSendSecureXSiteReferrer;

    
    PRPackedBool   mEnablePersistentHttpsCaching;
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
