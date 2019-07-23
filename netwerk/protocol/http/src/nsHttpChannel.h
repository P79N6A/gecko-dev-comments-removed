







































#ifndef nsHttpChannel_h__
#define nsHttpChannel_h__

#include "nsHttpTransaction.h"
#include "nsHttpRequestHead.h"
#include "nsHttpAuthCache.h"
#include "nsHashPropertyBag.h"
#include "nsInputStreamPump.h"
#include "nsThreadUtils.h"
#include "nsString.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsInt64.h"

#include "nsIHttpChannel.h"
#include "nsIHttpChannelInternal.h"
#include "nsIHttpHeaderVisitor.h"
#include "nsIHttpEventSink.h"
#include "nsIChannelEventSink.h"
#include "nsIStreamListener.h"
#include "nsIIOService.h"
#include "nsIURI.h"
#include "nsILoadGroup.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIInputStream.h"
#include "nsIProgressEventSink.h"
#include "nsICachingChannel.h"
#include "nsICacheEntryDescriptor.h"
#include "nsICacheListener.h"
#include "nsIEncodedChannel.h"
#include "nsITransport.h"
#include "nsIUploadChannel.h"
#include "nsIStringEnumerator.h"
#include "nsIOutputStream.h"
#include "nsIAsyncInputStream.h"
#include "nsIPrompt.h"
#include "nsIResumableChannel.h"
#include "nsISupportsPriority.h"
#include "nsIProtocolProxyCallback.h"
#include "nsICancelable.h"
#include "nsIProxiedChannel.h"

class nsHttpResponseHead;
class nsAHttpConnection;
class nsIHttpAuthenticator;
class nsProxyInfo;





class nsHttpChannel : public nsHashPropertyBag
                    , public nsIHttpChannel
                    , public nsIHttpChannelInternal
                    , public nsIStreamListener
                    , public nsICachingChannel
                    , public nsIUploadChannel
                    , public nsICacheListener
                    , public nsIEncodedChannel
                    , public nsITransportEventSink
                    , public nsIResumableChannel
                    , public nsISupportsPriority
                    , public nsIProtocolProxyCallback
                    , public nsIProxiedChannel
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIREQUEST
    NS_DECL_NSICHANNEL
    NS_DECL_NSIHTTPCHANNEL
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSICACHINGCHANNEL
    NS_DECL_NSIUPLOADCHANNEL
    NS_DECL_NSICACHELISTENER
    NS_DECL_NSIENCODEDCHANNEL
    NS_DECL_NSIHTTPCHANNELINTERNAL
    NS_DECL_NSITRANSPORTEVENTSINK
    NS_DECL_NSIRESUMABLECHANNEL
    NS_DECL_NSISUPPORTSPRIORITY
    NS_DECL_NSIPROTOCOLPROXYCALLBACK
    NS_DECL_NSIPROXIEDCHANNEL

    nsHttpChannel();
    virtual ~nsHttpChannel();

    nsresult Init(nsIURI *uri,
                  PRUint8 capabilities,
                  nsProxyInfo* proxyInfo);

public:  
    typedef void (nsHttpChannel:: *nsAsyncCallback)(void);

private:

    
    template <class T>
    void GetCallback(nsCOMPtr<T> &aResult)
    {
        NS_QueryNotificationCallbacks(mCallbacks, mLoadGroup,
                                      NS_GET_TEMPLATE_IID(T),
                                      getter_AddRefs(aResult));
    }

    
    nsresult AsyncCall(nsAsyncCallback funcPtr);

    PRBool   RequestIsConditional();
    nsresult Connect(PRBool firstTime = PR_TRUE);
    nsresult AsyncAbort(nsresult status);
    nsresult SetupTransaction();
    void     AddCookiesToRequest();
    nsresult ApplyContentConversions();
    nsresult CallOnStartRequest();
    nsresult ProcessResponse();
    nsresult ProcessNormal();
    nsresult ProcessNotModified();
    nsresult ProcessRedirection(PRUint32 httpStatus);
    nsresult ProcessAuthentication(PRUint32 httpStatus);
    PRBool   ResponseWouldVary();

    
    void     HandleAsyncRedirect();
    void     HandleAsyncNotModified();
    nsresult PromptTempRedirect();
    nsresult SetupReplacementChannel(nsIURI *, nsIChannel *, PRBool preserveMethod);

    
    nsresult ProxyFailover();
    nsresult ReplaceWithProxy(nsIProxyInfo *);
    nsresult ResolveProxy();

    
    nsresult OpenCacheEntry(PRBool offline, PRBool *delayed);
    nsresult OpenOfflineCacheEntryForWriting();
    nsresult GenerateCacheKey(nsACString &key);
    nsresult UpdateExpirationTime();
    nsresult CheckCache();
    nsresult ShouldUpdateOfflineCacheEntry(PRBool *shouldCacheForOfflineUse);
    nsresult ReadFromCache();
    void     CloseCacheEntry();
    void     CloseOfflineCacheEntry();
    nsresult InitCacheEntry();
    nsresult InitOfflineCacheEntry();
    nsresult AddCacheEntryHeaders(nsICacheEntryDescriptor *entry);
    nsresult StoreAuthorizationMetaData(nsICacheEntryDescriptor *entry);
    nsresult FinalizeCacheEntry();
    nsresult InstallCacheListener(PRUint32 offset = 0);
    nsresult InstallOfflineCacheListener();

    
    nsresult SetupByteRangeRequest(PRUint32 partialLen);
    nsresult ProcessPartialContent();
    nsresult OnDoneReadingPartialCacheEntry(PRBool *streamDone);

    
    nsresult PrepareForAuthentication(PRBool proxyAuth);
    nsresult GenCredsAndSetEntry(nsIHttpAuthenticator *, PRBool proxyAuth, const char *scheme, const char *host, PRInt32 port, const char *dir, const char *realm, const char *challenge, const nsHttpAuthIdentity &ident, nsCOMPtr<nsISupports> &session, char **result);
    nsresult GetCredentials(const char *challenges, PRBool proxyAuth, nsAFlatCString &creds);
    nsresult GetCredentialsForChallenge(const char *challenge, const char *scheme,  PRBool proxyAuth, nsIHttpAuthenticator *auth, nsAFlatCString &creds);
    nsresult GetAuthenticator(const char *challenge, nsCString &scheme, nsIHttpAuthenticator **auth); 
    void     ParseRealm(const char *challenge, nsACString &realm);
    void     GetIdentityFromURI(PRUint32 authFlags, nsHttpAuthIdentity&);
    nsresult PromptForIdentity(PRUint32 level, PRBool proxyAuth, const char *realm, const char *authType, PRUint32 authFlags, nsHttpAuthIdentity &);
    PRBool   ConfirmAuth(const nsString &bundleKey, PRBool doYesNoPrompt);
    void     CheckForSuperfluousAuth();
    void     SetAuthorizationHeader(nsHttpAuthCache *, nsHttpAtom header, const char *scheme, const char *host, PRInt32 port, const char *path, nsHttpAuthIdentity &ident);
    void     AddAuthorizationHeaders();
    nsresult GetCurrentPath(nsACString &);
    nsresult DoAuthRetry(nsAHttpConnection *);

private:
    nsCOMPtr<nsIURI>                  mOriginalURI;
    nsCOMPtr<nsIURI>                  mURI;
    nsCOMPtr<nsIURI>                  mDocumentURI;
    nsCOMPtr<nsIStreamListener>       mListener;
    nsCOMPtr<nsISupports>             mListenerContext;
    nsCOMPtr<nsILoadGroup>            mLoadGroup;
    nsCOMPtr<nsISupports>             mOwner;
    nsCOMPtr<nsIInterfaceRequestor>   mCallbacks;
    nsCOMPtr<nsIProgressEventSink>    mProgressSink;
    nsCOMPtr<nsIInputStream>          mUploadStream;
    nsCOMPtr<nsIURI>                  mReferrer;
    nsCOMPtr<nsISupports>             mSecurityInfo;
    nsCOMPtr<nsICancelable>           mProxyRequest;

    nsHttpRequestHead                 mRequestHead;
    nsHttpResponseHead               *mResponseHead;

    nsRefPtr<nsInputStreamPump>       mTransactionPump;
    nsHttpTransaction                *mTransaction;     
    nsHttpConnectionInfo             *mConnectionInfo;  

    nsCString                         mSpec; 

    PRUint32                          mLoadFlags;
    PRUint32                          mStatus;
    nsUint64                          mLogicalOffset;
    PRUint8                           mCaps;
    PRInt16                           mPriority;

    nsCString                         mContentTypeHint;
    nsCString                         mContentCharsetHint;
    nsCString                         mUserSetCookieHeader;

    
    nsCOMPtr<nsICacheEntryDescriptor> mCacheEntry;
    nsRefPtr<nsInputStreamPump>       mCachePump;
    nsHttpResponseHead               *mCachedResponseHead;
    nsCacheAccessMode                 mCacheAccess;
    PRUint32                          mPostID;
    PRUint32                          mRequestTime;

    nsCOMPtr<nsICacheEntryDescriptor> mOfflineCacheEntry;
    nsCacheAccessMode                 mOfflineCacheAccess;
    nsCString                         mOfflineCacheClientID;

    
    nsISupports                      *mProxyAuthContinuationState;
    nsCString                         mProxyAuthType;
    nsISupports                      *mAuthContinuationState;
    nsCString                         mAuthType;
    nsHttpAuthIdentity                mIdent;
    nsHttpAuthIdentity                mProxyIdent;

    
    nsCString                         mEntityID;
    PRUint64                          mStartPos;

    
    PRUint8                           mRedirectionLimit;

    
    PRUint32                          mIsPending                : 1;
    PRUint32                          mWasOpened                : 1;
    PRUint32                          mApplyConversion          : 1;
    PRUint32                          mAllowPipelining          : 1;
    PRUint32                          mCachedContentIsValid     : 1;
    PRUint32                          mCachedContentIsPartial   : 1;
    PRUint32                          mResponseHeadersModified  : 1;
    PRUint32                          mCanceled                 : 1;
    PRUint32                          mTransactionReplaced      : 1;
    PRUint32                          mUploadStreamHasHeaders   : 1;
    PRUint32                          mAuthRetryPending         : 1;
    PRUint32                          mSuppressDefensiveAuth    : 1;
    PRUint32                          mResuming                 : 1;
    PRUint32                          mInitedCacheEntry         : 1;
    PRUint32                          mCacheForOfflineUse       : 1;

    class nsContentEncodings : public nsIUTF8StringEnumerator
    {
    public:
        NS_DECL_ISUPPORTS
        NS_DECL_NSIUTF8STRINGENUMERATOR

        nsContentEncodings(nsIHttpChannel* aChannel, const char* aEncodingHeader);
        virtual ~nsContentEncodings();
        
    private:
        nsresult PrepareForNext(void);
        
        
        const char* mEncodingHeader;
        const char* mCurStart;  
        const char* mCurEnd;  
        
        
        
        nsCOMPtr<nsIHttpChannel> mChannel;
        
        PRPackedBool mReady;
    };
};

#endif 
