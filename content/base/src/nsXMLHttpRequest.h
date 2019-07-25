




































#ifndef nsXMLHttpRequest_h__
#define nsXMLHttpRequest_h__

#include "nsIXMLHttpRequest.h"
#include "nsISupportsUtils.h"
#include "nsString.h"
#include "nsIDOMDocument.h"
#include "nsIURI.h"
#include "nsIHttpChannel.h"
#include "nsIDocument.h"
#include "nsIStreamListener.h"
#include "nsWeakReference.h"
#include "jsapi.h"
#include "nsIScriptContext.h"
#include "nsIChannelEventSink.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsIInterfaceRequestor.h"
#include "nsIHttpHeaderVisitor.h"
#include "nsIProgressEventSink.h"
#include "nsCOMArray.h"
#include "nsJSUtils.h"
#include "nsTArray.h"
#include "nsIJSNativeInitializer.h"
#include "nsIDOMLSProgressEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsITimer.h"
#include "nsIPrivateDOMEvent.h"
#include "nsDOMProgressEvent.h"
#include "nsDOMEventTargetWrapperCache.h"
#include "nsContentUtils.h"

class nsILoadGroup;
class AsyncVerifyRedirectCallbackForwarder;

class nsXHREventTarget : public nsDOMEventTargetWrapperCache,
                         public nsIXMLHttpRequestEventTarget
{
public:
  virtual ~nsXHREventTarget() {}
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsXHREventTarget,
                                           nsDOMEventTargetWrapperCache)
  NS_DECL_NSIXMLHTTPREQUESTEVENTTARGET
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

protected:
  nsRefPtr<nsDOMEventListenerWrapper> mOnLoadListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnErrorListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnAbortListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnLoadStartListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnProgressListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnLoadendListener;
};

class nsXMLHttpRequestUpload : public nsXHREventTarget,
                               public nsIXMLHttpRequestUpload
{
public:
  nsXMLHttpRequestUpload(nsPIDOMWindow* aOwner,
                         nsIScriptContext* aScriptContext)
  {
    mOwner = aOwner;
    mScriptContext = aScriptContext;
  }
  virtual ~nsXMLHttpRequestUpload();
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIXMLHTTPREQUESTEVENTTARGET(nsXHREventTarget::)
  NS_FORWARD_NSIDOMEVENTTARGET(nsXHREventTarget::)
  NS_DECL_NSIXMLHTTPREQUESTUPLOAD

  PRBool HasListeners()
  {
    return mListenerManager && mListenerManager->HasListeners();
  }
};

class nsXMLHttpRequest : public nsXHREventTarget,
                         public nsIXMLHttpRequest,
                         public nsIJSXMLHttpRequest,
                         public nsIStreamListener,
                         public nsIChannelEventSink,
                         public nsIProgressEventSink,
                         public nsIInterfaceRequestor,
                         public nsSupportsWeakReference,
                         public nsIJSNativeInitializer,
                         public nsITimerCallback
{
public:
  nsXMLHttpRequest();
  virtual ~nsXMLHttpRequest();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIXMLHTTPREQUEST

  
  NS_IMETHOD GetOnuploadprogress(nsIDOMEventListener** aOnuploadprogress);
  NS_IMETHOD SetOnuploadprogress(nsIDOMEventListener* aOnuploadprogress);

  NS_FORWARD_NSIXMLHTTPREQUESTEVENTTARGET(nsXHREventTarget::)

  
  NS_DECL_NSISTREAMLISTENER

  
  NS_DECL_NSIREQUESTOBSERVER

  
  NS_DECL_NSICHANNELEVENTSINK

  
  NS_DECL_NSIPROGRESSEVENTSINK

  
  NS_DECL_NSIINTERFACEREQUESTOR

  
  NS_DECL_NSITIMERCALLBACK

  
  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext* cx, JSObject* obj,
                       PRUint32 argc, jsval* argv);

  NS_FORWARD_NSIDOMEVENTTARGET(nsXHREventTarget::)

  
  
  static nsresult CreateReadystatechangeEvent(nsIDOMEvent** aDOMEvent);
  
  
  
  
  void DispatchProgressEvent(nsDOMEventTargetHelper* aTarget,
                             const nsAString& aType,
                             
                             
                             PRBool aUseLSEventWrapper,
                             PRBool aLengthComputable,
                             
                             PRUint64 aLoaded, PRUint64 aTotal,
                             
                             PRUint64 aPosition, PRUint64 aTotalSize);
  void DispatchProgressEvent(nsDOMEventTargetHelper* aTarget,
                             const nsAString& aType,
                             PRBool aLengthComputable,
                             PRUint64 aLoaded, PRUint64 aTotal)
  {
    DispatchProgressEvent(aTarget, aType, PR_FALSE,
                          aLengthComputable, aLoaded, aTotal,
                          aLoaded, aLengthComputable ? aTotal : LL_MAXUINT);
  }

  
  nsresult Init();

  void SetRequestObserver(nsIRequestObserver* aObserver);

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(nsXMLHttpRequest,
                                           nsXHREventTarget)
  PRBool AllowUploadProgress();
  void RootResultArrayBuffer();
  
protected:
  friend class nsMultipartProxyListener;

  nsresult DetectCharset(nsACString& aCharset);
  nsresult ConvertBodyToText(nsAString& aOutBuffer);
  static NS_METHOD StreamReaderFunc(nsIInputStream* in,
                void* closure,
                const char* fromRawSegment,
                PRUint32 toOffset,
                PRUint32 count,
                PRUint32 *writeCount);
  nsresult CreateResponseArrayBuffer(JSContext* aCx);
  void CreateResponseBlob(nsIRequest *request);
  
  
  nsresult ChangeState(PRUint32 aState, PRBool aBroadcast = PR_TRUE);
  already_AddRefed<nsILoadGroup> GetLoadGroup() const;
  nsIURI *GetBaseURI();

  nsresult RemoveAddEventListener(const nsAString& aType,
                                  nsRefPtr<nsDOMEventListenerWrapper>& aCurrent,
                                  nsIDOMEventListener* aNew);

  nsresult GetInnerEventListener(nsRefPtr<nsDOMEventListenerWrapper>& aWrapper,
                                 nsIDOMEventListener** aListener);

  already_AddRefed<nsIHttpChannel> GetCurrentHttpChannel();

  bool IsSystemXHR();

  





  nsresult CheckChannelForCrossSiteRequest(nsIChannel* aChannel);

  void StartProgressEventTimer();

  friend class AsyncVerifyRedirectCallbackForwarder;
  void OnRedirectVerifyCallback(nsresult result);

  nsCOMPtr<nsISupports> mContext;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<nsIChannel> mChannel;
  
  nsCOMPtr<nsIRequest> mReadRequest;
  nsCOMPtr<nsIDOMDocument> mResponseXML;
  nsCOMPtr<nsIChannel> mCORSPreflightChannel;
  nsTArray<nsCString> mCORSUnsafeHeaders;

  nsRefPtr<nsDOMEventListenerWrapper> mOnUploadProgressListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnReadystatechangeListener;

  nsCOMPtr<nsIStreamListener> mXMLParserStreamListener;

  
  class nsHeaderVisitor : public nsIHttpHeaderVisitor {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHTTPHEADERVISITOR
    nsHeaderVisitor() { }
    virtual ~nsHeaderVisitor() {}
    const nsACString &Headers() { return mHeaders; }
  private:
    nsCString mHeaders;
  };

  
  nsCString mResponseBody;

  
  
  
  
  
  nsString mResponseBodyUnicode;

  enum {
    XML_HTTP_RESPONSE_TYPE_DEFAULT,
    XML_HTTP_RESPONSE_TYPE_ARRAYBUFFER,
    XML_HTTP_RESPONSE_TYPE_BLOB,
    XML_HTTP_RESPONSE_TYPE_DOCUMENT,
    XML_HTTP_RESPONSE_TYPE_TEXT
  } mResponseType;

  nsCOMPtr<nsIDOMBlob> mResponseBlob;

  nsCString mOverrideMimeType;

  



  nsCOMPtr<nsIInterfaceRequestor> mNotificationCallbacks;
  




  nsCOMPtr<nsIChannelEventSink> mChannelEventSink;
  nsCOMPtr<nsIProgressEventSink> mProgressEventSink;

  nsIRequestObserver* mRequestObserver;

  nsCOMPtr<nsIURI> mBaseURI;

  PRUint32 mState;

  nsRefPtr<nsXMLHttpRequestUpload> mUpload;
  PRUint64 mUploadTransferred;
  PRUint64 mUploadTotal;
  PRPackedBool mUploadComplete;
  PRUint64 mUploadProgress; 
  PRUint64 mUploadProgressMax; 

  PRPackedBool mErrorLoad;

  PRPackedBool mTimerIsActive;
  PRPackedBool mProgressEventWasDelayed;
  PRPackedBool mLoadLengthComputable;
  PRUint64 mLoadTotal; 
  nsCOMPtr<nsITimer> mProgressNotifier;

  PRPackedBool mFirstStartRequestSeen;
  
  nsCOMPtr<nsIAsyncVerifyRedirectCallback> mRedirectCallback;
  nsCOMPtr<nsIChannel> mNewRedirectChannel;
  
  JSObject* mResultArrayBuffer;

  struct RequestHeader
  {
    nsCString header;
    nsCString value;
  };
  nsTArray<RequestHeader> mModifiedRequestHeaders;
};



class nsXMLHttpProgressEvent : public nsIDOMProgressEvent,
                               public nsIDOMLSProgressEvent,
                               public nsIDOMNSEvent,
                               public nsIPrivateDOMEvent
{
public:
  nsXMLHttpProgressEvent(nsIDOMProgressEvent* aInner,
                         PRUint64 aCurrentProgress,
                         PRUint64 aMaxProgress);
  virtual ~nsXMLHttpProgressEvent();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXMLHttpProgressEvent, nsIDOMNSEvent)
  NS_FORWARD_NSIDOMEVENT(mInner->)
  NS_FORWARD_NSIDOMNSEVENT(mInner->)
  NS_FORWARD_NSIDOMPROGRESSEVENT(mInner->)
  NS_DECL_NSIDOMLSPROGRESSEVENT
  
  NS_IMETHOD DuplicatePrivateData()
  {
    return mInner->DuplicatePrivateData();
  }
  NS_IMETHOD SetTarget(nsIDOMEventTarget* aTarget)
  {
    return mInner->SetTarget(aTarget);
  }
  NS_IMETHOD_(PRBool) IsDispatchStopped()
  {
    return mInner->IsDispatchStopped();
  }
  NS_IMETHOD_(nsEvent*) GetInternalNSEvent()
  {
    return mInner->GetInternalNSEvent();
  }
  NS_IMETHOD SetTrusted(PRBool aTrusted)
  {
    return mInner->SetTrusted(aTrusted);
  }
  virtual void Serialize(IPC::Message* aMsg,
                         PRBool aSerializeInterfaceType)
  {
    mInner->Serialize(aMsg, aSerializeInterfaceType);
  }
  virtual PRBool Deserialize(const IPC::Message* aMsg, void** aIter)
  {
    return mInner->Deserialize(aMsg, aIter);
  }

protected:
  
  
  nsRefPtr<nsDOMProgressEvent> mInner;
  PRUint64 mCurProgress;
  PRUint64 mMaxProgress;
};

#endif
