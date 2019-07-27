



#include "CameraControlImpl.h"
#include "base/basictypes.h"
#include "mozilla/Assertions.h"
#include "mozilla/unused.h"
#include "nsPrintfCString.h"
#include "nsIWeakReferenceUtils.h"
#include "CameraCommon.h"
#include "nsGlobalWindow.h"
#include "DeviceStorageFileDescriptor.h"
#include "CameraControlListener.h"

using namespace mozilla;

 StaticRefPtr<nsIThread> CameraControlImpl::sCameraThread;

CameraControlImpl::CameraControlImpl()
  : mListenerLock(PR_NewRWLock(PR_RWLOCK_RANK_NONE, "CameraControlImpl.Listeners.Lock"))
  , mPreviewState(CameraControlListener::kPreviewStopped)
  , mHardwareState(CameraControlListener::kHardwareUninitialized)
  , mHardwareStateChangeReason(NS_OK)
{
  DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  mCurrentConfiguration.mMode = ICameraControl::kUnspecifiedMode;

  class Delegate : public nsRunnable
  {
  public:
    NS_IMETHOD
    Run()
    {
      char stackBaseGuess;
      profiler_register_thread("CameraThread", &stackBaseGuess);
      return NS_OK;
    }
  };

  
  nsCOMPtr<nsIThread> ct = do_QueryInterface(sCameraThread);
  if (ct) {
    mCameraThread = ct.forget();
  } else {
    nsresult rv = NS_NewNamedThread("CameraThread", getter_AddRefs(mCameraThread));
    if (NS_FAILED(rv)) {
      MOZ_CRASH("Failed to create new Camera Thread");
    }
    mCameraThread->Dispatch(new Delegate(), NS_DISPATCH_NORMAL);
    sCameraThread = mCameraThread;
  }

  
  
  
  
  
  
  
  
  
  
  
  if (!mListenerLock) {
    MOZ_CRASH("Out of memory getting new PRRWLock");
  }
}

CameraControlImpl::~CameraControlImpl()
{
  DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);

  MOZ_ASSERT(mListenerLock, "mListenerLock missing in ~CameraControlImpl()");
  if (mListenerLock) {
    PR_DestroyRWLock(mListenerLock);
    mListenerLock = nullptr;
  }
}

void
CameraControlImpl::OnHardwareStateChange(CameraControlListener::HardwareState aNewState,
                                         nsresult aReason)
{
  
  
  
  RwLockAutoEnterRead lock(mListenerLock);

  if (aNewState == mHardwareState) {
    DOM_CAMERA_LOGI("OnHardwareStateChange: state did not change from %d\n", mHardwareState);
    return;
  }

#ifdef PR_LOGGING
  const char* state[] = { "uninitialized", "closed", "open", "failed" };
  MOZ_ASSERT(aNewState >= 0);
  if (static_cast<unsigned int>(aNewState) < sizeof(state) / sizeof(state[0])) {
    DOM_CAMERA_LOGI("New hardware state is '%s' (reason=0x%x)\n",
      state[aNewState], aReason);
  } else {
    DOM_CAMERA_LOGE("OnHardwareStateChange: got invalid HardwareState value %d\n", aNewState);
  }
#endif

  mHardwareState = aNewState;
  mHardwareStateChangeReason = aReason;

  for (uint32_t i = 0; i < mListeners.Length(); ++i) {
    CameraControlListener* l = mListeners[i];
    l->OnHardwareStateChange(mHardwareState, mHardwareStateChangeReason);
  }
}

void
CameraControlImpl::OnConfigurationChange()
{
  MOZ_ASSERT(NS_GetCurrentThread() == mCameraThread);
  RwLockAutoEnterRead lock(mListenerLock);

  DOM_CAMERA_LOGI("OnConfigurationChange : %zu listeners\n", mListeners.Length());

  for (uint32_t i = 0; i < mListeners.Length(); ++i) {
    CameraControlListener* l = mListeners[i];
    l->OnConfigurationChange(mCurrentConfiguration);
  }
}

void
CameraControlImpl::OnAutoFocusComplete(bool aAutoFocusSucceeded)
{
  
  
  
  RwLockAutoEnterRead lock(mListenerLock);

  for (uint32_t i = 0; i < mListeners.Length(); ++i) {
    CameraControlListener* l = mListeners[i];
    l->OnAutoFocusComplete(aAutoFocusSucceeded);
  }
}

void
CameraControlImpl::OnAutoFocusMoving(bool aIsMoving)
{
  RwLockAutoEnterRead lock(mListenerLock);

  for (uint32_t i = 0; i < mListeners.Length(); ++i) {
    CameraControlListener* l = mListeners[i];
    l->OnAutoFocusMoving(aIsMoving);
  }
}

void
CameraControlImpl::OnFacesDetected(const nsTArray<Face>& aFaces)
{
  
  
  
  RwLockAutoEnterRead lock(mListenerLock);

  for (uint32_t i = 0; i < mListeners.Length(); ++i) {
    CameraControlListener* l = mListeners[i];
    l->OnFacesDetected(aFaces);
  }
}

void
CameraControlImpl::OnTakePictureComplete(const uint8_t* aData, uint32_t aLength, const nsAString& aMimeType)
{
  
  
  
  RwLockAutoEnterRead lock(mListenerLock);

  for (uint32_t i = 0; i < mListeners.Length(); ++i) {
    CameraControlListener* l = mListeners[i];
    l->OnTakePictureComplete(aData, aLength, aMimeType);
  }
}

void
CameraControlImpl::OnShutter()
{
  
  
  
  RwLockAutoEnterRead lock(mListenerLock);

  for (uint32_t i = 0; i < mListeners.Length(); ++i) {
    CameraControlListener* l = mListeners[i];
    l->OnShutter();
  }
}

void
CameraControlImpl::OnRecorderStateChange(CameraControlListener::RecorderState aState,
                                         int32_t aStatus, int32_t aTrackNumber)
{
  
  
  
  RwLockAutoEnterRead lock(mListenerLock);

  for (uint32_t i = 0; i < mListeners.Length(); ++i) {
    CameraControlListener* l = mListeners[i];
    l->OnRecorderStateChange(aState, aStatus, aTrackNumber);
  }
}

void
CameraControlImpl::OnPreviewStateChange(CameraControlListener::PreviewState aNewState)
{
  
  
  
  RwLockAutoEnterRead lock(mListenerLock);

  if (aNewState == mPreviewState) {
    DOM_CAMERA_LOGI("OnPreviewStateChange: state did not change from %d\n", mPreviewState);
    return;
  }

#ifdef PR_LOGGING
  const char* state[] = { "stopped", "paused", "started" };
  MOZ_ASSERT(aNewState >= 0);
  if (static_cast<unsigned int>(aNewState) < sizeof(state) / sizeof(state[0])) {
    DOM_CAMERA_LOGI("New preview state is '%s'\n", state[aNewState]);
  } else {
    DOM_CAMERA_LOGE("OnPreviewStateChange: got unknown PreviewState value %d\n", aNewState);
  }
#endif

  mPreviewState = aNewState;

  for (uint32_t i = 0; i < mListeners.Length(); ++i) {
    CameraControlListener* l = mListeners[i];
    l->OnPreviewStateChange(mPreviewState);
  }
}

void
CameraControlImpl::OnRateLimitPreview(bool aLimit)
{
  
  RwLockAutoEnterRead lock(mListenerLock);

  DOM_CAMERA_LOGI("OnRateLimitPreview: %d\n", aLimit);

  for (uint32_t i = 0; i < mListeners.Length(); ++i) {
    CameraControlListener* l = mListeners[i];
    l->OnRateLimitPreview(aLimit);
  }
}

bool
CameraControlImpl::OnNewPreviewFrame(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight)
{
  
  
  RwLockAutoEnterRead lock(mListenerLock);

  DOM_CAMERA_LOGI("OnNewPreviewFrame: we have %zu preview frame listener(s)\n",
    mListeners.Length());

  bool consumed = false;

  for (uint32_t i = 0; i < mListeners.Length(); ++i) {
    CameraControlListener* l = mListeners[i];
    consumed = l->OnNewPreviewFrame(aImage, aWidth, aHeight) || consumed;
  }
  return consumed;
}

void
CameraControlImpl::OnUserError(CameraControlListener::UserContext aContext,
                               nsresult aError)
{
  
  
  RwLockAutoEnterRead lock(mListenerLock);

#ifdef PR_LOGGING
  const char* context[] = {
    "StartCamera",
    "StopCamera",
    "AutoFocus",
    "StartFaceDetection",
    "StopFaceDetection",
    "TakePicture",
    "StartRecording",
    "StopRecording",
    "SetConfiguration",
    "StartPreview",
    "StopPreview",
    "SetPictureSize",
    "SetThumbnailSize",
    "ResumeContinuousFocus",
    "Unspecified"
  };
  if (static_cast<size_t>(aContext) < sizeof(context) / sizeof(context[0])) {
    DOM_CAMERA_LOGW("CameraControlImpl::OnUserError : aContext='%s' (%d), aError=0x%x\n",
      context[aContext], aContext, aError);
  } else {
    DOM_CAMERA_LOGE("CameraControlImpl::OnUserError : aContext=%d, aError=0x%x\n",
      aContext, aError);
  }
#endif

  for (uint32_t i = 0; i < mListeners.Length(); ++i) {
    CameraControlListener* l = mListeners[i];
    l->OnUserError(aContext, aError);
  }
}

void
CameraControlImpl::OnSystemError(CameraControlListener::SystemContext aContext,
                                 nsresult aError)
{
  
  
  RwLockAutoEnterRead lock(mListenerLock);

#ifdef PR_LOGGING
  const char* context[] = {
    "Camera Service"
  };
  if (static_cast<size_t>(aContext) < sizeof(context) / sizeof(context[0])) {
    DOM_CAMERA_LOGW("CameraControlImpl::OnSystemError : aContext='%s' (%d), aError=0x%x\n",
      context[aContext], aContext, aError);
  } else {
    DOM_CAMERA_LOGE("CameraControlImpl::OnSystemError : aContext=%d, aError=0x%x\n",
      aContext, aError);
  }
#endif

  for (uint32_t i = 0; i < mListeners.Length(); ++i) {
    CameraControlListener* l = mListeners[i];
    l->OnSystemError(aContext, aError);
  }
}




class CameraControlImpl::ControlMessage : public nsRunnable
{
public:
  ControlMessage(CameraControlImpl* aCameraControl,
                 CameraControlListener::UserContext aContext)
    : mCameraControl(aCameraControl)
    , mContext(aContext)
  { }

  virtual nsresult RunImpl() = 0;

  NS_IMETHOD
  Run() override
  {
    MOZ_ASSERT(mCameraControl);
    MOZ_ASSERT(NS_GetCurrentThread() == mCameraControl->mCameraThread);

    nsresult rv = RunImpl();
    if (NS_FAILED(rv)) {
      nsPrintfCString msg("Camera control API(%d) failed with 0x%x", mContext, rv);
      NS_WARNING(msg.get());
      mCameraControl->OnUserError(mContext, rv);
    }

    return NS_OK;
  }

protected:
  virtual ~ControlMessage() { }

  nsRefPtr<CameraControlImpl> mCameraControl;
  CameraControlListener::UserContext mContext;
};

nsresult
CameraControlImpl::Dispatch(ControlMessage* aMessage)
{
  nsresult rv = mCameraThread->Dispatch(aMessage, NS_DISPATCH_NORMAL);
  if (NS_SUCCEEDED(rv)) {
    return NS_OK;
  }

  nsPrintfCString msg("Failed to dispatch camera control message (0x%x)", rv);
  NS_WARNING(msg.get());
  return NS_ERROR_FAILURE;
}

nsresult
CameraControlImpl::Start(const Configuration* aConfig)
{
  class Message : public ControlMessage
  {
  public:
    Message(CameraControlImpl* aCameraControl,
            CameraControlListener::UserContext aContext,
            const Configuration* aConfig)
      : ControlMessage(aCameraControl, aContext)
      , mHaveInitialConfig(false)
    {
      if (aConfig) {
        mConfig = *aConfig;
        mHaveInitialConfig = true;
      }
    }

    nsresult
    RunImpl() override
    {
      if (mHaveInitialConfig) {
        return mCameraControl->StartImpl(&mConfig);
      }
      return mCameraControl->StartImpl();
    }

  protected:
    bool mHaveInitialConfig;
    Configuration mConfig;
  };

  return Dispatch(new Message(this, CameraControlListener::kInStartCamera, aConfig));
}

nsresult
CameraControlImpl::SetConfiguration(const Configuration& aConfig)
{
  class Message : public ControlMessage
  {
  public:
    Message(CameraControlImpl* aCameraControl,
            CameraControlListener::UserContext aContext,
            const Configuration& aConfig)
      : ControlMessage(aCameraControl, aContext)
      , mConfig(aConfig)
    { }

    nsresult
    RunImpl() override
    {
      return mCameraControl->SetConfigurationImpl(mConfig);
    }

  protected:
    Configuration mConfig;
  };

  return Dispatch(new Message(this, CameraControlListener::kInSetConfiguration, aConfig));
}

nsresult
CameraControlImpl::AutoFocus()
{
  class Message : public ControlMessage
  {
  public:
    Message(CameraControlImpl* aCameraControl,
            CameraControlListener::UserContext aContext)
      : ControlMessage(aCameraControl, aContext)
    { }

    nsresult
    RunImpl() override
    {
      return mCameraControl->AutoFocusImpl();
    }
  };

  return Dispatch(new Message(this, CameraControlListener::kInAutoFocus));
}

nsresult
CameraControlImpl::StartFaceDetection()
{
  class Message : public ControlMessage
  {
  public:
    Message(CameraControlImpl* aCameraControl,
            CameraControlListener::UserContext aContext)
      : ControlMessage(aCameraControl, aContext)
    { }

    nsresult
    RunImpl() override
    {
      return mCameraControl->StartFaceDetectionImpl();
    }
  };

  return Dispatch(new Message(this, CameraControlListener::kInStartFaceDetection));
}

nsresult
CameraControlImpl::StopFaceDetection()
{
  class Message : public ControlMessage
  {
  public:
    Message(CameraControlImpl* aCameraControl,
            CameraControlListener::UserContext aContext)
      : ControlMessage(aCameraControl, aContext)
    { }

    nsresult
    RunImpl() override
    {
      return mCameraControl->StopFaceDetectionImpl();
    }
  };

  return Dispatch(new Message(this, CameraControlListener::kInStopFaceDetection));
}

nsresult
CameraControlImpl::TakePicture()
{
  class Message : public ControlMessage
  {
  public:
    Message(CameraControlImpl* aCameraControl,
            CameraControlListener::UserContext aContext)
      : ControlMessage(aCameraControl, aContext)
    { }

    nsresult
    RunImpl() override
    {
      return mCameraControl->TakePictureImpl();
    }
  };

  return Dispatch(new Message(this, CameraControlListener::kInTakePicture));
}

nsresult
CameraControlImpl::StartRecording(DeviceStorageFileDescriptor* aFileDescriptor,
                                  const StartRecordingOptions* aOptions)
{
  class Message : public ControlMessage
  {
  public:
    Message(CameraControlImpl* aCameraControl,
            CameraControlListener::UserContext aContext,
            const StartRecordingOptions* aOptions,
            DeviceStorageFileDescriptor* aFileDescriptor)
      : ControlMessage(aCameraControl, aContext)
      , mOptionsPassed(false)
      , mFileDescriptor(aFileDescriptor)
    {
      if (aOptions) {
        mOptions = *aOptions;
        mOptionsPassed = true;
      }
    }

    nsresult
    RunImpl() override
    {
      return mCameraControl->StartRecordingImpl(mFileDescriptor,
        mOptionsPassed ? &mOptions : nullptr);
    }

  protected:
    StartRecordingOptions mOptions;
    bool mOptionsPassed;
    nsRefPtr<DeviceStorageFileDescriptor> mFileDescriptor;
  };

  if (!aFileDescriptor) {
    return NS_ERROR_INVALID_ARG;
  }
  return Dispatch(new Message(this, CameraControlListener::kInStartRecording,
    aOptions, aFileDescriptor));
}

nsresult
CameraControlImpl::StopRecording()
{
  class Message : public ControlMessage
  {
  public:
    Message(CameraControlImpl* aCameraControl,
            CameraControlListener::UserContext aContext)
      : ControlMessage(aCameraControl, aContext)
    { }

    nsresult
    RunImpl() override
    {
      return mCameraControl->StopRecordingImpl();
    }
  };

  return Dispatch(new Message(this, CameraControlListener::kInStopRecording));
}

nsresult
CameraControlImpl::StartPreview()
{
  class Message : public ControlMessage
  {
  public:
    Message(CameraControlImpl* aCameraControl,
            CameraControlListener::UserContext aContext)
      : ControlMessage(aCameraControl, aContext)
    { }

    nsresult
    RunImpl() override
    {
      return mCameraControl->StartPreviewImpl();
    }
  };

  return Dispatch(new Message(this, CameraControlListener::kInStartPreview));
}

nsresult
CameraControlImpl::StopPreview()
{
  class Message : public ControlMessage
  {
  public:
    Message(CameraControlImpl* aCameraControl,
            CameraControlListener::UserContext aContext)
      : ControlMessage(aCameraControl, aContext)
    { }

    nsresult
    RunImpl() override
    {
      return mCameraControl->StopPreviewImpl();
    }
  };

  return Dispatch(new Message(this, CameraControlListener::kInStopPreview));
}

nsresult
CameraControlImpl::ResumeContinuousFocus()
{
  class Message : public ControlMessage
  {
  public:
    Message(CameraControlImpl* aCameraControl,
            CameraControlListener::UserContext aContext)
      : ControlMessage(aCameraControl, aContext)
    { }

    nsresult
    RunImpl() override
    {
      return mCameraControl->ResumeContinuousFocusImpl();
    }
  };

  return Dispatch(new Message(this, CameraControlListener::kInResumeContinuousFocus));
}

nsresult
CameraControlImpl::Stop()
{
  class Message : public ControlMessage
  {
  public:
    Message(CameraControlImpl* aCameraControl,
            CameraControlListener::UserContext aContext)
      : ControlMessage(aCameraControl, aContext)
    { }

    nsresult
    RunImpl() override
    {
      return mCameraControl->StopImpl();
    }
  };

  return Dispatch(new Message(this, CameraControlListener::kInStopCamera));
}

class CameraControlImpl::ListenerMessage : public CameraControlImpl::ControlMessage
{
public:
  ListenerMessage(CameraControlImpl* aCameraControl,
                  CameraControlListener* aListener)
    : ControlMessage(aCameraControl, CameraControlListener::kInUnspecified)
    , mListener(aListener)
  { }

protected:
  nsRefPtr<CameraControlListener> mListener;
};

void
CameraControlImpl::AddListenerImpl(already_AddRefed<CameraControlListener> aListener)
{
  RwLockAutoEnterWrite lock(mListenerLock);

  CameraControlListener* l = *mListeners.AppendElement() = aListener;
  DOM_CAMERA_LOGI("Added camera control listener %p\n", l);

  
  l->OnConfigurationChange(mCurrentConfiguration);
  l->OnHardwareStateChange(mHardwareState, mHardwareStateChangeReason);
  l->OnPreviewStateChange(mPreviewState);
}

void
CameraControlImpl::AddListener(CameraControlListener* aListener)
 {
  class Message : public ListenerMessage
  {
  public:
    Message(CameraControlImpl* aCameraControl,
            CameraControlListener* aListener)
      : ListenerMessage(aCameraControl, aListener)
    { }

    nsresult
    RunImpl() override
    {
      mCameraControl->AddListenerImpl(mListener.forget());
      return NS_OK;
    }
  };

  if (aListener) {
    Dispatch(new Message(this, aListener));
  }
}

void
CameraControlImpl::RemoveListenerImpl(CameraControlListener* aListener)
{
  RwLockAutoEnterWrite lock(mListenerLock);

  nsRefPtr<CameraControlListener> l(aListener);
  mListeners.RemoveElement(l);
  DOM_CAMERA_LOGI("Removed camera control listener %p\n", l.get());
  
}

void
CameraControlImpl::RemoveListener(CameraControlListener* aListener)
 {
  class Message : public ListenerMessage
  {
  public:
    Message(CameraControlImpl* aCameraControl, CameraControlListener* aListener)
      : ListenerMessage(aCameraControl, aListener)
    { }

    nsresult
    RunImpl() override
    {
      mCameraControl->RemoveListenerImpl(mListener);
      return NS_OK;
    }
  };

  if (aListener) {
    Dispatch(new Message(this, aListener));
  }
}
