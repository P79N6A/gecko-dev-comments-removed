





#ifndef nsHttpChannel_h__
#define nsHttpChannel_h__

#include "HttpBaseChannel.h"

#include "nsHttpTransaction.h"
#include "nsInputStreamPump.h"
#include "nsThreadUtils.h"
#include "nsTArray.h"

#include "nsIHttpEventSink.h"
#include "nsICachingChannel.h"
#include "nsICacheEntryDescriptor.h"
#include "nsICacheListener.h"
#include "nsIApplicationCacheChannel.h"
#include "nsIPrompt.h"
#include "nsIResumableChannel.h"
#include "nsIProtocolProxyCallback.h"
#include "nsICancelable.h"
#include "nsIHttpAuthenticableChannel.h"
#include "nsIHttpChannelAuthProvider.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsITimedChannel.h"
#include "nsIFile.h"
#include "nsDNSPrefetch.h"
#include "TimingStruct.h"
#include "AutoClose.h"
#include "mozilla/Telemetry.h"

class nsAHttpConnection;

namespace mozilla { namespace net {

class HttpCacheQuery;





class nsHttpChannel : public HttpBaseChannel
                    , public HttpAsyncAborter<nsHttpChannel>
                    , public nsIStreamListener
                    , public nsICachingChannel
                    , public nsICacheListener
                    , public nsITransportEventSink
                    , public nsIProtocolProxyCallback
                    , public nsIHttpAuthenticableChannel
                    , public nsIApplicationCacheChannel
                    , public nsIAsyncVerifyRedirectCallback
                    , public nsITimedChannel
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSICACHEINFOCHANNEL
    NS_DECL_NSICACHINGCHANNEL
    NS_DECL_NSICACHELISTENER
    NS_DECL_NSITRANSPORTEVENTSINK
    NS_DECL_NSIPROTOCOLPROXYCALLBACK
    NS_DECL_NSIPROXIEDCHANNEL
    NS_DECL_NSIAPPLICATIONCACHECONTAINER
    NS_DECL_NSIAPPLICATIONCACHECHANNEL
    NS_DECL_NSIASYNCVERIFYREDIRECTCALLBACK
    NS_DECL_NSITIMEDCHANNEL

    
    
    
    NS_IMETHOD GetIsSSL(bool *aIsSSL);
    NS_IMETHOD GetProxyMethodIsConnect(bool *aProxyMethodIsConnect);
    NS_IMETHOD GetServerResponseHeader(nsACString & aServerResponseHeader);
    NS_IMETHOD GetProxyChallenges(nsACString & aChallenges);
    NS_IMETHOD GetWWWChallenges(nsACString & aChallenges);
    NS_IMETHOD SetProxyCredentials(const nsACString & aCredentials);
    NS_IMETHOD SetWWWCredentials(const nsACString & aCredentials);
    NS_IMETHOD OnAuthAvailable();
    NS_IMETHOD OnAuthCancelled(bool userCancel);
    
    
    
    NS_IMETHOD GetLoadFlags(nsLoadFlags *aLoadFlags);
    NS_IMETHOD GetURI(nsIURI **aURI);
    NS_IMETHOD GetNotificationCallbacks(nsIInterfaceRequestor **aCallbacks);
    NS_IMETHOD GetLoadGroup(nsILoadGroup **aLoadGroup);
    NS_IMETHOD GetRequestMethod(nsACString& aMethod);

    nsHttpChannel();
    virtual ~nsHttpChannel();

    virtual nsresult Init(nsIURI *aURI, uint32_t aCaps, nsProxyInfo *aProxyInfo,
                          uint32_t aProxyResolveFlags,
                          nsIURI *aProxyURI);

    
    
    
    NS_IMETHOD Cancel(nsresult status);
    NS_IMETHOD Suspend();
    NS_IMETHOD Resume();
    
    NS_IMETHOD GetSecurityInfo(nsISupports **aSecurityInfo);
    NS_IMETHOD AsyncOpen(nsIStreamListener *listener, nsISupports *aContext);
    
    NS_IMETHOD SetupFallbackChannel(const char *aFallbackKey);
    
    NS_IMETHOD SetPriority(int32_t value);
    
    NS_IMETHOD ResumeAt(uint64_t startPos, const nsACString& entityID);

    NS_IMETHOD SetNotificationCallbacks(nsIInterfaceRequestor *aCallbacks);
    NS_IMETHOD SetLoadGroup(nsILoadGroup *aLoadGroup);

public:  

    void InternalSetUploadStream(nsIInputStream *uploadStream) 
      { mUploadStream = uploadStream; }
    void SetUploadStreamHasHeaders(bool hasHeaders) 
      { mUploadStreamHasHeaders = hasHeaders; }

    nsresult SetReferrerInternal(nsIURI *referrer) {
        nsAutoCString spec;
        nsresult rv = referrer->GetAsciiSpec(spec);
        if (NS_FAILED(rv)) return rv;
        mReferrer = referrer;
        mRequestHead.SetHeader(nsHttp::Referer, spec);
        return NS_OK;
    }

    
    
    class OfflineCacheEntryAsForeignMarker {
        nsCOMPtr<nsIApplicationCache> mApplicationCache;
        nsCString mCacheKey;
    public:
        OfflineCacheEntryAsForeignMarker(nsIApplicationCache* appCache,
                                         const nsCSubstring& key)
             : mApplicationCache(appCache)
             , mCacheKey(key)
        {}

        nsresult MarkAsForeign();
    };

    OfflineCacheEntryAsForeignMarker* GetOfflineCacheEntryAsForeignMarker();

private:
    typedef nsresult (nsHttpChannel::*nsContinueRedirectionFunc)(nsresult result);

    bool     RequestIsConditional();
    nsresult BeginConnect();
    nsresult Connect();
    nsresult ContinueConnect();
    void     SpeculativeConnect();
    nsresult SetupTransaction();
    void     SetupTransactionLoadGroupInfo();
    nsresult CallOnStartRequest();
    nsresult ProcessResponse();
    nsresult ContinueProcessResponse(nsresult);
    nsresult ProcessNormal();
    nsresult ContinueProcessNormal(nsresult);
    nsresult ProcessNotModified();
    nsresult AsyncProcessRedirection(uint32_t httpStatus);
    nsresult ContinueProcessRedirection(nsresult);
    nsresult ContinueProcessRedirectionAfterFallback(nsresult);
    nsresult ProcessFailedProxyConnect(uint32_t httpStatus);
    nsresult ProcessFallback(bool *waitingForRedirectCallback);
    nsresult ContinueProcessFallback(nsresult);
    void     HandleAsyncAbort();
    nsresult EnsureAssocReq();

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
    nsresult StartRedirectChannelToURI(nsIURI *);
    virtual  nsresult SetupReplacementChannel(nsIURI *, nsIChannel *, bool preserveMethod);

    
    nsresult ProxyFailover();
    nsresult AsyncDoReplaceWithProxy(nsIProxyInfo *);
    nsresult ContinueDoReplaceWithProxy(nsresult);
    nsresult ResolveProxy();

    
    nsresult OpenCacheEntry(bool usingSSL);
    nsresult OnOfflineCacheEntryAvailable(nsICacheEntryDescriptor *aEntry,
                                          nsCacheAccessMode aAccess,
                                          nsresult aResult);
    nsresult OpenNormalCacheEntry(bool usingSSL);
    nsresult OnNormalCacheEntryAvailable(nsICacheEntryDescriptor *aEntry,
                                         nsCacheAccessMode aAccess,
                                         nsresult aResult);
    nsresult OpenOfflineCacheEntryForWriting();
    nsresult OnOfflineCacheEntryForWritingAvailable(
        nsICacheEntryDescriptor *aEntry,
        nsCacheAccessMode aAccess,
        nsresult aResult);
    nsresult OnCacheEntryAvailableInternal(nsICacheEntryDescriptor *entry,
                                           nsCacheAccessMode access,
                                           nsresult status);
    nsresult GenerateCacheKey(uint32_t postID, nsACString &key);
    nsresult UpdateExpirationTime();
    nsresult CheckCache();
    bool ShouldUpdateOfflineCacheEntry();
    nsresult ReadFromCache(bool alreadyMarkedValid);
    void     CloseCacheEntry(bool doomOnFailure);
    void     CloseOfflineCacheEntry();
    nsresult InitCacheEntry();
    void     UpdateInhibitPersistentCachingFlag();
    nsresult InitOfflineCacheEntry();
    nsresult AddCacheEntryHeaders(nsICacheEntryDescriptor *entry);
    nsresult StoreAuthorizationMetaData(nsICacheEntryDescriptor *entry);
    nsresult FinalizeCacheEntry();
    nsresult InstallCacheListener(uint32_t offset = 0);
    nsresult InstallOfflineCacheListener();
    void     MaybeInvalidateCacheEntryForSubsequentGet();
    nsCacheStoragePolicy DetermineStoragePolicy();
    nsresult DetermineCacheAccess(nsCacheAccessMode *_retval);
    void     AsyncOnExamineCachedResponse();

    
    void ClearBogusContentEncodingIfNeeded();

    
    nsresult ProcessPartialContent();
    nsresult OnDoneReadingPartialCacheEntry(bool *streamDone);

    nsresult DoAuthRetry(nsAHttpConnection *);

    void     HandleAsyncRedirectChannelToHttps();
    nsresult StartRedirectChannelToHttps();
    nsresult ContinueAsyncRedirectChannelToURI(nsresult rv);
    nsresult OpenRedirectChannel(nsresult rv);

    





    nsresult ProcessSTSHeader();

    void InvalidateCacheEntryForLocation(const char *location);
    void AssembleCacheKey(const char *spec, uint32_t postID, nsACString &key);
    nsresult CreateNewURI(const char *loc, nsIURI **newURI);
    void DoInvalidateCacheEntry(const nsCString &key);

    
    
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

private:
    nsCOMPtr<nsISupports>             mSecurityInfo;
    nsCOMPtr<nsICancelable>           mProxyRequest;

    nsRefPtr<nsInputStreamPump>       mTransactionPump;
    nsRefPtr<nsHttpTransaction>       mTransaction;

    uint64_t                          mLogicalOffset;

    
    nsRefPtr<HttpCacheQuery>          mCacheQuery;
    nsCOMPtr<nsICacheEntryDescriptor> mCacheEntry;
    
    AutoClose<nsIInputStream>         mCacheInputStream;
    nsRefPtr<nsInputStreamPump>       mCachePump;
    nsAutoPtr<nsHttpResponseHead>     mCachedResponseHead;
    nsCOMPtr<nsISupports>             mCachedSecurityInfo;
    nsCacheAccessMode                 mCacheAccess;
    mozilla::Telemetry::ID            mCacheEntryDeviceTelemetryID;
    uint32_t                          mPostID;
    uint32_t                          mRequestTime;

    typedef nsresult (nsHttpChannel:: *nsOnCacheEntryAvailableCallback)(
        nsICacheEntryDescriptor *, nsCacheAccessMode, nsresult);
    nsOnCacheEntryAvailableCallback   mOnCacheEntryAvailableCallback;

    nsCOMPtr<nsICacheEntryDescriptor> mOfflineCacheEntry;
    nsCacheAccessMode                 mOfflineCacheAccess;
    uint32_t                          mOfflineCacheLastModifiedTime;
    nsCOMPtr<nsIApplicationCache>     mApplicationCacheForWrite;

    
    nsCOMPtr<nsIHttpChannelAuthProvider> mAuthProvider;

    
    
    
    nsCString                         mFallbackKey;

    friend class AutoRedirectVetoNotifier;
    friend class HttpAsyncAborter<nsHttpChannel>;
    friend class HttpCacheQuery;

    nsCOMPtr<nsIURI>                  mRedirectURI;
    nsCOMPtr<nsIChannel>              mRedirectChannel;
    uint32_t                          mRedirectType;

    
    uint32_t                          mCachedContentIsValid     : 1;
    uint32_t                          mCachedContentIsPartial   : 1;
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

    nsTArray<nsContinueRedirectionFunc> mRedirectFuncStack;

    PRTime                            mChannelCreationTime;
    mozilla::TimeStamp                mChannelCreationTimestamp;
    mozilla::TimeStamp                mAsyncOpenTime;
    mozilla::TimeStamp                mCacheReadStart;
    mozilla::TimeStamp                mCacheReadEnd;
    
    
    TimingStruct                      mTransactionTimings;
    
    nsRefPtr<nsDNSPrefetch>           mDNSPrefetch;

    nsresult WaitForRedirectCallback();
    void PushRedirectAsyncFunc(nsContinueRedirectionFunc func);
    void PopRedirectAsyncFunc(nsContinueRedirectionFunc func);


    
    
    mozilla::TimeStamp                mCacheEffectExperimentAsyncOpenTime;
protected:
    virtual void DoNotifyListenerCleanup();

private: 
    bool mDidReval;
};

} } 

#endif 
