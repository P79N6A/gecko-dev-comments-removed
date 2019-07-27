






#ifndef mozilla_net_HttpBaseChannel_h
#define mozilla_net_HttpBaseChannel_h

#include "nsHttp.h"
#include "nsAutoPtr.h"
#include "nsHashPropertyBag.h"
#include "nsProxyInfo.h"
#include "nsHttpRequestHead.h"
#include "nsHttpResponseHead.h"
#include "nsHttpConnectionInfo.h"
#include "nsIEncodedChannel.h"
#include "nsIHttpChannel.h"
#include "nsHttpHandler.h"
#include "nsIHttpChannelInternal.h"
#include "nsIForcePendingChannel.h"
#include "nsIRedirectHistory.h"
#include "nsIUploadChannel.h"
#include "nsIUploadChannel2.h"
#include "nsIProgressEventSink.h"
#include "nsIURI.h"
#include "nsIEffectiveTLDService.h"
#include "nsIStringEnumerator.h"
#include "nsISupportsPriority.h"
#include "nsIClassOfService.h"
#include "nsIApplicationCache.h"
#include "nsIResumableChannel.h"
#include "nsITraceableChannel.h"
#include "nsILoadContext.h"
#include "nsILoadInfo.h"
#include "mozilla/net/NeckoCommon.h"
#include "nsThreadUtils.h"
#include "PrivateBrowsingChannel.h"
#include "mozilla/net/DNS.h"
#include "nsITimedChannel.h"
#include "nsIHttpChannel.h"
#include "nsISecurityConsoleMessage.h"
#include "nsCOMArray.h"

extern PRLogModuleInfo *gHttpLog;
class nsPerformance;
class nsISecurityConsoleMessage;
class nsIPrincipal;

namespace mozilla {
namespace net {








class HttpBaseChannel : public nsHashPropertyBag
                      , public nsIEncodedChannel
                      , public nsIHttpChannel
                      , public nsIHttpChannelInternal
                      , public nsIRedirectHistory
                      , public nsIUploadChannel
                      , public nsIUploadChannel2
                      , public nsISupportsPriority
                      , public nsIClassOfService
                      , public nsIResumableChannel
                      , public nsITraceableChannel
                      , public PrivateBrowsingChannel<HttpBaseChannel>
                      , public nsITimedChannel
                      , public nsIForcePendingChannel
{
protected:
  virtual ~HttpBaseChannel();

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIUPLOADCHANNEL
  NS_DECL_NSIUPLOADCHANNEL2
  NS_DECL_NSITRACEABLECHANNEL
  NS_DECL_NSITIMEDCHANNEL
  NS_DECL_NSIREDIRECTHISTORY

  HttpBaseChannel();

  virtual nsresult Init(nsIURI *aURI, uint32_t aCaps, nsProxyInfo *aProxyInfo,
                        uint32_t aProxyResolveFlags,
                        nsIURI *aProxyURI);

  
  NS_IMETHOD GetName(nsACString& aName) MOZ_OVERRIDE;
  NS_IMETHOD IsPending(bool *aIsPending) MOZ_OVERRIDE;
  NS_IMETHOD GetStatus(nsresult *aStatus) MOZ_OVERRIDE;
  NS_IMETHOD GetLoadGroup(nsILoadGroup **aLoadGroup) MOZ_OVERRIDE;
  NS_IMETHOD SetLoadGroup(nsILoadGroup *aLoadGroup) MOZ_OVERRIDE;
  NS_IMETHOD GetLoadFlags(nsLoadFlags *aLoadFlags) MOZ_OVERRIDE;
  NS_IMETHOD SetLoadFlags(nsLoadFlags aLoadFlags) MOZ_OVERRIDE;

  
  NS_IMETHOD GetOriginalURI(nsIURI **aOriginalURI) MOZ_OVERRIDE;
  NS_IMETHOD SetOriginalURI(nsIURI *aOriginalURI) MOZ_OVERRIDE;
  NS_IMETHOD GetURI(nsIURI **aURI) MOZ_OVERRIDE;
  NS_IMETHOD GetOwner(nsISupports **aOwner) MOZ_OVERRIDE;
  NS_IMETHOD SetOwner(nsISupports *aOwner) MOZ_OVERRIDE;
  NS_IMETHOD GetLoadInfo(nsILoadInfo **aLoadInfo) MOZ_OVERRIDE;
  NS_IMETHOD SetLoadInfo(nsILoadInfo *aLoadInfo) MOZ_OVERRIDE;
  NS_IMETHOD GetNotificationCallbacks(nsIInterfaceRequestor **aCallbacks) MOZ_OVERRIDE;
  NS_IMETHOD SetNotificationCallbacks(nsIInterfaceRequestor *aCallbacks) MOZ_OVERRIDE;
  NS_IMETHOD GetContentType(nsACString& aContentType) MOZ_OVERRIDE;
  NS_IMETHOD SetContentType(const nsACString& aContentType) MOZ_OVERRIDE;
  NS_IMETHOD GetContentCharset(nsACString& aContentCharset) MOZ_OVERRIDE;
  NS_IMETHOD SetContentCharset(const nsACString& aContentCharset) MOZ_OVERRIDE;
  NS_IMETHOD GetContentDisposition(uint32_t *aContentDisposition) MOZ_OVERRIDE;
  NS_IMETHOD SetContentDisposition(uint32_t aContentDisposition) MOZ_OVERRIDE;
  NS_IMETHOD GetContentDispositionFilename(nsAString& aContentDispositionFilename) MOZ_OVERRIDE;
  NS_IMETHOD SetContentDispositionFilename(const nsAString& aContentDispositionFilename) MOZ_OVERRIDE;
  NS_IMETHOD GetContentDispositionHeader(nsACString& aContentDispositionHeader) MOZ_OVERRIDE;
  NS_IMETHOD GetContentLength(int64_t *aContentLength) MOZ_OVERRIDE;
  NS_IMETHOD SetContentLength(int64_t aContentLength) MOZ_OVERRIDE;
  NS_IMETHOD Open(nsIInputStream **aResult) MOZ_OVERRIDE;

  
  NS_IMETHOD GetApplyConversion(bool *value) MOZ_OVERRIDE;
  NS_IMETHOD SetApplyConversion(bool value) MOZ_OVERRIDE;
  NS_IMETHOD GetContentEncodings(nsIUTF8StringEnumerator** aEncodings) MOZ_OVERRIDE;
  NS_IMETHOD DoApplyContentConversions(nsIStreamListener *aNextListener,
                                       nsIStreamListener **aNewNextListener,
                                       nsISupports *aCtxt) MOZ_OVERRIDE;

  
  NS_IMETHOD GetRequestMethod(nsACString& aMethod) MOZ_OVERRIDE;
  NS_IMETHOD SetRequestMethod(const nsACString& aMethod) MOZ_OVERRIDE;
  NS_IMETHOD GetReferrer(nsIURI **referrer) MOZ_OVERRIDE;
  NS_IMETHOD SetReferrer(nsIURI *referrer) MOZ_OVERRIDE;
  NS_IMETHOD GetReferrerPolicy(uint32_t *referrerPolicy) MOZ_OVERRIDE;
  NS_IMETHOD SetReferrerWithPolicy(nsIURI *referrer, uint32_t referrerPolicy) MOZ_OVERRIDE;
  NS_IMETHOD GetRequestHeader(const nsACString& aHeader, nsACString& aValue) MOZ_OVERRIDE;
  NS_IMETHOD SetRequestHeader(const nsACString& aHeader,
                              const nsACString& aValue, bool aMerge) MOZ_OVERRIDE;
  NS_IMETHOD VisitRequestHeaders(nsIHttpHeaderVisitor *visitor) MOZ_OVERRIDE;
  NS_IMETHOD GetResponseHeader(const nsACString &header, nsACString &value) MOZ_OVERRIDE;
  NS_IMETHOD SetResponseHeader(const nsACString& header,
                               const nsACString& value, bool merge) MOZ_OVERRIDE;
  NS_IMETHOD VisitResponseHeaders(nsIHttpHeaderVisitor *visitor) MOZ_OVERRIDE;
  NS_IMETHOD GetAllowPipelining(bool *value) MOZ_OVERRIDE;
  NS_IMETHOD SetAllowPipelining(bool value) MOZ_OVERRIDE;
  NS_IMETHOD GetAllowSTS(bool *value) MOZ_OVERRIDE;
  NS_IMETHOD SetAllowSTS(bool value) MOZ_OVERRIDE;
  NS_IMETHOD GetRedirectionLimit(uint32_t *value) MOZ_OVERRIDE;
  NS_IMETHOD SetRedirectionLimit(uint32_t value) MOZ_OVERRIDE;
  NS_IMETHOD IsNoStoreResponse(bool *value) MOZ_OVERRIDE;
  NS_IMETHOD IsNoCacheResponse(bool *value) MOZ_OVERRIDE;
  NS_IMETHOD IsPrivateResponse(bool *value) MOZ_OVERRIDE;
  NS_IMETHOD GetResponseStatus(uint32_t *aValue) MOZ_OVERRIDE;
  NS_IMETHOD GetResponseStatusText(nsACString& aValue) MOZ_OVERRIDE;
  NS_IMETHOD GetRequestSucceeded(bool *aValue) MOZ_OVERRIDE;
  NS_IMETHOD RedirectTo(nsIURI *newURI) MOZ_OVERRIDE;

  
  NS_IMETHOD GetDocumentURI(nsIURI **aDocumentURI) MOZ_OVERRIDE;
  NS_IMETHOD SetDocumentURI(nsIURI *aDocumentURI) MOZ_OVERRIDE;
  NS_IMETHOD GetRequestVersion(uint32_t *major, uint32_t *minor) MOZ_OVERRIDE;
  NS_IMETHOD GetResponseVersion(uint32_t *major, uint32_t *minor) MOZ_OVERRIDE;
  NS_IMETHOD SetCookie(const char *aCookieHeader) MOZ_OVERRIDE;
  NS_IMETHOD GetThirdPartyFlags(uint32_t *aForce) MOZ_OVERRIDE;
  NS_IMETHOD SetThirdPartyFlags(uint32_t aForce) MOZ_OVERRIDE;
  NS_IMETHOD GetForceAllowThirdPartyCookie(bool *aForce) MOZ_OVERRIDE;
  NS_IMETHOD SetForceAllowThirdPartyCookie(bool aForce) MOZ_OVERRIDE;
  NS_IMETHOD GetCanceled(bool *aCanceled) MOZ_OVERRIDE;
  NS_IMETHOD GetChannelIsForDownload(bool *aChannelIsForDownload) MOZ_OVERRIDE;
  NS_IMETHOD SetChannelIsForDownload(bool aChannelIsForDownload) MOZ_OVERRIDE;
  NS_IMETHOD SetCacheKeysRedirectChain(nsTArray<nsCString> *cacheKeys) MOZ_OVERRIDE;
  NS_IMETHOD GetLocalAddress(nsACString& addr) MOZ_OVERRIDE;
  NS_IMETHOD GetLocalPort(int32_t* port) MOZ_OVERRIDE;
  NS_IMETHOD GetRemoteAddress(nsACString& addr) MOZ_OVERRIDE;
  NS_IMETHOD GetRemotePort(int32_t* port) MOZ_OVERRIDE;
  NS_IMETHOD GetAllowSpdy(bool *aAllowSpdy) MOZ_OVERRIDE;
  NS_IMETHOD SetAllowSpdy(bool aAllowSpdy) MOZ_OVERRIDE;
  NS_IMETHOD GetApiRedirectToURI(nsIURI * *aApiRedirectToURI) MOZ_OVERRIDE;
  nsresult AddSecurityMessage(const nsAString &aMessageTag, const nsAString &aMessageCategory);
  NS_IMETHOD TakeAllSecurityMessages(nsCOMArray<nsISecurityConsoleMessage> &aMessages) MOZ_OVERRIDE;
  NS_IMETHOD GetResponseTimeoutEnabled(bool *aEnable) MOZ_OVERRIDE;
  NS_IMETHOD SetResponseTimeoutEnabled(bool aEnable) MOZ_OVERRIDE;
  NS_IMETHOD AddRedirect(nsIPrincipal *aRedirect) MOZ_OVERRIDE;
  NS_IMETHOD ForcePending(bool aForcePending) MOZ_OVERRIDE;
  NS_IMETHOD GetLastModifiedTime(PRTime* lastModifiedTime) MOZ_OVERRIDE;
  NS_IMETHOD ForceNoIntercept() MOZ_OVERRIDE;
  NS_IMETHOD GetTopWindowURI(nsIURI **aTopWindowURI) MOZ_OVERRIDE;
  NS_IMETHOD ContinueBeginConnect() MOZ_OVERRIDE;
  NS_IMETHOD GetProxyURI(nsIURI **proxyURI) MOZ_OVERRIDE;

  inline void CleanRedirectCacheChainIfNecessary()
  {
      mRedirectedCachekeys = nullptr;
  }
  NS_IMETHOD HTTPUpgrade(const nsACString & aProtocolName,
                         nsIHttpUpgradeListener *aListener) MOZ_OVERRIDE;

  
  NS_IMETHOD GetPriority(int32_t *value) MOZ_OVERRIDE;
  NS_IMETHOD AdjustPriority(int32_t delta) MOZ_OVERRIDE;

  
  NS_IMETHOD GetClassFlags(uint32_t *outFlags) MOZ_OVERRIDE { *outFlags = mClassOfService; return NS_OK; }

  
  NS_IMETHOD GetEntityID(nsACString& aEntityID) MOZ_OVERRIDE;

  class nsContentEncodings : public nsIUTF8StringEnumerator
    {
    public:
        NS_DECL_ISUPPORTS
        NS_DECL_NSIUTF8STRINGENUMERATOR

        nsContentEncodings(nsIHttpChannel* aChannel, const char* aEncodingHeader);

    private:
        virtual ~nsContentEncodings();

        nsresult PrepareForNext(void);

        
        const char* mEncodingHeader;
        const char* mCurStart;  
        const char* mCurEnd;  

        
        
        nsCOMPtr<nsIHttpChannel> mChannel;

        bool mReady;
    };

    nsHttpResponseHead * GetResponseHead() const { return mResponseHead; }
    nsHttpRequestHead * GetRequestHead() { return &mRequestHead; }

    const NetAddr& GetSelfAddr() { return mSelfAddr; }
    const NetAddr& GetPeerAddr() { return mPeerAddr; }

public: 


    
    
    static bool ShouldRewriteRedirectToGET(uint32_t httpStatus,
                                           nsHttpRequestHead::ParsedMethodType method);

    
    
    nsresult DoApplyContentConversions(nsIStreamListener *aNextListener,
                                       nsIStreamListener **aNewNextListener);

protected:
  nsCOMArray<nsISecurityConsoleMessage> mSecurityConsoleMessages;

  
  void     DoNotifyListener();
  virtual void DoNotifyListenerCleanup() = 0;

  
  void ReleaseListeners();

  nsPerformance* GetPerformance();

  void AddCookiesToRequest();
  virtual nsresult SetupReplacementChannel(nsIURI *,
                                           nsIChannel *,
                                           bool preserveMethod);

  
  inline void CallOnModifyRequestObservers() {
    gHttpHandler->OnModifyRequest(this);
    mRequestObserversCalled = true;
  }

  
  template <class T>
  void GetCallback(nsCOMPtr<T> &aResult)
  {
    NS_QueryNotificationCallbacks(mCallbacks, mLoadGroup,
                                  NS_GET_TEMPLATE_IID(T),
                                  getter_AddRefs(aResult));
  }

  
  
  bool SameOriginWithOriginalUri(nsIURI *aURI);

  
  nsIPrincipal *GetURIPrincipal();

  
  
  bool ShouldIntercept();

  friend class PrivateBrowsingChannel<HttpBaseChannel>;

  nsCOMPtr<nsIURI>                  mURI;
  nsCOMPtr<nsIURI>                  mOriginalURI;
  nsCOMPtr<nsIURI>                  mDocumentURI;
  nsCOMPtr<nsIStreamListener>       mListener;
  nsCOMPtr<nsISupports>             mListenerContext;
  nsCOMPtr<nsILoadGroup>            mLoadGroup;
  nsCOMPtr<nsISupports>             mOwner;
  nsCOMPtr<nsILoadInfo>             mLoadInfo;
  nsCOMPtr<nsIInterfaceRequestor>   mCallbacks;
  nsCOMPtr<nsIProgressEventSink>    mProgressSink;
  nsCOMPtr<nsIURI>                  mReferrer;
  nsCOMPtr<nsIApplicationCache>     mApplicationCache;

  nsHttpRequestHead                 mRequestHead;
  nsCOMPtr<nsIInputStream>          mUploadStream;
  nsAutoPtr<nsHttpResponseHead>     mResponseHead;
  nsRefPtr<nsHttpConnectionInfo>    mConnectionInfo;
  nsCOMPtr<nsIProxyInfo>            mProxyInfo;

  nsCString                         mSpec; 
  nsCString                         mContentTypeHint;
  nsCString                         mContentCharsetHint;
  nsCString                         mUserSetCookieHeader;

  NetAddr                           mSelfAddr;
  NetAddr                           mPeerAddr;

  
  nsCString                        mUpgradeProtocol;
  nsCOMPtr<nsIHttpUpgradeListener> mUpgradeProtocolCallback;

  
  nsCString                         mEntityID;
  uint64_t                          mStartPos;

  nsresult                          mStatus;
  uint32_t                          mLoadFlags;
  uint32_t                          mCaps;
  uint32_t                          mClassOfService;
  int16_t                           mPriority;
  uint8_t                           mRedirectionLimit;

  uint32_t                          mApplyConversion            : 1;
  uint32_t                          mCanceled                   : 1;
  uint32_t                          mIsPending                  : 1;
  uint32_t                          mWasOpened                  : 1;
  
  uint32_t                          mRequestObserversCalled     : 1;
  uint32_t                          mResponseHeadersModified    : 1;
  uint32_t                          mAllowPipelining            : 1;
  uint32_t                          mAllowSTS                   : 1;
  uint32_t                          mThirdPartyFlags            : 3;
  uint32_t                          mUploadStreamHasHeaders     : 1;
  uint32_t                          mInheritApplicationCache    : 1;
  uint32_t                          mChooseApplicationCache     : 1;
  uint32_t                          mLoadedFromApplicationCache : 1;
  uint32_t                          mChannelIsForDownload       : 1;
  uint32_t                          mTracingEnabled             : 1;
  
  uint32_t                          mTimingEnabled              : 1;
  uint32_t                          mAllowSpdy                  : 1;
  uint32_t                          mResponseTimeoutEnabled     : 1;
  
  uint32_t                          mAllRedirectsSameOrigin     : 1;

  
  
  uint32_t                          mAllRedirectsPassTimingAllowCheck : 1;

  
  uint32_t                          mForceNoIntercept           : 1;

  
  uint32_t                          mSuspendCount;

  nsCOMPtr<nsIURI>                  mAPIRedirectToURI;
  nsAutoPtr<nsTArray<nsCString> >   mRedirectedCachekeys;
  
  nsCOMArray<nsIPrincipal>          mRedirects;

  uint32_t                          mProxyResolveFlags;
  nsCOMPtr<nsIURI>                  mProxyURI;

  uint32_t                          mContentDispositionHint;
  nsAutoPtr<nsString>               mContentDispositionFilename;

  nsRefPtr<nsHttpHandler>           mHttpHandler;  

  uint32_t                          mReferrerPolicy;

  
  
  
  nsString                          mInitiatorType;
  
  int16_t                           mRedirectCount;
  
  
  mozilla::TimeStamp                mRedirectStartTimeStamp;
  
  
  mozilla::TimeStamp                mRedirectEndTimeStamp;

  PRTime                            mChannelCreationTime;
  TimeStamp                         mChannelCreationTimestamp;
  TimeStamp                         mAsyncOpenTime;
  TimeStamp                         mCacheReadStart;
  TimeStamp                         mCacheReadEnd;
  
  
  TimingStruct                      mTransactionTimings;

  nsCOMPtr<nsIPrincipal>            mPrincipal;

  bool                              mForcePending;
  nsCOMPtr<nsIURI>                  mTopWindowURI;
};








template <class T>
class HttpAsyncAborter
{
public:
  explicit HttpAsyncAborter(T *derived) : mThis(derived), mCallOnResume(0) {}

  
  
  nsresult AsyncAbort(nsresult status);

  
  void HandleAsyncAbort();

  
  
  
  nsresult AsyncCall(void (T::*funcPtr)(),
                     nsRunnableMethod<T> **retval = nullptr);
private:
  T *mThis;

protected:
  
  void (T::* mCallOnResume)(void);
};

template <class T>
nsresult HttpAsyncAborter<T>::AsyncAbort(nsresult status)
{
  PR_LOG(gHttpLog, 4,
         ("HttpAsyncAborter::AsyncAbort [this=%p status=%x]\n", mThis, status));

  mThis->mStatus = status;

  
  return AsyncCall(&T::HandleAsyncAbort);
}



template <class T>
inline void HttpAsyncAborter<T>::HandleAsyncAbort()
{
  NS_PRECONDITION(!mCallOnResume, "How did that happen?");

  if (mThis->mSuspendCount) {
    PR_LOG(gHttpLog, 4,
           ("Waiting until resume to do async notification [this=%p]\n", mThis));
    mCallOnResume = &T::HandleAsyncAbort;
    return;
  }

  mThis->DoNotifyListener();

  
  if (mThis->mLoadGroup)
    mThis->mLoadGroup->RemoveRequest(mThis, nullptr, mThis->mStatus);
}

template <class T>
nsresult HttpAsyncAborter<T>::AsyncCall(void (T::*funcPtr)(),
                                   nsRunnableMethod<T> **retval)
{
  nsresult rv;

  nsRefPtr<nsRunnableMethod<T> > event = NS_NewRunnableMethod(mThis, funcPtr);
  rv = NS_DispatchToCurrentThread(event);
  if (NS_SUCCEEDED(rv) && retval) {
    *retval = event;
  }

  return rv;
}

} 
} 

#endif 
