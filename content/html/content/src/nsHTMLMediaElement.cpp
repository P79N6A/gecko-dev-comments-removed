





































#include "nsIDOMHTMLMediaElement.h"
#include "nsIDOMHTMLSourceElement.h"
#include "nsHTMLMediaElement.h"
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
#include "nsContentUtils.h"

#include "nsIScriptSecurityManager.h"
#include "nsIXPConnect.h"
#include "jsapi.h"

#include "nsIRenderingContext.h"
#include "nsITimer.h"

#include "nsEventDispatcher.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMProgressEvent.h"
#include "nsHTMLMediaError.h"
#include "nsICategoryManager.h"
#include "nsCommaSeparatedTokenizer.h"
#include "nsMediaStream.h"

#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsContentErrors.h"
#include "nsCrossSiteListenerProxy.h"
#include "nsCycleCollectionParticipant.h"

#ifdef MOZ_OGG
#include "nsOggDecoder.h"
#endif
#ifdef MOZ_WAVE
#include "nsWaveDecoder.h"
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

  nsCOMPtr<nsHTMLMediaElement> mElement;
  PRUint32 mLoadID;
};


class nsAsyncEventRunner : public nsMediaEvent
{
private:
  nsString mName;
  PRPackedBool mProgress;
  
public:
  nsAsyncEventRunner(const nsAString& aName, nsHTMLMediaElement* aElement, PRBool aProgress) : 
    nsMediaEvent(aElement), mName(aName), mProgress(aProgress)
  {
  }
  
  NS_IMETHOD Run() {
    
    if (IsCancelled())
      return NS_OK;
    return mProgress ?
      mElement->DispatchProgressEvent(mName) :
      mElement->DispatchSimpleEvent(mName);
  }
};

class nsHTMLMediaElement::LoadNextSourceEvent : public nsMediaEvent {
public:
  LoadNextSourceEvent(nsHTMLMediaElement *aElement)
    : nsMediaEvent(aElement) {}
  NS_IMETHOD Run() {
    if (!IsCancelled())
      mElement->LoadFromSourceChildren();
    return NS_OK;
  }
};

class nsHTMLMediaElement::SelectResourceEvent : public nsMediaEvent {
public:
  SelectResourceEvent(nsHTMLMediaElement *aElement)
    : nsMediaEvent(aElement) {}
  NS_IMETHOD Run() {
    if (!IsCancelled()) {
      NS_ASSERTION(mElement->mIsRunningSelectResource,
                   "Should have flagged that we're running SelectResource()");
      mElement->SelectResource();
      mElement->mIsRunningSelectResource = PR_FALSE;
    }
    return NS_OK;
  }
};

void nsHTMLMediaElement::QueueSelectResourceTask()
{
  
  if (mIsRunningSelectResource)
    return;
  mIsRunningSelectResource = PR_TRUE;
  ChangeDelayLoadStatus(PR_TRUE);
  nsCOMPtr<nsIRunnable> event = new SelectResourceEvent(this);
  NS_DispatchToMainThread(event);
}

void nsHTMLMediaElement::QueueLoadFromSourceTask()
{
  ChangeDelayLoadStatus(PR_TRUE);
  nsCOMPtr<nsIRunnable> event = new LoadNextSourceEvent(this);
  NS_DispatchToMainThread(event);
}







class nsHTMLMediaElement::MediaLoadListener : public nsIStreamListener,
                                              public nsIChannelEventSink,
                                              public nsIInterfaceRequestor
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIINTERFACEREQUESTOR

public:
  MediaLoadListener(nsHTMLMediaElement* aElement)
    : mElement(aElement)
  {
    NS_ABORT_IF_FALSE(mElement, "Must pass an element to call back");
  }

private:
  nsRefPtr<nsHTMLMediaElement> mElement;
  nsCOMPtr<nsIStreamListener> mNextListener;
};

NS_IMPL_ISUPPORTS4(nsHTMLMediaElement::MediaLoadListener, nsIRequestObserver,
                   nsIStreamListener, nsIChannelEventSink,
                   nsIInterfaceRequestor)

NS_IMETHODIMP nsHTMLMediaElement::MediaLoadListener::OnStartRequest(nsIRequest* aRequest, nsISupports* aContext)
{
  
  
  nsRefPtr<nsHTMLMediaElement> element;
  element.swap(mElement);

  
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

NS_IMETHODIMP nsHTMLMediaElement::MediaLoadListener::OnChannelRedirect(nsIChannel* aOldChannel,
                                                                       nsIChannel* aNewChannel,
                                                                       PRUint32 aFlags)
{
  if (mElement)
    mElement->OnChannelRedirect(aOldChannel, aNewChannel, aFlags);
  nsCOMPtr<nsIChannelEventSink> sink = do_QueryInterface(mNextListener);
  if (sink)
    return sink->OnChannelRedirect(aOldChannel, aNewChannel, aFlags);
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
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsHTMLMediaElement, nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mSourcePointer)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsHTMLMediaElement)
NS_INTERFACE_MAP_END_INHERITING(nsGenericHTMLElement)


NS_IMPL_URI_ATTR(nsHTMLMediaElement, Src, src)
NS_IMPL_BOOL_ATTR(nsHTMLMediaElement, Controls, controls)
NS_IMPL_BOOL_ATTR(nsHTMLMediaElement, Autoplay, autoplay)
NS_IMPL_BOOL_ATTR(nsHTMLMediaElement, Autobuffer, autobuffer)


NS_IMETHODIMP nsHTMLMediaElement::GetMozAutoplayEnabled(PRBool *aAutoplayEnabled)
{
  *aAutoplayEnabled = mAutoplayEnabled;

  return NS_OK;
}


NS_IMETHODIMP nsHTMLMediaElement::GetError(nsIDOMHTMLMediaError * *aError)
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
  return NS_OK;
}

void nsHTMLMediaElement::AbortExistingLoads()
{
  
  mLoadWaitStatus = NOT_WAITING;

  
  
  mCurrentLoadID++;

  if (mDecoder) {
    mDecoder->Shutdown();
    mDecoder = nsnull;
  }

  if (mNetworkState == nsIDOMHTMLMediaElement::NETWORK_LOADING ||
      mNetworkState == nsIDOMHTMLMediaElement::NETWORK_IDLE)
  {
    mError = new nsHTMLMediaError(nsIDOMHTMLMediaError::MEDIA_ERR_ABORTED);
    DispatchProgressEvent(NS_LITERAL_STRING("abort"));
  }

  mError = nsnull;
  mLoadedFirstFrame = PR_FALSE;
  mAutoplaying = PR_TRUE;
  mIsLoadingFromSrcAttribute = PR_FALSE;
  mSuspendedAfterFirstFrame = PR_FALSE;
  mAllowSuspendAfterFirstFrame = PR_TRUE;

  

  if (mNetworkState != nsIDOMHTMLMediaElement::NETWORK_EMPTY) {
    mNetworkState = nsIDOMHTMLMediaElement::NETWORK_EMPTY;
    ChangeReadyState(nsIDOMHTMLMediaElement::HAVE_NOTHING);
    mPaused = PR_TRUE;

    
    DispatchSimpleEvent(NS_LITERAL_STRING("emptied"));
  }

  mIsRunningSelectResource = PR_FALSE;
}

void nsHTMLMediaElement::NoSupportedMediaSourceError()
{
  mError = new nsHTMLMediaError(nsIDOMHTMLMediaError::MEDIA_ERR_SRC_NOT_SUPPORTED);
  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_NO_SOURCE;
  DispatchAsyncProgressEvent(NS_LITERAL_STRING("error"));
  ChangeDelayLoadStatus(PR_FALSE);
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
    
    
    mNetworkState = nsIDOMHTMLMediaElement::NETWORK_NO_SOURCE;
    mLoadWaitStatus = WAITING_FOR_SRC_OR_SOURCE;
    ChangeDelayLoadStatus(PR_FALSE);
    return;
  }

  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_LOADING;
  DispatchAsyncProgressEvent(NS_LITERAL_STRING("loadstart"));

  nsAutoString src;
  nsCOMPtr<nsIURI> uri;

  
  if (GetAttr(kNameSpaceID_None, nsGkAtoms::src, src)) {
    nsresult rv = NewURIFromString(src, getter_AddRefs(uri));
    if (NS_SUCCEEDED(rv)) {
      LOG(PR_LOG_DEBUG, ("%p Trying load from src=%s", this, NS_ConvertUTF16toUTF8(src).get()));  
      mIsLoadingFromSrcAttribute = PR_TRUE;
      rv = LoadResource(uri);
      if (NS_SUCCEEDED(rv))
        return;
    }
    NoSupportedMediaSourceError();
  } else {
    
    LoadFromSourceChildren();
  }
}

void nsHTMLMediaElement::NotifyLoadError()
{
  if (mIsLoadingFromSrcAttribute) {
    NoSupportedMediaSourceError();
  } else {
    QueueLoadFromSourceTask();
  }
}

void nsHTMLMediaElement::LoadFromSourceChildren()
{
  NS_ASSERTION(!IsInDoc() || mDelayingLoadEvent,
               "Should delay load event while loading in document");
  while (PR_TRUE) {
    nsresult rv;
    nsCOMPtr<nsIURI> uri = GetNextSource();
    if (!uri) {
      
      
      mLoadWaitStatus = WAITING_FOR_SOURCE;
      NoSupportedMediaSourceError();
      return;
    }

    mNetworkState = nsIDOMHTMLMediaElement::NETWORK_LOADING;
  
    rv = LoadResource(uri);
    if (NS_SUCCEEDED(rv))
      return;

    
  }
  NS_NOTREACHED("Execution should not reach here!");
}

nsresult nsHTMLMediaElement::LoadResource(nsIURI* aURI)
{
  NS_ASSERTION(!IsInDoc() || mDelayingLoadEvent,
               "Should delay load event while loading in document");
  nsresult rv;

  if (mChannel) {
    mChannel->Cancel(NS_BINDING_ABORTED);
    mChannel = nsnull;
  }

  PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_MEDIA,
                                 aURI,
                                 NodePrincipal(),
                                 this,
                                 EmptyCString(), 
                                 nsnull, 
                                 &shouldLoad,
                                 nsContentUtils::GetContentPolicy(),
                                 nsContentUtils::GetSecurityManager());
  NS_ENSURE_SUCCESS(rv,rv);
  if (NS_CP_REJECTED(shouldLoad)) return NS_ERROR_FAILURE;

  nsCOMPtr<nsILoadGroup> loadGroup = GetDocumentLoadGroup();
  rv = NS_NewChannel(getter_AddRefs(mChannel),
                     aURI,
                     nsnull,
                     loadGroup,
                     nsnull,
                     nsIRequest::LOAD_NORMAL);
  NS_ENSURE_SUCCESS(rv,rv);

  
  
  
  nsRefPtr<MediaLoadListener> loadListener = new MediaLoadListener(this);
  if (!loadListener) return NS_ERROR_OUT_OF_MEMORY;
  
  mChannel->SetNotificationCallbacks(loadListener);

  nsCOMPtr<nsIStreamListener> listener;
  if (ShouldCheckAllowOrigin()) {
    listener = new nsCrossSiteListenerProxy(loadListener,
                                            NodePrincipal(),
                                            mChannel, 
                                            PR_FALSE,
                                            &rv);
    NS_ENSURE_SUCCESS(rv,rv);
    if (!listener) return NS_ERROR_OUT_OF_MEMORY;
  } else {
    rv = nsContentUtils::GetSecurityManager()->
           CheckLoadURIWithPrincipal(NodePrincipal(),
                                     aURI,
                                     nsIScriptSecurityManager::STANDARD);
    NS_ENSURE_SUCCESS(rv,rv);
    listener = loadListener;
  }

  nsCOMPtr<nsIHttpChannel> hc = do_QueryInterface(mChannel);
  if (hc) {
    
    
    
    hc->SetRequestHeader(NS_LITERAL_CSTRING("Range"),
                         NS_LITERAL_CSTRING("bytes=0-"),
                         PR_FALSE);
  }

  rv = mChannel->AsyncOpen(listener, nsnull);
  if (NS_FAILED(rv)) {
    
    
    
    
    
    
    mChannel = nsnull;
    return rv;
  }

  
  
  
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

  DispatchAsyncProgressEvent(NS_LITERAL_STRING("loadstart"));

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

  DispatchAsyncProgressEvent(NS_LITERAL_STRING("loadstart"));

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


NS_IMETHODIMP nsHTMLMediaElement::GetCurrentTime(float *aCurrentTime)
{
  *aCurrentTime = mDecoder ? mDecoder->GetCurrentTime() : 0.0;
  return NS_OK;
}

NS_IMETHODIMP nsHTMLMediaElement::SetCurrentTime(float aCurrentTime)
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

  
  float clampedTime = PR_MAX(0, aCurrentTime);
  float duration = mDecoder->GetDuration();
  if (duration >= 0) {
    clampedTime = PR_MIN(clampedTime, duration);
  }

  mPlayingBeforeSeek = IsPotentiallyPlaying();
  
  
  LOG(PR_LOG_DEBUG, ("%p SetCurrentTime(%f) starting seek", this, aCurrentTime));  
  nsresult rv = mDecoder->Seek(clampedTime);
  return rv;
}


NS_IMETHODIMP nsHTMLMediaElement::GetDuration(float *aDuration)
{
  *aDuration =  mDecoder ? mDecoder->GetDuration() : 0.0;
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
    nsresult rv = Load();
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (mDecoder) {
    mDecoder->Pause();
  }

  PRBool oldPaused = mPaused;
  mPaused = PR_TRUE;
  mAutoplaying = PR_FALSE;
  
  if (!oldPaused) {
    DispatchAsyncSimpleEvent(NS_LITERAL_STRING("timeupdate"));
    DispatchAsyncSimpleEvent(NS_LITERAL_STRING("pause"));
  }

  return NS_OK;
}


NS_IMETHODIMP nsHTMLMediaElement::GetVolume(float *aVolume)
{
  *aVolume = mVolume;

  return NS_OK;
}

NS_IMETHODIMP nsHTMLMediaElement::SetVolume(float aVolume)
{
  if (aVolume < 0.0f || aVolume > 1.0f)
    return NS_ERROR_DOM_INDEX_SIZE_ERR;

  if (aVolume == mVolume)
    return NS_OK;

  mVolume = aVolume;

  if (mDecoder && !mMuted)
    mDecoder->SetVolume(mVolume);

  DispatchAsyncSimpleEvent(NS_LITERAL_STRING("volumechange"));

  return NS_OK;
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
  }

  DispatchAsyncSimpleEvent(NS_LITERAL_STRING("volumechange"));

  return NS_OK;
}

nsHTMLMediaElement::nsHTMLMediaElement(nsINodeInfo *aNodeInfo, PRBool aFromParser)
  : nsGenericHTMLElement(aNodeInfo),
    mCurrentLoadID(0),
    mNetworkState(nsIDOMHTMLMediaElement::NETWORK_EMPTY),
    mReadyState(nsIDOMHTMLMediaElement::HAVE_NOTHING),
    mLoadWaitStatus(NOT_WAITING),
    mVolume(1.0),
    mMediaSize(-1,-1),
    mBegun(PR_FALSE),
    mLoadedFirstFrame(PR_FALSE),
    mAutoplaying(PR_TRUE),
    mAutoplayEnabled(PR_TRUE),
    mPaused(PR_TRUE),
    mMuted(PR_FALSE),
    mIsDoneAddingChildren(!aFromParser),
    mPlayingBeforeSeek(PR_FALSE),
    mPausedBeforeFreeze(PR_FALSE),
    mWaitingFired(PR_FALSE),
    mIsBindingToTree(PR_FALSE),
    mIsRunningLoadMethod(PR_FALSE),
    mIsLoadingFromSrcAttribute(PR_FALSE),
    mDelayingLoadEvent(PR_FALSE),
    mIsRunningSelectResource(PR_FALSE),
    mSuspendedAfterFirstFrame(PR_FALSE),
    mAllowSuspendAfterFirstFrame(PR_TRUE),
    mHasPlayedOrSeeked(PR_FALSE)
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
}

nsHTMLMediaElement::~nsHTMLMediaElement()
{
  UnregisterFreezableElement();
  if (mDecoder) {
    mDecoder->Shutdown();
    mDecoder = nsnull;
  }
  if (mChannel) {
    mChannel->Cancel(NS_BINDING_ABORTED);
    mChannel = nsnull;
  }
}

void nsHTMLMediaElement::StopSuspendingAfterFirstFrame()
{
  mAllowSuspendAfterFirstFrame = PR_FALSE;
  if (!mSuspendedAfterFirstFrame)
    return;
  mSuspendedAfterFirstFrame = PR_FALSE;
  if (mDecoder) {
    mDecoder->Resume();
  }
}

void nsHTMLMediaElement::SetPlayedOrSeeked(PRBool aValue)
{
  if (aValue == mHasPlayedOrSeeked)
    return;

  mHasPlayedOrSeeked = aValue;

  
  nsIDocument *doc = GetDocument();
  if (!doc) return;
  nsIPresShell *presShell = doc->GetPrimaryShell();  
  if (!presShell) return;
  nsIFrame* frame = presShell->GetPrimaryFrameFor(this);
  if (!frame) return;
  presShell->FrameNeedsReflow(frame,
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
  } else if (mDecoder) {
    if (mDecoder->IsEnded()) {
      SetCurrentTime(0);
    }
    nsresult rv = mDecoder->Play();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  if (mPaused) {
    DispatchAsyncSimpleEvent(NS_LITERAL_STRING("play"));
    switch (mReadyState) {
    case nsIDOMHTMLMediaElement::HAVE_METADATA:
    case nsIDOMHTMLMediaElement::HAVE_CURRENT_DATA:
      DispatchAsyncSimpleEvent(NS_LITERAL_STRING("waiting"));
      break;
    case nsIDOMHTMLMediaElement::HAVE_FUTURE_DATA:
    case nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA:
      DispatchAsyncSimpleEvent(NS_LITERAL_STRING("playing"));
      break;
    }
  }

  mPaused = PR_FALSE;
  mAutoplaying = PR_FALSE;

  return NS_OK;
}

PRBool nsHTMLMediaElement::ParseAttribute(PRInt32 aNamespaceID,
                                          nsIAtom* aAttribute,
                                          const nsAString& aValue,
                                          nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::src) {
      static const char* kWhitespace = " \n\r\t\b";
      aResult.SetTo(nsContentUtils::TrimCharsInSet(kWhitespace, aValue));
      return PR_TRUE;
    }
    else if (aAttribute == nsGkAtoms::loopstart
            || aAttribute == nsGkAtoms::loopend
            || aAttribute == nsGkAtoms::start
            || aAttribute == nsGkAtoms::end) {
      return aResult.ParseFloatValue(aValue);
    }
    else if (ParseImageAttribute(aAttribute, aValue, aResult)) {
      return PR_TRUE;
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
  if (aNotify && aNameSpaceID == kNameSpaceID_None) {
    if (aName == nsGkAtoms::src) {
      if (mLoadWaitStatus == WAITING_FOR_SRC_OR_SOURCE) {
        
        
        
        mLoadWaitStatus = NOT_WAITING;
        QueueSelectResourceTask();
      }
    } else if (aName == nsGkAtoms::autoplay) {
      StopSuspendingAfterFirstFrame();
      if (mReadyState == nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA) {
        NotifyAutoplayDataReady();
      }
    } else if (aName == nsGkAtoms::autobuffer) {
      StopSuspendingAfterFirstFrame();
    }
  }

  return rv;
}

static PRBool IsAutoplayEnabled()
{
  return nsContentUtils::GetBoolPref("media.autoplay.enabled");
}

nsresult nsHTMLMediaElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                        nsIContent* aBindingParent,
                                        PRBool aCompileEventHandlers)
{
  mIsBindingToTree = PR_TRUE;
  mAutoplayEnabled = IsAutoplayEnabled();
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, 
                                                 aParent, 
                                                 aBindingParent, 
                                                 aCompileEventHandlers);
  if (NS_SUCCEEDED(rv) &&
      mIsDoneAddingChildren &&
      mNetworkState == nsIDOMHTMLMediaElement::NETWORK_EMPTY)
  {
    QueueSelectResourceTask();
  }

  mIsBindingToTree = PR_FALSE;

  return rv;
}

void nsHTMLMediaElement::UnbindFromTree(PRBool aDeep,
                                        PRBool aNullParent)
{
  if (!mPaused && mNetworkState != nsIDOMHTMLMediaElement::NETWORK_EMPTY)
    Pause();
  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
}

#ifdef MOZ_OGG


static const char gOggTypes[][16] = {
  "video/ogg",
  "audio/ogg",
  "application/ogg"
};

static const char* gOggCodecs[] = {
  "vorbis",
  "theora",
  nsnull
};

static PRBool IsOggEnabled()
{
  return nsContentUtils::GetBoolPref("media.ogg.enabled");
}

static PRBool IsOggType(const nsACString& aType)
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



static const char gWaveTypes[][16] = {
  "audio/x-wav",
  "audio/wav",
  "audio/wave",
  "audio/x-pn-wav"
};

static const char* gWaveCodecs[] = {
  "1", 
  nsnull
};

static PRBool IsWaveEnabled()
{
  return nsContentUtils::GetBoolPref("media.wave.enabled");
}

static PRBool IsWaveType(const nsACString& aType)
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


PRBool nsHTMLMediaElement::CanHandleMediaType(const char* aMIMEType,
                                              const char*** aCodecList)
{
#ifdef MOZ_OGG
  if (IsOggType(nsDependentCString(aMIMEType))) {
    *aCodecList = gOggCodecs;
    return PR_TRUE;
  }
#endif
#ifdef MOZ_WAVE
  if (IsWaveType(nsDependentCString(aMIMEType))) {
    *aCodecList = gWaveCodecs;
    return PR_TRUE;
  }
#endif
  return PR_FALSE;
}


PRBool nsHTMLMediaElement::ShouldHandleMediaType(const char* aMIMEType)
{
#ifdef MOZ_OGG
  if (IsOggType(nsDependentCString(aMIMEType)))
    return PR_TRUE;
#endif
  
  
  
  
  
  return PR_FALSE;
}

static PRBool
CodecListContains(const char** aCodecs, const nsAString& aCodec)
{
  for (PRInt32 i = 0; aCodecs[i]; ++i) {
    if (aCodec.EqualsASCII(aCodecs[i]))
      return PR_TRUE;
  }
  return PR_FALSE;
}

enum CanPlayStatus {
  CANPLAY_NO,
  CANPLAY_MAYBE,
  CANPLAY_YES
};

static CanPlayStatus GetCanPlay(const nsAString& aType)
{
  nsContentTypeParser parser(aType);
  nsAutoString mimeType;
  nsresult rv = parser.GetType(mimeType);
  if (NS_FAILED(rv))
    return CANPLAY_NO;

  NS_ConvertUTF16toUTF8 mimeTypeUTF8(mimeType);
  const char** supportedCodecs;
  if (!nsHTMLMediaElement::CanHandleMediaType(mimeTypeUTF8.get(),
                                              &supportedCodecs))
    return CANPLAY_NO;

  nsAutoString codecs;
  rv = parser.GetParameter("codecs", codecs);
  if (NS_FAILED(rv))
    
    return CANPLAY_MAYBE;

  CanPlayStatus result = CANPLAY_YES;
  
  
  nsCommaSeparatedTokenizer tokenizer(codecs);
  PRBool expectMoreTokens = PR_FALSE;
  while (tokenizer.hasMoreTokens()) {
    const nsSubstring& token = tokenizer.nextToken();

    if (!CodecListContains(supportedCodecs, token)) {
      
      return CANPLAY_NO;
    }
    expectMoreTokens = tokenizer.lastTokenEndedWithComma();
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


void nsHTMLMediaElement::InitMediaTypes()
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catMan(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
  if (NS_SUCCEEDED(rv)) {
#ifdef MOZ_OGG
    if (IsOggEnabled()) {
      for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(gOggTypes); i++) {
        catMan->AddCategoryEntry("Gecko-Content-Viewers", gOggTypes[i],
                                 "@mozilla.org/content/document-loader-factory;1",
                                 PR_FALSE, PR_TRUE, nsnull);
      }
    }
#endif
  }
}


void nsHTMLMediaElement::ShutdownMediaTypes()
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catMan(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
  if (NS_SUCCEEDED(rv)) {
#ifdef MOZ_OGG
    for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(gOggTypes); i++) {
      catMan->DeleteCategoryEntry("Gecko-Content-Viewers", gOggTypes[i], PR_FALSE);
    }
#endif
#ifdef MOZ_WAVE
    for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(gWaveTypes); i++) {
      catMan->DeleteCategoryEntry("Gecko-Content-Viewers", gWaveTypes[i], PR_FALSE);
    }
#endif
  }
}

PRBool nsHTMLMediaElement::CreateDecoder(const nsACString& aType)
{
#ifdef MOZ_OGG
  if (IsOggType(aType)) {
    mDecoder = new nsOggDecoder();
    if (mDecoder && !mDecoder->Init(this)) {
      mDecoder = nsnull;
    }
  }
#endif
#ifdef MOZ_WAVE
  if (IsWaveType(aType)) {
    mDecoder = new nsWaveDecoder();
    if (mDecoder && !mDecoder->Init(this)) {
      mDecoder = nsnull;
    }
  }
#endif
  return mDecoder != nsnull;
}

nsresult nsHTMLMediaElement::InitializeDecoderAsClone(nsMediaDecoder* aOriginal)
{
  nsMediaStream* originalStream = aOriginal->GetCurrentStream();
  if (!originalStream)
    return NS_ERROR_FAILURE;
  mDecoder = aOriginal->Clone();
  if (!mDecoder)
    return NS_ERROR_FAILURE;

  LOG(PR_LOG_DEBUG, ("%p Cloned decoder %p from %p", this, mDecoder.get(), aOriginal));  

  if (!mDecoder->Init(this)) {
    mDecoder = nsnull;
    return NS_ERROR_FAILURE;
  }

  float duration = aOriginal->GetDuration();
  if (duration >= 0) {
    mDecoder->SetDuration(PRInt64(NS_round(duration * 1000)));
    mDecoder->SetSeekable(aOriginal->GetSeekable());
  }

  nsMediaStream* stream = originalStream->CloneData(mDecoder);
  if (!stream) {
    mDecoder = nsnull;
    return NS_ERROR_FAILURE;
  }

  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_LOADING;

  nsresult rv = mDecoder->Load(stream, nsnull);
  if (NS_FAILED(rv)) {
    mDecoder = nsnull;
    return rv;
  }

  return FinishDecoderSetup();
}

nsresult nsHTMLMediaElement::InitializeDecoderForChannel(nsIChannel *aChannel,
                                                         nsIStreamListener **aListener)
{
  nsCAutoString mimeType;
  aChannel->GetContentType(mimeType);

  if (!CreateDecoder(mimeType))
    return NS_ERROR_FAILURE;

  LOG(PR_LOG_DEBUG, ("%p Created decoder %p for type %s", this, mDecoder.get(), mimeType.get()));  

  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_LOADING;

  nsMediaStream* stream = nsMediaStream::Create(mDecoder, aChannel);
  if (!stream)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = mDecoder->Load(stream, aListener);
  if (NS_FAILED(rv)) {
    mDecoder = nsnull;
    return rv;
  }

  
  
  mChannel = nsnull;

  return FinishDecoderSetup();
}

nsresult nsHTMLMediaElement::FinishDecoderSetup()
{
  nsresult rv = NS_OK;

  mDecoder->SetVolume(mMuted ? 0.0 : mVolume);

  if (!mPaused) {
    SetPlayedOrSeeked(PR_TRUE);
    rv = mDecoder->Play();
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

void nsHTMLMediaElement::MetadataLoaded()
{
  ChangeReadyState(nsIDOMHTMLMediaElement::HAVE_METADATA);
  DispatchAsyncSimpleEvent(NS_LITERAL_STRING("durationchange"));
  DispatchAsyncSimpleEvent(NS_LITERAL_STRING("loadedmetadata"));
}

void nsHTMLMediaElement::FirstFrameLoaded(PRBool aResourceFullyLoaded)
{
  ChangeReadyState(nsIDOMHTMLMediaElement::HAVE_CURRENT_DATA);
  ChangeDelayLoadStatus(PR_FALSE);

  NS_ASSERTION(!mSuspendedAfterFirstFrame, "Should not have already suspended");

  if (mDecoder && mAllowSuspendAfterFirstFrame && mPaused &&
      !aResourceFullyLoaded &&
      !HasAttr(kNameSpaceID_None, nsGkAtoms::autoplay) &&
      !HasAttr(kNameSpaceID_None, nsGkAtoms::autobuffer)) {
    mSuspendedAfterFirstFrame = PR_TRUE;
    mDecoder->Suspend();
  }
}

void nsHTMLMediaElement::ResourceLoaded()
{
  mBegun = PR_FALSE;
  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_IDLE;
  ChangeReadyState(nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA);
  
  DispatchAsyncSimpleEvent(NS_LITERAL_STRING("suspend"));
}

void nsHTMLMediaElement::NetworkError()
{
  mError = new nsHTMLMediaError(nsIDOMHTMLMediaError::MEDIA_ERR_NETWORK);
  mBegun = PR_FALSE;
  DispatchAsyncProgressEvent(NS_LITERAL_STRING("error"));
  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_EMPTY;
  DispatchAsyncSimpleEvent(NS_LITERAL_STRING("emptied"));
  ChangeDelayLoadStatus(PR_FALSE);
}

void nsHTMLMediaElement::DecodeError()
{
  mError = new nsHTMLMediaError(nsIDOMHTMLMediaError::MEDIA_ERR_DECODE);
  mBegun = PR_FALSE;
  DispatchAsyncProgressEvent(NS_LITERAL_STRING("error"));
  mNetworkState = nsIDOMHTMLMediaElement::NETWORK_EMPTY;
  DispatchAsyncSimpleEvent(NS_LITERAL_STRING("emptied"));
  ChangeDelayLoadStatus(PR_FALSE);
}

void nsHTMLMediaElement::PlaybackEnded()
{
  NS_ASSERTION(mDecoder->IsEnded(), "Decoder fired ended, but not in ended state");
  DispatchAsyncSimpleEvent(NS_LITERAL_STRING("ended"));
}

void nsHTMLMediaElement::SeekStarted()
{
  DispatchAsyncSimpleEvent(NS_LITERAL_STRING("seeking"));
}

void nsHTMLMediaElement::SeekCompleted()
{
  mPlayingBeforeSeek = PR_FALSE;
  SetPlayedOrSeeked(PR_TRUE);
  DispatchAsyncSimpleEvent(NS_LITERAL_STRING("seeked"));
}

void nsHTMLMediaElement::DownloadSuspended()
{
  if (mBegun) {
    mNetworkState = nsIDOMHTMLMediaElement::NETWORK_IDLE;
    DispatchAsyncSimpleEvent(NS_LITERAL_STRING("suspend"));
  }
}

void nsHTMLMediaElement::DownloadResumed()
{
  if (mBegun) {
    mNetworkState = nsIDOMHTMLMediaElement::NETWORK_LOADING;
  }
}

void nsHTMLMediaElement::DownloadStalled()
{
  if (mNetworkState == nsIDOMHTMLMediaElement::NETWORK_LOADING) {
    DispatchAsyncProgressEvent(NS_LITERAL_STRING("stalled"));
  }
}

PRBool nsHTMLMediaElement::ShouldCheckAllowOrigin()
{
  return nsContentUtils::GetBoolPref("media.enforce_same_site_origin",
                                     PR_TRUE);
}




static const PRInt32 gDownloadSizeSafetyMargin = 1000000;

void nsHTMLMediaElement::UpdateReadyStateForData(NextFrameStatus aNextFrame)
{
  if (mReadyState < nsIDOMHTMLMediaElement::HAVE_METADATA) {
    
    
    
    
    return;
  }

  nsMediaDecoder::Statistics stats = mDecoder->GetStatistics();

  if (aNextFrame != NEXT_FRAME_AVAILABLE) {
    ChangeReadyState(nsIDOMHTMLMediaElement::HAVE_CURRENT_DATA);
    if (!mWaitingFired && aNextFrame == NEXT_FRAME_UNAVAILABLE_BUFFERING) {
      DispatchAsyncSimpleEvent(NS_LITERAL_STRING("waiting"));
      mWaitingFired = PR_TRUE;
    }
    return;
  }

  
  
  
  
  
  
  if (stats.mTotalBytes < 0 ? stats.mDownloadRateReliable :
                              stats.mTotalBytes == stats.mDownloadPosition) {
    ChangeReadyState(nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA);
    return;
  }

  if (stats.mDownloadRateReliable && stats.mPlaybackRateReliable) {
    PRInt64 bytesToDownload = stats.mTotalBytes - stats.mDownloadPosition;
    PRInt64 bytesToPlayback = stats.mTotalBytes - stats.mPlaybackPosition;
    double timeToDownload =
      (bytesToDownload + gDownloadSizeSafetyMargin)/stats.mDownloadRate;
    double timeToPlay = bytesToPlayback/stats.mPlaybackRate;
    if (timeToDownload <= timeToPlay) {
      ChangeReadyState(nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA);
      return;
    }
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
    DispatchAsyncSimpleEvent(NS_LITERAL_STRING("waiting"));
  }

  if (oldState < nsIDOMHTMLMediaElement::HAVE_CURRENT_DATA &&
      mReadyState >= nsIDOMHTMLMediaElement::HAVE_CURRENT_DATA && 
      !mLoadedFirstFrame)
  {
    DispatchAsyncSimpleEvent(NS_LITERAL_STRING("loadeddata"));
    mLoadedFirstFrame = PR_TRUE;
  }

  if (mReadyState == nsIDOMHTMLMediaElement::HAVE_CURRENT_DATA) {
    mWaitingFired = PR_FALSE;
  }

  if (oldState < nsIDOMHTMLMediaElement::HAVE_FUTURE_DATA && 
      mReadyState >= nsIDOMHTMLMediaElement::HAVE_FUTURE_DATA) {
    DispatchAsyncSimpleEvent(NS_LITERAL_STRING("canplay"));
  }

  if (mReadyState == nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA) {
    NotifyAutoplayDataReady();
  }
  
  if (oldState < nsIDOMHTMLMediaElement::HAVE_FUTURE_DATA && 
      mReadyState >= nsIDOMHTMLMediaElement::HAVE_FUTURE_DATA &&
      IsPotentiallyPlaying()) {
    DispatchAsyncSimpleEvent(NS_LITERAL_STRING("playing"));
  }

  if (oldState < nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA &&
      mReadyState >= nsIDOMHTMLMediaElement::HAVE_ENOUGH_DATA) {
    DispatchAsyncSimpleEvent(NS_LITERAL_STRING("canplaythrough"));
  }
}

void nsHTMLMediaElement::NotifyAutoplayDataReady()
{
  if (mAutoplaying &&
      mPaused &&
      HasAttr(kNameSpaceID_None, nsGkAtoms::autoplay) &&
      mAutoplayEnabled) {
    mPaused = PR_FALSE;
    if (mDecoder) {
      SetPlayedOrSeeked(PR_TRUE);
      mDecoder->Play();
    }
    DispatchAsyncSimpleEvent(NS_LITERAL_STRING("play"));
  }
}

void nsHTMLMediaElement::Paint(gfxContext* aContext,
                               gfxPattern::GraphicsFilter aFilter,
                               const gfxRect& aRect) 
{
  if (mDecoder)
    mDecoder->Paint(aContext, aFilter, aRect);
}

nsresult nsHTMLMediaElement::DispatchSimpleEvent(const nsAString& aName)
{
  LOG_EVENT(PR_LOG_DEBUG, ("%p Dispatching simple event %s", this,
                          NS_ConvertUTF16toUTF8(aName).get()));

  return nsContentUtils::DispatchTrustedEvent(GetOwnerDoc(), 
                                              static_cast<nsIContent*>(this), 
                                              aName, 
                                              PR_TRUE, 
                                              PR_TRUE);
}

nsresult nsHTMLMediaElement::DispatchAsyncSimpleEvent(const nsAString& aName)
{
  LOG_EVENT(PR_LOG_DEBUG, ("%p Queuing simple event %s", this, NS_ConvertUTF16toUTF8(aName).get()));

  nsCOMPtr<nsIRunnable> event = new nsAsyncEventRunner(aName, this, PR_FALSE);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL); 
  return NS_OK;                           
}

nsresult nsHTMLMediaElement::DispatchAsyncProgressEvent(const nsAString& aName)
{
  LOG_EVENT(PR_LOG_DEBUG, ("%p Queuing progress event %s", this, NS_ConvertUTF16toUTF8(aName).get()));

  nsCOMPtr<nsIRunnable> event = new nsAsyncEventRunner(aName, this, PR_TRUE);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL); 
  return NS_OK;                           
}

nsresult nsHTMLMediaElement::DispatchProgressEvent(const nsAString& aName)
{
  nsCOMPtr<nsIDOMDocumentEvent> docEvent(do_QueryInterface(GetOwnerDoc()));
  nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(static_cast<nsIContent*>(this)));
  NS_ENSURE_TRUE(docEvent && target, NS_ERROR_INVALID_ARG);

  nsCOMPtr<nsIDOMEvent> event;
  nsresult rv = docEvent->CreateEvent(NS_LITERAL_STRING("ProgressEvent"), getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIDOMProgressEvent> progressEvent(do_QueryInterface(event));
  NS_ENSURE_TRUE(progressEvent, NS_ERROR_FAILURE);

  PRInt64 totalBytes = 0;
  PRUint64 downloadPosition = 0;
  if (mDecoder) {
    nsMediaDecoder::Statistics stats = mDecoder->GetStatistics();
    totalBytes = stats.mTotalBytes;
    downloadPosition = stats.mDownloadPosition;
  }
  rv = progressEvent->InitProgressEvent(aName, PR_TRUE, PR_TRUE,
    totalBytes >= 0, downloadPosition, totalBytes);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG_EVENT(PR_LOG_DEBUG, ("%p Dispatching progress event %s", this,
                          NS_ConvertUTF16toUTF8(aName).get()));
  
  PRBool dummy;
  return target->DispatchEvent(event, &dummy);  
}

nsresult nsHTMLMediaElement::DoneAddingChildren(PRBool aHaveNotified)
{
  if (!mIsDoneAddingChildren) {
    mIsDoneAddingChildren = PR_TRUE;
  
    if (mNetworkState == nsIDOMHTMLMediaElement::NETWORK_EMPTY) {
      QueueSelectResourceTask();
    }
  }

  return NS_OK;
}

PRBool nsHTMLMediaElement::IsDoneAddingChildren()
{
  return mIsDoneAddingChildren;
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

void nsHTMLMediaElement::DestroyContent()
{
  if (mDecoder) {
    mDecoder->Shutdown();
    mDecoder = nsnull;
  }
  if (mChannel) {
    mChannel->Cancel(NS_BINDING_ABORTED);
    mChannel = nsnull;
  }
  nsGenericHTMLElement::DestroyContent();
}

void nsHTMLMediaElement::Freeze()
{
  mPausedBeforeFreeze = mPaused;
  if (!mPaused) {
    Pause();
  }
  if (mDecoder) {
    mDecoder->Suspend();
  }
}

void nsHTMLMediaElement::Thaw()
{
  if (!mPausedBeforeFreeze) {
    Play();
  }

  if (mDecoder) {
    mDecoder->Resume();
  }
}

PRBool
nsHTMLMediaElement::IsNodeOfType(PRUint32 aFlags) const
{
  return !(aFlags & ~(eCONTENT | eELEMENT | eMEDIA));
}

void nsHTMLMediaElement::NotifyAddedSource()
{
  if (mLoadWaitStatus == WAITING_FOR_SRC_OR_SOURCE) {
    QueueSelectResourceTask();
  } else if (mLoadWaitStatus == WAITING_FOR_SOURCE) { 
    QueueLoadFromSourceTask();
  }
}

already_AddRefed<nsIURI> nsHTMLMediaElement::GetNextSource()
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIDOMNode> thisDomNode = do_QueryInterface(this);

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
      nsCOMPtr<nsIURI> uri;
      nsAutoString src,type;

      
      if (!child->GetAttr(kNameSpaceID_None, nsGkAtoms::src, src))
        continue;

      
      if (child->GetAttr(kNameSpaceID_None, nsGkAtoms::type, type) &&
          GetCanPlay(type) == CANPLAY_NO)
        continue;
      
      LOG(PR_LOG_DEBUG, ("%p Trying load from <source>=%s type=%s", this,
                         NS_ConvertUTF16toUTF8(src).get(), NS_ConvertUTF16toUTF8(type).get()));  
      NewURIFromString(src, getter_AddRefs(uri));
      return uri.forget();
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
    NS_ASSERTION(mLoadBlockedDoc, "Need a doc to block on");
    LOG(PR_LOG_DEBUG, ("%p ChangeDelayLoadStatus(%d) doc=0x%p", this, aDelay, mLoadBlockedDoc.get()));
    mLoadBlockedDoc->UnblockOnload(PR_FALSE);
    mLoadBlockedDoc = nsnull;
  }
}

already_AddRefed<nsILoadGroup> nsHTMLMediaElement::GetDocumentLoadGroup()
{
  nsIDocument* doc = GetOwnerDoc();
  return doc ? doc->GetDocumentLoadGroup() : nsnull;
}
