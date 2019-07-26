











#include "nsImageLoadingContent.h"
#include "nsAutoPtr.h"
#include "nsError.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMWindow.h"
#include "nsServiceManagerUtils.h"
#include "nsContentPolicyUtils.h"
#include "nsIURI.h"
#include "nsILoadGroup.h"
#include "imgIContainer.h"
#include "imgLoader.h"
#include "imgRequestProxy.h"
#include "nsThreadUtils.h"
#include "nsNetUtil.h"
#include "nsAsyncDOMEvent.h"
#include "nsImageFrame.h"

#include "nsIPresShell.h"
#include "nsEventStates.h"
#include "nsGUIEvent.h"

#include "nsIChannel.h"
#include "nsIStreamListener.h"

#include "nsIFrame.h"
#include "nsIDOMNode.h"

#include "nsContentUtils.h"
#include "nsLayoutUtils.h"
#include "nsIContentPolicy.h"
#include "nsEventDispatcher.h"
#include "nsSVGEffects.h"

#include "mozAutoDocUpdate.h"
#include "mozilla/dom/Element.h"

using namespace mozilla;

#ifdef DEBUG_chb
static void PrintReqURL(imgIRequest* req) {
  if (!req) {
    printf("(null req)\n");
    return;
  }

  nsCOMPtr<nsIURI> uri;
  req->GetURI(getter_AddRefs(uri));
  if (!uri) {
    printf("(null uri)\n");
    return;
  }

  nsAutoCString spec;
  uri->GetSpec(spec);
  printf("spec='%s'\n", spec.get());
}
#endif 


nsImageLoadingContent::nsImageLoadingContent()
  : mCurrentRequestFlags(0),
    mPendingRequestFlags(0),
    mObserverList(nullptr),
    mImageBlockingStatus(nsIContentPolicy::ACCEPT),
    mLoadingEnabled(true),
    mIsImageStateForced(false),
    mLoading(false),
    
    mBroken(true),
    mUserDisabled(false),
    mSuppressed(false),
    mFireEventsOnDecode(false),
    mNewRequestsWillNeedAnimationReset(false),
    mStateChangerDepth(0),
    mCurrentRequestRegistered(false),
    mPendingRequestRegistered(false),
    mVisibleCount(0)
{
  if (!nsContentUtils::GetImgLoaderForChannel(nullptr)) {
    mLoadingEnabled = false;
  }
}

void
nsImageLoadingContent::DestroyImageLoadingContent()
{
  
  ClearCurrentRequest(NS_BINDING_ABORTED);
  ClearPendingRequest(NS_BINDING_ABORTED);
}

nsImageLoadingContent::~nsImageLoadingContent()
{
  NS_ASSERTION(!mCurrentRequest && !mPendingRequest,
               "DestroyImageLoadingContent not called");
  NS_ASSERTION(!mObserverList.mObserver && !mObserverList.mNext,
               "Observers still registered?");
}




NS_IMETHODIMP
nsImageLoadingContent::Notify(imgIRequest* aRequest,
                              int32_t aType,
                              const nsIntRect* aData)
{
  if (aType == imgINotificationObserver::IS_ANIMATED) {
    return OnImageIsAnimated(aRequest);
  }

  if (aType == imgINotificationObserver::UNLOCKED_DRAW) {
    OnUnlockedDraw();
    return NS_OK;
  }

  if (aType == imgINotificationObserver::LOAD_COMPLETE) {
    
    NS_ABORT_IF_FALSE(aRequest, "no request?");

    NS_PRECONDITION(aRequest == mCurrentRequest || aRequest == mPendingRequest,
                    "Unknown request");
  }

  {
    nsAutoScriptBlocker scriptBlocker;

    for (ImageObserver* observer = &mObserverList, *next; observer;
         observer = next) {
      next = observer->mNext;
      if (observer->mObserver) {
        observer->mObserver->Notify(aRequest, aType, aData);
      }
    }
  }

  if (aType == imgINotificationObserver::SIZE_AVAILABLE) {
    
    
    UpdateImageState(true);
  }

  if (aType == imgINotificationObserver::LOAD_COMPLETE) {
    uint32_t reqStatus;
    aRequest->GetImageStatus(&reqStatus);
    nsresult status =
        reqStatus & imgIRequest::STATUS_ERROR ? NS_ERROR_FAILURE : NS_OK;
    return OnStopRequest(aRequest, status);
  }

  if (aType == imgINotificationObserver::DECODE_COMPLETE && mFireEventsOnDecode) {
    mFireEventsOnDecode = false;

    uint32_t reqStatus;
    aRequest->GetImageStatus(&reqStatus);
    if (reqStatus & imgIRequest::STATUS_ERROR) {
      FireEvent(NS_LITERAL_STRING("error"));
    } else {
      FireEvent(NS_LITERAL_STRING("load"));
    }

    UpdateImageState(true);
  }

  return NS_OK;
}

nsresult
nsImageLoadingContent::OnStopRequest(imgIRequest* aRequest,
                                     nsresult aStatus)
{
  uint32_t oldStatus;
  aRequest->GetImageStatus(&oldStatus);

  
  
  
  
  
  
  
  if (!(oldStatus & (imgIRequest::STATUS_ERROR | imgIRequest::STATUS_LOAD_COMPLETE)))
    return NS_OK;

  
  AutoStateChanger changer(this, true);

  
  if (aRequest == mPendingRequest) {
    MakePendingRequestCurrent();
  }
  NS_ABORT_IF_FALSE(aRequest == mCurrentRequest,
                    "One way or another, we should be current by now");

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  bool startedDecoding = false;
  nsIDocument* doc = GetOurOwnerDoc();
  nsIPresShell* shell = doc ? doc->GetShell() : nullptr;
  if (shell && shell->IsVisible() &&
      (!shell->DidInitialize() || shell->IsPaintingSuppressed())) {

    if (NS_SUCCEEDED(mCurrentRequest->StartDecoding())) {
      startedDecoding = true;
    }
  }

  
  
  
  
  uint32_t reqStatus;
  aRequest->GetImageStatus(&reqStatus);
  if (NS_SUCCEEDED(aStatus) && !(reqStatus & imgIRequest::STATUS_ERROR) &&
      (reqStatus & imgIRequest::STATUS_DECODE_STARTED ||
       (startedDecoding && !(reqStatus & imgIRequest::STATUS_DECODE_COMPLETE)))) {
    mFireEventsOnDecode = true;
  } else {
    
    if (NS_SUCCEEDED(aStatus)) {
      FireEvent(NS_LITERAL_STRING("load"));
    } else {
      FireEvent(NS_LITERAL_STRING("error"));
    }
  }

  nsCOMPtr<nsINode> thisNode = do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  nsSVGEffects::InvalidateDirectRenderingObservers(thisNode->AsElement());

  return NS_OK;
}

void
nsImageLoadingContent::OnUnlockedDraw()
{
  if (mVisibleCount > 0) {
    
    return;
  }

  nsPresContext* presContext = GetFramePresContext();
  if (!presContext)
    return;

  nsIPresShell* presShell = presContext->PresShell();
  if (!presShell)
    return;

  presShell->EnsureImageInVisibleList(this);
}

nsresult
nsImageLoadingContent::OnImageIsAnimated(imgIRequest *aRequest)
{
  bool* requestFlag = GetRegisteredFlagForRequest(aRequest);
  if (requestFlag) {
    nsLayoutUtils::RegisterImageRequest(GetFramePresContext(),
                                        aRequest, requestFlag);
  }

  return NS_OK;
}





NS_IMETHODIMP
nsImageLoadingContent::GetLoadingEnabled(bool *aLoadingEnabled)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  *aLoadingEnabled = mLoadingEnabled;
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::SetLoadingEnabled(bool aLoadingEnabled)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  if (nsContentUtils::GetImgLoaderForChannel(nullptr)) {
    mLoadingEnabled = aLoadingEnabled;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::GetImageBlockingStatus(int16_t* aStatus)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  NS_PRECONDITION(aStatus, "Null out param");
  *aStatus = ImageBlockingStatus();
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::AddObserver(imgINotificationObserver* aObserver)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  NS_ENSURE_ARG_POINTER(aObserver);

  if (!mObserverList.mObserver) {
    mObserverList.mObserver = aObserver;
    
    return NS_OK;
  }

  

  ImageObserver* observer = &mObserverList;
  while (observer->mNext) {
    observer = observer->mNext;
  }

  observer->mNext = new ImageObserver(aObserver);
  if (! observer->mNext) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::RemoveObserver(imgINotificationObserver* aObserver)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  NS_ENSURE_ARG_POINTER(aObserver);

  if (mObserverList.mObserver == aObserver) {
    mObserverList.mObserver = nullptr;
    
    return NS_OK;
  }

  
  ImageObserver* observer = &mObserverList;
  while (observer->mNext && observer->mNext->mObserver != aObserver) {
    observer = observer->mNext;
  }

  
  
  if (observer->mNext) {
    
    ImageObserver* oldObserver = observer->mNext;
    observer->mNext = oldObserver->mNext;
    oldObserver->mNext = nullptr;  
    delete oldObserver;
  }
#ifdef DEBUG
  else {
    NS_WARNING("Asked to remove nonexistent observer");
  }
#endif
  return NS_OK;
}

already_AddRefed<imgIRequest>
nsImageLoadingContent::GetRequest(int32_t aRequestType,
                                  ErrorResult& aError)
{
  nsCOMPtr<imgIRequest> request;
  switch(aRequestType) {
  case CURRENT_REQUEST:
    request = mCurrentRequest;
    break;
  case PENDING_REQUEST:
    request = mPendingRequest;
    break;
  default:
    NS_ERROR("Unknown request type");
    aError.Throw(NS_ERROR_UNEXPECTED);
  }

  return request.forget();
}

NS_IMETHODIMP
nsImageLoadingContent::GetRequest(int32_t aRequestType,
                                  imgIRequest** aRequest)
{
  NS_ENSURE_ARG_POINTER(aRequest);

  ErrorResult result;
  *aRequest = GetRequest(aRequestType, result).get();

  return result.ErrorCode();
}

NS_IMETHODIMP_(void)
nsImageLoadingContent::FrameCreated(nsIFrame* aFrame)
{
  NS_ASSERTION(aFrame, "aFrame is null");

  if (aFrame->HasAnyStateBits(NS_FRAME_IN_POPUP)) {
    
    IncrementVisibleCount();
  }

  nsPresContext* presContext = aFrame->PresContext();
  if (mVisibleCount == 0) {
    presContext->PresShell()->EnsureImageInVisibleList(this);
  }

  
  
  TrackImage(mCurrentRequest, SKIP_FRAME_CHECK);
  TrackImage(mPendingRequest, SKIP_FRAME_CHECK);

  
  
  if (mCurrentRequest) {
    nsLayoutUtils::RegisterImageRequestIfAnimated(presContext, mCurrentRequest,
                                                  &mCurrentRequestRegistered);
  }

  if (mPendingRequest) {
    nsLayoutUtils::RegisterImageRequestIfAnimated(presContext, mPendingRequest,
                                                  &mPendingRequestRegistered);
  }
}

NS_IMETHODIMP_(void)
nsImageLoadingContent::FrameDestroyed(nsIFrame* aFrame)
{
  NS_ASSERTION(aFrame, "aFrame is null");

  
  if (mCurrentRequest) {
    nsLayoutUtils::DeregisterImageRequest(GetFramePresContext(),
                                          mCurrentRequest,
                                          &mCurrentRequestRegistered);
  }

  if (mPendingRequest) {
    nsLayoutUtils::DeregisterImageRequest(GetFramePresContext(),
                                          mPendingRequest,
                                          &mPendingRequestRegistered);
  }

  UntrackImage(mCurrentRequest);
  UntrackImage(mPendingRequest);

  if (aFrame->HasAnyStateBits(NS_FRAME_IN_POPUP)) {
    
    
    DecrementVisibleCount();
  }
}

int32_t
nsImageLoadingContent::GetRequestType(imgIRequest* aRequest,
                                      ErrorResult& aError)
{
  if (aRequest == mCurrentRequest) {
    return CURRENT_REQUEST;
  }

  if (aRequest == mPendingRequest) {
    return PENDING_REQUEST;
  }

  NS_ERROR("Unknown request");
  aError.Throw(NS_ERROR_UNEXPECTED);
  return UNKNOWN_REQUEST;
}

NS_IMETHODIMP
nsImageLoadingContent::GetRequestType(imgIRequest* aRequest,
                                      int32_t* aRequestType)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  NS_PRECONDITION(aRequestType, "Null out param");

  ErrorResult result;
  *aRequestType = GetRequestType(aRequest, result);
  return result.ErrorCode();
}

already_AddRefed<nsIURI>
nsImageLoadingContent::GetCurrentURI(ErrorResult& aError)
{
  nsCOMPtr<nsIURI> uri;
  if (mCurrentRequest) {
    mCurrentRequest->GetURI(getter_AddRefs(uri));
  } else if (mCurrentURI) {
    nsresult rv = NS_EnsureSafeToReturn(mCurrentURI, getter_AddRefs(uri));
    if (NS_FAILED(rv)) {
      aError.Throw(rv);
    }
  }

  return uri.forget();
}

NS_IMETHODIMP
nsImageLoadingContent::GetCurrentURI(nsIURI** aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);

  ErrorResult result;
  *aURI = GetCurrentURI(result).get();
  return result.ErrorCode();
}

already_AddRefed<nsIStreamListener>
nsImageLoadingContent::LoadImageWithChannel(nsIChannel* aChannel,
                                            ErrorResult& aError)
{
  if (!nsContentUtils::GetImgLoaderForChannel(aChannel)) {
    aError.Throw(NS_ERROR_NULL_POINTER);
    return nullptr;
  }

  nsCOMPtr<nsIDocument> doc = GetOurOwnerDoc();
  if (!doc) {
    
    return nullptr;
  }

  
  
  

  
  AutoStateChanger changer(this, true);

  
  nsCOMPtr<nsIStreamListener> listener;
  nsRefPtr<imgRequestProxy>& req = PrepareNextRequest();
  nsresult rv = nsContentUtils::GetImgLoaderForChannel(aChannel)->
    LoadImageWithChannel(aChannel, this, doc,
                         getter_AddRefs(listener),
                         getter_AddRefs(req));
  if (NS_SUCCEEDED(rv)) {
    TrackImage(req);
    ResetAnimationIfNeeded();
  } else {
    MOZ_ASSERT(!req, "Shouldn't have non-null request here");
    
    
    if (!mCurrentRequest)
      aChannel->GetURI(getter_AddRefs(mCurrentURI));
    FireEvent(NS_LITERAL_STRING("error"));
    aError.Throw(rv);
  }
  return listener.forget();
}

NS_IMETHODIMP
nsImageLoadingContent::LoadImageWithChannel(nsIChannel* aChannel,
                                            nsIStreamListener** aListener)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);
  NS_ENSURE_ARG_POINTER(aListener);

  ErrorResult result;
  *aListener = LoadImageWithChannel(aChannel, result).get();
  return result.ErrorCode();
}

void
nsImageLoadingContent::ForceReload(ErrorResult& aError)
{
  nsCOMPtr<nsIURI> currentURI;
  GetCurrentURI(getter_AddRefs(currentURI));
  if (!currentURI) {
    aError.Throw(NS_ERROR_NOT_AVAILABLE);
    return;
  }

  nsresult rv = LoadImage(currentURI, true, true, nullptr, nsIRequest::VALIDATE_ALWAYS);
  if (NS_FAILED(rv)) {
    aError.Throw(rv);
  }
}

NS_IMETHODIMP nsImageLoadingContent::ForceReload()
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  ErrorResult result;
  ForceReload(result);
  return result.ErrorCode();
}

NS_IMETHODIMP
nsImageLoadingContent::BlockOnload(imgIRequest* aRequest)
{
  if (aRequest != mCurrentRequest) {
    return NS_OK;
  }

  NS_ASSERTION(!(mCurrentRequestFlags & REQUEST_BLOCKS_ONLOAD),
               "Double BlockOnload!?");
  mCurrentRequestFlags |= REQUEST_BLOCKS_ONLOAD;

  nsIDocument* doc = GetOurCurrentDoc();
  if (doc) {
    doc->BlockOnload();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::UnblockOnload(imgIRequest* aRequest)
{
  if (aRequest != mCurrentRequest) {
    return NS_OK;
  }

  NS_ASSERTION(mCurrentRequestFlags & REQUEST_BLOCKS_ONLOAD,
               "Double UnblockOnload!?");
  mCurrentRequestFlags &= ~REQUEST_BLOCKS_ONLOAD;

  nsIDocument* doc = GetOurCurrentDoc();
  if (doc) {
    doc->UnblockOnload(false);
  }

  return NS_OK;
}

void
nsImageLoadingContent::IncrementVisibleCount()
{
  mVisibleCount++;
  if (mVisibleCount == 1) {
    TrackImage(mCurrentRequest);
    TrackImage(mPendingRequest);
  }
}

void
nsImageLoadingContent::DecrementVisibleCount()
{
  NS_ASSERTION(mVisibleCount > 0, "visible count should be positive here");
  mVisibleCount--;

  if (mVisibleCount == 0) {
    UntrackImage(mCurrentRequest);
    UntrackImage(mPendingRequest);
  }
}

uint32_t
nsImageLoadingContent::GetVisibleCount()
{
  return mVisibleCount;
}





nsresult
nsImageLoadingContent::LoadImage(const nsAString& aNewURI,
                                 bool aForce,
                                 bool aNotify)
{
  
  nsIDocument* doc = GetOurOwnerDoc();
  if (!doc) {
    
    return NS_OK;
  }

  nsCOMPtr<nsIURI> imageURI;
  nsresult rv = StringToURI(aNewURI, doc, getter_AddRefs(imageURI));
  NS_ENSURE_SUCCESS(rv, rv);
  

  bool equal;

  if (aNewURI.IsEmpty() &&
      doc->GetDocumentURI() &&
      NS_SUCCEEDED(doc->GetDocumentURI()->EqualsExceptRef(imageURI, &equal)) &&
      equal)  {

    
    
    
    
    
    
    
    CancelImageRequests(aNotify);
    return NS_OK;
  }

  NS_TryToSetImmutable(imageURI);

  return LoadImage(imageURI, aForce, aNotify, doc);
}

nsresult
nsImageLoadingContent::LoadImage(nsIURI* aNewURI,
                                 bool aForce,
                                 bool aNotify,
                                 nsIDocument* aDocument,
                                 nsLoadFlags aLoadFlags)
{
  if (!mLoadingEnabled) {
    
    
    FireEvent(NS_LITERAL_STRING("error"));
    return NS_OK;
  }

  NS_ASSERTION(!aDocument || aDocument == GetOurOwnerDoc(),
               "Bogus document passed in");
  
  if (!aDocument) {
    aDocument = GetOurOwnerDoc();
    if (!aDocument) {
      
      return NS_OK;
    }
  }

  
  
  
  
  if (!aForce && NS_CP_ACCEPTED(mImageBlockingStatus)) {
    nsCOMPtr<nsIURI> currentURI;
    GetCurrentURI(getter_AddRefs(currentURI));
    bool equal;
    if (currentURI &&
        NS_SUCCEEDED(currentURI->Equals(aNewURI, &equal)) &&
        equal) {
      
      return NS_OK;
    }
  }

  
  AutoStateChanger changer(this, aNotify);

  
  
  
  
#ifdef DEBUG
  nsCOMPtr<nsIContent> thisContent = do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ABORT_IF_FALSE(thisContent &&
                    thisContent->NodePrincipal() == aDocument->NodePrincipal(),
                    "Principal mismatch?");
#endif

  
  int16_t cpDecision = nsIContentPolicy::REJECT_REQUEST;
  nsContentUtils::CanLoadImage(aNewURI,
                               static_cast<nsIImageLoadingContent*>(this),
                               aDocument,
                               aDocument->NodePrincipal(),
                               &cpDecision);
  if (!NS_CP_ACCEPTED(cpDecision)) {
    FireEvent(NS_LITERAL_STRING("error"));
    SetBlockedRequest(aNewURI, cpDecision);
    return NS_OK;
  }

  nsLoadFlags loadFlags = aLoadFlags;
  int32_t corsmode = GetCORSMode();
  if (corsmode == CORS_ANONYMOUS) {
    loadFlags |= imgILoader::LOAD_CORS_ANONYMOUS;
  } else if (corsmode == CORS_USE_CREDENTIALS) {
    loadFlags |= imgILoader::LOAD_CORS_USE_CREDENTIALS;
  }

  
  nsRefPtr<imgRequestProxy>& req = PrepareNextRequest();
  nsresult rv;
  rv = nsContentUtils::LoadImage(aNewURI, aDocument,
                                 aDocument->NodePrincipal(),
                                 aDocument->GetDocumentURI(),
                                 this, loadFlags,
                                 getter_AddRefs(req));
  if (NS_SUCCEEDED(rv)) {
    TrackImage(req);
    ResetAnimationIfNeeded();

    
    
    
    
    if (req == mPendingRequest) {
      uint32_t pendingLoadStatus;
      rv = req->GetImageStatus(&pendingLoadStatus);
      if (NS_SUCCEEDED(rv) &&
          (pendingLoadStatus & imgIRequest::STATUS_LOAD_COMPLETE)) {
        MakePendingRequestCurrent();
        MOZ_ASSERT(mCurrentRequest,
                   "How could we not have a current request here?");

        nsImageFrame *f = do_QueryFrame(GetOurPrimaryFrame());
        if (f) {
          f->NotifyNewCurrentRequest(mCurrentRequest, NS_OK);
        }
      }
    }
  } else {
    MOZ_ASSERT(!req, "Shouldn't have non-null request here");
    
    
    if (!mCurrentRequest)
      mCurrentURI = aNewURI;
    FireEvent(NS_LITERAL_STRING("error"));
    return NS_OK;
  }

  return NS_OK;
}

nsresult
nsImageLoadingContent::ForceImageState(bool aForce, nsEventStates::InternalType aState)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  mIsImageStateForced = aForce;
  mForcedImageState = nsEventStates(aState);
  return NS_OK;
}

nsEventStates
nsImageLoadingContent::ImageState() const
{
  if (mIsImageStateForced) {
    return mForcedImageState;
  }

  nsEventStates states;

  if (mBroken) {
    states |= NS_EVENT_STATE_BROKEN;
  }
  if (mUserDisabled) {
    states |= NS_EVENT_STATE_USERDISABLED;
  }
  if (mSuppressed) {
    states |= NS_EVENT_STATE_SUPPRESSED;
  }
  if (mLoading) {
    states |= NS_EVENT_STATE_LOADING;
  }

  return states;
}

void
nsImageLoadingContent::UpdateImageState(bool aNotify)
{
  if (mStateChangerDepth > 0) {
    
    
    
    
    
    
    return;
  }
  
  nsCOMPtr<nsIContent> thisContent = do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  if (!thisContent) {
    return;
  }

  mLoading = mBroken = mUserDisabled = mSuppressed = false;
  
  
  
  
  if (mImageBlockingStatus == nsIContentPolicy::REJECT_SERVER) {
    mSuppressed = true;
  } else if (mImageBlockingStatus == nsIContentPolicy::REJECT_TYPE) {
    mUserDisabled = true;
  } else if (!mCurrentRequest) {
    
    mBroken = true;
  } else {
    uint32_t currentLoadStatus;
    nsresult rv = mCurrentRequest->GetImageStatus(&currentLoadStatus);
    if (NS_FAILED(rv) || (currentLoadStatus & imgIRequest::STATUS_ERROR)) {
      mBroken = true;
    } else if (!(currentLoadStatus & imgIRequest::STATUS_SIZE_AVAILABLE)) {
      mLoading = true;
    }
  }

  NS_ASSERTION(thisContent->IsElement(), "Not an element?");
  thisContent->AsElement()->UpdateState(aNotify);
}

void
nsImageLoadingContent::CancelImageRequests(bool aNotify)
{
  AutoStateChanger changer(this, aNotify);
  ClearPendingRequest(NS_BINDING_ABORTED);
  ClearCurrentRequest(NS_BINDING_ABORTED);
}

nsresult
nsImageLoadingContent::UseAsPrimaryRequest(imgRequestProxy* aRequest,
                                           bool aNotify)
{
  
  AutoStateChanger changer(this, aNotify);

  
  ClearPendingRequest(NS_BINDING_ABORTED);
  ClearCurrentRequest(NS_BINDING_ABORTED);

  
  nsRefPtr<imgRequestProxy>& req = PrepareNextRequest();
  nsresult rv = aRequest->Clone(this, getter_AddRefs(req));
  if (NS_SUCCEEDED(rv)) {
    TrackImage(req);
  } else {
    MOZ_ASSERT(!req, "Shouldn't have non-null request here");
    return rv;
  }

  return NS_OK;
}

nsIDocument*
nsImageLoadingContent::GetOurOwnerDoc()
{
  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ENSURE_TRUE(thisContent, nullptr);

  return thisContent->OwnerDoc();
}

nsIDocument*
nsImageLoadingContent::GetOurCurrentDoc()
{
  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ENSURE_TRUE(thisContent, nullptr);

  return thisContent->GetCurrentDoc();
}

nsIFrame*
nsImageLoadingContent::GetOurPrimaryFrame()
{
  nsCOMPtr<nsIContent> thisContent =
    do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  return thisContent->GetPrimaryFrame();
}

nsPresContext* nsImageLoadingContent::GetFramePresContext()
{
  nsIFrame* frame = GetOurPrimaryFrame();
  if (!frame) {
    return nullptr;
  }

  return frame->PresContext();
}

nsresult
nsImageLoadingContent::StringToURI(const nsAString& aSpec,
                                   nsIDocument* aDocument,
                                   nsIURI** aURI)
{
  NS_PRECONDITION(aDocument, "Must have a document");
  NS_PRECONDITION(aURI, "Null out param");

  
  nsCOMPtr<nsIContent> thisContent = do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  NS_ASSERTION(thisContent, "An image loading content must be an nsIContent");
  nsCOMPtr<nsIURI> baseURL = thisContent->GetBaseURI();

  
  const nsAFlatCString &charset = aDocument->GetDocumentCharacterSet();

  
  return NS_NewURI(aURI,
                   aSpec,
                   charset.IsEmpty() ? nullptr : charset.get(),
                   baseURL,
                   nsContentUtils::GetIOService());
}

nsresult
nsImageLoadingContent::FireEvent(const nsAString& aEventType)
{
  
  
  

  nsCOMPtr<nsINode> thisNode = do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));

  nsRefPtr<nsAsyncDOMEvent> event =
    new nsLoadBlockingAsyncDOMEvent(thisNode, aEventType, false, false);
  event->PostDOMEvent();
  
  return NS_OK;
}

nsRefPtr<imgRequestProxy>&
nsImageLoadingContent::PrepareNextRequest()
{
  
  
  if (!HaveSize(mCurrentRequest))
    return PrepareCurrentRequest();

  
  return PreparePendingRequest();
}

void
nsImageLoadingContent::SetBlockedRequest(nsIURI* aURI, int16_t aContentDecision)
{
  
  NS_ABORT_IF_FALSE(!NS_CP_ACCEPTED(aContentDecision), "Blocked but not?");

  
  
  
  
  
  
  ClearPendingRequest(NS_ERROR_IMAGE_BLOCKED);

  
  
  if (!HaveSize(mCurrentRequest)) {

    mImageBlockingStatus = aContentDecision;
    ClearCurrentRequest(NS_ERROR_IMAGE_BLOCKED);

    
    
    mCurrentURI = aURI;
  }
}

nsRefPtr<imgRequestProxy>&
nsImageLoadingContent::PrepareCurrentRequest()
{
  
  
  mImageBlockingStatus = nsIContentPolicy::ACCEPT;

  
  ClearCurrentRequest(NS_ERROR_IMAGE_SRC_CHANGED);

  if (mNewRequestsWillNeedAnimationReset) {
    mCurrentRequestFlags |= REQUEST_NEEDS_ANIMATION_RESET;
  }

  
  return mCurrentRequest;
}

nsRefPtr<imgRequestProxy>&
nsImageLoadingContent::PreparePendingRequest()
{
  
  ClearPendingRequest(NS_ERROR_IMAGE_SRC_CHANGED);

  if (mNewRequestsWillNeedAnimationReset) {
    mPendingRequestFlags |= REQUEST_NEEDS_ANIMATION_RESET;
  }

  
  return mPendingRequest;
}

namespace {

class ImageRequestAutoLock
{
public:
  ImageRequestAutoLock(imgIRequest* aRequest)
    : mRequest(aRequest)
  {
    if (mRequest) {
      mRequest->LockImage();
    }
  }

  ~ImageRequestAutoLock()
  {
    if (mRequest) {
      mRequest->UnlockImage();
    }
  }

private:
  nsCOMPtr<imgIRequest> mRequest;
};

} 

void
nsImageLoadingContent::MakePendingRequestCurrent()
{
  MOZ_ASSERT(mPendingRequest);

  
  
  
  
  
  ImageRequestAutoLock autoLock(mCurrentRequest);

  PrepareCurrentRequest() = mPendingRequest;
  mPendingRequest = nullptr;
  mCurrentRequestFlags = mPendingRequestFlags;
  mPendingRequestFlags = 0;
  ResetAnimationIfNeeded();
}

void
nsImageLoadingContent::ClearCurrentRequest(nsresult aReason)
{
  if (!mCurrentRequest) {
    
    
    mCurrentURI = nullptr;
    return;
  }
  NS_ABORT_IF_FALSE(!mCurrentURI,
                    "Shouldn't have both mCurrentRequest and mCurrentURI!");

  
  
  nsLayoutUtils::DeregisterImageRequest(GetFramePresContext(), mCurrentRequest,
                                        &mCurrentRequestRegistered);

  
  UntrackImage(mCurrentRequest, REQUEST_DISCARD);
  mCurrentRequest->CancelAndForgetObserver(aReason);
  mCurrentRequest = nullptr;
  mCurrentRequestFlags = 0;
}

void
nsImageLoadingContent::ClearPendingRequest(nsresult aReason)
{
  if (!mPendingRequest)
    return;

  
  
  
  nsCxPusher pusher;
  pusher.PushNull();

  
  
  nsLayoutUtils::DeregisterImageRequest(GetFramePresContext(), mPendingRequest,
                                        &mPendingRequestRegistered);

  UntrackImage(mPendingRequest, REQUEST_DISCARD);
  mPendingRequest->CancelAndForgetObserver(aReason);
  mPendingRequest = nullptr;
  mPendingRequestFlags = 0;
}

bool*
nsImageLoadingContent::GetRegisteredFlagForRequest(imgIRequest* aRequest)
{
  if (aRequest == mCurrentRequest) {
    return &mCurrentRequestRegistered;
  } else if (aRequest == mPendingRequest) {
    return &mPendingRequestRegistered;
  } else {
    return nullptr;
  }
}

void
nsImageLoadingContent::ResetAnimationIfNeeded()
{
  if (mCurrentRequest &&
      (mCurrentRequestFlags & REQUEST_NEEDS_ANIMATION_RESET)) {
    nsCOMPtr<imgIContainer> container;
    mCurrentRequest->GetImage(getter_AddRefs(container));
    if (container)
      container->ResetAnimation();
    mCurrentRequestFlags &= ~REQUEST_NEEDS_ANIMATION_RESET;
  }
}

bool
nsImageLoadingContent::HaveSize(imgIRequest *aImage)
{
  
  if (!aImage)
    return false;

  
  uint32_t status;
  nsresult rv = aImage->GetImageStatus(&status);
  return (NS_SUCCEEDED(rv) && (status & imgIRequest::STATUS_SIZE_AVAILABLE));
}

void
nsImageLoadingContent::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                  nsIContent* aBindingParent,
                                  bool aCompileEventHandlers)
{
  
  
  if (!aDocument)
    return;

  
  
  nsCxPusher pusher;
  pusher.PushNull();

  TrackImage(mCurrentRequest);
  TrackImage(mPendingRequest);

  if (mCurrentRequestFlags & REQUEST_BLOCKS_ONLOAD)
    aDocument->BlockOnload();
}

void
nsImageLoadingContent::UnbindFromTree(bool aDeep, bool aNullParent)
{
  
  nsCOMPtr<nsIDocument> doc = GetOurCurrentDoc();
  if (!doc)
    return;

  
  
  nsCxPusher pusher;
  pusher.PushNull();

  UntrackImage(mCurrentRequest);
  UntrackImage(mPendingRequest);

  if (mCurrentRequestFlags & REQUEST_BLOCKS_ONLOAD)
    doc->UnblockOnload(false);
}

nsresult
nsImageLoadingContent::TrackImage(imgIRequest* aImage, uint32_t aFlags )
{
  if (!aImage)
    return NS_OK;

  MOZ_ASSERT(aImage == mCurrentRequest || aImage == mPendingRequest,
             "Why haven't we heard of this request?");

  nsIDocument* doc = GetOurCurrentDoc();
  if (doc && ((aFlags & SKIP_FRAME_CHECK) || GetOurPrimaryFrame()) &&
      (mVisibleCount > 0)) {
    if (aImage == mCurrentRequest && !(mCurrentRequestFlags & REQUEST_IS_TRACKED)) {
      mCurrentRequestFlags |= REQUEST_IS_TRACKED;
      doc->AddImage(mCurrentRequest);
    }
    if (aImage == mPendingRequest && !(mPendingRequestFlags & REQUEST_IS_TRACKED)) {
      mPendingRequestFlags |= REQUEST_IS_TRACKED;
      doc->AddImage(mPendingRequest);
    }
  }
  return NS_OK;
}

nsresult
nsImageLoadingContent::UntrackImage(imgIRequest* aImage, uint32_t aFlags )
{
  if (!aImage)
    return NS_OK;

  MOZ_ASSERT(aImage == mCurrentRequest || aImage == mPendingRequest,
             "Why haven't we heard of this request?");

  
  
  
  nsIDocument* doc = GetOurCurrentDoc();
  if (doc) {
    if (aImage == mCurrentRequest && (mCurrentRequestFlags & REQUEST_IS_TRACKED)) {
      mCurrentRequestFlags &= ~REQUEST_IS_TRACKED;
      doc->RemoveImage(mCurrentRequest,
                       (aFlags & REQUEST_DISCARD) ? nsIDocument::REQUEST_DISCARD : 0);
    }
    if (aImage == mPendingRequest && (mPendingRequestFlags & REQUEST_IS_TRACKED)) {
      mPendingRequestFlags &= ~REQUEST_IS_TRACKED;
      doc->RemoveImage(mPendingRequest,
                       (aFlags & REQUEST_DISCARD) ? nsIDocument::REQUEST_DISCARD : 0);
    }
  }
  return NS_OK;
}


void
nsImageLoadingContent::CreateStaticImageClone(nsImageLoadingContent* aDest) const
{
  aDest->mCurrentRequest = nsContentUtils::GetStaticRequest(mCurrentRequest);
  aDest->TrackImage(aDest->mCurrentRequest);
  aDest->mForcedImageState = mForcedImageState;
  aDest->mImageBlockingStatus = mImageBlockingStatus;
  aDest->mLoadingEnabled = mLoadingEnabled;
  aDest->mStateChangerDepth = mStateChangerDepth;
  aDest->mIsImageStateForced = mIsImageStateForced;
  aDest->mLoading = mLoading;
  aDest->mBroken = mBroken;
  aDest->mUserDisabled = mUserDisabled;
  aDest->mSuppressed = mSuppressed;
}

CORSMode
nsImageLoadingContent::GetCORSMode()
{
  return CORS_NONE;
}

nsImageLoadingContent::ImageObserver::ImageObserver(imgINotificationObserver* aObserver)
  : mObserver(aObserver)
  , mNext(nullptr)
{
  MOZ_COUNT_CTOR(ImageObserver);
}

nsImageLoadingContent::ImageObserver::~ImageObserver()
{
  MOZ_COUNT_DTOR(ImageObserver);
  NS_CONTENT_DELETE_LIST_MEMBER(ImageObserver, this, mNext);
}
