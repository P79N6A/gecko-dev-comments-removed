






































#include "imgRequestProxy.h"

#include "nsIInputStream.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIMultiPartChannel.h"

#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"

#include "Image.h"
#include "ImageErrors.h"
#include "ImageLogging.h"

#include "nspr.h"

using namespace mozilla::imagelib;

NS_IMPL_ADDREF(imgRequestProxy)
NS_IMPL_RELEASE(imgRequestProxy)

NS_INTERFACE_MAP_BEGIN(imgRequestProxy)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, imgIRequest)
  NS_INTERFACE_MAP_ENTRY(imgIRequest)
  NS_INTERFACE_MAP_ENTRY(nsIRequest)
  NS_INTERFACE_MAP_ENTRY(nsISupportsPriority)
  NS_INTERFACE_MAP_ENTRY(nsISecurityInfoProvider)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsITimedChannel, TimedChannel() != nsnull)
NS_INTERFACE_MAP_END

imgRequestProxy::imgRequestProxy() :
  mOwner(nsnull),
  mURI(nsnull),
  mImage(nsnull),
  mPrincipal(nsnull),
  mListener(nsnull),
  mLoadFlags(nsIRequest::LOAD_NORMAL),
  mLockCount(0),
  mAnimationConsumers(0),
  mCanceled(PR_FALSE),
  mIsInLoadGroup(PR_FALSE),
  mListenerIsStrongRef(PR_FALSE),
  mDecodeRequested(PR_FALSE),
  mDeferNotifications(PR_FALSE),
  mSentStartContainer(PR_FALSE)
{
  

}

imgRequestProxy::~imgRequestProxy()
{
  
  NS_PRECONDITION(!mListener, "Someone forgot to properly cancel this request!");

  
  
  while (mLockCount)
    UnlockImage();

  ClearAnimationConsumers();

  
  
  
  
  NullOutListener();

  if (mOwner) {
    if (!mCanceled) {
      mCanceled = PR_TRUE;

      







      mOwner->RemoveProxy(this, NS_OK, PR_FALSE);
    }
  }
}

nsresult imgRequestProxy::Init(imgRequest* request, nsILoadGroup* aLoadGroup, Image* aImage,
                               nsIURI* aURI, imgIDecoderObserver* aObserver)
{
  NS_PRECONDITION(!mOwner && !mListener, "imgRequestProxy is already initialized");

  LOG_SCOPE_WITH_PARAM(gImgLog, "imgRequestProxy::Init", "request", request);

  NS_ABORT_IF_FALSE(mAnimationConsumers == 0, "Cannot have animation before Init");

  mOwner = request;
  mListener = aObserver;
  
  
  
  if (mListener) {
    mListenerIsStrongRef = PR_TRUE;
    NS_ADDREF(mListener);
  }
  mLoadGroup = aLoadGroup;
  mImage = aImage;
  mURI = aURI;

  
  if (mOwner)
    mOwner->AddProxy(this);

  return NS_OK;
}

nsresult imgRequestProxy::ChangeOwner(imgRequest *aNewOwner)
{
  NS_PRECONDITION(mOwner, "Cannot ChangeOwner on a proxy without an owner!");

  
  
  PRUint32 oldLockCount = mLockCount;
  while (mLockCount)
    UnlockImage();

  
  PRUint32 oldAnimationConsumers = mAnimationConsumers;
  ClearAnimationConsumers();

  
  
  mImage = aNewOwner->mImage;

  
  for (PRUint32 i = 0; i < oldLockCount; i++)
    LockImage();

  if (mCanceled) {
    
    
    for (PRUint32 i = 0; i < oldAnimationConsumers; i++)
      IncrementAnimationConsumers();

    return NS_OK;
  }

  
  PRBool wasDecoded = PR_FALSE;
  if (mImage &&
      (mImage->GetStatusTracker().GetImageStatus() &
       imgIRequest::STATUS_FRAME_COMPLETE)) {
    wasDecoded = PR_TRUE;
  }

  
  
  mOwner->RemoveProxy(this, NS_IMAGELIB_CHANGING_OWNER, PR_FALSE);

  
  
  
  for (PRUint32 i = 0; i < oldAnimationConsumers; i++)
    IncrementAnimationConsumers();

  mOwner = aNewOwner;

  mOwner->AddProxy(this);

  
  
  if (wasDecoded || mDecodeRequested)
    mOwner->RequestDecode();

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
  if (mCanceled)
    return NS_ERROR_FAILURE;

  LOG_SCOPE(gImgLog, "imgRequestProxy::Cancel");

  mCanceled = PR_TRUE;

  nsCOMPtr<nsIRunnable> ev = new imgCancelRunnable(this, status);
  return NS_DispatchToCurrentThread(ev);
}

void
imgRequestProxy::DoCancel(nsresult status)
{
  
  
  if (mOwner)
    mOwner->RemoveProxy(this, status, PR_FALSE);

  NullOutListener();
}


NS_IMETHODIMP imgRequestProxy::CancelAndForgetObserver(nsresult aStatus)
{
  if (mCanceled)
    return NS_ERROR_FAILURE;

  LOG_SCOPE(gImgLog, "imgRequestProxy::CancelAndForgetObserver");

  mCanceled = PR_TRUE;

  
  PRBool oldIsInLoadGroup = mIsInLoadGroup;
  mIsInLoadGroup = PR_FALSE;

  
  
  if (mOwner)
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
  mLockCount++;
  if (mImage)
    return mImage->LockImage();
  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::UnlockImage()
{
  NS_ABORT_IF_FALSE(mLockCount > 0, "calling unlock but no locks!");

  mLockCount--;
  if (mImage)
    return mImage->UnlockImage();
  return NS_OK;
}

NS_IMETHODIMP
imgRequestProxy::IncrementAnimationConsumers()
{
  mAnimationConsumers++;
  if (mImage)
    mImage->IncrementAnimationConsumers();
  return NS_OK;
}

NS_IMETHODIMP
imgRequestProxy::DecrementAnimationConsumers()
{
  
  
  
  
  
  
  if (mAnimationConsumers > 0) {
    mAnimationConsumers--;
    if (mImage)
      mImage->DecrementAnimationConsumers();
  }
  return NS_OK;
}

void
imgRequestProxy::ClearAnimationConsumers()
{
  while (mAnimationConsumers > 0)
    DecrementAnimationConsumers();
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
  
  
  
  
  imgIContainer* imageToReturn = mImage ? mImage : mOwner->mImage;

  if (!imageToReturn)
    return NS_ERROR_FAILURE;

  NS_ADDREF(*aImage = imageToReturn);

  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetImageStatus(PRUint32 *aStatus)
{
  *aStatus = GetStatusTracker().GetImageStatus();

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

  LOG_SCOPE(gImgLog, "imgRequestProxy::Clone");

  *aClone = nsnull;
  nsRefPtr<imgRequestProxy> clone = new imgRequestProxy();

  
  
  
  
  
  
  clone->SetLoadFlags(mLoadFlags);
  nsresult rv = clone->Init(mOwner, mLoadGroup,
                            mImage ? mImage : mOwner->mImage,
                            mURI, aObserver);
  if (NS_FAILED(rv))
    return rv;

  clone->SetPrincipal(mPrincipal);

  
  
  
  NS_ADDREF(*aClone = clone);

  
  
  clone->SyncNotifyListener();

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



void imgRequestProxy::FrameChanged(imgIContainer *container,
                                   const nsIntRect *dirtyRect)
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

  if (mListener && !mCanceled && !mSentStartContainer) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnStartContainer(this, image);
    mSentStartContainer = PR_TRUE;
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
  
  if (mListener)
    ClearAnimationConsumers();

  if (mListenerIsStrongRef) {
    
    nsCOMPtr<imgIDecoderObserver> obs;
    obs.swap(mListener);
    mListenerIsStrongRef = PR_FALSE;
  } else {
    mListener = nsnull;
  }
}

NS_IMETHODIMP
imgRequestProxy::GetStaticRequest(imgIRequest** aReturn)
{
  *aReturn = nsnull;

  PRBool animated;
  if (!mImage || (NS_SUCCEEDED(mImage->GetAnimated(&animated)) && !animated)) {
    
    NS_ADDREF(*aReturn = this);
    return NS_OK;
  }

  
  PRInt32 w = 0;
  PRInt32 h = 0;
  mImage->GetWidth(&w);
  mImage->GetHeight(&h);
  nsIntRect rect(0, 0, w, h);
  nsCOMPtr<imgIContainer> currentFrame;
  nsresult rv = mImage->ExtractFrame(imgIContainer::FRAME_CURRENT, rect,
                                     imgIContainer::FLAG_SYNC_DECODE,
                                     getter_AddRefs(currentFrame));
  if (NS_FAILED(rv))
    return rv;

  nsRefPtr<Image> frame = static_cast<Image*>(currentFrame.get());

  
  nsRefPtr<imgRequestProxy> req = new imgRequestProxy();
  req->Init(nsnull, nsnull, frame, mURI, nsnull);
  req->SetPrincipal(mPrincipal);

  NS_ADDREF(*aReturn = req);

  return NS_OK;
}

void imgRequestProxy::SetPrincipal(nsIPrincipal *aPrincipal)
{
  mPrincipal = aPrincipal;
}

void imgRequestProxy::NotifyListener()
{
  
  
  
  

  if (mOwner) {
    
    GetStatusTracker().Notify(mOwner, this);
  } else {
    
    
    NS_ABORT_IF_FALSE(mImage,
                      "if we have no imgRequest, we should have an Image");
    mImage->GetStatusTracker().NotifyCurrentState(this);
  }
}

void imgRequestProxy::SyncNotifyListener()
{
  
  
  
  

  GetStatusTracker().SyncNotify(this);
}

void
imgRequestProxy::SetImage(Image* aImage)
{
  NS_ABORT_IF_FALSE(aImage,  "Setting null image");
  NS_ABORT_IF_FALSE(!mImage, "Setting image when we already have one");

  mImage = aImage;

  
  for (PRUint32 i = 0; i < mLockCount; ++i)
    mImage->LockImage();

  
  for (PRUint32 i = 0; i < mAnimationConsumers; i++)
    mImage->IncrementAnimationConsumers();
}

imgStatusTracker&
imgRequestProxy::GetStatusTracker()
{
  
  
  
  
  
  
  return mImage ? mImage->GetStatusTracker() : mOwner->GetStatusTracker();
}
