





#ifndef nsXMLHttpRequest_h__
#define nsXMLHttpRequest_h__

#include "nsIXMLHttpRequest.h"
#include "nsISupportsUtils.h"
#include "nsString.h"
#include "nsIURI.h"
#include "nsIHttpChannel.h"
#include "nsIJARChannel.h"
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
#include "nsITimer.h"
#include "nsIDOMProgressEvent.h"
#include "nsDOMEventTargetHelper.h"
#include "nsDOMFile.h"
#include "nsDOMBlobBuilder.h"
#include "nsIPrincipal.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsISizeOfEventTarget.h"

#include "mozilla/Assertions.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/dom/XMLHttpRequestBinding.h"
#include "mozilla/dom/XMLHttpRequestUploadBinding.h"
#include "mozilla/dom/EventHandlerBinding.h"

#ifdef Status


#undef Status
#endif

class nsILoadGroup;
class AsyncVerifyRedirectCallbackForwarder;
class nsIUnicodeDecoder;
class nsFormData;

class nsXHREventTarget : public nsDOMEventTargetHelper,
                         public nsIXMLHttpRequestEventTarget
{
public:
  typedef mozilla::dom::XMLHttpRequestResponseType
          XMLHttpRequestResponseType;
  typedef mozilla::ErrorResult
          ErrorResult;

  virtual ~nsXHREventTarget() {}
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsXHREventTarget,
                                           nsDOMEventTargetHelper)
  NS_DECL_NSIXMLHTTPREQUESTEVENTTARGET
  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  IMPL_EVENT_HANDLER(loadstart)
  IMPL_EVENT_HANDLER(progress)
  IMPL_EVENT_HANDLER(abort)
  IMPL_EVENT_HANDLER(error)
  IMPL_EVENT_HANDLER(load)
  IMPL_EVENT_HANDLER(timeout)
  IMPL_EVENT_HANDLER(loadend)
  
  virtual void DisconnectFromOwner();
};

class nsXMLHttpRequestUpload : public nsXHREventTarget,
                               public nsIXMLHttpRequestUpload
{
public:
  nsXMLHttpRequestUpload(nsDOMEventTargetHelper* aOwner)
  {
    BindToOwner(aOwner);
    SetIsDOMBinding();
  }                                         
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIXMLHTTPREQUESTEVENTTARGET(nsXHREventTarget::)
  NS_FORWARD_NSIDOMEVENTTARGET(nsXHREventTarget::)
  NS_DECL_NSIXMLHTTPREQUESTUPLOAD

  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope) MOZ_OVERRIDE
  {
    return mozilla::dom::XMLHttpRequestUploadBinding::Wrap(cx, scope, this);
  }
  nsISupports* GetParentObject()
  {
    return GetOwner();
  }

  bool HasListeners()
  {
    return mListenerManager && mListenerManager->HasListeners();
  }
};

class nsXMLHttpRequestXPCOMifier;



class nsXMLHttpRequest : public nsXHREventTarget,
                         public nsIXMLHttpRequest,
                         public nsIJSXMLHttpRequest,
                         public nsIStreamListener,
                         public nsIChannelEventSink,
                         public nsIProgressEventSink,
                         public nsIInterfaceRequestor,
                         public nsSupportsWeakReference,
                         public nsITimerCallback,
                         public nsISizeOfEventTarget
{
  friend class nsXHRParseEndListener;
  friend class nsXMLHttpRequestXPCOMifier;

public:
  nsXMLHttpRequest();
  virtual ~nsXMLHttpRequest();

  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope) MOZ_OVERRIDE
  {
    return mozilla::dom::XMLHttpRequestBinding::Wrap(cx, scope, this);
  }
  nsISupports* GetParentObject()
  {
    return GetOwner();
  }

  
  static already_AddRefed<nsXMLHttpRequest>
  Constructor(const mozilla::dom::GlobalObject& aGlobal,
              JSContext* aCx,
              const mozilla::dom::MozXMLHttpRequestParameters& aParams,
              ErrorResult& aRv)
  {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.Get());
    nsCOMPtr<nsIScriptObjectPrincipal> principal =
      do_QueryInterface(aGlobal.Get());
    if (!window || ! principal) {
      aRv.Throw(NS_ERROR_FAILURE);
      return NULL;
    }

    nsRefPtr<nsXMLHttpRequest> req = new nsXMLHttpRequest();
    req->Construct(principal->GetPrincipal(), window);
    req->InitParameters(aParams.mMozAnon, aParams.mMozSystem);
    return req.forget();
  }

  static already_AddRefed<nsXMLHttpRequest>
  Constructor(const mozilla::dom::GlobalObject& aGlobal,
              JSContext* aCx,
              const nsAString& ignored,
              ErrorResult& aRv)
  {
    
    mozilla::dom::MozXMLHttpRequestParameters params;
    if (!params.Init(aCx, nullptr, JS::NullValue())) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }

    return Constructor(aGlobal, aCx, params, aRv);
  }

  void Construct(nsIPrincipal* aPrincipal,
                 nsPIDOMWindow* aOwnerWindow,
                 nsIURI* aBaseURI = NULL)
  {
    MOZ_ASSERT(aPrincipal);
    MOZ_ASSERT_IF(aOwnerWindow, aOwnerWindow->IsInnerWindow());
    mPrincipal = aPrincipal;
    BindToOwner(aOwnerWindow);
    mBaseURI = aBaseURI;
  }

  void InitParameters(bool aAnon, bool aSystem);

  void SetParameters(bool aAnon, bool aSystem)
  {
    mIsAnon = aAnon;
    mIsSystem = aSystem;
  }

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIXMLHTTPREQUEST

  NS_FORWARD_NSIXMLHTTPREQUESTEVENTTARGET(nsXHREventTarget::)

  
  NS_DECL_NSISTREAMLISTENER

  
  NS_DECL_NSIREQUESTOBSERVER

  
  NS_DECL_NSICHANNELEVENTSINK

  
  NS_DECL_NSIPROGRESSEVENTSINK

  
  NS_DECL_NSIINTERFACEREQUESTOR

  
  NS_DECL_NSITIMERCALLBACK

  
  virtual size_t
    SizeOfEventTargetIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

  NS_FORWARD_NSIDOMEVENTTARGET(nsXHREventTarget::)

#ifdef DEBUG
  void StaticAssertions();
#endif

  
  IMPL_EVENT_HANDLER(readystatechange)

  
  uint16_t ReadyState();

  
  void Open(const nsAString& aMethod, const nsAString& aUrl, bool aAsync,
            const mozilla::dom::Optional<nsAString>& aUser,
            const mozilla::dom::Optional<nsAString>& aPassword,
            ErrorResult& aRv)
  {
    aRv = Open(NS_ConvertUTF16toUTF8(aMethod), NS_ConvertUTF16toUTF8(aUrl),
               aAsync, aUser, aPassword);
  }
  void SetRequestHeader(const nsAString& aHeader, const nsAString& aValue,
                        ErrorResult& aRv)
  {
    aRv = SetRequestHeader(NS_ConvertUTF16toUTF8(aHeader),
                           NS_ConvertUTF16toUTF8(aValue));
  }
  uint32_t Timeout()
  {
    return mTimeoutMilliseconds;
  }
  void SetTimeout(uint32_t aTimeout, ErrorResult& aRv);
  bool WithCredentials();
  void SetWithCredentials(bool aWithCredentials, ErrorResult& aRv);
  nsXMLHttpRequestUpload* Upload();

private:
  class RequestBody
  {
  public:
    RequestBody() : mType(Uninitialized)
    {
    }
    RequestBody(mozilla::dom::ArrayBuffer* aArrayBuffer) : mType(ArrayBuffer)
    {
      mValue.mArrayBuffer = aArrayBuffer;
    }
    RequestBody(mozilla::dom::ArrayBufferView* aArrayBufferView) : mType(ArrayBufferView)
    {
      mValue.mArrayBufferView = aArrayBufferView;
    }
    RequestBody(nsIDOMBlob* aBlob) : mType(Blob)
    {
      mValue.mBlob = aBlob;
    }
    RequestBody(nsIDocument* aDocument) : mType(Document)
    {
      mValue.mDocument = aDocument;
    }
    RequestBody(const nsAString& aString) : mType(DOMString)
    {
      mValue.mString = &aString;
    }
    RequestBody(nsFormData& aFormData) : mType(FormData)
    {
      mValue.mFormData = &aFormData;
    }
    RequestBody(nsIInputStream* aStream) : mType(InputStream)
    {
      mValue.mStream = aStream;
    }

    enum Type {
      Uninitialized,
      ArrayBuffer,
      ArrayBufferView,
      Blob,
      Document,
      DOMString,
      FormData,
      InputStream
    };
    union Value {
      mozilla::dom::ArrayBuffer* mArrayBuffer;
      mozilla::dom::ArrayBufferView* mArrayBufferView;
      nsIDOMBlob* mBlob;
      nsIDocument* mDocument;
      const nsAString* mString;
      nsFormData* mFormData;
      nsIInputStream* mStream;
    };

    Type GetType() const
    {
      MOZ_ASSERT(mType != Uninitialized);
      return mType;
    }
    Value GetValue() const
    {
      MOZ_ASSERT(mType != Uninitialized);
      return mValue;
    }

  private:
    Type mType;
    Value mValue;
  };

  static nsresult GetRequestBody(nsIVariant* aVariant,
                                 const Nullable<RequestBody>& aBody,
                                 nsIInputStream** aResult,
                                 uint64_t* aContentLength,
                                 nsACString& aContentType,
                                 nsACString& aCharset);

  nsresult Send(nsIVariant* aVariant, const Nullable<RequestBody>& aBody);
  nsresult Send(const Nullable<RequestBody>& aBody)
  {
    return Send(nullptr, aBody);
  }
  nsresult Send(const RequestBody& aBody)
  {
    return Send(Nullable<RequestBody>(aBody));
  }

public:
  void Send(ErrorResult& aRv)
  {
    aRv = Send(Nullable<RequestBody>());
  }
  void Send(mozilla::dom::ArrayBuffer& aArrayBuffer, ErrorResult& aRv)
  {
    aRv = Send(RequestBody(&aArrayBuffer));
  }
  void Send(mozilla::dom::ArrayBufferView& aArrayBufferView, ErrorResult& aRv)
  {
    aRv = Send(RequestBody(&aArrayBufferView));
  }
  void Send(nsIDOMBlob* aBlob, ErrorResult& aRv)
  {
    NS_ASSERTION(aBlob, "Null should go to string version");
    aRv = Send(RequestBody(aBlob));
  }
  void Send(nsIDocument& aDoc, ErrorResult& aRv)
  {
    aRv = Send(RequestBody(&aDoc));
  }
  void Send(const nsAString& aString, ErrorResult& aRv)
  {
    if (DOMStringIsNull(aString)) {
      Send(aRv);
    }
    else {
      aRv = Send(RequestBody(aString));
    }
  }
  void Send(nsFormData& aFormData, ErrorResult& aRv)
  {
    aRv = Send(RequestBody(aFormData));
  }
  void Send(nsIInputStream* aStream, ErrorResult& aRv)
  {
    NS_ASSERTION(aStream, "Null should go to string version");
    aRv = Send(RequestBody(aStream));
  }
  void SendAsBinary(const nsAString& aBody, ErrorResult& aRv);

  void Abort();

  
  uint32_t Status();
  void GetStatusText(nsString& aStatusText);
  void GetResponseHeader(const nsACString& aHeader, nsACString& aResult,
                         ErrorResult& aRv);
  void GetResponseHeader(const nsAString& aHeader, nsString& aResult,
                         ErrorResult& aRv)
  {
    nsCString result;
    GetResponseHeader(NS_ConvertUTF16toUTF8(aHeader), result, aRv);
    if (result.IsVoid()) {
      aResult.SetIsVoid(true);
    }
    else {
      
      CopyASCIItoUTF16(result, aResult);
    }
  }
  void GetAllResponseHeaders(nsString& aResponseHeaders);
  bool IsSafeHeader(const nsACString& aHeaderName, nsIHttpChannel* aHttpChannel);
  void OverrideMimeType(const nsAString& aMimeType)
  {
    
    mOverrideMimeType = aMimeType;
  }
  XMLHttpRequestResponseType ResponseType()
  {
    return XMLHttpRequestResponseType(mResponseType);
  }
  void SetResponseType(XMLHttpRequestResponseType aType, ErrorResult& aRv);
  JS::Value GetResponse(JSContext* aCx, ErrorResult& aRv);
  void GetResponseText(nsString& aResponseText, ErrorResult& aRv);
  nsIDocument* GetResponseXML(ErrorResult& aRv);

  bool MozBackgroundRequest();
  void SetMozBackgroundRequest(bool aMozBackgroundRequest, nsresult& aRv);

  bool MozAnon();
  bool MozSystem();

  nsIChannel* GetChannel()
  {
    return mChannel;
  }

  
  JS::Value GetInterface(JSContext* aCx, nsIJSID* aIID, ErrorResult& aRv);

  
  
  nsresult CreateReadystatechangeEvent(nsIDOMEvent** aDOMEvent);
  void DispatchProgressEvent(nsDOMEventTargetHelper* aTarget,
                             const nsAString& aType,
                             bool aLengthComputable,
                             uint64_t aLoaded, uint64_t aTotal);

  
  
  
  void MaybeDispatchProgressEvents(bool aFinalProgress);

  
  nsresult Init();

  void SetRequestObserver(nsIRequestObserver* aObserver);

  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS_INHERITED(nsXMLHttpRequest,
                                                                   nsXHREventTarget)
  bool AllowUploadProgress();
  void RootJSResultObjects();

  virtual void DisconnectFromOwner();

protected:
  nsresult DetectCharset();
  nsresult AppendToResponseText(const char * aBuffer, uint32_t aBufferLen);
  static NS_METHOD StreamReaderFunc(nsIInputStream* in,
                void* closure,
                const char* fromRawSegment,
                uint32_t toOffset,
                uint32_t count,
                uint32_t *writeCount);
  nsresult CreateResponseParsedJSON(JSContext* aCx);
  void CreatePartialBlob();
  bool CreateDOMFile(nsIRequest *request);
  
  
  nsresult ChangeState(uint32_t aState, bool aBroadcast = true);
  already_AddRefed<nsILoadGroup> GetLoadGroup() const;
  nsIURI *GetBaseURI();

  already_AddRefed<nsIHttpChannel> GetCurrentHttpChannel();
  already_AddRefed<nsIJARChannel> GetCurrentJARChannel();

  bool IsSystemXHR();

  void ChangeStateToDone();

  





  nsresult CheckChannelForCrossSiteRequest(nsIChannel* aChannel);

  void StartProgressEventTimer();

  friend class AsyncVerifyRedirectCallbackForwarder;
  void OnRedirectVerifyCallback(nsresult result);

  nsresult Open(const nsACString& method, const nsACString& url, bool async,
                const mozilla::dom::Optional<nsAString>& user,
                const mozilla::dom::Optional<nsAString>& password);

  already_AddRefed<nsXMLHttpRequestXPCOMifier> EnsureXPCOMifier();

  nsCOMPtr<nsISupports> mContext;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<nsIChannel> mChannel;
  nsCOMPtr<nsIDocument> mResponseXML;
  nsCOMPtr<nsIChannel> mCORSPreflightChannel;
  nsTArray<nsCString> mCORSUnsafeHeaders;

  nsCOMPtr<nsIStreamListener> mXMLParserStreamListener;

  
  class nsHeaderVisitor : public nsIHttpHeaderVisitor {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHTTPHEADERVISITOR
    nsHeaderVisitor(nsXMLHttpRequest* aXMLHttpRequest, nsIHttpChannel* aHttpChannel)
      : mXHR(aXMLHttpRequest), mHttpChannel(aHttpChannel) {}
    virtual ~nsHeaderVisitor() {}
    const nsACString &Headers() { return mHeaders; }
  private:
    nsCString mHeaders;
    nsXMLHttpRequest* mXHR;
    nsCOMPtr<nsIHttpChannel> mHttpChannel;
  };

  
  
  nsCString mResponseBody;

  
  
  
  
  
  nsString mResponseText;

  
  
  uint32_t mResponseBodyDecodedPos;

  
  
  
  
  
  
  nsCOMPtr<nsIUnicodeDecoder> mDecoder;

  nsCString mResponseCharset;

  enum ResponseTypeEnum {
    XML_HTTP_RESPONSE_TYPE_DEFAULT,
    XML_HTTP_RESPONSE_TYPE_ARRAYBUFFER,
    XML_HTTP_RESPONSE_TYPE_BLOB,
    XML_HTTP_RESPONSE_TYPE_DOCUMENT,
    XML_HTTP_RESPONSE_TYPE_JSON,
    XML_HTTP_RESPONSE_TYPE_TEXT,
    XML_HTTP_RESPONSE_TYPE_CHUNKED_TEXT,
    XML_HTTP_RESPONSE_TYPE_CHUNKED_ARRAYBUFFER,
    XML_HTTP_RESPONSE_TYPE_MOZ_BLOB
  };

  void SetResponseType(nsXMLHttpRequest::ResponseTypeEnum aType, ErrorResult& aRv);

  ResponseTypeEnum mResponseType;

  
  
  nsCOMPtr<nsIDOMBlob> mResponseBlob;
  
  
  nsRefPtr<nsDOMFile> mDOMFile;
  
  
  nsAutoPtr<BlobSet> mBlobSet;

  nsString mOverrideMimeType;

  



  nsCOMPtr<nsIInterfaceRequestor> mNotificationCallbacks;
  




  nsCOMPtr<nsIChannelEventSink> mChannelEventSink;
  nsCOMPtr<nsIProgressEventSink> mProgressEventSink;

  nsIRequestObserver* mRequestObserver;

  nsCOMPtr<nsIURI> mBaseURI;

  uint32_t mState;

  nsRefPtr<nsXMLHttpRequestUpload> mUpload;
  uint64_t mUploadTransferred;
  uint64_t mUploadTotal;
  bool mUploadLengthComputable;
  bool mUploadComplete;
  bool mProgressSinceLastProgressEvent;

  
  PRTime mRequestSentTime;
  uint32_t mTimeoutMilliseconds;
  nsCOMPtr<nsITimer> mTimeoutTimer;
  void StartTimeoutTimer();
  void HandleTimeoutCallback();

  bool mErrorLoad;
  bool mWaitingForOnStopRequest;
  bool mProgressTimerIsActive;
  bool mProgressEventWasDelayed;
  bool mIsHtml;
  bool mWarnAboutMultipartHtml;
  bool mWarnAboutSyncHtml;
  bool mLoadLengthComputable;
  uint64_t mLoadTotal; 
  uint64_t mLoadTransferred;
  nsCOMPtr<nsITimer> mProgressNotifier;
  void HandleProgressTimerCallback();

  bool mIsSystem;
  bool mIsAnon;

  







  void CloseRequestWithError(const nsAString& aType, const uint32_t aFlag);

  bool mFirstStartRequestSeen;
  bool mInLoadProgressEvent;
  
  nsCOMPtr<nsIAsyncVerifyRedirectCallback> mRedirectCallback;
  nsCOMPtr<nsIChannel> mNewRedirectChannel;
  
  jsval mResultJSON;
  JSObject* mResultArrayBuffer;

  void ResetResponse();

  struct RequestHeader
  {
    nsCString header;
    nsCString value;
  };
  nsTArray<RequestHeader> mModifiedRequestHeaders;

  nsTHashtable<nsCStringHashKey> mAlreadySetHeaders;

  
  nsXMLHttpRequestXPCOMifier* mXPCOMifier;
};



class nsXMLHttpRequestXPCOMifier MOZ_FINAL : public nsIStreamListener,
                                             public nsIChannelEventSink,
                                             public nsIProgressEventSink,
                                             public nsIInterfaceRequestor,
                                             public nsITimerCallback,
                                             public nsCycleCollectionParticipant
{
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXMLHttpRequestXPCOMifier,
                                           nsIStreamListener)

  nsXMLHttpRequestXPCOMifier(nsXMLHttpRequest* aXHR) :
    mXHR(aXHR)
  {
  }

  ~nsXMLHttpRequestXPCOMifier() {
    if (mXHR) {
      mXHR->mXPCOMifier = nullptr;
    }
  }

  NS_FORWARD_NSISTREAMLISTENER(mXHR->)
  NS_FORWARD_NSIREQUESTOBSERVER(mXHR->)
  NS_FORWARD_NSICHANNELEVENTSINK(mXHR->)
  NS_FORWARD_NSIPROGRESSEVENTSINK(mXHR->)
  NS_FORWARD_NSITIMERCALLBACK(mXHR->)

  NS_DECL_NSIINTERFACEREQUESTOR

private:
  nsRefPtr<nsXMLHttpRequest> mXHR;
};

class nsXHRParseEndListener : public nsIDOMEventListener
{
public:
  NS_DECL_ISUPPORTS
  NS_IMETHOD HandleEvent(nsIDOMEvent *event)
  {
    nsCOMPtr<nsIXMLHttpRequest> xhr = do_QueryReferent(mXHR);
    if (xhr) {
      static_cast<nsXMLHttpRequest*>(xhr.get())->ChangeStateToDone();
    }
    mXHR = nullptr;
    return NS_OK;
  }
  nsXHRParseEndListener(nsIXMLHttpRequest* aXHR)
    : mXHR(do_GetWeakReference(aXHR)) {}
  virtual ~nsXHRParseEndListener() {}
private:
  nsWeakPtr mXHR;
};

#endif
