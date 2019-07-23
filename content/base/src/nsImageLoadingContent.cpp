












































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

#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIEventStateManager.h"
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
    mStartingLoad(PR_FALSE),
    mIsImageStateForced(PR_FALSE),
    mLoading(PR_FALSE),
    
    mBroken(PR_TRUE),
    mUserDisabled(PR_FALSE),
    mSuppressed(PR_FALSE),
    mBlockingOnload(PR_FALSE)
{
  if (!nsContentUtils::GetImgLoader()) {
    mLoadingEnabled = PR_FALSE;
  }
}

void
nsImageLoadingContent::DestroyImageLoadingContent()
{
  
  SetBlockingOnload(PR_FALSE);

  
  if (mCurrentRequest) {
    mCurrentRequest->CancelAndForgetObserver(NS_ERROR_FAILURE);
    mCurrentRequest = nsnull;
  }
  if (mPendingRequest) {
    mPendingRequest->CancelAndForgetObserver(NS_ERROR_FAILURE);
    mPendingRequest = nsnull;
  }
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
                                    nsIntRect* aDirtyRect)
{
  LOOP_OVER_OBSERVERS(FrameChanged(aContainer, aDirtyRect));
  return NS_OK;
}
            



NS_IMETHODIMP
nsImageLoadingContent::OnStartRequest(imgIRequest* aRequest)
{
  LOOP_OVER_OBSERVERS(OnStartRequest(aRequest));
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::OnStartDecode(imgIRequest* aRequest)
{
  
  if (aRequest == mCurrentRequest) {
    NS_ABORT_IF_FALSE(!mBlockingOnload, "Shouldn't already be blocking");
    SetBlockingOnload(PR_TRUE);
  }

  LOOP_OVER_OBSERVERS(OnStartDecode(aRequest));
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::OnStartContainer(imgIRequest* aRequest,
                                        imgIContainer* aContainer)
{
  LOOP_OVER_OBSERVERS(OnStartContainer(aRequest, aContainer));

  
  
  UpdateImageState(PR_TRUE);
  return NS_OK;    
}

NS_IMETHODIMP
nsImageLoadingContent::OnStartFrame(imgIRequest* aRequest,
                                    PRUint32 aFrame)
{
  LOOP_OVER_OBSERVERS(OnStartFrame(aRequest, aFrame));
  return NS_OK;    
}

NS_IMETHODIMP
nsImageLoadingContent::OnDataAvailable(imgIRequest* aRequest,
                                       PRBool aCurrentFrame,
                                       const nsIntRect* aRect)
{
  LOOP_OVER_OBSERVERS(OnDataAvailable(aRequest, aCurrentFrame, aRect));
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::OnStopFrame(imgIRequest* aRequest,
                                   PRUint32 aFrame)
{
  
  if (aRequest == mCurrentRequest)
    SetBlockingOnload(PR_FALSE);

  LOOP_OVER_OBSERVERS(OnStopFrame(aRequest, aFrame));
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::OnStopContainer(imgIRequest* aRequest,
                                       imgIContainer* aContainer)
{
  
  
  
  
  
  
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
  
  NS_ABORT_IF_FALSE(aRequest, "no request?");

  NS_PRECONDITION(aRequest == mCurrentRequest || aRequest == mPendingRequest,
                  "Unknown request");
  LOOP_OVER_OBSERVERS(OnStopDecode(aRequest, aStatus, aStatusArg));

  if (aRequest == mPendingRequest) {

    
    SetBlockingOnload(PR_FALSE);

    
    
    
    mCurrentRequest->Cancel(NS_ERROR_IMAGE_SRC_CHANGED);
    mPendingRequest.swap(mCurrentRequest);
    mPendingRequest = nsnull;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  nsIPresShell* shell = GetOurDocument()->GetPrimaryShell();
  if (shell) {

    
    PRBool doRequestDecode = PR_FALSE;

    
    
    if (!shell->DidInitialReflow())
      doRequestDecode = PR_TRUE;

    
    
    
    
    PRBool isSuppressed = PR_FALSE;
    nsresult rv = shell->IsPaintingSuppressed(&isSuppressed);
    if (NS_SUCCEEDED(rv) && isSuppressed)
      doRequestDecode = PR_TRUE;

    
    if (doRequestDecode)
      aRequest->RequestDecode();
  }

  
  
  

  if (NS_SUCCEEDED(aStatus)) {
    FireEvent(NS_LITERAL_STRING("load"));
  } else {
    FireEvent(NS_LITERAL_STRING("error"));
  }

  
  
  
  
  
  UpdateImageState(PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::OnStopRequest(imgIRequest* aRequest, PRBool aLastPart)
{
  LOOP_OVER_OBSERVERS(OnStopRequest(aRequest, aLastPart));

  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::OnDiscard(imgIRequest *aRequest)
{
  LOOP_OVER_OBSERVERS(OnDiscard(aRequest));

  return NS_OK;
}





NS_IMETHODIMP
nsImageLoadingContent::GetLoadingEnabled(PRBool *aLoadingEnabled)
{
  *aLoadingEnabled = mLoadingEnabled;
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::SetLoadingEnabled(PRBool aLoadingEnabled)
{
  if (nsContentUtils::GetImgLoader()) {
    mLoadingEnabled = aLoadingEnabled;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::GetImageBlockingStatus(PRInt16* aStatus)
{
  NS_PRECONDITION(aStatus, "Null out param");
  *aStatus = mImageBlockingStatus;
  return NS_OK;
}

NS_IMETHODIMP
nsImageLoadingContent::AddObserver(imgIDecoderObserver* aObserver)
{
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
    NS_WARNING("Asked to remove non-existent observer");
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
  NS_PRECONDITION(aListener, "null out param");
  
  NS_ENSURE_ARG_POINTER(aChannel);

  if (!nsContentUtils::GetImgLoader()) {
    return NS_ERROR_NULL_POINTER;
  }

  
  
  
  
  nsCOMPtr<nsIDocument> doc = GetOurDocument();
  if (!doc) {
    
    return NS_OK;
  }

  
  mCurrentURI = nsnull;
  
  CancelImageRequests(NS_ERROR_IMAGE_SRC_CHANGED, PR_FALSE,
                      nsIContentPolicy::ACCEPT);

  nsCOMPtr<imgIRequest> & req = mCurrentRequest ? mPendingRequest : mCurrentRequest;

  nsresult rv = nsContentUtils::GetImgLoader()->
    LoadImageWithChannel(aChannel, this, doc, aListener, getter_AddRefs(req));

  
  UpdateImageState(PR_TRUE);

  return rv;
}

NS_IMETHODIMP nsImageLoadingContent::ForceReload()
{
  nsCOMPtr<nsIURI> currentURI;
  GetCurrentURI(getter_AddRefs(currentURI));
  if (!currentURI) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return LoadImage(currentURI, PR_TRUE, PR_TRUE, nsnull, nsIRequest::VALIDATE_ALWAYS);
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


  nsresult rv;   

  
  
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
  NS_ASSERTION(thisContent &&
               thisContent->NodePrincipal() == aDocument->NodePrincipal(),
               "Principal mismatch?");
#endif
  
  
  
  
  
  
  
  

  PRInt16 newImageStatus;
  PRBool loadImage = nsContentUtils::CanLoadImage(aNewURI, this, aDocument,
                                                  aDocument->NodePrincipal(),
                                                  &newImageStatus);
  NS_ASSERTION(loadImage || !NS_CP_ACCEPTED(newImageStatus),
               "CanLoadImage lied");

  nsresult cancelResult = loadImage ? NS_ERROR_IMAGE_SRC_CHANGED
                                    : NS_ERROR_IMAGE_BLOCKED;

  CancelImageRequests(cancelResult, PR_FALSE, newImageStatus);

  
  
  
  
  if (!mCurrentRequest) {
    mCurrentURI = aNewURI;
  }
  
  if (!loadImage) {
    
    FireEvent(NS_LITERAL_STRING("error"));
    return NS_OK;
  }

  nsCOMPtr<imgIRequest> & req = mCurrentRequest ? mPendingRequest : mCurrentRequest;

  rv = nsContentUtils::LoadImage(aNewURI, aDocument,
                                 aDocument->NodePrincipal(),
                                 aDocument->GetDocumentURI(),
                                 this, aLoadFlags,
                                 getter_AddRefs(req));
  if (NS_FAILED(rv)) {
    FireEvent(NS_LITERAL_STRING("error"));
    return NS_OK;
  }

  
  
  if (mCurrentRequest) {
    mCurrentURI = nsnull;
  }

  return NS_OK;
}

nsresult
nsImageLoadingContent::ForceImageState(PRBool aForce, PRInt32 aState)
{
  mIsImageStateForced = aForce;
  mForcedImageState = aState;
  return NS_OK;
}

PRInt32
nsImageLoadingContent::ImageState() const
{
  return mIsImageStateForced ? mForcedImageState :
    (mBroken * NS_EVENT_STATE_BROKEN) |
    (mUserDisabled * NS_EVENT_STATE_USERDISABLED) |
    (mSuppressed * NS_EVENT_STATE_SUPPRESSED) |
    (mLoading * NS_EVENT_STATE_LOADING);
}

void
nsImageLoadingContent::UpdateImageState(PRBool aNotify)
{
  if (mStartingLoad) {
    
    
    
    
    
    return;
  }
  
  nsCOMPtr<nsIContent> thisContent = do_QueryInterface(this);
  if (!thisContent) {
    return;
  }

  PRInt32 oldState = ImageState();

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
      PRInt32 changedBits = oldState ^ ImageState();
      if (changedBits) {
        mozAutoDocUpdate upd(doc, UPDATE_CONTENT_STATE, PR_TRUE);
        doc->ContentStatesChanged(thisContent, nsnull, changedBits);
      }
    }
  }
}

void
nsImageLoadingContent::CancelImageRequests(PRBool aNotify)
{
  
  AutoStateChanger changer(this, aNotify);
  mCurrentURI = nsnull;
  CancelImageRequests(NS_BINDING_ABORTED, PR_TRUE, nsIContentPolicy::ACCEPT);
}

void
nsImageLoadingContent::CancelImageRequests(nsresult aReason,
                                           PRBool   aEvenIfSizeAvailable,
                                           PRInt16  aNewImageStatus)
{
  
  if (mPendingRequest) {
    mPendingRequest->Cancel(aReason);
    mPendingRequest = nsnull;
  }

  
  
  if (mCurrentRequest) {
    PRUint32 loadStatus = imgIRequest::STATUS_ERROR;
    mCurrentRequest->GetImageStatus(&loadStatus);

    NS_ASSERTION(NS_CP_ACCEPTED(mImageBlockingStatus),
                 "Have current request but blocked image?");
    
    if (aEvenIfSizeAvailable ||
        !(loadStatus & imgIRequest::STATUS_SIZE_AVAILABLE)) {
      
      
      
      

      
      SetBlockingOnload(PR_FALSE);

      
      mImageBlockingStatus = aNewImageStatus;
      mCurrentRequest->Cancel(aReason);
      mCurrentRequest = nsnull;
    }
  } else {
    
    
    mImageBlockingStatus = aNewImageStatus;
  }

  
  
  
  
  
  
  
}

nsresult
nsImageLoadingContent::UseAsPrimaryRequest(imgIRequest* aRequest,
                                           PRBool aNotify)
{
  
  
  
  
  NS_PRECONDITION(aRequest, "Must have a request here!");
  AutoStateChanger changer(this, aNotify);
  mCurrentURI = nsnull;
  CancelImageRequests(NS_BINDING_ABORTED, PR_TRUE, nsIContentPolicy::ACCEPT);

  NS_ASSERTION(!mCurrentRequest, "We should not have a current request now");

  return aRequest->Clone(this, getter_AddRefs(mCurrentRequest));
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






class nsImageLoadingContent::Event : public nsRunnable
{
public:
  Event(nsPresContext* aPresContext, nsImageLoadingContent* aContent,
             const nsAString& aMessage, nsIDocument* aDocument)
    : mPresContext(aPresContext),
      mContent(aContent),
      mMessage(aMessage),
      mDocument(aDocument)
  {
  }
  ~Event()
  {
    mDocument->UnblockOnload(PR_TRUE);
  }

  NS_IMETHOD Run();
  
  nsCOMPtr<nsPresContext> mPresContext;
  nsRefPtr<nsImageLoadingContent> mContent;
  nsString mMessage;
  
  
  
  nsCOMPtr<nsIDocument> mDocument;
};

NS_IMETHODIMP
nsImageLoadingContent::Event::Run()
{
  PRUint32 eventMsg;

  if (mMessage.EqualsLiteral("load")) {
    eventMsg = NS_LOAD;
  } else {
    eventMsg = NS_LOAD_ERROR;
  }

  nsCOMPtr<nsIContent> ourContent = do_QueryInterface(mContent);

  nsEvent event(PR_TRUE, eventMsg);
  event.flags |= NS_EVENT_FLAG_CANT_BUBBLE;
  nsEventDispatcher::Dispatch(ourContent, mPresContext, &event);

  return NS_OK;
}

nsresult
nsImageLoadingContent::FireEvent(const nsAString& aEventType)
{
  
  
  

  nsCOMPtr<nsIDocument> document = GetOurDocument();
  if (!document) {
    
    return NS_OK;
  }                                                                             

  
  NS_ASSERTION(NS_IsMainThread(), "should be on the main thread");

  nsIPresShell *shell = document->GetPrimaryShell();
  nsPresContext *presContext = shell ? shell->GetPresContext() : nsnull;

  nsCOMPtr<nsIRunnable> evt =
      new nsImageLoadingContent::Event(presContext, this, aEventType, document);
  NS_ENSURE_TRUE(evt, NS_ERROR_OUT_OF_MEMORY);

  
  
  document->BlockOnload();
  
  return NS_DispatchToCurrentThread(evt);
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
