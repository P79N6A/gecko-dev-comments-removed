




































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













class nsBaseChannel : public nsHashPropertyBag
                    , public nsIChannel
                    , public nsIInterfaceRequestor
                    , public nsITransportEventSink
                    , private nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIREQUEST
  NS_DECL_NSICHANNEL
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSITRANSPORTEVENTSINK

  nsBaseChannel(); 

  
  nsresult Init() {
    return nsHashPropertyBag::Init();
  }

protected:
  
  

  virtual ~nsBaseChannel() {}

private:
  
  
  
  
  
  
  
  
  
  
  
  virtual nsresult OpenContentStream(PRBool async, nsIInputStream **stream) = 0;

  
  
  
  
  
  
  
  
  virtual PRBool GetStatusArg(nsresult status, nsString &statusArg) {
    return PR_FALSE;
  }

  
  virtual void OnCallbacksChanged() {
  }

public:
  
  

  
  
  
  
  
  
  nsresult Redirect(nsIChannel *newChannel, PRUint32 redirectFlags);

  
  
  
  
  PRBool HasContentTypeHint() const;

  
  
  nsIURI *URI() {
    return mURI;
  }
  void SetURI(nsIURI *uri) {
    NS_ASSERTION(uri, "must specify a non-null URI");
    NS_ASSERTION(!mURI, "must not modify URI");
    mURI = uri;
  }
  nsIURI *OriginalURI() {
    return mOriginalURI ? mOriginalURI : mURI;
  }

  
  
  nsISupports *SecurityInfo() {
    return mSecurityInfo; 
  }
  void SetSecurityInfo(nsISupports *info) {
    mSecurityInfo = info;
  }

  
  PRBool HasLoadFlag(PRUint32 flag) {
    return (mLoadFlags & flag) != 0;
  }

  
  PRBool IsPending() const {
    return (mPump != nsnull);
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

  
  
  
  
  void EnableSynthesizedProgressEvents(PRBool enable) {
    mSynthProgressEvents = enable;
  }

  
  
  void SetStreamListener(nsIStreamListener *listener) {
    mListener = listener;
  }
  nsIStreamListener *StreamListener() {
    return mListener;
  }

  
  
  
  
  
  
  
  nsresult PushStreamConverter(const char *fromType, const char *toType,
                               PRBool invalidatesContentLength = PR_TRUE,
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

  nsRefPtr<nsInputStreamPump>         mPump;
  nsCOMPtr<nsIInterfaceRequestor>     mCallbacks;
  nsCOMPtr<nsIProgressEventSink>      mProgressSink;
  nsCOMPtr<nsIURI>                    mOriginalURI;
  nsCOMPtr<nsIURI>                    mURI;
  nsCOMPtr<nsILoadGroup>              mLoadGroup;
  nsCOMPtr<nsISupports>               mOwner;
  nsCOMPtr<nsISupports>               mSecurityInfo;
  nsCOMPtr<nsIStreamListener>         mListener;
  nsCOMPtr<nsISupports>               mListenerContext;
  nsCString                           mContentType;
  nsCString                           mContentCharset;
  PRUint32                            mLoadFlags;
  nsresult                            mStatus;
  PRPackedBool                        mQueriedProgressSink;
  PRPackedBool                        mSynthProgressEvents;
  PRPackedBool                        mWasOpened;
};

#endif 
