





#include "ImageLogging.h"
#include "imgRequestProxy.h"
#include "imgIOnloadBlocker.h"

#include "Image.h"
#include "ImageOps.h"
#include "nsError.h"
#include "nsCRTGlue.h"
#include "imgINotificationObserver.h"
#include "nsNetUtil.h"

using namespace mozilla::image;





class ProxyBehaviour
{
 public:
  virtual ~ProxyBehaviour() {}

  virtual already_AddRefed<mozilla::image::Image> GetImage() const = 0;
  virtual bool HasImage() const = 0;
  virtual already_AddRefed<imgStatusTracker> GetStatusTracker() const = 0;
  virtual imgRequest* GetOwner() const = 0;
  virtual void SetOwner(imgRequest* aOwner) = 0;
};

class RequestBehaviour : public ProxyBehaviour
{
 public:
  RequestBehaviour() : mOwner(nullptr), mOwnerHasImage(false) {}

  virtual already_AddRefed<mozilla::image::Image> GetImage() const MOZ_OVERRIDE;
  virtual bool HasImage() const MOZ_OVERRIDE;
  virtual already_AddRefed<imgStatusTracker> GetStatusTracker() const MOZ_OVERRIDE;

  virtual imgRequest* GetOwner() const MOZ_OVERRIDE {
    return mOwner;
  }

  virtual void SetOwner(imgRequest* aOwner) MOZ_OVERRIDE {
    mOwner = aOwner;

    if (mOwner) {
      nsRefPtr<imgStatusTracker> ownerStatusTracker = GetStatusTracker();
      mOwnerHasImage = ownerStatusTracker && ownerStatusTracker->HasImage();
    } else {
      mOwnerHasImage = false;
    }
  }

 private:
  
  
  
  
  
  
  nsRefPtr<imgRequest> mOwner;

  bool mOwnerHasImage;
};

already_AddRefed<mozilla::image::Image>
RequestBehaviour::GetImage() const
{
  if (!mOwnerHasImage)
    return nullptr;
  nsRefPtr<imgStatusTracker> statusTracker = GetStatusTracker();
  return statusTracker->GetImage();
}

already_AddRefed<imgStatusTracker>
RequestBehaviour::GetStatusTracker() const
{
  
  
  
  
  
  
  return mOwner->GetStatusTracker();
}

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
  mBehaviour(new RequestBehaviour),
  mURI(nullptr),
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

  if (GetOwner()) {
    




    mCanceled = true;
    GetOwner()->RemoveProxy(this, NS_OK);
  }
}

nsresult imgRequestProxy::Init(imgRequest* aOwner,
                               nsILoadGroup* aLoadGroup,
                               ImageURL* aURI,
                               imgINotificationObserver* aObserver)
{
  NS_PRECONDITION(!GetOwner() && !mListener, "imgRequestProxy is already initialized");

  LOG_SCOPE_WITH_PARAM(GetImgLog(), "imgRequestProxy::Init", "request", aOwner);

  NS_ABORT_IF_FALSE(mAnimationConsumers == 0, "Cannot have animation before Init");

  mBehaviour->SetOwner(aOwner);
  mListener = aObserver;
  
  
  
  if (mListener) {
    mListenerIsStrongRef = true;
    NS_ADDREF(mListener);
  }
  mLoadGroup = aLoadGroup;
  mURI = aURI;

  
  if (GetOwner())
    GetOwner()->AddProxy(this);

  return NS_OK;
}

nsresult imgRequestProxy::ChangeOwner(imgRequest *aNewOwner)
{
  NS_PRECONDITION(GetOwner(), "Cannot ChangeOwner on a proxy without an owner!");

  if (mCanceled) {
    
    
    SyncNotifyListener();
  }

  
  
  uint32_t oldLockCount = mLockCount;
  while (mLockCount)
    UnlockImage();

  
  uint32_t oldAnimationConsumers = mAnimationConsumers;
  ClearAnimationConsumers();

  
  bool wasDecoded = false;
  nsRefPtr<imgStatusTracker> statusTracker = GetStatusTracker();
  if (statusTracker->HasImage() &&
      statusTracker->GetImageStatus() & imgIRequest::STATUS_FRAME_COMPLETE) {
    wasDecoded = true;
  }

  GetOwner()->RemoveProxy(this, NS_IMAGELIB_CHANGING_OWNER);

  mBehaviour->SetOwner(aNewOwner);

  
  for (uint32_t i = 0; i < oldLockCount; i++)
    LockImage();

  
  
  
  for (uint32_t i = 0; i < oldAnimationConsumers; i++)
    IncrementAnimationConsumers();

  GetOwner()->AddProxy(this);

  
  
  if (wasDecoded || mDecodeRequested)
    GetOwner()->StartDecoding();

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

  LOG_SCOPE(GetImgLog(), "imgRequestProxy::Cancel");

  mCanceled = true;

  nsCOMPtr<nsIRunnable> ev = new imgCancelRunnable(this, status);
  return NS_DispatchToCurrentThread(ev);
}

void
imgRequestProxy::DoCancel(nsresult status)
{
  if (GetOwner()) {
    GetOwner()->RemoveProxy(this, status);
  }

  NullOutListener();
}


NS_IMETHODIMP imgRequestProxy::CancelAndForgetObserver(nsresult aStatus)
{
  
  
  
  
  
  
  if (mCanceled && !mListener)
    return NS_ERROR_FAILURE;

  LOG_SCOPE(GetImgLog(), "imgRequestProxy::CancelAndForgetObserver");

  mCanceled = true;

  
  bool oldIsInLoadGroup = mIsInLoadGroup;
  mIsInLoadGroup = false;

  if (GetOwner()) {
    GetOwner()->RemoveProxy(this, aStatus);
  }

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
  if (!GetOwner())
    return NS_ERROR_FAILURE;

  
  mDecodeRequested = true;

  
  return GetOwner()->StartDecoding();
}


NS_IMETHODIMP
imgRequestProxy::RequestDecode()
{
  if (!GetOwner())
    return NS_ERROR_FAILURE;

  
  mDecodeRequested = true;

  
  return GetOwner()->RequestDecode();
}



NS_IMETHODIMP
imgRequestProxy::LockImage()
{
  mLockCount++;
  nsRefPtr<Image> image = GetImage();
  if (image)
    return image->LockImage();
  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::UnlockImage()
{
  NS_ABORT_IF_FALSE(mLockCount > 0, "calling unlock but no locks!");

  mLockCount--;
  nsRefPtr<Image> image = GetImage();
  if (image)
    return image->UnlockImage();
  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::RequestDiscard()
{
  nsRefPtr<Image> image = GetImage();
  if (image)
    return image->RequestDiscard();
  return NS_OK;
}

NS_IMETHODIMP
imgRequestProxy::IncrementAnimationConsumers()
{
  mAnimationConsumers++;
  nsRefPtr<Image> image = GetImage();
  if (image)
    image->IncrementAnimationConsumers();
  return NS_OK;
}

NS_IMETHODIMP
imgRequestProxy::DecrementAnimationConsumers()
{
  
  
  
  
  
  
  if (mAnimationConsumers > 0) {
    mAnimationConsumers--;
    nsRefPtr<Image> image = GetImage();
    if (image)
      image->DecrementAnimationConsumers();
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




NS_IMETHODIMP imgRequestProxy::GetImage(imgIContainer **aImage)
{
  NS_ENSURE_TRUE(aImage, NS_ERROR_NULL_POINTER);
  
  
  
  
  nsRefPtr<Image> image = GetImage();
  nsCOMPtr<imgIContainer> imageToReturn;
  if (image)
    imageToReturn = do_QueryInterface(image);
  if (!imageToReturn && GetOwner())
    imageToReturn = GetOwner()->mImage;

  if (!imageToReturn)
    return NS_ERROR_FAILURE;

  imageToReturn.swap(*aImage);

  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetImageStatus(uint32_t *aStatus)
{
  nsRefPtr<imgStatusTracker> statusTracker = GetStatusTracker();
  *aStatus = statusTracker->GetImageStatus();

  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetImageErrorCode(nsresult *aStatus)
{
  if (!GetOwner())
    return NS_ERROR_FAILURE;

  *aStatus = GetOwner()->GetImageErrorCode();

  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetURI(nsIURI **aURI)
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread to convert URI");
  nsCOMPtr<nsIURI> uri = mURI->ToIURI();
  uri.forget(aURI);
  return NS_OK;
}

nsresult imgRequestProxy::GetURI(ImageURL **aURI)
{
  if (!mURI)
    return NS_ERROR_FAILURE;

  NS_ADDREF(*aURI = mURI);

  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetNotificationObserver(imgINotificationObserver **aObserver)
{
  *aObserver = mListener;
  NS_IF_ADDREF(*aObserver);
  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetMimeType(char **aMimeType)
{
  if (!GetOwner())
    return NS_ERROR_FAILURE;

  const char *type = GetOwner()->GetMimeType();
  if (!type)
    return NS_ERROR_FAILURE;

  *aMimeType = NS_strdup(type);

  return NS_OK;
}

static imgRequestProxy* NewProxy(imgRequestProxy* )
{
  return new imgRequestProxy();
}

imgRequestProxy* NewStaticProxy(imgRequestProxy* aThis)
{
  nsCOMPtr<nsIPrincipal> currentPrincipal;
  aThis->GetImagePrincipal(getter_AddRefs(currentPrincipal));
  nsRefPtr<Image> image = aThis->GetImage();
  return new imgRequestProxyStatic(image, currentPrincipal);
}

NS_IMETHODIMP imgRequestProxy::Clone(imgINotificationObserver* aObserver,
                                     imgIRequest** aClone)
{
  nsresult result;
  imgRequestProxy* proxy;
  result = Clone(aObserver, &proxy);
  *aClone = proxy;
  return result;
}

nsresult imgRequestProxy::Clone(imgINotificationObserver* aObserver,
                                imgRequestProxy** aClone)
{
  return PerformClone(aObserver, NewProxy, aClone);
}

nsresult imgRequestProxy::PerformClone(imgINotificationObserver* aObserver,
                                       imgRequestProxy* (aAllocFn)(imgRequestProxy*),
                                       imgRequestProxy** aClone)
{
  NS_PRECONDITION(aClone, "Null out param");

  LOG_SCOPE(GetImgLog(), "imgRequestProxy::Clone");

  *aClone = nullptr;
  nsRefPtr<imgRequestProxy> clone = aAllocFn(this);

  
  
  
  
  
  
  clone->SetLoadFlags(mLoadFlags);
  nsresult rv = clone->Init(mBehaviour->GetOwner(), mLoadGroup, mURI, aObserver);
  if (NS_FAILED(rv))
    return rv;

  
  
  
  NS_ADDREF(*aClone = clone);

  
  
  clone->SyncNotifyListener();

  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetImagePrincipal(nsIPrincipal **aPrincipal)
{
  if (!GetOwner())
    return NS_ERROR_FAILURE;

  NS_ADDREF(*aPrincipal = GetOwner()->GetPrincipal());
  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetMultipart(bool *aMultipart)
{
  if (!GetOwner())
    return NS_ERROR_FAILURE;

  *aMultipart = GetOwner()->GetMultipart();

  return NS_OK;
}


NS_IMETHODIMP imgRequestProxy::GetCORSMode(int32_t* aCorsMode)
{
  if (!GetOwner())
    return NS_ERROR_FAILURE;

  *aCorsMode = GetOwner()->GetCORSMode();

  return NS_OK;
}



NS_IMETHODIMP imgRequestProxy::GetPriority(int32_t *priority)
{
  NS_ENSURE_STATE(GetOwner());
  *priority = GetOwner()->Priority();
  return NS_OK;
}

NS_IMETHODIMP imgRequestProxy::SetPriority(int32_t priority)
{
  NS_ENSURE_STATE(GetOwner() && !mCanceled);
  GetOwner()->AdjustPriority(this, priority - GetOwner()->Priority());
  return NS_OK;
}

NS_IMETHODIMP imgRequestProxy::AdjustPriority(int32_t priority)
{
  
  
  
  NS_ENSURE_STATE(GetOwner());
  GetOwner()->AdjustPriority(this, priority);
  return NS_OK;
}



NS_IMETHODIMP imgRequestProxy::GetSecurityInfo(nsISupports** _retval)
{
  if (GetOwner())
    return GetOwner()->GetSecurityInfo(_retval);

  *_retval = nullptr;
  return NS_OK;
}

NS_IMETHODIMP imgRequestProxy::GetHasTransferredData(bool* hasData)
{
  if (GetOwner()) {
    *hasData = GetOwner()->HasTransferredData();
  } else {
    
    *hasData = true;
  }
  return NS_OK;
}



void imgRequestProxy::OnStartDecode()
{
  
  
  if (GetOwner()) {
    
    
    
    
    
    GetOwner()->ResetCacheEntry();
  }
}

void imgRequestProxy::OnStartContainer()
{
  LOG_FUNC(GetImgLog(), "imgRequestProxy::OnStartContainer");

  if (mListener && !mCanceled && !mSentStartContainer) {
    
    nsCOMPtr<imgINotificationObserver> kungFuDeathGrip(mListener);
    mListener->Notify(this, imgINotificationObserver::SIZE_AVAILABLE, nullptr);
    mSentStartContainer = true;
  }
}

void imgRequestProxy::OnFrameUpdate(const nsIntRect * rect)
{
  LOG_FUNC(GetImgLog(), "imgRequestProxy::OnDataAvailable");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgINotificationObserver> kungFuDeathGrip(mListener);
    mListener->Notify(this, imgINotificationObserver::FRAME_UPDATE, rect);
  }
}

void imgRequestProxy::OnStopFrame()
{
  LOG_FUNC(GetImgLog(), "imgRequestProxy::OnStopFrame");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgINotificationObserver> kungFuDeathGrip(mListener);
    mListener->Notify(this, imgINotificationObserver::FRAME_COMPLETE, nullptr);
  }
}

void imgRequestProxy::OnStopDecode()
{
  LOG_FUNC(GetImgLog(), "imgRequestProxy::OnStopDecode");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgINotificationObserver> kungFuDeathGrip(mListener);
    mListener->Notify(this, imgINotificationObserver::DECODE_COMPLETE, nullptr);
  }

  if (GetOwner()) {
    
    
    GetOwner()->UpdateCacheEntrySize();

    
    if (GetOwner()->GetMultipart())
      mSentStartContainer = false;
  }
}

void imgRequestProxy::OnDiscard()
{
  LOG_FUNC(GetImgLog(), "imgRequestProxy::OnDiscard");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgINotificationObserver> kungFuDeathGrip(mListener);
    mListener->Notify(this, imgINotificationObserver::DISCARD, nullptr);
  }
  if (GetOwner()) {
    
    GetOwner()->UpdateCacheEntrySize();
  }
}

void imgRequestProxy::OnUnlockedDraw()
{
  LOG_FUNC(GetImgLog(), "imgRequestProxy::OnUnlockedDraw");

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgINotificationObserver> kungFuDeathGrip(mListener);
    mListener->Notify(this, imgINotificationObserver::UNLOCKED_DRAW, nullptr);
  }
}

void imgRequestProxy::OnImageIsAnimated()
{
  LOG_FUNC(GetImgLog(), "imgRequestProxy::OnImageIsAnimated");
  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgINotificationObserver> kungFuDeathGrip(mListener);
    mListener->Notify(this, imgINotificationObserver::IS_ANIMATED, nullptr);
  }
}

void imgRequestProxy::OnStartRequest()
{
#ifdef PR_LOGGING
  nsAutoCString name;
  GetName(name);
  LOG_FUNC_WITH_PARAM(GetImgLog(), "imgRequestProxy::OnStartRequest", "name", name.get());
#endif
}

void imgRequestProxy::OnStopRequest(bool lastPart)
{
#ifdef PR_LOGGING
  nsAutoCString name;
  GetName(name);
  LOG_FUNC_WITH_PARAM(GetImgLog(), "imgRequestProxy::OnStopRequest", "name", name.get());
#endif
  
  
  
  nsCOMPtr<imgIRequest> kungFuDeathGrip(this);

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgINotificationObserver> kungFuDeathGrip(mListener);
    mListener->Notify(this, imgINotificationObserver::LOAD_COMPLETE, nullptr);
  }

  
  
  
  
  if (lastPart || (mLoadFlags & nsIRequest::LOAD_BACKGROUND) == 0) {
    RemoveFromLoadGroup(lastPart);
    
    
    if (!lastPart) {
      mLoadFlags |= nsIRequest::LOAD_BACKGROUND;
      AddToLoadGroup();
    }
  }

  if (mListenerIsStrongRef && lastPart) {
    NS_PRECONDITION(mListener, "How did that happen?");
    
    
    
    imgINotificationObserver* obs = mListener;
    mListenerIsStrongRef = false;
    NS_RELEASE(obs);
  }
}

void imgRequestProxy::BlockOnload()
{
#ifdef PR_LOGGING
  nsAutoCString name;
  GetName(name);
  LOG_FUNC_WITH_PARAM(GetImgLog(), "imgRequestProxy::BlockOnload", "name", name.get());
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
  LOG_FUNC_WITH_PARAM(GetImgLog(), "imgRequestProxy::UnblockOnload", "name", name.get());
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
    
    nsCOMPtr<imgINotificationObserver> obs;
    obs.swap(mListener);
    mListenerIsStrongRef = false;
  } else {
    mListener = nullptr;
  }
}

NS_IMETHODIMP
imgRequestProxy::GetStaticRequest(imgIRequest** aReturn)
{
  imgRequestProxy *proxy;
  nsresult result = GetStaticRequest(&proxy);
  *aReturn = proxy;
  return result;
}

nsresult
imgRequestProxy::GetStaticRequest(imgRequestProxy** aReturn)
{
  *aReturn = nullptr;
  nsRefPtr<Image> image = GetImage();

  bool animated;
  if (!image || (NS_SUCCEEDED(image->GetAnimated(&animated)) && !animated)) {
    
    NS_ADDREF(*aReturn = this);
    return NS_OK;
  }

  
  
  
  if (image->HasError()) {
    return NS_ERROR_FAILURE;
  }

  
  nsRefPtr<Image> frozenImage = ImageOps::Freeze(image);

  
  nsCOMPtr<nsIPrincipal> currentPrincipal;
  GetImagePrincipal(getter_AddRefs(currentPrincipal));
  nsRefPtr<imgRequestProxy> req = new imgRequestProxyStatic(frozenImage,
                                                            currentPrincipal);
  req->Init(nullptr, nullptr, mURI, nullptr);

  NS_ADDREF(*aReturn = req);

  return NS_OK;
}

void imgRequestProxy::NotifyListener()
{
  
  
  
  

  nsRefPtr<imgStatusTracker> statusTracker = GetStatusTracker();
  if (GetOwner()) {
    
    statusTracker->Notify(this);
  } else {
    
    
    NS_ABORT_IF_FALSE(HasImage(),
                      "if we have no imgRequest, we should have an Image");
    statusTracker->NotifyCurrentState(this);
  }
}

void imgRequestProxy::SyncNotifyListener()
{
  
  
  
  

  nsRefPtr<imgStatusTracker> statusTracker = GetStatusTracker();
  statusTracker->SyncNotify(this);
}

void
imgRequestProxy::SetHasImage()
{
  nsRefPtr<imgStatusTracker> statusTracker = GetStatusTracker();
  MOZ_ASSERT(statusTracker);
  nsRefPtr<Image> image = statusTracker->GetImage();
  MOZ_ASSERT(image);

  
  
  mBehaviour->SetOwner(mBehaviour->GetOwner());

  
  for (uint32_t i = 0; i < mLockCount; ++i)
    image->LockImage();

  
  for (uint32_t i = 0; i < mAnimationConsumers; i++)
    image->IncrementAnimationConsumers();
}

already_AddRefed<imgStatusTracker>
imgRequestProxy::GetStatusTracker() const
{
  return mBehaviour->GetStatusTracker();
}

already_AddRefed<mozilla::image::Image>
imgRequestProxy::GetImage() const
{
  return mBehaviour->GetImage();
}

bool
RequestBehaviour::HasImage() const
{
  if (!mOwnerHasImage)
    return false;
  nsRefPtr<imgStatusTracker> statusTracker = GetStatusTracker();
  return statusTracker ? statusTracker->HasImage() : false;
}

bool
imgRequestProxy::HasImage() const
{
  return mBehaviour->HasImage();
}

imgRequest*
imgRequestProxy::GetOwner() const
{
  return mBehaviour->GetOwner();
}



class StaticBehaviour : public ProxyBehaviour
{
public:
  explicit StaticBehaviour(mozilla::image::Image* aImage) : mImage(aImage) {}

  virtual already_AddRefed<mozilla::image::Image>
  GetImage() const MOZ_OVERRIDE {
    nsRefPtr<mozilla::image::Image> image = mImage;
    return image.forget();
  }

  virtual bool HasImage() const MOZ_OVERRIDE {
    return mImage;
  }

  virtual already_AddRefed<imgStatusTracker> GetStatusTracker() const MOZ_OVERRIDE  {
    return mImage->GetStatusTracker();
  }

  virtual imgRequest* GetOwner() const MOZ_OVERRIDE {
    return nullptr;
  }

  virtual void SetOwner(imgRequest* aOwner) MOZ_OVERRIDE {
    MOZ_ASSERT(!aOwner, "We shouldn't be giving static requests a non-null owner.");
  }

private:
  
  
  nsRefPtr<mozilla::image::Image> mImage;
};

imgRequestProxyStatic::imgRequestProxyStatic(mozilla::image::Image* aImage,
                                             nsIPrincipal* aPrincipal)
: mPrincipal(aPrincipal)
{
  mBehaviour = new StaticBehaviour(aImage);
}

NS_IMETHODIMP imgRequestProxyStatic::GetImagePrincipal(nsIPrincipal **aPrincipal)
{
  if (!mPrincipal)
    return NS_ERROR_FAILURE;

  NS_ADDREF(*aPrincipal = mPrincipal);

  return NS_OK;
}

nsresult
imgRequestProxyStatic::Clone(imgINotificationObserver* aObserver,
                             imgRequestProxy** aClone)
{
  return PerformClone(aObserver, NewStaticProxy, aClone);
}
