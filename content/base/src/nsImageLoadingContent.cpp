











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
#include "imgILoader.h"
#include "nsThreadUtils.h"
#include "nsNetUtil.h"
#include "nsAsyncDOMEvent.h"
#include "nsGenericElement.h"
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
    mNewRequestsWillNeedAnimationReset(false),
    mStateChangerDepth(0),
    mCurrentRequestRegistered(false),
    mPendingRequestRegistered(false)
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



#define LOOP_OVER_OBSERVERS(func_)                                       \
  PR_BEGIN_MACRO                                                         \
    for (ImageObserver* observer = &mObserverList, *next; observer;      \
         observer = next) {                                              \
      next = observer->mNext;                                            \
      if (observer->mObserver) {                                         \
        observer->mObserver->func_;                                      \
      }                                                                  \
    }                                                                    \
  PR_END_MACRO





NS_IMETHODIMP
nsImageLoadingContent::FrameChanged(imgIRequest* aRequest,
                                    imgIContainer* aContainer,
                                    const nsIntRect* aDirtyRect)
{
  LOOP_OVER_OBSERVERS(FrameChanged(aRequest, aContainer, aDirtyRect));
  return NS_OK;
}
            



NS_IMETHODIMP
nsImageLoadingContent::OnStartRequest(imgIRequest* aRequest)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  LOOP_OVER_OBSERVERS(OnStartRequest(aRequest));
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::OnStartDecode(imgIRequest* aRequest)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  LOOP_OVER_OBSERVERS(OnStartDecode(aRequest));
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::OnStartContainer(imgIRequest* aRequest,
                                        imgIContainer* aContainer)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  LOOP_OVER_OBSERVERS(OnStartContainer(aRequest, aContainer));

  
  
  UpdateImageState(true);
  return NS_OK;    
}

NS_IMETHODIMP
nsImageLoadingContent::OnStartFrame(imgIRequest* aRequest,
                                    uint32_t aFrame)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  LOOP_OVER_OBSERVERS(OnStartFrame(aRequest, aFrame));
  return NS_OK;    
}

NS_IMETHODIMP
nsImageLoadingContent::OnDataAvailable(imgIRequest* aRequest,
                                       bool aCurrentFrame,
                                       const nsIntRect* aRect)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  LOOP_OVER_OBSERVERS(OnDataAvailable(aRequest, aCurrentFrame, aRect));
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::OnStopFrame(imgIRequest* aRequest,
                                   uint32_t aFrame)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  LOOP_OVER_OBSERVERS(OnStopFrame(aRequest, aFrame));
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::OnStopContainer(imgIRequest* aRequest,
                                       imgIContainer* aContainer)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  LOOP_OVER_OBSERVERS(OnStopContainer(aRequest, aContainer));
  return NS_OK;
}




NS_IMETHODIMP
nsImageLoadingContent::OnStopDecode(imgIRequest* aRequest,
                                    nsresult aStatus,
                                    const PRUnichar* aStatusArg)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  
  NS_ABORT_IF_FALSE(aRequest, "no request?");

  NS_PRECONDITION(aRequest == mCurrentRequest || aRequest == mPendingRequest,
                  "Unknown request");
  LOOP_OVER_OBSERVERS(OnStopDecode(aRequest, aStatus, aStatusArg));

  
  

  
  AutoStateChanger changer(this, true);

  
  if (aRequest == mPendingRequest) {
    MakePendingRequestCurrent();
  }
  NS_ABORT_IF_FALSE(aRequest == mCurrentRequest,
                    "One way or another, we should be current by now");

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  nsIDocument* doc = GetOurOwnerDoc();
  nsIPresShell* shell = doc ? doc->GetShell() : nullptr;
  if (shell && shell->IsVisible() &&
      (!shell->DidInitialize() || shell->IsPaintingSuppressed())) {

    mCurrentRequest->RequestDecode();
  }

  
  if (NS_SUCCEEDED(aStatus)) {
    FireEvent(NS_LITERAL_STRING("load"));
  } else {
    FireEvent(NS_LITERAL_STRING("error"));
  }

  nsCOMPtr<nsINode> thisNode = do_QueryInterface(static_cast<nsIImageLoadingContent*>(this));
  nsSVGEffects::InvalidateDirectRenderingObservers(thisNode->AsElement());

  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::OnStopRequest(imgIRequest* aRequest, bool aLastPart)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  LOOP_OVER_OBSERVERS(OnStopRequest(aRequest, aLastPart));

  return NS_OK;
}

NS_IMETHODIMP
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
nsImageLoadingContent::OnDiscard(imgIRequest *aRequest)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  LOOP_OVER_OBSERVERS(OnDiscard(aRequest));

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
  *aStatus = mImageBlockingStatus;
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::AddObserver(imgIDecoderObserver* aObserver)
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
nsImageLoadingContent::RemoveObserver(imgIDecoderObserver* aObserver)
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

NS_IMETHODIMP
nsImageLoadingContent::GetRequest(int32_t aRequestType,
                                  imgIRequest** aRequest)
{
  switch(aRequestType) {
  case CURRENT_REQUEST:
    *aRequest = mCurrentRequest;
    break;
  case PENDING_REQUEST:
    *aRequest = mPendingRequest;
    break;
  default:
    NS_ERROR("Unknown request type");
    *aRequest = nullptr;
    return NS_ERROR_UNEXPECTED;
  }
  
  NS_IF_ADDREF(*aRequest);
  return NS_OK;
}

NS_IMETHODIMP_(void)
nsImageLoadingContent::FrameCreated(nsIFrame* aFrame)
{
  NS_ASSERTION(aFrame, "aFrame is null");

  
  
  nsPresContext* presContext = aFrame->PresContext();

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
}

NS_IMETHODIMP
nsImageLoadingContent::GetRequestType(imgIRequest* aRequest,
                                      int32_t* aRequestType)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  NS_PRECONDITION(aRequestType, "Null out param");
  
  if (aRequest == mCurrentRequest) {
    *aRequestType = CURRENT_REQUEST;
    return NS_OK;
  }

  if (aRequest == mPendingRequest) {
    *aRequestType = PENDING_REQUEST;
    return NS_OK;
  }

  *aRequestType = UNKNOWN_REQUEST;
  NS_ERROR("Unknown request");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsImageLoadingContent::GetCurrentURI(nsIURI** aURI)
{
  if (mCurrentRequest) {
    return mCurrentRequest->GetURI(aURI);
  }

  if (!mCurrentURI) {
    *aURI = nullptr;
    return NS_OK;
  }
  
  return NS_EnsureSafeToReturn(mCurrentURI, aURI);
}

NS_IMETHODIMP
nsImageLoadingContent::LoadImageWithChannel(nsIChannel* aChannel,
                                            nsIStreamListener** aListener)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  if (!nsContentUtils::GetImgLoaderForChannel(aChannel)) {
    return NS_ERROR_NULL_POINTER;
  }

  nsCOMPtr<nsIDocument> doc = GetOurOwnerDoc();
  if (!doc) {
    
    return NS_OK;
  }

  
  
  

  
  AutoStateChanger changer(this, true);

  
  nsCOMPtr<imgIRequest>& req = PrepareNextRequest();
  nsresult rv = nsContentUtils::GetImgLoaderForChannel(aChannel)->
    LoadImageWithChannel(aChannel, this, doc, aListener,
                         getter_AddRefs(req));
  if (NS_SUCCEEDED(rv)) {
    TrackImage(req);
    ResetAnimationIfNeeded();
  } else {
    
    
    if (!mCurrentRequest)
      aChannel->GetURI(getter_AddRefs(mCurrentURI));
    FireEvent(NS_LITERAL_STRING("error"));
    return rv;
  }
  return NS_OK;;
}

NS_IMETHODIMP nsImageLoadingContent::ForceReload()
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIURI> currentURI;
  GetCurrentURI(getter_AddRefs(currentURI));
  if (!currentURI) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return LoadImage(currentURI, true, true, nullptr, nsIRequest::VALIDATE_ALWAYS);
}

NS_IMETHODIMP
nsImageLoadingContent::BlockOnload(imgIRequest* aRequest)
{
  if (aRequest != mCurrentRequest) {
    return NS_OK;
  }

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

  nsIDocument* doc = GetOurCurrentDoc();
  if (doc) {
    doc->UnblockOnload(false);
  }

  return NS_OK;
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
      NS_SUCCEEDED(doc->GetDocumentURI()->Equals(imageURI, &equal)) && 
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

  
  nsCOMPtr<imgIRequest>& req = PrepareNextRequest();
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
nsImageLoadingContent::UseAsPrimaryRequest(imgIRequest* aRequest,
                                           bool aNotify)
{
  
  AutoStateChanger changer(this, aNotify);

  
  ClearPendingRequest(NS_BINDING_ABORTED);
  ClearCurrentRequest(NS_BINDING_ABORTED);

  
  nsCOMPtr<imgIRequest>& req = PrepareNextRequest();;
  nsresult rv = aRequest->Clone(this, getter_AddRefs(req));
  if (NS_SUCCEEDED(rv))
    TrackImage(req);
  else
    return rv;

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

nsCOMPtr<imgIRequest>&
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

nsCOMPtr<imgIRequest>&
nsImageLoadingContent::PrepareCurrentRequest()
{
  
  
  mImageBlockingStatus = nsIContentPolicy::ACCEPT;

  
  ClearCurrentRequest(NS_ERROR_IMAGE_SRC_CHANGED);

  if (mNewRequestsWillNeedAnimationReset) {
    mCurrentRequestFlags |= REQUEST_NEEDS_ANIMATION_RESET;
  }

  
  return mCurrentRequest;
}

nsCOMPtr<imgIRequest>&
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

  
  UntrackImage(mCurrentRequest);
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

  UntrackImage(mPendingRequest);
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

  if (mCurrentRequestFlags & REQUEST_SHOULD_BE_TRACKED)
    aDocument->AddImage(mCurrentRequest);
  if (mPendingRequestFlags & REQUEST_SHOULD_BE_TRACKED)
    aDocument->AddImage(mPendingRequest);
}

void
nsImageLoadingContent::UnbindFromTree(bool aDeep, bool aNullParent)
{
  
  nsCOMPtr<nsIDocument> doc = GetOurCurrentDoc();
  if (!doc)
    return;

  
  
  nsCxPusher pusher;
  pusher.PushNull();

  if (mCurrentRequestFlags & REQUEST_SHOULD_BE_TRACKED)
    doc->RemoveImage(mCurrentRequest);
  if (mPendingRequestFlags & REQUEST_SHOULD_BE_TRACKED)
    doc->RemoveImage(mPendingRequest);
}

nsresult
nsImageLoadingContent::TrackImage(imgIRequest* aImage)
{
  if (!aImage)
    return NS_OK;

  MOZ_ASSERT(aImage == mCurrentRequest || aImage == mPendingRequest,
             "Why haven't we heard of this request?");
  if (aImage == mCurrentRequest) {
    mCurrentRequestFlags |= REQUEST_SHOULD_BE_TRACKED;
  } else {
    mPendingRequestFlags |= REQUEST_SHOULD_BE_TRACKED;
  }

  nsIDocument* doc = GetOurCurrentDoc();
  if (doc)
    return doc->AddImage(aImage);
  return NS_OK;
}

nsresult
nsImageLoadingContent::UntrackImage(imgIRequest* aImage)
{
  if (!aImage)
    return NS_OK;

  MOZ_ASSERT(aImage == mCurrentRequest || aImage == mPendingRequest,
             "Why haven't we heard of this request?");
  if (aImage == mCurrentRequest) {
    mCurrentRequestFlags &= ~REQUEST_SHOULD_BE_TRACKED;
  } else {
    mPendingRequestFlags &= ~REQUEST_SHOULD_BE_TRACKED;
  }

  
  
  
  nsIDocument* doc = GetOurCurrentDoc();
  if (doc)
    return doc->RemoveImage(aImage);
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
