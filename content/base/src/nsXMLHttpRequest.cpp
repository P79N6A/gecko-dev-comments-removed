





#include "nsXMLHttpRequest.h"

#include "mozilla/dom/XMLHttpRequestUploadBinding.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Util.h"
#include "nsDOMBlobBuilder.h"
#include "nsICharsetConverterManager.h"
#include "nsIDOMDocument.h"
#include "nsIDOMProgressEvent.h"
#include "nsIJARChannel.h"
#include "nsLayoutCID.h"
#include "nsReadableUtils.h"

#include "nsIURI.h"
#include "nsILoadGroup.h"
#include "nsNetUtil.h"
#include "nsStreamUtils.h"
#include "nsThreadUtils.h"
#include "nsIUploadChannel.h"
#include "nsIUploadChannel2.h"
#include "nsIDOMSerializer.h"
#include "nsXPCOM.h"
#include "nsIDOMEventListener.h"
#include "nsIScriptSecurityManager.h"
#include "nsIDOMWindow.h"
#include "nsIVariant.h"
#include "nsVariant.h"
#include "nsIScriptError.h"
#include "nsIStreamConverterService.h"
#include "nsICachingChannel.h"
#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "nsEventDispatcher.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsError.h"
#include "nsCrossSiteListenerProxy.h"
#include "nsIHTMLDocument.h"
#include "nsIStorageStream.h"
#include "nsIPromptFactory.h"
#include "nsIWindowWatcher.h"
#include "nsIConsoleService.h"
#include "nsIChannelPolicy.h"
#include "nsChannelPolicy.h"
#include "nsIContentSecurityPolicy.h"
#include "nsAsyncRedirectVerifyHelper.h"
#include "nsStringBuffer.h"
#include "nsDOMFile.h"
#include "nsIFileChannel.h"
#include "mozilla/Telemetry.h"
#include "jsfriendapi.h"
#include "GeckoProfiler.h"
#include "mozilla/dom/EncodingUtils.h"
#include "mozilla/dom/XMLHttpRequestBinding.h"
#include "mozilla/Attributes.h"
#include "nsIPermissionManager.h"
#include "nsMimeTypes.h"
#include "nsIHttpChannelInternal.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsFormData.h"
#include "nsStreamListenerWrapper.h"
#include "xpcjsid.h"

#include "nsWrapperCacheInlines.h"

using namespace mozilla;
using namespace mozilla::dom;



#define XML_HTTP_REQUEST_ARRAYBUFFER_MAX_GROWTH (32*1024*1024)

#define XML_HTTP_REQUEST_ARRAYBUFFER_MIN_SIZE (32*1024)


#define XML_HTTP_REQUEST_MAX_CONTENT_LENGTH_PREALLOCATE (1*1024*1024*1024LL)

#define LOAD_STR "load"
#define ERROR_STR "error"
#define ABORT_STR "abort"
#define TIMEOUT_STR "timeout"
#define LOADSTART_STR "loadstart"
#define PROGRESS_STR "progress"
#define READYSTATE_STR "readystatechange"
#define LOADEND_STR "loadend"




#define XML_HTTP_REQUEST_UNSENT           (1 << 0) // 0 UNSENT
#define XML_HTTP_REQUEST_OPENED           (1 << 1) // 1 OPENED
#define XML_HTTP_REQUEST_HEADERS_RECEIVED (1 << 2) // 2 HEADERS_RECEIVED
#define XML_HTTP_REQUEST_LOADING          (1 << 3) // 3 LOADING
#define XML_HTTP_REQUEST_DONE             (1 << 4) // 4 DONE
#define XML_HTTP_REQUEST_SENT             (1 << 5) // Internal, OPENED in IE and external view


#define XML_HTTP_REQUEST_ABORTED        (1 << 7)  // Internal
#define XML_HTTP_REQUEST_ASYNC          (1 << 8)  // Internal
#define XML_HTTP_REQUEST_PARSEBODY      (1 << 9)  // Internal
#define XML_HTTP_REQUEST_SYNCLOOPING    (1 << 10) // Internal
#define XML_HTTP_REQUEST_BACKGROUND     (1 << 13) // Internal
#define XML_HTTP_REQUEST_USE_XSITE_AC   (1 << 14) // Internal
#define XML_HTTP_REQUEST_NEED_AC_PREFLIGHT (1 << 15) // Internal
#define XML_HTTP_REQUEST_AC_WITH_CREDENTIALS (1 << 16) // Internal
#define XML_HTTP_REQUEST_TIMED_OUT (1 << 17) // Internal
#define XML_HTTP_REQUEST_DELETED (1 << 18) // Internal

#define XML_HTTP_REQUEST_LOADSTATES         \
  (XML_HTTP_REQUEST_UNSENT |                \
   XML_HTTP_REQUEST_OPENED |                \
   XML_HTTP_REQUEST_HEADERS_RECEIVED |      \
   XML_HTTP_REQUEST_LOADING |               \
   XML_HTTP_REQUEST_DONE |                  \
   XML_HTTP_REQUEST_SENT)

#define NS_BADCERTHANDLER_CONTRACTID \
  "@mozilla.org/content/xmlhttprequest-bad-cert-handler;1"

#define NS_PROGRESS_EVENT_INTERVAL 50

#define IMPL_CSTRING_GETTER(_name)                                              \
  NS_IMETHODIMP                                                                 \
  nsXMLHttpRequest::_name(nsACString& aOut)                                     \
  {                                                                             \
    nsCString tmp;                                                              \
    _name(tmp);                                                                 \
    aOut = tmp;                                                                 \
    return NS_OK;                                                               \
  }

NS_IMPL_ISUPPORTS1(nsXHRParseEndListener, nsIDOMEventListener)

class nsResumeTimeoutsEvent : public nsRunnable
{
public:
  nsResumeTimeoutsEvent(nsPIDOMWindow* aWindow) : mWindow(aWindow) {}

  NS_IMETHOD Run()
  {
    mWindow->ResumeTimeouts(false);
    return NS_OK;
  }

private:
  nsCOMPtr<nsPIDOMWindow> mWindow;
};




static void AddLoadFlags(nsIRequest *request, nsLoadFlags newFlags)
{
  nsLoadFlags flags;
  request->GetLoadFlags(&flags);
  flags |= newFlags;
  request->SetLoadFlags(flags);
}





class XMLHttpRequestAuthPrompt : public nsIAuthPrompt
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAUTHPROMPT

  XMLHttpRequestAuthPrompt();
  virtual ~XMLHttpRequestAuthPrompt();
};

NS_IMPL_ISUPPORTS1(XMLHttpRequestAuthPrompt, nsIAuthPrompt)

XMLHttpRequestAuthPrompt::XMLHttpRequestAuthPrompt()
{
  MOZ_COUNT_CTOR(XMLHttpRequestAuthPrompt);
}

XMLHttpRequestAuthPrompt::~XMLHttpRequestAuthPrompt()
{
  MOZ_COUNT_DTOR(XMLHttpRequestAuthPrompt);
}

NS_IMETHODIMP
XMLHttpRequestAuthPrompt::Prompt(const PRUnichar* aDialogTitle,
                                 const PRUnichar* aText,
                                 const PRUnichar* aPasswordRealm,
                                 uint32_t aSavePassword,
                                 const PRUnichar* aDefaultText,
                                 PRUnichar** aResult,
                                 bool* aRetval)
{
  *aRetval = false;
  return NS_OK;
}

NS_IMETHODIMP
XMLHttpRequestAuthPrompt::PromptUsernameAndPassword(const PRUnichar* aDialogTitle,
                                                    const PRUnichar* aDialogText,
                                                    const PRUnichar* aPasswordRealm,
                                                    uint32_t aSavePassword,
                                                    PRUnichar** aUser,
                                                    PRUnichar** aPwd,
                                                    bool* aRetval)
{
  *aRetval = false;
  return NS_OK;
}

NS_IMETHODIMP
XMLHttpRequestAuthPrompt::PromptPassword(const PRUnichar* aDialogTitle,
                                         const PRUnichar* aText,
                                         const PRUnichar* aPasswordRealm,
                                         uint32_t aSavePassword,
                                         PRUnichar** aPwd,
                                         bool* aRetval)
{
  *aRetval = false;
  return NS_OK;
}



NS_IMPL_CYCLE_COLLECTION_CLASS(nsXHREventTarget)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsXHREventTarget,
                                                  nsDOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsXHREventTarget,
                                                nsDOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsXHREventTarget)
  NS_INTERFACE_MAP_ENTRY(nsIXMLHttpRequestEventTarget)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(nsXHREventTarget, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(nsXHREventTarget, nsDOMEventTargetHelper)

void
nsXHREventTarget::DisconnectFromOwner()
{
  nsDOMEventTargetHelper::DisconnectFromOwner();
}



NS_INTERFACE_MAP_BEGIN(nsXMLHttpRequestUpload)
  NS_INTERFACE_MAP_ENTRY(nsIXMLHttpRequestUpload)
NS_INTERFACE_MAP_END_INHERITING(nsXHREventTarget)

NS_IMPL_ADDREF_INHERITED(nsXMLHttpRequestUpload, nsXHREventTarget)
NS_IMPL_RELEASE_INHERITED(nsXMLHttpRequestUpload, nsXHREventTarget)

 JSObject*
nsXMLHttpRequestUpload::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return XMLHttpRequestUploadBinding::Wrap(aCx, aScope, this);
}






nsXMLHttpRequest::nsXMLHttpRequest()
  : mResponseBodyDecodedPos(0),
    mResponseType(XML_HTTP_RESPONSE_TYPE_DEFAULT),
    mRequestObserver(nullptr), mState(XML_HTTP_REQUEST_UNSENT),
    mUploadTransferred(0), mUploadTotal(0), mUploadComplete(true),
    mProgressSinceLastProgressEvent(false),
    mRequestSentTime(0), mTimeoutMilliseconds(0),
    mErrorLoad(false), mWaitingForOnStopRequest(false),
    mProgressTimerIsActive(false),
    mIsHtml(false),
    mWarnAboutSyncHtml(false),
    mLoadLengthComputable(false), mLoadTotal(0),
    mIsSystem(false),
    mIsAnon(false),
    mFirstStartRequestSeen(false),
    mInLoadProgressEvent(false),
    mResultJSON(JSVAL_VOID),
    mResultArrayBuffer(nullptr),
    mXPCOMifier(nullptr)
{
  SetIsDOMBinding();
#ifdef DEBUG
  StaticAssertions();
#endif
}

nsXMLHttpRequest::~nsXMLHttpRequest()
{
  mState |= XML_HTTP_REQUEST_DELETED;

  if (mState & (XML_HTTP_REQUEST_SENT |
                XML_HTTP_REQUEST_LOADING)) {
    Abort();
  }

  NS_ABORT_IF_FALSE(!(mState & XML_HTTP_REQUEST_SYNCLOOPING), "we rather crash than hang");
  mState &= ~XML_HTTP_REQUEST_SYNCLOOPING;

  mResultJSON = JSVAL_VOID;
  mResultArrayBuffer = nullptr;
  mozilla::DropJSObjects(this);
}

void
nsXMLHttpRequest::RootJSResultObjects()
{
  mozilla::HoldJSObjects(this);
}




nsresult
nsXMLHttpRequest::Init()
{
  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  if (secMan) {
    secMan->GetSystemPrincipal(getter_AddRefs(subjectPrincipal));
  }
  NS_ENSURE_STATE(subjectPrincipal);

  
  
  
  nsCOMPtr<nsIGlobalObject> global = xpc::GetJunkScopeGlobal();
  NS_ENSURE_TRUE(global, NS_ERROR_FAILURE);
  Construct(subjectPrincipal, global);
  return NS_OK;
}




NS_IMETHODIMP
nsXMLHttpRequest::Init(nsIPrincipal* aPrincipal,
                       nsIScriptContext* aScriptContext,
                       nsIGlobalObject* aGlobalObject,
                       nsIURI* aBaseURI)
{
  NS_ENSURE_ARG_POINTER(aPrincipal);
  
  if (nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(aGlobalObject)) {
    if (win->IsOuterWindow()) {
      
      nsCOMPtr<nsIGlobalObject> inner = do_QueryInterface(
        win->GetCurrentInnerWindow());
      aGlobalObject = inner.get();
    }
  }

  Construct(aPrincipal, aGlobalObject, aBaseURI);
  return NS_OK;
}

void
nsXMLHttpRequest::InitParameters(bool aAnon, bool aSystem)
{
  if (!aAnon && !aSystem) {
    return;
  }

  
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(GetOwner());
  if (!window || !window->GetDocShell()) {
    return;
  }

  
  
  if (!IsSystemXHR()) {
    nsCOMPtr<nsIDocument> doc = window->GetExtantDoc();
    if (!doc) {
      return;
    }

    nsCOMPtr<nsIPrincipal> principal = doc->NodePrincipal();
    nsCOMPtr<nsIPermissionManager> permMgr =
      do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
    if (!permMgr)
      return;

    uint32_t permission;
    nsresult rv =
      permMgr->TestPermissionFromPrincipal(principal, "systemXHR", &permission);
    if (NS_FAILED(rv) || permission != nsIPermissionManager::ALLOW_ACTION) {
      return;
    }
  }

  SetParameters(aAnon, aSystem);
}

void
nsXMLHttpRequest::ResetResponse()
{
  mResponseXML = nullptr;
  mResponseBody.Truncate();
  mResponseText.Truncate();
  mResponseBlob = nullptr;
  mDOMFile = nullptr;
  mBlobSet = nullptr;
  mResultArrayBuffer = nullptr;
  mArrayBufferBuilder.reset();
  mResultJSON = JSVAL_VOID;
  mLoadTransferred = 0;
  mResponseBodyDecodedPos = 0;
}

void
nsXMLHttpRequest::SetRequestObserver(nsIRequestObserver* aObserver)
{
  mRequestObserver = aObserver;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXMLHttpRequest)

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_BEGIN(nsXMLHttpRequest)
  bool isBlack = tmp->IsBlack();
  if (isBlack || tmp->mWaitingForOnStopRequest) {
    if (tmp->mListenerManager) {
      tmp->mListenerManager->MarkForCC();
    }
    if (!isBlack && tmp->PreservingWrapper()) {
      
      tmp->GetWrapper();
    }
    return true;
  }
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_BEGIN(nsXMLHttpRequest)
  return tmp->
    IsBlackAndDoesNotNeedTracing(static_cast<nsDOMEventTargetHelper*>(tmp));
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_BEGIN(nsXMLHttpRequest)
  return tmp->IsBlack();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsXMLHttpRequest,
                                                  nsXHREventTarget)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mChannel)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mResponseXML)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCORSPreflightChannel)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mXMLParserStreamListener)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mChannelEventSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mProgressEventSink)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mUpload)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsXMLHttpRequest,
                                                nsXHREventTarget)
  tmp->mResultArrayBuffer = nullptr;
  tmp->mArrayBufferBuilder.reset();
  tmp->mResultJSON = JSVAL_VOID;
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mContext)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mChannel)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mResponseXML)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mCORSPreflightChannel)

  NS_IMPL_CYCLE_COLLECTION_UNLINK(mXMLParserStreamListener)

  NS_IMPL_CYCLE_COLLECTION_UNLINK(mChannelEventSink)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mProgressEventSink)

  NS_IMPL_CYCLE_COLLECTION_UNLINK(mUpload)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(nsXMLHttpRequest,
                                               nsXHREventTarget)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mResultArrayBuffer)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JSVAL_MEMBER_CALLBACK(mResultJSON)
NS_IMPL_CYCLE_COLLECTION_TRACE_END


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsXMLHttpRequest)
  NS_INTERFACE_MAP_ENTRY(nsIXMLHttpRequest)
  NS_INTERFACE_MAP_ENTRY(nsIJSXMLHttpRequest)
  NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
  NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
  NS_INTERFACE_MAP_ENTRY(nsIChannelEventSink)
  NS_INTERFACE_MAP_ENTRY(nsIProgressEventSink)
  NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
  NS_INTERFACE_MAP_ENTRY(nsISizeOfEventTarget)
NS_INTERFACE_MAP_END_INHERITING(nsXHREventTarget)

NS_IMPL_ADDREF_INHERITED(nsXMLHttpRequest, nsXHREventTarget)
NS_IMPL_RELEASE_INHERITED(nsXMLHttpRequest, nsXHREventTarget)

NS_IMPL_EVENT_HANDLER(nsXMLHttpRequest, readystatechange)

void
nsXMLHttpRequest::DisconnectFromOwner()
{
  nsXHREventTarget::DisconnectFromOwner();
  Abort();
}

size_t
nsXMLHttpRequest::SizeOfEventTargetIncludingThis(
  MallocSizeOf aMallocSizeOf) const
{
  size_t n = aMallocSizeOf(this);
  n += mResponseBody.SizeOfExcludingThisIfUnshared(aMallocSizeOf);

  
  
  
  
  
  
  
  
  
  
  
  n += mResponseText.SizeOfExcludingThisEvenIfShared(aMallocSizeOf);

  return n;

  
  
  
}


NS_IMETHODIMP
nsXMLHttpRequest::GetChannel(nsIChannel **aChannel)
{
  NS_ENSURE_ARG_POINTER(aChannel);
  NS_IF_ADDREF(*aChannel = mChannel);

  return NS_OK;
}

static void LogMessage(const char* aWarning, nsPIDOMWindow* aWindow)
{
  nsCOMPtr<nsIDocument> doc;
  if (aWindow) {
    doc = aWindow->GetExtantDoc();
  }
  nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                  NS_LITERAL_CSTRING("DOM"), doc,
                                  nsContentUtils::eDOM_PROPERTIES,
                                  aWarning);
}


NS_IMETHODIMP
nsXMLHttpRequest::GetResponseXML(nsIDOMDocument **aResponseXML)
{
  ErrorResult rv;
  nsIDocument* responseXML = GetResponseXML(rv);
  if (rv.Failed()) {
    return rv.ErrorCode();
  }

  if (!responseXML) {
    *aResponseXML = nullptr;
    return NS_OK;
  }

  return CallQueryInterface(responseXML, aResponseXML);
}

nsIDocument*
nsXMLHttpRequest::GetResponseXML(ErrorResult& aRv)
{
  if (mResponseType != XML_HTTP_RESPONSE_TYPE_DEFAULT &&
      mResponseType != XML_HTTP_RESPONSE_TYPE_DOCUMENT) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return nullptr;
  }
  if (mWarnAboutSyncHtml) {
    mWarnAboutSyncHtml = false;
    LogMessage("HTMLSyncXHRWarning", GetOwner());
  }
  return (XML_HTTP_REQUEST_DONE & mState) ? mResponseXML : nullptr;
}





nsresult
nsXMLHttpRequest::DetectCharset()
{
  mResponseCharset.Truncate();
  mDecoder = nullptr;

  if (mResponseType != XML_HTTP_RESPONSE_TYPE_DEFAULT &&
      mResponseType != XML_HTTP_RESPONSE_TYPE_TEXT &&
      mResponseType != XML_HTTP_RESPONSE_TYPE_JSON &&
      mResponseType != XML_HTTP_RESPONSE_TYPE_CHUNKED_TEXT) {
    return NS_OK;
  }

  nsAutoCString charsetVal;
  bool ok = mChannel &&
            NS_SUCCEEDED(mChannel->GetContentCharset(charsetVal)) &&
            EncodingUtils::FindEncodingForLabel(charsetVal, mResponseCharset);
  if (!ok || mResponseCharset.IsEmpty()) {
    
    mResponseCharset.AssignLiteral("UTF-8");
  }

  if (mResponseType == XML_HTTP_RESPONSE_TYPE_JSON &&
      !mResponseCharset.EqualsLiteral("UTF-8")) {
    
    LogMessage("JSONCharsetWarning", GetOwner());
    mResponseCharset.AssignLiteral("UTF-8");
  }

  nsresult rv;
  nsCOMPtr<nsICharsetConverterManager> ccm =
    do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return ccm->GetUnicodeDecoderRaw(mResponseCharset.get(),
                                   getter_AddRefs(mDecoder));
}

nsresult
nsXMLHttpRequest::AppendToResponseText(const char * aSrcBuffer,
                                       uint32_t aSrcBufferLen)
{
  NS_ENSURE_STATE(mDecoder);

  int32_t destBufferLen;
  nsresult rv = mDecoder->GetMaxLength(aSrcBuffer, aSrcBufferLen,
                                       &destBufferLen);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mResponseText.SetCapacity(mResponseText.Length() + destBufferLen, fallible_t())) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  PRUnichar* destBuffer = mResponseText.BeginWriting() + mResponseText.Length();

  int32_t totalChars = mResponseText.Length();

  
  
  int32_t srclen = (int32_t)aSrcBufferLen;
  int32_t destlen = (int32_t)destBufferLen;
  rv = mDecoder->Convert(aSrcBuffer,
                         &srclen,
                         destBuffer,
                         &destlen);
  MOZ_ASSERT(NS_SUCCEEDED(rv));

  totalChars += destlen;

  mResponseText.SetLength(totalChars);

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetResponseText(nsAString& aResponseText)
{
  ErrorResult rv;
  nsString responseText;
  GetResponseText(responseText, rv);
  aResponseText = responseText;
  return rv.ErrorCode();
}

void
nsXMLHttpRequest::GetResponseText(nsString& aResponseText, ErrorResult& aRv)
{
  aResponseText.Truncate();

  if (mResponseType != XML_HTTP_RESPONSE_TYPE_DEFAULT &&
      mResponseType != XML_HTTP_RESPONSE_TYPE_TEXT &&
      mResponseType != XML_HTTP_RESPONSE_TYPE_CHUNKED_TEXT) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  if (mResponseType == XML_HTTP_RESPONSE_TYPE_CHUNKED_TEXT &&
      !mInLoadProgressEvent) {
    aResponseText.SetIsVoid(true);
    return;
  }

  if (!(mState & (XML_HTTP_REQUEST_DONE | XML_HTTP_REQUEST_LOADING))) {
    return;
  }

  
  
  
  if (!mResponseXML ||
      mResponseBodyDecodedPos == mResponseBody.Length()) {
    aResponseText = mResponseText;
    return;
  }

  if (mResponseCharset != mResponseXML->GetDocumentCharacterSet()) {
    mResponseCharset = mResponseXML->GetDocumentCharacterSet();
    mResponseText.Truncate();
    mResponseBodyDecodedPos = 0;

    nsresult rv;
    nsCOMPtr<nsICharsetConverterManager> ccm =
      do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) {
      aRv.Throw(rv);
      return;
    }

    aRv = ccm->GetUnicodeDecoderRaw(mResponseCharset.get(),
                                    getter_AddRefs(mDecoder));
    if (aRv.Failed()) {
      return;
    }
  }

  NS_ASSERTION(mResponseBodyDecodedPos < mResponseBody.Length(),
               "Unexpected mResponseBodyDecodedPos");
  aRv = AppendToResponseText(mResponseBody.get() + mResponseBodyDecodedPos,
                             mResponseBody.Length() - mResponseBodyDecodedPos);
  if (aRv.Failed()) {
    return;
  }

  mResponseBodyDecodedPos = mResponseBody.Length();
  
  if (mState & XML_HTTP_REQUEST_DONE) {
    
    mResponseBody.Truncate();
    mResponseBodyDecodedPos = 0;
  }

  aResponseText = mResponseText;
}

nsresult
nsXMLHttpRequest::CreateResponseParsedJSON(JSContext* aCx)
{
  if (!aCx) {
    return NS_ERROR_FAILURE;
  }
  RootJSResultObjects();

  
  JS::Rooted<JS::Value> value(aCx);
  if (!JS_ParseJSON(aCx,
                    static_cast<const jschar*>(mResponseText.get()), mResponseText.Length(),
                    &value)) {
    return NS_ERROR_FAILURE;
  }

  mResultJSON = value;
  return NS_OK;
}

void
nsXMLHttpRequest::CreatePartialBlob()
{
  if (mDOMFile) {
    if (mLoadTotal == mLoadTransferred) {
      mResponseBlob = mDOMFile;
    } else {
      mResponseBlob =
        mDOMFile->CreateSlice(0, mLoadTransferred, EmptyString());
    }
    return;
  }

  
  if (!mBlobSet) {
    return;
  }

  nsAutoCString contentType;
  if (mLoadTotal == mLoadTransferred) {
    mChannel->GetContentType(contentType);
  }

  mResponseBlob = mBlobSet->GetBlobInternal(contentType);
}


NS_IMETHODIMP nsXMLHttpRequest::GetResponseType(nsAString& aResponseType)
{
  switch (mResponseType) {
  case XML_HTTP_RESPONSE_TYPE_DEFAULT:
    aResponseType.Truncate();
    break;
  case XML_HTTP_RESPONSE_TYPE_ARRAYBUFFER:
    aResponseType.AssignLiteral("arraybuffer");
    break;
  case XML_HTTP_RESPONSE_TYPE_BLOB:
    aResponseType.AssignLiteral("blob");
    break;
  case XML_HTTP_RESPONSE_TYPE_DOCUMENT:
    aResponseType.AssignLiteral("document");
    break;
  case XML_HTTP_RESPONSE_TYPE_TEXT:
    aResponseType.AssignLiteral("text");
    break;
  case XML_HTTP_RESPONSE_TYPE_JSON:
    aResponseType.AssignLiteral("json");
    break;
  case XML_HTTP_RESPONSE_TYPE_CHUNKED_TEXT:
    aResponseType.AssignLiteral("moz-chunked-text");
    break;
  case XML_HTTP_RESPONSE_TYPE_CHUNKED_ARRAYBUFFER:
    aResponseType.AssignLiteral("moz-chunked-arraybuffer");
    break;
  case XML_HTTP_RESPONSE_TYPE_MOZ_BLOB:
    aResponseType.AssignLiteral("moz-blob");
    break;
  default:
    NS_ERROR("Should not happen");
  }

  return NS_OK;
}

#ifdef DEBUG
void
nsXMLHttpRequest::StaticAssertions()
{
#define ASSERT_ENUM_EQUAL(_lc, _uc) \
  static_assert(\
    static_cast<int>(XMLHttpRequestResponseType::_lc)  \
    == XML_HTTP_RESPONSE_TYPE_ ## _uc, \
    #_uc " should match")

  ASSERT_ENUM_EQUAL(_empty, DEFAULT);
  ASSERT_ENUM_EQUAL(Arraybuffer, ARRAYBUFFER);
  ASSERT_ENUM_EQUAL(Blob, BLOB);
  ASSERT_ENUM_EQUAL(Document, DOCUMENT);
  ASSERT_ENUM_EQUAL(Json, JSON);
  ASSERT_ENUM_EQUAL(Text, TEXT);
  ASSERT_ENUM_EQUAL(Moz_chunked_text, CHUNKED_TEXT);
  ASSERT_ENUM_EQUAL(Moz_chunked_arraybuffer, CHUNKED_ARRAYBUFFER);
  ASSERT_ENUM_EQUAL(Moz_blob, MOZ_BLOB);
#undef ASSERT_ENUM_EQUAL
}
#endif


NS_IMETHODIMP nsXMLHttpRequest::SetResponseType(const nsAString& aResponseType)
{
  nsXMLHttpRequest::ResponseTypeEnum responseType;
  if (aResponseType.IsEmpty()) {
    responseType = XML_HTTP_RESPONSE_TYPE_DEFAULT;
  } else if (aResponseType.EqualsLiteral("arraybuffer")) {
    responseType = XML_HTTP_RESPONSE_TYPE_ARRAYBUFFER;
  } else if (aResponseType.EqualsLiteral("blob")) {
    responseType = XML_HTTP_RESPONSE_TYPE_BLOB;
  } else if (aResponseType.EqualsLiteral("document")) {
    responseType = XML_HTTP_RESPONSE_TYPE_DOCUMENT;
  } else if (aResponseType.EqualsLiteral("text")) {
    responseType = XML_HTTP_RESPONSE_TYPE_TEXT;
  } else if (aResponseType.EqualsLiteral("json")) {
    responseType = XML_HTTP_RESPONSE_TYPE_JSON;
  } else if (aResponseType.EqualsLiteral("moz-chunked-text")) {
    responseType = XML_HTTP_RESPONSE_TYPE_CHUNKED_TEXT;
  } else if (aResponseType.EqualsLiteral("moz-chunked-arraybuffer")) {
    responseType = XML_HTTP_RESPONSE_TYPE_CHUNKED_ARRAYBUFFER;
  } else if (aResponseType.EqualsLiteral("moz-blob")) {
    responseType = XML_HTTP_RESPONSE_TYPE_MOZ_BLOB;
  } else {
    return NS_OK;
  }

  ErrorResult rv;
  SetResponseType(responseType, rv);
  return rv.ErrorCode();
}

void
nsXMLHttpRequest::SetResponseType(XMLHttpRequestResponseType aType,
                                  ErrorResult& aRv)
{
  SetResponseType(ResponseTypeEnum(static_cast<int>(aType)), aRv);
}

void
nsXMLHttpRequest::SetResponseType(nsXMLHttpRequest::ResponseTypeEnum aResponseType,
                                  ErrorResult& aRv)
{
  
  
  if (!(mState & (XML_HTTP_REQUEST_OPENED | XML_HTTP_REQUEST_SENT |
                  XML_HTTP_REQUEST_HEADERS_RECEIVED))) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  
  if (HasOrHasHadOwner() &&
      !(mState & (XML_HTTP_REQUEST_UNSENT | XML_HTTP_REQUEST_ASYNC))) {
    LogMessage("ResponseTypeSyncXHRWarning", GetOwner());
    aRv.Throw(NS_ERROR_DOM_INVALID_ACCESS_ERR);
    return;
  }

  if (!(mState & XML_HTTP_REQUEST_ASYNC) &&
      (aResponseType == XML_HTTP_RESPONSE_TYPE_CHUNKED_TEXT ||
       aResponseType == XML_HTTP_RESPONSE_TYPE_CHUNKED_ARRAYBUFFER)) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  
  mResponseType = aResponseType;

}


NS_IMETHODIMP
nsXMLHttpRequest::GetResponse(JSContext *aCx, JS::Value *aResult)
{
  ErrorResult rv;
  *aResult = GetResponse(aCx, rv);
  return rv.ErrorCode();
}

JS::Value
nsXMLHttpRequest::GetResponse(JSContext* aCx, ErrorResult& aRv)
{
  switch (mResponseType) {
  case XML_HTTP_RESPONSE_TYPE_DEFAULT:
  case XML_HTTP_RESPONSE_TYPE_TEXT:
  case XML_HTTP_RESPONSE_TYPE_CHUNKED_TEXT:
  {
    nsString str;
    aRv = GetResponseText(str);
    if (aRv.Failed()) {
      return JSVAL_NULL;
    }
    JS::Rooted<JS::Value> result(aCx);
    if (!xpc::StringToJsval(aCx, str, &result)) {
      aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
      return JSVAL_NULL;
    }
    return result;
  }

  case XML_HTTP_RESPONSE_TYPE_ARRAYBUFFER:
  case XML_HTTP_RESPONSE_TYPE_CHUNKED_ARRAYBUFFER:
  {
    if (!(mResponseType == XML_HTTP_RESPONSE_TYPE_ARRAYBUFFER &&
          mState & XML_HTTP_REQUEST_DONE) &&
        !(mResponseType == XML_HTTP_RESPONSE_TYPE_CHUNKED_ARRAYBUFFER &&
          mInLoadProgressEvent)) {
      return JSVAL_NULL;
    }

    if (!mResultArrayBuffer) {
      RootJSResultObjects();

      mResultArrayBuffer = mArrayBufferBuilder.getArrayBuffer(aCx);
      if (!mResultArrayBuffer) {
        aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
        return JSVAL_NULL;
      }
    }
    return OBJECT_TO_JSVAL(mResultArrayBuffer);
  }
  case XML_HTTP_RESPONSE_TYPE_BLOB:
  case XML_HTTP_RESPONSE_TYPE_MOZ_BLOB:
  {
    if (!(mState & XML_HTTP_REQUEST_DONE)) {
      if (mResponseType != XML_HTTP_RESPONSE_TYPE_MOZ_BLOB) {
        return JSVAL_NULL;
      }

      if (!mResponseBlob) {
        CreatePartialBlob();
      }
    }

    if (!mResponseBlob) {
      return JSVAL_NULL;
    }

    JS::Rooted<JS::Value> result(aCx, JSVAL_NULL);
    JS::Rooted<JSObject*> scope(aCx, JS::CurrentGlobalOrNull(aCx));
    aRv = nsContentUtils::WrapNative(aCx, scope, mResponseBlob, &result,
                                     nullptr, true);
    return result;
  }
  case XML_HTTP_RESPONSE_TYPE_DOCUMENT:
  {
    if (!(mState & XML_HTTP_REQUEST_DONE) || !mResponseXML) {
      return JSVAL_NULL;
    }

    JS::Rooted<JSObject*> scope(aCx, JS::CurrentGlobalOrNull(aCx));
    JS::Rooted<JS::Value> result(aCx, JSVAL_NULL);
    aRv = nsContentUtils::WrapNative(aCx, scope, mResponseXML, &result,
                                     nullptr, true);
    return result;
  }
  case XML_HTTP_RESPONSE_TYPE_JSON:
  {
    if (!(mState & XML_HTTP_REQUEST_DONE)) {
      return JSVAL_NULL;
    }

    if (mResultJSON == JSVAL_VOID) {
      aRv = CreateResponseParsedJSON(aCx);
      mResponseText.Truncate();
      if (aRv.Failed()) {
        
        aRv = NS_OK;
        
        
        JS_ClearPendingException(aCx);
        mResultJSON = JSVAL_NULL;
      }
    }
    return mResultJSON;
  }
  default:
    NS_ERROR("Should not happen");
  }

  return JSVAL_NULL;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetStatus(uint32_t *aStatus)
{
  *aStatus = Status();
  return NS_OK;
}

uint32_t
nsXMLHttpRequest::Status()
{
  if (mState & XML_HTTP_REQUEST_USE_XSITE_AC) {
    
    
    if (mChannel) {
      nsresult status;
      mChannel->GetStatus(&status);
      if (NS_FAILED(status)) {
        return 0;
      }
    }
  }

  uint16_t readyState;
  GetReadyState(&readyState);
  if (readyState == UNSENT || readyState == OPENED) {
    return 0;
  }

  if (mErrorLoad) {
    
    nsCOMPtr<nsIJARChannel> jarChannel = GetCurrentJARChannel();
    if (jarChannel) {
      nsresult status;
      mChannel->GetStatus(&status);

      if (status == NS_ERROR_FILE_NOT_FOUND) {
        return 404; 
      } else {
        return 500; 
      }
    }

    return 0;
  }

  nsCOMPtr<nsIHttpChannel> httpChannel = GetCurrentHttpChannel();
  if (!httpChannel) {

    
    nsCOMPtr<nsIJARChannel> jarChannel = GetCurrentJARChannel();
    if (jarChannel) {
      return 200; 
    }

    return 0;
  }

  uint32_t status;
  nsresult rv = httpChannel->GetResponseStatus(&status);
  if (NS_FAILED(rv)) {
    status = 0;
  }

  return status;
}

IMPL_CSTRING_GETTER(GetStatusText)
void
nsXMLHttpRequest::GetStatusText(nsCString& aStatusText)
{
  nsCOMPtr<nsIHttpChannel> httpChannel = GetCurrentHttpChannel();

  aStatusText.Truncate();

  if (!httpChannel) {
    return;
  }

  if (mState & XML_HTTP_REQUEST_USE_XSITE_AC) {
    
    
    if (mChannel) {
      nsresult status;
      mChannel->GetStatus(&status);
      if (NS_FAILED(status)) {
        return;
      }
    }
  }

  httpChannel->GetResponseStatusText(aStatusText);

}

void
nsXMLHttpRequest::CloseRequestWithError(const nsAString& aType,
                                        const uint32_t aFlag)
{
  if (mChannel) {
    mChannel->Cancel(NS_BINDING_ABORTED);
  }
  if (mCORSPreflightChannel) {
    mCORSPreflightChannel->Cancel(NS_BINDING_ABORTED);
  }
  if (mTimeoutTimer) {
    mTimeoutTimer->Cancel();
  }
  uint32_t responseLength = mResponseBody.Length();
  ResetResponse();
  mState |= aFlag;

  
  if (mState & XML_HTTP_REQUEST_DELETED) {
    mState &= ~XML_HTTP_REQUEST_SYNCLOOPING;
    return;
  }

  if (!(mState & (XML_HTTP_REQUEST_UNSENT |
                  XML_HTTP_REQUEST_OPENED |
                  XML_HTTP_REQUEST_DONE))) {
    ChangeState(XML_HTTP_REQUEST_DONE, true);

    if (!(mState & XML_HTTP_REQUEST_SYNCLOOPING)) {
      DispatchProgressEvent(this, aType, mLoadLengthComputable, responseLength,
                            mLoadTotal);
      if (mUpload && !mUploadComplete) {
        mUploadComplete = true;
        DispatchProgressEvent(mUpload, aType, true, mUploadTransferred,
                              mUploadTotal);
      }
    }
  }

  
  
  
  if (mState & XML_HTTP_REQUEST_ABORTED) {
    ChangeState(XML_HTTP_REQUEST_UNSENT, false);  
  }

  mState &= ~XML_HTTP_REQUEST_SYNCLOOPING;
}


void
nsXMLHttpRequest::Abort()
{
  CloseRequestWithError(NS_LITERAL_STRING(ABORT_STR), XML_HTTP_REQUEST_ABORTED);
}

NS_IMETHODIMP
nsXMLHttpRequest::SlowAbort()
{
  Abort();
  return NS_OK;
}



bool
nsXMLHttpRequest::IsSafeHeader(const nsACString& header, nsIHttpChannel* httpChannel)
{
  
  if (!IsSystemXHR() &&
       (header.LowerCaseEqualsASCII("set-cookie") ||
        header.LowerCaseEqualsASCII("set-cookie2"))) {
    NS_WARNING("blocked access to response header");
    return false;
  }
  
  if (!(mState & XML_HTTP_REQUEST_USE_XSITE_AC)){
    return true;
  }
  
  
  
  if (mChannel) {
    nsresult status;
    mChannel->GetStatus(&status);
    if (NS_FAILED(status)) {
      return false;
    }
  }  
  const char* kCrossOriginSafeHeaders[] = {
    "cache-control", "content-language", "content-type", "expires",
    "last-modified", "pragma"
  };
  for (uint32_t i = 0; i < ArrayLength(kCrossOriginSafeHeaders); ++i) {
    if (header.LowerCaseEqualsASCII(kCrossOriginSafeHeaders[i])) {
      return true;
    }
  }
  nsAutoCString headerVal;
  
  
  httpChannel->
      GetResponseHeader(NS_LITERAL_CSTRING("Access-Control-Expose-Headers"),
                        headerVal);
  nsCCharSeparatedTokenizer exposeTokens(headerVal, ',');
  bool isSafe = false;
  while (exposeTokens.hasMoreTokens()) {
    const nsDependentCSubstring& token = exposeTokens.nextToken();
    if (token.IsEmpty()) {
      continue;
    }
    if (!IsValidHTTPToken(token)) {
      return false;
    }
    if (header.Equals(token, nsCaseInsensitiveCStringComparator())) {
      isSafe = true;
    }
  }
  return isSafe;
}


IMPL_CSTRING_GETTER(GetAllResponseHeaders)
void
nsXMLHttpRequest::GetAllResponseHeaders(nsCString& aResponseHeaders)
{
  aResponseHeaders.Truncate();

  
  
  if (mState & (XML_HTTP_REQUEST_UNSENT |
                XML_HTTP_REQUEST_OPENED | XML_HTTP_REQUEST_SENT)) {
    return;
  }

  if (nsCOMPtr<nsIHttpChannel> httpChannel = GetCurrentHttpChannel()) {
    nsRefPtr<nsHeaderVisitor> visitor = new nsHeaderVisitor(this, httpChannel);
    if (NS_SUCCEEDED(httpChannel->VisitResponseHeaders(visitor))) {
      aResponseHeaders = visitor->Headers();
    }
    return;
  }

  if (!mChannel) {
    return;
  }

  
  nsAutoCString value;
  if (NS_SUCCEEDED(mChannel->GetContentType(value))) {
    aResponseHeaders.AppendLiteral("Content-Type: ");
    aResponseHeaders.Append(value);
    if (NS_SUCCEEDED(mChannel->GetContentCharset(value)) && !value.IsEmpty()) {
      aResponseHeaders.AppendLiteral(";charset=");
      aResponseHeaders.Append(value);
    }
    aResponseHeaders.AppendLiteral("\r\n");
  }

  int64_t length;
  if (NS_SUCCEEDED(mChannel->GetContentLength(&length))) {
    aResponseHeaders.AppendLiteral("Content-Length: ");
    aResponseHeaders.AppendInt(length);
    aResponseHeaders.AppendLiteral("\r\n");
  }
}

NS_IMETHODIMP
nsXMLHttpRequest::GetResponseHeader(const nsACString& aHeader,
                                    nsACString& aResult)
{
  ErrorResult rv;
  GetResponseHeader(aHeader, aResult, rv);
  return rv.ErrorCode();
}

void
nsXMLHttpRequest::GetResponseHeader(const nsACString& header,
                                    nsACString& _retval, ErrorResult& aRv)
{
  _retval.SetIsVoid(true);

  nsCOMPtr<nsIHttpChannel> httpChannel = GetCurrentHttpChannel();

  if (!httpChannel) {
    
    
    if (mState & (XML_HTTP_REQUEST_UNSENT |
                  XML_HTTP_REQUEST_OPENED | XML_HTTP_REQUEST_SENT)) {
      return;
    }

    
    
    
    nsresult status;
    if (!mChannel ||
        NS_FAILED(mChannel->GetStatus(&status)) ||
        NS_FAILED(status)) {
      return;
    }

    
    if (header.LowerCaseEqualsASCII("content-type")) {
      if (NS_FAILED(mChannel->GetContentType(_retval))) {
        
        _retval.SetIsVoid(true);
        return;
      }

      nsCString value;
      if (NS_SUCCEEDED(mChannel->GetContentCharset(value)) &&
          !value.IsEmpty()) {
        _retval.Append(";charset=");
        _retval.Append(value);
      }
    }

    
    else if (header.LowerCaseEqualsASCII("content-length")) {
      int64_t length;
      if (NS_SUCCEEDED(mChannel->GetContentLength(&length))) {
        _retval.AppendInt(length);
      }
    }

    return;
  }

  
  if (!IsSafeHeader(header, httpChannel)) {
    return;
  }

  aRv = httpChannel->GetResponseHeader(header, _retval);
  if (aRv.ErrorCode() == NS_ERROR_NOT_AVAILABLE) {
    
    _retval.SetIsVoid(true);
    aRv = NS_OK;
  }
}

already_AddRefed<nsILoadGroup>
nsXMLHttpRequest::GetLoadGroup() const
{
  if (mState & XML_HTTP_REQUEST_BACKGROUND) {                 
    return nullptr;
  }

  nsresult rv = NS_ERROR_FAILURE;
  nsIScriptContext* sc =
    const_cast<nsXMLHttpRequest*>(this)->GetContextForEventHandlers(&rv);
  nsCOMPtr<nsIDocument> doc =
    nsContentUtils::GetDocumentFromScriptContext(sc);
  if (doc) {
    return doc->GetDocumentLoadGroup();
  }

  return nullptr;
}

nsresult
nsXMLHttpRequest::CreateReadystatechangeEvent(nsIDOMEvent** aDOMEvent)
{
  nsresult rv = nsEventDispatcher::CreateEvent(this, nullptr, nullptr,
                                               NS_LITERAL_STRING("Events"),
                                               aDOMEvent);
  if (NS_FAILED(rv)) {
    return rv;
  }

  (*aDOMEvent)->InitEvent(NS_LITERAL_STRING(READYSTATE_STR),
                          false, false);

  
  (*aDOMEvent)->SetTrusted(true);

  return NS_OK;
}

void
nsXMLHttpRequest::DispatchProgressEvent(nsDOMEventTargetHelper* aTarget,
                                        const nsAString& aType,
                                        bool aLengthComputable,
                                        uint64_t aLoaded, uint64_t aTotal)
{
  NS_ASSERTION(aTarget, "null target");
  NS_ASSERTION(!aType.IsEmpty(), "missing event type");

  if (NS_FAILED(CheckInnerWindowCorrectness()) ||
      (!AllowUploadProgress() && aTarget == mUpload)) {
    return;
  }

  bool dispatchLoadend = aType.EqualsLiteral(LOAD_STR) ||
                         aType.EqualsLiteral(ERROR_STR) ||
                         aType.EqualsLiteral(TIMEOUT_STR) ||
                         aType.EqualsLiteral(ABORT_STR);

  nsCOMPtr<nsIDOMEvent> event;
  nsresult rv = NS_NewDOMProgressEvent(getter_AddRefs(event), this,
                                       nullptr, nullptr);
  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<nsIDOMProgressEvent> progress = do_QueryInterface(event);
  if (!progress) {
    return;
  }

  progress->InitProgressEvent(aType, false, false, aLengthComputable,
                              aLoaded, (aTotal == UINT64_MAX) ? 0 : aTotal);

  event->SetTrusted(true);

  aTarget->DispatchDOMEvent(nullptr, event, nullptr, nullptr);

  if (dispatchLoadend) {
    DispatchProgressEvent(aTarget, NS_LITERAL_STRING(LOADEND_STR),
                          aLengthComputable, aLoaded, aTotal);
  }
}
                                          
already_AddRefed<nsIHttpChannel>
nsXMLHttpRequest::GetCurrentHttpChannel()
{
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(mChannel);
  return httpChannel.forget();
}

already_AddRefed<nsIJARChannel>
nsXMLHttpRequest::GetCurrentJARChannel()
{
  nsCOMPtr<nsIJARChannel> appChannel = do_QueryInterface(mChannel);
  return appChannel.forget();
}

bool
nsXMLHttpRequest::IsSystemXHR()
{
  return mIsSystem || nsContentUtils::IsSystemPrincipal(mPrincipal);
}

nsresult
nsXMLHttpRequest::CheckChannelForCrossSiteRequest(nsIChannel* aChannel)
{
  
  
  
  if (IsSystemXHR()) {
    if (!nsContentUtils::IsSystemPrincipal(mPrincipal)) {
      nsIScriptSecurityManager *secMan = nsContentUtils::GetSecurityManager();
      nsCOMPtr<nsIURI> uri;
      aChannel->GetOriginalURI(getter_AddRefs(uri));
      return secMan->CheckLoadURIWithPrincipal(
        mPrincipal, uri, nsIScriptSecurityManager::STANDARD);
    }
    return NS_OK;
  }

  
  
  if (nsContentUtils::CheckMayLoad(mPrincipal, aChannel, true)) {
    return NS_OK;
  }

  
  mState |= XML_HTTP_REQUEST_USE_XSITE_AC;

  
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aChannel);
  NS_ENSURE_TRUE(httpChannel, NS_ERROR_DOM_BAD_URI);

  nsAutoCString method;
  httpChannel->GetRequestMethod(method);
  if (!mCORSUnsafeHeaders.IsEmpty() ||
      (mUpload && mUpload->HasListeners()) ||
      (!method.LowerCaseEqualsLiteral("get") &&
       !method.LowerCaseEqualsLiteral("post") &&
       !method.LowerCaseEqualsLiteral("head"))) {
    mState |= XML_HTTP_REQUEST_NEED_AC_PREFLIGHT;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXMLHttpRequest::Open(const nsACString& method, const nsACString& url,
                       bool async, const nsAString& user,
                       const nsAString& password, uint8_t optional_argc)
{
  if (!optional_argc) {
    
    async = true;
  }
  Optional<nsAString> realUser;
  if (optional_argc > 1) {
    realUser = &user;
  }
  Optional<nsAString> realPassword;
  if (optional_argc > 2) {
    realPassword = &password;
  }
  return Open(method, url, async, realUser, realPassword);
}

nsresult
nsXMLHttpRequest::Open(const nsACString& method, const nsACString& url,
                       bool async, const Optional<nsAString>& user,
                       const Optional<nsAString>& password)
{
  NS_ENSURE_ARG(!method.IsEmpty());

  Telemetry::Accumulate(Telemetry::XMLHTTPREQUEST_ASYNC_OR_SYNC,
                        async ? 0 : 1);

  NS_ENSURE_TRUE(mPrincipal, NS_ERROR_NOT_INITIALIZED);

  
  
  if (method.LowerCaseEqualsLiteral("trace") ||
      method.LowerCaseEqualsLiteral("track")) {
    return NS_ERROR_INVALID_ARG;
  }

  
  
  if (!async && HasOrHasHadOwner() &&
      (mState & XML_HTTP_REQUEST_AC_WITH_CREDENTIALS ||
       mTimeoutMilliseconds ||
       mResponseType != XML_HTTP_RESPONSE_TYPE_DEFAULT)) {
    if (mState & XML_HTTP_REQUEST_AC_WITH_CREDENTIALS) {
      LogMessage("WithCredentialsSyncXHRWarning", GetOwner());
    }
    if (mTimeoutMilliseconds) {
      LogMessage("TimeoutSyncXHRWarning", GetOwner());
    }
    if (mResponseType != XML_HTTP_RESPONSE_TYPE_DEFAULT) {
      LogMessage("ResponseTypeSyncXHRWarning", GetOwner());
    }
    return NS_ERROR_DOM_INVALID_ACCESS_ERR;
  }

  nsresult rv;
  nsCOMPtr<nsIURI> uri;

  if (mState & (XML_HTTP_REQUEST_OPENED |
                XML_HTTP_REQUEST_HEADERS_RECEIVED |
                XML_HTTP_REQUEST_LOADING |
                XML_HTTP_REQUEST_SENT)) {
    
    Abort();

    
    
    
    
    
  }

  
  mState &= ~XML_HTTP_REQUEST_ABORTED & ~XML_HTTP_REQUEST_TIMED_OUT;

  if (async) {
    mState |= XML_HTTP_REQUEST_ASYNC;
  } else {
    mState &= ~XML_HTTP_REQUEST_ASYNC;
  }

  nsIScriptContext* sc = GetContextForEventHandlers(&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDocument> doc =
    nsContentUtils::GetDocumentFromScriptContext(sc);
  
  nsCOMPtr<nsIURI> baseURI;
  if (mBaseURI) {
    baseURI = mBaseURI;
  }
  else if (doc) {
    baseURI = doc->GetBaseURI();
  }

  rv = NS_NewURI(getter_AddRefs(uri), url, nullptr, baseURI);
  if (NS_FAILED(rv)) return rv;

  rv = CheckInnerWindowCorrectness();
  NS_ENSURE_SUCCESS(rv, rv);
  int16_t shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_XMLHTTPREQUEST,
                                 uri,
                                 mPrincipal,
                                 doc,
                                 EmptyCString(), 
                                 nullptr,         
                                 &shouldLoad,
                                 nsContentUtils::GetContentPolicy(),
                                 nsContentUtils::GetSecurityManager());
  if (NS_FAILED(rv)) return rv;
  if (NS_CP_REJECTED(shouldLoad)) {
    
    return NS_ERROR_CONTENT_BLOCKED;
  }

  
  
  if (user.WasPassed() && !user.Value().IsEmpty()) {
    nsAutoCString userpass;
    CopyUTF16toUTF8(user.Value(), userpass);
    if (password.WasPassed() && !password.Value().IsEmpty()) {
      userpass.Append(':');
      AppendUTF16toUTF8(password.Value(), userpass);
    }
    uri->SetUserPass(userpass);
  }

  
  
  mAlreadySetHeaders.Clear();

  
  
  
  nsCOMPtr<nsILoadGroup> loadGroup = GetLoadGroup();

  
  nsCOMPtr<nsIChannelPolicy> channelPolicy;
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  rv = mPrincipal->GetCsp(getter_AddRefs(csp));
  NS_ENSURE_SUCCESS(rv, rv);
  if (csp) {
    channelPolicy = do_CreateInstance("@mozilla.org/nschannelpolicy;1");
    channelPolicy->SetContentSecurityPolicy(csp);
    channelPolicy->SetLoadType(nsIContentPolicy::TYPE_XMLHTTPREQUEST);
  }
  rv = NS_NewChannel(getter_AddRefs(mChannel),
                     uri,
                     nullptr,                    
                     loadGroup,
                     nullptr,                    
                     nsIRequest::LOAD_BACKGROUND,
                     channelPolicy);
  if (NS_FAILED(rv)) return rv;

  mState &= ~(XML_HTTP_REQUEST_USE_XSITE_AC |
              XML_HTTP_REQUEST_NEED_AC_PREFLIGHT);

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mChannel));
  if (httpChannel) {
    rv = httpChannel->SetRequestMethod(method);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  ChangeState(XML_HTTP_REQUEST_OPENED);

  return rv;
}




NS_METHOD
nsXMLHttpRequest::StreamReaderFunc(nsIInputStream* in,
                                   void* closure,
                                   const char* fromRawSegment,
                                   uint32_t toOffset,
                                   uint32_t count,
                                   uint32_t *writeCount)
{
  nsXMLHttpRequest* xmlHttpRequest = static_cast<nsXMLHttpRequest*>(closure);
  if (!xmlHttpRequest || !writeCount) {
    NS_WARNING("XMLHttpRequest cannot read from stream: no closure or writeCount");
    return NS_ERROR_FAILURE;
  }

  nsresult rv = NS_OK;

  if (xmlHttpRequest->mResponseType == XML_HTTP_RESPONSE_TYPE_BLOB ||
      xmlHttpRequest->mResponseType == XML_HTTP_RESPONSE_TYPE_MOZ_BLOB) {
    if (!xmlHttpRequest->mDOMFile) {
      if (!xmlHttpRequest->mBlobSet) {
        xmlHttpRequest->mBlobSet = new BlobSet();
      }
      rv = xmlHttpRequest->mBlobSet->AppendVoidPtr(fromRawSegment, count);
    }
    
    if (xmlHttpRequest->mResponseType == XML_HTTP_RESPONSE_TYPE_MOZ_BLOB) {
      xmlHttpRequest->mResponseBlob = nullptr;
    }
  } else if (xmlHttpRequest->mResponseType == XML_HTTP_RESPONSE_TYPE_ARRAYBUFFER ||
             xmlHttpRequest->mResponseType == XML_HTTP_RESPONSE_TYPE_CHUNKED_ARRAYBUFFER) {
    
    
    if (xmlHttpRequest->mArrayBufferBuilder.capacity() == 0)
      xmlHttpRequest->mArrayBufferBuilder.setCapacity(PR_MAX(count, XML_HTTP_REQUEST_ARRAYBUFFER_MIN_SIZE));

    xmlHttpRequest->mArrayBufferBuilder.append(reinterpret_cast<const uint8_t*>(fromRawSegment), count,
                                               XML_HTTP_REQUEST_ARRAYBUFFER_MAX_GROWTH);
  } else if (xmlHttpRequest->mResponseType == XML_HTTP_RESPONSE_TYPE_DEFAULT &&
             xmlHttpRequest->mResponseXML) {
    
    uint32_t previousLength = xmlHttpRequest->mResponseBody.Length();
    xmlHttpRequest->mResponseBody.Append(fromRawSegment,count);
    if (count > 0 && xmlHttpRequest->mResponseBody.Length() == previousLength) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  } else if (xmlHttpRequest->mResponseType == XML_HTTP_RESPONSE_TYPE_DEFAULT ||
             xmlHttpRequest->mResponseType == XML_HTTP_RESPONSE_TYPE_TEXT ||
             xmlHttpRequest->mResponseType == XML_HTTP_RESPONSE_TYPE_JSON ||
             xmlHttpRequest->mResponseType == XML_HTTP_RESPONSE_TYPE_CHUNKED_TEXT) {
    NS_ASSERTION(!xmlHttpRequest->mResponseXML,
                 "We shouldn't be parsing a doc here");
    xmlHttpRequest->AppendToResponseText(fromRawSegment, count);
  }

  if (xmlHttpRequest->mState & XML_HTTP_REQUEST_PARSEBODY) {
    

    
    
    
    nsCOMPtr<nsIInputStream> copyStream;
    rv = NS_NewByteInputStream(getter_AddRefs(copyStream), fromRawSegment, count);

    if (NS_SUCCEEDED(rv) && xmlHttpRequest->mXMLParserStreamListener) {
      NS_ASSERTION(copyStream, "NS_NewByteInputStream lied");
      nsresult parsingResult = xmlHttpRequest->mXMLParserStreamListener
                                  ->OnDataAvailable(xmlHttpRequest->mChannel,
                                                    xmlHttpRequest->mContext,
                                                    copyStream, toOffset, count);

      
      
      if (NS_FAILED(parsingResult)) {
        xmlHttpRequest->mState &= ~XML_HTTP_REQUEST_PARSEBODY;
      }
    }
  }

  if (NS_SUCCEEDED(rv)) {
    *writeCount = count;
  } else {
    *writeCount = 0;
  }

  return rv;
}

bool nsXMLHttpRequest::CreateDOMFile(nsIRequest *request)
{
  nsCOMPtr<nsIFile> file;
  nsCOMPtr<nsIFileChannel> fc = do_QueryInterface(request);
  if (fc) {
    fc->GetFile(getter_AddRefs(file));
  }

  if (!file)
    return false;

  nsAutoCString contentType;
  mChannel->GetContentType(contentType);

  mDOMFile =
    new nsDOMFileFile(file, EmptyString(), NS_ConvertASCIItoUTF16(contentType));
  mBlobSet = nullptr;
  NS_ASSERTION(mResponseBody.IsEmpty(), "mResponseBody should be empty");
  return true;
}

NS_IMETHODIMP
nsXMLHttpRequest::OnDataAvailable(nsIRequest *request,
                                  nsISupports *ctxt,
                                  nsIInputStream *inStr,
                                  uint64_t sourceOffset,
                                  uint32_t count)
{
  NS_ENSURE_ARG_POINTER(inStr);

  NS_ABORT_IF_FALSE(mContext.get() == ctxt,"start context different from OnDataAvailable context");

  mProgressSinceLastProgressEvent = true;

  bool cancelable = false;
  if ((mResponseType == XML_HTTP_RESPONSE_TYPE_BLOB ||
       mResponseType == XML_HTTP_RESPONSE_TYPE_MOZ_BLOB) && !mDOMFile) {
    cancelable = CreateDOMFile(request);
    
    
  }

  uint32_t totalRead;
  nsresult rv = inStr->ReadSegments(nsXMLHttpRequest::StreamReaderFunc,
                                    (void*)this, count, &totalRead);
  NS_ENSURE_SUCCESS(rv, rv);

  if (cancelable) {
    
    mDOMFile->GetSize(&mLoadTransferred);
    ChangeState(XML_HTTP_REQUEST_LOADING);
    return request->Cancel(NS_OK);
  }

  mLoadTransferred += totalRead;

  ChangeState(XML_HTTP_REQUEST_LOADING);
  
  MaybeDispatchProgressEvents(false);

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::OnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
  PROFILER_LABEL("nsXMLHttpRequest", "OnStartRequest");
  nsresult rv = NS_OK;
  if (!mFirstStartRequestSeen && mRequestObserver) {
    mFirstStartRequestSeen = true;
    mRequestObserver->OnStartRequest(request, ctxt);
  }

  if (request != mChannel) {
    
    return NS_OK;
  }

  
  if (mState & XML_HTTP_REQUEST_UNSENT)
    return NS_OK;

  

  if (mState & XML_HTTP_REQUEST_ABORTED) {
    NS_ERROR("Ugh, still getting data on an aborted XMLHttpRequest!");

    return NS_ERROR_UNEXPECTED;
  }

  
  if (mState & XML_HTTP_REQUEST_TIMED_OUT) {
    return NS_OK;
  }

  nsCOMPtr<nsIChannel> channel(do_QueryInterface(request));
  NS_ENSURE_TRUE(channel, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIPrincipal> documentPrincipal;
  if (IsSystemXHR()) {
    
    
    
    
    nsresult rv;
    documentPrincipal = do_CreateInstance("@mozilla.org/nullprincipal;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    documentPrincipal = mPrincipal;
  }

  channel->SetOwner(documentPrincipal);

  nsresult status;
  request->GetStatus(&status);
  mErrorLoad = mErrorLoad || NS_FAILED(status);

  if (mUpload && !mUploadComplete && !mErrorLoad &&
      (mState & XML_HTTP_REQUEST_ASYNC)) {
    if (mProgressTimerIsActive) {
      mProgressTimerIsActive = false;
      mProgressNotifier->Cancel();
    }
    MaybeDispatchProgressEvents(true);
    mUploadComplete = true;
    DispatchProgressEvent(mUpload, NS_LITERAL_STRING(LOAD_STR),
                          true, mUploadTotal, mUploadTotal);
  }

  mContext = ctxt;
  mState |= XML_HTTP_REQUEST_PARSEBODY;
  ChangeState(XML_HTTP_REQUEST_HEADERS_RECEIVED);

  ResetResponse();

  if (!mOverrideMimeType.IsEmpty()) {
    channel->SetContentType(NS_ConvertUTF16toUTF8(mOverrideMimeType));
  }

  DetectCharset();

  
  if (mResponseType == XML_HTTP_RESPONSE_TYPE_ARRAYBUFFER && NS_SUCCEEDED(status)) {
    int64_t contentLength;
    rv = channel->GetContentLength(&contentLength);
    if (NS_SUCCEEDED(rv) &&
        contentLength > 0 &&
        contentLength < XML_HTTP_REQUEST_MAX_CONTENT_LENGTH_PREALLOCATE) {
      mArrayBufferBuilder.setCapacity(static_cast<int32_t>(contentLength));
    }
  }

  
  bool parseBody = mResponseType == XML_HTTP_RESPONSE_TYPE_DEFAULT ||
                     mResponseType == XML_HTTP_RESPONSE_TYPE_DOCUMENT;
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mChannel));
  if (parseBody && httpChannel) {
    nsAutoCString method;
    httpChannel->GetRequestMethod(method);
    parseBody = !method.EqualsLiteral("HEAD");
  }

  mIsHtml = false;
  mWarnAboutSyncHtml = false;
  if (parseBody && NS_SUCCEEDED(status)) {
    
    
    
    
    nsAutoCString type;
    channel->GetContentType(type);

    if ((mResponseType == XML_HTTP_RESPONSE_TYPE_DOCUMENT) &&
        type.EqualsLiteral("text/html")) {
      
      
      
      
      
      if (!(mState & XML_HTTP_REQUEST_ASYNC)) {
        
        
        mWarnAboutSyncHtml = true;
        mState &= ~XML_HTTP_REQUEST_PARSEBODY;
      } else {
        mIsHtml = true;
      }
    } else if (type.Find("xml") == kNotFound) {
      mState &= ~XML_HTTP_REQUEST_PARSEBODY;
    }
  } else {
    
    mState &= ~XML_HTTP_REQUEST_PARSEBODY;
  }

  if (mState & XML_HTTP_REQUEST_PARSEBODY) {
    nsCOMPtr<nsIURI> baseURI, docURI;
    nsIScriptContext* sc = GetContextForEventHandlers(&rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIDocument> doc =
      nsContentUtils::GetDocumentFromScriptContext(sc);

    if (doc) {
      docURI = doc->GetDocumentURI();
      baseURI = doc->GetBaseURI();
    }

    
    
    
    
    const nsAString& emptyStr = EmptyString();
    nsCOMPtr<nsIDOMDocument> responseDoc;
    nsIGlobalObject* global = nsDOMEventTargetHelper::GetParentObject();
    rv = NS_NewDOMDocument(getter_AddRefs(responseDoc),
                           emptyStr, emptyStr, nullptr, docURI,
                           baseURI, mPrincipal, true, global,
                           mIsHtml ? DocumentFlavorHTML :
                                     DocumentFlavorLegacyGuess);
    NS_ENSURE_SUCCESS(rv, rv);
    mResponseXML = do_QueryInterface(responseDoc);
    mResponseXML->SetPrincipal(documentPrincipal);

    if (nsContentUtils::IsSystemPrincipal(mPrincipal)) {
      mResponseXML->ForceEnableXULXBL();
    }

    if (mState & XML_HTTP_REQUEST_USE_XSITE_AC) {
      nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(mResponseXML);
      if (htmlDoc) {
        htmlDoc->DisableCookieAccess();
      }
    }

    nsCOMPtr<nsIStreamListener> listener;
    nsCOMPtr<nsILoadGroup> loadGroup;
    channel->GetLoadGroup(getter_AddRefs(loadGroup));

    rv = mResponseXML->StartDocumentLoad(kLoadAsData, channel, loadGroup,
                                         nullptr, getter_AddRefs(listener),
                                         !(mState & XML_HTTP_REQUEST_USE_XSITE_AC));
    NS_ENSURE_SUCCESS(rv, rv);

    mXMLParserStreamListener = listener;
    rv = mXMLParserStreamListener->OnStartRequest(request, ctxt);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  if (NS_SUCCEEDED(rv) &&
      (mState & XML_HTTP_REQUEST_ASYNC) &&
      HasListenersFor(nsGkAtoms::onprogress)) {
    StartProgressEventTimer();
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::OnStopRequest(nsIRequest *request, nsISupports *ctxt, nsresult status)
{
  PROFILER_LABEL("content", "nsXMLHttpRequest::OnStopRequest");
  if (request != mChannel) {
    
    return NS_OK;
  }

  mWaitingForOnStopRequest = false;

  if (mRequestObserver) {
    NS_ASSERTION(mFirstStartRequestSeen, "Inconsistent state!");
    mFirstStartRequestSeen = false;
    mRequestObserver->OnStopRequest(request, ctxt, status);
  }

  
  
  
  if ((mState & XML_HTTP_REQUEST_UNSENT) ||
      (mState & XML_HTTP_REQUEST_TIMED_OUT)) {
    if (mXMLParserStreamListener)
      (void) mXMLParserStreamListener->OnStopRequest(request, ctxt, status);
    return NS_OK;
  }

  
  if (mState & XML_HTTP_REQUEST_PARSEBODY && mXMLParserStreamListener) {
    mXMLParserStreamListener->OnStopRequest(request, ctxt, status);
  }

  mXMLParserStreamListener = nullptr;
  mContext = nullptr;

  
  
  
  if (!mIsHtml) {
    MaybeDispatchProgressEvents(true);
  }

  if (NS_SUCCEEDED(status) &&
      (mResponseType == XML_HTTP_RESPONSE_TYPE_BLOB ||
       mResponseType == XML_HTTP_RESPONSE_TYPE_MOZ_BLOB)) {
    if (!mDOMFile) {
      CreateDOMFile(request);
    }
    if (mDOMFile) {
      mResponseBlob = mDOMFile;
      mDOMFile = nullptr;
    } else {
      
      
      if (!mBlobSet) {
        mBlobSet = new BlobSet();
      }
      
      
      nsAutoCString contentType;
      mChannel->GetContentType(contentType);
      mResponseBlob = mBlobSet->GetBlobInternal(contentType);
      mBlobSet = nullptr;
    }
    NS_ASSERTION(mResponseBody.IsEmpty(), "mResponseBody should be empty");
    NS_ASSERTION(mResponseText.IsEmpty(), "mResponseText should be empty");
  } else if (NS_SUCCEEDED(status) &&
             (mResponseType == XML_HTTP_RESPONSE_TYPE_ARRAYBUFFER ||
              mResponseType == XML_HTTP_RESPONSE_TYPE_CHUNKED_ARRAYBUFFER)) {
    
    
    if (!mArrayBufferBuilder.setCapacity(mArrayBufferBuilder.length())) {
      
      status = NS_ERROR_UNEXPECTED;
    }
  }

  nsCOMPtr<nsIChannel> channel(do_QueryInterface(request));
  NS_ENSURE_TRUE(channel, NS_ERROR_UNEXPECTED);

  channel->SetNotificationCallbacks(nullptr);
  mNotificationCallbacks = nullptr;
  mChannelEventSink = nullptr;
  mProgressEventSink = nullptr;

  mState &= ~XML_HTTP_REQUEST_SYNCLOOPING;

  if (NS_FAILED(status)) {
    
    

    mErrorLoad = true;
    mResponseXML = nullptr;
  }

  
  
  
  if (mState & (XML_HTTP_REQUEST_UNSENT |
                XML_HTTP_REQUEST_DONE)) {
    return NS_OK;
  }

  if (!mResponseXML) {
    ChangeStateToDone();
    return NS_OK;
  }
  if (mIsHtml) {
    NS_ASSERTION(!(mState & XML_HTTP_REQUEST_SYNCLOOPING),
      "We weren't supposed to support HTML parsing with XHR!");
    nsCOMPtr<EventTarget> eventTarget = do_QueryInterface(mResponseXML);
    nsEventListenerManager* manager =
      eventTarget->GetOrCreateListenerManager();
    manager->AddEventListenerByType(new nsXHRParseEndListener(this),
                                    NS_LITERAL_STRING("DOMContentLoaded"),
                                    dom::TrustedEventsAtSystemGroupBubble());
    return NS_OK;
  }
  
  
  
  
  if (!mResponseXML->GetRootElement()) {
    mResponseXML = nullptr;
  }
  ChangeStateToDone();
  return NS_OK;
}

void
nsXMLHttpRequest::ChangeStateToDone()
{
  if (mIsHtml) {
    
    
    MaybeDispatchProgressEvents(true);
  }

  ChangeState(XML_HTTP_REQUEST_DONE, true);
  if (mTimeoutTimer) {
    mTimeoutTimer->Cancel();
  }

  NS_NAMED_LITERAL_STRING(errorStr, ERROR_STR);
  NS_NAMED_LITERAL_STRING(loadStr, LOAD_STR);
  DispatchProgressEvent(this,
                        mErrorLoad ? errorStr : loadStr,
                        !mErrorLoad,
                        mLoadTransferred,
                        mErrorLoad ? 0 : mLoadTransferred);
  if (mErrorLoad && mUpload && !mUploadComplete) {
    DispatchProgressEvent(mUpload, errorStr, true,
                          mUploadTransferred, mUploadTotal);
  }

  if (mErrorLoad) {
    
    
    
    
    mChannel = nullptr;
    mCORSPreflightChannel = nullptr;
  }
}

NS_IMETHODIMP
nsXMLHttpRequest::SendAsBinary(const nsAString &aBody)
{
  ErrorResult rv;
  SendAsBinary(aBody, rv);
  return rv.ErrorCode();
}

void
nsXMLHttpRequest::SendAsBinary(const nsAString &aBody,
                               ErrorResult& aRv)
{
  char *data = static_cast<char*>(NS_Alloc(aBody.Length() + 1));
  if (!data) {
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return;
  }

  nsAString::const_iterator iter, end;
  aBody.BeginReading(iter);
  aBody.EndReading(end);
  char *p = data;
  while (iter != end) {
    if (*iter & 0xFF00) {
      NS_Free(data);
      aRv.Throw(NS_ERROR_DOM_INVALID_CHARACTER_ERR);
      return;
    }
    *p++ = static_cast<char>(*iter++);
  }
  *p = '\0';

  nsCOMPtr<nsIInputStream> stream;
  aRv = NS_NewByteInputStream(getter_AddRefs(stream), data, aBody.Length(),
                              NS_ASSIGNMENT_ADOPT);
  if (aRv.Failed()) {
    NS_Free(data);
    return;
  }

  nsCOMPtr<nsIWritableVariant> variant = new nsVariant();

  aRv = variant->SetAsISupports(stream);
  if (aRv.Failed()) {
    return;
  }

  aRv = Send(variant);
}

static nsresult
GetRequestBody(nsIDOMDocument* aDoc, nsIInputStream** aResult,
               uint64_t* aContentLength, nsACString& aContentType,
               nsACString& aCharset)
{
  aContentType.AssignLiteral("application/xml");
  nsAutoString inputEncoding;
  aDoc->GetInputEncoding(inputEncoding);
  if (!DOMStringIsNull(inputEncoding)) {
    CopyUTF16toUTF8(inputEncoding, aCharset);
  }
  else {
    aCharset.AssignLiteral("UTF-8");
  }

  
  
  nsresult rv;
  nsCOMPtr<nsIDOMSerializer> serializer =
    do_CreateInstance(NS_XMLSERIALIZER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStorageStream> storStream;
  rv = NS_NewStorageStream(4096, UINT32_MAX, getter_AddRefs(storStream));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIOutputStream> output;
  rv = storStream->GetOutputStream(0, getter_AddRefs(output));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = serializer->SerializeToStream(aDoc, output, aCharset);
  NS_ENSURE_SUCCESS(rv, rv);

  output->Close();

  uint32_t length;
  rv = storStream->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);
  *aContentLength = length;

  return storStream->NewInputStream(0, aResult);
}

static nsresult
GetRequestBody(const nsAString& aString, nsIInputStream** aResult,
               uint64_t* aContentLength, nsACString& aContentType,
               nsACString& aCharset)
{
  aContentType.AssignLiteral("text/plain");
  aCharset.AssignLiteral("UTF-8");

  nsCString converted = NS_ConvertUTF16toUTF8(aString);
  *aContentLength = converted.Length();
  return NS_NewCStringInputStream(aResult, converted);
}

static nsresult
GetRequestBody(nsIInputStream* aStream, nsIInputStream** aResult,
               uint64_t* aContentLength, nsACString& aContentType,
               nsACString& aCharset)
{
  aContentType.AssignLiteral("text/plain");
  aCharset.Truncate();

  nsresult rv = aStream->Available(aContentLength);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aResult = aStream);

  return NS_OK;
}

static nsresult
GetRequestBody(nsIXHRSendable* aSendable, nsIInputStream** aResult, uint64_t* aContentLength,
               nsACString& aContentType, nsACString& aCharset)
{
  return aSendable->GetSendInfo(aResult, aContentLength, aContentType, aCharset);
}


static nsresult
GetRequestBody(const uint8_t* aData, uint32_t aDataLength,
               nsIInputStream** aResult, uint64_t* aContentLength,
               nsACString& aContentType, nsACString& aCharset)
{
  aContentType.SetIsVoid(true);
  aCharset.Truncate();

  *aContentLength = aDataLength;
  const char* data = reinterpret_cast<const char*>(aData);

  nsCOMPtr<nsIInputStream> stream;
  nsresult rv = NS_NewByteInputStream(getter_AddRefs(stream), data, aDataLength,
                                      NS_ASSIGNMENT_COPY);
  NS_ENSURE_SUCCESS(rv, rv);

  stream.forget(aResult);

  return NS_OK;
}

static nsresult
GetRequestBody(nsIVariant* aBody, nsIInputStream** aResult, uint64_t* aContentLength,
               nsACString& aContentType, nsACString& aCharset)
{
  *aResult = nullptr;

  uint16_t dataType;
  nsresult rv = aBody->GetDataType(&dataType);
  NS_ENSURE_SUCCESS(rv, rv);

  if (dataType == nsIDataType::VTYPE_INTERFACE ||
      dataType == nsIDataType::VTYPE_INTERFACE_IS) {
    nsCOMPtr<nsISupports> supports;
    nsID *iid;
    rv = aBody->GetAsInterface(&iid, getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);

    nsMemory::Free(iid);

    
    nsCOMPtr<nsIDOMDocument> doc = do_QueryInterface(supports);
    if (doc) {
      return GetRequestBody(doc, aResult, aContentLength, aContentType, aCharset);
    }

    
    nsCOMPtr<nsISupportsString> wstr = do_QueryInterface(supports);
    if (wstr) {
      nsAutoString string;
      wstr->GetData(string);

      return GetRequestBody(string, aResult, aContentLength, aContentType, aCharset);
    }

    
    nsCOMPtr<nsIInputStream> stream = do_QueryInterface(supports);
    if (stream) {
      return GetRequestBody(stream, aResult, aContentLength, aContentType, aCharset);
    }

    
    nsCOMPtr<nsIXHRSendable> sendable = do_QueryInterface(supports);
    if (sendable) {
      return GetRequestBody(sendable, aResult, aContentLength, aContentType, aCharset);
    }

    
    AutoSafeJSContext cx;
    JS::Rooted<JS::Value> realVal(cx);

    nsresult rv = aBody->GetAsJSVal(realVal.address());
    if (NS_SUCCEEDED(rv) && !JSVAL_IS_PRIMITIVE(realVal)) {
      JS::Rooted<JSObject*> obj(cx, JSVAL_TO_OBJECT(realVal));
      if (JS_IsArrayBufferObject(obj)) {
          ArrayBuffer buf(obj);
          return GetRequestBody(buf.Data(), buf.Length(), aResult,
                                aContentLength, aContentType, aCharset);
      }
    }
  }
  else if (dataType == nsIDataType::VTYPE_VOID ||
           dataType == nsIDataType::VTYPE_EMPTY) {
    
    aContentType.AssignLiteral("text/plain");
    aCharset.AssignLiteral("UTF-8");
    *aContentLength = 0;

    return NS_OK;
  }

  PRUnichar* data = nullptr;
  uint32_t len = 0;
  rv = aBody->GetAsWStringWithSize(&len, &data);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString string;
  string.Adopt(data, len);

  return GetRequestBody(string, aResult, aContentLength, aContentType, aCharset);
}


nsresult
nsXMLHttpRequest::GetRequestBody(nsIVariant* aVariant,
                                 const Nullable<RequestBody>& aBody,
                                 nsIInputStream** aResult,
                                 uint64_t* aContentLength,
                                 nsACString& aContentType, nsACString& aCharset)
{
  if (aVariant) {
    return ::GetRequestBody(aVariant, aResult, aContentLength, aContentType, aCharset);
  }

  const RequestBody& body = aBody.Value();
  RequestBody::Value value = body.GetValue();
  switch (body.GetType()) {
    case nsXMLHttpRequest::RequestBody::ArrayBuffer:
    {
      return ::GetRequestBody(value.mArrayBuffer->Data(),
                              value.mArrayBuffer->Length(), aResult,
                              aContentLength, aContentType, aCharset);
    }
    case nsXMLHttpRequest::RequestBody::ArrayBufferView:
    {
      return ::GetRequestBody(value.mArrayBufferView->Data(),
                              value.mArrayBufferView->Length(), aResult,
                              aContentLength, aContentType, aCharset);
    }
    case nsXMLHttpRequest::RequestBody::Blob:
    {
      nsresult rv;
      nsCOMPtr<nsIXHRSendable> sendable = do_QueryInterface(value.mBlob, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      return ::GetRequestBody(sendable, aResult, aContentLength, aContentType, aCharset);
    }
    case nsXMLHttpRequest::RequestBody::Document:
    {
      nsCOMPtr<nsIDOMDocument> document = do_QueryInterface(value.mDocument);
      return ::GetRequestBody(document, aResult, aContentLength, aContentType, aCharset);
    }
    case nsXMLHttpRequest::RequestBody::DOMString:
    {
      return ::GetRequestBody(*value.mString, aResult, aContentLength,
                              aContentType, aCharset);
    }
    case nsXMLHttpRequest::RequestBody::FormData:
    {
      MOZ_ASSERT(value.mFormData);
      return ::GetRequestBody(value.mFormData, aResult, aContentLength,
                              aContentType, aCharset);
    }
    case nsXMLHttpRequest::RequestBody::InputStream:
    {
      return ::GetRequestBody(value.mStream, aResult, aContentLength,
                              aContentType, aCharset);
    }
    default:
    {
      return NS_ERROR_FAILURE;
    }
  }

  NS_NOTREACHED("Default cases exist for a reason");
  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::Send(nsIVariant *aBody)
{
  return Send(aBody, Nullable<RequestBody>());
}

nsresult
nsXMLHttpRequest::Send(nsIVariant* aVariant, const Nullable<RequestBody>& aBody)
{
  NS_ENSURE_TRUE(mPrincipal, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = CheckInnerWindowCorrectness();
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (XML_HTTP_REQUEST_SENT & mState) {
    return NS_ERROR_FAILURE;
  }

  
  if (!mChannel || !(XML_HTTP_REQUEST_OPENED & mState)) {
    return NS_ERROR_NOT_INITIALIZED;
  }


  
  
  
  
  if (HasListenersFor(nsGkAtoms::onprogress) ||
      (mUpload && mUpload->HasListenersFor(nsGkAtoms::onprogress))) {
    nsLoadFlags loadFlags;
    mChannel->GetLoadFlags(&loadFlags);
    loadFlags &= ~nsIRequest::LOAD_BACKGROUND;
    loadFlags |= nsIRequest::LOAD_NORMAL;
    mChannel->SetLoadFlags(loadFlags);
  }

  
  
  

  
  
  nsAutoCString method;
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mChannel));

  if (httpChannel) {
    httpChannel->GetRequestMethod(method); 

    if (!IsSystemXHR()) {
      
      
      
      
      
      
      
      
      
      
      

      nsCOMPtr<nsIURI> principalURI;
      mPrincipal->GetURI(getter_AddRefs(principalURI));

      nsIScriptContext* sc = GetContextForEventHandlers(&rv);
      NS_ENSURE_SUCCESS(rv, rv);
      nsCOMPtr<nsIDocument> doc =
        nsContentUtils::GetDocumentFromScriptContext(sc);

      nsCOMPtr<nsIURI> docCurURI;
      nsCOMPtr<nsIURI> docOrigURI;
      if (doc) {
        docCurURI = doc->GetDocumentURI();
        docOrigURI = doc->GetOriginalURI();
      }

      nsCOMPtr<nsIURI> referrerURI;

      if (principalURI && docCurURI && docOrigURI) {
        bool equal = false;
        principalURI->Equals(docOrigURI, &equal);
        if (equal) {
          referrerURI = docCurURI;
        }
      }

      if (!referrerURI)
        referrerURI = principalURI;

      httpChannel->SetReferrer(referrerURI);
    }

    
    
    
    
    
    
    
    nsCOMPtr<nsIUploadChannel2> uploadChannel2 =
      do_QueryInterface(httpChannel);
    if (!uploadChannel2) {
      nsCOMPtr<nsIConsoleService> consoleService =
        do_GetService(NS_CONSOLESERVICE_CONTRACTID);
      if (consoleService) {
        consoleService->LogStringMessage(NS_LITERAL_STRING(
          "Http channel implementation doesn't support nsIUploadChannel2. An extension has supplied a non-functional http protocol handler. This will break behavior and in future releases not work at all."
                                                           ).get());
      }
    }
  }

  mUploadTransferred = 0;
  mUploadTotal = 0;
  
  mUploadComplete = true;
  mErrorLoad = false;
  mLoadLengthComputable = false;
  mLoadTotal = 0;
  if ((aVariant || !aBody.IsNull()) && httpChannel &&
      !method.LowerCaseEqualsLiteral("get") &&
      !method.LowerCaseEqualsLiteral("head")) {

    nsAutoCString charset;
    nsAutoCString defaultContentType;
    nsCOMPtr<nsIInputStream> postDataStream;

    rv = GetRequestBody(aVariant, aBody, getter_AddRefs(postDataStream),
                        &mUploadTotal, defaultContentType, charset);
    NS_ENSURE_SUCCESS(rv, rv);

    if (postDataStream) {
      
      
      nsAutoCString contentType;
      if (NS_FAILED(httpChannel->
                      GetRequestHeader(NS_LITERAL_CSTRING("Content-Type"),
                                       contentType)) ||
          contentType.IsEmpty()) {
        contentType = defaultContentType;
      }

      
      if (!charset.IsEmpty()) {
        nsAutoCString specifiedCharset;
        bool haveCharset;
        int32_t charsetStart, charsetEnd;
        rv = NS_ExtractCharsetFromContentType(contentType, specifiedCharset,
                                              &haveCharset, &charsetStart,
                                              &charsetEnd);
        if (NS_SUCCEEDED(rv)) {
          
          
          
          if (specifiedCharset.Length() >= 2 &&
              specifiedCharset.First() == '\'' &&
              specifiedCharset.Last() == '\'') {
            specifiedCharset = Substring(specifiedCharset, 1,
                                         specifiedCharset.Length() - 2);
          }

          
          
          
          
          
          
          
          if (!specifiedCharset.Equals(charset,
                                       nsCaseInsensitiveCStringComparator())) {
            nsAutoCString newCharset("; charset=");
            newCharset.Append(charset);
            contentType.Replace(charsetStart, charsetEnd - charsetStart,
                                newCharset);
          }
        }
      }

      
      
      if (!NS_InputStreamIsBuffered(postDataStream)) {
        nsCOMPtr<nsIInputStream> bufferedStream;
        rv = NS_NewBufferedInputStream(getter_AddRefs(bufferedStream),
                                       postDataStream, 
                                       4096);
        NS_ENSURE_SUCCESS(rv, rv);

        postDataStream = bufferedStream;
      }

      mUploadComplete = false;

      
      
      nsCOMPtr<nsIUploadChannel2> uploadChannel2(do_QueryInterface(httpChannel));
      
      NS_ASSERTION(uploadChannel2, "http must support nsIUploadChannel2");
      if (uploadChannel2) {
          uploadChannel2->ExplicitSetUploadStream(postDataStream, contentType,
                                                 mUploadTotal, method, false);
      }
      else {
        
        
        if (contentType.IsEmpty()) {
          contentType.AssignLiteral("application/octet-stream");
        }
        nsCOMPtr<nsIUploadChannel> uploadChannel =
          do_QueryInterface(httpChannel);
        uploadChannel->SetUploadStream(postDataStream, contentType, mUploadTotal);
        
        httpChannel->SetRequestMethod(method);
      }
    }
  }

  if (httpChannel) {
    nsAutoCString contentTypeHeader;
    rv = httpChannel->GetRequestHeader(NS_LITERAL_CSTRING("Content-Type"),
                                       contentTypeHeader);
    if (NS_SUCCEEDED(rv)) {
      nsAutoCString contentType, charset;
      rv = NS_ParseContentType(contentTypeHeader, contentType, charset);
      NS_ENSURE_SUCCESS(rv, rv);
  
      if (!contentType.LowerCaseEqualsLiteral("text/plain") &&
          !contentType.LowerCaseEqualsLiteral("application/x-www-form-urlencoded") &&
          !contentType.LowerCaseEqualsLiteral("multipart/form-data")) {
        mCORSUnsafeHeaders.AppendElement(NS_LITERAL_CSTRING("Content-Type"));
      }
    }
  }

  ResetResponse();

  rv = CheckChannelForCrossSiteRequest(mChannel);
  NS_ENSURE_SUCCESS(rv, rv);

  bool withCredentials = !!(mState & XML_HTTP_REQUEST_AC_WITH_CREDENTIALS);

  
  mChannel->GetNotificationCallbacks(getter_AddRefs(mNotificationCallbacks));
  mChannel->SetNotificationCallbacks(this);

  
  
  AddLoadFlags(mChannel, nsIRequest::INHIBIT_PIPELINE);

  nsCOMPtr<nsIHttpChannelInternal>
    internalHttpChannel(do_QueryInterface(mChannel));
  if (internalHttpChannel) {
    
    
    
    internalHttpChannel->SetLoadUnblocked(true);
  }

  nsCOMPtr<nsIStreamListener> listener = this;
  if (!IsSystemXHR()) {
    
    
    nsRefPtr<nsCORSListenerProxy> corsListener =
      new nsCORSListenerProxy(listener, mPrincipal, withCredentials);
    rv = corsListener->Init(mChannel, true);
    NS_ENSURE_SUCCESS(rv, rv);
    listener = corsListener;
  }
  else {
    
    
    

    listener = new nsStreamListenerWrapper(listener);
  }

  if (mIsAnon) {
    AddLoadFlags(mChannel, nsIRequest::LOAD_ANONYMOUS);
  }
  else {
    AddLoadFlags(mChannel, nsIChannel::LOAD_EXPLICIT_CREDENTIALS);
  }

  NS_ASSERTION(listener != this,
               "Using an object as a listener that can't be exposed to JS");

  
  
  
  
  if (method.EqualsLiteral("POST")) {
    AddLoadFlags(mChannel,
        nsIRequest::LOAD_BYPASS_CACHE | nsIRequest::INHIBIT_CACHING);
  }
  
  
  
  else if (!(mState & XML_HTTP_REQUEST_ASYNC)) {
    AddLoadFlags(mChannel,
        nsICachingChannel::LOAD_BYPASS_LOCAL_CACHE_IF_BUSY);
  }

  
  
  
  
  nsAutoCString contentType;
  if (NS_FAILED(mChannel->GetContentType(contentType)) ||
      contentType.IsEmpty() ||
      contentType.Equals(UNKNOWN_CONTENT_TYPE)) {
    mChannel->SetContentType(NS_LITERAL_CSTRING("application/xml"));
  }

  
  mRequestSentTime = PR_Now();
  StartTimeoutTimer();

  
  if (mState & XML_HTTP_REQUEST_NEED_AC_PREFLIGHT) {
    
    

    rv = NS_StartCORSPreflight(mChannel, listener,
                               mPrincipal, withCredentials,
                               mCORSUnsafeHeaders,
                               getter_AddRefs(mCORSPreflightChannel));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    
    rv = mChannel->AsyncOpen(listener, nullptr);
  }

  if (NS_FAILED(rv)) {
    
    mChannel = nullptr;
    mCORSPreflightChannel = nullptr;
    return rv;
  }

  
  mWaitingForOnStopRequest = true;

  
  if (!(mState & XML_HTTP_REQUEST_ASYNC)) {
    mState |= XML_HTTP_REQUEST_SYNCLOOPING;

    nsCOMPtr<nsIDocument> suspendedDoc;
    nsCOMPtr<nsIRunnable> resumeTimeoutRunnable;
    if (GetOwner()) {
      nsCOMPtr<nsIDOMWindow> topWindow;
      if (NS_SUCCEEDED(GetOwner()->GetTop(getter_AddRefs(topWindow)))) {
        nsCOMPtr<nsPIDOMWindow> suspendedWindow(do_QueryInterface(topWindow));
        if (suspendedWindow &&
            (suspendedWindow = suspendedWindow->GetCurrentInnerWindow())) {
          suspendedDoc = suspendedWindow->GetExtantDoc();
          if (suspendedDoc) {
            suspendedDoc->SuppressEventHandling();
          }
          suspendedWindow->SuspendTimeouts(1, false);
          resumeTimeoutRunnable = new nsResumeTimeoutsEvent(suspendedWindow);
        }
      }
    }

    ChangeState(XML_HTTP_REQUEST_SENT);

    {
      nsAutoSyncOperation sync(suspendedDoc);
      
      
      nsIThread *thread = NS_GetCurrentThread();
      while (mState & XML_HTTP_REQUEST_SYNCLOOPING) {
        if (!NS_ProcessNextEvent(thread)) {
          rv = NS_ERROR_UNEXPECTED;
          break;
        }
      }
    }

    if (suspendedDoc) {
      suspendedDoc->UnsuppressEventHandlingAndFireEvents(true);
    }

    if (resumeTimeoutRunnable) {
      NS_DispatchToCurrentThread(resumeTimeoutRunnable);
    }
  } else {
    
    
    
    
    ChangeState(XML_HTTP_REQUEST_SENT);
    if (mUpload && mUpload->HasListenersFor(nsGkAtoms::onprogress)) {
      StartProgressEventTimer();
    }
    DispatchProgressEvent(this, NS_LITERAL_STRING(LOADSTART_STR), false,
                          0, 0);
    if (mUpload && !mUploadComplete) {
      DispatchProgressEvent(mUpload, NS_LITERAL_STRING(LOADSTART_STR), true,
                            0, mUploadTotal);
    }
  }

  if (!mChannel) {
    return NS_ERROR_FAILURE;
  }

  return rv;
}



NS_IMETHODIMP
nsXMLHttpRequest::SetRequestHeader(const nsACString& header,
                                   const nsACString& value)
{
  
  if (!(mState & XML_HTTP_REQUEST_OPENED)) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }
  NS_ASSERTION(mChannel, "mChannel must be valid if we're OPENED.");

  
  
  if (!IsValidHTTPToken(header)) { 
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  
  
  
  if (mCORSPreflightChannel) {
    bool pending;
    nsresult rv = mCORSPreflightChannel->IsPending(&pending);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (pending) {
      return NS_ERROR_IN_PROGRESS;
    }
  }

  if (!mChannel)             
    return NS_ERROR_FAILURE; 

  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(mChannel);
  if (!httpChannel) {
    return NS_OK;
  }

  
  
  
  
  
  bool mergeHeaders = true;

  
  
  bool isInvalidHeader = false;
  static const char *kInvalidHeaders[] = {
    "accept-charset", "accept-encoding", "access-control-request-headers",
    "access-control-request-method", "connection", "content-length",
    "cookie", "cookie2", "content-transfer-encoding", "date", "dnt",
    "expect", "host", "keep-alive", "origin", "referer", "te", "trailer",
    "transfer-encoding", "upgrade", "user-agent", "via"
  };
  uint32_t i;
  for (i = 0; i < ArrayLength(kInvalidHeaders); ++i) {
    if (header.LowerCaseEqualsASCII(kInvalidHeaders[i])) {
      isInvalidHeader = true;
      break;
    }
  }

  if (!IsSystemXHR()) {
    
    if (isInvalidHeader) {
      NS_WARNING("refusing to set request header");
      return NS_OK;
    }
    if (StringBeginsWith(header, NS_LITERAL_CSTRING("proxy-"),
                         nsCaseInsensitiveCStringComparator()) ||
        StringBeginsWith(header, NS_LITERAL_CSTRING("sec-"),
                         nsCaseInsensitiveCStringComparator())) {
      NS_WARNING("refusing to set request header");
      return NS_OK;
    }

    
    bool safeHeader = IsSystemXHR();
    if (!safeHeader) {
      
      const char *kCrossOriginSafeHeaders[] = {
        "accept", "accept-language", "content-language", "content-type",
        "last-event-id"
      };
      for (i = 0; i < ArrayLength(kCrossOriginSafeHeaders); ++i) {
        if (header.LowerCaseEqualsASCII(kCrossOriginSafeHeaders[i])) {
          safeHeader = true;
          break;
        }
      }
    }

    if (!safeHeader) {
      if (!mCORSUnsafeHeaders.Contains(header)) {
        mCORSUnsafeHeaders.AppendElement(header);
      }
    }
  } else {
    
    if (isInvalidHeader) {
      mergeHeaders = false;
    }
  }

  if (!mAlreadySetHeaders.Contains(header)) {
    
    mergeHeaders = false;
  }

  
  nsresult rv = httpChannel->SetRequestHeader(header, value, mergeHeaders);
  if (rv == NS_ERROR_INVALID_ARG) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }
  if (NS_SUCCEEDED(rv)) {
    
    mAlreadySetHeaders.PutEntry(nsCString(header));

    
    RequestHeader reqHeader = {
      nsCString(header), nsCString(value)
    };
    mModifiedRequestHeaders.AppendElement(reqHeader);
  }
  return rv;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetTimeout(uint32_t *aTimeout)
{
  *aTimeout = Timeout();
  return NS_OK;
}

NS_IMETHODIMP
nsXMLHttpRequest::SetTimeout(uint32_t aTimeout)
{
  ErrorResult rv;
  SetTimeout(aTimeout, rv);
  return rv.ErrorCode();
}

void
nsXMLHttpRequest::SetTimeout(uint32_t aTimeout, ErrorResult& aRv)
{
  if (!(mState & (XML_HTTP_REQUEST_ASYNC | XML_HTTP_REQUEST_UNSENT)) &&
      HasOrHasHadOwner()) {
    

    LogMessage("TimeoutSyncXHRWarning", GetOwner());
    aRv.Throw(NS_ERROR_DOM_INVALID_ACCESS_ERR);
    return;
  }

  mTimeoutMilliseconds = aTimeout;
  if (mRequestSentTime) {
    StartTimeoutTimer();
  }
}

void
nsXMLHttpRequest::StartTimeoutTimer()
{
  NS_ABORT_IF_FALSE(mRequestSentTime,
                    "StartTimeoutTimer mustn't be called before the request was sent!");
  if (mState & XML_HTTP_REQUEST_DONE) {
    
    return;
  }

  if (mTimeoutTimer) {
    mTimeoutTimer->Cancel();
  }

  if (!mTimeoutMilliseconds) {
    return;
  }

  if (!mTimeoutTimer) {
    mTimeoutTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  }
  uint32_t elapsed =
    (uint32_t)((PR_Now() - mRequestSentTime) / PR_USEC_PER_MSEC);
  mTimeoutTimer->InitWithCallback(
    this,
    mTimeoutMilliseconds > elapsed ? mTimeoutMilliseconds - elapsed : 0,
    nsITimer::TYPE_ONE_SHOT
  );
}


NS_IMETHODIMP
nsXMLHttpRequest::GetReadyState(uint16_t *aState)
{
  *aState = ReadyState();
  return NS_OK;
}

uint16_t
nsXMLHttpRequest::ReadyState()
{
  
  if (mState & XML_HTTP_REQUEST_UNSENT) {
    return UNSENT;
  }
  if (mState & (XML_HTTP_REQUEST_OPENED | XML_HTTP_REQUEST_SENT)) {
    return OPENED;
  }
  if (mState & XML_HTTP_REQUEST_HEADERS_RECEIVED) {
    return HEADERS_RECEIVED;
  }
  if (mState & XML_HTTP_REQUEST_LOADING) {
    return LOADING;
  }
  MOZ_ASSERT(mState & XML_HTTP_REQUEST_DONE);
  return DONE;
}


NS_IMETHODIMP
nsXMLHttpRequest::SlowOverrideMimeType(const nsAString& aMimeType)
{
  OverrideMimeType(aMimeType);
  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetMozBackgroundRequest(bool *_retval)
{
  *_retval = MozBackgroundRequest();
  return NS_OK;
}

bool
nsXMLHttpRequest::MozBackgroundRequest()
{
  return !!(mState & XML_HTTP_REQUEST_BACKGROUND);
}

NS_IMETHODIMP
nsXMLHttpRequest::SetMozBackgroundRequest(bool aMozBackgroundRequest)
{
  nsresult rv = NS_OK;
  SetMozBackgroundRequest(aMozBackgroundRequest, rv);
  return rv;
}

void
nsXMLHttpRequest::SetMozBackgroundRequest(bool aMozBackgroundRequest, nsresult& aRv)
{
  if (!IsSystemXHR()) {
    aRv = NS_ERROR_DOM_SECURITY_ERR;
    return;
  }

  if (!(mState & XML_HTTP_REQUEST_UNSENT)) {
    
    aRv = NS_ERROR_IN_PROGRESS;
    return;
  }

  if (aMozBackgroundRequest) {
    mState |= XML_HTTP_REQUEST_BACKGROUND;
  } else {
    mState &= ~XML_HTTP_REQUEST_BACKGROUND;
  }
}


NS_IMETHODIMP
nsXMLHttpRequest::GetWithCredentials(bool *_retval)
{
  *_retval = WithCredentials();
  return NS_OK;
}

bool
nsXMLHttpRequest::WithCredentials()
{
  return !!(mState & XML_HTTP_REQUEST_AC_WITH_CREDENTIALS);
}

NS_IMETHODIMP
nsXMLHttpRequest::SetWithCredentials(bool aWithCredentials)
{
  ErrorResult rv;
  SetWithCredentials(aWithCredentials, rv);
  return rv.ErrorCode();
}

void
nsXMLHttpRequest::SetWithCredentials(bool aWithCredentials, ErrorResult& aRv)
{
  
  if (XML_HTTP_REQUEST_SENT & mState) {
    aRv = NS_ERROR_FAILURE;
    return;
  }

  
  if (HasOrHasHadOwner() &&
      !(mState & (XML_HTTP_REQUEST_UNSENT | XML_HTTP_REQUEST_ASYNC))) {
    LogMessage("WithCredentialsSyncXHRWarning", GetOwner());
    aRv.Throw(NS_ERROR_DOM_INVALID_ACCESS_ERR);
    return;
  }

  if (aWithCredentials) {
    mState |= XML_HTTP_REQUEST_AC_WITH_CREDENTIALS;
  } else {
    mState &= ~XML_HTTP_REQUEST_AC_WITH_CREDENTIALS;
  }
}

nsresult
nsXMLHttpRequest::ChangeState(uint32_t aState, bool aBroadcast)
{
  
  
  if (aState & XML_HTTP_REQUEST_LOADSTATES) {
    mState &= ~XML_HTTP_REQUEST_LOADSTATES;
  }
  mState |= aState;
  nsresult rv = NS_OK;

  if (mProgressNotifier &&
      !(aState & (XML_HTTP_REQUEST_HEADERS_RECEIVED | XML_HTTP_REQUEST_LOADING))) {
    mProgressTimerIsActive = false;
    mProgressNotifier->Cancel();
  }

  if ((aState & XML_HTTP_REQUEST_LOADSTATES) &&  
      aState != XML_HTTP_REQUEST_SENT && 
      aBroadcast &&
      (mState & XML_HTTP_REQUEST_ASYNC ||
       aState & XML_HTTP_REQUEST_OPENED ||
       aState & XML_HTTP_REQUEST_DONE)) {
    nsCOMPtr<nsIDOMEvent> event;
    rv = CreateReadystatechangeEvent(getter_AddRefs(event));
    NS_ENSURE_SUCCESS(rv, rv);

    DispatchDOMEvent(nullptr, event, nullptr, nullptr);
  }

  return rv;
}





class AsyncVerifyRedirectCallbackForwarder MOZ_FINAL : public nsIAsyncVerifyRedirectCallback
{
public:
  AsyncVerifyRedirectCallbackForwarder(nsXMLHttpRequest *xhr)
    : mXHR(xhr)
  {
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(AsyncVerifyRedirectCallbackForwarder)

  
  NS_IMETHOD OnRedirectVerifyCallback(nsresult result)
  {
    mXHR->OnRedirectVerifyCallback(result);

    return NS_OK;
  }

private:
  nsRefPtr<nsXMLHttpRequest> mXHR;
};

NS_IMPL_CYCLE_COLLECTION_1(AsyncVerifyRedirectCallbackForwarder, mXHR)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AsyncVerifyRedirectCallbackForwarder)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIAsyncVerifyRedirectCallback)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(AsyncVerifyRedirectCallbackForwarder)
NS_IMPL_CYCLE_COLLECTING_RELEASE(AsyncVerifyRedirectCallbackForwarder)





NS_IMETHODIMP
nsXMLHttpRequest::AsyncOnChannelRedirect(nsIChannel *aOldChannel,
                                         nsIChannel *aNewChannel,
                                         uint32_t    aFlags,
                                         nsIAsyncVerifyRedirectCallback *callback)
{
  NS_PRECONDITION(aNewChannel, "Redirect without a channel?");

  nsresult rv;

  if (!NS_IsInternalSameURIRedirect(aOldChannel, aNewChannel, aFlags)) {
    rv = CheckChannelForCrossSiteRequest(aNewChannel);
    if (NS_FAILED(rv)) {
      NS_WARNING("nsXMLHttpRequest::OnChannelRedirect: "
                 "CheckChannelForCrossSiteRequest returned failure");
      return rv;
    }

    
    
    
    if ((mState & XML_HTTP_REQUEST_NEED_AC_PREFLIGHT)) {
       return NS_ERROR_DOM_BAD_URI;
    }
  }

  
  mRedirectCallback = callback;
  mNewRedirectChannel = aNewChannel;

  if (mChannelEventSink) {
    nsRefPtr<AsyncVerifyRedirectCallbackForwarder> fwd =
      new AsyncVerifyRedirectCallbackForwarder(this);

    rv = mChannelEventSink->AsyncOnChannelRedirect(aOldChannel,
                                                   aNewChannel,
                                                   aFlags, fwd);
    if (NS_FAILED(rv)) {
        mRedirectCallback = nullptr;
        mNewRedirectChannel = nullptr;
    }
    return rv;
  }
  OnRedirectVerifyCallback(NS_OK);
  return NS_OK;
}

void
nsXMLHttpRequest::OnRedirectVerifyCallback(nsresult result)
{
  NS_ASSERTION(mRedirectCallback, "mRedirectCallback not set in callback");
  NS_ASSERTION(mNewRedirectChannel, "mNewRedirectChannel not set in callback");

  if (NS_SUCCEEDED(result)) {
    mChannel = mNewRedirectChannel;

    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mChannel));
    if (httpChannel) {
      
      for (uint32_t i = mModifiedRequestHeaders.Length(); i > 0; ) {
        --i;
        httpChannel->SetRequestHeader(mModifiedRequestHeaders[i].header,
                                      mModifiedRequestHeaders[i].value,
                                      false);
      }
    }
  } else {
    mErrorLoad = true;
  }

  mNewRedirectChannel = nullptr;

  mRedirectCallback->OnRedirectVerifyCallback(result);
  mRedirectCallback = nullptr;
}





void
nsXMLHttpRequest::MaybeDispatchProgressEvents(bool aFinalProgress)
{
  if (aFinalProgress && mProgressTimerIsActive) {
    mProgressTimerIsActive = false;
    mProgressNotifier->Cancel();
  }

  if (mProgressTimerIsActive ||
      !mProgressSinceLastProgressEvent ||
      mErrorLoad ||
      !(mState & XML_HTTP_REQUEST_ASYNC)) {
    return;
  }

  if (!aFinalProgress) {
    StartProgressEventTimer();
  }

  
  
  if ((XML_HTTP_REQUEST_OPENED | XML_HTTP_REQUEST_SENT) & mState) {
    if (aFinalProgress) {
      mUploadTotal = mUploadTransferred;
    }
    if (mUpload && !mUploadComplete) {
      DispatchProgressEvent(mUpload, NS_LITERAL_STRING(PROGRESS_STR),
                            mUploadLengthComputable, mUploadTransferred,
                            mUploadTotal);
    }
  } else {
    if (aFinalProgress) {
      mLoadTotal = mLoadTransferred;
    }
    mInLoadProgressEvent = true;
    DispatchProgressEvent(this, NS_LITERAL_STRING(PROGRESS_STR),
                          mLoadLengthComputable, mLoadTransferred,
                          mLoadTotal);
    mInLoadProgressEvent = false;
    if (mResponseType == XML_HTTP_RESPONSE_TYPE_CHUNKED_TEXT ||
        mResponseType == XML_HTTP_RESPONSE_TYPE_CHUNKED_ARRAYBUFFER) {
      mResponseBody.Truncate();
      mResponseText.Truncate();
      mResultArrayBuffer = nullptr;
      mArrayBufferBuilder.reset();
    }
  }

  mProgressSinceLastProgressEvent = false;
}

NS_IMETHODIMP
nsXMLHttpRequest::OnProgress(nsIRequest *aRequest, nsISupports *aContext, uint64_t aProgress, uint64_t aProgressMax)
{
  
  
  bool upload = !!((XML_HTTP_REQUEST_OPENED | XML_HTTP_REQUEST_SENT) & mState);
  
  
  bool lengthComputable = (aProgressMax != UINT64_MAX);
  if (upload) {
    uint64_t loaded = aProgress;
    uint64_t total = aProgressMax;
    if (lengthComputable) {
      uint64_t headerSize = aProgressMax - mUploadTotal;
      loaded -= headerSize;
      total -= headerSize;
    }
    mUploadLengthComputable = lengthComputable;
    mUploadTransferred = loaded;
    mProgressSinceLastProgressEvent = true;

    MaybeDispatchProgressEvents(false);
  } else {
    mLoadLengthComputable = lengthComputable;
    mLoadTotal = lengthComputable ? aProgressMax : 0;
    
    
    
  }

  if (mProgressEventSink) {
    mProgressEventSink->OnProgress(aRequest, aContext, aProgress,
                                   aProgressMax);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXMLHttpRequest::OnStatus(nsIRequest *aRequest, nsISupports *aContext, nsresult aStatus, const PRUnichar *aStatusArg)
{
  if (mProgressEventSink) {
    mProgressEventSink->OnStatus(aRequest, aContext, aStatus, aStatusArg);
  }

  return NS_OK;
}

bool
nsXMLHttpRequest::AllowUploadProgress()
{
  return !(mState & XML_HTTP_REQUEST_USE_XSITE_AC) ||
    (mState & XML_HTTP_REQUEST_NEED_AC_PREFLIGHT);
}




NS_IMETHODIMP
nsXMLHttpRequest::GetInterface(const nsIID & aIID, void **aResult)
{
  nsresult rv;

  
  
  
  
  if (aIID.Equals(NS_GET_IID(nsIChannelEventSink))) {
    mChannelEventSink = do_GetInterface(mNotificationCallbacks);
    *aResult = static_cast<nsIChannelEventSink*>(EnsureXPCOMifier().get());
    return NS_OK;
  } else if (aIID.Equals(NS_GET_IID(nsIProgressEventSink))) {
    mProgressEventSink = do_GetInterface(mNotificationCallbacks);
    *aResult = static_cast<nsIProgressEventSink*>(EnsureXPCOMifier().get());
    return NS_OK;
  }

  
  
  if (mNotificationCallbacks) {
    rv = mNotificationCallbacks->GetInterface(aIID, aResult);
    if (NS_SUCCEEDED(rv)) {
      NS_ASSERTION(*aResult, "Lying nsIInterfaceRequestor implementation!");
      return rv;
    }
  }

  if (mState & XML_HTTP_REQUEST_BACKGROUND) {
    nsCOMPtr<nsIInterfaceRequestor> badCertHandler(do_CreateInstance(NS_BADCERTHANDLER_CONTRACTID, &rv));

    
    
    if (NS_SUCCEEDED(rv)) {
      rv = badCertHandler->GetInterface(aIID, aResult);
      if (NS_SUCCEEDED(rv))
        return rv;
    }
  }
  else if (aIID.Equals(NS_GET_IID(nsIAuthPrompt)) ||
           aIID.Equals(NS_GET_IID(nsIAuthPrompt2))) {

    nsCOMPtr<nsIURI> uri;
    rv = mChannel->GetURI(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    bool showPrompt = true;

    
    
    





    
    if (showPrompt) {
      for (uint32_t i = 0, len = mModifiedRequestHeaders.Length(); i < len; ++i) {
        if (mModifiedRequestHeaders[i].header.
              LowerCaseEqualsLiteral("authorization")) {
          showPrompt = false;
          break;
        }
      }
    }

    
    if (showPrompt) {

      nsCString username;
      rv = uri->GetUsername(username);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCString password;
      rv = uri->GetPassword(password);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!username.IsEmpty() || !password.IsEmpty()) {
        showPrompt = false;
      }
    }

    
    if (!showPrompt) {
      nsRefPtr<XMLHttpRequestAuthPrompt> prompt = new XMLHttpRequestAuthPrompt();
      if (!prompt)
        return NS_ERROR_OUT_OF_MEMORY;

      return prompt->QueryInterface(aIID, aResult);
    }

    nsCOMPtr<nsIPromptFactory> wwatch =
      do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    
    

    nsCOMPtr<nsIDOMWindow> window;
    if (GetOwner()) {
      window = GetOwner()->GetOuterWindow();
    }

    return wwatch->GetPrompt(window, aIID,
                             reinterpret_cast<void**>(aResult));
  }
  
  
  
  else if (aIID.Equals(NS_GET_IID(nsIStreamListener))) {
    *aResult = static_cast<nsIStreamListener*>(EnsureXPCOMifier().get());
    return NS_OK;
  }
  else if (aIID.Equals(NS_GET_IID(nsIRequestObserver))) {
    *aResult = static_cast<nsIRequestObserver*>(EnsureXPCOMifier().get());
    return NS_OK;
  }
  else if (aIID.Equals(NS_GET_IID(nsITimerCallback))) {
    *aResult = static_cast<nsITimerCallback*>(EnsureXPCOMifier().get());
    return NS_OK;
  }

  return QueryInterface(aIID, aResult);
}

JS::Value
nsXMLHttpRequest::GetInterface(JSContext* aCx, nsIJSID* aIID, ErrorResult& aRv)
{
  const nsID* iid = aIID->GetID();
  nsCOMPtr<nsISupports> result;
  JS::Rooted<JS::Value> v(aCx, JSVAL_NULL);
  aRv = GetInterface(*iid, getter_AddRefs(result));
  NS_ENSURE_FALSE(aRv.Failed(), JSVAL_NULL);

  JS::Rooted<JSObject*> wrapper(aCx, GetWrapper());
  JSAutoCompartment ac(aCx, wrapper);
  JS::Rooted<JSObject*> global(aCx, JS_GetGlobalForObject(aCx, wrapper));
  aRv = nsContentUtils::WrapNative(aCx, global, result, iid, &v);
  return aRv.Failed() ? JSVAL_NULL : v;
}

nsXMLHttpRequestUpload*
nsXMLHttpRequest::Upload()
{
  if (!mUpload) {
    mUpload = new nsXMLHttpRequestUpload(this);
  }
  return mUpload;
}

NS_IMETHODIMP
nsXMLHttpRequest::GetUpload(nsIXMLHttpRequestUpload** aUpload)
{
  nsRefPtr<nsXMLHttpRequestUpload> upload = Upload();
  upload.forget(aUpload);
  return NS_OK;
}

bool
nsXMLHttpRequest::MozAnon()
{
  return mIsAnon;
}

NS_IMETHODIMP
nsXMLHttpRequest::GetMozAnon(bool* aAnon)
{
  *aAnon = MozAnon();
  return NS_OK;
}

bool
nsXMLHttpRequest::MozSystem()
{
  return IsSystemXHR();
}

NS_IMETHODIMP
nsXMLHttpRequest::GetMozSystem(bool* aSystem)
{
  *aSystem = MozSystem();
  return NS_OK;
}

void
nsXMLHttpRequest::HandleTimeoutCallback()
{
  if (mState & XML_HTTP_REQUEST_DONE) {
    NS_NOTREACHED("nsXMLHttpRequest::HandleTimeoutCallback with completed request");
    
    return;
  }

  CloseRequestWithError(NS_LITERAL_STRING(TIMEOUT_STR),
                        XML_HTTP_REQUEST_TIMED_OUT);
}

NS_IMETHODIMP
nsXMLHttpRequest::Notify(nsITimer* aTimer)
{
  if (mProgressNotifier == aTimer) {
    HandleProgressTimerCallback();
    return NS_OK;
  }

  if (mTimeoutTimer == aTimer) {
    HandleTimeoutCallback();
    return NS_OK;
  }

  
  NS_WARNING("Unexpected timer!");
  return NS_ERROR_INVALID_POINTER;
}

void
nsXMLHttpRequest::HandleProgressTimerCallback()
{
  mProgressTimerIsActive = false;
  MaybeDispatchProgressEvents(false);
}

void
nsXMLHttpRequest::StartProgressEventTimer()
{
  if (!mProgressNotifier) {
    mProgressNotifier = do_CreateInstance(NS_TIMER_CONTRACTID);
  }
  if (mProgressNotifier) {
    mProgressTimerIsActive = true;
    mProgressNotifier->Cancel();
    mProgressNotifier->InitWithCallback(this, NS_PROGRESS_EVENT_INTERVAL,
                                        nsITimer::TYPE_ONE_SHOT);
  }
}

already_AddRefed<nsXMLHttpRequestXPCOMifier>
nsXMLHttpRequest::EnsureXPCOMifier()
{
  if (!mXPCOMifier) {
    mXPCOMifier = new nsXMLHttpRequestXPCOMifier(this);
  }
  nsRefPtr<nsXMLHttpRequestXPCOMifier> newRef(mXPCOMifier);
  return newRef.forget();
}

NS_IMPL_ISUPPORTS1(nsXMLHttpRequest::nsHeaderVisitor, nsIHttpHeaderVisitor)

NS_IMETHODIMP nsXMLHttpRequest::
nsHeaderVisitor::VisitHeader(const nsACString &header, const nsACString &value)
{
  if (mXHR->IsSafeHeader(header, mHttpChannel)) {
    mHeaders.Append(header);
    mHeaders.Append(": ");
    mHeaders.Append(value);
    mHeaders.Append("\r\n");
  }
  return NS_OK;
}


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsXMLHttpRequestXPCOMifier)
  NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
  NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
  NS_INTERFACE_MAP_ENTRY(nsIChannelEventSink)
  NS_INTERFACE_MAP_ENTRY(nsIProgressEventSink)
  NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStreamListener)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsXMLHttpRequestXPCOMifier)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsXMLHttpRequestXPCOMifier)



NS_IMPL_CYCLE_COLLECTION_CLASS(nsXMLHttpRequestXPCOMifier)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsXMLHttpRequestXPCOMifier)
if (tmp->mXHR) {
  tmp->mXHR->mXPCOMifier = nullptr;
}
NS_IMPL_CYCLE_COLLECTION_UNLINK(mXHR)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXMLHttpRequestXPCOMifier)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mXHR)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMETHODIMP
nsXMLHttpRequestXPCOMifier::GetInterface(const nsIID & aIID, void **aResult)
{
  
  
  if (!aIID.Equals(NS_GET_IID(nsIInterfaceRequestor))) {
    nsresult rv = QueryInterface(aIID, aResult);
    if (NS_SUCCEEDED(rv)) {
      return rv;
    }
  }

  return mXHR->GetInterface(aIID, aResult);
}

namespace mozilla {

ArrayBufferBuilder::ArrayBufferBuilder()
  : mRawContents(nullptr),
    mDataPtr(nullptr),
    mCapacity(0),
    mLength(0)
{
}

ArrayBufferBuilder::~ArrayBufferBuilder()
{
  reset();
}

void
ArrayBufferBuilder::reset()
{
  if (mRawContents) {
    JS_free(nullptr, mRawContents);
  }
  mRawContents = mDataPtr = nullptr;
  mCapacity = mLength = 0;
}

bool
ArrayBufferBuilder::setCapacity(uint32_t aNewCap)
{
  if (!JS_ReallocateArrayBufferContents(nullptr, aNewCap, &mRawContents, &mDataPtr)) {
    return false;
  }

  mCapacity = aNewCap;
  if (mLength > aNewCap) {
    mLength = aNewCap;
  }

  return true;
}

bool
ArrayBufferBuilder::append(const uint8_t *aNewData, uint32_t aDataLen,
                           uint32_t aMaxGrowth)
{
  if (mLength + aDataLen > mCapacity) {
    uint32_t newcap;
    
    if (!aMaxGrowth || mCapacity < aMaxGrowth) {
      newcap = mCapacity * 2;
    } else {
      newcap = mCapacity + aMaxGrowth;
    }

    
    if (newcap < mLength + aDataLen) {
      newcap = mLength + aDataLen;
    }

    
    if (newcap < mCapacity) {
      return false;
    }

    if (!setCapacity(newcap)) {
      return false;
    }
  }

  
  MOZ_ASSERT(!areOverlappingRegions(aNewData, aDataLen, mDataPtr + mLength,
                                    aDataLen));

  memcpy(mDataPtr + mLength, aNewData, aDataLen);
  mLength += aDataLen;

  return true;
}

JSObject*
ArrayBufferBuilder::getArrayBuffer(JSContext* aCx)
{
  
  
  if (mCapacity > mLength || mLength == 0) {
    if (!setCapacity(mLength)) {
      return nullptr;
    }
  }

  JSObject* obj = JS_NewArrayBufferWithContents(aCx, mRawContents);
  if (!obj) {
    return nullptr;
  }

  mRawContents = mDataPtr = nullptr;
  mLength = mCapacity = 0;

  return obj;
}

 bool
ArrayBufferBuilder::areOverlappingRegions(const uint8_t* aStart1,
                                          uint32_t aLength1,
                                          const uint8_t* aStart2,
                                          uint32_t aLength2)
{
  const uint8_t* end1 = aStart1 + aLength1;
  const uint8_t* end2 = aStart2 + aLength2;

  const uint8_t* max_start = aStart1 > aStart2 ? aStart1 : aStart2;
  const uint8_t* min_end   = end1 < end2 ? end1 : end2;

  return max_start < min_end;
}

} 
