




































#ifndef nsBaseChannel_h__
#define nsBaseChannel_h__

#include "nsString.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsHashPropertyBag.h"
#include "nsInputStreamPump.h"

#include "nsIChannel.h"
#include "nsIInputStream.h"
#include "nsIURI.h"
#include "nsILoadGroup.h"
#include "nsIStreamListener.h"
#include "nsIInterfaceRequestor.h"
#include "nsIProgressEventSink.h"
#include "nsITransport.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsThreadUtils.h"













class nsBaseChannel : public nsHashPropertyBag
                    , public nsIChannel
                    , public nsIInterfaceRequestor
                    , public nsITransportEventSink
                    , public nsIAsyncVerifyRedirectCallback
                    , private nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIREQUEST
  NS_DECL_NSICHANNEL
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSITRANSPORTEVENTSINK
  NS_DECL_NSIASYNCVERIFYREDIRECTCALLBACK

  nsBaseChannel(); 

  
  nsresult Init() {
    return nsHashPropertyBag::Init();
  }

protected:
  
  

  virtual ~nsBaseChannel() {}

private:
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual nsresult OpenContentStream(bool async, nsIInputStream **stream,
                                     nsIChannel** channel) = 0;

  
  
  
  
  
  
  
  
  virtual bool GetStatusArg(nsresult status, nsString &statusArg) {
    return PR_FALSE;
  }

  
  virtual void OnCallbacksChanged() {
  }

public:
  
  

  
  
  
  
  
  
  
  nsresult Redirect(nsIChannel *newChannel, PRUint32 redirectFlags,
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

  
  bool HasLoadFlag(PRUint32 flag) {
    return (mLoadFlags & flag) != 0;
  }

  
  bool IsPending() const {
    return mPump || mWaitingOnAsyncRedirect;
  }

  
  
  void SetContentLength64(PRInt64 len);
  PRInt64 ContentLength64();

  
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
                               nsIStreamListener **converter = nsnull);

private:
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER

  
  nsresult BeginPumpingData();

  
  void CallbacksChanged() {
    mProgressSink = nsnull;
    mQueriedProgressSink = PR_FALSE;
    OnCallbacksChanged();
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
  nsCOMPtr<nsIInterfaceRequestor>     mCallbacks;
  nsCOMPtr<nsIProgressEventSink>      mProgressSink;
  nsCOMPtr<nsIURI>                    mOriginalURI;
  nsCOMPtr<nsIURI>                    mURI;
  nsCOMPtr<nsISupports>               mOwner;
  nsCOMPtr<nsISupports>               mSecurityInfo;
  nsCOMPtr<nsIChannel>                mRedirectChannel;
  nsCString                           mContentType;
  nsCString                           mContentCharset;
  PRUint32                            mLoadFlags;
  bool                                mQueriedProgressSink;
  bool                                mSynthProgressEvents;
  bool                                mWasOpened;
  bool                                mWaitingOnAsyncRedirect;
  bool                                mOpenRedirectChannel;
  PRUint32                            mRedirectFlags;

protected:
  nsCOMPtr<nsILoadGroup>              mLoadGroup;
  nsCOMPtr<nsIStreamListener>         mListener;
  nsCOMPtr<nsISupports>               mListenerContext;
  nsresult                            mStatus;
};

#endif 
