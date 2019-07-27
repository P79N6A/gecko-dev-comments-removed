





#ifndef nsHttpChannel_h__
#define nsHttpChannel_h__

#include "HttpBaseChannel.h"
#include "nsTArray.h"
#include "nsICachingChannel.h"
#include "nsICacheEntry.h"
#include "nsICacheEntryOpenCallback.h"
#include "nsIDNSListener.h"
#include "nsIApplicationCacheChannel.h"
#include "nsIProtocolProxyCallback.h"
#include "nsIHttpAuthenticableChannel.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsIThreadRetargetableRequest.h"
#include "nsIThreadRetargetableStreamListener.h"
#include "nsWeakReference.h"
#include "TimingStruct.h"
#include "AutoClose.h"

class nsIPrincipal;
class nsDNSPrefetch;
class nsICancelable;
class nsIHttpChannelAuthProvider;
class nsInputStreamPump;
class nsISSLStatus;

namespace mozilla { namespace net {

class Http2PushedStream;

class HttpChannelSecurityWarningReporter
{
public:
  virtual nsresult ReportSecurityMessage(const nsAString& aMessageTag,
                                         const nsAString& aMessageCategory) = 0;
};






#define NS_HTTPCHANNEL_IID                         \
{                                                  \
  0x301bf95b,                                      \
  0x7bb3,                                          \
  0x4ae1,                                          \
  {0xa9, 0x71, 0x40, 0xbc, 0xfa, 0x81, 0xde, 0x12} \
}

class nsHttpChannel final : public HttpBaseChannel
                          , public HttpAsyncAborter<nsHttpChannel>
                          , public nsIStreamListener
                          , public nsICachingChannel
                          , public nsICacheEntryOpenCallback
                          , public nsITransportEventSink
                          , public nsIProtocolProxyCallback
                          , public nsIHttpAuthenticableChannel
                          , public nsIApplicationCacheChannel
                          , public nsIAsyncVerifyRedirectCallback
                          , public nsIThreadRetargetableRequest
                          , public nsIThreadRetargetableStreamListener
                          , public nsIDNSListener
                          , public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSITHREADRETARGETABLESTREAMLISTENER
    NS_DECL_NSICACHEINFOCHANNEL
    NS_DECL_NSICACHINGCHANNEL
    NS_DECL_NSICACHEENTRYOPENCALLBACK
    NS_DECL_NSITRANSPORTEVENTSINK
    NS_DECL_NSIPROTOCOLPROXYCALLBACK
    NS_DECL_NSIPROXIEDCHANNEL
    NS_DECL_NSIAPPLICATIONCACHECONTAINER
    NS_DECL_NSIAPPLICATIONCACHECHANNEL
    NS_DECL_NSIASYNCVERIFYREDIRECTCALLBACK
    NS_DECL_NSITHREADRETARGETABLEREQUEST
    NS_DECL_NSIDNSLISTENER
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_HTTPCHANNEL_IID)

    
    
    
    NS_IMETHOD GetIsSSL(bool *aIsSSL) override;
    NS_IMETHOD GetProxyMethodIsConnect(bool *aProxyMethodIsConnect) override;
    NS_IMETHOD GetServerResponseHeader(nsACString & aServerResponseHeader) override;
    NS_IMETHOD GetProxyChallenges(nsACString & aChallenges) override;
    NS_IMETHOD GetWWWChallenges(nsACString & aChallenges) override;
    NS_IMETHOD SetProxyCredentials(const nsACString & aCredentials) override;
    NS_IMETHOD SetWWWCredentials(const nsACString & aCredentials) override;
    NS_IMETHOD OnAuthAvailable() override;
    NS_IMETHOD OnAuthCancelled(bool userCancel) override;
    
    
    
    NS_IMETHOD GetLoadFlags(nsLoadFlags *aLoadFlags) override;
    NS_IMETHOD GetURI(nsIURI **aURI) override;
    NS_IMETHOD GetNotificationCallbacks(nsIInterfaceRequestor **aCallbacks) override;
    NS_IMETHOD GetLoadGroup(nsILoadGroup **aLoadGroup) override;
    NS_IMETHOD GetRequestMethod(nsACString& aMethod) override;

    nsHttpChannel();

    virtual nsresult Init(nsIURI *aURI, uint32_t aCaps, nsProxyInfo *aProxyInfo,
                          uint32_t aProxyResolveFlags,
                          nsIURI *aProxyURI) override;

    nsresult OnPush(const nsACString &uri, Http2PushedStream *pushedStream);

    
    
    
    NS_IMETHOD Cancel(nsresult status) override;
    NS_IMETHOD Suspend() override;
    NS_IMETHOD Resume() override;
    
    NS_IMETHOD GetSecurityInfo(nsISupports **aSecurityInfo) override;
    NS_IMETHOD AsyncOpen(nsIStreamListener *listener, nsISupports *aContext) override;
    
    NS_IMETHOD SetupFallbackChannel(const char *aFallbackKey) override;
    NS_IMETHOD ContinueBeginConnect() override;
    
    NS_IMETHOD SetPriority(int32_t value) override;
    
    NS_IMETHOD SetClassFlags(uint32_t inFlags) override;
    NS_IMETHOD AddClassFlags(uint32_t inFlags) override;
    NS_IMETHOD ClearClassFlags(uint32_t inFlags) override;

    
    NS_IMETHOD ResumeAt(uint64_t startPos, const nsACString& entityID) override;

    NS_IMETHOD SetNotificationCallbacks(nsIInterfaceRequestor *aCallbacks) override;
    NS_IMETHOD SetLoadGroup(nsILoadGroup *aLoadGroup) override;
    
    NS_IMETHOD GetDomainLookupStart(mozilla::TimeStamp *aDomainLookupStart) override;
    NS_IMETHOD GetDomainLookupEnd(mozilla::TimeStamp *aDomainLookupEnd) override;
    NS_IMETHOD GetConnectStart(mozilla::TimeStamp *aConnectStart) override;
    NS_IMETHOD GetConnectEnd(mozilla::TimeStamp *aConnectEnd) override;
    NS_IMETHOD GetRequestStart(mozilla::TimeStamp *aRequestStart) override;
    NS_IMETHOD GetResponseStart(mozilla::TimeStamp *aResponseStart) override;
    NS_IMETHOD GetResponseEnd(mozilla::TimeStamp *aResponseEnd) override;

    nsresult AddSecurityMessage(const nsAString& aMessageTag,
                                const nsAString& aMessageCategory) override;

    void SetWarningReporter(HttpChannelSecurityWarningReporter* aReporter)
      { mWarningReporter = aReporter; }

public: 

    void InternalSetUploadStream(nsIInputStream *uploadStream)
      { mUploadStream = uploadStream; }
    void SetUploadStreamHasHeaders(bool hasHeaders)
      { mUploadStreamHasHeaders = hasHeaders; }

    nsresult SetReferrerWithPolicyInternal(nsIURI *referrer,
                                           uint32_t referrerPolicy) {
        nsAutoCString spec;
        nsresult rv = referrer->GetAsciiSpec(spec);
        if (NS_FAILED(rv)) return rv;
        mReferrer = referrer;
        mReferrerPolicy = referrerPolicy;
        mRequestHead.SetHeader(nsHttp::Referer, spec);
        return NS_OK;
    }

    nsresult SetTopWindowURI(nsIURI* aTopWindowURI) {
        mTopWindowURI = aTopWindowURI;
        return NS_OK;
    }

    nsresult OpenCacheEntry(bool usingSSL);
    nsresult ContinueConnect();

    nsresult StartRedirectChannelToURI(nsIURI *, uint32_t);

    
    
    class OfflineCacheEntryAsForeignMarker {
        nsCOMPtr<nsIApplicationCache> mApplicationCache;
        nsCOMPtr<nsIURI> mCacheURI;
    public:
        OfflineCacheEntryAsForeignMarker(nsIApplicationCache* appCache,
                                         nsIURI* aURI)
             : mApplicationCache(appCache)
             , mCacheURI(aURI)
        {}

        nsresult MarkAsForeign();
    };

    OfflineCacheEntryAsForeignMarker* GetOfflineCacheEntryAsForeignMarker();

    
    class AutoCacheWaitFlags
    {
    public:
      explicit AutoCacheWaitFlags(nsHttpChannel* channel)
        : mChannel(channel)
        , mKeep(0)
      {
        
        mChannel->mCacheEntriesToWaitFor =
          nsHttpChannel::WAIT_FOR_CACHE_ENTRY |
          nsHttpChannel::WAIT_FOR_OFFLINE_CACHE_ENTRY;
      }

      void Keep(uint32_t flags)
      {
        
        mKeep |= flags;
      }

      ~AutoCacheWaitFlags()
      {
        
        mChannel->mCacheEntriesToWaitFor &= mKeep;
      }

    private:
      nsHttpChannel* mChannel;
      uint32_t mKeep : 2;
    };

    void MarkIntercepted();
    bool AwaitingCacheCallbacks();

protected:
    virtual ~nsHttpChannel();

private:
    typedef nsresult (nsHttpChannel::*nsContinueRedirectionFunc)(nsresult result);

    bool     RequestIsConditional();
    nsresult BeginConnect();
    nsresult Connect();
    void     SpeculativeConnect();
    nsresult SetupTransaction();
    void     SetupTransactionLoadGroupInfo();
    nsresult CallOnStartRequest();
    nsresult ProcessResponse();
    nsresult ContinueProcessResponse(nsresult);
    nsresult ProcessNormal();
    nsresult ContinueProcessNormal(nsresult);
    void     ProcessAltService();
    nsresult ProcessNotModified();
    nsresult AsyncProcessRedirection(uint32_t httpStatus);
    nsresult ContinueProcessRedirection(nsresult);
    nsresult ContinueProcessRedirectionAfterFallback(nsresult);
    nsresult ProcessFailedProxyConnect(uint32_t httpStatus);
    nsresult ProcessFallback(bool *waitingForRedirectCallback);
    nsresult ContinueProcessFallback(nsresult);
    void     HandleAsyncAbort();
    nsresult EnsureAssocReq();
    void     ProcessSSLInformation();
    bool     IsHTTPS();

    nsresult ContinueOnStartRequest1(nsresult);
    nsresult ContinueOnStartRequest2(nsresult);
    nsresult ContinueOnStartRequest3(nsresult);

    
    void     HandleAsyncRedirect();
    void     HandleAsyncAPIRedirect();
    nsresult ContinueHandleAsyncRedirect(nsresult);
    void     HandleAsyncNotModified();
    void     HandleAsyncFallback();
    nsresult ContinueHandleAsyncFallback(nsresult);
    nsresult PromptTempRedirect();
    virtual  nsresult SetupReplacementChannel(nsIURI *, nsIChannel *, bool preserveMethod) override;

    
    nsresult ProxyFailover();
    nsresult AsyncDoReplaceWithProxy(nsIProxyInfo *);
    nsresult ContinueDoReplaceWithProxy(nsresult);
    nsresult ResolveProxy();

    
    nsresult OnOfflineCacheEntryAvailable(nsICacheEntry *aEntry,
                                          bool aNew,
                                          nsIApplicationCache* aAppCache,
                                          nsresult aResult);
    nsresult OnNormalCacheEntryAvailable(nsICacheEntry *aEntry,
                                         bool aNew,
                                         nsresult aResult);
    nsresult OpenOfflineCacheEntryForWriting();
    nsresult OnOfflineCacheEntryForWritingAvailable(nsICacheEntry *aEntry,
                                                    nsIApplicationCache* aAppCache,
                                                    nsresult aResult);
    nsresult OnCacheEntryAvailableInternal(nsICacheEntry *entry,
                                      bool aNew,
                                      nsIApplicationCache* aAppCache,
                                      nsresult status);
    nsresult GenerateCacheKey(uint32_t postID, nsACString &key);
    nsresult UpdateExpirationTime();
    nsresult CheckPartial(nsICacheEntry* aEntry, int64_t *aSize, int64_t *aContentLength);
    bool ShouldUpdateOfflineCacheEntry();
    nsresult ReadFromCache(bool alreadyMarkedValid);
    void     CloseCacheEntry(bool doomOnFailure);
    void     CloseOfflineCacheEntry();
    nsresult InitCacheEntry();
    void     UpdateInhibitPersistentCachingFlag();
    nsresult InitOfflineCacheEntry();
    nsresult AddCacheEntryHeaders(nsICacheEntry *entry);
    nsresult FinalizeCacheEntry();
    nsresult InstallCacheListener(int64_t offset = 0);
    nsresult InstallOfflineCacheListener(int64_t offset = 0);
    void     MaybeInvalidateCacheEntryForSubsequentGet();
    void     AsyncOnExamineCachedResponse();

    
    void ClearBogusContentEncodingIfNeeded();

    
    nsresult ProcessPartialContent();
    nsresult OnDoneReadingPartialCacheEntry(bool *streamDone);

    nsresult DoAuthRetry(nsAHttpConnection *);

    void     HandleAsyncRedirectChannelToHttps();
    nsresult StartRedirectChannelToHttps();
    nsresult ContinueAsyncRedirectChannelToURI(nsresult rv);
    nsresult OpenRedirectChannel(nsresult rv);

    





    nsresult ProcessSecurityHeaders();

    




    nsresult ProcessSingleSecurityHeader(uint32_t aType,
                                         nsISSLStatus *aSSLStatus,
                                         uint32_t aFlags);

    void InvalidateCacheEntryForLocation(const char *location);
    void AssembleCacheKey(const char *spec, uint32_t postID, nsACString &key);
    nsresult CreateNewURI(const char *loc, nsIURI **newURI);
    void DoInvalidateCacheEntry(nsIURI* aURI);

    
    
    inline bool HostPartIsTheSame(nsIURI *uri) {
        nsAutoCString tmpHost1, tmpHost2;
        return (NS_SUCCEEDED(mURI->GetAsciiHost(tmpHost1)) &&
                NS_SUCCEEDED(uri->GetAsciiHost(tmpHost2)) &&
                (tmpHost1 == tmpHost2));
    }

    inline static bool DoNotRender3xxBody(nsresult rv) {
        return rv == NS_ERROR_REDIRECT_LOOP         ||
               rv == NS_ERROR_CORRUPTED_CONTENT     ||
               rv == NS_ERROR_UNKNOWN_PROTOCOL      ||
               rv == NS_ERROR_MALFORMED_URI;
    }

    
    
    void UpdateAggregateCallbacks();

    static bool HasQueryString(nsHttpRequestHead::ParsedMethodType method, nsIURI * uri);
    bool ResponseWouldVary(nsICacheEntry* entry) const;
    bool MustValidateBasedOnQueryUrl() const;
    bool IsResumable(int64_t partialLen, int64_t contentLength,
                     bool ignoreMissingPartialLen = false) const;
    nsresult MaybeSetupByteRangeRequest(int64_t partialLen, int64_t contentLength,
                                        bool ignoreMissingPartialLen = false);
    nsresult SetupByteRangeRequest(int64_t partialLen);
    void UntieByteRangeRequest();
    void UntieValidationRequest();
    nsresult OpenCacheInputStream(nsICacheEntry* cacheEntry, bool startBuffering,
                                  bool checkingAppCacheEntry);

    void SetPushedStream(Http2PushedStream *stream);

private:
    nsCOMPtr<nsICancelable>           mProxyRequest;

    nsRefPtr<nsInputStreamPump>       mTransactionPump;
    nsRefPtr<nsHttpTransaction>       mTransaction;

    uint64_t                          mLogicalOffset;

    
    nsCOMPtr<nsICacheEntry>           mCacheEntry;
    
    AutoClose<nsIInputStream>         mCacheInputStream;
    nsRefPtr<nsInputStreamPump>       mCachePump;
    nsAutoPtr<nsHttpResponseHead>     mCachedResponseHead;
    nsCOMPtr<nsISupports>             mCachedSecurityInfo;
    uint32_t                          mPostID;
    uint32_t                          mRequestTime;

    nsCOMPtr<nsICacheEntry> mOfflineCacheEntry;
    uint32_t                          mOfflineCacheLastModifiedTime;
    nsCOMPtr<nsIApplicationCache>     mApplicationCacheForWrite;

    
    nsCOMPtr<nsIHttpChannelAuthProvider> mAuthProvider;

    
    enum {
        DO_NOT_INTERCEPT,  
        MAYBE_INTERCEPT,   
        INTERCEPTED,       
    } mInterceptCache;

    bool PossiblyIntercepted() {
        return mInterceptCache != DO_NOT_INTERCEPT;
    }

    
    
    
    nsCString                         mFallbackKey;

    friend class AutoRedirectVetoNotifier;
    friend class HttpAsyncAborter<nsHttpChannel>;

    nsCOMPtr<nsIURI>                  mRedirectURI;
    nsCOMPtr<nsIChannel>              mRedirectChannel;
    uint32_t                          mRedirectType;

    static const uint32_t WAIT_FOR_CACHE_ENTRY = 1;
    static const uint32_t WAIT_FOR_OFFLINE_CACHE_ENTRY = 2;

    
    uint32_t                          mCachedContentIsValid     : 1;
    uint32_t                          mCachedContentIsPartial   : 1;
    uint32_t                          mCacheOnlyMetadata        : 1;
    uint32_t                          mTransactionReplaced      : 1;
    uint32_t                          mAuthRetryPending         : 1;
    uint32_t                          mProxyAuthPending         : 1;
    uint32_t                          mResuming                 : 1;
    uint32_t                          mInitedCacheEntry         : 1;
    
    
    uint32_t                          mFallbackChannel          : 1;
    
    
    
    uint32_t                          mCustomConditionalRequest : 1;
    uint32_t                          mFallingBack              : 1;
    uint32_t                          mWaitingForRedirectCallback : 1;
    
    
    uint32_t                          mRequestTimeInitialized : 1;
    uint32_t                          mCacheEntryIsReadOnly : 1;
    uint32_t                          mCacheEntryIsWriteOnly : 1;
    
    uint32_t                          mCacheEntriesToWaitFor : 2;
    uint32_t                          mHasQueryString : 1;
    
    
    
    
    uint32_t                          mConcurentCacheAccess : 1;
    
    uint32_t                          mIsPartialRequest : 1;
    
    uint32_t                          mHasAutoRedirectVetoNotifier : 1;

    nsTArray<nsContinueRedirectionFunc> mRedirectFuncStack;

    
    nsRefPtr<nsDNSPrefetch>           mDNSPrefetch;

    Http2PushedStream                 *mPushedStream;
    
    
    bool                              mLocalBlocklist;

    nsresult WaitForRedirectCallback();
    void PushRedirectAsyncFunc(nsContinueRedirectionFunc func);
    void PopRedirectAsyncFunc(nsContinueRedirectionFunc func);

    nsCString mUsername;

    
    HttpChannelSecurityWarningReporter* mWarningReporter;

protected:
    virtual void DoNotifyListenerCleanup() override;

private: 
    bool mDidReval;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsHttpChannel, NS_HTTPCHANNEL_IID)
} } 

#endif 
