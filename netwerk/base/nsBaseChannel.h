




#ifndef nsBaseChannel_h__
#define nsBaseChannel_h__

#include "nsString.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsHashPropertyBag.h"
#include "nsInputStreamPump.h"

#include "nsIChannel.h"
#include "nsIURI.h"
#include "nsILoadGroup.h"
#include "nsILoadInfo.h"
#include "nsIStreamListener.h"
#include "nsIInterfaceRequestor.h"
#include "nsIProgressEventSink.h"
#include "nsITransport.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsIThreadRetargetableRequest.h"
#include "nsIThreadRetargetableStreamListener.h"
#include "PrivateBrowsingChannel.h"
#include "nsThreadUtils.h"
#include "nsNetUtil.h"

class nsIInputStream;













class nsBaseChannel : public nsHashPropertyBag
                    , public nsIChannel
                    , public nsIThreadRetargetableRequest
                    , public nsIInterfaceRequestor
                    , public nsITransportEventSink
                    , public nsIAsyncVerifyRedirectCallback
                    , public mozilla::net::PrivateBrowsingChannel<nsBaseChannel>
                    , protected nsIStreamListener
                    , protected nsIThreadRetargetableStreamListener
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIREQUEST
  NS_DECL_NSICHANNEL
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSITRANSPORTEVENTSINK
  NS_DECL_NSIASYNCVERIFYREDIRECTCALLBACK
  NS_DECL_NSITHREADRETARGETABLEREQUEST
  NS_DECL_NSITHREADRETARGETABLESTREAMLISTENER

  nsBaseChannel(); 

  
  nsresult Init() {
    return NS_OK;
  }

protected:
  
  

  virtual ~nsBaseChannel();

private:
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual nsresult OpenContentStream(bool async, nsIInputStream **stream,
                                     nsIChannel** channel) = 0;

  
  
  
  
  
  
  
  
  virtual bool GetStatusArg(nsresult status, nsString &statusArg) {
    return false;
  }

  
  virtual void OnCallbacksChanged() {
  }

  
  virtual void OnChannelDone() {
  }

public:
  
  

  
  
  
  
  
  
  
  nsresult Redirect(nsIChannel *newChannel, uint32_t redirectFlags,
                    bool openNewChannel);

  
  
  
  
  bool HasContentTypeHint() const;

  
  
  nsIURI *URI() {
    return mURI;
  }
  void SetURI(nsIURI *uri) {
    NS_ASSERTION(uri, "must specify a non-null URI");
    NS_ASSERTION(!mURI, "must not modify URI");
    NS_ASSERTION(!mOriginalURI, "how did that get set so early?");
    mURI = uri;
    mOriginalURI = uri;
  }
  nsIURI *OriginalURI() {
    return mOriginalURI;
  }

  
  
  nsISupports *SecurityInfo() {
    return mSecurityInfo; 
  }
  void SetSecurityInfo(nsISupports *info) {
    mSecurityInfo = info;
  }

  
  bool HasLoadFlag(uint32_t flag) {
    return (mLoadFlags & flag) != 0;
  }

  
  virtual bool Pending() const {
    return mPump || mWaitingOnAsyncRedirect;
 }

  
  template <class T> void GetCallback(nsCOMPtr<T> &result) {
    GetInterface(NS_GET_TEMPLATE_IID(T), getter_AddRefs(result));
  }

  
  nsQueryInterface do_QueryInterface() {
    return nsQueryInterface(static_cast<nsIChannel *>(this));
  }
  
  nsQueryInterface do_QueryInterface(nsISupports *obj) {
    return nsQueryInterface(obj);
  }

  
  
  
  
  void EnableSynthesizedProgressEvents(bool enable) {
    mSynthProgressEvents = enable;
  }

  
  
  void SetStreamListener(nsIStreamListener *listener) {
    mListener = listener;
  }
  nsIStreamListener *StreamListener() {
    return mListener;
  }

  
  
  
  
  
  
  
  nsresult PushStreamConverter(const char *fromType, const char *toType,
                               bool invalidatesContentLength = true,
                               nsIStreamListener **converter = nullptr);

protected:
  void DisallowThreadRetargeting() {
    mAllowThreadRetargeting = false;
  }

private:
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER

  
  nsresult BeginPumpingData();

  
  void CallbacksChanged() {
    mProgressSink = nullptr;
    mQueriedProgressSink = false;
    OnCallbacksChanged();
  }

  
  void ChannelDone() {
      mListener = nullptr;
      mListenerContext = nullptr;
      OnChannelDone();
  }

  
  
  void HandleAsyncRedirect(nsIChannel* newChannel);
  void ContinueHandleAsyncRedirect(nsresult result);
  nsresult ContinueRedirect();

  
  void ClassifyURI();

  class RedirectRunnable : public nsRunnable
  {
  public:
    RedirectRunnable(nsBaseChannel* chan, nsIChannel* newChannel)
      : mChannel(chan), mNewChannel(newChannel)
    {
      NS_PRECONDITION(newChannel, "Must have channel to redirect to");
    }
    
    NS_IMETHOD Run()
    {
      mChannel->HandleAsyncRedirect(mNewChannel);
      return NS_OK;
    }

  private:
    nsRefPtr<nsBaseChannel> mChannel;
    nsCOMPtr<nsIChannel> mNewChannel;
  };
  friend class RedirectRunnable;

  nsRefPtr<nsInputStreamPump>         mPump;
  nsCOMPtr<nsIProgressEventSink>      mProgressSink;
  nsCOMPtr<nsIURI>                    mOriginalURI;
  nsCOMPtr<nsISupports>               mOwner;
  nsCOMPtr<nsILoadInfo>               mLoadInfo;
  nsCOMPtr<nsISupports>               mSecurityInfo;
  nsCOMPtr<nsIChannel>                mRedirectChannel;
  nsCString                           mContentType;
  nsCString                           mContentCharset;
  uint32_t                            mLoadFlags;
  bool                                mQueriedProgressSink;
  bool                                mSynthProgressEvents;
  bool                                mAllowThreadRetargeting;
  bool                                mWaitingOnAsyncRedirect;
  bool                                mOpenRedirectChannel;
  uint32_t                            mRedirectFlags;

protected:
  nsCOMPtr<nsIURI>                    mURI;
  nsCOMPtr<nsILoadGroup>              mLoadGroup;
  nsCOMPtr<nsIInterfaceRequestor>     mCallbacks;
  nsCOMPtr<nsIStreamListener>         mListener;
  nsCOMPtr<nsISupports>               mListenerContext;
  nsresult                            mStatus;
  uint32_t                            mContentDispositionHint;
  nsAutoPtr<nsString>                 mContentDispositionFilename;
  int64_t                             mContentLength;
  bool                                mWasOpened;

  friend class mozilla::net::PrivateBrowsingChannel<nsBaseChannel>;
};

#endif 
