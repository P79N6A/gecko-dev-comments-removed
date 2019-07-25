





































#include "mozilla/Util.h"

#include "nsIDOMHTMLMediaElement.h"
#include "nsIDOMHTMLSourceElement.h"
#include "nsHTMLMediaElement.h"
#include "nsTimeRanges.h"
#include "nsGenericHTMLElement.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsGkAtoms.h"
#include "nsSize.h"
#include "nsIFrame.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsDOMError.h"
#include "nsNodeInfoManager.h"
#include "nsNetUtil.h"
#include "nsXPCOMStrings.h"
#include "nsThreadUtils.h"
#include "nsIThreadInternal.h"
#include "nsContentUtils.h"
#include "nsFrameManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsIXPConnect.h"
#include "jsapi.h"

#include "nsITimer.h"

#include "nsEventDispatcher.h"
#include "nsMediaError.h"
#include "nsICategoryManager.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsMediaStream.h"

#include "nsIDOMHTMLVideoElement.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsContentErrors.h"
#include "nsCrossSiteListenerProxy.h"
#include "nsCycleCollectionParticipant.h"
#include "nsICachingChannel.h"
#include "nsLayoutUtils.h"
#include "nsVideoFrame.h"
#include "BasicLayers.h"
#include <limits>
#include "nsIDocShellTreeItem.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsIAppShell.h"
#include "nsWidgetsCID.h"

#include "nsIPrivateDOMEvent.h"
#include "nsIDOMNotifyAudioAvailableEvent.h"
#include "nsMediaFragmentURIParser.h"

#ifdef MOZ_OGG
#include "nsOggDecoder.h"
#endif
#ifdef MOZ_WAVE
#include "nsWaveDecoder.h"
#endif
#ifdef MOZ_WEBM
#include "nsWebMDecoder.h"
#endif
#ifdef MOZ_RAW
#include "nsRawDecoder.h"
#endif

#ifdef PR_LOGGING
static PRLogModuleInfo* gMediaElementLog;
static PRLogModuleInfo* gMediaElementEventsLog;
#define LOG(type, msg) PR_LOG(gMediaElementLog, type, msg)
#define LOG_EVENT(type, msg) PR_LOG(gMediaElementEventsLog, type, msg)
#else
#define LOG(type, msg)
#define LOG_EVENT(type, msg)
#endif

#include "nsIContentSecurityPolicy.h"
#include "nsIChannelPolicy.h"
#include "nsChannelPolicy.h"

#include "mozilla/Preferences.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::layers;


#define TIMEUPDATE_MS 250











































class nsMediaEvent : public nsRunnable
{
public:

  nsMediaEvent(nsHTMLMediaElement* aElement) :
    mElement(aElement),
    mLoadID(mElement->GetCurrentLoadID()) {}
  ~nsMediaEvent() {}

  NS_IMETHOD Run() = 0;

protected:
  bool IsCancelled() {
    return mElement->GetCurrentLoadID() != mLoadID;
  }

  nsRefPtr<nsHTMLMediaElement> mElement;
  PRUint32 mLoadID;
};

class nsAsyncEventRunner : public nsMediaEvent
{
private:
  nsString mName;

public:
  nsAsyncEventRunner(const nsAString& aName, nsHTMLMediaElement* aElement) :
    nsMediaEvent(aElement), mName(aName)
  {
  }

  NS_IMETHOD Run()
  {
    
    if (IsCancelled())
      return NS_OK;

    return mElement->DispatchEvent(mName);
  }
};

class nsSourceErrorEventRunner : public nsMediaEvent
{
private:
  nsCOMPtr<nsIContent> mSource;
public:
  nsSourceErrorEventRunner(nsHTMLMediaElement* aElement,
                           nsIContent* aSource)
    : nsMediaEvent(aElement),
      mSource(aSource)
  {
  }

  NS_IMETHOD Run() {
    
    if (IsCancelled())
      return NS_OK;
    LOG_EVENT(PR_LOG_DEBUG, ("%p Dispatching simple event source error", mElement.get()));
    return nsContentUtils::DispatchTrustedEvent(mElement->OwnerDoc(),
                                                mSource,
                                                NS_LITERAL_STRING("error"),
                                                false,
                                                true);
  }
};







class nsHTMLMediaElement::MediaLoadListener : public nsIStreamListener,
                                              public nsIChannelEventSink,
                                              public nsIInterfaceRequestor,
                                              public nsIObserver
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIINTERFACEREQUESTOR

public:
  MediaLoadListener(nsHTMLMediaElement* aElement)
    : mElement(aElement),
      mLoadID(aElement->GetCurrentLoadID())
  {
    NS_ABORT_IF_FALSE(mElement, "Must pass an element to call back");
  }

private:
  nsRefPtr<nsHTMLMediaElement> mElement;
  nsCOMPtr<nsIStreamListener> mNextListener;
  PRUint32 mLoadID;
};

NS_IMPL_ISUPPORTS5(nsHTMLMediaElement::MediaLoadListener, nsIRequestObserver,
                   nsIStreamListener, nsIChannelEventSink,
                   nsIInterfaceRequestor, nsIObserver)

NS_IMETHODIMP
nsHTMLMediaElement::MediaLoadListener::Observe(nsISupports* aSubject,
                                               const char* aTopic, const PRUnichar* aData)
{
  nsContentUtils::UnregisterShutdownObserver(this);

  
  mElement = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsHTMLMediaElement::MediaLoadListener::OnStartRequest(nsIRequest* aRequest, nsISupports* aContext)
{
  nsContentUtils::UnregisterShutdownObserver(this);

  
  
  nsRefPtr<nsHTMLMediaElement> element;
  element.swap(mElement);

  if (mLoadID != element->GetCurrentLoadID()) {
    
    
    
    return NS_BINDING_ABORTED;
  }

  
  nsresult status;
  nsresult rv = aRequest->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);
  if (NS_FAILED(status)) {
    if (element)
      element->NotifyLoadError();
    return status;
  }

  nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
  if (channel &&
      element &&
      NS_SUCCEEDED(rv = element->InitializeDecoderForChannel(channel, getter_AddRefs(mNextListener))) &&
      mNextListener) {
    rv = mNextListener->OnStartRequest(aRequest, aContext);
  } else {
    
    
    if (NS_FAILED(rv) && !mNextListener && element) {
      
      
      element->NotifyLoadError();
    }
    
    
    
    rv = NS_BINDING_ABORTED;
  }

  return rv;
}

NS_IMETHODIMP nsHTMLMediaElement::MediaLoadListener::OnStopRequest(nsIRequest* aRequest, nsISupports* aContext,
                                                                   nsresult aStatus)
{
  if (mNextListener) {
    return mNextListener->OnStopRequest(aRequest, aContext, aStatus);
  }
  return NS_OK;
}

NS_IMETHODIMP nsHTMLMediaElement::MediaLoadListener::OnDataAvailable(nsIRequest* aRequest, nsISupports* aContext,
                                                                     nsIInputStream* aStream, PRUint32 aOffset,
                                                                     PRUint32 aCount)
{
  if (!mNextListener) {
    NS_ERROR("Must have a chained listener; OnStartRequest should have canceled this request");
    return NS_BINDING_ABORTED;
  }
  return mNextListener->OnDataAvailable(aRequest, aContext, aStream, aOffset, aCount);
}

NS_IMETHODIMP nsHTMLMediaElement::MediaLoadListener::AsyncOnChannelRedirect(nsIChannel* aOldChannel,
                                                                            nsIChannel* aNewChannel,
                                                                            PRUint32 aFlags,
                                                                            nsIAsyncVerifyRedirectCallback* cb)
{
  
  if (mElement)
    mElement->OnChannelRedirect(aOldChannel, aNewChannel, aFlags);
  nsCOMPtr<nsIChannelEventSink> sink = do_QueryInterface(mNextListener);
  if (sink)
    return sink->AsyncOnChannelRedirect(aOldChannel, aNewChannel, aFlags, cb);

  cb->OnRedirectVerifyCallback(NS_OK);
  return NS_OK;
}

NS_IMETHODIMP nsHTMLMediaElement::MediaLoadListener::GetInterface(const nsIID & aIID, void **aResult)
{
  return QueryInterface(aIID, aResult);
}

NS_IMPL_ADDREF_INHERITED(nsHTMLMediaElement, nsGenericHTMLElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLMediaElement, nsGenericHTMLElement)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLMediaElement)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLMediaElement, nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mSourcePointer)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mLoadBlockedDoc)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mSourceLoadCandidate)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsHTMLMediaElement, nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mSourcePointer)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mLoadBlockedDoc)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mSourceLoadCandidate)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsHTMLMediaElement)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
NS_INTERFACE_MAP_END_INHERITING(nsGenericHTMLElement)


NS_IMPL_URI_ATTR(nsHTMLMediaElement, Src, src)
NS_IMPL_BOOL_ATTR(nsHTMLMediaElement, Controls, controls)
NS_IMPL_BOOL_ATTR(nsHTMLMediaElement, Autoplay, autoplay)
NS_IMPL_BOOL_ATTR(nsHTMLMediaElement, Loop, loop)
NS_IMPL_ENUM_ATTR_DEFAULT_VALUE(nsHTMLMediaElement, Preload, preload, NULL)


NS_IMETHODIMP nsHTMLMediaElement::GetMozAutoplayEnabled(bool *aAutoplayEnabled)
{
  *aAutoplayEnabled = mAutoplayEnabled;

  return NS_OK;
}


NS_IMETHODIMP nsHTMLMediaElement::GetError(nsIDOMMediaError * *aError)
{
  NS_IF_ADDREF(*aError = mError);

  return NS_OK;
}


NS_IMETHODIMP nsHTMLMediaElement::GetEnded(bool *aEnded)
{
  *aEnded = mDecoder ? mDecoder->IsEnded() : false;

  return NS_OK;
}


NS_IMETHODIMP nsHTMLMediaElement::GetCurrentSrc(nsAString & aCurrentSrc)
{
  nsCAutoString src;
  GetCurrentSpec(src);
  aCurrentSrc = NS_ConvertUTF8toUTF16(src);
  return NS_OK;
}


NS_IMETHODIMP nsHTMLMediaElement::GetNetworkState(PRUint16 *aNetworkState)
{
  *aNetworkState = mNetworkState;

  return NS_OK;
}

nsresult
nsHTMLMediaElement::OnChannelRedirect(nsIChannel *aChannel,
                                      nsIChannel *aNewChannel,
                                      PRUint32 aFlags)
{
  NS_ASSERTION(aChannel == mChannel, "Channels should match!");
  mChannel = aNewChannel;

  
  
  nsCOMPtr<nsIHttpChannel> http = do_QueryInterface(aChannel);
  NS_ENSURE_STATE(http);

  NS_NAMED_LITERAL_CSTRING(rangeHdr, "Range");
 
  nsCAutoString rangeVal;
  if (NS_SUCCEEDED(http->GetRequestHeader(rangeHdr, rangeVal))) {
    NS_ENSURE_STATE(!rangeVal.IsEmpty());

    http = do_QueryInterface(aNewChannel);
    NS_ENSURE_STATE(http);
 
    nsresult rv = http->SetRequestHeader(rangeHdr, rangeVal, false);
    NS_ENSURE_SUCCESS(rv, rv);
  }
 
  return NS_OK;
}

void nsHTMLMediaElement::AbortExistingLoads()
{
  
  mLoadWaitStatus = NOT_WAITING;

  
  
  mCurrentLoadID++;

  bool fireTimeUpdate = false;
  if (mDecoder) {
    fireTimeUpdate = mDecoder->GetCurrentTime() != 0.0;
    mDecoder->Shutdown();
    mDecoder = nsnull;
  }

  if (mNetworkState == nsIDOMHTMLMediaElement::NETWORK_LOADING ||
      mNetworkState == nsIDOMHTMLMediaElement::NETWORK_IDLE)
  {
    DispatchEvent(NS_LITERAL_STRING("abort"));
  }

  mError = nsnull;
  mLoadedFirstFrame = false;
  mAutoplaying = true;
  mIsLoadingFromSourceChildren = false;
  mSuspendedAfterFirstFrame = false;
  mAllowSuspendAfterFirstFrame = true;
  mSourcePointer = nsnull;

  

  if (mNetworkState != nsIDOMHTMLMediaElement::NETWORK_EMPTY) {
    mNetworkState = nsIDOMHTMLMediaElement::NETWORK_EMPTY;
    ChangeReadyState(nsIDOMHTMLMediaElement::HAVE_NOTHING);
    mPaused = true;

    if (fireTimeUpdate) {
      
      
      
      
      FireTimeUpdate(false);
    }
    DispatchEvent(NS_LITERAL_STRING("emptied"));
  }

  
  
  AddRemoveSelfReference();

  mIsRunningSelectResource = false;
}

void nsHTMLMediaElement::NoSupportedMediaSourceError()
{
  NS_ASSERTION(mDelayingLoadEvent, "Load event not delayed during source selection?");

  mError = new nsMediaError(nsIDOMMediaError::MEDIA_ERR_SRC_NOT_SUPPORTED);
  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_NO_SOURCE;
  DispatchAsyncEvent(NS_LITERAL_STRING("error"));
  
  ChangeDelayLoadStatus(false);
}

typedef void (nsHTMLMediaElement::*SyncSectionFn)();




class nsSyncSection : public nsMediaEvent
{
private:
  SyncSectionFn mClosure;
public:
  nsSyncSection(nsHTMLMediaElement* aElement,
                SyncSectionFn aClosure) :
    nsMediaEvent(aElement),
    mClosure(aClosure)
  {
  }

  NS_IMETHOD Run() {
    
    if (IsCancelled())
      return NS_OK;
    (mElement.get()->*mClosure)();
    return NS_OK;
  }
};

static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);




void AsyncAwaitStableState(nsHTMLMediaElement* aElement,
                           SyncSectionFn aClosure)
{
  nsCOMPtr<nsIRunnable> event = new nsSyncSection(aElement, aClosure);
  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  appShell->RunInStableState(event);
}

void nsHTMLMediaElement::QueueLoadFromSourceTask()
{
  ChangeDelayLoadStatus(true);
  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_LOADING;
  AsyncAwaitStableState(this, &nsHTMLMediaElement::LoadFromSourceChildren);
}

void nsHTMLMediaElement::QueueSelectResourceTask()
{
  
  if (mIsRunningSelectResource)
    return;
  mIsRunningSelectResource = true;
  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_NO_SOURCE;
  AsyncAwaitStableState(this, &nsHTMLMediaElement::SelectResource);
}


NS_IMETHODIMP nsHTMLMediaElement::Load()
{
  if (mIsRunningLoadMethod)
    return NS_OK;
  SetPlayedOrSeeked(false);
  mIsRunningLoadMethod = true;
  AbortExistingLoads();
  QueueSelectResourceTask();
  mIsRunningLoadMethod = false;
  return NS_OK;
}

static bool HasSourceChildren(nsIContent *aElement)
{
  for (nsIContent* child = aElement->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->IsHTML(nsGkAtoms::source))
    {
      return true;
    }
  }
  return false;
}

void nsHTMLMediaElement::SelectResource()
{
  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::src) && !HasSourceChildren(this)) {
    
    
    mNetworkState = nsIDOMHTMLMediaElement::NETWORK_EMPTY;
    
    ChangeDelayLoadStatus(false);
    mIsRunningSelectResource = false;
    return;
  }

  ChangeDelayLoadStatus(true);

  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_LOADING;
  
  
  DispatchAsyncEvent(NS_LITERAL_STRING("loadstart"));

  
  nsAutoString src;
  if (GetAttr(kNameSpaceID_None, nsGkAtoms::src, src)) {
    nsCOMPtr<nsIURI> uri;
    nsresult rv = NewURIFromString(src, getter_AddRefs(uri));
    if (NS_SUCCEEDED(rv)) {
      LOG(PR_LOG_DEBUG, ("%p Trying load from src=%s", this, NS_ConvertUTF16toUTF8(src).get()));
      NS_ASSERTION(!mIsLoadingFromSourceChildren,
        "Should think we're not loading from source children by default");
      mLoadingSrc = uri;
      if (mPreloadAction == nsHTMLMediaElement::PRELOAD_NONE) {
        
        
        SuspendLoad(uri);
        mIsRunningSelectResource = false;
        return;
      }

      rv = LoadResource(uri);
      if (NS_SUCCEEDED(rv)) {
        mIsRunningSelectResource = false;
        return;
      }
    }
    NoSupportedMediaSourceError();
  } else {
    
    mIsLoadingFromSourceChildren = true;
    LoadFromSourceChildren();
  }
  mIsRunningSelectResource = false;
}

void nsHTMLMediaElement::NotifyLoadError()
{
  if (!mIsLoadingFromSourceChildren) {
    LOG(PR_LOG_DEBUG, ("NotifyLoadError(), no supported media error"));
    NoSupportedMediaSourceError();
  } else if (mSourceLoadCandidate) {
    DispatchAsyncSourceError(mSourceLoadCandidate);
    QueueLoadFromSourceTask();
  } else {
    NS_WARNING("Should know the source we were loading from!");
  }
}

void nsHTMLMediaElement::NotifyAudioAvailable(float* aFrameBuffer,
                                              PRUint32 aFrameBufferLength,
                                              float aTime)
{
  
  
  
  
  nsAutoArrayPtr<float> frameBuffer(aFrameBuffer);
  
  if (!mMediaSecurityVerified) {
    nsCOMPtr<nsIPrincipal> principal = GetCurrentPrincipal();
    nsresult rv = NodePrincipal()->Subsumes(principal, &mAllowAudioData);
    if (NS_FAILED(rv)) {
      mAllowAudioData = false;
    }
  }

  DispatchAudioAvailableEvent(frameBuffer.forget(), aFrameBufferLength, aTime);
}

bool nsHTMLMediaElement::MayHaveAudioAvailableEventListener()
{
  
  
  
  nsIDocument *document = GetDocument();
  if (!document) {
    return true;
  }

  nsPIDOMWindow *window = document->GetInnerWindow();
  if (!window) {
    return true;
  }

  return window->HasAudioAvailableEventListeners();
}

void nsHTMLMediaElement::LoadFromSourceChildren()
{
  NS_ASSERTION(mDelayingLoadEvent,
               "Should delay load event (if in document) during load");
  NS_ASSERTION(mIsLoadingFromSourceChildren,
               "Must remember we're loading from source children");
  while (true) {
    nsIContent* child = GetNextSource();
    if (!child) {
      
      
      mLoadWaitStatus = WAITING_FOR_SOURCE;
      mNetworkState = nsIDOMHTMLMediaElement::NETWORK_NO_SOURCE;
      ChangeDelayLoadStatus(false);
      return;
    }

    
    nsAutoString src;
    if (!child->GetAttr(kNameSpaceID_None, nsGkAtoms::src, src)) {
      DispatchAsyncSourceError(child);
      continue;
    }

    
    nsAutoString type;
    if (child->GetAttr(kNameSpaceID_None, nsGkAtoms::type, type) &&
        GetCanPlay(type) == CANPLAY_NO) {
      DispatchAsyncSourceError(child);
      continue;
    }
    LOG(PR_LOG_DEBUG, ("%p Trying load from <source>=%s type=%s", this,
      NS_ConvertUTF16toUTF8(src).get(), NS_ConvertUTF16toUTF8(type).get()));

    nsCOMPtr<nsIURI> uri;
    NewURIFromString(src, getter_AddRefs(uri));
    if (!uri) {
      DispatchAsyncSourceError(child);
      continue;
    }

    mLoadingSrc = uri;
    NS_ASSERTION(mNetworkState == nsIDOMHTMLMediaElement::NETWORK_LOADING,
                 "Network state should be loading");

    if (mPreloadAction == nsHTMLMediaElement::PRELOAD_NONE) {
      
      
      SuspendLoad(uri);
      return;
    }

    if (NS_SUCCEEDED(LoadResource(uri))) {
      return;
    }

    
    DispatchAsyncSourceError(child);
  }
  NS_NOTREACHED("Execution should not reach here!");
}

void nsHTMLMediaElement::SuspendLoad(nsIURI* aURI)
{
  mLoadIsSuspended = true;
  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_IDLE;
  DispatchAsyncEvent(NS_LITERAL_STRING("suspend"));
  ChangeDelayLoadStatus(false);
}

void nsHTMLMediaElement::ResumeLoad(PreloadAction aAction)
{
  NS_ASSERTION(mLoadIsSuspended, "Can only resume preload if halted for one");
  nsCOMPtr<nsIURI> uri = mLoadingSrc;
  mLoadIsSuspended = false;
  mPreloadAction = aAction;
  ChangeDelayLoadStatus(true);
  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_LOADING;
  if (!mIsLoadingFromSourceChildren) {
    
    if (NS_FAILED(LoadResource(uri))) {
      NoSupportedMediaSourceError();
    }
  } else {
    
    
    if (NS_FAILED(LoadResource(uri))) {
      LoadFromSourceChildren();
    }
  }
}

static bool IsAutoplayEnabled()
{
  return Preferences::GetBool("media.autoplay.enabled");
}

void nsHTMLMediaElement::UpdatePreloadAction()
{
  PreloadAction nextAction = PRELOAD_UNDEFINED;
  
  
  if ((IsAutoplayEnabled() && HasAttr(kNameSpaceID_None, nsGkAtoms::autoplay)) ||
      !mPaused)
  {
    nextAction = nsHTMLMediaElement::PRELOAD_ENOUGH;
  } else {
    
    const nsAttrValue* val = mAttrsAndChildren.GetAttr(nsGkAtoms::preload,
                                                       kNameSpaceID_None);
    PRUint32 preloadDefault =
      Preferences::GetInt("media.preload.default",
                          nsHTMLMediaElement::PRELOAD_ATTR_METADATA);
    PRUint32 preloadAuto =
      Preferences::GetInt("media.preload.auto",
                          nsHTMLMediaElement::PRELOAD_ENOUGH);
    if (!val) {
      
      
      nextAction = static_cast<PreloadAction>(preloadDefault);
    } else if (val->Type() == nsAttrValue::eEnum) {
      PreloadAttrValue attr = static_cast<PreloadAttrValue>(val->GetEnumValue());
      if (attr == nsHTMLMediaElement::PRELOAD_ATTR_EMPTY ||
          attr == nsHTMLMediaElement::PRELOAD_ATTR_AUTO)
      {
        nextAction = static_cast<PreloadAction>(preloadAuto);
      } else if (attr == nsHTMLMediaElement::PRELOAD_ATTR_METADATA) {
        nextAction = nsHTMLMediaElement::PRELOAD_METADATA;
      } else if (attr == nsHTMLMediaElement::PRELOAD_ATTR_NONE) {
        nextAction = nsHTMLMediaElement::PRELOAD_NONE;
      }
    } else {
      
      
      nextAction = static_cast<PreloadAction>(preloadDefault);
    }
  }

  if ((mBegun || mIsRunningSelectResource) && nextAction < mPreloadAction) {
    
    
    
    return;
  }

  mPreloadAction = nextAction;
  if (nextAction == nsHTMLMediaElement::PRELOAD_ENOUGH) {
    if (mLoadIsSuspended) {
      
      
      
      ResumeLoad(PRELOAD_ENOUGH);
    } else {
      
      
      StopSuspendingAfterFirstFrame();
    }

  } else if (nextAction == nsHTMLMediaElement::PRELOAD_METADATA) {
    
    mAllowSuspendAfterFirstFrame = true;
    if (mLoadIsSuspended) {
      
      
      
      
      ResumeLoad(PRELOAD_METADATA);
    }
  }
}

nsresult nsHTMLMediaElement::LoadResource(nsIURI* aURI)
{
  NS_ASSERTION(mDelayingLoadEvent,
               "Should delay load event (if in document) during load");

  
  
  if (mAudioStream) {
    mAudioStream->Shutdown();
    mAudioStream = nsnull;
  }

  if (mChannel) {
    mChannel->Cancel(NS_BINDING_ABORTED);
    mChannel = nsnull;
  }

  PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
  nsresult rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_MEDIA,
                                          aURI,
                                          NodePrincipal(),
                                          static_cast<nsGenericElement*>(this),
                                          EmptyCString(), 
                                          nsnull, 
                                          &shouldLoad,
                                          nsContentUtils::GetContentPolicy(),
                                          nsContentUtils::GetSecurityManager());
  NS_ENSURE_SUCCESS(rv, rv);
  if (NS_CP_REJECTED(shouldLoad)) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsILoadGroup> loadGroup = GetDocumentLoadGroup();

  
  
  nsCOMPtr<nsIChannelPolicy> channelPolicy;
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  rv = NodePrincipal()->GetCsp(getter_AddRefs(csp));
  NS_ENSURE_SUCCESS(rv,rv);
  if (csp) {
    channelPolicy = do_CreateInstance("@mozilla.org/nschannelpolicy;1");
    channelPolicy->SetContentSecurityPolicy(csp);
    channelPolicy->SetLoadType(nsIContentPolicy::TYPE_MEDIA);
  }
  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel),
                     aURI,
                     nsnull,
                     loadGroup,
                     nsnull,
                     nsICachingChannel::LOAD_BYPASS_LOCAL_CACHE_IF_BUSY,
                     channelPolicy);
  NS_ENSURE_SUCCESS(rv,rv);

  
  
  
  
  
  
  nsRefPtr<MediaLoadListener> loadListener = new MediaLoadListener(this);

  channel->SetNotificationCallbacks(loadListener);

  nsCOMPtr<nsIStreamListener> listener;
  if (ShouldCheckAllowOrigin()) {
    listener =
      new nsCORSListenerProxy(loadListener,
                              NodePrincipal(),
                              channel,
                              false,
                              &rv);
  } else {
    rv = nsContentUtils::GetSecurityManager()->
           CheckLoadURIWithPrincipal(NodePrincipal(),
                                     aURI,
                                     nsIScriptSecurityManager::STANDARD);
    listener = loadListener;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIHttpChannel> hc = do_QueryInterface(channel);
  if (hc) {
    
    
    
    hc->SetRequestHeader(NS_LITERAL_CSTRING("Range"),
                         NS_LITERAL_CSTRING("bytes=0-"),
                         false);

    SetRequestHeaders(hc);
  }

  rv = channel->AsyncOpen(listener, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  mChannel = channel;

  
  
  nsContentUtils::RegisterShutdownObserver(loadListener);
  return NS_OK;
}

nsresult nsHTMLMediaElement::LoadWithChannel(nsIChannel *aChannel,
                                             nsIStreamListener **aListener)
{
  NS_ENSURE_ARG_POINTER(aChannel);
  NS_ENSURE_ARG_POINTER(aListener);

  *aListener = nsnull;

  AbortExistingLoads();

  ChangeDelayLoadStatus(true);

  nsresult rv = InitializeDecoderForChannel(aChannel, aListener);
  if (NS_FAILED(rv)) {
    ChangeDelayLoadStatus(false);
    return rv;
  }

  DispatchAsyncEvent(NS_LITERAL_STRING("loadstart"));

  return NS_OK;
}

NS_IMETHODIMP nsHTMLMediaElement::MozLoadFrom(nsIDOMHTMLMediaElement* aOther)
{
  NS_ENSURE_ARG_POINTER(aOther);

  AbortExistingLoads();

  nsCOMPtr<nsIContent> content = do_QueryInterface(aOther);
  nsHTMLMediaElement* other = static_cast<nsHTMLMediaElement*>(content.get());
  if (!other || !other->mDecoder)
    return NS_OK;

  ChangeDelayLoadStatus(true);

  nsresult rv = InitializeDecoderAsClone(other->mDecoder);
  if (NS_FAILED(rv)) {
    ChangeDelayLoadStatus(false);
    return rv;
  }

  DispatchAsyncEvent(NS_LITERAL_STRING("loadstart"));

  return NS_OK;
}


NS_IMETHODIMP nsHTMLMediaElement::GetReadyState(PRUint16 *aReadyState)
{
  *aReadyState = mReadyState;

  return NS_OK;
}


NS_IMETHODIMP nsHTMLMediaElement::GetSeeking(bool *aSeeking)
{
  *aSeeking = mDecoder && mDecoder->IsSeeking();

  return NS_OK;
}


NS_IMETHODIMP nsHTMLMediaElement::GetCurrentTime(double *aCurrentTime)
{
  *aCurrentTime = mDecoder ? mDecoder->GetCurrentTime() : 0.0;
  return NS_OK;
}

NS_IMETHODIMP nsHTMLMediaElement::SetCurrentTime(double aCurrentTime)
{
  StopSuspendingAfterFirstFrame();

  if (!mDecoder) {
    LOG(PR_LOG_DEBUG, ("%p SetCurrentTime(%f) failed: no decoder", this, aCurrentTime));
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  if (mReadyState == nsIDOMHTMLMediaElement::HAVE_NOTHING) {
    LOG(PR_LOG_DEBUG, ("%p SetCurrentTime(%f) failed: no source", this, aCurrentTime));
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  
  if (aCurrentTime != aCurrentTime) {
    LOG(PR_LOG_DEBUG, ("%p SetCurrentTime(%f) failed: bad time", this, aCurrentTime));
    return NS_ERROR_FAILURE;
  }

  
  double clampedTime = NS_MAX(0.0, aCurrentTime);
  double duration = mDecoder->GetDuration();
  if (duration >= 0) {
    clampedTime = NS_MIN(clampedTime, duration);
  }

  mPlayingBeforeSeek = IsPotentiallyPlaying();
  
  
  LOG(PR_LOG_DEBUG, ("%p SetCurrentTime(%f) starting seek", this, aCurrentTime));
  nsresult rv = mDecoder->Seek(clampedTime);

  
  AddRemoveSelfReference();

  return rv;
}


NS_IMETHODIMP nsHTMLMediaElement::GetDuration(double *aDuration)
{
  *aDuration = mDecoder ? mDecoder->GetDuration() : std::numeric_limits<double>::quiet_NaN();
  return NS_OK;
}


NS_IMETHODIMP nsHTMLMediaElement::GetSeekable(nsIDOMTimeRanges** aSeekable)
{
  nsRefPtr<nsTimeRanges> ranges = new nsTimeRanges();
  if (mDecoder && mReadyState > nsIDOMHTMLMediaElement::HAVE_NOTHING) {
    mDecoder->GetSeekable(ranges);
  }
  ranges.forget(aSeekable);
  return NS_OK;
}



NS_IMETHODIMP nsHTMLMediaElement::GetPaused(bool *aPaused)
{
  *aPaused = mPaused;

  return NS_OK;
}


NS_IMETHODIMP nsHTMLMediaElement::Pause()
{
  if (mNetworkState == nsIDOMHTMLMediaElement::NETWORK_EMPTY) {
    LOG(PR_LOG_DEBUG, ("Loading due to Pause()"));
    nsresult rv = Load();
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (mDecoder) {
    mDecoder->Pause();
  }

  bool oldPaused = mPaused;
  mPaused = true;
  mAutoplaying = false;
  
  AddRemoveSelfReference();

  if (!oldPaused) {
    FireTimeUpdate(false);
    DispatchAsyncEvent(NS_LITERAL_STRING("pause"));
  }

  return NS_OK;
}


NS_IMETHODIMP nsHTMLMediaElement::GetVolume(double *aVolume)
{
  *aVolume = mVolume;

  return NS_OK;
}

NS_IMETHODIMP nsHTMLMediaElement::SetVolume(double aVolume)
{
  if (aVolume < 0.0 || aVolume > 1.0)
    return NS_ERROR_DOM_INDEX_SIZE_ERR;

  if (aVolume == mVolume)
    return NS_OK;

  mVolume = aVolume;

  if (mDecoder && !mMuted) {
    mDecoder->SetVolume(mVolume);
  } else if (mAudioStream && !mMuted) {
    mAudioStream->SetVolume(mVolume);
  }

  DispatchAsyncEvent(NS_LITERAL_STRING("volumechange"));

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLMediaElement::GetMozChannels(PRUint32 *aMozChannels)
{
  if (!mDecoder && !mAudioStream) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  *aMozChannels = mChannels;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLMediaElement::GetMozSampleRate(PRUint32 *aMozSampleRate)
{
  if (!mDecoder && !mAudioStream) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  *aMozSampleRate = mRate;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLMediaElement::GetMozFrameBufferLength(PRUint32 *aMozFrameBufferLength)
{
  
  
  if (!mDecoder) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  *aMozFrameBufferLength = mDecoder->GetFrameBufferLength();
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLMediaElement::SetMozFrameBufferLength(PRUint32 aMozFrameBufferLength)
{
  if (!mDecoder)
    return NS_ERROR_DOM_INVALID_STATE_ERR;

  return mDecoder->RequestFrameBufferLength(aMozFrameBufferLength);
}


NS_IMETHODIMP nsHTMLMediaElement::GetMuted(bool *aMuted)
{
  *aMuted = mMuted;

  return NS_OK;
}

NS_IMETHODIMP nsHTMLMediaElement::SetMuted(bool aMuted)
{
  if (aMuted == mMuted)
    return NS_OK;

  mMuted = aMuted;

  if (mDecoder) {
    mDecoder->SetVolume(mMuted ? 0.0 : mVolume);
  } else if (mAudioStream) {
    mAudioStream->SetVolume(mMuted ? 0.0 : mVolume);
  }

  DispatchAsyncEvent(NS_LITERAL_STRING("volumechange"));

  return NS_OK;
}

nsHTMLMediaElement::nsHTMLMediaElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo),
    mCurrentLoadID(0),
    mNetworkState(nsIDOMHTMLMediaElement::NETWORK_EMPTY),
    mReadyState(nsIDOMHTMLMediaElement::HAVE_NOTHING),
    mLoadWaitStatus(NOT_WAITING),
    mVolume(1.0),
    mChannels(0),
    mRate(0),
    mPreloadAction(PRELOAD_UNDEFINED),
    mMediaSize(-1,-1),
    mLastCurrentTime(0.0),
    mFragmentStart(-1.0),
    mFragmentEnd(-1.0),
    mAllowAudioData(false),
    mBegun(false),
    mLoadedFirstFrame(false),
    mAutoplaying(true),
    mAutoplayEnabled(true),
    mPaused(true),
    mMuted(false),
    mPlayingBeforeSeek(false),
    mPausedForInactiveDocument(false),
    mWaitingFired(false),
    mIsRunningLoadMethod(false),
    mIsLoadingFromSourceChildren(false),
    mDelayingLoadEvent(false),
    mIsRunningSelectResource(false),
    mSuspendedAfterFirstFrame(false),
    mAllowSuspendAfterFirstFrame(true),
    mHasPlayedOrSeeked(false),
    mHasSelfReference(false),
    mShuttingDown(false),
    mLoadIsSuspended(false),
    mMediaSecurityVerified(false)
{
#ifdef PR_LOGGING
  if (!gMediaElementLog) {
    gMediaElementLog = PR_NewLogModule("nsMediaElement");
  }
  if (!gMediaElementEventsLog) {
    gMediaElementEventsLog = PR_NewLogModule("nsMediaElementEvents");
  }
#endif

  RegisterFreezableElement();
  NotifyOwnerDocumentActivityChanged();
}

nsHTMLMediaElement::~nsHTMLMediaElement()
{
  NS_ASSERTION(!mHasSelfReference,
               "How can we be destroyed if we're still holding a self reference?");

  UnregisterFreezableElement();
  if (mDecoder) {
    mDecoder->Shutdown();
    mDecoder = nsnull;
  }
  if (mChannel) {
    mChannel->Cancel(NS_BINDING_ABORTED);
    mChannel = nsnull;
  }
  if (mAudioStream) {
    mAudioStream->Shutdown();
    mAudioStream = nsnull;
  }
}

void nsHTMLMediaElement::StopSuspendingAfterFirstFrame()
{
  mAllowSuspendAfterFirstFrame = false;
  if (!mSuspendedAfterFirstFrame)
    return;
  mSuspendedAfterFirstFrame = false;
  if (mDecoder) {
    mDecoder->Resume(true);
  }
}

void nsHTMLMediaElement::SetPlayedOrSeeked(bool aValue)
{
  if (aValue == mHasPlayedOrSeeked) {
    return;
  }

  mHasPlayedOrSeeked = aValue;

  
  nsIFrame* frame = GetPrimaryFrame();
  if (!frame) {
    return;
  }
  frame->PresContext()->PresShell()->FrameNeedsReflow(frame,
                                                      nsIPresShell::eTreeChange,
                                                      NS_FRAME_IS_DIRTY);
}

NS_IMETHODIMP nsHTMLMediaElement::Play()
{
  StopSuspendingAfterFirstFrame();
  SetPlayedOrSeeked(true);

  if (mNetworkState == nsIDOMHTMLMediaElement::NETWORK_EMPTY) {
    nsresult rv = Load();
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (mLoadIsSuspended) {
    ResumeLoad(PRELOAD_ENOUGH);
  } else if (mDecoder) {
    if (mDecoder->IsEnded()) {
      SetCurrentTime(0);
    }
    if (!mPausedForInactiveDocument) {
      nsresult rv = mDecoder->Play();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  
  
  if (mPaused) {
    DispatchAsyncEvent(NS_LITERAL_STRING("play"));
    switch (mReadyState) {
    case nsIDOMHTMLMediaElement::HAVE_NOTHING:
      DispatchAsyncEvent(NS_LITERAL_STRING("waiting"));
      break;
    case nsIDOMHTMLMediaElement::HAVE_METADATA:
    case nsIDOMHTMLMediaElement::HAVE_CURRENT_DATA:
      FireTimeUpdate(false);
      DispatchAsyncEvent(NS_LITERAL_STRING("waiting"));
      break;
    case nsIDOMHTMLMediaElement::HAVE_FUTURE_DATA:
    case nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA:
      DispatchAsyncEvent(NS_LITERAL_STRING("playing"));
      break;
    }
  }

  mPaused = false;
  mAutoplaying = false;
  
  
  AddRemoveSelfReference();
  UpdatePreloadAction();

  return NS_OK;
}

bool nsHTMLMediaElement::ParseAttribute(PRInt32 aNamespaceID,
                                          nsIAtom* aAttribute,
                                          const nsAString& aValue,
                                          nsAttrValue& aResult)
{
  
  static const nsAttrValue::EnumTable kPreloadTable[] = {
    { "",         nsHTMLMediaElement::PRELOAD_ATTR_EMPTY },
    { "none",     nsHTMLMediaElement::PRELOAD_ATTR_NONE },
    { "metadata", nsHTMLMediaElement::PRELOAD_ATTR_METADATA },
    { "auto",     nsHTMLMediaElement::PRELOAD_ATTR_AUTO },
    { 0 }
  };

  if (aNamespaceID == kNameSpaceID_None) {
    if (ParseImageAttribute(aAttribute, aValue, aResult)) {
      return true;
    }
    if (aAttribute == nsGkAtoms::preload) {
      return aResult.ParseEnumValue(aValue, kPreloadTable, false);
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

nsresult nsHTMLMediaElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                     nsIAtom* aPrefix, const nsAString& aValue,
                                     bool aNotify)
{
  nsresult rv =
    nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix, aValue,
                                  aNotify);
  if (NS_FAILED(rv))
    return rv;
  if (aNameSpaceID == kNameSpaceID_None && aName == nsGkAtoms::src) {
    Load();
  }
  if (aNotify && aNameSpaceID == kNameSpaceID_None) {
    if (aName == nsGkAtoms::autoplay) {
      StopSuspendingAfterFirstFrame();
      if (mReadyState == nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA) {
        NotifyAutoplayDataReady();
      }
      
      AddRemoveSelfReference();
      UpdatePreloadAction();
    } else if (aName == nsGkAtoms::preload) {
      UpdatePreloadAction();
    }
  }

  return rv;
}

nsresult nsHTMLMediaElement::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttr,
                                       bool aNotify)
{
  nsresult rv = nsGenericHTMLElement::UnsetAttr(aNameSpaceID, aAttr, aNotify);
  if (NS_FAILED(rv))
    return rv;
  if (aNotify && aNameSpaceID == kNameSpaceID_None) {
    if (aAttr == nsGkAtoms::autoplay) {
      
      AddRemoveSelfReference();
      UpdatePreloadAction();
    } else if (aAttr == nsGkAtoms::preload) {
      UpdatePreloadAction();
    }
  }

  return rv;
}

nsresult nsHTMLMediaElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                        nsIContent* aBindingParent,
                                        bool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument,
                                                 aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  if (aDocument) {
    mAutoplayEnabled =
      IsAutoplayEnabled() && (!aDocument || !aDocument->IsStaticDocument()) &&
      !IsEditable();
    
    
    UpdatePreloadAction();
  }

  return rv;
}

void nsHTMLMediaElement::UnbindFromTree(bool aDeep,
                                        bool aNullParent)
{
  if (!mPaused && mNetworkState != nsIDOMHTMLMediaElement::NETWORK_EMPTY)
    Pause();
  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
}

#ifdef MOZ_RAW
static const char gRawTypes[][16] = {
  "video/x-raw",
  "video/x-raw-yuv"
};

static const char* gRawCodecs[] = {
  nsnull
};

static bool IsRawEnabled()
{
  return Preferences::GetBool("media.raw.enabled");
}

static bool IsRawType(const nsACString& aType)
{
  if (!IsRawEnabled()) {
    return false;
  }

  for (PRUint32 i = 0; i < ArrayLength(gRawTypes); ++i) {
    if (aType.EqualsASCII(gRawTypes[i])) {
      return true;
    }
  }

  return false;
}
#endif
#ifdef MOZ_OGG


const char nsHTMLMediaElement::gOggTypes[3][16] = {
  "video/ogg",
  "audio/ogg",
  "application/ogg"
};

char const *const nsHTMLMediaElement::gOggCodecs[3] = {
  "vorbis",
  "theora",
  nsnull
};

bool
nsHTMLMediaElement::IsOggEnabled()
{
  return Preferences::GetBool("media.ogg.enabled");
}

bool
nsHTMLMediaElement::IsOggType(const nsACString& aType)
{
  if (!IsOggEnabled()) {
    return false;
  }

  for (PRUint32 i = 0; i < ArrayLength(gOggTypes); ++i) {
    if (aType.EqualsASCII(gOggTypes[i])) {
      return true;
    }
  }

  return false;
}
#endif

#ifdef MOZ_WAVE



const char nsHTMLMediaElement::gWaveTypes[4][16] = {
  "audio/x-wav",
  "audio/wav",
  "audio/wave",
  "audio/x-pn-wav"
};

char const *const nsHTMLMediaElement::gWaveCodecs[2] = {
  "1", 
  nsnull
};

bool
nsHTMLMediaElement::IsWaveEnabled()
{
  return Preferences::GetBool("media.wave.enabled");
}

bool
nsHTMLMediaElement::IsWaveType(const nsACString& aType)
{
  if (!IsWaveEnabled()) {
    return false;
  }

  for (PRUint32 i = 0; i < ArrayLength(gWaveTypes); ++i) {
    if (aType.EqualsASCII(gWaveTypes[i])) {
      return true;
    }
  }

  return false;
}
#endif

#ifdef MOZ_WEBM
const char nsHTMLMediaElement::gWebMTypes[2][17] = {
  "video/webm",
  "audio/webm"
};

char const *const nsHTMLMediaElement::gWebMCodecs[4] = {
  "vp8",
  "vp8.0",
  "vorbis",
  nsnull
};

bool
nsHTMLMediaElement::IsWebMEnabled()
{
  return Preferences::GetBool("media.webm.enabled");
}

bool
nsHTMLMediaElement::IsWebMType(const nsACString& aType)
{
  if (!IsWebMEnabled()) {
    return false;
  }

  for (PRUint32 i = 0; i < ArrayLength(gWebMTypes); ++i) {
    if (aType.EqualsASCII(gWebMTypes[i])) {
      return true;
    }
  }

  return false;
}
#endif


nsHTMLMediaElement::CanPlayStatus 
nsHTMLMediaElement::CanHandleMediaType(const char* aMIMEType,
                                       char const *const ** aCodecList)
{
#ifdef MOZ_RAW
  if (IsRawType(nsDependentCString(aMIMEType))) {
    *aCodecList = gRawCodecs;
    return CANPLAY_MAYBE;
  }
#endif
#ifdef MOZ_OGG
  if (IsOggType(nsDependentCString(aMIMEType))) {
    *aCodecList = gOggCodecs;
    return CANPLAY_MAYBE;
  }
#endif
#ifdef MOZ_WAVE
  if (IsWaveType(nsDependentCString(aMIMEType))) {
    *aCodecList = gWaveCodecs;
    return CANPLAY_MAYBE;
  }
#endif
#ifdef MOZ_WEBM
  if (IsWebMType(nsDependentCString(aMIMEType))) {
    *aCodecList = gWebMCodecs;
    return CANPLAY_YES;
  }
#endif
  return CANPLAY_NO;
}


bool nsHTMLMediaElement::ShouldHandleMediaType(const char* aMIMEType)
{
#ifdef MOZ_RAW
  if (IsRawType(nsDependentCString(aMIMEType)))
    return true;
#endif
#ifdef MOZ_OGG
  if (IsOggType(nsDependentCString(aMIMEType)))
    return true;
#endif
#ifdef MOZ_WEBM
  if (IsWebMType(nsDependentCString(aMIMEType)))
    return true;
#endif
  
  
  
  
  
  return false;
}

static bool
CodecListContains(char const *const * aCodecs, const nsAString& aCodec)
{
  for (PRInt32 i = 0; aCodecs[i]; ++i) {
    if (aCodec.EqualsASCII(aCodecs[i]))
      return true;
  }
  return false;
}


nsHTMLMediaElement::CanPlayStatus
nsHTMLMediaElement::GetCanPlay(const nsAString& aType)
{
  nsContentTypeParser parser(aType);
  nsAutoString mimeType;
  nsresult rv = parser.GetType(mimeType);
  if (NS_FAILED(rv))
    return CANPLAY_NO;

  NS_ConvertUTF16toUTF8 mimeTypeUTF8(mimeType);
  char const *const * supportedCodecs;
  CanPlayStatus status = CanHandleMediaType(mimeTypeUTF8.get(),
                                            &supportedCodecs);
  if (status == CANPLAY_NO)
    return CANPLAY_NO;

  nsAutoString codecs;
  rv = parser.GetParameter("codecs", codecs);
  if (NS_FAILED(rv)) {
    
    return status;
  }

  CanPlayStatus result = CANPLAY_YES;
  
  
  nsCharSeparatedTokenizer tokenizer(codecs, ',');
  bool expectMoreTokens = false;
  while (tokenizer.hasMoreTokens()) {
    const nsSubstring& token = tokenizer.nextToken();

    if (!CodecListContains(supportedCodecs, token)) {
      
      return CANPLAY_NO;
    }
    expectMoreTokens = tokenizer.lastTokenEndedWithSeparator();
  }
  if (expectMoreTokens) {
    
    return CANPLAY_NO;
  }
  return result;
}

NS_IMETHODIMP
nsHTMLMediaElement::CanPlayType(const nsAString& aType, nsAString& aResult)
{
  switch (GetCanPlay(aType)) {
  case CANPLAY_NO:
    aResult.Truncate();
    break;
  case CANPLAY_YES:
    aResult.AssignLiteral("probably");
    break;
  default:
  case CANPLAY_MAYBE:
    aResult.AssignLiteral("maybe");
    break;
  }
  return NS_OK;
}

already_AddRefed<nsMediaDecoder>
nsHTMLMediaElement::CreateDecoder(const nsACString& aType)
{
#ifdef MOZ_RAW
  if (IsRawType(aType)) {
    nsRefPtr<nsRawDecoder> decoder = new nsRawDecoder();
    if (decoder->Init(this)) {
      return decoder.forget();
    }
  }
#endif
#ifdef MOZ_OGG
  if (IsOggType(aType)) {
    nsRefPtr<nsOggDecoder> decoder = new nsOggDecoder();
    if (decoder->Init(this)) {
      return decoder.forget();
    }
  }
#endif
#ifdef MOZ_WAVE
  if (IsWaveType(aType)) {
    nsRefPtr<nsWaveDecoder> decoder = new nsWaveDecoder();
    if (decoder->Init(this)) {
      return decoder.forget();
    }
  }
#endif
#ifdef MOZ_WEBM
  if (IsWebMType(aType)) {
    nsRefPtr<nsWebMDecoder> decoder = new nsWebMDecoder();
    if (decoder->Init(this)) {
      return decoder.forget();
    }
  }
#endif
  return nsnull;
}

nsresult nsHTMLMediaElement::InitializeDecoderAsClone(nsMediaDecoder* aOriginal)
{
  nsMediaStream* originalStream = aOriginal->GetCurrentStream();
  if (!originalStream)
    return NS_ERROR_FAILURE;
  nsRefPtr<nsMediaDecoder> decoder = aOriginal->Clone();
  if (!decoder)
    return NS_ERROR_FAILURE;

  LOG(PR_LOG_DEBUG, ("%p Cloned decoder %p from %p", this, decoder.get(), aOriginal));

  if (!decoder->Init(this)) {
    return NS_ERROR_FAILURE;
  }

  double duration = aOriginal->GetDuration();
  if (duration >= 0) {
    decoder->SetDuration(duration);
    decoder->SetSeekable(aOriginal->IsSeekable());
  }

  nsMediaStream* stream = originalStream->CloneData(decoder);
  if (!stream) {
    return NS_ERROR_FAILURE;
  }

  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_LOADING;

  nsresult rv = decoder->Load(stream, nsnull, aOriginal);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return FinishDecoderSetup(decoder);
}

nsresult nsHTMLMediaElement::InitializeDecoderForChannel(nsIChannel *aChannel,
                                                         nsIStreamListener **aListener)
{
  nsCAutoString mimeType;
  aChannel->GetContentType(mimeType);

  nsRefPtr<nsMediaDecoder> decoder = CreateDecoder(mimeType);
  if (!decoder) {
    return NS_ERROR_FAILURE;
  }

  LOG(PR_LOG_DEBUG, ("%p Created decoder %p for type %s", this, decoder.get(), mimeType.get()));

  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_LOADING;

  nsMediaStream* stream = nsMediaStream::Create(decoder, aChannel);
  if (!stream)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = decoder->Load(stream, aListener, nsnull);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  mChannel = nsnull;

  return FinishDecoderSetup(decoder);
}

nsresult nsHTMLMediaElement::FinishDecoderSetup(nsMediaDecoder* aDecoder)
{
  mDecoder = aDecoder;

  
  mLoadingSrc = nsnull;

  
  mMediaSecurityVerified = false;

  
  mPausedForInactiveDocument = false;
  
  
  NotifyOwnerDocumentActivityChanged();

  nsresult rv = NS_OK;

  mDecoder->SetVolume(mMuted ? 0.0 : mVolume);

  if (!mPaused) {
    SetPlayedOrSeeked(true);
    if (!mPausedForInactiveDocument) {
      rv = mDecoder->Play();
    }
  }

  mBegun = true;
  return rv;
}

nsresult nsHTMLMediaElement::NewURIFromString(const nsAutoString& aURISpec, nsIURI** aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);

  *aURI = nsnull;

  nsCOMPtr<nsIDocument> doc = OwnerDoc();

  nsCOMPtr<nsIURI> baseURI = GetBaseURI();
  nsCOMPtr<nsIURI> uri;
  nsresult rv = nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(uri),
                                                          aURISpec,
                                                          doc,
                                                          baseURI);
  NS_ENSURE_SUCCESS(rv, rv);

  bool equal;
  if (aURISpec.IsEmpty() &&
      doc->GetDocumentURI() &&
      NS_SUCCEEDED(doc->GetDocumentURI()->Equals(uri, &equal)) &&
      equal) {
    
    
    
    
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  uri.forget(aURI);
  return NS_OK;
}

void nsHTMLMediaElement::ProcessMediaFragmentURI()
{
  nsCAutoString ref;
  GetCurrentSpec(ref);
  nsMediaFragmentURIParser parser(ref);
  parser.Parse();
  double start = parser.GetStartTime();
  if (mDecoder) {
    double end = parser.GetEndTime();
    if (end < 0.0 || end > start) {
      mFragmentEnd = end;
    }
    else {
      start = -1.0;
      end = -1.0;
    }
  }
  if (start > 0.0) {
    SetCurrentTime(start);
    mFragmentStart = start;
  }
}

void nsHTMLMediaElement::MetadataLoaded(PRUint32 aChannels, PRUint32 aRate)
{
  mChannels = aChannels;
  mRate = aRate;
  ChangeReadyState(nsIDOMHTMLMediaElement::HAVE_METADATA);
  DispatchAsyncEvent(NS_LITERAL_STRING("durationchange"));
  DispatchAsyncEvent(NS_LITERAL_STRING("loadedmetadata"));
  if (mDecoder && mDecoder->IsSeekable()) {
    ProcessMediaFragmentURI();
    mDecoder->SetEndTime(mFragmentEnd);
  }
}

void nsHTMLMediaElement::FirstFrameLoaded(bool aResourceFullyLoaded)
{
  ChangeReadyState(aResourceFullyLoaded ?
    nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA :
    nsIDOMHTMLMediaElement::HAVE_CURRENT_DATA);
  ChangeDelayLoadStatus(false);

  NS_ASSERTION(!mSuspendedAfterFirstFrame, "Should not have already suspended");

  if (mDecoder && mAllowSuspendAfterFirstFrame && mPaused &&
      !aResourceFullyLoaded &&
      !HasAttr(kNameSpaceID_None, nsGkAtoms::autoplay) &&
      mPreloadAction == nsHTMLMediaElement::PRELOAD_METADATA) {
    mSuspendedAfterFirstFrame = true;
    mDecoder->Suspend();
  }
}

void nsHTMLMediaElement::ResourceLoaded()
{
  mBegun = false;
  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_IDLE;
  AddRemoveSelfReference();
  if (mReadyState >= nsIDOMHTMLMediaElement::HAVE_METADATA) {
    ChangeReadyState(nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA);
  }
  
  DispatchAsyncEvent(NS_LITERAL_STRING("progress"));
  
  DispatchAsyncEvent(NS_LITERAL_STRING("suspend"));
}

void nsHTMLMediaElement::NetworkError()
{
  Error(nsIDOMMediaError::MEDIA_ERR_NETWORK);
}

void nsHTMLMediaElement::DecodeError()
{
  if (mDecoder) {
    mDecoder->Shutdown();
    mDecoder = nsnull;
  }
  if (mIsLoadingFromSourceChildren) {
    mError = nsnull;
    if (mSourceLoadCandidate) {
      DispatchAsyncSourceError(mSourceLoadCandidate);
      QueueLoadFromSourceTask();
    } else {
      NS_WARNING("Should know the source we were loading from!");
    }
  } else {
    Error(nsIDOMMediaError::MEDIA_ERR_DECODE);
  }
}

void nsHTMLMediaElement::LoadAborted()
{
  Error(nsIDOMMediaError::MEDIA_ERR_ABORTED);
}

void nsHTMLMediaElement::Error(PRUint16 aErrorCode)
{
  NS_ASSERTION(aErrorCode == nsIDOMMediaError::MEDIA_ERR_DECODE ||
               aErrorCode == nsIDOMMediaError::MEDIA_ERR_NETWORK ||
               aErrorCode == nsIDOMMediaError::MEDIA_ERR_ABORTED,
               "Only use nsIDOMMediaError codes!");
  mError = new nsMediaError(aErrorCode);
  mBegun = false;
  DispatchAsyncEvent(NS_LITERAL_STRING("error"));
  if (mReadyState == nsIDOMHTMLMediaElement::HAVE_NOTHING) {
    mNetworkState = nsIDOMHTMLMediaElement::NETWORK_EMPTY;
    DispatchAsyncEvent(NS_LITERAL_STRING("emptied"));
  } else {
    mNetworkState = nsIDOMHTMLMediaElement::NETWORK_IDLE;
  }
  AddRemoveSelfReference();
  ChangeDelayLoadStatus(false);
}

void nsHTMLMediaElement::PlaybackEnded()
{
  NS_ASSERTION(mDecoder->IsEnded(), "Decoder fired ended, but not in ended state");
  
  AddRemoveSelfReference();

  if (mDecoder && mDecoder->IsInfinite()) {
    LOG(PR_LOG_DEBUG, ("%p, got duration by reaching the end of the stream", this));
    DispatchAsyncEvent(NS_LITERAL_STRING("durationchange"));
  }

  if (HasAttr(kNameSpaceID_None, nsGkAtoms::loop)) {
    SetCurrentTime(0);
    return;
  }

  FireTimeUpdate(false);
  DispatchAsyncEvent(NS_LITERAL_STRING("ended"));
}

void nsHTMLMediaElement::SeekStarted()
{
  DispatchAsyncEvent(NS_LITERAL_STRING("seeking"));
  FireTimeUpdate(false);
}

void nsHTMLMediaElement::SeekCompleted()
{
  mPlayingBeforeSeek = false;
  SetPlayedOrSeeked(true);
  DispatchAsyncEvent(NS_LITERAL_STRING("seeked"));
  
  AddRemoveSelfReference();
}

void nsHTMLMediaElement::DownloadSuspended()
{
  DispatchAsyncEvent(NS_LITERAL_STRING("progress"));
  if (mBegun) {
    mNetworkState = nsIDOMHTMLMediaElement::NETWORK_IDLE;
    AddRemoveSelfReference();
    DispatchAsyncEvent(NS_LITERAL_STRING("suspend"));
  }
}

void nsHTMLMediaElement::DownloadResumed()
{
  if (mBegun) {
    mNetworkState = nsIDOMHTMLMediaElement::NETWORK_LOADING;
    AddRemoveSelfReference();
  }
}

void nsHTMLMediaElement::DownloadStalled()
{
  if (mNetworkState == nsIDOMHTMLMediaElement::NETWORK_LOADING) {
    DispatchAsyncEvent(NS_LITERAL_STRING("stalled"));
  }
}

bool nsHTMLMediaElement::ShouldCheckAllowOrigin()
{
  return Preferences::GetBool("media.enforce_same_site_origin", true);
}

void nsHTMLMediaElement::UpdateReadyStateForData(NextFrameStatus aNextFrame)
{
  if (mReadyState < nsIDOMHTMLMediaElement::HAVE_METADATA) {
    
    
    
    
    return;
  }

  if (aNextFrame != NEXT_FRAME_AVAILABLE) {
    ChangeReadyState(nsIDOMHTMLMediaElement::HAVE_CURRENT_DATA);
    if (!mWaitingFired && aNextFrame == NEXT_FRAME_UNAVAILABLE_BUFFERING) {
      FireTimeUpdate(false);
      DispatchAsyncEvent(NS_LITERAL_STRING("waiting"));
      mWaitingFired = true;
    }
    return;
  }

  
  
  
  
  
  
  
  
  nsMediaDecoder::Statistics stats = mDecoder->GetStatistics();
  if (stats.mTotalBytes < 0 ? stats.mDownloadRateReliable :
                              stats.mTotalBytes == stats.mDownloadPosition ||
      mDecoder->CanPlayThrough())
  {
    ChangeReadyState(nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA);
    return;
  }
  ChangeReadyState(nsIDOMHTMLMediaElement::HAVE_FUTURE_DATA);
}

#ifdef PR_LOGGING
static const char* gReadyStateToString[] = {
  "HAVE_NOTHING",
  "HAVE_METADATA",
  "HAVE_CURRENT_DATA",
  "HAVE_FUTURE_DATA",
  "HAVE_ENOUGH_DATA"
};
#endif

void nsHTMLMediaElement::ChangeReadyState(nsMediaReadyState aState)
{
  nsMediaReadyState oldState = mReadyState;
  mReadyState = aState;

  if (mNetworkState == nsIDOMHTMLMediaElement::NETWORK_EMPTY ||
      oldState == mReadyState) {
    return;
  }

  LOG(PR_LOG_DEBUG, ("%p Ready state changed to %s", this, gReadyStateToString[aState]));

  
  if (mPlayingBeforeSeek &&
      oldState < nsIDOMHTMLMediaElement::HAVE_FUTURE_DATA) {
    DispatchAsyncEvent(NS_LITERAL_STRING("waiting"));
  }

  if (oldState < nsIDOMHTMLMediaElement::HAVE_CURRENT_DATA &&
      mReadyState >= nsIDOMHTMLMediaElement::HAVE_CURRENT_DATA &&
      !mLoadedFirstFrame)
  {
    DispatchAsyncEvent(NS_LITERAL_STRING("loadeddata"));
    mLoadedFirstFrame = true;
  }

  if (mReadyState == nsIDOMHTMLMediaElement::HAVE_CURRENT_DATA) {
    mWaitingFired = false;
  }

  if (oldState < nsIDOMHTMLMediaElement::HAVE_FUTURE_DATA &&
      mReadyState >= nsIDOMHTMLMediaElement::HAVE_FUTURE_DATA) {
    DispatchAsyncEvent(NS_LITERAL_STRING("canplay"));
  }

  if (mReadyState == nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA) {
    NotifyAutoplayDataReady();
  }

  if (oldState < nsIDOMHTMLMediaElement::HAVE_FUTURE_DATA &&
      mReadyState >= nsIDOMHTMLMediaElement::HAVE_FUTURE_DATA &&
      IsPotentiallyPlaying()) {
    DispatchAsyncEvent(NS_LITERAL_STRING("playing"));
  }

  if (oldState < nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA &&
      mReadyState >= nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA) {
    DispatchAsyncEvent(NS_LITERAL_STRING("canplaythrough"));
  }
}

bool nsHTMLMediaElement::CanActivateAutoplay()
{
  return mAutoplaying &&
         mPaused &&
         HasAttr(kNameSpaceID_None, nsGkAtoms::autoplay) &&
         mAutoplayEnabled &&
         !IsEditable();
}

void nsHTMLMediaElement::NotifyAutoplayDataReady()
{
  if (CanActivateAutoplay()) {
    mPaused = false;
    
    AddRemoveSelfReference();

    if (mDecoder) {
      SetPlayedOrSeeked(true);
      mDecoder->Play();
    }
    DispatchAsyncEvent(NS_LITERAL_STRING("play"));
  }
}

ImageContainer* nsHTMLMediaElement::GetImageContainer()
{
  if (mImageContainer)
    return mImageContainer;

  
  
  if (mPrintSurface)
    return nsnull;

  
  nsCOMPtr<nsIDOMHTMLVideoElement> video = do_QueryObject(this);
  if (!video)
    return nsnull;

  nsRefPtr<LayerManager> manager =
    nsContentUtils::PersistentLayerManagerForDocument(OwnerDoc());
  if (!manager)
    return nsnull;

  mImageContainer = manager->CreateImageContainer();
  if (manager->IsCompositingCheap()) {
    mImageContainer->SetDelayedConversion(true);
  }
  return mImageContainer;
}

nsresult nsHTMLMediaElement::DispatchAudioAvailableEvent(float* aFrameBuffer,
                                                         PRUint32 aFrameBufferLength,
                                                         float aTime)
{
  
  
  
  
  nsAutoArrayPtr<float> frameBuffer(aFrameBuffer);

  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(OwnerDoc());
  nsCOMPtr<nsIDOMEventTarget> target(do_QueryObject(this));
  NS_ENSURE_TRUE(domDoc && target, NS_ERROR_INVALID_ARG);

  nsCOMPtr<nsIDOMEvent> event;
  nsresult rv = domDoc->CreateEvent(NS_LITERAL_STRING("MozAudioAvailableEvent"),
                                    getter_AddRefs(event));
  nsCOMPtr<nsIDOMNotifyAudioAvailableEvent> audioavailableEvent(do_QueryInterface(event));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = audioavailableEvent->InitAudioAvailableEvent(NS_LITERAL_STRING("MozAudioAvailable"),
                                                    true, true, frameBuffer.forget(), aFrameBufferLength,
                                                    aTime, mAllowAudioData);
  NS_ENSURE_SUCCESS(rv, rv);

  bool dummy;
  return target->DispatchEvent(event, &dummy);
}

nsresult nsHTMLMediaElement::DispatchEvent(const nsAString& aName)
{
  LOG_EVENT(PR_LOG_DEBUG, ("%p Dispatching event %s", this,
                          NS_ConvertUTF16toUTF8(aName).get()));

  
  
  if (mPausedForInactiveDocument) {
    mPendingEvents.AppendElement(aName);
    return NS_OK;
  }

  return nsContentUtils::DispatchTrustedEvent(OwnerDoc(),
                                              static_cast<nsIContent*>(this),
                                              aName,
                                              false,
                                              true);
}

nsresult nsHTMLMediaElement::DispatchAsyncEvent(const nsAString& aName)
{
  LOG_EVENT(PR_LOG_DEBUG, ("%p Queuing event %s", this,
            NS_ConvertUTF16toUTF8(aName).get()));

  nsCOMPtr<nsIRunnable> event = new nsAsyncEventRunner(aName, this);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  return NS_OK;
}

nsresult nsHTMLMediaElement::DispatchPendingMediaEvents()
{
  NS_ASSERTION(!mPausedForInactiveDocument,
               "Must not be in bfcache when dispatching pending media events");

  PRUint32 count = mPendingEvents.Length();
  for (PRUint32 i = 0; i < count; ++i) {
    DispatchAsyncEvent(mPendingEvents[i]);
  }
  mPendingEvents.Clear();

  return NS_OK;
}

bool nsHTMLMediaElement::IsPotentiallyPlaying() const
{
  
  
  
  return
    !mPaused &&
    (mReadyState == nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA ||
    mReadyState == nsIDOMHTMLMediaElement::HAVE_FUTURE_DATA) &&
    !IsPlaybackEnded();
}

bool nsHTMLMediaElement::IsPlaybackEnded() const
{
  
  
  
  return mNetworkState >= nsIDOMHTMLMediaElement::HAVE_METADATA &&
    mDecoder ? mDecoder->IsEnded() : false;
}

already_AddRefed<nsIPrincipal> nsHTMLMediaElement::GetCurrentPrincipal()
{
  if (!mDecoder)
    return nsnull;

  return mDecoder->GetCurrentPrincipal();
}

void nsHTMLMediaElement::UpdateMediaSize(nsIntSize size)
{
  mMediaSize = size;
}

void nsHTMLMediaElement::NotifyOwnerDocumentActivityChanged()
{
  nsIDocument* ownerDoc = OwnerDoc();
  
  
  bool pauseForInactiveDocument =
    !ownerDoc->IsActive() || !ownerDoc->IsVisible();

  if (pauseForInactiveDocument != mPausedForInactiveDocument) {
    mPausedForInactiveDocument = pauseForInactiveDocument;
    if (mDecoder) {
      if (pauseForInactiveDocument) {
        mDecoder->Pause();
        mDecoder->Suspend();
      } else {
        mDecoder->Resume(false);
        DispatchPendingMediaEvents();
        if (!mPaused && !mDecoder->IsEnded()) {
          mDecoder->Play();
        }
      }
    }
  }

  AddRemoveSelfReference();
}

void nsHTMLMediaElement::AddRemoveSelfReference()
{
  
  
  
  
  
  nsIDocument* ownerDoc = OwnerDoc();

  
  
  bool needSelfReference = !mShuttingDown &&
    ownerDoc->IsActive() &&
    (mDelayingLoadEvent ||
     (!mPaused && mDecoder && !mDecoder->IsEnded()) ||
     (mDecoder && mDecoder->IsSeeking()) ||
     CanActivateAutoplay() ||
     mNetworkState == nsIDOMHTMLMediaElement::NETWORK_LOADING);

  if (needSelfReference != mHasSelfReference) {
    mHasSelfReference = needSelfReference;
    if (needSelfReference) {
      
      
      
      nsContentUtils::RegisterShutdownObserver(this);
    } else {
      
      
      nsCOMPtr<nsIRunnable> event =
        NS_NewRunnableMethod(this, &nsHTMLMediaElement::DoRemoveSelfReference);
      NS_DispatchToMainThread(event);
    }
  }
}

void nsHTMLMediaElement::DoRemoveSelfReference()
{
  
  
  nsContentUtils::UnregisterShutdownObserver(this);
}

nsresult nsHTMLMediaElement::Observe(nsISupports* aSubject,
                                     const char* aTopic, const PRUnichar* aData)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);
  
  if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
    mShuttingDown = true;
    AddRemoveSelfReference();
  }
  return NS_OK;
}

bool
nsHTMLMediaElement::IsNodeOfType(PRUint32 aFlags) const
{
  return !(aFlags & ~(eCONTENT | eMEDIA));
}

void nsHTMLMediaElement::DispatchAsyncSourceError(nsIContent* aSourceElement)
{
  LOG_EVENT(PR_LOG_DEBUG, ("%p Queuing simple source error event", this));

  nsCOMPtr<nsIRunnable> event = new nsSourceErrorEventRunner(this, aSourceElement);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
}

void nsHTMLMediaElement::NotifyAddedSource()
{
  
  
  
  
  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::src) &&
      mNetworkState == nsIDOMHTMLMediaElement::NETWORK_EMPTY)
  {
    QueueSelectResourceTask();
  }

  
  
  if (mLoadWaitStatus == WAITING_FOR_SOURCE) {
    QueueLoadFromSourceTask();
  }
}

nsIContent* nsHTMLMediaElement::GetNextSource()
{
  nsCOMPtr<nsIDOMNode> thisDomNode = do_QueryObject(this);

  mSourceLoadCandidate = nsnull;

  nsresult rv = NS_OK;
  if (!mSourcePointer) {
    
    mSourcePointer = do_CreateInstance("@mozilla.org/content/range;1");

    rv = mSourcePointer->SelectNodeContents(thisDomNode);
    if (NS_FAILED(rv)) return nsnull;

    rv = mSourcePointer->Collapse(true);
    if (NS_FAILED(rv)) return nsnull;
  }

  while (true) {
#ifdef DEBUG
    nsCOMPtr<nsIDOMNode> startContainer;
    rv = mSourcePointer->GetStartContainer(getter_AddRefs(startContainer));
    if (NS_FAILED(rv)) return nsnull;
    NS_ASSERTION(startContainer == thisDomNode,
                "Should only iterate over direct children");
#endif

    PRInt32 startOffset = 0;
    rv = mSourcePointer->GetStartOffset(&startOffset);
    NS_ENSURE_SUCCESS(rv, nsnull);

    if (PRUint32(startOffset) == GetChildCount())
      return nsnull; 

    
    rv = mSourcePointer->SetStart(thisDomNode, startOffset + 1);
    NS_ENSURE_SUCCESS(rv, nsnull);

    nsIContent* child = GetChildAt(startOffset);

    
    if (child && child->IsHTML(nsGkAtoms::source)) {
      mSourceLoadCandidate = child;
      return child;
    }
  }
  NS_NOTREACHED("Execution should not reach here!");
  return nsnull;
}

void nsHTMLMediaElement::ChangeDelayLoadStatus(bool aDelay)
{
  if (mDelayingLoadEvent == aDelay)
    return;

  mDelayingLoadEvent = aDelay;

  if (aDelay) {
    mLoadBlockedDoc = OwnerDoc();
    mLoadBlockedDoc->BlockOnload();
    LOG(PR_LOG_DEBUG, ("%p ChangeDelayLoadStatus(%d) doc=0x%p", this, aDelay, mLoadBlockedDoc.get()));
  } else {
    if (mDecoder) {
      mDecoder->MoveLoadsToBackground();
    }
    LOG(PR_LOG_DEBUG, ("%p ChangeDelayLoadStatus(%d) doc=0x%p", this, aDelay, mLoadBlockedDoc.get()));
    
    if (mLoadBlockedDoc) {
      mLoadBlockedDoc->UnblockOnload(false);
      mLoadBlockedDoc = nsnull;
    }
  }

  
  AddRemoveSelfReference();
}

already_AddRefed<nsILoadGroup> nsHTMLMediaElement::GetDocumentLoadGroup()
{
  return OwnerDoc()->GetDocumentLoadGroup();
}

nsresult
nsHTMLMediaElement::CopyInnerTo(nsGenericElement* aDest) const
{
  nsresult rv = nsGenericHTMLElement::CopyInnerTo(aDest);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aDest->OwnerDoc()->IsStaticDocument()) {
    nsHTMLMediaElement* dest = static_cast<nsHTMLMediaElement*>(aDest);
    if (mPrintSurface) {
      dest->mPrintSurface = mPrintSurface;
      dest->mMediaSize = mMediaSize;
    } else {
      nsIFrame* frame = GetPrimaryFrame();
      nsCOMPtr<nsIDOMElement> elem;
      if (frame && frame->GetType() == nsGkAtoms::HTMLVideoFrame &&
          static_cast<nsVideoFrame*>(frame)->ShouldDisplayPoster()) {
        elem = do_QueryInterface(static_cast<nsVideoFrame*>(frame)->
                                 GetPosterImage());
      } else {
        elem = do_QueryInterface(
          static_cast<nsGenericElement*>(const_cast<nsHTMLMediaElement*>(this)));
      }

      nsLayoutUtils::SurfaceFromElementResult res =
        nsLayoutUtils::SurfaceFromElement(elem,
                                          nsLayoutUtils::SFE_WANT_NEW_SURFACE);
      dest->mPrintSurface = res.mSurface;
      dest->mMediaSize = nsIntSize(res.mSize.width, res.mSize.height);
    }
  }
  return rv;
}

nsresult nsHTMLMediaElement::GetBuffered(nsIDOMTimeRanges** aBuffered)
{
  nsRefPtr<nsTimeRanges> ranges = new nsTimeRanges();
  if (mReadyState >= nsIDOMHTMLMediaElement::HAVE_CURRENT_DATA && mDecoder) {
    
    
    mDecoder->GetBuffered(ranges);
  }
  ranges.forget(aBuffered);
  return NS_OK;
}

void nsHTMLMediaElement::SetRequestHeaders(nsIHttpChannel* aChannel)
{
  
  SetAcceptHeader(aChannel);

  
  
  
  
  
  aChannel->SetRequestHeader(NS_LITERAL_CSTRING("Accept-Encoding"),
                             EmptyCString(), false);

  
  aChannel->SetReferrer(OwnerDoc()->GetDocumentURI());
}

void nsHTMLMediaElement::FireTimeUpdate(bool aPeriodic)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  TimeStamp now = TimeStamp::Now();
  double time = 0;
  GetCurrentTime(&time);

  
  
  
  
  if (!aPeriodic ||
      (mLastCurrentTime != time &&
       (mTimeUpdateTime.IsNull() ||
        now - mTimeUpdateTime >= TimeDuration::FromMilliseconds(TIMEUPDATE_MS)))) {
    DispatchAsyncEvent(NS_LITERAL_STRING("timeupdate"));
    mTimeUpdateTime = now;
    mLastCurrentTime = time;
  }
  if (mFragmentEnd >= 0.0 && time >= mFragmentEnd) {
    Pause();
    mFragmentEnd = -1.0;
    mFragmentStart = -1.0;
    mDecoder->SetEndTime(mFragmentEnd);
  }
}

void nsHTMLMediaElement::GetCurrentSpec(nsCString& aString)
{
  if (mDecoder) {
    nsMediaStream* stream = mDecoder->GetCurrentStream();
    if (stream) {
      stream->URI()->GetSpec(aString);
    }
  } else if (mLoadingSrc) {
    mLoadingSrc->GetSpec(aString);
  }
}


NS_IMETHODIMP nsHTMLMediaElement::GetInitialTime(double *aTime)
{
  
  
  double duration = 0.0;
  nsresult rv = GetDuration(&duration);
  NS_ENSURE_SUCCESS(rv, rv);

  *aTime = mFragmentStart < 0.0 ? 0.0 : mFragmentStart;
  if (*aTime > duration) {
    *aTime = duration;
  }
  return NS_OK;
}


NS_IMETHODIMP nsHTMLMediaElement::GetMozFragmentEnd(double *aTime)
{
  double duration = 0.0;
  nsresult rv = GetDuration(&duration);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  *aTime = (mFragmentEnd < 0.0 || mFragmentEnd > duration) ? duration : mFragmentEnd;
  return NS_OK;
}
