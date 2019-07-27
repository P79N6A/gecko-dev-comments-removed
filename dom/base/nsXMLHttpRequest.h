





#ifndef nsXMLHttpRequest_h__
#define nsXMLHttpRequest_h__

#include "mozilla/Attributes.h"
#include "nsIXMLHttpRequest.h"
#include "nsISupportsUtils.h"
#include "nsString.h"
#include "nsIURI.h"
#include "nsIHttpChannel.h"
#include "nsIDocument.h"
#include "nsIStreamListener.h"
#include "nsWeakReference.h"
#include "nsIChannelEventSink.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsIInterfaceRequestor.h"
#include "nsIHttpHeaderVisitor.h"
#include "nsIProgressEventSink.h"
#include "nsJSUtils.h"
#include "nsTArray.h"
#include "nsITimer.h"
#include "nsIPrincipal.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsISizeOfEventTarget.h"
#include "nsIXPConnect.h"
#include "nsIInputStream.h"
#include "mozilla/Assertions.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/dom/XMLHttpRequestBinding.h"

#ifdef Status


#undef Status
#endif

class AsyncVerifyRedirectCallbackForwarder;
class nsFormData;
class nsIJARChannel;
class nsILoadGroup;
class nsIUnicodeDecoder;
class nsIJSID;

namespace mozilla {

namespace dom {
class BlobSet;
class File;
}









class ArrayBufferBuilder
{
  uint8_t* mDataPtr;
  uint32_t mCapacity;
  uint32_t mLength;
  void* mMapPtr;
public:
  ArrayBufferBuilder();
  ~ArrayBufferBuilder();

  void reset();

  
  bool setCapacity(uint32_t aNewCap);

  
  
  
  
  
  
  
  bool append(const uint8_t* aNewData, uint32_t aDataLen,
              uint32_t aMaxGrowth = 0);

  uint32_t length()   { return mLength; }
  uint32_t capacity() { return mCapacity; }

  JSObject* getArrayBuffer(JSContext* aCx);

  
  
  
  
  
  
  nsresult mapToFileInPackage(const nsCString& aFile, nsIFile* aJarFile);

protected:
  static bool areOverlappingRegions(const uint8_t* aStart1, uint32_t aLength1,
                                    const uint8_t* aStart2, uint32_t aLength2);
};

} 

class nsXHREventTarget : public mozilla::DOMEventTargetHelper,
                         public nsIXMLHttpRequestEventTarget
{
protected:
  explicit nsXHREventTarget(mozilla::DOMEventTargetHelper* aOwner)
    : mozilla::DOMEventTargetHelper(aOwner)
  {
  }

  nsXHREventTarget()
  {
  }

  virtual ~nsXHREventTarget() {}

public:
  typedef mozilla::dom::XMLHttpRequestResponseType
          XMLHttpRequestResponseType;
  typedef mozilla::ErrorResult
          ErrorResult;

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsXHREventTarget,
                                           mozilla::DOMEventTargetHelper)
  NS_DECL_NSIXMLHTTPREQUESTEVENTTARGET
  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(mozilla::DOMEventTargetHelper)

  IMPL_EVENT_HANDLER(loadstart)
  IMPL_EVENT_HANDLER(progress)
  IMPL_EVENT_HANDLER(abort)
  IMPL_EVENT_HANDLER(error)
  IMPL_EVENT_HANDLER(load)
  IMPL_EVENT_HANDLER(timeout)
  IMPL_EVENT_HANDLER(loadend)
  
  virtual void DisconnectFromOwner() override;
};

class nsXMLHttpRequestUpload final : public nsXHREventTarget,
                                     public nsIXMLHttpRequestUpload
{
public:
  explicit nsXMLHttpRequestUpload(mozilla::DOMEventTargetHelper* aOwner)
    : nsXHREventTarget(aOwner)
  {
  }

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIXMLHTTPREQUESTEVENTTARGET(nsXHREventTarget::)
  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(nsXHREventTarget)
  NS_DECL_NSIXMLHTTPREQUESTUPLOAD

  virtual JSObject* WrapObject(JSContext *cx, JS::Handle<JSObject*> aGivenProto) override;
  nsISupports* GetParentObject()
  {
    return GetOwner();
  }

  bool HasListeners()
  {
    return mListenerManager && mListenerManager->HasListeners();
  }

private:
  virtual ~nsXMLHttpRequestUpload() {}
};

class nsXMLHttpRequestXPCOMifier;



class nsXMLHttpRequest final : public nsXHREventTarget,
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

  virtual JSObject* WrapObject(JSContext *cx, JS::Handle<JSObject*> aGivenProto) override
  {
    return mozilla::dom::XMLHttpRequestBinding::Wrap(cx, this, aGivenProto);
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
    nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(aGlobal.GetAsSupports());
    nsCOMPtr<nsIScriptObjectPrincipal> principal =
      do_QueryInterface(aGlobal.GetAsSupports());
    if (!global || ! principal) {
      aRv.Throw(NS_ERROR_FAILURE);
      return nullptr;
    }

    nsRefPtr<nsXMLHttpRequest> req = new nsXMLHttpRequest();
    req->Construct(principal->GetPrincipal(), global);
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
    if (!params.Init(aCx, JS::NullHandleValue)) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }

    return Constructor(aGlobal, aCx, params, aRv);
  }

  void Construct(nsIPrincipal* aPrincipal,
                 nsIGlobalObject* aGlobalObject,
                 nsIURI* aBaseURI = nullptr,
                 nsILoadGroup* aLoadGroup = nullptr)
  {
    MOZ_ASSERT(aPrincipal);
    MOZ_ASSERT_IF(nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(
      aGlobalObject), win->IsInnerWindow());
    mPrincipal = aPrincipal;
    BindToOwner(aGlobalObject);
    mBaseURI = aBaseURI;
    mLoadGroup = aLoadGroup;
  }

  void InitParameters(bool aAnon, bool aSystem);

  void SetParameters(bool aAnon, bool aSystem)
  {
    mIsAnon = aAnon || aSystem;
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
    SizeOfEventTargetIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const override;

  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(nsXHREventTarget)

#ifdef DEBUG
  void StaticAssertions();
#endif

  
  IMPL_EVENT_HANDLER(readystatechange)

  
  uint16_t ReadyState();

  
  void Open(const nsACString& aMethod, const nsAString& aUrl, ErrorResult& aRv)
  {
    Open(aMethod, aUrl, true,
         mozilla::dom::Optional<nsAString>(),
         mozilla::dom::Optional<nsAString>(),
         aRv);
  }
  void Open(const nsACString& aMethod, const nsAString& aUrl, bool aAsync,
            const mozilla::dom::Optional<nsAString>& aUser,
            const mozilla::dom::Optional<nsAString>& aPassword,
            ErrorResult& aRv)
  {
    aRv = Open(aMethod, NS_ConvertUTF16toUTF8(aUrl),
               aAsync, aUser, aPassword);
  }
  void SetRequestHeader(const nsACString& aHeader, const nsACString& aValue,
                        ErrorResult& aRv)
  {
    aRv = SetRequestHeader(aHeader, aValue);
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
  virtual ~nsXMLHttpRequest();

  class RequestBody
  {
  public:
    RequestBody() : mType(Uninitialized)
    {
    }
    explicit RequestBody(const mozilla::dom::ArrayBuffer* aArrayBuffer) : mType(ArrayBuffer)
    {
      mValue.mArrayBuffer = aArrayBuffer;
    }
    explicit RequestBody(const mozilla::dom::ArrayBufferView* aArrayBufferView) : mType(ArrayBufferView)
    {
      mValue.mArrayBufferView = aArrayBufferView;
    }
    explicit RequestBody(mozilla::dom::File& aBlob) : mType(Blob)
    {
      mValue.mBlob = &aBlob;
    }
    explicit RequestBody(nsIDocument* aDocument) : mType(Document)
    {
      mValue.mDocument = aDocument;
    }
    explicit RequestBody(const nsAString& aString) : mType(DOMString)
    {
      mValue.mString = &aString;
    }
    explicit RequestBody(nsFormData& aFormData) : mType(FormData)
    {
      mValue.mFormData = &aFormData;
    }
    explicit RequestBody(nsIInputStream* aStream) : mType(InputStream)
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
      const mozilla::dom::ArrayBuffer* mArrayBuffer;
      const mozilla::dom::ArrayBufferView* mArrayBufferView;
      mozilla::dom::File* mBlob;
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

  bool IsDeniedCrossSiteRequest();

  
  
  
  void PopulateNetworkInterfaceId();

public:
  void Send(JSContext* , ErrorResult& aRv)
  {
    aRv = Send(Nullable<RequestBody>());
  }
  void Send(JSContext* ,
            const mozilla::dom::ArrayBuffer& aArrayBuffer,
            ErrorResult& aRv)
  {
    aRv = Send(RequestBody(&aArrayBuffer));
  }
  void Send(JSContext* ,
            const mozilla::dom::ArrayBufferView& aArrayBufferView,
            ErrorResult& aRv)
  {
    aRv = Send(RequestBody(&aArrayBufferView));
  }
  void Send(JSContext* , mozilla::dom::File& aBlob, ErrorResult& aRv)
  {
    aRv = Send(RequestBody(aBlob));
  }
  void Send(JSContext* , nsIDocument& aDoc, ErrorResult& aRv)
  {
    aRv = Send(RequestBody(&aDoc));
  }
  void Send(JSContext* aCx, const nsAString& aString, ErrorResult& aRv)
  {
    if (DOMStringIsNull(aString)) {
      Send(aCx, aRv);
    }
    else {
      aRv = Send(RequestBody(aString));
    }
  }
  void Send(JSContext* , nsFormData& aFormData, ErrorResult& aRv)
  {
    aRv = Send(RequestBody(aFormData));
  }
  void Send(JSContext* aCx, nsIInputStream* aStream, ErrorResult& aRv)
  {
    NS_ASSERTION(aStream, "Null should go to string version");
    nsCOMPtr<nsIXPConnectWrappedJS> wjs = do_QueryInterface(aStream);
    if (wjs) {
      JSObject* data = wjs->GetJSObject();
      if (!data) {
        aRv.Throw(NS_ERROR_DOM_TYPE_ERR);
        return;
      }
      JS::Rooted<JS::Value> dataAsValue(aCx, JS::ObjectValue(*data));
      nsAutoString dataAsString;
      if (ConvertJSValueToString(aCx, dataAsValue, mozilla::dom::eNull,
                                 mozilla::dom::eNull, dataAsString)) {
        Send(aCx, dataAsString, aRv);
      } else {
        aRv.Throw(NS_ERROR_FAILURE);
      }
      return;
    }
    aRv = Send(RequestBody(aStream));
  }

  void Abort();

  
  void GetResponseURL(nsAString& aUrl);
  uint32_t Status();
  void GetStatusText(nsCString& aStatusText);
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
  void GetAllResponseHeaders(nsCString& aResponseHeaders);
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
  void GetResponse(JSContext* aCx, JS::MutableHandle<JS::Value> aResponse,
                   ErrorResult& aRv);
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

  void GetNetworkInterfaceId(nsACString& aId) const
  {
    aId = mNetworkInterfaceId;
  }

  void SetNetworkInterfaceId(const nsACString& aId)
  {
    mNetworkInterfaceId = aId;
  }

  
  void GetInterface(JSContext* aCx, nsIJSID* aIID,
                    JS::MutableHandle<JS::Value> aRetval, ErrorResult& aRv);

  
  
  nsresult CreateReadystatechangeEvent(nsIDOMEvent** aDOMEvent);
  void DispatchProgressEvent(mozilla::DOMEventTargetHelper* aTarget,
                             const nsAString& aType,
                             bool aLengthComputable,
                             int64_t aLoaded, int64_t aTotal);

  
  
  
  void MaybeDispatchProgressEvents(bool aFinalProgress);

  
  nsresult Init();

  nsresult init(nsIPrincipal* principal,
                nsIScriptContext* scriptContext,
                nsPIDOMWindow* globalObject,
                nsIURI* baseURI);

  void SetRequestObserver(nsIRequestObserver* aObserver);

  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS_INHERITED(nsXMLHttpRequest,
                                                                   nsXHREventTarget)
  bool AllowUploadProgress();
  void RootJSResultObjects();

  virtual void DisconnectFromOwner() override;

  static void SetDontWarnAboutSyncXHR(bool aVal)
  {
    sDontWarnAboutSyncXHR = aVal;
  }
  static bool DontWarnAboutSyncXHR()
  {
    return sDontWarnAboutSyncXHR;
  }
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

  
  class nsHeaderVisitor : public nsIHttpHeaderVisitor
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIHTTPHEADERVISITOR
    nsHeaderVisitor(nsXMLHttpRequest* aXMLHttpRequest, nsIHttpChannel* aHttpChannel)
      : mXHR(aXMLHttpRequest), mHttpChannel(aHttpChannel) {}
    const nsACString &Headers() { return mHeaders; }
  private:
    virtual ~nsHeaderVisitor() {}

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

  
  
  nsRefPtr<mozilla::dom::File> mResponseBlob;
  
  
  nsRefPtr<mozilla::dom::File> mDOMFile;
  
  
  nsAutoPtr<mozilla::dom::BlobSet> mBlobSet;

  nsString mOverrideMimeType;

  



  nsCOMPtr<nsIInterfaceRequestor> mNotificationCallbacks;
  




  nsCOMPtr<nsIChannelEventSink> mChannelEventSink;
  nsCOMPtr<nsIProgressEventSink> mProgressEventSink;

  nsIRequestObserver* mRequestObserver;

  nsCOMPtr<nsIURI> mBaseURI;
  nsCOMPtr<nsILoadGroup> mLoadGroup;

  uint32_t mState;

  nsRefPtr<nsXMLHttpRequestUpload> mUpload;
  int64_t mUploadTransferred;
  int64_t mUploadTotal;
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
  bool mIsHtml;
  bool mWarnAboutMultipartHtml;
  bool mWarnAboutSyncHtml;
  bool mLoadLengthComputable;
  int64_t mLoadTotal; 
  
  
  uint64_t mDataAvailable;
  
  
  
  
  
  
  
  int64_t mLoadTransferred;
  nsCOMPtr<nsITimer> mProgressNotifier;
  void HandleProgressTimerCallback();

  bool mIsSystem;
  bool mIsAnon;

  
  
  nsCString mNetworkInterfaceId;

  







  void CloseRequestWithError(const nsAString& aType, const uint32_t aFlag);

  bool mFirstStartRequestSeen;
  bool mInLoadProgressEvent;

  nsCOMPtr<nsIAsyncVerifyRedirectCallback> mRedirectCallback;
  nsCOMPtr<nsIChannel> mNewRedirectChannel;

  JS::Heap<JS::Value> mResultJSON;

  mozilla::ArrayBufferBuilder mArrayBufferBuilder;
  JS::Heap<JSObject*> mResultArrayBuffer;
  bool mIsMappedArrayBuffer;

  void ResetResponse();

  struct RequestHeader
  {
    nsCString header;
    nsCString value;
  };
  nsTArray<RequestHeader> mModifiedRequestHeaders;

  nsTHashtable<nsCStringHashKey> mAlreadySetHeaders;

  
  nsXMLHttpRequestXPCOMifier* mXPCOMifier;

  static bool sDontWarnAboutSyncXHR;
};

class MOZ_STACK_CLASS AutoDontWarnAboutSyncXHR
{
public:
  AutoDontWarnAboutSyncXHR() : mOldVal(nsXMLHttpRequest::DontWarnAboutSyncXHR())
  {
    nsXMLHttpRequest::SetDontWarnAboutSyncXHR(true);
  }

  ~AutoDontWarnAboutSyncXHR()
  {
    nsXMLHttpRequest::SetDontWarnAboutSyncXHR(mOldVal);
  }

private:
  bool mOldVal;
};



class nsXMLHttpRequestXPCOMifier final : public nsIStreamListener,
                                         public nsIChannelEventSink,
                                         public nsIProgressEventSink,
                                         public nsIInterfaceRequestor,
                                         public nsITimerCallback
{
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXMLHttpRequestXPCOMifier,
                                           nsIStreamListener)

  explicit nsXMLHttpRequestXPCOMifier(nsXMLHttpRequest* aXHR) :
    mXHR(aXHR)
  {
  }

private:
  ~nsXMLHttpRequestXPCOMifier() {
    if (mXHR) {
      mXHR->mXPCOMifier = nullptr;
    }
  }

public:
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
  NS_IMETHOD HandleEvent(nsIDOMEvent *event) override
  {
    nsCOMPtr<nsIXMLHttpRequest> xhr = do_QueryReferent(mXHR);
    if (xhr) {
      static_cast<nsXMLHttpRequest*>(xhr.get())->ChangeStateToDone();
    }
    mXHR = nullptr;
    return NS_OK;
  }
  explicit nsXHRParseEndListener(nsIXMLHttpRequest* aXHR)
    : mXHR(do_GetWeakReference(aXHR)) {}
private:
  virtual ~nsXHRParseEndListener() {}

  nsWeakPtr mXHR;
};

#endif
