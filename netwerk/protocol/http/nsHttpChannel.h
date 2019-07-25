









































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
#include "nsITraceableChannel.h"
#include "nsIHttpChannelAuthProvider.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsICryptoHash.h"

class nsAHttpConnection;
class AutoRedirectVetoNotifier;

using namespace mozilla::net;





class nsHttpChannel : public HttpBaseChannel
                    , public nsIStreamListener
                    , public nsICachingChannel
                    , public nsICacheListener
                    , public nsITransportEventSink
                    , public nsIProtocolProxyCallback
                    , public nsIHttpAuthenticableChannel
                    , public nsITraceableChannel
                    , public nsIApplicationCacheChannel
                    , public nsIAsyncVerifyRedirectCallback
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
    NS_DECL_NSITRACEABLECHANNEL
    NS_DECL_NSIAPPLICATIONCACHECONTAINER
    NS_DECL_NSIAPPLICATIONCACHECHANNEL
    NS_DECL_NSIASYNCVERIFYREDIRECTCALLBACK

    
    
    
    NS_IMETHOD GetIsSSL(PRBool *aIsSSL);
    NS_IMETHOD GetProxyMethodIsConnect(PRBool *aProxyMethodIsConnect);
    NS_IMETHOD GetServerResponseHeader(nsACString & aServerResponseHeader);
    NS_IMETHOD GetProxyChallenges(nsACString & aChallenges);
    NS_IMETHOD GetWWWChallenges(nsACString & aChallenges);
    NS_IMETHOD SetProxyCredentials(const nsACString & aCredentials);
    NS_IMETHOD SetWWWCredentials(const nsACString & aCredentials);
    NS_IMETHOD OnAuthAvailable();
    NS_IMETHOD OnAuthCancelled(PRBool userCancel);
    
    
    
    NS_IMETHOD GetLoadFlags(nsLoadFlags *aLoadFlags);
    NS_IMETHOD GetURI(nsIURI **aURI);
    NS_IMETHOD GetNotificationCallbacks(nsIInterfaceRequestor **aCallbacks);
    NS_IMETHOD GetLoadGroup(nsILoadGroup **aLoadGroup);
    NS_IMETHOD GetRequestMethod(nsACString& aMethod);

    nsHttpChannel();
    virtual ~nsHttpChannel();

    virtual nsresult Init(nsIURI *aURI, PRUint8 aCaps, nsProxyInfo *aProxyInfo);

    
    
    
    NS_IMETHOD Cancel(nsresult status);
    NS_IMETHOD Suspend();
    NS_IMETHOD Resume();
    
    NS_IMETHOD GetSecurityInfo(nsISupports **aSecurityInfo);
    NS_IMETHOD AsyncOpen(nsIStreamListener *listener, nsISupports *aContext);
    
    NS_IMETHOD SetupFallbackChannel(const char *aFallbackKey);
    NS_IMETHOD GetLocalAddress(nsACString& addr);
    NS_IMETHOD GetLocalPort(PRInt32* port);
    NS_IMETHOD GetRemoteAddress(nsACString& addr);
    NS_IMETHOD GetRemotePort(PRInt32* port);
    
    NS_IMETHOD SetPriority(PRInt32 value);
    
    NS_IMETHOD ResumeAt(PRUint64 startPos, const nsACString& entityID);

public:  
    typedef void (nsHttpChannel:: *nsAsyncCallback)(void);

    void InternalSetUploadStream(nsIInputStream *uploadStream) 
      { mUploadStream = uploadStream; }
    void SetUploadStreamHasHeaders(PRBool hasHeaders) 
      { mUploadStreamHasHeaders = hasHeaders; }

    nsresult SetReferrerInternal(nsIURI *referrer) {
        nsCAutoString spec;
        nsresult rv = referrer->GetAsciiSpec(spec);
        if (NS_FAILED(rv)) return rv;
        mReferrer = referrer;
        mRequestHead.SetHeader(nsHttp::Referer, spec);
        return NS_OK;
    }

private:
    typedef nsresult (nsHttpChannel::*nsContinueRedirectionFunc)(nsresult result);

    
    
    
    nsresult AsyncCall(nsAsyncCallback funcPtr,
                       nsRunnableMethod<nsHttpChannel> **retval = nsnull);

    PRBool   RequestIsConditional();
    nsresult Connect(PRBool firstTime = PR_TRUE);
    nsresult AsyncAbort(nsresult status);
    
    void     HandleAsyncNotifyListener();
    void     DoNotifyListener();
    nsresult SetupTransaction();
    nsresult CallOnStartRequest();
    nsresult ProcessResponse();
    nsresult ContinueProcessResponse(nsresult);
    nsresult ProcessNormal();
    nsresult ContinueProcessNormal(nsresult);
    nsresult ProcessNotModified();
    nsresult AsyncProcessRedirection(PRUint32 httpStatus);
    nsresult ContinueProcessRedirection(nsresult);
    nsresult ContinueProcessRedirectionAfterFallback(nsresult);
    PRBool   ShouldSSLProxyResponseContinue(PRUint32 httpStatus);
    nsresult ProcessFailedSSLConnect(PRUint32 httpStatus);
    nsresult ProcessFallback(PRBool *waitingForRedirectCallback);
    nsresult ContinueProcessFallback(nsresult);
    PRBool   ResponseWouldVary();

    nsresult ContinueOnStartRequest1(nsresult);
    nsresult ContinueOnStartRequest2(nsresult);
    nsresult ContinueOnStartRequest3(nsresult);

    
    void     HandleAsyncRedirect();
    nsresult ContinueHandleAsyncRedirect(nsresult);
    void     HandleAsyncNotModified();
    void     HandleAsyncFallback();
    nsresult ContinueHandleAsyncFallback(nsresult);
    nsresult PromptTempRedirect();
    virtual nsresult SetupReplacementChannel(nsIURI *, nsIChannel *, PRBool preserveMethod);

    
    nsresult ProxyFailover();
    nsresult AsyncDoReplaceWithProxy(nsIProxyInfo *);
    nsresult ContinueDoReplaceWithProxy(nsresult);
    void HandleAsyncReplaceWithProxy();
    nsresult ContinueHandleAsyncReplaceWithProxy(nsresult);
    nsresult ResolveProxy();

    
    nsresult OpenCacheEntry();
    nsresult OnOfflineCacheEntryAvailable(nsICacheEntryDescriptor *aEntry,
                                          nsCacheAccessMode aAccess,
                                          nsresult aResult,
                                          PRBool aSync);
    nsresult OpenNormalCacheEntry(PRBool aSync);
    nsresult OnNormalCacheEntryAvailable(nsICacheEntryDescriptor *aEntry,
                                         nsCacheAccessMode aAccess,
                                         nsresult aResult,
                                         PRBool aSync);
    nsresult OpenOfflineCacheEntryForWriting();
    nsresult GenerateCacheKey(PRUint32 postID, nsACString &key);
    nsresult UpdateExpirationTime();
    nsresult CheckCache();
    nsresult ShouldUpdateOfflineCacheEntry(PRBool *shouldCacheForOfflineUse);
    nsresult ReadFromCache();
    void     CloseCacheEntry(PRBool doomOnFailure);
    void     CloseOfflineCacheEntry();
    nsresult InitCacheEntry();
    nsresult InitOfflineCacheEntry();
    nsresult AddCacheEntryHeaders(nsICacheEntryDescriptor *entry);
    nsresult StoreAuthorizationMetaData(nsICacheEntryDescriptor *entry);
    nsresult FinalizeCacheEntry();
    nsresult InstallCacheListener(PRUint32 offset = 0);
    nsresult InstallOfflineCacheListener();
    void     MaybeInvalidateCacheEntryForSubsequentGet();
    nsCacheStoragePolicy DetermineStoragePolicy();
    nsresult DetermineCacheAccess(nsCacheAccessMode *_retval);
    void     AsyncOnExamineCachedResponse();

    
    void ClearBogusContentEncodingIfNeeded();

    
    nsresult SetupByteRangeRequest(PRUint32 partialLen);
    nsresult ProcessPartialContent();
    nsresult OnDoneReadingPartialCacheEntry(PRBool *streamDone);

    nsresult DoAuthRetry(nsAHttpConnection *);
    PRBool   MustValidateBasedOnQueryUrl();

    void     HandleAsyncRedirectChannelToHttps();
    nsresult AsyncRedirectChannelToHttps();
    nsresult ContinueAsyncRedirectChannelToHttps(nsresult rv);

    





    nsresult ProcessSTSHeader();

    



    nsresult Hash(const char *buf, nsACString &hash);

private:
    nsCOMPtr<nsISupports>             mSecurityInfo;
    nsCOMPtr<nsICancelable>           mProxyRequest;

    nsRefPtr<nsInputStreamPump>       mTransactionPump;
    nsRefPtr<nsHttpTransaction>       mTransaction;

    PRUint64                          mLogicalOffset;

    
    nsCOMPtr<nsICacheEntryDescriptor> mCacheEntry;
    nsRefPtr<nsInputStreamPump>       mCachePump;
    nsAutoPtr<nsHttpResponseHead>     mCachedResponseHead;
    nsCacheAccessMode                 mCacheAccess;
    PRUint32                          mPostID;
    PRUint32                          mRequestTime;

    typedef nsresult (nsHttpChannel:: *nsOnCacheEntryAvailableCallback)(
        nsICacheEntryDescriptor *, nsCacheAccessMode, nsresult, PRBool);
    nsOnCacheEntryAvailableCallback   mOnCacheEntryAvailableCallback;
    PRBool                            mAsyncCacheOpen;

    nsCOMPtr<nsICacheEntryDescriptor> mOfflineCacheEntry;
    nsCacheAccessMode                 mOfflineCacheAccess;
    nsCString                         mOfflineCacheClientID;

    
    nsCOMPtr<nsIHttpChannelAuthProvider> mAuthProvider;

    
    
    
    nsAsyncCallback                   mPendingAsyncCallOnResume;

    
    nsCOMPtr<nsIProxyInfo>            mTargetProxyInfo;

    
    
    PRUint32                          mSuspendCount;

    
    
    
    nsCString                         mFallbackKey;

    friend class AutoRedirectVetoNotifier;
    nsCOMPtr<nsIURI>                  mRedirectURI;
    nsCOMPtr<nsIChannel>              mRedirectChannel;
    PRUint32                          mRedirectType;

    
    PRUint32                          mCachedContentIsValid     : 1;
    PRUint32                          mCachedContentIsPartial   : 1;
    PRUint32                          mTransactionReplaced      : 1;
    PRUint32                          mAuthRetryPending         : 1;
    PRUint32                          mResuming                 : 1;
    PRUint32                          mInitedCacheEntry         : 1;
    PRUint32                          mCacheForOfflineUse       : 1;
    
    
    PRUint32                          mCachingOpportunistically : 1;
    
    
    PRUint32                          mFallbackChannel          : 1;
    PRUint32                          mTracingEnabled           : 1;
    
    
    
    PRUint32                          mCustomConditionalRequest : 1;
    PRUint32                          mFallingBack              : 1;
    PRUint32                          mWaitingForRedirectCallback : 1;
    
    
    PRUint32                          mRequestTimeInitialized : 1;

    PRNetAddr                         mSelfAddr;
    PRNetAddr                         mPeerAddr;

    nsTArray<nsContinueRedirectionFunc> mRedirectFuncStack;

    nsCOMPtr<nsICryptoHash>        mHasher;

    nsresult WaitForRedirectCallback();
    void PushRedirectAsyncFunc(nsContinueRedirectionFunc func);
    void PopRedirectAsyncFunc(nsContinueRedirectionFunc func);
};

#endif 
