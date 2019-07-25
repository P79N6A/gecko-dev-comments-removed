





































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
#include "plbase64.h"
#include "nsNetUtil.h"
#include "prmem.h"
#include "nsNetUtil.h"
#include "nsXPCOMStrings.h"
#include "prlock.h"
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
  PRBool IsCancelled() {
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
    return nsContentUtils::DispatchTrustedEvent(mElement->GetOwnerDoc(),
                                                mSource,
                                                NS_LITERAL_STRING("error"),
                                                PR_FALSE,
                                                PR_TRUE);
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

  
  nsresult rv;
  nsresult status;
  rv = aRequest->GetStatus(&status);
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
NS_IMPL_STRING_ATTR(nsHTMLMediaElement, Preload, preload)


NS_IMETHODIMP nsHTMLMediaElement::GetMozAutoplayEnabled(PRBool *aAutoplayEnabled)
{
  *aAutoplayEnabled = mAutoplayEnabled;

  return NS_OK;
}


NS_IMETHODIMP nsHTMLMediaElement::GetError(nsIDOMMediaError * *aError)
{
  NS_IF_ADDREF(*aError = mError);

  return NS_OK;
}


NS_IMETHODIMP nsHTMLMediaElement::GetEnded(PRBool *aEnded)
{
  *aEnded = mDecoder ? mDecoder->IsEnded() : PR_FALSE;

  return NS_OK;
}


NS_IMETHODIMP nsHTMLMediaElement::GetCurrentSrc(nsAString & aCurrentSrc)
{
  nsCAutoString src;

  if (mDecoder) {
    nsMediaStream* stream = mDecoder->GetCurrentStream();
    if (stream) {
      stream->URI()->GetSpec(src);
    }
  } else if (mLoadingSrc) {
    mLoadingSrc->GetSpec(src);
  }

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
 
    nsresult rv = http->SetRequestHeader(rangeHdr, rangeVal, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }
 
  return NS_OK;
}

void nsHTMLMediaElement::AbortExistingLoads()
{
  
  mLoadWaitStatus = NOT_WAITING;

  
  
  mCurrentLoadID++;

  PRBool fireTimeUpdate = PR_FALSE;
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
  mLoadedFirstFrame = PR_FALSE;
  mAutoplaying = PR_TRUE;
  mIsLoadingFromSourceChildren = PR_FALSE;
  mSuspendedAfterFirstFrame = PR_FALSE;
  mAllowSuspendAfterFirstFrame = PR_TRUE;
  mSourcePointer = nsnull;

  

  if (mNetworkState != nsIDOMHTMLMediaElement::NETWORK_EMPTY) {
    mNetworkState = nsIDOMHTMLMediaElement::NETWORK_EMPTY;
    ChangeReadyState(nsIDOMHTMLMediaElement::HAVE_NOTHING);
    mPaused = PR_TRUE;

    if (fireTimeUpdate) {
      
      
      
      
      FireTimeUpdate(PR_FALSE);
    }
    DispatchEvent(NS_LITERAL_STRING("emptied"));
  }

  
  
  AddRemoveSelfReference();

  mIsRunningSelectResource = PR_FALSE;
}

void nsHTMLMediaElement::NoSupportedMediaSourceError()
{
  NS_ASSERTION(mDelayingLoadEvent, "Load event not delayed during source selection?");

  mError = new nsMediaError(nsIDOMMediaError::MEDIA_ERR_SRC_NOT_SUPPORTED);
  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_NO_SOURCE;
  DispatchAsyncEvent(NS_LITERAL_STRING("error"));
  
  ChangeDelayLoadStatus(PR_FALSE);
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
  ChangeDelayLoadStatus(PR_TRUE);
  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_LOADING;
  AsyncAwaitStableState(this, &nsHTMLMediaElement::LoadFromSourceChildren);
}

void nsHTMLMediaElement::QueueSelectResourceTask()
{
  
  if (mIsRunningSelectResource)
    return;
  mIsRunningSelectResource = PR_TRUE;
  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_NO_SOURCE;
  AsyncAwaitStableState(this, &nsHTMLMediaElement::SelectResource);
}


NS_IMETHODIMP nsHTMLMediaElement::Load()
{
  if (mIsRunningLoadMethod)
    return NS_OK;
  SetPlayedOrSeeked(PR_FALSE);
  mIsRunningLoadMethod = PR_TRUE;
  AbortExistingLoads();
  QueueSelectResourceTask();
  mIsRunningLoadMethod = PR_FALSE;
  return NS_OK;
}

static PRBool HasSourceChildren(nsIContent *aElement)
{
  PRUint32 count = aElement->GetChildCount();
  for (PRUint32 i = 0; i < count; ++i) {
    nsIContent* child = aElement->GetChildAt(i);
    NS_ASSERTION(child, "GetChildCount lied!");
    if (child &&
        child->Tag() == nsGkAtoms::source &&
        child->IsHTML())
    {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}


static PRBool HasPotentialResource(nsIContent *aElement)
{
  nsAutoString src;
  if (aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::src, src))
    return PR_TRUE;
  return HasSourceChildren(aElement);
}

void nsHTMLMediaElement::SelectResource()
{
  if (!HasPotentialResource(this)) {
    
    
    mNetworkState = nsIDOMHTMLMediaElement::NETWORK_EMPTY;
    
    ChangeDelayLoadStatus(PR_FALSE);
    mIsRunningSelectResource = PR_FALSE;
    return;
  }

  ChangeDelayLoadStatus(PR_TRUE);

  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_LOADING;
  
  
  DispatchAsyncEvent(NS_LITERAL_STRING("loadstart"));

  nsAutoString src;
  nsCOMPtr<nsIURI> uri;

  
  if (GetAttr(kNameSpaceID_None, nsGkAtoms::src, src)) {
    nsresult rv = NewURIFromString(src, getter_AddRefs(uri));
    if (NS_SUCCEEDED(rv)) {
      LOG(PR_LOG_DEBUG, ("%p Trying load from src=%s", this, NS_ConvertUTF16toUTF8(src).get()));
      NS_ASSERTION(!mIsLoadingFromSourceChildren,
        "Should think we're not loading from source children by default");
      mLoadingSrc = uri;
      if (mPreloadAction == nsHTMLMediaElement::PRELOAD_NONE) {
        
        
        SuspendLoad(uri);
        mIsRunningSelectResource = PR_FALSE;
        return;
      }

      rv = LoadResource(uri);
      if (NS_SUCCEEDED(rv)) {
        mIsRunningSelectResource = PR_FALSE;
        return;
      }
    }
    NoSupportedMediaSourceError();
  } else {
    
    mIsLoadingFromSourceChildren = PR_TRUE;
    LoadFromSourceChildren();
  }
  mIsRunningSelectResource = PR_FALSE;
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
      mAllowAudioData = PR_FALSE;
    }
  }

  DispatchAudioAvailableEvent(frameBuffer.forget(), aFrameBufferLength, aTime);
}

PRBool nsHTMLMediaElement::MayHaveAudioAvailableEventListener()
{
  
  
  
  nsIDocument *document = GetDocument();
  if (!document) {
    return PR_TRUE;
  }

  nsPIDOMWindow *window = document->GetInnerWindow();
  if (!window) {
    return PR_TRUE;
  }

  return window->HasAudioAvailableEventListeners();
}

void nsHTMLMediaElement::LoadFromSourceChildren()
{
  NS_ASSERTION(mDelayingLoadEvent,
               "Should delay load event (if in document) during load");
  NS_ASSERTION(mIsLoadingFromSourceChildren,
               "Must remember we're loading from source children");
  while (PR_TRUE) {
    nsresult rv;
    nsIContent* child = GetNextSource();
    if (!child) {
      
      
      mLoadWaitStatus = WAITING_FOR_SOURCE;
      mNetworkState = nsIDOMHTMLMediaElement::NETWORK_NO_SOURCE;
      ChangeDelayLoadStatus(PR_FALSE);
      return;
    }

    nsCOMPtr<nsIURI> uri;
    nsAutoString src,type;

    
    if (!child->GetAttr(kNameSpaceID_None, nsGkAtoms::src, src)) {
      DispatchAsyncSourceError(child);
      continue;
    }

    
    if (child->HasAttr(kNameSpaceID_None, nsGkAtoms::type) &&
        child->GetAttr(kNameSpaceID_None, nsGkAtoms::type, type) &&
        GetCanPlay(type) == CANPLAY_NO)
    {
      DispatchAsyncSourceError(child);
      continue;
    }
    LOG(PR_LOG_DEBUG, ("%p Trying load from <source>=%s type=%s", this,
      NS_ConvertUTF16toUTF8(src).get(), NS_ConvertUTF16toUTF8(type).get()));
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

    rv = LoadResource(uri);
    if (NS_SUCCEEDED(rv))
      return;

    
    DispatchAsyncSourceError(child);
  }
  NS_NOTREACHED("Execution should not reach here!");
}

void nsHTMLMediaElement::SuspendLoad(nsIURI* aURI)
{
  mLoadIsSuspended = PR_TRUE;
  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_IDLE;
  DispatchAsyncEvent(NS_LITERAL_STRING("suspend"));
  ChangeDelayLoadStatus(PR_FALSE);
}

void nsHTMLMediaElement::ResumeLoad(PreloadAction aAction)
{
  NS_ASSERTION(mLoadIsSuspended, "Can only resume preload if halted for one");
  nsCOMPtr<nsIURI> uri = mLoadingSrc;
  mLoadIsSuspended = PR_FALSE;
  mPreloadAction = aAction;
  ChangeDelayLoadStatus(PR_TRUE);
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

static PRBool IsAutoplayEnabled()
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
    
    mAllowSuspendAfterFirstFrame = PR_TRUE;
    if (mLoadIsSuspended) {
      
      
      
      
      ResumeLoad(PRELOAD_METADATA);
    }
  }

  return;
}

nsresult nsHTMLMediaElement::LoadResource(nsIURI* aURI)
{
  NS_ASSERTION(mDelayingLoadEvent,
               "Should delay load event (if in document) during load");
  nsresult rv;

  
  
  if (mAudioStream) {
    mAudioStream->Shutdown();
    mAudioStream = nsnull;
  }

  if (mChannel) {
    mChannel->Cancel(NS_BINDING_ABORTED);
    mChannel = nsnull;
  }

  PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_MEDIA,
                                 aURI,
                                 NodePrincipal(),
                                 static_cast<nsGenericElement*>(this),
                                 EmptyCString(), 
                                 nsnull, 
                                 &shouldLoad,
                                 nsContentUtils::GetContentPolicy(),
                                 nsContentUtils::GetSecurityManager());
  NS_ENSURE_SUCCESS(rv,rv);
  if (NS_CP_REJECTED(shouldLoad)) return NS_ERROR_FAILURE;

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
                              PR_FALSE,
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
                         PR_FALSE);

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

  ChangeDelayLoadStatus(PR_TRUE);

  nsresult rv = InitializeDecoderForChannel(aChannel, aListener);
  if (NS_FAILED(rv)) {
    ChangeDelayLoadStatus(PR_FALSE);
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

  ChangeDelayLoadStatus(PR_TRUE);

  nsresult rv = InitializeDecoderAsClone(other->mDecoder);
  if (NS_FAILED(rv)) {
    ChangeDelayLoadStatus(PR_FALSE);
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


NS_IMETHODIMP nsHTMLMediaElement::GetSeeking(PRBool *aSeeking)
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


NS_IMETHODIMP nsHTMLMediaElement::GetPaused(PRBool *aPaused)
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

  PRBool oldPaused = mPaused;
  mPaused = PR_TRUE;
  mAutoplaying = PR_FALSE;
  
  AddRemoveSelfReference();

  if (!oldPaused) {
    FireTimeUpdate(PR_FALSE);
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
  if (aVolume < 0.0f || aVolume > 1.0f)
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


NS_IMETHODIMP nsHTMLMediaElement::GetMuted(PRBool *aMuted)
{
  *aMuted = mMuted;

  return NS_OK;
}

NS_IMETHODIMP nsHTMLMediaElement::SetMuted(PRBool aMuted)
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

nsHTMLMediaElement::nsHTMLMediaElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                                       FromParser aFromParser)
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
    mAllowAudioData(PR_FALSE),
    mBegun(PR_FALSE),
    mLoadedFirstFrame(PR_FALSE),
    mAutoplaying(PR_TRUE),
    mAutoplayEnabled(PR_TRUE),
    mPaused(PR_TRUE),
    mMuted(PR_FALSE),
    mPlayingBeforeSeek(PR_FALSE),
    mPausedForInactiveDocument(PR_FALSE),
    mWaitingFired(PR_FALSE),
    mIsRunningLoadMethod(PR_FALSE),
    mIsLoadingFromSourceChildren(PR_FALSE),
    mDelayingLoadEvent(PR_FALSE),
    mIsRunningSelectResource(PR_FALSE),
    mSuspendedAfterFirstFrame(PR_FALSE),
    mAllowSuspendAfterFirstFrame(PR_TRUE),
    mHasPlayedOrSeeked(PR_FALSE),
    mHasSelfReference(PR_FALSE),
    mShuttingDown(PR_FALSE),
    mLoadIsSuspended(PR_FALSE),
    mMediaSecurityVerified(PR_FALSE)
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
  mAllowSuspendAfterFirstFrame = PR_FALSE;
  if (!mSuspendedAfterFirstFrame)
    return;
  mSuspendedAfterFirstFrame = PR_FALSE;
  if (mDecoder) {
    mDecoder->Resume(PR_TRUE);
  }
}

void nsHTMLMediaElement::SetPlayedOrSeeked(PRBool aValue)
{
  if (aValue == mHasPlayedOrSeeked)
    return;

  mHasPlayedOrSeeked = aValue;

  
  nsIFrame* frame = GetPrimaryFrame();
  if (!frame) return;
  frame->PresContext()->PresShell()->FrameNeedsReflow(frame,
                                                      nsIPresShell::eTreeChange,
                                                      NS_FRAME_IS_DIRTY);
}

NS_IMETHODIMP nsHTMLMediaElement::Play()
{
  StopSuspendingAfterFirstFrame();
  SetPlayedOrSeeked(PR_TRUE);

  if (mNetworkState == nsIDOMHTMLMediaElement::NETWORK_EMPTY) {
    nsresult rv = Load();
    NS_ENSURE_SUCCESS(rv, rv);
  }  else if (mLoadIsSuspended) {
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
      FireTimeUpdate(PR_FALSE);
      DispatchAsyncEvent(NS_LITERAL_STRING("waiting"));
      break;
    case nsIDOMHTMLMediaElement::HAVE_FUTURE_DATA:
    case nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA:
      DispatchAsyncEvent(NS_LITERAL_STRING("playing"));
      break;
    }
  }

  mPaused = PR_FALSE;
  mAutoplaying = PR_FALSE;
  
  
  AddRemoveSelfReference();
  UpdatePreloadAction();

  return NS_OK;
}

PRBool nsHTMLMediaElement::ParseAttribute(PRInt32 aNamespaceID,
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
    if (aAttribute == nsGkAtoms::loopstart
       || aAttribute == nsGkAtoms::loopend
       || aAttribute == nsGkAtoms::start
       || aAttribute == nsGkAtoms::end) {
      return aResult.ParseDoubleValue(aValue);
    }
    else if (ParseImageAttribute(aAttribute, aValue, aResult)) {
      return PR_TRUE;
    }
    else if (aAttribute == nsGkAtoms::preload) {
      return aResult.ParseEnumValue(aValue, kPreloadTable, PR_FALSE);
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

nsresult nsHTMLMediaElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                     nsIAtom* aPrefix, const nsAString& aValue,
                                     PRBool aNotify)
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
                                       PRBool aNotify)
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
                                        PRBool aCompileEventHandlers)
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

void nsHTMLMediaElement::UnbindFromTree(PRBool aDeep,
                                        PRBool aNullParent)
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

static PRBool IsRawEnabled()
{
  return Preferences::GetBool("media.raw.enabled");
}

static PRBool IsRawType(const nsACString& aType)
{
  if (!IsRawEnabled())
    return PR_FALSE;
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(gRawTypes); ++i) {
    if (aType.EqualsASCII(gRawTypes[i]))
      return PR_TRUE;
  }
  return PR_FALSE;
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
  if (!IsOggEnabled())
    return PR_FALSE;
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(gOggTypes); ++i) {
    if (aType.EqualsASCII(gOggTypes[i]))
      return PR_TRUE;
  }
  return PR_FALSE;
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
  if (!IsWaveEnabled())
    return PR_FALSE;
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(gWaveTypes); ++i) {
    if (aType.EqualsASCII(gWaveTypes[i]))
      return PR_TRUE;
  }
  return PR_FALSE;
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
  if (!IsWebMEnabled())
    return PR_FALSE;
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(gWebMTypes); ++i) {
    if (aType.EqualsASCII(gWebMTypes[i]))
      return PR_TRUE;
  }
  return PR_FALSE;
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


PRBool nsHTMLMediaElement::ShouldHandleMediaType(const char* aMIMEType)
{
#ifdef MOZ_RAW
  if (IsRawType(nsDependentCString(aMIMEType)))
    return PR_TRUE;
#endif
#ifdef MOZ_OGG
  if (IsOggType(nsDependentCString(aMIMEType)))
    return PR_TRUE;
#endif
#ifdef MOZ_WEBM
  if (IsWebMType(nsDependentCString(aMIMEType)))
    return PR_TRUE;
#endif
  
  
  
  
  
  return PR_FALSE;
}

static PRBool
CodecListContains(char const *const * aCodecs, const nsAString& aCodec)
{
  for (PRInt32 i = 0; aCodecs[i]; ++i) {
    if (aCodec.EqualsASCII(aCodecs[i]))
      return PR_TRUE;
  }
  return PR_FALSE;
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
  PRBool expectMoreTokens = PR_FALSE;
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
  case CANPLAY_NO: aResult.AssignLiteral(""); break;
  case CANPLAY_YES: aResult.AssignLiteral("probably"); break;
  default:
  case CANPLAY_MAYBE: aResult.AssignLiteral("maybe"); break;
  }
  return NS_OK;
}

already_AddRefed<nsMediaDecoder>
nsHTMLMediaElement::CreateDecoder(const nsACString& aType)
{
#ifdef MOZ_RAW
  if (IsRawType(aType)) {
    nsRefPtr<nsRawDecoder> decoder = new nsRawDecoder();
    if (decoder && decoder->Init(this)) {
      return decoder.forget().get();
    }
  }
#endif
#ifdef MOZ_OGG
  if (IsOggType(aType)) {
    nsRefPtr<nsOggDecoder> decoder = new nsOggDecoder();
    if (decoder && decoder->Init(this)) {
      return decoder.forget().get();
    }
  }
#endif
#ifdef MOZ_WAVE
  if (IsWaveType(aType)) {
    nsRefPtr<nsWaveDecoder> decoder = new nsWaveDecoder();
    if (decoder && decoder->Init(this)) {
      return decoder.forget().get();
    }
  }
#endif
#ifdef MOZ_WEBM
  if (IsWebMType(aType)) {
    nsRefPtr<nsWebMDecoder> decoder = new nsWebMDecoder();
    if (decoder && decoder->Init(this)) {
      return decoder.forget().get();
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
    decoder->SetSeekable(aOriginal->GetSeekable());
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

  
  mMediaSecurityVerified = PR_FALSE;

  
  mPausedForInactiveDocument = PR_FALSE;
  
  
  NotifyOwnerDocumentActivityChanged();

  nsresult rv = NS_OK;

  mDecoder->SetVolume(mMuted ? 0.0 : mVolume);

  if (!mPaused) {
    SetPlayedOrSeeked(PR_TRUE);
    if (!mPausedForInactiveDocument) {
      rv = mDecoder->Play();
    }
  }

  mBegun = PR_TRUE;
  return rv;
}

nsresult nsHTMLMediaElement::NewURIFromString(const nsAutoString& aURISpec, nsIURI** aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);

  *aURI = nsnull;

  nsCOMPtr<nsIDocument> doc = GetOwnerDoc();
  if (!doc) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  nsCOMPtr<nsIURI> baseURI = GetBaseURI();
  nsresult rv = nsContentUtils::NewURIWithDocumentCharset(aURI,
                                                          aURISpec,
                                                          doc,
                                                          baseURI);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool equal;
  if (aURISpec.IsEmpty() &&
      doc->GetDocumentURI() &&
      NS_SUCCEEDED(doc->GetDocumentURI()->Equals(*aURI, &equal)) &&
      equal) {
    
    
    
    
    NS_RELEASE(*aURI);
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  return NS_OK;
}

void nsHTMLMediaElement::MetadataLoaded(PRUint32 aChannels, PRUint32 aRate)
{
  mChannels = aChannels;
  mRate = aRate;
  ChangeReadyState(nsIDOMHTMLMediaElement::HAVE_METADATA);
  DispatchAsyncEvent(NS_LITERAL_STRING("durationchange"));
  DispatchAsyncEvent(NS_LITERAL_STRING("loadedmetadata"));
}

void nsHTMLMediaElement::FirstFrameLoaded(PRBool aResourceFullyLoaded)
{
  ChangeReadyState(aResourceFullyLoaded ?
    nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA :
    nsIDOMHTMLMediaElement::HAVE_CURRENT_DATA);
  ChangeDelayLoadStatus(PR_FALSE);

  NS_ASSERTION(!mSuspendedAfterFirstFrame, "Should not have already suspended");

  if (mDecoder && mAllowSuspendAfterFirstFrame && mPaused &&
      !aResourceFullyLoaded &&
      !HasAttr(kNameSpaceID_None, nsGkAtoms::autoplay) &&
      mPreloadAction == nsHTMLMediaElement::PRELOAD_METADATA) {
    mSuspendedAfterFirstFrame = PR_TRUE;
    mDecoder->Suspend();
  }
}

void nsHTMLMediaElement::ResourceLoaded()
{
  mBegun = PR_FALSE;
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
  if (mIsLoadingFromSourceChildren) {
    if (mDecoder) {
      mDecoder->Shutdown();
      mDecoder = nsnull;
    }
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
  mBegun = PR_FALSE;
  DispatchAsyncEvent(NS_LITERAL_STRING("error"));
  if (mReadyState == nsIDOMHTMLMediaElement::HAVE_NOTHING) {
    mNetworkState = nsIDOMHTMLMediaElement::NETWORK_EMPTY;
    DispatchAsyncEvent(NS_LITERAL_STRING("emptied"));
  } else {
    mNetworkState = nsIDOMHTMLMediaElement::NETWORK_IDLE;
  }
  AddRemoveSelfReference();
  ChangeDelayLoadStatus(PR_FALSE);
}

void nsHTMLMediaElement::PlaybackEnded()
{
  NS_ASSERTION(mDecoder->IsEnded(), "Decoder fired ended, but not in ended state");
  
  AddRemoveSelfReference();

  FireTimeUpdate(PR_FALSE);
  DispatchAsyncEvent(NS_LITERAL_STRING("ended"));
}

void nsHTMLMediaElement::SeekStarted()
{
  DispatchAsyncEvent(NS_LITERAL_STRING("seeking"));
  FireTimeUpdate(PR_FALSE);
}

void nsHTMLMediaElement::SeekCompleted()
{
  mPlayingBeforeSeek = PR_FALSE;
  SetPlayedOrSeeked(PR_TRUE);
  DispatchAsyncEvent(NS_LITERAL_STRING("seeked"));
  
  AddRemoveSelfReference();
}

void nsHTMLMediaElement::DownloadSuspended()
{
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

PRBool nsHTMLMediaElement::ShouldCheckAllowOrigin()
{
  return Preferences::GetBool("media.enforce_same_site_origin", PR_TRUE);
}

void nsHTMLMediaElement::UpdateReadyStateForData(NextFrameStatus aNextFrame)
{
  if (mReadyState < nsIDOMHTMLMediaElement::HAVE_METADATA) {
    
    
    
    
    return;
  }

  if (aNextFrame != NEXT_FRAME_AVAILABLE) {
    ChangeReadyState(nsIDOMHTMLMediaElement::HAVE_CURRENT_DATA);
    if (!mWaitingFired && aNextFrame == NEXT_FRAME_UNAVAILABLE_BUFFERING) {
      FireTimeUpdate(PR_FALSE);
      DispatchAsyncEvent(NS_LITERAL_STRING("waiting"));
      mWaitingFired = PR_TRUE;
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
    mLoadedFirstFrame = PR_TRUE;
  }

  if (mReadyState == nsIDOMHTMLMediaElement::HAVE_CURRENT_DATA) {
    mWaitingFired = PR_FALSE;
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

PRBool nsHTMLMediaElement::CanActivateAutoplay()
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
    mPaused = PR_FALSE;
    
    AddRemoveSelfReference();

    if (mDecoder) {
      SetPlayedOrSeeked(PR_TRUE);
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

  
  nsCOMPtr<nsIDOMHTMLVideoElement> video =
    do_QueryInterface(static_cast<nsIContent*>(this));
  if (!video)
    return nsnull;

  nsRefPtr<LayerManager> manager =
    nsContentUtils::PersistentLayerManagerForDocument(GetOwnerDoc());
  if (!manager)
    return nsnull;

  mImageContainer = manager->CreateImageContainer();
  if (manager->IsCompositingCheap()) {
    mImageContainer->SetDelayedConversion(PR_TRUE);
  }
  return mImageContainer;
}

nsresult nsHTMLMediaElement::DispatchAudioAvailableEvent(float* aFrameBuffer,
                                                         PRUint32 aFrameBufferLength,
                                                         float aTime)
{
  
  
  
  
  nsAutoArrayPtr<float> frameBuffer(aFrameBuffer);

  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(GetOwnerDoc());
  nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(static_cast<nsIContent*>(this)));
  NS_ENSURE_TRUE(domDoc && target, NS_ERROR_INVALID_ARG);

  nsCOMPtr<nsIDOMEvent> event;
  nsresult rv = domDoc->CreateEvent(NS_LITERAL_STRING("MozAudioAvailableEvent"),
                                    getter_AddRefs(event));
  nsCOMPtr<nsIDOMNotifyAudioAvailableEvent> audioavailableEvent(do_QueryInterface(event));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = audioavailableEvent->InitAudioAvailableEvent(NS_LITERAL_STRING("MozAudioAvailable"),
                                                    PR_TRUE, PR_TRUE, frameBuffer.forget(), aFrameBufferLength,
                                                    aTime, mAllowAudioData);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool dummy;
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

  return nsContentUtils::DispatchTrustedEvent(GetOwnerDoc(),
                                              static_cast<nsIContent*>(this),
                                              aName,
                                              PR_FALSE,
                                              PR_TRUE);
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

PRBool nsHTMLMediaElement::IsPotentiallyPlaying() const
{
  
  
  
  return
    !mPaused &&
    (mReadyState == nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA ||
    mReadyState == nsIDOMHTMLMediaElement::HAVE_FUTURE_DATA) &&
    !IsPlaybackEnded();
}

PRBool nsHTMLMediaElement::IsPlaybackEnded() const
{
  
  
  
  return mNetworkState >= nsIDOMHTMLMediaElement::HAVE_METADATA &&
    mDecoder ? mDecoder->IsEnded() : PR_FALSE;
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
  nsIDocument* ownerDoc = GetOwnerDoc();
  
  
  PRBool pauseForInactiveDocument =
    ownerDoc && (!ownerDoc->IsActive() || !ownerDoc->IsVisible());

  if (pauseForInactiveDocument != mPausedForInactiveDocument) {
    mPausedForInactiveDocument = pauseForInactiveDocument;
    if (mDecoder) {
      if (pauseForInactiveDocument) {
        mDecoder->Pause();
        mDecoder->Suspend();
      } else {
        mDecoder->Resume(PR_FALSE);
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
  
  
  
  
  
  nsIDocument* ownerDoc = GetOwnerDoc();

  
  
  PRBool needSelfReference = !mShuttingDown &&
    (!ownerDoc || ownerDoc->IsActive()) &&
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
    mShuttingDown = PR_TRUE;
    AddRemoveSelfReference();
  }
  return NS_OK;
}

PRBool
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
  nsresult rv = NS_OK;
  nsCOMPtr<nsIDOMNode> thisDomNode =
    do_QueryInterface(static_cast<nsGenericElement*>(this));

  mSourceLoadCandidate = nsnull;

  if (!mSourcePointer) {
    
    mSourcePointer = do_CreateInstance("@mozilla.org/content/range;1");

    rv = mSourcePointer->SelectNodeContents(thisDomNode);
    if (NS_FAILED(rv)) return nsnull;

    rv = mSourcePointer->Collapse(PR_TRUE);
    if (NS_FAILED(rv)) return nsnull;
  }

  while (PR_TRUE) {
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

    
    rv = mSourcePointer->SetStart(thisDomNode, startOffset+1);
    NS_ENSURE_SUCCESS(rv, nsnull);

    nsIContent* child = GetChildAt(startOffset);

    
    if (child &&
        child->Tag() == nsGkAtoms::source &&
        child->IsHTML())
    {
      mSourceLoadCandidate = child;
      return child;
    }
  }
  NS_NOTREACHED("Execution should not reach here!");
  return nsnull;
}

void nsHTMLMediaElement::ChangeDelayLoadStatus(PRBool aDelay) {
  if (mDelayingLoadEvent == aDelay)
    return;

  mDelayingLoadEvent = aDelay;

  if (aDelay) {
    mLoadBlockedDoc = GetOwnerDoc();
    mLoadBlockedDoc->BlockOnload();
    LOG(PR_LOG_DEBUG, ("%p ChangeDelayLoadStatus(%d) doc=0x%p", this, aDelay, mLoadBlockedDoc.get()));
  } else {
    if (mDecoder) {
      mDecoder->MoveLoadsToBackground();
    }
    LOG(PR_LOG_DEBUG, ("%p ChangeDelayLoadStatus(%d) doc=0x%p", this, aDelay, mLoadBlockedDoc.get()));
    
    if (mLoadBlockedDoc) {
      mLoadBlockedDoc->UnblockOnload(PR_FALSE);
      mLoadBlockedDoc = nsnull;
    }
  }

  
  AddRemoveSelfReference();
}

already_AddRefed<nsILoadGroup> nsHTMLMediaElement::GetDocumentLoadGroup()
{
  nsIDocument* doc = GetOwnerDoc();
  return doc ? doc->GetDocumentLoadGroup() : nsnull;
}

nsresult
nsHTMLMediaElement::CopyInnerTo(nsGenericElement* aDest) const
{
  nsresult rv = nsGenericHTMLElement::CopyInnerTo(aDest);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aDest->GetOwnerDoc()->IsStaticDocument()) {
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
  nsTimeRanges* ranges = new nsTimeRanges();
  NS_ADDREF(*aBuffered = ranges);
  if (mReadyState >= nsIDOMHTMLMediaElement::HAVE_CURRENT_DATA && mDecoder) {
    
    
    mDecoder->GetBuffered(ranges);
  }
  return NS_OK;
}

void nsHTMLMediaElement::SetRequestHeaders(nsIHttpChannel* aChannel)
{
  
  SetAcceptHeader(aChannel);

  
  
  
  
  
  aChannel->SetRequestHeader(NS_LITERAL_CSTRING("Accept-Encoding"),
                             NS_LITERAL_CSTRING(""), PR_FALSE);

  
  nsIDocument* doc = GetOwnerDoc();
  if (doc) {
    aChannel->SetReferrer(doc->GetDocumentURI());
  }
}

void nsHTMLMediaElement::FireTimeUpdate(PRBool aPeriodic)
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
}
