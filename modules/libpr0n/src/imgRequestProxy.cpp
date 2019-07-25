






































#include "imgRequestProxy.h"

#include "nsIInputStream.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIMultiPartChannel.h"

#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"

#include "ImageErrors.h"
#include "ImageLogging.h"

#include "nspr.h"

NS_IMPL_ISUPPORTS4(imgRequestProxy, imgIRequest, nsIRequest,
                   nsISupportsPriority, nsISecurityInfoProvider)

imgRequestProxy::imgRequestProxy() :
  mOwner(nsnull),
  mURI(nsnull),
  mImage(nsnull),
  mPrincipal(nsnull),
  mImageStatus(0),
  mState(0),
  mListener(nsnull),
  mLoadFlags(nsIRequest::LOAD_NORMAL),
  mLocksHeld(0),
  mCanceled(PR_FALSE),
  mIsInLoadGroup(PR_FALSE),
  mListenerIsStrongRef(PR_FALSE),
  mDecodeRequested(PR_FALSE),
  mHadLastPart(PR_FALSE)
{
  

}

imgRequestProxy::~imgRequestProxy()
{
  
  NS_PRECONDITION(!mListener, "Someone forgot to properly cancel this request!");

  
  
  if (mOwner) {
    while (mLocksHeld)
      UnlockImage();
  }

  
  
  
  
  NullOutListener();

  if (mOwner) {
    if (!mCanceled) {
      mCanceled = PR_TRUE;

      







      mOwner->RemoveProxy(this, NS_OK, PR_FALSE);
    }
  }
}

nsresult imgRequestProxy::Init(imgRequest* request, nsILoadGroup* aLoadGroup, nsIURI* aURI,
                               imgIDecoderObserver* aObserver)
{
  NS_PRECONDITION(!mOwner && !mListener, "imgRequestProxy is already initialized");
  NS_PRECONDITION(request, "no request");
  if (!request)
    return NS_ERROR_NULL_POINTER;

  LOG_SCOPE_WITH_PARAM(gImgLog, "imgRequestProxy::Init", "request", request);

  mOwner = request;
  mListener = aObserver;
  
  
  
  if (mListener) {
    mListenerIsStrongRef = PR_TRUE;
    NS_ADDREF(mListener);
  }
  mLoadGroup = aLoadGroup;
  mURI = aURI;

  
  request->AddProxy(this);

  return NS_OK;
}

nsresult imgRequestProxy::ChangeOwner(imgRequest *aNewOwner)
{
  NS_PRECONDITION(mOwner, "Cannot ChangeOwner on a proxy without an owner!");

  if (mCanceled)
    return NS_OK;

  
  PRBool wasDecoded = PR_FALSE;
  if (mOwner->GetImageStatus() & imgIRequest::STATUS_FRAME_COMPLETE)
    wasDecoded = PR_TRUE;

  
  
  PRUint32 oldLockCount = mLocksHeld;
  while (mLocksHeld)
    UnlockImage();

  
  
  mOwner->RemoveProxy(this, NS_IMAGELIB_CHANGING_OWNER, PR_FALSE);

  mOwner = aNewOwner;

  mOwner->AddProxy(this);

  
  
  if (wasDecoded || mDecodeRequested)
    mOwner->RequestDecode();

  
  for (PRUint32 i = 0; i < oldLockCount; i++)
    LockImage();

  return NS_OK;
}

void imgRequestProxy::AddToLoadGroup()
{
  NS_ASSERTION(!mIsInLoadGroup, "Whaa, we're already in the loadgroup!");

  if (!mIsInLoadGroup && mLoadGroup) {
    mLoadGroup->AddRequest(this, nsnull);
    mIsInLoadGroup = PR_TRUE;
  }
}

void imgRequestProxy::RemoveFromLoadGroup(PRBool releaseLoadGroup)
{
  if (!mIsInLoadGroup)
    return;

  




  nsCOMPtr<imgIRequest> kungFuDeathGrip(this);

  mLoadGroup->RemoveRequest(this, nsnull, NS_OK);
  mIsInLoadGroup = PR_FALSE;

  if (releaseLoadGroup) {
    
    mLoadGroup = nsnull;
  }
}





NS_IMETHODIMP imgRequestProxy::GetName(nsACString &aName)
{
  aName.Truncate();

  if (mURI)
    mURI->GetSpec(aName);

  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::IsPending(PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP imgRequestProxy::GetStatus(nsresult *aStatus)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP imgRequestProxy::Cancel(nsresult status)
{
  if (mCanceled || !mOwner)
    return NS_ERROR_FAILURE;

  LOG_SCOPE(gImgLog, "imgRequestProxy::Cancel");

  mCanceled = PR_TRUE;

  nsCOMPtr<nsIRunnable> ev = new imgCancelRunnable(this, status);
  return NS_DispatchToCurrentThread(ev);
}

void
imgRequestProxy::DoCancel(nsresult status)
{
  
  
  mOwner->RemoveProxy(this, status, PR_FALSE);

  NullOutListener();
}


NS_IMETHODIMP imgRequestProxy::CancelAndForgetObserver(nsresult aStatus)
{
  if (mCanceled || !mOwner)
    return NS_ERROR_FAILURE;

  LOG_SCOPE(gImgLog, "imgRequestProxy::CancelAndForgetObserver");

  mCanceled = PR_TRUE;

  
  PRBool oldIsInLoadGroup = mIsInLoadGroup;
  mIsInLoadGroup = PR_FALSE;
  
  
  
  mOwner->RemoveProxy(this, aStatus, PR_FALSE);

  mIsInLoadGroup = oldIsInLoadGroup;

  if (mIsInLoadGroup) {
    nsCOMPtr<nsIRunnable> ev =
      NS_NewRunnableMethod(this, &imgRequestProxy::DoRemoveFromLoadGroup);
    NS_DispatchToCurrentThread(ev);
  }

  NullOutListener();

  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::RequestDecode()
{
  if (!mOwner)
    return NS_ERROR_FAILURE;

  
  mDecodeRequested = PR_TRUE;

  
  return mOwner->RequestDecode();
}


NS_IMETHODIMP
imgRequestProxy::LockImage()
{
  if (!mImage)
    return NS_ERROR_FAILURE;

  mLocksHeld++;

  return mImage->LockImage();
}


NS_IMETHODIMP
imgRequestProxy::UnlockImage()
{
  if (!mImage)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(mLocksHeld > 0, "calling unlock but no locks!");
  mLocksHeld--;

  return mImage->UnlockImage();
}


NS_IMETHODIMP imgRequestProxy::Suspend()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP imgRequestProxy::Resume()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP imgRequestProxy::GetLoadGroup(nsILoadGroup **loadGroup)
{
  NS_IF_ADDREF(*loadGroup = mLoadGroup.get());
  return NS_OK;
}
NS_IMETHODIMP imgRequestProxy::SetLoadGroup(nsILoadGroup *loadGroup)
{
  mLoadGroup = loadGroup;
  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetLoadFlags(nsLoadFlags *flags)
{
  *flags = mLoadFlags;
  return NS_OK;
}
NS_IMETHODIMP imgRequestProxy::SetLoadFlags(nsLoadFlags flags)
{
  mLoadFlags = flags;
  return NS_OK;
}




NS_IMETHODIMP imgRequestProxy::GetImage(imgIContainer * *aImage)
{
  if (!mImage)
    return NS_ERROR_FAILURE;

  NS_ADDREF(*aImage = mImage);

  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetImageStatus(PRUint32 *aStatus)
{
  if (!mOwner && !mImage) {
    *aStatus = imgIRequest::STATUS_ERROR;
    return NS_ERROR_FAILURE;
  }

  if (mOwner) {
    *aStatus = mOwner->GetImageStatus();
    return NS_OK;
  }

  
  
  *aStatus = mImageStatus;
  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetURI(nsIURI **aURI)
{
  if (!mURI)
    return NS_ERROR_FAILURE;

  NS_ADDREF(*aURI = mURI);

  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetDecoderObserver(imgIDecoderObserver **aDecoderObserver)
{
  *aDecoderObserver = mListener;
  NS_IF_ADDREF(*aDecoderObserver);
  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetMimeType(char **aMimeType)
{
  if (!mOwner)
    return NS_ERROR_FAILURE;

  const char *type = mOwner->GetMimeType();
  if (!type)
    return NS_ERROR_FAILURE;

  *aMimeType = NS_strdup(type);

  return NS_OK;
}

NS_IMETHODIMP imgRequestProxy::Clone(imgIDecoderObserver* aObserver,
                                     imgIRequest** aClone)
{
  NS_PRECONDITION(aClone, "Null out param");
  *aClone = nsnull;
  nsRefPtr<imgRequestProxy> clone = new imgRequestProxy();

  
  
  
  
  
  
  clone->SetLoadFlags(mLoadFlags);
  nsresult rv = clone->Init(mOwner, mLoadGroup, mURI, aObserver);
  if (NS_FAILED(rv))
    return rv;

  clone->mHadLastPart = mHadLastPart;
  clone->SetImage(mImage);
  clone->SetPrincipal(mPrincipal);

  
  PRUint32 imageStatus = 0;
  GetImageStatus(&imageStatus);
  PRUint32 state = 0;
  GetState(&state);
  clone->mImageStatus = imageStatus;
  clone->mState = state;

  if (NS_FAILED(rv))
    return rv;

  
  
  
  *aClone = clone.forget().get();

  
  static_cast<imgRequestProxy*>(*aClone)->NotifyListener();

  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetImagePrincipal(nsIPrincipal **aPrincipal)
{
  if (!mPrincipal)
    return NS_ERROR_FAILURE;

  NS_ADDREF(*aPrincipal = mPrincipal);

  return NS_OK;
}



NS_IMETHODIMP imgRequestProxy::GetPriority(PRInt32 *priority)
{
  NS_ENSURE_STATE(mOwner);
  *priority = mOwner->Priority();
  return NS_OK;
}

NS_IMETHODIMP imgRequestProxy::SetPriority(PRInt32 priority)
{
  NS_ENSURE_STATE(mOwner && !mCanceled);
  mOwner->AdjustPriority(this, priority - mOwner->Priority());
  return NS_OK;
}

NS_IMETHODIMP imgRequestProxy::AdjustPriority(PRInt32 priority)
{
  NS_ENSURE_STATE(mOwner && !mCanceled);
  mOwner->AdjustPriority(this, priority);
  return NS_OK;
}



NS_IMETHODIMP imgRequestProxy::GetSecurityInfo(nsISupports** _retval)
{
  if (mOwner)
    return mOwner->GetSecurityInfo(_retval);

  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP imgRequestProxy::GetHasTransferredData(PRBool* hasData)
{
  if (mOwner) {
    *hasData = mOwner->HasTransferredData();
  } else {
    
    *hasData = PR_TRUE;
  }
  return NS_OK;
}



void imgRequestProxy::FrameChanged(imgIContainer *container, nsIntRect * dirtyRect)
{
  LOG_FUNC(gImgLog, "imgRequestProxy::FrameChanged");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->FrameChanged(container, dirtyRect);
  }
}



void imgRequestProxy::OnStartDecode()
{
  LOG_FUNC(gImgLog, "imgRequestProxy::OnStartDecode");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnStartDecode(this);
  }
}

void imgRequestProxy::OnStartContainer(imgIContainer *image)
{
  LOG_FUNC(gImgLog, "imgRequestProxy::OnStartContainer");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnStartContainer(this, image);
  }
}

void imgRequestProxy::OnStartFrame(PRUint32 frame)
{
  LOG_FUNC(gImgLog, "imgRequestProxy::OnStartFrame");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnStartFrame(this, frame);
  }
}

void imgRequestProxy::OnDataAvailable(PRBool aCurrentFrame, const nsIntRect * rect)
{
  LOG_FUNC(gImgLog, "imgRequestProxy::OnDataAvailable");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnDataAvailable(this, aCurrentFrame, rect);
  }
}

void imgRequestProxy::OnStopFrame(PRUint32 frame)
{
  LOG_FUNC(gImgLog, "imgRequestProxy::OnStopFrame");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnStopFrame(this, frame);
  }
}

void imgRequestProxy::OnStopContainer(imgIContainer *image)
{
  LOG_FUNC(gImgLog, "imgRequestProxy::OnStopContainer");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnStopContainer(this, image);
  }
}

void imgRequestProxy::OnStopDecode(nsresult status, const PRUnichar *statusArg)
{
  LOG_FUNC(gImgLog, "imgRequestProxy::OnStopDecode");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnStopDecode(this, status, statusArg);
  }
}

void imgRequestProxy::OnDiscard()
{
  LOG_FUNC(gImgLog, "imgRequestProxy::OnDiscard");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnDiscard(this);
  }
}

void imgRequestProxy::OnStartRequest()
{
#ifdef PR_LOGGING
  nsCAutoString name;
  GetName(name);
  LOG_FUNC_WITH_PARAM(gImgLog, "imgRequestProxy::OnStartRequest", "name", name.get());
#endif

  
  
  if (mListener) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnStartRequest(this);
  }
}

void imgRequestProxy::OnStopRequest(PRBool lastPart)
{
#ifdef PR_LOGGING
  nsCAutoString name;
  GetName(name);
  LOG_FUNC_WITH_PARAM(gImgLog, "imgRequestProxy::OnStopRequest", "name", name.get());
#endif
  
  
  
  nsCOMPtr<imgIRequest> kungFuDeathGrip(this);

  mHadLastPart = lastPart;

  if (mListener) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnStopRequest(this, lastPart);
  }

  
  
  
  
  if (lastPart || (mLoadFlags & nsIRequest::LOAD_BACKGROUND) == 0) {
    RemoveFromLoadGroup(lastPart);
    
    
    if (!lastPart) {
      mLoadFlags |= nsIRequest::LOAD_BACKGROUND;
      AddToLoadGroup();
    }
  }

  if (mListenerIsStrongRef) {
    NS_PRECONDITION(mListener, "How did that happen?");
    
    
    
    imgIDecoderObserver* obs = mListener;
    mListenerIsStrongRef = PR_FALSE;
    NS_RELEASE(obs);
  }
}

void imgRequestProxy::NullOutListener()
{
  if (mListenerIsStrongRef) {
    
    nsCOMPtr<imgIDecoderObserver> obs;
    obs.swap(mListener);
    mListenerIsStrongRef = PR_FALSE;
  } else {
    mListener = nsnull;
  }
}

nsresult imgRequestProxy::GetState(PRUint32 *aState)
{
  if (!mOwner && !mImage) {
    *aState = 0;
    return NS_ERROR_FAILURE;
  }

  if (mOwner) {
    *aState = mOwner->GetState();
    return NS_OK;
  }

  
  
  *aState = mState;
  return NS_OK;
}

NS_IMETHODIMP
imgRequestProxy::GetStaticRequest(imgIRequest** aReturn)
{
  *aReturn = nsnull;

  nsCOMPtr<imgIContainer> img;
  nsCOMPtr<imgIContainer> currentFrame;
  GetImage(getter_AddRefs(img));
  PRBool animated;

  if (img && NS_SUCCEEDED(img->GetAnimated(&animated)) && !animated) {
    
    NS_ADDREF(*aReturn = this);
    return NS_OK;
  } else if (img) {
    
    PRInt32 w = 0;
    PRInt32 h = 0;
    img->GetWidth(&w);
    img->GetHeight(&h);
    nsIntRect rect(0, 0, w, h);
    img->ExtractFrame(imgIContainer::FRAME_CURRENT, rect,
                      imgIContainer::FLAG_SYNC_DECODE,
                      getter_AddRefs(currentFrame));
  }

  
  
  PRUint32 imageStatus = 0;
  GetImageStatus(&imageStatus);
  PRUint32 state = 0;
  GetState(&state);

  imgRequestProxy* req = new imgRequestProxy();
  req->Init(nsnull, nsnull, mURI, nsnull);
  req->SetImage(currentFrame);
  req->SetPrincipal(mPrincipal);

  
  req->mImageStatus = imageStatus;
  req->mState = state;

  NS_ADDREF(*aReturn = req);

  return NS_OK;
}

void imgRequestProxy::SetPrincipal(nsIPrincipal *aPrincipal)
{
  mPrincipal = aPrincipal;
}

void imgRequestProxy::SetImage(imgIContainer *aImage)
{
  mImage = aImage;
}

nsresult imgRequestProxy::NotifyListener()
{
  nsCOMPtr<imgIRequest> kungFuDeathGrip(this);

  PRUint32 imageStatus = 0;
  GetImageStatus(&imageStatus);
  PRUint32 state = 0;
  GetState(&state);

  nsCOMPtr<imgIContainer> image;
  GetImage(getter_AddRefs(image));

  
  if (state & stateRequestStarted)
    OnStartRequest();

  
  if (state & stateHasSize)
    OnStartContainer(image);

  
  if (state & stateDecodeStarted)
    OnStartDecode();

  
  PRUint32 nframes = 0;
  if (image)
    image->GetNumFrames(&nframes);

  if (nframes > 0) {
    PRUint32 frame;
    image->GetCurrentFrameIndex(&frame);
    OnStartFrame(frame);

    
    
    
    nsIntRect r;
    image->GetCurrentFrameRect(r);
    OnDataAvailable(frame, &r);

    if (state & stateRequestStopped)
      OnStopFrame(frame);
  }

  
  if (state & stateDecodeStopped)
    OnStopContainer(image);

  if (state & stateRequestStopped) {
    OnStopDecode(imgRequest::GetResultFromImageStatus(imageStatus), nsnull);
    OnStopRequest(mHadLastPart);
  }

  return NS_OK;
}
