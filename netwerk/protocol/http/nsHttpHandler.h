




#ifndef nsHttpHandler_h__
#define nsHttpHandler_h__

#include "nsHttp.h"
#include "nsHttpAuthCache.h"
#include "nsHttpConnectionMgr.h"
#include "ASpdySession.h"

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsWeakReference.h"

#include "nsIHttpProtocolHandler.h"
#include "nsIObserver.h"
#include "nsISpeculativeConnect.h"

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
class nsHttpConnection;
class nsHttpConnectionInfo;
class nsHttpTransaction;
class AltSvcMapping;

enum FrameCheckLevel {
    FRAMECHECK_LAX,
    FRAMECHECK_BARELY,
    FRAMECHECK_STRICT
};





class nsHttpHandler final : public nsIHttpProtocolHandler
                          , public nsIObserver
                          , public nsSupportsWeakReference
                          , public nsISpeculativeConnect
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    NS_DECL_NSIPROXIEDPROTOCOLHANDLER
    NS_DECL_NSIHTTPPROTOCOLHANDLER
    NS_DECL_NSIOBSERVER
    NS_DECL_NSISPECULATIVECONNECT

    nsHttpHandler();

    nsresult Init();
    nsresult AddStandardRequestHeaders(nsHttpHeaderArray *);
    nsresult AddConnectionHeader(nsHttpHeaderArray *,
                                 uint32_t capabilities);
    bool     IsAcceptableEncoding(const char *encoding);

    const nsAFlatCString &UserAgent();

    nsHttpVersion  HttpVersion()             { return mHttpVersion; }
    nsHttpVersion  ProxyHttpVersion()        { return mProxyHttpVersion; }
    uint8_t        ReferrerLevel()           { return mReferrerLevel; }
    bool           SpoofReferrerSource()     { return mSpoofReferrerSource; }
    uint8_t        ReferrerTrimmingPolicy()  { return mReferrerTrimmingPolicy; }
    uint8_t        ReferrerXOriginPolicy()   { return mReferrerXOriginPolicy; }
    bool           SendSecureXSiteReferrer() { return mSendSecureXSiteReferrer; }
    uint8_t        RedirectionLimit()        { return mRedirectionLimit; }
    PRIntervalTime IdleTimeout()             { return mIdleTimeout; }
    PRIntervalTime SpdyTimeout()             { return mSpdyTimeout; }
    PRIntervalTime ResponseTimeout() {
      return mResponseTimeoutEnabled ? mResponseTimeout : 0;
    }
    PRIntervalTime ResponseTimeoutEnabled()  { return mResponseTimeoutEnabled; }
    uint32_t       NetworkChangedTimeout()   { return mNetworkChangedTimeout; }
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
    bool           IsSpdyV31Enabled() { return mSpdyV31; }
    bool           IsHttp2DraftEnabled() { return mHttp2DraftEnabled; }
    bool           IsHttp2Enabled() { return mHttp2DraftEnabled && mHttp2Enabled; }
    bool           EnforceHttp2TlsProfile() { return mEnforceHttp2TlsProfile; }
    bool           CoalesceSpdy() { return mCoalesceSpdy; }
    bool           UseSpdyPersistentSettings() { return mSpdyPersistentSettings; }
    uint32_t       SpdySendingChunkSize() { return mSpdySendingChunkSize; }
    uint32_t       SpdySendBufferSize()      { return mSpdySendBufferSize; }
    uint32_t       SpdyPushAllowance()       { return mSpdyPushAllowance; }
    uint32_t       DefaultSpdyConcurrent()   { return mDefaultSpdyConcurrent; }
    PRIntervalTime SpdyPingThreshold() { return mSpdyPingThreshold; }
    PRIntervalTime SpdyPingTimeout() { return mSpdyPingTimeout; }
    bool           AllowPush()   { return mAllowPush; }
    bool           AllowAltSvc() { return mEnableAltSvc; }
    bool           AllowAltSvcOE() { return mEnableAltSvcOE; }
    uint32_t       ConnectTimeout()  { return mConnectTimeout; }
    uint32_t       ParallelSpeculativeConnectLimit() { return mParallelSpeculativeConnectLimit; }
    bool           CriticalRequestPrioritization() { return mCriticalRequestPrioritization; }
    bool           UseH2Deps() { return mUseH2Deps; }

    uint32_t       MaxConnectionsPerOrigin() { return mMaxPersistentConnectionsPerServer; }
    bool           UseRequestTokenBucket() { return mRequestTokenBucketEnabled; }
    uint16_t       RequestTokenBucketMinParallelism() { return mRequestTokenBucketMinParallelism; }
    uint32_t       RequestTokenBucketHz() { return mRequestTokenBucketHz; }
    uint32_t       RequestTokenBucketBurst() {return mRequestTokenBucketBurst; }

    bool           PromptTempRedirect()      { return mPromptTempRedirect; }

    

    
    bool TCPKeepaliveEnabledForShortLivedConns() {
      return mTCPKeepaliveShortLivedEnabled;
    }
    
    
    int32_t GetTCPKeepaliveShortLivedTime() {
      return mTCPKeepaliveShortLivedTimeS;
    }
    
    
    int32_t GetTCPKeepaliveShortLivedIdleTime() {
      return mTCPKeepaliveShortLivedIdleTimeS;
    }

    
    bool TCPKeepaliveEnabledForLongLivedConns() {
      return mTCPKeepaliveLongLivedEnabled;
    }
    
    
    int32_t GetTCPKeepaliveLongLivedIdleTime() {
      return mTCPKeepaliveLongLivedIdleTimeS;
    }

    
    
    FrameCheckLevel GetEnforceH1Framing() { return mEnforceH1Framing; }

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

    
    void UpdateAltServiceMapping(AltSvcMapping *map,
                                 nsProxyInfo *proxyInfo,
                                 nsIInterfaceRequestor *callbacks,
                                 uint32_t caps)
    {
        mConnMgr->UpdateAltServiceMapping(map, proxyInfo, callbacks, caps);
    }

    AltSvcMapping *GetAltServiceMapping(const nsACString &scheme,
                                        const nsACString &host,
                                        int32_t port, bool pb)
    {
        return mConnMgr->GetAltServiceMapping(scheme, host, port, pb);
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
                                     nsACString& hostLine);

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

    SpdyInformation *SpdyInfo() { return &mSpdyInfo; }
    bool IsH2MandatorySuiteEnabled() { return mH2MandatorySuiteEnabled; }

    
    bool Active() { return mHandlerActive; }

    
    
    TimeStamp GetCacheSkippedUntil() { return mCacheSkippedUntil; }
    void SetCacheSkippedUntil(TimeStamp arg) { mCacheSkippedUntil = arg; }
    void ClearCacheSkippedUntil() { mCacheSkippedUntil = TimeStamp(); }

private:
    virtual ~nsHttpHandler();

    
    
    
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
    uint8_t  mSpoofReferrerSource;
    uint8_t  mReferrerTrimmingPolicy;
    uint8_t  mReferrerXOriginPolicy;

    bool mFastFallbackToIPv4;
    bool mProxyPipelining;
    PRIntervalTime mIdleTimeout;
    PRIntervalTime mSpdyTimeout;
    PRIntervalTime mResponseTimeout;
    bool mResponseTimeoutEnabled;
    uint32_t mNetworkChangedTimeout; 
    uint16_t mMaxRequestAttempts;
    uint16_t mMaxRequestDelay;
    uint16_t mIdleSynTimeout;

    bool     mH2MandatorySuiteEnabled;
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
    nsCString      mDeviceModelId;

    nsCString      mUserAgent;
    nsXPIDLCString mUserAgentOverride;
    bool           mUserAgentIsDirty; 

    bool           mUseCache;

    bool           mPromptTempRedirect;
    
    
    bool           mSendSecureXSiteReferrer;

    
    bool           mEnablePersistentHttpsCaching;

    
    bool           mDoNotTrackEnabled;

    
    bool           mSafeHintEnabled;
    bool           mParentalControlEnabled;

    
    uint32_t           mTelemetryEnabled : 1;

    
    uint32_t           mAllowExperiments : 1;

    
    uint32_t           mHandlerActive : 1;

    uint32_t           mEnableSpdy : 1;
    uint32_t           mSpdyV31 : 1;
    uint32_t           mHttp2DraftEnabled : 1;
    uint32_t           mHttp2Enabled : 1;
    uint32_t           mUseH2Deps : 1;
    uint32_t           mEnforceHttp2TlsProfile : 1;
    uint32_t           mCoalesceSpdy : 1;
    uint32_t           mSpdyPersistentSettings : 1;
    uint32_t           mAllowPush : 1;
    uint32_t           mEnableAltSvc : 1;
    uint32_t           mEnableAltSvcOE : 1;

    
    SpdyInformation    mSpdyInfo;

    uint32_t       mSpdySendingChunkSize;
    uint32_t       mSpdySendBufferSize;
    uint32_t       mSpdyPushAllowance;
    uint32_t       mDefaultSpdyConcurrent;
    PRIntervalTime mSpdyPingThreshold;
    PRIntervalTime mSpdyPingTimeout;

    
    
    uint32_t       mConnectTimeout;

    
    
    uint32_t       mParallelSpeculativeConnectLimit;

    
    
    bool           mRequestTokenBucketEnabled;
    uint16_t       mRequestTokenBucketMinParallelism;
    uint32_t       mRequestTokenBucketHz;  
    uint32_t       mRequestTokenBucketBurst; 

    
    
    bool           mCriticalRequestPrioritization;

    
    
    TimeStamp      mCacheSkippedUntil;

    

    
    bool mTCPKeepaliveShortLivedEnabled;
    
    int32_t mTCPKeepaliveShortLivedTimeS;
    
    int32_t mTCPKeepaliveShortLivedIdleTimeS;

    
    bool mTCPKeepaliveLongLivedEnabled;
    
    int32_t mTCPKeepaliveLongLivedIdleTimeS;

    
    
    FrameCheckLevel mEnforceH1Framing;

private:
    
    
    void MakeNewRequestTokenBucket();
    nsRefPtr<EventTokenBucket> mRequestTokenBucket;

public:
    
    nsresult SubmitPacedRequest(ATokenBucketEvent *event,
                                nsICancelable **cancel)
    {
        if (!mRequestTokenBucket)
            return NS_ERROR_UNEXPECTED;
        return mRequestTokenBucket->SubmitEvent(event, cancel);
    }

    
    void SetRequestTokenBucket(EventTokenBucket *aTokenBucket)
    {
        mRequestTokenBucket = aTokenBucket;
    }

private:
    nsRefPtr<Tickler> mWifiTickler;
    void TickleWifi(nsIInterfaceRequestor *cb);
};

extern nsHttpHandler *gHttpHandler;






class nsHttpsHandler : public nsIHttpProtocolHandler
                     , public nsSupportsWeakReference
                     , public nsISpeculativeConnect
{
    virtual ~nsHttpsHandler() { }
public:
    
    

    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    NS_FORWARD_NSIPROXIEDPROTOCOLHANDLER (gHttpHandler->)
    NS_FORWARD_NSIHTTPPROTOCOLHANDLER    (gHttpHandler->)
    NS_FORWARD_NSISPECULATIVECONNECT     (gHttpHandler->)

    nsHttpsHandler() { }

    nsresult Init();
};

}} 

#endif 
