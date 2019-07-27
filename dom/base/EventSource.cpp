




#include "mozilla/dom/EventSource.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/LoadInfo.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/EventSourceBinding.h"
#include "mozilla/dom/MessageEvent.h"
#include "mozilla/dom/ScriptSettings.h"

#include "nsNetUtil.h"
#include "nsMimeTypes.h"
#include "nsIPromptFactory.h"
#include "nsIWindowWatcher.h"
#include "nsPresContext.h"
#include "nsContentPolicyUtils.h"
#include "nsIStringBundle.h"
#include "nsIConsoleService.h"
#include "nsIObserverService.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsJSUtils.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsIScriptError.h"
#include "mozilla/dom/EncodingUtils.h"
#include "nsIContentSecurityPolicy.h"
#include "nsContentUtils.h"
#include "mozilla/Preferences.h"
#include "xpcpublic.h"
#include "nsCORSListenerProxy.h"
#include "nsWrapperCacheInlines.h"
#include "mozilla/Attributes.h"
#include "nsError.h"

namespace mozilla {
namespace dom {

#define REPLACEMENT_CHAR     (char16_t)0xFFFD
#define BOM_CHAR             (char16_t)0xFEFF
#define SPACE_CHAR           (char16_t)0x0020
#define CR_CHAR              (char16_t)0x000D
#define LF_CHAR              (char16_t)0x000A
#define COLON_CHAR           (char16_t)0x003A

#define DEFAULT_BUFFER_SIZE 4096



#define MIN_RECONNECTION_TIME_VALUE       500
#define DEFAULT_RECONNECTION_TIME_VALUE   5000
#define MAX_RECONNECTION_TIME_VALUE       PR_IntervalToMilliseconds(DELAY_INTERVAL_LIMIT)

EventSource::EventSource(nsPIDOMWindow* aOwnerWindow) :
  DOMEventTargetHelper(aOwnerWindow),
  mStatus(PARSE_STATE_OFF),
  mFrozen(false),
  mErrorLoadOnRedirect(false),
  mGoingToDispatchAllMessages(false),
  mWithCredentials(false),
  mWaitingForOnStopRequest(false),
  mLastConvertionResult(NS_OK),
  mReadyState(CONNECTING),
  mScriptLine(0),
  mInnerWindowID(0)
{
}

EventSource::~EventSource()
{
  Close();
}





NS_IMPL_CYCLE_COLLECTION_CLASS(EventSource)

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_BEGIN(EventSource)
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

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_BEGIN(EventSource)
  return tmp->IsBlack();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_BEGIN(EventSource)
  return tmp->IsBlack();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(EventSource,
                                               DOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(EventSource,
                                                  DOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mSrc)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mNotificationCallbacks)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mLoadGroup)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mChannelEventSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mHttpChannel)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTimer)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mUnicodeDecoder)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(EventSource,
                                                DOMEventTargetHelper)
  tmp->Close();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(EventSource)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
  NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
  NS_INTERFACE_MAP_ENTRY(nsIChannelEventSink)
  NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(EventSource, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(EventSource, DOMEventTargetHelper)

void
EventSource::DisconnectFromOwner()
{
  DOMEventTargetHelper::DisconnectFromOwner();
  Close();
}

void
EventSource::Close()
{
  if (mReadyState == CLOSED) {
    return;
  }

  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os) {
    os->RemoveObserver(this, DOM_WINDOW_DESTROYED_TOPIC);
    os->RemoveObserver(this, DOM_WINDOW_FROZEN_TOPIC);
    os->RemoveObserver(this, DOM_WINDOW_THAWED_TOPIC);
  }

  if (mTimer) {
    mTimer->Cancel();
    mTimer = nullptr;
  }

  ResetConnection();

  ClearFields();

  while (mMessagesToDispatch.GetSize() != 0) {
    delete static_cast<Message*>(mMessagesToDispatch.PopFront());
  }

  mSrc = nullptr;
  mFrozen = false;

  mUnicodeDecoder = nullptr;

  mReadyState = CLOSED;
}

nsresult
EventSource::Init(nsISupports* aOwner,
                  const nsAString& aURL,
                  bool aWithCredentials)
{
  if (mReadyState != CONNECTING || !PrefEnabled()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface(aOwner);
  NS_ENSURE_STATE(sgo);
  nsCOMPtr<nsIScriptContext> scriptContext = sgo->GetContext();
  NS_ENSURE_STATE(scriptContext);

  nsCOMPtr<nsIScriptObjectPrincipal> scriptPrincipal =
    do_QueryInterface(aOwner);
  NS_ENSURE_STATE(scriptPrincipal);
  nsCOMPtr<nsIPrincipal> principal = scriptPrincipal->GetPrincipal();
  NS_ENSURE_STATE(principal);

  mPrincipal = principal;
  mWithCredentials = aWithCredentials;

  
  if (JSContext *cx = nsContentUtils::GetCurrentJSContext()) {
    nsJSUtils::GetCallingLocation(cx, mScriptFile, &mScriptLine);
    mInnerWindowID = nsJSUtils::GetCurrentlyRunningCodeInnerWindowID(cx);
  }

  
  
  
  nsresult rv;
  nsIScriptContext* sc = GetContextForEventHandlers(&rv);
  if (sc) {
    nsCOMPtr<nsIDocument> doc =
      nsContentUtils::GetDocumentFromScriptContext(sc);
    if (doc) {
      mLoadGroup = doc->GetDocumentLoadGroup();
    }
  }

  
  nsCOMPtr<nsIURI> baseURI;
  rv = GetBaseURI(getter_AddRefs(baseURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> srcURI;
  rv = NS_NewURI(getter_AddRefs(srcURI), aURL, nullptr, baseURI);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_SYNTAX_ERR);

  
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  NS_ENSURE_STATE(os);

  rv = os->AddObserver(this, DOM_WINDOW_DESTROYED_TOPIC, true);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = os->AddObserver(this, DOM_WINDOW_FROZEN_TOPIC, true);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = os->AddObserver(this, DOM_WINDOW_THAWED_TOPIC, true);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString origin;
  rv = nsContentUtils::GetUTFOrigin(srcURI, origin);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString spec;
  rv = srcURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  mOriginalURL = NS_ConvertUTF8toUTF16(spec);
  mSrc = srcURI;
  mOrigin = origin;

  mReconnectionTime =
    Preferences::GetInt("dom.server-events.default-reconnection-time",
                        DEFAULT_RECONNECTION_TIME_VALUE);

  mUnicodeDecoder = EncodingUtils::DecoderForEncoding("UTF-8");

  
  
  
  InitChannelAndRequestEventSource();

  return NS_OK;
}

 JSObject*
EventSource::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return EventSourceBinding::Wrap(aCx, this, aGivenProto);
}

 already_AddRefed<EventSource>
EventSource::Constructor(const GlobalObject& aGlobal,
                         const nsAString& aURL,
                         const EventSourceInit& aEventSourceInitDict,
                         ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> ownerWindow =
    do_QueryInterface(aGlobal.GetAsSupports());
  if (!ownerWindow) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }
  MOZ_ASSERT(ownerWindow->IsInnerWindow());

  nsRefPtr<EventSource> eventSource = new EventSource(ownerWindow);
  aRv = eventSource->Init(aGlobal.GetAsSupports(), aURL,
                          aEventSourceInitDict.mWithCredentials);
  return eventSource.forget();
}





NS_IMETHODIMP
EventSource::Observe(nsISupports* aSubject,
                     const char* aTopic,
                     const char16_t* aData)
{
  if (mReadyState == CLOSED) {
    return NS_OK;
  }

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aSubject);
  if (!GetOwner() || window != GetOwner()) {
    return NS_OK;
  }

  DebugOnly<nsresult> rv;
  if (strcmp(aTopic, DOM_WINDOW_FROZEN_TOPIC) == 0) {
    rv = Freeze();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Freeze() failed");
  } else if (strcmp(aTopic, DOM_WINDOW_THAWED_TOPIC) == 0) {
    rv = Thaw();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Thaw() failed");
  } else if (strcmp(aTopic, DOM_WINDOW_DESTROYED_TOPIC) == 0) {
    Close();
  }

  return NS_OK;
}





NS_IMETHODIMP
EventSource::OnStartRequest(nsIRequest *aRequest,
                            nsISupports *ctxt)
{
  nsresult rv = CheckHealthOfRequestCallback(aRequest);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aRequest, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsresult status;
  rv = aRequest->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);

  if (NS_FAILED(status)) {
    
    
    return NS_ERROR_ABORT;
  }

  uint32_t httpStatus;
  rv = httpChannel->GetResponseStatus(&httpStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  if (httpStatus != 200) {
    DispatchFailConnection();
    return NS_ERROR_ABORT;
  }

  nsAutoCString contentType;
  rv = httpChannel->GetContentType(contentType);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!contentType.EqualsLiteral(TEXT_EVENT_STREAM)) {
    DispatchFailConnection();
    return NS_ERROR_ABORT;
  }

  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(this, &EventSource::AnnounceConnection);
  NS_ENSURE_STATE(event);

  rv = NS_DispatchToMainThread(event);
  NS_ENSURE_SUCCESS(rv, rv);

  mStatus = PARSE_STATE_BEGIN_OF_STREAM;

  return NS_OK;
}



NS_METHOD
EventSource::StreamReaderFunc(nsIInputStream *aInputStream,
                              void *aClosure,
                              const char *aFromRawSegment,
                              uint32_t aToOffset,
                              uint32_t aCount,
                              uint32_t *aWriteCount)
{
  EventSource* thisObject = static_cast<EventSource*>(aClosure);
  if (!thisObject || !aWriteCount) {
    NS_WARNING("EventSource cannot read from stream: no aClosure or aWriteCount");
    return NS_ERROR_FAILURE;
  }

  *aWriteCount = 0;

  int32_t srcCount, outCount;
  char16_t out[2];
  nsresult rv;

  const char *p = aFromRawSegment,
             *end = aFromRawSegment + aCount;

  do {
    srcCount = aCount - (p - aFromRawSegment);
    outCount = 2;

    thisObject->mLastConvertionResult =
      thisObject->mUnicodeDecoder->Convert(p, &srcCount, out, &outCount);
    MOZ_ASSERT(thisObject->mLastConvertionResult != NS_ERROR_ILLEGAL_INPUT);

    for (int32_t i = 0; i < outCount; ++i) {
      rv = thisObject->ParseCharacter(out[i]);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    p = p + srcCount;
  } while (p < end &&
           thisObject->mLastConvertionResult != NS_PARTIAL_MORE_INPUT &&
           thisObject->mLastConvertionResult != NS_OK);

  *aWriteCount = aCount;
  return NS_OK;
}

NS_IMETHODIMP
EventSource::OnDataAvailable(nsIRequest *aRequest,
                             nsISupports *aContext,
                             nsIInputStream *aInputStream,
                             uint64_t aOffset,
                             uint32_t aCount)
{
  NS_ENSURE_ARG_POINTER(aInputStream);

  nsresult rv = CheckHealthOfRequestCallback(aRequest);
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t totalRead;
  return aInputStream->ReadSegments(EventSource::StreamReaderFunc, this,
                                    aCount, &totalRead);
}

NS_IMETHODIMP
EventSource::OnStopRequest(nsIRequest *aRequest,
                           nsISupports *aContext,
                           nsresult aStatusCode)
{
  mWaitingForOnStopRequest = false;

  if (mReadyState == CLOSED) {
    return NS_ERROR_ABORT;
  }

  
  
  
  
  
  
  

  if (NS_FAILED(aStatusCode) &&
      aStatusCode != NS_ERROR_CONNECTION_REFUSED &&
      aStatusCode != NS_ERROR_NET_TIMEOUT &&
      aStatusCode != NS_ERROR_NET_RESET &&
      aStatusCode != NS_ERROR_NET_INTERRUPT &&
      aStatusCode != NS_ERROR_PROXY_CONNECTION_REFUSED &&
      aStatusCode != NS_ERROR_DNS_LOOKUP_QUEUE_FULL) {
    DispatchFailConnection();
    return NS_ERROR_ABORT;
  }

  nsresult rv = CheckHealthOfRequestCallback(aRequest);
  NS_ENSURE_SUCCESS(rv, rv);

  ClearFields();

  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(this, &EventSource::ReestablishConnection);
  NS_ENSURE_STATE(event);

  rv = NS_DispatchToMainThread(event);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}





class AsyncVerifyRedirectCallbackFwr final : public nsIAsyncVerifyRedirectCallback
{
public:
  explicit AsyncVerifyRedirectCallbackFwr(EventSource* aEventsource)
    : mEventSource(aEventsource)
  {
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(AsyncVerifyRedirectCallbackFwr)

  
  NS_IMETHOD OnRedirectVerifyCallback(nsresult aResult) override
  {
    nsresult rv = mEventSource->OnRedirectVerifyCallback(aResult);
    if (NS_FAILED(rv)) {
      mEventSource->mErrorLoadOnRedirect = true;
      mEventSource->DispatchFailConnection();
    }

    return NS_OK;
  }

private:
  ~AsyncVerifyRedirectCallbackFwr() {}
  nsRefPtr<EventSource> mEventSource;
};

NS_IMPL_CYCLE_COLLECTION(AsyncVerifyRedirectCallbackFwr, mEventSource)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AsyncVerifyRedirectCallbackFwr)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIAsyncVerifyRedirectCallback)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(AsyncVerifyRedirectCallbackFwr)
NS_IMPL_CYCLE_COLLECTING_RELEASE(AsyncVerifyRedirectCallbackFwr)





NS_IMETHODIMP
EventSource::AsyncOnChannelRedirect(nsIChannel *aOldChannel,
                                    nsIChannel *aNewChannel,
                                    uint32_t    aFlags,
                                    nsIAsyncVerifyRedirectCallback *aCallback)
{
  nsCOMPtr<nsIRequest> aOldRequest = do_QueryInterface(aOldChannel);
  NS_PRECONDITION(aOldRequest, "Redirect from a null request?");

  nsresult rv = CheckHealthOfRequestCallback(aOldRequest);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_PRECONDITION(aNewChannel, "Redirect without a channel?");

  nsCOMPtr<nsIURI> newURI;
  rv = NS_GetFinalChannelURI(aNewChannel, getter_AddRefs(newURI));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!CheckCanRequestSrc(newURI)) {
    DispatchFailConnection();
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  
  mRedirectFlags = aFlags;
  mRedirectCallback = aCallback;
  mNewRedirectChannel = aNewChannel;

  if (mChannelEventSink) {
    nsRefPtr<AsyncVerifyRedirectCallbackFwr> fwd =
      new AsyncVerifyRedirectCallbackFwr(this);

    rv = mChannelEventSink->AsyncOnChannelRedirect(aOldChannel,
                                                   aNewChannel,
                                                   aFlags, fwd);
    if (NS_FAILED(rv)) {
      mRedirectCallback = nullptr;
      mNewRedirectChannel = nullptr;
      mErrorLoadOnRedirect = true;
      DispatchFailConnection();
    }
    return rv;
  }
  OnRedirectVerifyCallback(NS_OK);
  return NS_OK;
}

nsresult
EventSource::OnRedirectVerifyCallback(nsresult aResult)
{
  MOZ_ASSERT(mRedirectCallback, "mRedirectCallback not set in callback");
  MOZ_ASSERT(mNewRedirectChannel,
             "mNewRedirectChannel not set in callback");

  NS_ENSURE_SUCCESS(aResult, aResult);

  

  mHttpChannel = do_QueryInterface(mNewRedirectChannel);
  NS_ENSURE_STATE(mHttpChannel);

  nsresult rv = SetupHttpChannel();
  NS_ENSURE_SUCCESS(rv, rv);

  if ((mRedirectFlags & nsIChannelEventSink::REDIRECT_PERMANENT) != 0) {
    rv = NS_GetFinalChannelURI(mHttpChannel, getter_AddRefs(mSrc));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mNewRedirectChannel = nullptr;

  mRedirectCallback->OnRedirectVerifyCallback(aResult);
  mRedirectCallback = nullptr;

  return NS_OK;
}





NS_IMETHODIMP
EventSource::GetInterface(const nsIID & aIID,
                          void **aResult)
{
  
  
  
  
  if (aIID.Equals(NS_GET_IID(nsIChannelEventSink))) {
    mChannelEventSink = do_GetInterface(mNotificationCallbacks);
    *aResult = static_cast<nsIChannelEventSink*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  
  
  if (mNotificationCallbacks) {
    nsresult rv = mNotificationCallbacks->GetInterface(aIID, aResult);
    if (NS_SUCCEEDED(rv)) {
      NS_ASSERTION(*aResult, "Lying nsIInterfaceRequestor implementation!");
      return rv;
    }
  }

  if (aIID.Equals(NS_GET_IID(nsIAuthPrompt)) ||
      aIID.Equals(NS_GET_IID(nsIAuthPrompt2))) {
    nsresult rv = CheckInnerWindowCorrectness();
    NS_ENSURE_SUCCESS(rv, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIPromptFactory> wwatch =
      do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    
    

    nsCOMPtr<nsIDOMWindow> window;
    if (GetOwner()) {
      window = GetOwner()->GetOuterWindow();
    }

    return wwatch->GetPrompt(window, aIID, aResult);
  }

  return QueryInterface(aIID, aResult);
}


bool
EventSource::PrefEnabled(JSContext* aCx, JSObject* aGlobal)
{
  return Preferences::GetBool("dom.server-events.enabled", false);
}

nsresult
EventSource::GetBaseURI(nsIURI **aBaseURI)
{
  NS_ENSURE_ARG_POINTER(aBaseURI);

  *aBaseURI = nullptr;

  nsCOMPtr<nsIURI> baseURI;

  
  nsresult rv;
  nsIScriptContext* sc = GetContextForEventHandlers(&rv);
  nsCOMPtr<nsIDocument> doc =
    nsContentUtils::GetDocumentFromScriptContext(sc);
  if (doc) {
    baseURI = doc->GetBaseURI();
  }

  
  if (!baseURI) {
    rv = mPrincipal->GetURI(getter_AddRefs(baseURI));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ENSURE_STATE(baseURI);

  baseURI.forget(aBaseURI);
  return NS_OK;
}

net::ReferrerPolicy
EventSource::GetReferrerPolicy()
{
  nsresult rv;
  nsIScriptContext* sc = GetContextForEventHandlers(&rv);
  NS_ENSURE_SUCCESS(rv, mozilla::net::RP_Default);

  nsCOMPtr<nsIDocument> doc = nsContentUtils::GetDocumentFromScriptContext(sc);
  return doc ? doc->GetReferrerPolicy() : mozilla::net::RP_Default;
}

nsresult
EventSource::SetupHttpChannel()
{
  mHttpChannel->SetRequestMethod(NS_LITERAL_CSTRING("GET"));

  

  mHttpChannel->SetRequestHeader(NS_LITERAL_CSTRING("Accept"),
    NS_LITERAL_CSTRING(TEXT_EVENT_STREAM), false);

  

  if (!mLastEventID.IsEmpty()) {
    mHttpChannel->SetRequestHeader(NS_LITERAL_CSTRING("Last-Event-ID"),
      NS_ConvertUTF16toUTF8(mLastEventID), false);
  }

  nsCOMPtr<nsIURI> codebase;
  nsresult rv = GetBaseURI(getter_AddRefs(codebase));
  if (NS_SUCCEEDED(rv)) {
    rv = mHttpChannel->SetReferrerWithPolicy(codebase, this->GetReferrerPolicy());
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
EventSource::InitChannelAndRequestEventSource()
{
  if (mReadyState == CLOSED) {
    return NS_ERROR_ABORT;
  }

  

  if (!CheckCanRequestSrc()) {
    DispatchFailConnection();
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsLoadFlags loadFlags;
  loadFlags = nsIRequest::LOAD_BACKGROUND | nsIRequest::LOAD_BYPASS_CACHE;

  nsresult rv;
  nsIScriptContext* sc = GetContextForEventHandlers(&rv);
  nsCOMPtr<nsIDocument> doc =
    nsContentUtils::GetDocumentFromScriptContext(sc);

  nsCOMPtr<nsIChannel> channel;
  
  if (doc) {
    rv = NS_NewChannel(getter_AddRefs(channel),
                       mSrc,
                       doc,
                       nsILoadInfo::SEC_FORCE_INHERIT_PRINCIPAL,
                       nsIContentPolicy::TYPE_DATAREQUEST,
                       mLoadGroup,       
                       nullptr,          
                       loadFlags);       
  } else {
    
    rv = NS_NewChannel(getter_AddRefs(channel),
                       mSrc,
                       mPrincipal,
                       nsILoadInfo::SEC_FORCE_INHERIT_PRINCIPAL,
                       nsIContentPolicy::TYPE_DATAREQUEST,
                       mLoadGroup,       
                       nullptr,          
                       loadFlags);       
  }

  NS_ENSURE_SUCCESS(rv, rv);

  mHttpChannel = do_QueryInterface(channel);
  NS_ENSURE_TRUE(mHttpChannel, NS_ERROR_NO_INTERFACE);

  rv = SetupHttpChannel();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInterfaceRequestor> notificationCallbacks;
  mHttpChannel->GetNotificationCallbacks(getter_AddRefs(notificationCallbacks));
  if (notificationCallbacks != this) {
    mNotificationCallbacks = notificationCallbacks;
    mHttpChannel->SetNotificationCallbacks(this);
  }

  nsRefPtr<nsCORSListenerProxy> listener =
    new nsCORSListenerProxy(this, mPrincipal, mWithCredentials);
  rv = listener->Init(mHttpChannel, DataURIHandling::Allow);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mHttpChannel->AsyncOpen(listener, nullptr);
  if (NS_SUCCEEDED(rv)) {
    mWaitingForOnStopRequest = true;
  }
  return rv;
}

void
EventSource::AnnounceConnection()
{
  if (mReadyState == CLOSED) {
    return;
  }

  if (mReadyState != CONNECTING) {
    NS_WARNING("Unexpected mReadyState!!!");
    return;
  }

  
  
  

  mReadyState = OPEN;

  nsresult rv = CheckInnerWindowCorrectness();
  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<nsIDOMEvent> event;
  rv = NS_NewDOMEvent(getter_AddRefs(event), this, nullptr, nullptr);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to create the open event!!!");
    return;
  }

  
  rv = event->InitEvent(NS_LITERAL_STRING("open"), false, false);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to init the open event!!!");
    return;
  }

  event->SetTrusted(true);

  rv = DispatchDOMEvent(nullptr, event, nullptr, nullptr);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch the open event!!!");
    return;
  }
}

nsresult
EventSource::ResetConnection()
{
  if (mHttpChannel) {
    mHttpChannel->Cancel(NS_ERROR_ABORT);
  }

  if (mUnicodeDecoder) {
    mUnicodeDecoder->Reset();
  }
  mLastConvertionResult = NS_OK;

  mHttpChannel = nullptr;
  mNotificationCallbacks = nullptr;
  mChannelEventSink = nullptr;
  mStatus = PARSE_STATE_OFF;
  mRedirectCallback = nullptr;
  mNewRedirectChannel = nullptr;

  mReadyState = CONNECTING;

  return NS_OK;
}

void
EventSource::ReestablishConnection()
{
  if (mReadyState == CLOSED) {
    return;
  }

  nsresult rv = ResetConnection();
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to reset the connection!!!");
    return;
  }

  rv = CheckInnerWindowCorrectness();
  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<nsIDOMEvent> event;
  rv = NS_NewDOMEvent(getter_AddRefs(event), this, nullptr, nullptr);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to create the error event!!!");
    return;
  }

  
  rv = event->InitEvent(NS_LITERAL_STRING("error"), false, false);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to init the error event!!!");
    return;
  }

  event->SetTrusted(true);

  rv = DispatchDOMEvent(nullptr, event, nullptr, nullptr);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch the error event!!!");
    return;
  }

  rv = SetReconnectionTimeout();
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to set the timeout for reestablishing the connection!!!");
    return;
  }
}

nsresult
EventSource::SetReconnectionTimeout()
{
  if (mReadyState == CLOSED) {
    return NS_ERROR_ABORT;
  }

  
  if (!mTimer) {
    mTimer = do_CreateInstance("@mozilla.org/timer;1");
    NS_ENSURE_STATE(mTimer);
  }

  nsresult rv = mTimer->InitWithFuncCallback(TimerCallback, this,
                                             mReconnectionTime,
                                             nsITimer::TYPE_ONE_SHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
EventSource::PrintErrorOnConsole(const char *aBundleURI,
                                 const char16_t *aError,
                                 const char16_t **aFormatStrings,
                                 uint32_t aFormatStringsLen)
{
  nsCOMPtr<nsIStringBundleService> bundleService =
    mozilla::services::GetStringBundleService();
  NS_ENSURE_STATE(bundleService);

  nsCOMPtr<nsIStringBundle> strBundle;
  nsresult rv =
    bundleService->CreateBundle(aBundleURI, getter_AddRefs(strBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIConsoleService> console(
    do_GetService(NS_CONSOLESERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIScriptError> errObj(
    do_CreateInstance(NS_SCRIPTERROR_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsXPIDLString message;
  if (aFormatStrings) {
    rv = strBundle->FormatStringFromName(aError, aFormatStrings,
                                         aFormatStringsLen,
                                         getter_Copies(message));
  } else {
    rv = strBundle->GetStringFromName(aError, getter_Copies(message));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  rv = errObj->InitWithWindowID(message,
                                mScriptFile,
                                EmptyString(),
                                mScriptLine, 0,
                                nsIScriptError::errorFlag,
                                "Event Source", mInnerWindowID);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = console->LogMessage(errObj);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
EventSource::ConsoleError()
{
  nsAutoCString targetSpec;
  nsresult rv = mSrc->GetSpec(targetSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ConvertUTF8toUTF16 specUTF16(targetSpec);
  const char16_t *formatStrings[] = { specUTF16.get() };

  if (mReadyState == CONNECTING) {
    rv = PrintErrorOnConsole("chrome://global/locale/appstrings.properties",
                             MOZ_UTF16("connectionFailure"),
                             formatStrings, ArrayLength(formatStrings));
  } else {
    rv = PrintErrorOnConsole("chrome://global/locale/appstrings.properties",
                             MOZ_UTF16("netInterrupt"),
                             formatStrings, ArrayLength(formatStrings));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
EventSource::DispatchFailConnection()
{
  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(this, &EventSource::FailConnection);
  NS_ENSURE_STATE(event);

  return NS_DispatchToMainThread(event);
}

void
EventSource::FailConnection()
{
  if (mReadyState == CLOSED) {
    return;
  }

  nsresult rv = ConsoleError();
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to print to the console error");
  }

  
  
  

  Close(); 

  rv = CheckInnerWindowCorrectness();
  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<nsIDOMEvent> event;
  rv = NS_NewDOMEvent(getter_AddRefs(event), this, nullptr, nullptr);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to create the error event!!!");
    return;
  }

  
  rv = event->InitEvent(NS_LITERAL_STRING("error"), false, false);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to init the error event!!!");
    return;
  }

  event->SetTrusted(true);

  rv = DispatchDOMEvent(nullptr, event, nullptr, nullptr);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch the error event!!!");
    return;
  }
}

bool
EventSource::CheckCanRequestSrc(nsIURI* aSrc)
{
  if (mReadyState == CLOSED) {
    return false;
  }

  bool isValidURI = false;
  bool isValidContentLoadPolicy = false;
  bool isValidProtocol = false;

  nsCOMPtr<nsIURI> srcToTest = aSrc ? aSrc : mSrc.get();
  NS_ENSURE_TRUE(srcToTest, false);

  uint32_t aCheckURIFlags =
    nsIScriptSecurityManager::DISALLOW_INHERIT_PRINCIPAL |
    nsIScriptSecurityManager::DISALLOW_SCRIPT;

  nsresult rv = nsContentUtils::GetSecurityManager()->
    CheckLoadURIWithPrincipal(mPrincipal,
                              srcToTest,
                              aCheckURIFlags);
  isValidURI = NS_SUCCEEDED(rv);

  

  nsIScriptContext* sc = GetContextForEventHandlers(&rv);
  nsCOMPtr<nsIDocument> doc =
    nsContentUtils::GetDocumentFromScriptContext(sc);

  
  
  rv = CheckInnerWindowCorrectness();
  NS_ENSURE_SUCCESS(rv, false);
  int16_t shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_DATAREQUEST,
                                 srcToTest,
                                 mPrincipal,
                                 doc,
                                 NS_LITERAL_CSTRING(TEXT_EVENT_STREAM),
                                 nullptr,    
                                 &shouldLoad,
                                 nsContentUtils::GetContentPolicy(),
                                 nsContentUtils::GetSecurityManager());
  isValidContentLoadPolicy = NS_SUCCEEDED(rv) && NS_CP_ACCEPTED(shouldLoad);

  nsAutoCString targetURIScheme;
  rv = srcToTest->GetScheme(targetURIScheme);
  if (NS_SUCCEEDED(rv)) {
    
    isValidProtocol = targetURIScheme.EqualsLiteral("http") ||
                      targetURIScheme.EqualsLiteral("https");
  }

  return isValidURI && isValidContentLoadPolicy && isValidProtocol;
}


void
EventSource::TimerCallback(nsITimer* aTimer, void* aClosure)
{
  nsRefPtr<EventSource> thisObject = static_cast<EventSource*>(aClosure);

  if (thisObject->mReadyState == CLOSED) {
    return;
  }

  NS_PRECONDITION(!thisObject->mHttpChannel,
                  "the channel hasn't been cancelled!!");

  if (!thisObject->mFrozen) {
    nsresult rv = thisObject->InitChannelAndRequestEventSource();
    if (NS_FAILED(rv)) {
      NS_WARNING("thisObject->InitChannelAndRequestEventSource() failed");
      return;
    }
  }
}

nsresult
EventSource::Thaw()
{
  if (mReadyState == CLOSED || !mFrozen) {
    return NS_OK;
  }

  NS_ASSERTION(!mHttpChannel, "the connection hasn't been closed!!!");

  mFrozen = false;
  nsresult rv;
  if (!mGoingToDispatchAllMessages && mMessagesToDispatch.GetSize() > 0) {
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &EventSource::DispatchAllMessageEvents);
    NS_ENSURE_STATE(event);

    mGoingToDispatchAllMessages = true;

    rv = NS_DispatchToMainThread(event);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = InitChannelAndRequestEventSource();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
EventSource::Freeze()
{
  if (mReadyState == CLOSED || mFrozen) {
    return NS_OK;
  }

  NS_ASSERTION(!mHttpChannel, "the connection hasn't been closed!!!");
  mFrozen = true;
  return NS_OK;
}

nsresult
EventSource::DispatchCurrentMessageEvent()
{
  nsAutoPtr<Message> message(new Message());
  *message = mCurrentMessage;

  ClearFields();

  if (message->mData.IsEmpty()) {
    return NS_OK;
  }

  
  NS_ASSERTION(message->mData.CharAt(message->mData.Length() - 1) == LF_CHAR,
               "Invalid trailing character! LF was expected instead.");
  message->mData.SetLength(message->mData.Length() - 1);

  if (message->mEventName.IsEmpty()) {
    message->mEventName.AssignLiteral("message");
  }

  if (message->mLastEventID.IsEmpty() && !mLastEventID.IsEmpty()) {
    message->mLastEventID.Assign(mLastEventID);
  }

  int32_t sizeBefore = mMessagesToDispatch.GetSize();
  mMessagesToDispatch.Push(message.forget());
  NS_ENSURE_TRUE(mMessagesToDispatch.GetSize() == sizeBefore + 1,
                 NS_ERROR_OUT_OF_MEMORY);


  if (!mGoingToDispatchAllMessages) {
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &EventSource::DispatchAllMessageEvents);
    NS_ENSURE_STATE(event);

    mGoingToDispatchAllMessages = true;

    return NS_DispatchToMainThread(event);
  }

  return NS_OK;
}

void
EventSource::DispatchAllMessageEvents()
{
  if (mReadyState == CLOSED || mFrozen) {
    return;
  }

  mGoingToDispatchAllMessages = false;

  nsresult rv = CheckInnerWindowCorrectness();
  if (NS_FAILED(rv)) {
    return;
  }

  AutoJSAPI jsapi;
  if (NS_WARN_IF(!jsapi.Init(GetOwner()))) {
    return;
  }
  JSContext* cx = jsapi.cx();

  while (mMessagesToDispatch.GetSize() > 0) {
    nsAutoPtr<Message>
      message(static_cast<Message*>(mMessagesToDispatch.PopFront()));

    
    JS::Rooted<JS::Value> jsData(cx);
    {
      JSString* jsString;
      jsString = JS_NewUCStringCopyN(cx,
                                     message->mData.get(),
                                     message->mData.Length());
      NS_ENSURE_TRUE_VOID(jsString);

      jsData = STRING_TO_JSVAL(jsString);
    }

    
    

    nsCOMPtr<nsIDOMEvent> event;
    rv = NS_NewDOMMessageEvent(getter_AddRefs(event), this, nullptr, nullptr);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create the message event!!!");
      return;
    }

    nsCOMPtr<nsIDOMMessageEvent> messageEvent = do_QueryInterface(event);
    rv = messageEvent->InitMessageEvent(message->mEventName,
                                        false, false,
                                        jsData,
                                        mOrigin,
                                        message->mLastEventID, nullptr);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to init the message event!!!");
      return;
    }

    messageEvent->SetTrusted(true);

    rv = DispatchDOMEvent(nullptr, event, nullptr, nullptr);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to dispatch the message event!!!");
      return;
    }

    mLastEventID.Assign(message->mLastEventID);
  }
}

nsresult
EventSource::ClearFields()
{
  

  mCurrentMessage.mEventName.Truncate();
  mCurrentMessage.mLastEventID.Truncate();
  mCurrentMessage.mData.Truncate();

  mLastFieldName.Truncate();
  mLastFieldValue.Truncate();

  return NS_OK;
}

nsresult
EventSource::SetFieldAndClear()
{
  if (mLastFieldName.IsEmpty()) {
    mLastFieldValue.Truncate();
    return NS_OK;
  }

  char16_t first_char;
  first_char = mLastFieldName.CharAt(0);

  switch (first_char)  
  {
    case char16_t('d'):
      if (mLastFieldName.EqualsLiteral("data")) {
        
        
        
        mCurrentMessage.mData.Append(mLastFieldValue);
        mCurrentMessage.mData.Append(LF_CHAR);
      }
      break;

    case char16_t('e'):
      if (mLastFieldName.EqualsLiteral("event")) {
        mCurrentMessage.mEventName.Assign(mLastFieldValue);
      }
      break;

    case char16_t('i'):
      if (mLastFieldName.EqualsLiteral("id")) {
        mCurrentMessage.mLastEventID.Assign(mLastFieldValue);
      }
      break;

    case char16_t('r'):
      if (mLastFieldName.EqualsLiteral("retry")) {
        uint32_t newValue=0;
        uint32_t i = 0;  
        bool assign = true;
        for (i = 0; i < mLastFieldValue.Length(); ++i) {
          if (mLastFieldValue.CharAt(i) < (char16_t)'0' ||
              mLastFieldValue.CharAt(i) > (char16_t)'9') {
            assign = false;
            break;
          }
          newValue = newValue*10 +
                     (((uint32_t)mLastFieldValue.CharAt(i))-
                       ((uint32_t)((char16_t)'0')));
        }

        if (assign) {
          if (newValue < MIN_RECONNECTION_TIME_VALUE) {
            mReconnectionTime = MIN_RECONNECTION_TIME_VALUE;
          } else if (newValue > MAX_RECONNECTION_TIME_VALUE) {
            mReconnectionTime = MAX_RECONNECTION_TIME_VALUE;
          } else {
            mReconnectionTime = newValue;
          }
        }
        break;
      }
      break;
  }

  mLastFieldName.Truncate();
  mLastFieldValue.Truncate();

  return NS_OK;
}

nsresult
EventSource::CheckHealthOfRequestCallback(nsIRequest *aRequestCallback)
{
  
  
  if (mReadyState == CLOSED || !mHttpChannel ||
      mFrozen || mErrorLoadOnRedirect) {
    return NS_ERROR_ABORT;
  }

  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aRequestCallback);
  NS_ENSURE_STATE(httpChannel);

  if (httpChannel != mHttpChannel) {
    NS_WARNING("wrong channel from request callback");
    return NS_ERROR_ABORT;
  }

  return NS_OK;
}

nsresult
EventSource::ParseCharacter(char16_t aChr)
{
  nsresult rv;

  if (mReadyState == CLOSED) {
    return NS_ERROR_ABORT;
  }

  switch (mStatus)
  {
    case PARSE_STATE_OFF:
      NS_ERROR("Invalid state");
      return NS_ERROR_FAILURE;
      break;

    case PARSE_STATE_BEGIN_OF_STREAM:
      if (aChr == BOM_CHAR) {
        mStatus = PARSE_STATE_BOM_WAS_READ;  
      } else if (aChr == CR_CHAR) {
        mStatus = PARSE_STATE_CR_CHAR;
      } else if (aChr == LF_CHAR) {
        mStatus = PARSE_STATE_BEGIN_OF_LINE;
      } else if (aChr == COLON_CHAR) {
        mStatus = PARSE_STATE_COMMENT;
      } else {
        mLastFieldName += aChr;
        mStatus = PARSE_STATE_FIELD_NAME;
      }

      break;

    case PARSE_STATE_BOM_WAS_READ:
      if (aChr == CR_CHAR) {
        mStatus = PARSE_STATE_CR_CHAR;
      } else if (aChr == LF_CHAR) {
        mStatus = PARSE_STATE_BEGIN_OF_LINE;
      } else if (aChr == COLON_CHAR) {
        mStatus = PARSE_STATE_COMMENT;
      } else {
        mLastFieldName += aChr;
        mStatus = PARSE_STATE_FIELD_NAME;
      }
      break;

    case PARSE_STATE_CR_CHAR:
      if (aChr == CR_CHAR) {
        rv = DispatchCurrentMessageEvent();  
        NS_ENSURE_SUCCESS(rv, rv);
      } else if (aChr == LF_CHAR) {
        mStatus = PARSE_STATE_BEGIN_OF_LINE;
      } else if (aChr == COLON_CHAR) {
        mStatus = PARSE_STATE_COMMENT;
      } else {
        mLastFieldName += aChr;
        mStatus = PARSE_STATE_FIELD_NAME;
      }

      break;

    case PARSE_STATE_COMMENT:
      if (aChr == CR_CHAR) {
        mStatus = PARSE_STATE_CR_CHAR;
      } else if (aChr == LF_CHAR) {
        mStatus = PARSE_STATE_BEGIN_OF_LINE;
      }

      break;

    case PARSE_STATE_FIELD_NAME:
      if (aChr == CR_CHAR) {
        rv = SetFieldAndClear();
        NS_ENSURE_SUCCESS(rv, rv);

        mStatus = PARSE_STATE_CR_CHAR;
      } else if (aChr == LF_CHAR) {
        rv = SetFieldAndClear();
        NS_ENSURE_SUCCESS(rv, rv);

        mStatus = PARSE_STATE_BEGIN_OF_LINE;
      } else if (aChr == COLON_CHAR) {
        mStatus = PARSE_STATE_FIRST_CHAR_OF_FIELD_VALUE;
      } else {
        mLastFieldName += aChr;
      }

      break;

    case PARSE_STATE_FIRST_CHAR_OF_FIELD_VALUE:
      if (aChr == CR_CHAR) {
        rv = SetFieldAndClear();
        NS_ENSURE_SUCCESS(rv, rv);

        mStatus = PARSE_STATE_CR_CHAR;
      } else if (aChr == LF_CHAR) {
        rv = SetFieldAndClear();
        NS_ENSURE_SUCCESS(rv, rv);

        mStatus = PARSE_STATE_BEGIN_OF_LINE;
      } else if (aChr == SPACE_CHAR) {
        mStatus = PARSE_STATE_FIELD_VALUE;
      } else {
        mLastFieldValue += aChr;
        mStatus = PARSE_STATE_FIELD_VALUE;
      }

      break;

    case PARSE_STATE_FIELD_VALUE:
      if (aChr == CR_CHAR) {
        rv = SetFieldAndClear();
        NS_ENSURE_SUCCESS(rv, rv);

        mStatus = PARSE_STATE_CR_CHAR;
      } else if (aChr == LF_CHAR) {
        rv = SetFieldAndClear();
        NS_ENSURE_SUCCESS(rv, rv);

        mStatus = PARSE_STATE_BEGIN_OF_LINE;
      } else {
        mLastFieldValue += aChr;
      }

      break;

    case PARSE_STATE_BEGIN_OF_LINE:
      if (aChr == CR_CHAR) {
        rv = DispatchCurrentMessageEvent();  
        NS_ENSURE_SUCCESS(rv, rv);

        mStatus = PARSE_STATE_CR_CHAR;
      } else if (aChr == LF_CHAR) {
        rv = DispatchCurrentMessageEvent();  
        NS_ENSURE_SUCCESS(rv, rv);

        mStatus = PARSE_STATE_BEGIN_OF_LINE;
      } else if (aChr == COLON_CHAR) {
        mStatus = PARSE_STATE_COMMENT;
      } else {
        mLastFieldName += aChr;
        mStatus = PARSE_STATE_FIELD_NAME;
      }

      break;
  }

  return NS_OK;
}

} 
} 
