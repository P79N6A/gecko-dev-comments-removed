








































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
#include "nsIHttpChannelInternal.h"
#include "nsIUploadChannel.h"
#include "nsIUploadChannel2.h"
#include "nsIProgressEventSink.h"
#include "nsIURI.h"
#include "nsIStringEnumerator.h"
#include "nsISupportsPriority.h"
#include "nsIApplicationCache.h"
#include "nsIResumableChannel.h"
#include "nsITraceableChannel.h"
#include "mozilla/net/NeckoCommon.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace net {








class HttpBaseChannel : public nsHashPropertyBag
                      , public nsIEncodedChannel
                      , public nsIHttpChannel
                      , public nsIHttpChannelInternal
                      , public nsIUploadChannel
                      , public nsIUploadChannel2
                      , public nsISupportsPriority
                      , public nsIResumableChannel
                      , public nsITraceableChannel
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIUPLOADCHANNEL
  NS_DECL_NSIUPLOADCHANNEL2
  NS_DECL_NSITRACEABLECHANNEL

  HttpBaseChannel();
  virtual ~HttpBaseChannel();

  virtual nsresult Init(nsIURI *aURI, PRUint8 aCaps, nsProxyInfo *aProxyInfo);

  
  NS_IMETHOD GetName(nsACString& aName);
  NS_IMETHOD IsPending(bool *aIsPending);
  NS_IMETHOD GetStatus(nsresult *aStatus);
  NS_IMETHOD GetLoadGroup(nsILoadGroup **aLoadGroup);
  NS_IMETHOD SetLoadGroup(nsILoadGroup *aLoadGroup);
  NS_IMETHOD GetLoadFlags(nsLoadFlags *aLoadFlags);
  NS_IMETHOD SetLoadFlags(nsLoadFlags aLoadFlags);

  
  NS_IMETHOD GetOriginalURI(nsIURI **aOriginalURI);
  NS_IMETHOD SetOriginalURI(nsIURI *aOriginalURI);
  NS_IMETHOD GetURI(nsIURI **aURI);
  NS_IMETHOD GetOwner(nsISupports **aOwner);
  NS_IMETHOD SetOwner(nsISupports *aOwner);
  NS_IMETHOD GetNotificationCallbacks(nsIInterfaceRequestor **aCallbacks);
  NS_IMETHOD SetNotificationCallbacks(nsIInterfaceRequestor *aCallbacks);
  NS_IMETHOD GetContentType(nsACString& aContentType);
  NS_IMETHOD SetContentType(const nsACString& aContentType);
  NS_IMETHOD GetContentCharset(nsACString& aContentCharset);
  NS_IMETHOD SetContentCharset(const nsACString& aContentCharset);
  NS_IMETHOD GetContentDisposition(PRUint32 *aContentDisposition);
  NS_IMETHOD GetContentDispositionFilename(nsAString& aContentDispositionFilename);
  NS_IMETHOD GetContentDispositionHeader(nsACString& aContentDispositionHeader);
  NS_IMETHOD GetContentLength(PRInt32 *aContentLength);
  NS_IMETHOD SetContentLength(PRInt32 aContentLength);
  NS_IMETHOD Open(nsIInputStream **aResult);

  
  NS_IMETHOD GetApplyConversion(bool *value);
  NS_IMETHOD SetApplyConversion(bool value);
  NS_IMETHOD GetContentEncodings(nsIUTF8StringEnumerator** aEncodings);

  
  NS_IMETHOD GetRequestMethod(nsACString& aMethod);
  NS_IMETHOD SetRequestMethod(const nsACString& aMethod);
  NS_IMETHOD GetReferrer(nsIURI **referrer);
  NS_IMETHOD SetReferrer(nsIURI *referrer);
  NS_IMETHOD GetRequestHeader(const nsACString& aHeader, nsACString& aValue);
  NS_IMETHOD SetRequestHeader(const nsACString& aHeader, 
                              const nsACString& aValue, bool aMerge);
  NS_IMETHOD VisitRequestHeaders(nsIHttpHeaderVisitor *visitor);
  NS_IMETHOD GetResponseHeader(const nsACString &header, nsACString &value);
  NS_IMETHOD SetResponseHeader(const nsACString& header, 
                               const nsACString& value, bool merge);
  NS_IMETHOD VisitResponseHeaders(nsIHttpHeaderVisitor *visitor);
  NS_IMETHOD GetAllowPipelining(bool *value);
  NS_IMETHOD SetAllowPipelining(bool value);
  NS_IMETHOD GetRedirectionLimit(PRUint32 *value);
  NS_IMETHOD SetRedirectionLimit(PRUint32 value);
  NS_IMETHOD IsNoStoreResponse(bool *value);
  NS_IMETHOD IsNoCacheResponse(bool *value);
  NS_IMETHOD GetResponseStatus(PRUint32 *aValue);
  NS_IMETHOD GetResponseStatusText(nsACString& aValue);
  NS_IMETHOD GetRequestSucceeded(bool *aValue);

  
  NS_IMETHOD GetDocumentURI(nsIURI **aDocumentURI);
  NS_IMETHOD SetDocumentURI(nsIURI *aDocumentURI);
  NS_IMETHOD GetRequestVersion(PRUint32 *major, PRUint32 *minor);
  NS_IMETHOD GetResponseVersion(PRUint32 *major, PRUint32 *minor);
  NS_IMETHOD SetCookie(const char *aCookieHeader);
  NS_IMETHOD GetForceAllowThirdPartyCookie(bool *aForce);
  NS_IMETHOD SetForceAllowThirdPartyCookie(bool aForce);
  NS_IMETHOD GetCanceled(bool *aCanceled);
  NS_IMETHOD GetChannelIsForDownload(bool *aChannelIsForDownload);
  NS_IMETHOD SetChannelIsForDownload(bool aChannelIsForDownload);
  NS_IMETHOD SetCacheKeysRedirectChain(nsTArray<nsCString> *cacheKeys);
  NS_IMETHOD GetLocalAddress(nsACString& addr);
  NS_IMETHOD GetLocalPort(PRInt32* port);
  NS_IMETHOD GetRemoteAddress(nsACString& addr);
  NS_IMETHOD GetRemotePort(PRInt32* port);
  NS_IMETHOD GetAllowSpdy(bool *aAllowSpdy);
  NS_IMETHOD SetAllowSpdy(bool aAllowSpdy);
  
  inline void CleanRedirectCacheChainIfNecessary()
  {
      if (mRedirectedCachekeys) {
          delete mRedirectedCachekeys;
          mRedirectedCachekeys = nsnull;
      }
  }
  NS_IMETHOD HTTPUpgrade(const nsACString & aProtocolName,
                         nsIHttpUpgradeListener *aListener); 

  
  NS_IMETHOD GetPriority(PRInt32 *value);
  NS_IMETHOD AdjustPriority(PRInt32 delta);

  
  NS_IMETHOD GetEntityID(nsACString& aEntityID);

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
        
        bool mReady;
    };

    nsHttpResponseHead * GetResponseHead() const { return mResponseHead; }
    nsHttpRequestHead * GetRequestHead() { return &mRequestHead; }

    const PRNetAddr& GetSelfAddr() { return mSelfAddr; }
    const PRNetAddr& GetPeerAddr() { return mPeerAddr; }

public: 

  bool ShouldRewriteRedirectToGET(PRUint32 httpStatus, nsHttpAtom method);
  bool IsSafeMethod(nsHttpAtom method);
  
protected:

  
  void     DoNotifyListener();
  virtual void DoNotifyListenerCleanup() = 0;

  nsresult ApplyContentConversions();

  void AddCookiesToRequest();
  virtual nsresult SetupReplacementChannel(nsIURI *,
                                           nsIChannel *,
                                           bool preserveMethod);

  
  template <class T>
  void GetCallback(nsCOMPtr<T> &aResult)
  {
    NS_QueryNotificationCallbacks(mCallbacks, mLoadGroup,
                                  NS_GET_TEMPLATE_IID(T),
                                  getter_AddRefs(aResult));
  }

  nsCOMPtr<nsIURI>                  mURI;
  nsCOMPtr<nsIURI>                  mOriginalURI;
  nsCOMPtr<nsIURI>                  mDocumentURI;
  nsCOMPtr<nsIStreamListener>       mListener;
  nsCOMPtr<nsISupports>             mListenerContext;
  nsCOMPtr<nsILoadGroup>            mLoadGroup;
  nsCOMPtr<nsISupports>             mOwner;
  nsCOMPtr<nsIInterfaceRequestor>   mCallbacks;
  nsCOMPtr<nsIProgressEventSink>    mProgressSink;
  nsCOMPtr<nsIURI>                  mReferrer;
  nsCOMPtr<nsIApplicationCache>     mApplicationCache;

  nsHttpRequestHead                 mRequestHead;
  nsCOMPtr<nsIInputStream>          mUploadStream;
  nsAutoPtr<nsHttpResponseHead>     mResponseHead;
  nsRefPtr<nsHttpConnectionInfo>    mConnectionInfo;

  nsCString                         mSpec; 
  nsCString                         mContentTypeHint;
  nsCString                         mContentCharsetHint;
  nsCString                         mUserSetCookieHeader;

  PRNetAddr                         mSelfAddr;
  PRNetAddr                         mPeerAddr;

  
  nsCString                        mUpgradeProtocol;
  nsCOMPtr<nsIHttpUpgradeListener> mUpgradeProtocolCallback;

  
  nsCString                         mEntityID;
  PRUint64                          mStartPos;

  nsresult                          mStatus;
  PRUint32                          mLoadFlags;
  PRInt16                           mPriority;
  PRUint8                           mCaps;
  PRUint8                           mRedirectionLimit;

  PRUint32                          mApplyConversion            : 1;
  PRUint32                          mCanceled                   : 1;
  PRUint32                          mIsPending                  : 1;
  PRUint32                          mWasOpened                  : 1;
  PRUint32                          mResponseHeadersModified    : 1;
  PRUint32                          mAllowPipelining            : 1;
  PRUint32                          mForceAllowThirdPartyCookie : 1;
  PRUint32                          mUploadStreamHasHeaders     : 1;
  PRUint32                          mInheritApplicationCache    : 1;
  PRUint32                          mChooseApplicationCache     : 1;
  PRUint32                          mLoadedFromApplicationCache : 1;
  PRUint32                          mChannelIsForDownload       : 1;
  PRUint32                          mTracingEnabled             : 1;
  
  PRUint32                          mTimingEnabled              : 1;
  PRUint32                          mAllowSpdy                  : 1;

  
  PRUint32                          mSuspendCount;

  nsTArray<nsCString>              *mRedirectedCachekeys;
};








template <class T>
class HttpAsyncAborter
{
public:
  HttpAsyncAborter(T *derived) : mThis(derived), mCallOnResume(0) {}

  
  
  nsresult AsyncAbort(nsresult status);

  
  void HandleAsyncAbort();

  
  
  
  nsresult AsyncCall(void (T::*funcPtr)(),
                     nsRunnableMethod<T> **retval = nsnull);
private:
  T *mThis;

protected:
  
  void (T::* mCallOnResume)(void);
};

template <class T>
nsresult HttpAsyncAborter<T>::AsyncAbort(nsresult status)
{
  LOG(("HttpAsyncAborter::AsyncAbort [this=%p status=%x]\n", mThis, status));

  mThis->mStatus = status;
  mThis->mIsPending = false;

  
  return AsyncCall(&T::HandleAsyncAbort);
}



template <class T>
inline void HttpAsyncAborter<T>::HandleAsyncAbort()
{
  NS_PRECONDITION(!mCallOnResume, "How did that happen?");

  if (mThis->mSuspendCount) {
    LOG(("Waiting until resume to do async notification [this=%p]\n",
         mThis));
    mCallOnResume = &T::HandleAsyncAbort;
    return;
  }

  mThis->DoNotifyListener();

  
  if (mThis->mLoadGroup)
    mThis->mLoadGroup->RemoveRequest(mThis, nsnull, mThis->mStatus);
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
