





#include "imgRequestProxy.h"
#include "imgIOnloadBlocker.h"

#include "nsIInputStream.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIMultiPartChannel.h"

#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"

#include "Image.h"
#include "nsError.h"
#include "ImageLogging.h"

#include "nspr.h"

using namespace mozilla::image;

NS_IMPL_ADDREF(imgRequestProxy)
NS_IMPL_RELEASE(imgRequestProxy)

NS_INTERFACE_MAP_BEGIN(imgRequestProxy)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, imgIRequest)
  NS_INTERFACE_MAP_ENTRY(imgIRequest)
  NS_INTERFACE_MAP_ENTRY(nsIRequest)
  NS_INTERFACE_MAP_ENTRY(nsISupportsPriority)
  NS_INTERFACE_MAP_ENTRY(nsISecurityInfoProvider)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsITimedChannel, TimedChannel() != nullptr)
NS_INTERFACE_MAP_END

imgRequestProxy::imgRequestProxy() :
  mOwner(nullptr),
  mURI(nullptr),
  mImage(nullptr),
  mPrincipal(nullptr),
  mListener(nullptr),
  mLoadFlags(nsIRequest::LOAD_NORMAL),
  mLockCount(0),
  mAnimationConsumers(0),
  mCanceled(false),
  mIsInLoadGroup(false),
  mListenerIsStrongRef(false),
  mDecodeRequested(false),
  mDeferNotifications(false),
  mSentStartContainer(false)
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
      mCanceled = true;

      







      mOwner->RemoveProxy(this, NS_OK, false);
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
    mListenerIsStrongRef = true;
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

  
  
  uint32_t oldLockCount = mLockCount;
  while (mLockCount)
    UnlockImage();

  
  uint32_t oldAnimationConsumers = mAnimationConsumers;
  ClearAnimationConsumers();

  
  
  mImage = aNewOwner->mImage;

  
  for (uint32_t i = 0; i < oldLockCount; i++)
    LockImage();

  if (mCanceled) {
    
    
    for (uint32_t i = 0; i < oldAnimationConsumers; i++)
      IncrementAnimationConsumers();

    return NS_OK;
  }

  
  bool wasDecoded = false;
  if (mImage &&
      (mImage->GetStatusTracker().GetImageStatus() &
       imgIRequest::STATUS_FRAME_COMPLETE)) {
    wasDecoded = true;
  }

  
  
  mOwner->RemoveProxy(this, NS_IMAGELIB_CHANGING_OWNER, false);

  
  
  
  for (uint32_t i = 0; i < oldAnimationConsumers; i++)
    IncrementAnimationConsumers();

  mOwner = aNewOwner;

  mOwner->AddProxy(this);

  
  
  if (wasDecoded || mDecodeRequested)
    mOwner->StartDecoding();

  return NS_OK;
}

void imgRequestProxy::AddToLoadGroup()
{
  NS_ASSERTION(!mIsInLoadGroup, "Whaa, we're already in the loadgroup!");

  if (!mIsInLoadGroup && mLoadGroup) {
    mLoadGroup->AddRequest(this, nullptr);
    mIsInLoadGroup = true;
  }
}

void imgRequestProxy::RemoveFromLoadGroup(bool releaseLoadGroup)
{
  if (!mIsInLoadGroup)
    return;

  




  nsCOMPtr<imgIRequest> kungFuDeathGrip(this);

  mLoadGroup->RemoveRequest(this, nullptr, NS_OK);
  mIsInLoadGroup = false;

  if (releaseLoadGroup) {
    
    mLoadGroup = nullptr;
  }
}





NS_IMETHODIMP imgRequestProxy::GetName(nsACString &aName)
{
  aName.Truncate();

  if (mURI)
    mURI->GetSpec(aName);

  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::IsPending(bool *_retval)
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

  mCanceled = true;

  nsCOMPtr<nsIRunnable> ev = new imgCancelRunnable(this, status);
  return NS_DispatchToCurrentThread(ev);
}

void
imgRequestProxy::DoCancel(nsresult status)
{
  
  
  if (mOwner)
    mOwner->RemoveProxy(this, status, false);

  NullOutListener();
}


NS_IMETHODIMP imgRequestProxy::CancelAndForgetObserver(nsresult aStatus)
{
  
  
  
  
  
  
  if (mCanceled && !mListener)
    return NS_ERROR_FAILURE;

  LOG_SCOPE(gImgLog, "imgRequestProxy::CancelAndForgetObserver");

  mCanceled = true;

  
  bool oldIsInLoadGroup = mIsInLoadGroup;
  mIsInLoadGroup = false;

  
  
  if (mOwner)
    mOwner->RemoveProxy(this, aStatus, false);

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
imgRequestProxy::StartDecoding()
{
  if (!mOwner)
    return NS_ERROR_FAILURE;

  
  mDecodeRequested = true;

  
  return mOwner->StartDecoding();
}


NS_IMETHODIMP
imgRequestProxy::RequestDecode()
{
  if (!mOwner)
    return NS_ERROR_FAILURE;

  
  mDecodeRequested = true;

  
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
imgRequestProxy::RequestDiscard()
{
  if (mImage) {
    return mImage->RequestDiscard();
  }
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


NS_IMETHODIMP imgRequestProxy::GetImageStatus(uint32_t *aStatus)
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

  *aClone = nullptr;
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


NS_IMETHODIMP imgRequestProxy::GetMultipart(bool *aMultipart)
{
  if (!mOwner)
    return NS_ERROR_FAILURE;

  *aMultipart = mOwner->GetMultipart();

  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetCORSMode(int32_t* aCorsMode)
{
  if (!mOwner)
    return NS_ERROR_FAILURE;

  *aCorsMode = mOwner->GetCORSMode();

  return NS_OK;
}



NS_IMETHODIMP imgRequestProxy::GetPriority(int32_t *priority)
{
  NS_ENSURE_STATE(mOwner);
  *priority = mOwner->Priority();
  return NS_OK;
}

NS_IMETHODIMP imgRequestProxy::SetPriority(int32_t priority)
{
  NS_ENSURE_STATE(mOwner && !mCanceled);
  mOwner->AdjustPriority(this, priority - mOwner->Priority());
  return NS_OK;
}

NS_IMETHODIMP imgRequestProxy::AdjustPriority(int32_t priority)
{
  NS_ENSURE_STATE(mOwner && !mCanceled);
  mOwner->AdjustPriority(this, priority);
  return NS_OK;
}



NS_IMETHODIMP imgRequestProxy::GetSecurityInfo(nsISupports** _retval)
{
  if (mOwner)
    return mOwner->GetSecurityInfo(_retval);

  *_retval = nullptr;
  return NS_OK;
}

NS_IMETHODIMP imgRequestProxy::GetHasTransferredData(bool* hasData)
{
  if (mOwner) {
    *hasData = mOwner->HasTransferredData();
  } else {
    
    *hasData = true;
  }
  return NS_OK;
}



void imgRequestProxy::FrameChanged(imgIContainer *container,
                                   const nsIntRect *dirtyRect)
{
  LOG_FUNC(gImgLog, "imgRequestProxy::FrameChanged");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->FrameChanged(this, container, dirtyRect);
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
    mSentStartContainer = true;
  }
}

void imgRequestProxy::OnStartFrame(uint32_t frame)
{
  LOG_FUNC(gImgLog, "imgRequestProxy::OnStartFrame");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnStartFrame(this, frame);
  }
}

void imgRequestProxy::OnDataAvailable(bool aCurrentFrame, const nsIntRect * rect)
{
  LOG_FUNC(gImgLog, "imgRequestProxy::OnDataAvailable");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnDataAvailable(this, aCurrentFrame, rect);
  }
}

void imgRequestProxy::OnStopFrame(uint32_t frame)
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

  
  if (mOwner && mOwner->GetMultipart())
    mSentStartContainer = false;
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

void imgRequestProxy::OnImageIsAnimated()
{
  LOG_FUNC(gImgLog, "imgRequestProxy::OnImageIsAnimated");
  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnImageIsAnimated(this);
  }
}

void imgRequestProxy::OnStartRequest()
{
#ifdef PR_LOGGING
  nsAutoCString name;
  GetName(name);
  LOG_FUNC_WITH_PARAM(gImgLog, "imgRequestProxy::OnStartRequest", "name", name.get());
#endif

  
  
  if (mListener) {
    
    nsCOMPtr<imgIDecoderObserver> kungFuDeathGrip(mListener);
    mListener->OnStartRequest(this);
  }
}

void imgRequestProxy::OnStopRequest(bool lastPart)
{
#ifdef PR_LOGGING
  nsAutoCString name;
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
    mListenerIsStrongRef = false;
    NS_RELEASE(obs);
  }
}

void imgRequestProxy::BlockOnload()
{
#ifdef PR_LOGGING
  nsAutoCString name;
  GetName(name);
  LOG_FUNC_WITH_PARAM(gImgLog, "imgRequestProxy::BlockOnload", "name", name.get());
#endif

  nsCOMPtr<imgIOnloadBlocker> blocker = do_QueryInterface(mListener);
  if (blocker) {
    blocker->BlockOnload(this);
  }
}

void imgRequestProxy::UnblockOnload()
{
#ifdef PR_LOGGING
  nsAutoCString name;
  GetName(name);
  LOG_FUNC_WITH_PARAM(gImgLog, "imgRequestProxy::UnblockOnload", "name", name.get());
#endif

  nsCOMPtr<imgIOnloadBlocker> blocker = do_QueryInterface(mListener);
  if (blocker) {
    blocker->UnblockOnload(this);
  }
}

void imgRequestProxy::NullOutListener()
{
  
  if (mListener)
    ClearAnimationConsumers();

  if (mListenerIsStrongRef) {
    
    nsCOMPtr<imgIDecoderObserver> obs;
    obs.swap(mListener);
    mListenerIsStrongRef = false;
  } else {
    mListener = nullptr;
  }
}

NS_IMETHODIMP
imgRequestProxy::GetStaticRequest(imgIRequest** aReturn)
{
  *aReturn = nullptr;

  bool animated;
  if (!mImage || (NS_SUCCEEDED(mImage->GetAnimated(&animated)) && !animated)) {
    
    NS_ADDREF(*aReturn = this);
    return NS_OK;
  }

  
  int32_t w = 0;
  int32_t h = 0;
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
  req->Init(nullptr, nullptr, frame, mURI, nullptr);
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
  NS_ABORT_IF_FALSE(!mImage || mOwner->GetMultipart(),
                    "Setting image when we already have one");

  mImage = aImage;

  
  for (uint32_t i = 0; i < mLockCount; ++i)
    mImage->LockImage();

  
  for (uint32_t i = 0; i < mAnimationConsumers; i++)
    mImage->IncrementAnimationConsumers();
}

imgStatusTracker&
imgRequestProxy::GetStatusTracker()
{
  
  
  
  
  
  
  return mImage ? mImage->GetStatusTracker() : mOwner->GetStatusTracker();
}
