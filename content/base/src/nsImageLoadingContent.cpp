













































#include "nsImageLoadingContent.h"
#include "nsAutoPtr.h"
#include "nsContentErrors.h"
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
#include "nsPLDOMEvent.h"

#include "nsIPresShell.h"
#include "nsEventStates.h"
#include "nsGUIEvent.h"

#include "nsIChannel.h"
#include "nsIStreamListener.h"

#include "nsIFrame.h"
#include "nsIDOMNode.h"

#include "nsContentUtils.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsEventDispatcher.h"
#include "nsDOMClassInfo.h"
#ifdef MOZ_SVG
#include "nsSVGEffects.h"
#endif

#include "mozAutoDocUpdate.h"

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

  nsCAutoString spec;
  uri->GetSpec(spec);
  printf("spec='%s'\n", spec.get());
}
#endif 


nsImageLoadingContent::nsImageLoadingContent()
  : mObserverList(nsnull),
    mImageBlockingStatus(nsIContentPolicy::ACCEPT),
    mLoadingEnabled(PR_TRUE),
    mIsImageStateForced(PR_FALSE),
    mLoading(PR_FALSE),
    
    mBroken(PR_TRUE),
    mUserDisabled(PR_FALSE),
    mSuppressed(PR_FALSE),
    mBlockingOnload(PR_FALSE),
    mNewRequestsWillNeedAnimationReset(PR_FALSE),
    mPendingRequestNeedsResetAnimation(PR_FALSE),
    mCurrentRequestNeedsResetAnimation(PR_FALSE),
    mStateChangerDepth(0)
{
  if (!nsContentUtils::GetImgLoader()) {
    mLoadingEnabled = PR_FALSE;
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
nsImageLoadingContent::FrameChanged(imgIContainer* aContainer,
                                    const nsIntRect* aDirtyRect)
{
  LOOP_OVER_OBSERVERS(FrameChanged(aContainer, aDirtyRect));
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

  
  if (aRequest == mCurrentRequest) {

    
    
    PRUint32 loadFlags;
    nsresult rv = aRequest->GetLoadFlags(&loadFlags);
    PRBool background =
      (NS_SUCCEEDED(rv) && (loadFlags & nsIRequest::LOAD_BACKGROUND));

    
    if (!background) {
      NS_ABORT_IF_FALSE(!mBlockingOnload, "Shouldn't already be blocking");
      SetBlockingOnload(PR_TRUE);
    }
  }

  LOOP_OVER_OBSERVERS(OnStartDecode(aRequest));
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::OnStartContainer(imgIRequest* aRequest,
                                        imgIContainer* aContainer)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  LOOP_OVER_OBSERVERS(OnStartContainer(aRequest, aContainer));

  
  
  UpdateImageState(PR_TRUE);
  return NS_OK;    
}

NS_IMETHODIMP
nsImageLoadingContent::OnStartFrame(imgIRequest* aRequest,
                                    PRUint32 aFrame)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  LOOP_OVER_OBSERVERS(OnStartFrame(aRequest, aFrame));
  return NS_OK;    
}

NS_IMETHODIMP
nsImageLoadingContent::OnDataAvailable(imgIRequest* aRequest,
                                       PRBool aCurrentFrame,
                                       const nsIntRect* aRect)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  LOOP_OVER_OBSERVERS(OnDataAvailable(aRequest, aCurrentFrame, aRect));
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::OnStopFrame(imgIRequest* aRequest,
                                   PRUint32 aFrame)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  
  if (aRequest == mCurrentRequest)
    SetBlockingOnload(PR_FALSE);

  LOOP_OVER_OBSERVERS(OnStopFrame(aRequest, aFrame));
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::OnStopContainer(imgIRequest* aRequest,
                                       imgIContainer* aContainer)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  
  
  
  
  
  
  if (aRequest == mCurrentRequest)
    SetBlockingOnload(PR_FALSE);

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

  
  

  
  AutoStateChanger changer(this, PR_TRUE);

  
  if (aRequest == mPendingRequest) {
    PrepareCurrentRequest() = mPendingRequest;
    mPendingRequest = nsnull;
    mCurrentRequestNeedsResetAnimation = mPendingRequestNeedsResetAnimation;
    mPendingRequestNeedsResetAnimation = PR_FALSE;
  }
  NS_ABORT_IF_FALSE(aRequest == mCurrentRequest,
                    "One way or another, we should be current by now");

  if (mCurrentRequestNeedsResetAnimation) {
    nsCOMPtr<imgIContainer> container;
    mCurrentRequest->GetImage(getter_AddRefs(container));
    if (container)
      container->ResetAnimation();
    mCurrentRequestNeedsResetAnimation = PR_FALSE;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  

  
  nsIDocument* doc = GetOurDocument();
  nsIPresShell* shell = doc ? doc->GetShell() : nsnull;
  if (shell) {

    
    PRBool doRequestDecode = PR_FALSE;

    
    
    if (!shell->DidInitialReflow())
      doRequestDecode = PR_TRUE;

    
    
    
    
    if (shell->IsPaintingSuppressed())
      doRequestDecode = PR_TRUE;

    
    if (doRequestDecode)
      mCurrentRequest->RequestDecode();
  }

  
  if (NS_SUCCEEDED(aStatus)) {
    FireEvent(NS_LITERAL_STRING("load"));
  } else {
    FireEvent(NS_LITERAL_STRING("error"));
  }

#ifdef MOZ_SVG
  nsCOMPtr<nsINode> thisNode = do_QueryInterface(this);
  nsSVGEffects::InvalidateDirectRenderingObservers(thisNode->AsElement());
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::OnStopRequest(imgIRequest* aRequest, PRBool aLastPart)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  LOOP_OVER_OBSERVERS(OnStopRequest(aRequest, aLastPart));

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
nsImageLoadingContent::GetLoadingEnabled(PRBool *aLoadingEnabled)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  *aLoadingEnabled = mLoadingEnabled;
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::SetLoadingEnabled(PRBool aLoadingEnabled)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  if (nsContentUtils::GetImgLoader()) {
    mLoadingEnabled = aLoadingEnabled;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::GetImageBlockingStatus(PRInt16* aStatus)
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
    mObserverList.mObserver = nsnull;
    
    return NS_OK;
  }

  
  ImageObserver* observer = &mObserverList;
  while (observer->mNext && observer->mNext->mObserver != aObserver) {
    observer = observer->mNext;
  }

  
  
  if (observer->mNext) {
    
    ImageObserver* oldObserver = observer->mNext;
    observer->mNext = oldObserver->mNext;
    oldObserver->mNext = nsnull;  
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
nsImageLoadingContent::GetRequest(PRInt32 aRequestType,
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
    *aRequest = nsnull;
    return NS_ERROR_UNEXPECTED;
  }
  
  NS_IF_ADDREF(*aRequest);
  return NS_OK;
}


NS_IMETHODIMP
nsImageLoadingContent::GetRequestType(imgIRequest* aRequest,
                                      PRInt32* aRequestType)
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
    *aURI = nsnull;
    return NS_OK;
  }
  
  return NS_EnsureSafeToReturn(mCurrentURI, aURI);
}

NS_IMETHODIMP
nsImageLoadingContent::LoadImageWithChannel(nsIChannel* aChannel,
                                            nsIStreamListener** aListener)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);

  if (!nsContentUtils::GetImgLoader()) {
    return NS_ERROR_NULL_POINTER;
  }

  nsCOMPtr<nsIDocument> doc = GetOurDocument();
  if (!doc) {
    
    return NS_OK;
  }

  
  
  

  
  AutoStateChanger changer(this, PR_TRUE);

  
  nsCOMPtr<imgIRequest>& req = PrepareNextRequest();
  nsresult rv = nsContentUtils::GetImgLoader()->
    LoadImageWithChannel(aChannel, this, doc, aListener,
                         getter_AddRefs(req));
  if (NS_SUCCEEDED(rv)) {
    TrackImage(req);
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

  return LoadImage(currentURI, PR_TRUE, PR_TRUE, nsnull, nsIRequest::VALIDATE_ALWAYS);
}





void
nsImageLoadingContent::NotifyOwnerDocumentChanged(nsIDocument *aOldDoc)
{
  
  if (aOldDoc) {
    if (mCurrentRequest)
      aOldDoc->RemoveImage(mCurrentRequest);
    if (mPendingRequest)
      aOldDoc->RemoveImage(mPendingRequest);
  }

  
  TrackImage(mCurrentRequest);
  TrackImage(mPendingRequest);
}

nsresult
nsImageLoadingContent::LoadImage(const nsAString& aNewURI,
                                 PRBool aForce,
                                 PRBool aNotify)
{
  
  nsIDocument* doc = GetOurDocument();
  if (!doc) {
    
    return NS_OK;
  }

  nsCOMPtr<nsIURI> imageURI;
  nsresult rv = StringToURI(aNewURI, doc, getter_AddRefs(imageURI));
  NS_ENSURE_SUCCESS(rv, rv);
  

  PRBool equal;

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
                                 PRBool aForce,
                                 PRBool aNotify,
                                 nsIDocument* aDocument,
                                 nsLoadFlags aLoadFlags)
{
  if (!mLoadingEnabled) {
    
    
    FireEvent(NS_LITERAL_STRING("error"));
    return NS_OK;
  }

  NS_ASSERTION(!aDocument || aDocument == GetOurDocument(),
               "Bogus document passed in");
  
  if (!aDocument) {
    aDocument = GetOurDocument();
    if (!aDocument) {
      
      return NS_OK;
    }
  }

  
  
  
  
  if (!aForce && NS_CP_ACCEPTED(mImageBlockingStatus)) {
    nsCOMPtr<nsIURI> currentURI;
    GetCurrentURI(getter_AddRefs(currentURI));
    PRBool equal;
    if (currentURI &&
        NS_SUCCEEDED(currentURI->Equals(aNewURI, &equal)) &&
        equal) {
      
      return NS_OK;
    }
  }

  
  AutoStateChanger changer(this, aNotify);

  
  
  
  
#ifdef DEBUG
  nsCOMPtr<nsIContent> thisContent = do_QueryInterface(this);
  NS_ABORT_IF_FALSE(thisContent &&
                    thisContent->NodePrincipal() == aDocument->NodePrincipal(),
                    "Principal mismatch?");
#endif

  
  PRInt16 cpDecision = nsIContentPolicy::REJECT_REQUEST;
  nsContentUtils::CanLoadImage(aNewURI, this, aDocument,
                               aDocument->NodePrincipal(), &cpDecision);
  if (!NS_CP_ACCEPTED(cpDecision)) {
    FireEvent(NS_LITERAL_STRING("error"));
    SetBlockedRequest(aNewURI, cpDecision);
    return NS_OK;
  }

  
  nsCOMPtr<imgIRequest>& req = PrepareNextRequest();
  nsresult rv;
  rv = nsContentUtils::LoadImage(aNewURI, aDocument,
                                 aDocument->NodePrincipal(),
                                 aDocument->GetDocumentURI(),
                                 this, aLoadFlags,
                                 getter_AddRefs(req));
  if (NS_SUCCEEDED(rv)) {
    TrackImage(req);
  } else {
    
    
    if (!mCurrentRequest)
      mCurrentURI = aNewURI;
    FireEvent(NS_LITERAL_STRING("error"));
    return NS_OK;
  }

  return NS_OK;
}

nsresult
nsImageLoadingContent::ForceImageState(PRBool aForce, nsEventStates::InternalType aState)
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
nsImageLoadingContent::UpdateImageState(PRBool aNotify)
{
  if (mStateChangerDepth > 0) {
    
    
    
    
    
    
    return;
  }
  
  nsCOMPtr<nsIContent> thisContent = do_QueryInterface(this);
  if (!thisContent) {
    return;
  }

  nsEventStates oldState = ImageState();

  mLoading = mBroken = mUserDisabled = mSuppressed = PR_FALSE;
  
  
  
  
  if (mImageBlockingStatus == nsIContentPolicy::REJECT_SERVER) {
    mSuppressed = PR_TRUE;
  } else if (mImageBlockingStatus == nsIContentPolicy::REJECT_TYPE) {
    mUserDisabled = PR_TRUE;
  } else if (!mCurrentRequest) {
    
    mBroken = PR_TRUE;
  } else {
    PRUint32 currentLoadStatus;
    nsresult rv = mCurrentRequest->GetImageStatus(&currentLoadStatus);
    if (NS_FAILED(rv) || (currentLoadStatus & imgIRequest::STATUS_ERROR)) {
      mBroken = PR_TRUE;
    } else if (!(currentLoadStatus & imgIRequest::STATUS_SIZE_AVAILABLE)) {
      mLoading = PR_TRUE;
    }
  }

  if (aNotify) {
    nsIDocument* doc = thisContent->GetCurrentDoc();
    if (doc) {
      NS_ASSERTION(thisContent->IsInDoc(), "Something is confused");
      nsEventStates changedBits = oldState ^ ImageState();
      if (!changedBits.IsEmpty()) {
        mozAutoDocUpdate upd(doc, UPDATE_CONTENT_STATE, PR_TRUE);
        doc->ContentStateChanged(thisContent, changedBits);
      }
    }
  }
}

void
nsImageLoadingContent::CancelImageRequests(PRBool aNotify)
{
  AutoStateChanger changer(this, aNotify);
  ClearPendingRequest(NS_BINDING_ABORTED);
  ClearCurrentRequest(NS_BINDING_ABORTED);
}

nsresult
nsImageLoadingContent::UseAsPrimaryRequest(imgIRequest* aRequest,
                                           PRBool aNotify)
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
nsImageLoadingContent::GetOurDocument()
{
  nsCOMPtr<nsIContent> thisContent = do_QueryInterface(this);
  NS_ENSURE_TRUE(thisContent, nsnull);

  return thisContent->GetOwnerDoc();
}

nsresult
nsImageLoadingContent::StringToURI(const nsAString& aSpec,
                                   nsIDocument* aDocument,
                                   nsIURI** aURI)
{
  NS_PRECONDITION(aDocument, "Must have a document");
  NS_PRECONDITION(aURI, "Null out param");

  
  nsCOMPtr<nsIContent> thisContent = do_QueryInterface(this);
  NS_ASSERTION(thisContent, "An image loading content must be an nsIContent");
  nsCOMPtr<nsIURI> baseURL = thisContent->GetBaseURI();

  
  const nsAFlatCString &charset = aDocument->GetDocumentCharacterSet();

  
  return NS_NewURI(aURI,
                   aSpec,
                   charset.IsEmpty() ? nsnull : charset.get(),
                   baseURL,
                   nsContentUtils::GetIOService());
}

nsresult
nsImageLoadingContent::FireEvent(const nsAString& aEventType)
{
  
  
  

  nsCOMPtr<nsINode> thisNode = do_QueryInterface(this);

  nsRefPtr<nsPLDOMEvent> event =
    new nsLoadBlockingPLDOMEvent(thisNode, aEventType, PR_FALSE, PR_FALSE);
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
nsImageLoadingContent::SetBlockedRequest(nsIURI* aURI, PRInt16 aContentDecision)
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

  mCurrentRequestNeedsResetAnimation = mNewRequestsWillNeedAnimationReset;

  
  return mCurrentRequest;
}

nsCOMPtr<imgIRequest>&
nsImageLoadingContent::PreparePendingRequest()
{
  
  ClearPendingRequest(NS_ERROR_IMAGE_SRC_CHANGED);

  mPendingRequestNeedsResetAnimation = mNewRequestsWillNeedAnimationReset;

  
  return mPendingRequest;
}

void
nsImageLoadingContent::ClearCurrentRequest(nsresult aReason)
{
  if (!mCurrentRequest) {
    
    
    mCurrentURI = nsnull;
    return;
  }
  NS_ABORT_IF_FALSE(!mCurrentURI,
                    "Shouldn't have both mCurrentRequest and mCurrentURI!");

  
  UntrackImage(mCurrentRequest);
  mCurrentRequest->CancelAndForgetObserver(aReason);
  mCurrentRequest = nsnull;
  mCurrentRequestNeedsResetAnimation = PR_FALSE;

  
  
  SetBlockingOnload(PR_FALSE);
}

void
nsImageLoadingContent::ClearPendingRequest(nsresult aReason)
{
  if (!mPendingRequest)
    return;

  
  
  
  nsCxPusher pusher;
  pusher.PushNull();

  UntrackImage(mPendingRequest);
  mPendingRequest->CancelAndForgetObserver(aReason);
  mPendingRequest = nsnull;
  mPendingRequestNeedsResetAnimation = PR_FALSE;
}

bool
nsImageLoadingContent::HaveSize(imgIRequest *aImage)
{
  
  if (!aImage)
    return false;

  
  PRUint32 status;
  nsresult rv = aImage->GetImageStatus(&status);
  return (NS_SUCCEEDED(rv) && (status & imgIRequest::STATUS_SIZE_AVAILABLE));
}

void
nsImageLoadingContent::SetBlockingOnload(PRBool aBlocking)
{
  
  if (mBlockingOnload == aBlocking)
    return;

  
  nsIDocument* doc = GetOurDocument();

  if (doc) {
    
    if (aBlocking)
      doc->BlockOnload();
    else
      doc->UnblockOnload(PR_FALSE);

    
    mBlockingOnload = aBlocking;
  }
}

nsresult
nsImageLoadingContent::TrackImage(imgIRequest* aImage)
{
  if (!aImage)
    return NS_OK;

  nsIDocument* doc = GetOurDocument();
  if (doc)
    return doc->AddImage(aImage);
  return NS_OK;
}

nsresult
nsImageLoadingContent::UntrackImage(imgIRequest* aImage)
{
  if (!aImage)
    return NS_OK;

  
  
  
  nsIDocument* doc = GetOurDocument();
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

