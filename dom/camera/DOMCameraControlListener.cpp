



#include "DOMCameraControlListener.h"
#include "nsThreadUtils.h"
#include "CameraCommon.h"
#include "DOMCameraControl.h"
#include "CameraPreviewMediaStream.h"
#include "mozilla/dom/CameraManagerBinding.h"
#include "mozilla/dom/File.h"

using namespace mozilla;
using namespace mozilla::dom;

DOMCameraControlListener::DOMCameraControlListener(nsDOMCameraControl* aDOMCameraControl,
                                                   CameraPreviewMediaStream* aStream)
  : mDOMCameraControl(new nsMainThreadPtrHolder<nsDOMCameraControl>(aDOMCameraControl))
  , mStream(aStream)
{
  DOM_CAMERA_LOGT("%s:%d : this=%p, camera=%p, stream=%p\n",
    __func__, __LINE__, this, aDOMCameraControl, aStream);
}

DOMCameraControlListener::~DOMCameraControlListener()
{
  DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
}


class DOMCameraControlListener::DOMCallback : public nsRunnable
{
public:
  explicit DOMCallback(nsMainThreadPtrHandle<nsDOMCameraControl> aDOMCameraControl)
    : mDOMCameraControl(aDOMCameraControl)
  {
    MOZ_COUNT_CTOR(DOMCameraControlListener::DOMCallback);
  }

protected:
  virtual ~DOMCallback()
  {
    MOZ_COUNT_DTOR(DOMCameraControlListener::DOMCallback);
  }

public:
  virtual void RunCallback(nsDOMCameraControl* aDOMCameraControl) = 0;

  NS_IMETHOD
  Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    nsRefPtr<nsDOMCameraControl> camera = mDOMCameraControl.get();
    if (camera) {
      RunCallback(camera);
    }
    return NS_OK;
  }

protected:
  nsMainThreadPtrHandle<nsDOMCameraControl> mDOMCameraControl;
};


void
DOMCameraControlListener::OnHardwareStateChange(HardwareState aState,
                                                nsresult aReason)
{
  class Callback : public DOMCallback
  {
  public:
    Callback(nsMainThreadPtrHandle<nsDOMCameraControl> aDOMCameraControl,
             HardwareState aState, nsresult aReason)
      : DOMCallback(aDOMCameraControl)
      , mState(aState)
      , mReason(aReason)
    { }

    void
    RunCallback(nsDOMCameraControl* aDOMCameraControl) MOZ_OVERRIDE
    {
      aDOMCameraControl->OnHardwareStateChange(mState, mReason);
    }

  protected:
    HardwareState mState;
    nsresult mReason;
  };

  NS_DispatchToMainThread(new Callback(mDOMCameraControl, aState, aReason));
}

void
DOMCameraControlListener::OnPreviewStateChange(PreviewState aState)
{
  class Callback : public DOMCallback
  {
  public:
    Callback(nsMainThreadPtrHandle<nsDOMCameraControl> aDOMCameraControl,
             PreviewState aState)
      : DOMCallback(aDOMCameraControl)
      , mState(aState)
    { }

    void
    RunCallback(nsDOMCameraControl* aDOMCameraControl) MOZ_OVERRIDE
    {
      aDOMCameraControl->OnPreviewStateChange(mState);
    }

  protected:
    PreviewState mState;
  };

  switch (aState) {
    case kPreviewStopped:
      
      
      
      
      DOM_CAMERA_LOGI("Preview stopped, clearing current frame\n");
      mStream->ClearCurrentFrame();
      break;

    case kPreviewPaused:
      
      
      
      
      DOM_CAMERA_LOGI("Preview paused\n");
      break;

    case kPreviewStarted:
      DOM_CAMERA_LOGI("Preview started\n");
      break;

    default:
      DOM_CAMERA_LOGE("Unknown preview state %d\n", aState);
      MOZ_ASSERT_UNREACHABLE("Invalid preview state");
      return;
  }
  mStream->OnPreviewStateChange(aState == kPreviewStarted);
  NS_DispatchToMainThread(new Callback(mDOMCameraControl, aState));
}

void
DOMCameraControlListener::OnRecorderStateChange(RecorderState aState,
                                                int32_t aStatus, int32_t aTrackNum)
{
  class Callback : public DOMCallback
  {
  public:
    Callback(nsMainThreadPtrHandle<nsDOMCameraControl> aDOMCameraControl,
             RecorderState aState,
             int32_t aStatus,
             int32_t aTrackNum)
      : DOMCallback(aDOMCameraControl)
      , mState(aState)
      , mStatus(aStatus)
      , mTrackNum(aTrackNum)
    { }

    void
    RunCallback(nsDOMCameraControl* aDOMCameraControl) MOZ_OVERRIDE
    {
      aDOMCameraControl->OnRecorderStateChange(mState, mStatus, mTrackNum);
    }

  protected:
    RecorderState mState;
    int32_t mStatus;
    int32_t mTrackNum;
  };

  NS_DispatchToMainThread(new Callback(mDOMCameraControl, aState, aStatus, aTrackNum));
}

void
DOMCameraControlListener::OnConfigurationChange(const CameraListenerConfiguration& aConfiguration)
{
  class Callback : public DOMCallback
  {
  public:
    Callback(nsMainThreadPtrHandle<nsDOMCameraControl> aDOMCameraControl,
             const CameraListenerConfiguration& aConfiguration)
      : DOMCallback(aDOMCameraControl)
      , mConfiguration(aConfiguration)
    { }

    void
    RunCallback(nsDOMCameraControl* aDOMCameraControl) MOZ_OVERRIDE
    {
      nsRefPtr<nsDOMCameraControl::DOMCameraConfiguration> config =
        new nsDOMCameraControl::DOMCameraConfiguration();

      switch (mConfiguration.mMode) {
        case ICameraControl::kVideoMode:
          config->mMode = CameraMode::Video;
          break;

        case ICameraControl::kPictureMode:
          config->mMode = CameraMode::Picture;
          break;

        default:
          DOM_CAMERA_LOGI("Camera mode still unspecified, nothing to do\n");
          return;
      }

      
      config->mRecorderProfile = mConfiguration.mRecorderProfile;
      config->mPreviewSize.mWidth = mConfiguration.mPreviewSize.width;
      config->mPreviewSize.mHeight = mConfiguration.mPreviewSize.height;
      config->mMaxMeteringAreas = mConfiguration.mMaxMeteringAreas;
      config->mMaxFocusAreas = mConfiguration.mMaxFocusAreas;

      aDOMCameraControl->OnConfigurationChange(config);
    }

  protected:
    const CameraListenerConfiguration mConfiguration;
  };

  NS_DispatchToMainThread(new Callback(mDOMCameraControl, aConfiguration));
}

void
DOMCameraControlListener::OnAutoFocusMoving(bool aIsMoving)
{
  class Callback : public DOMCallback
  {
  public:
    Callback(nsMainThreadPtrHandle<nsDOMCameraControl> aDOMCameraControl, bool aIsMoving)
      : DOMCallback(aDOMCameraControl)
      , mIsMoving(aIsMoving)
    { }

    void
    RunCallback(nsDOMCameraControl* aDOMCameraControl) MOZ_OVERRIDE
    {
      aDOMCameraControl->OnAutoFocusMoving(mIsMoving);
    }

  protected:
    bool mIsMoving;
  };

  NS_DispatchToMainThread(new Callback(mDOMCameraControl, aIsMoving));
}

void
DOMCameraControlListener::OnFacesDetected(const nsTArray<ICameraControl::Face>& aFaces)
{
  class Callback : public DOMCallback
  {
  public:
    Callback(nsMainThreadPtrHandle<nsDOMCameraControl> aDOMCameraControl,
             const nsTArray<ICameraControl::Face>& aFaces)
      : DOMCallback(aDOMCameraControl)
      , mFaces(aFaces)
    { }

    void
    RunCallback(nsDOMCameraControl* aDOMCameraControl) MOZ_OVERRIDE
    {
      aDOMCameraControl->OnFacesDetected(mFaces);
    }

  protected:
    const nsTArray<ICameraControl::Face> mFaces;
  };

  NS_DispatchToMainThread(new Callback(mDOMCameraControl, aFaces));
}

void
DOMCameraControlListener::OnShutter()
{
  class Callback : public DOMCallback
  {
  public:
    explicit Callback(nsMainThreadPtrHandle<nsDOMCameraControl> aDOMCameraControl)
      : DOMCallback(aDOMCameraControl)
    { }

    void
    RunCallback(nsDOMCameraControl* aDOMCameraControl) MOZ_OVERRIDE
    {
      aDOMCameraControl->OnShutter();
    }
  };

  NS_DispatchToMainThread(new Callback(mDOMCameraControl));
}

void
DOMCameraControlListener::OnRateLimitPreview(bool aLimit)
{
  mStream->RateLimit(aLimit);
}

bool
DOMCameraControlListener::OnNewPreviewFrame(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight)
{
  DOM_CAMERA_LOGI("OnNewPreviewFrame: got %d x %d frame\n", aWidth, aHeight);

  mStream->SetCurrentFrame(gfxIntSize(aWidth, aHeight), aImage);
  return true;
}

void
DOMCameraControlListener::OnAutoFocusComplete(bool aAutoFocusSucceeded)
{
  class Callback : public DOMCallback
  {
  public:
    Callback(nsMainThreadPtrHandle<nsDOMCameraControl> aDOMCameraControl,
             bool aAutoFocusSucceeded)
      : DOMCallback(aDOMCameraControl)
      , mAutoFocusSucceeded(aAutoFocusSucceeded)
    { }

    void
    RunCallback(nsDOMCameraControl* aDOMCameraControl) MOZ_OVERRIDE
    {
      aDOMCameraControl->OnAutoFocusComplete(mAutoFocusSucceeded);
    }

  protected:
    bool mAutoFocusSucceeded;
  };

  NS_DispatchToMainThread(new Callback(mDOMCameraControl, aAutoFocusSucceeded));
}

void
DOMCameraControlListener::OnTakePictureComplete(uint8_t* aData, uint32_t aLength, const nsAString& aMimeType)
{
  class Callback : public DOMCallback
  {
  public:
    Callback(nsMainThreadPtrHandle<nsDOMCameraControl> aDOMCameraControl,
             uint8_t* aData, uint32_t aLength, const nsAString& aMimeType)
      : DOMCallback(aDOMCameraControl)
      , mData(aData)
      , mLength(aLength)
      , mMimeType(aMimeType)
    { }

    void
    RunCallback(nsDOMCameraControl* aDOMCameraControl) MOZ_OVERRIDE
    {
      nsCOMPtr<nsIDOMBlob> picture =
        File::CreateMemoryFile(mDOMCameraControl,
                               static_cast<void*>(mData),
                               static_cast<uint64_t>(mLength),
                               mMimeType);
      aDOMCameraControl->OnTakePictureComplete(picture);
    }

  protected:
    uint8_t* mData;
    uint32_t mLength;
    nsString mMimeType;
  };

  NS_DispatchToMainThread(new Callback(mDOMCameraControl, aData, aLength, aMimeType));
}

void
DOMCameraControlListener::OnUserError(UserContext aContext, nsresult aError)
{
  class Callback : public DOMCallback
  {
  public:
    Callback(nsMainThreadPtrHandle<nsDOMCameraControl> aDOMCameraControl,
             UserContext aContext,
             nsresult aError)
      : DOMCallback(aDOMCameraControl)
      , mContext(aContext)
      , mError(aError)
    { }

    virtual void
    RunCallback(nsDOMCameraControl* aDOMCameraControl) MOZ_OVERRIDE
    {
      aDOMCameraControl->OnUserError(mContext, mError);
    }

  protected:
    UserContext mContext;
    nsresult mError;
  };

  NS_DispatchToMainThread(new Callback(mDOMCameraControl, aContext, aError));
}
