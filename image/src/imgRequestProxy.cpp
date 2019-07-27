





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
  virtual already_AddRefed<ProgressTracker> GetProgressTracker() const = 0;
  virtual imgRequest* GetOwner() const = 0;
  virtual void SetOwner(imgRequest* aOwner) = 0;
};

class RequestBehaviour : public ProxyBehaviour
{
 public:
  RequestBehaviour() : mOwner(nullptr), mOwnerHasImage(false) {}

  virtual already_AddRefed<mozilla::image::Image>GetImage() const override;
  virtual bool HasImage() const override;
  virtual already_AddRefed<ProgressTracker> GetProgressTracker() const override;

  virtual imgRequest* GetOwner() const override {
    return mOwner;
  }

  virtual void SetOwner(imgRequest* aOwner) override {
    mOwner = aOwner;

    if (mOwner) {
      nsRefPtr<ProgressTracker> ownerProgressTracker = GetProgressTracker();
      mOwnerHasImage = ownerProgressTracker && ownerProgressTracker->HasImage();
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
  if (!mOwnerHasImage) {
    return nullptr;
  }
  nsRefPtr<ProgressTracker> progressTracker = GetProgressTracker();
  return progressTracker->GetImage();
}

already_AddRefed<ProgressTracker>
RequestBehaviour::GetProgressTracker() const
{
  
  
  
  
  
  
  return mOwner->GetProgressTracker();
}

NS_IMPL_ADDREF(imgRequestProxy)
NS_IMPL_RELEASE(imgRequestProxy)

NS_INTERFACE_MAP_BEGIN(imgRequestProxy)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, imgIRequest)
  NS_INTERFACE_MAP_ENTRY(imgIRequest)
  NS_INTERFACE_MAP_ENTRY(nsIRequest)
  NS_INTERFACE_MAP_ENTRY(nsISupportsPriority)
  NS_INTERFACE_MAP_ENTRY(nsISecurityInfoProvider)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsITimedChannel,
                                     TimedChannel() != nullptr)
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
  mDeferNotifications(false)
{
  

}

imgRequestProxy::~imgRequestProxy()
{
  
  NS_PRECONDITION(!mListener,
                  "Someone forgot to properly cancel this request!");

  
  
  while (mLockCount) {
    UnlockImage();
  }

  ClearAnimationConsumers();

  
  
  
  
  NullOutListener();

  if (GetOwner()) {
    




    mCanceled = true;
    GetOwner()->RemoveProxy(this, NS_OK);
  }
}

nsresult
imgRequestProxy::Init(imgRequest* aOwner,
                      nsILoadGroup* aLoadGroup,
                      ImageURL* aURI,
                      imgINotificationObserver* aObserver)
{
  NS_PRECONDITION(!GetOwner() && !mListener,
                  "imgRequestProxy is already initialized");

  LOG_SCOPE_WITH_PARAM(GetImgLog(), "imgRequestProxy::Init", "request",
                       aOwner);

  MOZ_ASSERT(mAnimationConsumers == 0, "Cannot have animation before Init");

  mBehaviour->SetOwner(aOwner);
  mListener = aObserver;
  
  
  
  if (mListener) {
    mListenerIsStrongRef = true;
    NS_ADDREF(mListener);
  }
  mLoadGroup = aLoadGroup;
  mURI = aURI;

  
  if (GetOwner()) {
    GetOwner()->AddProxy(this);
  }

  return NS_OK;
}

nsresult
imgRequestProxy::ChangeOwner(imgRequest* aNewOwner)
{
  NS_PRECONDITION(GetOwner(),
                  "Cannot ChangeOwner on a proxy without an owner!");

  if (mCanceled) {
    
    
    SyncNotifyListener();
  }

  
  
  uint32_t oldLockCount = mLockCount;
  while (mLockCount) {
    UnlockImage();
  }

  
  uint32_t oldAnimationConsumers = mAnimationConsumers;
  ClearAnimationConsumers();

  
  bool wasDecoded = false;
  nsRefPtr<ProgressTracker> progressTracker = GetProgressTracker();
  if (progressTracker->HasImage() &&
      progressTracker->GetImageStatus() &
        imgIRequest::STATUS_FRAME_COMPLETE) {
    wasDecoded = true;
  }

  GetOwner()->RemoveProxy(this, NS_IMAGELIB_CHANGING_OWNER);

  mBehaviour->SetOwner(aNewOwner);

  
  for (uint32_t i = 0; i < oldLockCount; i++) {
    LockImage();
  }

  
  
  
  for (uint32_t i = 0; i < oldAnimationConsumers; i++) {
    IncrementAnimationConsumers();
  }

  GetOwner()->AddProxy(this);

  
  
  if (wasDecoded || mDecodeRequested) {
    StartDecoding();
  }

  return NS_OK;
}

void
imgRequestProxy::AddToLoadGroup()
{
  NS_ASSERTION(!mIsInLoadGroup, "Whaa, we're already in the loadgroup!");

  if (!mIsInLoadGroup && mLoadGroup) {
    mLoadGroup->AddRequest(this, nullptr);
    mIsInLoadGroup = true;
  }
}

void
imgRequestProxy::RemoveFromLoadGroup(bool releaseLoadGroup)
{
  if (!mIsInLoadGroup) {
    return;
  }

  




  nsCOMPtr<imgIRequest> kungFuDeathGrip(this);

  mLoadGroup->RemoveRequest(this, nullptr, NS_OK);
  mIsInLoadGroup = false;

  if (releaseLoadGroup) {
    
    mLoadGroup = nullptr;
  }
}





NS_IMETHODIMP
imgRequestProxy::GetName(nsACString& aName)
{
  aName.Truncate();

  if (mURI) {
    mURI->GetSpec(aName);
  }

  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::IsPending(bool* _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
imgRequestProxy::GetStatus(nsresult* aStatus)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
imgRequestProxy::Cancel(nsresult status)
{
  if (mCanceled) {
    return NS_ERROR_FAILURE;
  }

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


NS_IMETHODIMP
imgRequestProxy::CancelAndForgetObserver(nsresult aStatus)
{
  
  
  
  
  
  
  if (mCanceled && !mListener) {
    return NS_ERROR_FAILURE;
  }

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
  
  mDecodeRequested = true;

  nsRefPtr<Image> image = GetImage();
  if (image) {
    return image->StartDecoding();
  }

  if (GetOwner()) {
    GetOwner()->RequestDecode();
  }

  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::RequestDecode()
{
  
  mDecodeRequested = true;

  nsRefPtr<Image> image = GetImage();
  if (image) {
    return image->RequestDecode();
  }

  if (GetOwner()) {
    GetOwner()->RequestDecode();
  }

  return NS_OK;
}



NS_IMETHODIMP
imgRequestProxy::LockImage()
{
  mLockCount++;
  nsRefPtr<Image> image = GetImage();
  if (image) {
    return image->LockImage();
  }
  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::UnlockImage()
{
  MOZ_ASSERT(mLockCount > 0, "calling unlock but no locks!");

  mLockCount--;
  nsRefPtr<Image> image = GetImage();
  if (image) {
    return image->UnlockImage();
  }
  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::RequestDiscard()
{
  nsRefPtr<Image> image = GetImage();
  if (image) {
    return image->RequestDiscard();
  }
  return NS_OK;
}

NS_IMETHODIMP
imgRequestProxy::IncrementAnimationConsumers()
{
  mAnimationConsumers++;
  nsRefPtr<Image> image = GetImage();
  if (image) {
    image->IncrementAnimationConsumers();
  }
  return NS_OK;
}

NS_IMETHODIMP
imgRequestProxy::DecrementAnimationConsumers()
{
  
  
  
  
  
  
  if (mAnimationConsumers > 0) {
    mAnimationConsumers--;
    nsRefPtr<Image> image = GetImage();
    if (image) {
      image->DecrementAnimationConsumers();
    }
  }
  return NS_OK;
}

void
imgRequestProxy::ClearAnimationConsumers()
{
  while (mAnimationConsumers > 0) {
    DecrementAnimationConsumers();
  }
}


NS_IMETHODIMP
imgRequestProxy::Suspend()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
imgRequestProxy::Resume()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
imgRequestProxy::GetLoadGroup(nsILoadGroup** loadGroup)
{
  NS_IF_ADDREF(*loadGroup = mLoadGroup.get());
  return NS_OK;
}
NS_IMETHODIMP
imgRequestProxy::SetLoadGroup(nsILoadGroup* loadGroup)
{
  mLoadGroup = loadGroup;
  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::GetLoadFlags(nsLoadFlags* flags)
{
  *flags = mLoadFlags;
  return NS_OK;
}
NS_IMETHODIMP
imgRequestProxy::SetLoadFlags(nsLoadFlags flags)
{
  mLoadFlags = flags;
  return NS_OK;
}




NS_IMETHODIMP
imgRequestProxy::GetImage(imgIContainer** aImage)
{
  NS_ENSURE_TRUE(aImage, NS_ERROR_NULL_POINTER);
  
  
  
  
  nsRefPtr<Image> image = GetImage();
  nsCOMPtr<imgIContainer> imageToReturn;
  if (image) {
    imageToReturn = do_QueryInterface(image);
  }
  if (!imageToReturn && GetOwner()) {
    imageToReturn = GetOwner()->GetImage();
  }
  if (!imageToReturn) {
    return NS_ERROR_FAILURE;
  }

  imageToReturn.swap(*aImage);

  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::GetImageStatus(uint32_t* aStatus)
{
  nsRefPtr<ProgressTracker> progressTracker = GetProgressTracker();
  *aStatus = progressTracker->GetImageStatus();

  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::GetImageErrorCode(nsresult* aStatus)
{
  if (!GetOwner()) {
    return NS_ERROR_FAILURE;
  }

  *aStatus = GetOwner()->GetImageErrorCode();

  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::GetURI(nsIURI** aURI)
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread to convert URI");
  nsCOMPtr<nsIURI> uri = mURI->ToIURI();
  uri.forget(aURI);
  return NS_OK;
}

nsresult
imgRequestProxy::GetCurrentURI(nsIURI** aURI)
{
  if (!GetOwner()) {
    return NS_ERROR_FAILURE;
  }

  return GetOwner()->GetCurrentURI(aURI);
}

nsresult
imgRequestProxy::GetURI(ImageURL** aURI)
{
  if (!mURI) {
    return NS_ERROR_FAILURE;
  }

  NS_ADDREF(*aURI = mURI);

  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::GetNotificationObserver(imgINotificationObserver** aObserver)
{
  *aObserver = mListener;
  NS_IF_ADDREF(*aObserver);
  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::GetMimeType(char** aMimeType)
{
  if (!GetOwner()) {
    return NS_ERROR_FAILURE;
  }

  const char* type = GetOwner()->GetMimeType();
  if (!type) {
    return NS_ERROR_FAILURE;
  }

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

NS_IMETHODIMP
imgRequestProxy::Clone(imgINotificationObserver* aObserver,
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

nsresult
imgRequestProxy::PerformClone(imgINotificationObserver* aObserver,
                              imgRequestProxy* (aAllocFn)(imgRequestProxy*),
                              imgRequestProxy** aClone)
{
  NS_PRECONDITION(aClone, "Null out param");

  LOG_SCOPE(GetImgLog(), "imgRequestProxy::Clone");

  *aClone = nullptr;
  nsRefPtr<imgRequestProxy> clone = aAllocFn(this);

  
  
  
  
  
  
  clone->SetLoadFlags(mLoadFlags);
  nsresult rv = clone->Init(mBehaviour->GetOwner(), mLoadGroup,
                            mURI, aObserver);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (GetOwner() && GetOwner()->GetValidator()) {
    clone->SetNotificationsDeferred(true);
    GetOwner()->GetValidator()->AddProxy(clone);
  }

  
  
  
  NS_ADDREF(*aClone = clone);

  
  
  clone->SyncNotifyListener();

  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::GetImagePrincipal(nsIPrincipal** aPrincipal)
{
  if (!GetOwner()) {
    return NS_ERROR_FAILURE;
  }

  NS_ADDREF(*aPrincipal = GetOwner()->GetPrincipal());
  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::GetMultipart(bool* aMultipart)
{
  if (!GetOwner()) {
    return NS_ERROR_FAILURE;
  }

  *aMultipart = GetOwner()->GetMultipart();

  return NS_OK;
}


NS_IMETHODIMP
imgRequestProxy::GetCORSMode(int32_t* aCorsMode)
{
  if (!GetOwner()) {
    return NS_ERROR_FAILURE;
  }

  *aCorsMode = GetOwner()->GetCORSMode();

  return NS_OK;
}



NS_IMETHODIMP
imgRequestProxy::GetPriority(int32_t* priority)
{
  NS_ENSURE_STATE(GetOwner());
  *priority = GetOwner()->Priority();
  return NS_OK;
}

NS_IMETHODIMP
imgRequestProxy::SetPriority(int32_t priority)
{
  NS_ENSURE_STATE(GetOwner() && !mCanceled);
  GetOwner()->AdjustPriority(this, priority - GetOwner()->Priority());
  return NS_OK;
}

NS_IMETHODIMP
imgRequestProxy::AdjustPriority(int32_t priority)
{
  
  
  
  NS_ENSURE_STATE(GetOwner());
  GetOwner()->AdjustPriority(this, priority);
  return NS_OK;
}



NS_IMETHODIMP
imgRequestProxy::GetSecurityInfo(nsISupports** _retval)
{
  if (GetOwner()) {
    return GetOwner()->GetSecurityInfo(_retval);
  }

  *_retval = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
imgRequestProxy::GetHasTransferredData(bool* hasData)
{
  if (GetOwner()) {
    *hasData = GetOwner()->HasTransferredData();
  } else {
    
    *hasData = true;
  }
  return NS_OK;
}

void
imgRequestProxy::OnStartDecode()
{
  
  
  if (GetOwner()) {
    
    
    
    
    
    GetOwner()->ResetCacheEntry();
  }
}

static const char*
NotificationTypeToString(int32_t aType)
{
  switch(aType)
  {
    case imgINotificationObserver::SIZE_AVAILABLE: return "SIZE_AVAILABLE";
    case imgINotificationObserver::FRAME_UPDATE: return "FRAME_UPDATE";
    case imgINotificationObserver::FRAME_COMPLETE: return "FRAME_COMPLETE";
    case imgINotificationObserver::LOAD_COMPLETE: return "LOAD_COMPLETE";
    case imgINotificationObserver::DECODE_COMPLETE: return "DECODE_COMPLETE";
    case imgINotificationObserver::DISCARD: return "DISCARD";
    case imgINotificationObserver::UNLOCKED_DRAW: return "UNLOCKED_DRAW";
    case imgINotificationObserver::IS_ANIMATED: return "IS_ANIMATED";
    case imgINotificationObserver::HAS_TRANSPARENCY: return "HAS_TRANSPARENCY";
    default:
      NS_NOTREACHED("Notification list should be exhaustive");
      return "(unknown notification)";
  }
}

void
imgRequestProxy::Notify(int32_t aType, const nsIntRect* aRect)
{
  MOZ_ASSERT(aType != imgINotificationObserver::LOAD_COMPLETE,
             "Should call OnLoadComplete");

  LOG_FUNC_WITH_PARAM(GetImgLog(), "imgRequestProxy::Notify", "type",
                      NotificationTypeToString(aType));

  if (!mListener || mCanceled) {
    return;
  }

  
  nsCOMPtr<imgINotificationObserver> listener(mListener);

  mListener->Notify(this, aType, aRect);
}

void
imgRequestProxy::OnLoadComplete(bool aLastPart)
{
#ifdef PR_LOGGING
  nsAutoCString name;
  GetName(name);
  LOG_FUNC_WITH_PARAM(GetImgLog(), "imgRequestProxy::OnLoadComplete",
                      "name", name.get());
#endif
  
  
  
  nsCOMPtr<imgIRequest> kungFuDeathGrip(this);

  if (mListener && !mCanceled) {
    
    nsCOMPtr<imgINotificationObserver> kungFuDeathGrip(mListener);
    mListener->Notify(this, imgINotificationObserver::LOAD_COMPLETE, nullptr);
  }

  
  
  
  
  if (aLastPart || (mLoadFlags & nsIRequest::LOAD_BACKGROUND) == 0) {
    RemoveFromLoadGroup(aLastPart);
    
    
    if (!aLastPart) {
      mLoadFlags |= nsIRequest::LOAD_BACKGROUND;
      AddToLoadGroup();
    }
  }

  if (mListenerIsStrongRef && aLastPart) {
    NS_PRECONDITION(mListener, "How did that happen?");
    
    
    
    imgINotificationObserver* obs = mListener;
    mListenerIsStrongRef = false;
    NS_RELEASE(obs);
  }
}

void
imgRequestProxy::BlockOnload()
{
#ifdef PR_LOGGING
  nsAutoCString name;
  GetName(name);
  LOG_FUNC_WITH_PARAM(GetImgLog(), "imgRequestProxy::BlockOnload",
                      "name", name.get());
#endif

  nsCOMPtr<imgIOnloadBlocker> blocker = do_QueryInterface(mListener);
  if (blocker) {
    blocker->BlockOnload(this);
  }
}

void
imgRequestProxy::UnblockOnload()
{
#ifdef PR_LOGGING
  nsAutoCString name;
  GetName(name);
  LOG_FUNC_WITH_PARAM(GetImgLog(), "imgRequestProxy::UnblockOnload",
                      "name", name.get());
#endif

  nsCOMPtr<imgIOnloadBlocker> blocker = do_QueryInterface(mListener);
  if (blocker) {
    blocker->UnblockOnload(this);
  }
}

void
imgRequestProxy::NullOutListener()
{
  
  if (mListener) {
    ClearAnimationConsumers();
  }

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
  imgRequestProxy* proxy;
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

void
imgRequestProxy::NotifyListener()
{
  
  
  
  

  nsRefPtr<ProgressTracker> progressTracker = GetProgressTracker();
  if (GetOwner()) {
    
    progressTracker->Notify(this);
  } else {
    
    
    MOZ_ASSERT(HasImage(),
               "if we have no imgRequest, we should have an Image");
    progressTracker->NotifyCurrentState(this);
  }
}

void
imgRequestProxy::SyncNotifyListener()
{
  
  
  
  

  nsRefPtr<ProgressTracker> progressTracker = GetProgressTracker();
  progressTracker->SyncNotify(this);
}

void
imgRequestProxy::SetHasImage()
{
  nsRefPtr<ProgressTracker> progressTracker = GetProgressTracker();
  MOZ_ASSERT(progressTracker);
  nsRefPtr<Image> image = progressTracker->GetImage();
  MOZ_ASSERT(image);

  
  
  mBehaviour->SetOwner(mBehaviour->GetOwner());

  
  for (uint32_t i = 0; i < mLockCount; ++i) {
    image->LockImage();
  }

  
  for (uint32_t i = 0; i < mAnimationConsumers; i++) {
    image->IncrementAnimationConsumers();
  }
}

already_AddRefed<ProgressTracker>
imgRequestProxy::GetProgressTracker() const
{
  return mBehaviour->GetProgressTracker();
}

already_AddRefed<mozilla::image::Image>
imgRequestProxy::GetImage() const
{
  return mBehaviour->GetImage();
}

bool
RequestBehaviour::HasImage() const
{
  if (!mOwnerHasImage) {
    return false;
  }
  nsRefPtr<ProgressTracker> progressTracker = GetProgressTracker();
  return progressTracker ? progressTracker->HasImage() : false;
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
  GetImage() const override {
    nsRefPtr<mozilla::image::Image> image = mImage;
    return image.forget();
  }

  virtual bool HasImage() const override {
    return mImage;
  }

  virtual already_AddRefed<ProgressTracker> GetProgressTracker()
    const override  {
    return mImage->GetProgressTracker();
  }

  virtual imgRequest* GetOwner() const override {
    return nullptr;
  }

  virtual void SetOwner(imgRequest* aOwner) override {
    MOZ_ASSERT(!aOwner,
               "We shouldn't be giving static requests a non-null owner.");
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

NS_IMETHODIMP
imgRequestProxyStatic::GetImagePrincipal(nsIPrincipal** aPrincipal)
{
  if (!mPrincipal) {
    return NS_ERROR_FAILURE;
  }

  NS_ADDREF(*aPrincipal = mPrincipal);

  return NS_OK;
}

nsresult
imgRequestProxyStatic::Clone(imgINotificationObserver* aObserver,
                             imgRequestProxy** aClone)
{
  return PerformClone(aObserver, NewStaticProxy, aClone);
}
