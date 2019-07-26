




#ifndef nsHttpHandler_h__
#define nsHttpHandler_h__

#include "nsHttp.h"
#include "nsHttpAuthCache.h"
#include "nsHttpConnectionMgr.h"
#include "ASpdySession.h"

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsWeakReference.h"

#include "nsIHttpDataUsage.h"
#include "nsIHttpProtocolHandler.h"
#include "nsIObserver.h"
#include "nsISpeculativeConnect.h"
#include "nsICache.h"

class nsHttpConnection;
class nsHttpConnectionInfo;
class nsHttpHeaderArray;
class nsHttpTransaction;
class nsIHttpChannel;
class nsIPrefBranch;
class nsICancelable;
class nsICookieService;
class nsIIOService;
class nsIObserverService;
class nsISiteSecurityService;
class nsIStreamConverterService;
class nsITimer;

namespace mozilla {
namespace net {
class ATokenBucketEvent;
class EventTokenBucket;
class Tickler;
}
}





class nsHttpHandler : public nsIHttpProtocolHandler
                    , public nsIObserver
                    , public nsSupportsWeakReference
                    , public nsISpeculativeConnect
                    , public nsIHttpDataUsage
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    NS_DECL_NSIPROXIEDPROTOCOLHANDLER
    NS_DECL_NSIHTTPPROTOCOLHANDLER
    NS_DECL_NSIOBSERVER
    NS_DECL_NSISPECULATIVECONNECT
    NS_DECL_NSIHTTPDATAUSAGE

    nsHttpHandler();
    virtual ~nsHttpHandler();

    nsresult Init();
    nsresult AddStandardRequestHeaders(nsHttpHeaderArray *);
    nsresult AddConnectionHeader(nsHttpHeaderArray *,
                                 uint32_t capabilities);
    bool     IsAcceptableEncoding(const char *encoding);

    const nsAFlatCString &UserAgent();

    nsHttpVersion  HttpVersion()             { return mHttpVersion; }
    nsHttpVersion  ProxyHttpVersion()        { return mProxyHttpVersion; }
    uint8_t        ReferrerLevel()           { return mReferrerLevel; }
    bool           SendSecureXSiteReferrer() { return mSendSecureXSiteReferrer; }
    uint8_t        RedirectionLimit()        { return mRedirectionLimit; }
    PRIntervalTime IdleTimeout()             { return mIdleTimeout; }
    PRIntervalTime SpdyTimeout()             { return mSpdyTimeout; }
    uint16_t       MaxRequestAttempts()      { return mMaxRequestAttempts; }
    const char    *DefaultSocketType()       { return mDefaultSocketType.get();  }
    uint32_t       PhishyUserPassLength()    { return mPhishyUserPassLength; }
    uint8_t        GetQoSBits()              { return mQoSBits; }
    uint16_t       GetIdleSynTimeout()       { return mIdleSynTimeout; }
    bool           FastFallbackToIPv4()      { return mFastFallbackToIPv4; }
    bool           ProxyPipelining()         { return mProxyPipelining; }
    uint32_t       MaxSocketCount();
    bool           EnforceAssocReq()         { return mEnforceAssocReq; }

    bool           IsPersistentHttpsCachingEnabled() { return mEnablePersistentHttpsCaching; }
    bool           IsTelemetryEnabled() { return mTelemetryEnabled; }
    bool           AllowExperiments() { return mTelemetryEnabled && mAllowExperiments; }

    bool           IsSpdyEnabled() { return mEnableSpdy; }
    bool           IsSpdyV2Enabled() { return mSpdyV2; }
    bool           IsSpdyV3Enabled() { return mSpdyV3; }
    bool           IsSpdyV31Enabled() { return mSpdyV31; }
    bool           CoalesceSpdy() { return mCoalesceSpdy; }
    bool           UseSpdyPersistentSettings() { return mSpdyPersistentSettings; }
    uint32_t       SpdySendingChunkSize() { return mSpdySendingChunkSize; }
    uint32_t       SpdySendBufferSize()      { return mSpdySendBufferSize; }
    uint32_t       SpdyPushAllowance()       { return mSpdyPushAllowance; }
    PRIntervalTime SpdyPingThreshold() { return mSpdyPingThreshold; }
    PRIntervalTime SpdyPingTimeout() { return mSpdyPingTimeout; }
    bool           AllowSpdyPush()   { return mAllowSpdyPush; }
    uint32_t       ConnectTimeout()  { return mConnectTimeout; }
    uint32_t       ParallelSpeculativeConnectLimit() { return mParallelSpeculativeConnectLimit; }
    bool           CritialRequestPrioritization() { return mCritialRequestPrioritization; }
    double         BypassCacheLockThreshold() { return mBypassCacheLockThreshold; }

    bool           UseRequestTokenBucket() { return mRequestTokenBucketEnabled; }
    uint16_t       RequestTokenBucketMinParallelism() { return mRequestTokenBucketMinParallelism; }
    uint32_t       RequestTokenBucketHz() { return mRequestTokenBucketHz; }
    uint32_t       RequestTokenBucketBurst() {return mRequestTokenBucketBurst; }

    bool           PromptTempRedirect()      { return mPromptTempRedirect; }

    nsHttpAuthCache     *AuthCache(bool aPrivate) {
        return aPrivate ? &mPrivateAuthCache : &mAuthCache;
    }
    nsHttpConnectionMgr *ConnMgr()   { return mConnMgr; }

    
    bool UseCache() const { return mUseCache; }
    uint32_t GenerateUniqueID() { return ++mLastUniqueID; }
    uint32_t SessionStartTime() { return mSessionStartTime; }

    
    
    
    
    
    
    
    
    

    
    
    
    nsresult InitiateTransaction(nsHttpTransaction *trans, int32_t priority)
    {
        return mConnMgr->AddTransaction(trans, priority);
    }

    
    
    nsresult RescheduleTransaction(nsHttpTransaction *trans, int32_t priority)
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

    nsresult ProcessPendingQ()
    {
        return mConnMgr->ProcessPendingQ();
    }

    nsresult GetSocketThreadTarget(nsIEventTarget **target)
    {
        return mConnMgr->GetSocketThreadTarget(target);
    }

    nsresult SpeculativeConnect(nsHttpConnectionInfo *ci,
                                nsIInterfaceRequestor *callbacks,
                                uint32_t caps = 0)
    {
        TickleWifi(callbacks);
        return mConnMgr->SpeculativeConnect(ci, callbacks, caps);
    }

    
    
    
    
    nsresult GetStreamConverterService(nsIStreamConverterService **);
    nsresult GetIOService(nsIIOService** service);
    nsICookieService * GetCookieService(); 
    nsISiteSecurityService * GetSSService();

    
    uint32_t Get32BitsOfPseudoRandom();

    
    void OnOpeningRequest(nsIHttpChannel *chan)
    {
        NotifyObservers(chan, NS_HTTP_ON_OPENING_REQUEST_TOPIC);
    }

    
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
                               uint32_t flags);

    
    
    void OnExamineCachedResponse(nsIHttpChannel *chan)
    {
        NotifyObservers(chan, NS_HTTP_ON_EXAMINE_CACHED_RESPONSE_TOPIC);
    }

    
    
    static nsresult GenerateHostPort(const nsCString& host, int32_t port,
                                     nsCString& hostLine);

    bool GetPipelineAggressive()     { return mPipelineAggressive; }
    void GetMaxPipelineObjectSize(int64_t *outVal)
    {
        *outVal = mMaxPipelineObjectSize;
    }

    bool GetPipelineEnabled()
    {
        return mCapabilities & NS_HTTP_ALLOW_PIPELINING;
    }

    bool GetPipelineRescheduleOnTimeout()
    {
        return mPipelineRescheduleOnTimeout;
    }

    PRIntervalTime GetPipelineRescheduleTimeout()
    {
        return mPipelineRescheduleTimeout;
    }

    PRIntervalTime GetPipelineTimeout()   { return mPipelineReadTimeout; }

    mozilla::net::SpdyInformation *SpdyInfo() { return &mSpdyInfo; }

    
    bool Active() { return mHandlerActive; }

    static void GetCacheSessionNameForStoragePolicy(
            nsCacheStoragePolicy storagePolicy,
            bool isPrivate,
            uint32_t appId,
            bool inBrowser,
            nsACString& sessionName);

    
    
    mozilla::TimeStamp GetCacheSkippedUntil() { return mCacheSkippedUntil; }
    void SetCacheSkippedUntil(mozilla::TimeStamp arg) { mCacheSkippedUntil = arg; }
    void ClearCacheSkippedUntil() { mCacheSkippedUntil = mozilla::TimeStamp(); }

private:

    
    
    
    void     BuildUserAgent();
    void     InitUserAgentComponents();
    void     PrefsChanged(nsIPrefBranch *prefs, const char *pref);

    nsresult SetAccept(const char *);
    nsresult SetAcceptLanguages(const char *);
    nsresult SetAcceptEncodings(const char *);

    nsresult InitConnectionMgr();

    void     NotifyObservers(nsIHttpChannel *chan, const char *event);

    static void TimerCallback(nsITimer * aTimer, void * aClosure);
private:

    
    nsMainThreadPtrHandle<nsIIOService>              mIOService;
    nsMainThreadPtrHandle<nsIStreamConverterService> mStreamConvSvc;
    nsMainThreadPtrHandle<nsIObserverService>        mObserverService;
    nsMainThreadPtrHandle<nsICookieService>          mCookieService;
    nsMainThreadPtrHandle<nsISiteSecurityService>    mSSService;

    
    nsHttpAuthCache mAuthCache;
    nsHttpAuthCache mPrivateAuthCache;

    
    nsHttpConnectionMgr *mConnMgr;

    
    
    

    uint8_t  mHttpVersion;
    uint8_t  mProxyHttpVersion;
    uint32_t mCapabilities;
    uint8_t  mReferrerLevel;

    bool mFastFallbackToIPv4;
    bool mProxyPipelining;
    PRIntervalTime mIdleTimeout;
    PRIntervalTime mSpdyTimeout;

    uint16_t mMaxRequestAttempts;
    uint16_t mMaxRequestDelay;
    uint16_t mIdleSynTimeout;

    bool     mPipeliningEnabled;
    uint16_t mMaxConnections;
    uint8_t  mMaxPersistentConnectionsPerServer;
    uint8_t  mMaxPersistentConnectionsPerProxy;
    uint16_t mMaxPipelinedRequests;
    uint16_t mMaxOptimisticPipelinedRequests;
    bool     mPipelineAggressive;
    int64_t  mMaxPipelineObjectSize;
    bool     mPipelineRescheduleOnTimeout;
    PRIntervalTime mPipelineRescheduleTimeout;
    PRIntervalTime mPipelineReadTimeout;
    nsCOMPtr<nsITimer> mPipelineTestTimer;

    uint8_t  mRedirectionLimit;

    
    
    
    
    uint8_t  mPhishyUserPassLength;

    uint8_t  mQoSBits;

    bool mPipeliningOverSSL;
    bool mEnforceAssocReq;

    nsCString mAccept;
    nsCString mAcceptLanguages;
    nsCString mAcceptEncodings;

    nsXPIDLCString mDefaultSocketType;

    
    uint32_t                  mLastUniqueID;
    uint32_t                  mSessionStartTime;

    
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
    bool           mCompatFirefoxEnabled;
    nsXPIDLCString mCompatDevice;

    nsCString      mUserAgent;
    nsXPIDLCString mUserAgentOverride;
    bool           mUserAgentIsDirty; 

    bool           mUseCache;

    bool           mPromptTempRedirect;
    
    
    bool           mSendSecureXSiteReferrer;

    
    bool           mEnablePersistentHttpsCaching;

    
    bool           mDoNotTrackEnabled;
    uint8_t        mDoNotTrackValue;

    
    bool           mTelemetryEnabled;

    
    bool           mAllowExperiments;

    
    bool           mHandlerActive;

    
    mozilla::net::SpdyInformation mSpdyInfo;
    bool           mEnableSpdy;
    bool           mSpdyV2;
    bool           mSpdyV3;
    bool           mSpdyV31;
    bool           mCoalesceSpdy;
    bool           mSpdyPersistentSettings;
    bool           mAllowSpdyPush;
    uint32_t       mSpdySendingChunkSize;
    uint32_t       mSpdySendBufferSize;
    uint32_t       mSpdyPushAllowance;
    PRIntervalTime mSpdyPingThreshold;
    PRIntervalTime mSpdyPingTimeout;

    
    
    uint32_t       mConnectTimeout;

    
    
    double         mBypassCacheLockThreshold;

    
    
    uint32_t       mParallelSpeculativeConnectLimit;

    
    
    bool           mRequestTokenBucketEnabled;
    uint16_t       mRequestTokenBucketMinParallelism;
    uint32_t       mRequestTokenBucketHz;  
    uint32_t       mRequestTokenBucketBurst; 

    
    
    bool           mCritialRequestPrioritization;

    
    
    mozilla::TimeStamp                mCacheSkippedUntil;

private:
    
    
    void MakeNewRequestTokenBucket();
    nsRefPtr<mozilla::net::EventTokenBucket> mRequestTokenBucket;

public:
    
    nsresult SubmitPacedRequest(mozilla::net::ATokenBucketEvent *event,
                                nsICancelable **cancel)
    {
        if (!mRequestTokenBucket)
            return NS_ERROR_UNEXPECTED;
        return mRequestTokenBucket->SubmitEvent(event, cancel);
    }

    
    void SetRequestTokenBucket(mozilla::net::EventTokenBucket *aTokenBucket)
    {
        mRequestTokenBucket = aTokenBucket;
    }

private:
    
    uint64_t mEthernetBytesRead;
    uint64_t mEthernetBytesWritten;
    uint64_t mCellBytesRead;
    uint64_t mCellBytesWritten;
    bool     mNetworkTypeKnown;
    bool     mNetworkTypeWasEthernet;

    nsRefPtr<mozilla::net::Tickler> mWifiTickler;
    nsresult GetNetworkEthernetInfo(nsIInterfaceRequestor *cb,
                                    bool *ethernet);
    nsresult GetNetworkEthernetInfoInner(nsIInterfaceRequestor *cb,
                                         bool *ethernet);
    nsresult GetNetworkInfo(nsIInterfaceRequestor *cb,
                            bool *ethernet, uint32_t *gw);
    nsresult GetNetworkInfoInner(nsIInterfaceRequestor *cb,
                                 bool *ethernet, uint32_t *gw);
    void TickleWifi(nsIInterfaceRequestor *cb);

public:
    
    
    void UpdateDataUsage(nsIInterfaceRequestor *cb,
                         uint64_t bytesRead, uint64_t bytesWritten);
};

extern nsHttpHandler *gHttpHandler;






class nsHttpsHandler : public nsIHttpProtocolHandler
                     , public nsSupportsWeakReference
                     , public nsISpeculativeConnect
{
public:
    
    

    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    NS_FORWARD_NSIPROXIEDPROTOCOLHANDLER (gHttpHandler->)
    NS_FORWARD_NSIHTTPPROTOCOLHANDLER    (gHttpHandler->)
    NS_FORWARD_NSISPECULATIVECONNECT     (gHttpHandler->)

    nsHttpsHandler() { }
    virtual ~nsHttpsHandler() { }

    nsresult Init();
};

#endif 
