















#include "GonkCameraControl.h"
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>
#include "base/basictypes.h"
#include "camera/CameraParameters.h"
#include "nsCOMPtr.h"
#include "nsMemory.h"
#include "nsThread.h"
#include <media/MediaProfiles.h>
#include "mozilla/FileUtils.h"
#include "mozilla/Services.h"
#include "mozilla/unused.h"
#include "mozilla/ipc/FileDescriptorUtils.h"
#include "nsAlgorithm.h"
#include <media/mediaplayer.h>
#include "nsPrintfCString.h"
#include "nsIObserverService.h"
#include "nsIVolume.h"
#include "nsIVolumeService.h"
#include "AutoRwLock.h"
#include "GonkCameraHwMgr.h"
#include "GonkRecorderProfiles.h"
#include "CameraCommon.h"
#include "GonkCameraParameters.h"
#include "DeviceStorageFileDescriptor.h"

using namespace mozilla;
using namespace mozilla::layers;
using namespace mozilla::gfx;
using namespace mozilla::ipc;
using namespace android;

#define RETURN_IF_NO_CAMERA_HW()                                          \
  do {                                                                    \
    if (!mCameraHw.get()) {                                               \
      NS_WARNING("Camera hardware is not initialized");                   \
      DOM_CAMERA_LOGE("%s:%d : mCameraHw is null\n", __func__, __LINE__); \
      return NS_ERROR_NOT_INITIALIZED;                                    \
    }                                                                     \
  } while(0)


nsGonkCameraControl::nsGonkCameraControl(uint32_t aCameraId)
  : CameraControlImpl(aCameraId)
  , mLastPictureSize({0, 0})
  , mLastThumbnailSize({0, 0})
  , mPreviewFps(30)
  , mResumePreviewAfterTakingPicture(false) 
  , mFlashSupported(false)
  , mLuminanceSupported(false)
  , mAutoFlashModeOverridden(false)
  , mSeparateVideoAndPreviewSizesSupported(false)
  , mDeferConfigUpdate(0)
  , mRecorder(nullptr)
  , mRecorderMonitor("GonkCameraControl::mRecorder.Monitor")
  , mVideoFile(nullptr)
  , mReentrantMonitor("GonkCameraControl::OnTakePicture.Monitor")
{
  
  DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  mImageContainer = LayerManager::CreateImageContainer();
}

nsresult
nsGonkCameraControl::StartImpl(const Configuration* aInitialConfig)
{
  


















  MOZ_ASSERT(NS_GetCurrentThread() == mCameraThread);

  nsresult rv = Initialize();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  if (aInitialConfig) {
    rv = SetConfigurationInternal(*aInitialConfig);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      
      StopImpl();
      return rv;
    }
  }

  OnHardwareStateChange(CameraControlListener::kHardwareOpen);
  if (aInitialConfig) {
    return StartPreviewImpl();
  }

  return NS_OK;
}

nsresult
nsGonkCameraControl::Initialize()
{
  if (mCameraHw.get()) {
    DOM_CAMERA_LOGI("Camera %d already connected (this=%p)\n", mCameraId, this);
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  mCameraHw = GonkCameraHardware::Connect(this, mCameraId);
  if (!mCameraHw.get()) {
    DOM_CAMERA_LOGE("Failed to connect to camera %d (this=%p)\n", mCameraId, this);
    return NS_ERROR_NOT_INITIALIZED;
  }

  DOM_CAMERA_LOGI("Initializing camera %d (this=%p, mCameraHw=%p)\n", mCameraId, this, mCameraHw.get());
  mCurrentConfiguration.mRecorderProfile.Truncate();

  
  PullParametersImpl();

  
  mParams.Set(CAMERA_PARAM_PREVIEWFORMAT, NS_LITERAL_STRING("yuv420sp"));
  
  mParams.Set(CAMERA_PARAM_SCENEMODE_HDR_RETURNNORMALPICTURE, false);
  PushParametersImpl();

  
  mParams.Get(CAMERA_PARAM_PICTURE_FILEFORMAT, mFileFormat);
  mParams.Get(CAMERA_PARAM_THUMBNAILSIZE, mLastThumbnailSize);

  
  int areas;
  mParams.Get(CAMERA_PARAM_SUPPORTED_MAXMETERINGAREAS, areas);
  mCurrentConfiguration.mMaxMeteringAreas = areas != -1 ? areas : 0;
  mParams.Get(CAMERA_PARAM_SUPPORTED_MAXFOCUSAREAS, areas);
  mCurrentConfiguration.mMaxFocusAreas = areas != -1 ? areas : 0;

  mParams.Get(CAMERA_PARAM_PICTURE_SIZE, mLastPictureSize);
  mParams.Get(CAMERA_PARAM_PREVIEWSIZE, mCurrentConfiguration.mPreviewSize);

  nsString luminance; 
  mParams.Get(CAMERA_PARAM_LUMINANCE, luminance);
  mLuminanceSupported = !luminance.IsEmpty();

  nsString flashMode;
  mParams.Get(CAMERA_PARAM_FLASHMODE, flashMode);
  mFlashSupported = !flashMode.IsEmpty();

  double quality; 
  mParams.Get(CAMERA_PARAM_PICTURE_QUALITY, quality);

  DOM_CAMERA_LOGI(" - maximum metering areas:        %u\n", mCurrentConfiguration.mMaxMeteringAreas);
  DOM_CAMERA_LOGI(" - maximum focus areas:           %u\n", mCurrentConfiguration.mMaxFocusAreas);
  DOM_CAMERA_LOGI(" - default picture size:          %u x %u\n",
    mLastPictureSize.width, mLastPictureSize.height);
  DOM_CAMERA_LOGI(" - default picture file format:   %s\n",
    NS_ConvertUTF16toUTF8(mFileFormat).get());
  DOM_CAMERA_LOGI(" - default picture quality:       %f\n", quality);
  DOM_CAMERA_LOGI(" - default thumbnail size:        %u x %u\n",
    mLastThumbnailSize.width, mLastThumbnailSize.height);
  DOM_CAMERA_LOGI(" - default preview size:          %u x %u\n",
    mCurrentConfiguration.mPreviewSize.width, mCurrentConfiguration.mPreviewSize.height);
  DOM_CAMERA_LOGI(" - luminance reporting:           %ssupported\n",
    mLuminanceSupported ? "" : "NOT ");
  if (mFlashSupported) {
    DOM_CAMERA_LOGI(" - flash:                         supported, default mode '%s'\n",
      NS_ConvertUTF16toUTF8(flashMode).get());
  } else {
    DOM_CAMERA_LOGI(" - flash:                         NOT supported\n");
  }

  nsAutoTArray<Size, 16> sizes;
  mParams.Get(CAMERA_PARAM_SUPPORTED_VIDEOSIZES, sizes);
  if (sizes.Length() > 0) {
    mSeparateVideoAndPreviewSizesSupported = true;
    DOM_CAMERA_LOGI(" - support for separate preview and video sizes\n");
    mParams.Get(CAMERA_PARAM_VIDEOSIZE, mLastRecorderSize);
    DOM_CAMERA_LOGI(" - default video recorder size:   %u x %u\n",
      mLastRecorderSize.width, mLastRecorderSize.height);
  } else {
    mLastRecorderSize = mCurrentConfiguration.mPreviewSize;
  }

  return NS_OK;
}

nsGonkCameraControl::~nsGonkCameraControl()
{
  DOM_CAMERA_LOGT("%s:%d : this=%p, mCameraHw = %p\n", __func__, __LINE__, this, mCameraHw.get());

  StopImpl();
  DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);
}

nsresult
nsGonkCameraControl::SetConfigurationInternal(const Configuration& aConfig)
{
  DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);

  nsresult rv;

  {
    ICameraControlParameterSetAutoEnter set(this);

    switch (aConfig.mMode) {
      case kPictureMode:
        rv = SetPictureConfiguration(aConfig);
        break;

      case kVideoMode:
        rv = SetVideoConfiguration(aConfig);
        break;

      default:
        MOZ_ASSERT_UNREACHABLE("Unanticipated camera mode in SetConfigurationInternal()");
        rv = NS_ERROR_FAILURE;
        break;
    }

    DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = Set(CAMERA_PARAM_RECORDINGHINT, aConfig.mMode == kVideoMode);
    if (NS_FAILED(rv)) {
      DOM_CAMERA_LOGE("Failed to set recording hint (0x%x)\n", rv);
    }
  }

  mCurrentConfiguration.mMode = aConfig.mMode;
  mCurrentConfiguration.mRecorderProfile = aConfig.mRecorderProfile;

  OnConfigurationChange();
  return NS_OK;
}

nsresult
nsGonkCameraControl::SetConfigurationImpl(const Configuration& aConfig)
{
  DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);
  MOZ_ASSERT(NS_GetCurrentThread() == mCameraThread);

  if (aConfig.mMode == kUnspecifiedMode) {
    DOM_CAMERA_LOGW("Can't set camera mode to 'unspecified', ignoring\n");
    return NS_ERROR_INVALID_ARG;
  }

  
  nsresult rv = PausePreview();
  if (NS_FAILED(rv)) {
    DOM_CAMERA_LOGW("PausePreview() in SetConfigurationImpl() failed (0x%x)\n", rv);
    if (rv == NS_ERROR_NOT_INITIALIZED) {
      
      
      return rv;
    }
  }

  DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);
  rv = SetConfigurationInternal(aConfig);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    StopPreviewImpl();
    return rv;
  }

  
  DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);
  return StartPreviewImpl();
}

nsresult
nsGonkCameraControl::SetPictureConfiguration(const Configuration& aConfig)
{
  DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);

  nsresult rv = SetPreviewSize(aConfig.mPreviewSize);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mParams.Get(CAMERA_PARAM_PREVIEWFRAMERATE, mPreviewFps);

  DOM_CAMERA_LOGI("picture mode preview: wanted %ux%u, got %ux%u (%u fps)\n",
    aConfig.mPreviewSize.width, aConfig.mPreviewSize.height,
    mCurrentConfiguration.mPreviewSize.width, mCurrentConfiguration.mPreviewSize.height,
    mPreviewFps);

  return NS_OK;
}


nsresult
nsGonkCameraControl::PushParameters()
{
  uint32_t dcu = mDeferConfigUpdate;
  if (dcu > 0) {
    DOM_CAMERA_LOGI("Defering config update (nest level %u)\n", dcu);
    return NS_OK;
  }

  





  if (NS_GetCurrentThread() != mCameraThread) {
    DOM_CAMERA_LOGT("%s:%d - dispatching to Camera Thread\n", __func__, __LINE__);
    nsCOMPtr<nsIRunnable> pushParametersTask =
      NS_NewRunnableMethod(this, &nsGonkCameraControl::PushParametersImpl);
    return mCameraThread->Dispatch(pushParametersTask, NS_DISPATCH_NORMAL);
  }

  DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);
  return PushParametersImpl();
}

void
nsGonkCameraControl::BeginBatchParameterSet()
{
  uint32_t dcu = ++mDeferConfigUpdate;
  if (dcu == 0) {
    NS_WARNING("Overflow weirdness incrementing mDeferConfigUpdate!");
    MOZ_CRASH();
  }
  DOM_CAMERA_LOGI("Begin deferring camera configuration updates (nest level %u)\n", dcu);
}

void
nsGonkCameraControl::EndBatchParameterSet()
{
  uint32_t dcu = mDeferConfigUpdate--;
  if (dcu == 0) {
    NS_WARNING("Underflow badness decrementing mDeferConfigUpdate!");
    MOZ_CRASH();
  }
  DOM_CAMERA_LOGI("End deferring camera configuration updates (nest level %u)\n", dcu);

  if (dcu == 1) {
    PushParameters();
  }
}

template<class T> nsresult
nsGonkCameraControl::SetAndPush(uint32_t aKey, const T& aValue)
{
  nsresult rv = mParams.Set(aKey, aValue);
  if (NS_FAILED(rv)) {
    DOM_CAMERA_LOGE("Camera parameter aKey=%d failed to set (0x%x)\n", aKey, rv);
    return rv;
  }
  return PushParameters();
}


nsresult
nsGonkCameraControl::Get(uint32_t aKey, nsTArray<Size>& aSizes)
{
  if (aKey == CAMERA_PARAM_SUPPORTED_VIDEOSIZES &&
      !mSeparateVideoAndPreviewSizesSupported) {
    aKey = CAMERA_PARAM_SUPPORTED_PREVIEWSIZES;
  }

  return mParams.Get(aKey, aSizes);
}


nsresult
nsGonkCameraControl::Get(uint32_t aKey, nsTArray<double>& aValues)
{
  return mParams.Get(aKey, aValues);
}


nsresult
nsGonkCameraControl::Get(uint32_t aKey, nsTArray<nsString>& aValues)
{
  return mParams.Get(aKey, aValues);
}


nsresult
nsGonkCameraControl::Set(uint32_t aKey, const nsAString& aValue)
{
  nsresult rv = mParams.Set(aKey, aValue);
  if (NS_FAILED(rv)) {
    return rv;
  }

  switch (aKey) {
    case CAMERA_PARAM_PICTURE_FILEFORMAT:
      
      mFileFormat = aValue;
      break;

    case CAMERA_PARAM_FLASHMODE:
      
      mAutoFlashModeOverridden = false;
      break;

    case CAMERA_PARAM_SCENEMODE:
      
      
      mParams.Set(CAMERA_PARAM_SCENEMODE_HDR_RETURNNORMALPICTURE, false);
      break;
  }

  return PushParameters();
}

nsresult
nsGonkCameraControl::Get(uint32_t aKey, nsAString& aRet)
{
  return mParams.Get(aKey, aRet);
}


nsresult
nsGonkCameraControl::Set(uint32_t aKey, double aValue)
{
  return SetAndPush(aKey, aValue);
}

nsresult
nsGonkCameraControl::Get(uint32_t aKey, double& aRet)
{
  return mParams.Get(aKey, aRet);
}


nsresult
nsGonkCameraControl::Set(uint32_t aKey, int64_t aValue)
{
  return SetAndPush(aKey, aValue);
}

nsresult
nsGonkCameraControl::Get(uint32_t aKey, int64_t& aRet)
{
  return mParams.Get(aKey, aRet);
}


nsresult
nsGonkCameraControl::Set(uint32_t aKey, bool aValue)
{
  return SetAndPush(aKey, aValue);
}

nsresult
nsGonkCameraControl::Get(uint32_t aKey, bool& aRet)
{
  return mParams.Get(aKey, aRet);
}


nsresult
nsGonkCameraControl::Set(uint32_t aKey, const nsTArray<Region>& aRegions)
{
  return SetAndPush(aKey, aRegions);
}

nsresult
nsGonkCameraControl::Get(uint32_t aKey, nsTArray<Region>& aRegions)
{
  return mParams.Get(aKey, aRegions);
}


nsresult
nsGonkCameraControl::Set(uint32_t aKey, const Size& aSize)
{
  switch (aKey) {
    case CAMERA_PARAM_PICTURE_SIZE:
      DOM_CAMERA_LOGI("setting picture size to %ux%u\n", aSize.width, aSize.height);
      return SetPictureSize(aSize);

    case CAMERA_PARAM_THUMBNAILSIZE:
      DOM_CAMERA_LOGI("setting thumbnail size to %ux%u\n", aSize.width, aSize.height);
      return SetThumbnailSize(aSize);

    default:
      return SetAndPush(aKey, aSize);
  }
}

nsresult
nsGonkCameraControl::Get(uint32_t aKey, Size& aSize)
{
  return mParams.Get(aKey, aSize);
}


nsresult
nsGonkCameraControl::Set(uint32_t aKey, int aValue)
{
  if (aKey == CAMERA_PARAM_PICTURE_ROTATION) {
    RETURN_IF_NO_CAMERA_HW();
    aValue = RationalizeRotation(aValue + mCameraHw->GetSensorOrientation());
  }
  return SetAndPush(aKey, aValue);
}

nsresult
nsGonkCameraControl::Get(uint32_t aKey, int& aRet)
{
  if (aKey == CAMERA_PARAM_SENSORANGLE) {
    RETURN_IF_NO_CAMERA_HW();
    aRet = mCameraHw->GetSensorOrientation();
    return NS_OK;
  }

  return mParams.Get(aKey, aRet);
}


nsresult
nsGonkCameraControl::SetLocation(const Position& aLocation)
{
  return SetAndPush(CAMERA_PARAM_PICTURE_LOCATION, aLocation);
}

nsresult
nsGonkCameraControl::StartPreviewImpl()
{
  MOZ_ASSERT(NS_GetCurrentThread() == mCameraThread);
  RETURN_IF_NO_CAMERA_HW();

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  if (mPreviewState == CameraControlListener::kPreviewStarted) {
    DOM_CAMERA_LOGW("Camera preview already started, nothing to do\n");
    return NS_OK;
  }

  DOM_CAMERA_LOGI("Starting preview (this=%p)\n", this);

  if (mCameraHw->StartPreview() != OK) {
    DOM_CAMERA_LOGE("Failed to start camera preview\n");
    return NS_ERROR_FAILURE;
  }

  OnPreviewStateChange(CameraControlListener::kPreviewStarted);
  return NS_OK;
}

nsresult
nsGonkCameraControl::StopPreviewImpl()
{
  RETURN_IF_NO_CAMERA_HW();

  DOM_CAMERA_LOGI("Stopping preview (this=%p)\n", this);

  mCameraHw->StopPreview();
  OnPreviewStateChange(CameraControlListener::kPreviewStopped);
  return NS_OK;
}

nsresult
nsGonkCameraControl::PausePreview()
{
  RETURN_IF_NO_CAMERA_HW();

  DOM_CAMERA_LOGI("Pausing preview (this=%p)\n", this);

  mCameraHw->StopPreview();
  OnPreviewStateChange(CameraControlListener::kPreviewPaused);
  return NS_OK;
}

nsresult
nsGonkCameraControl::AutoFocusImpl()
{
  MOZ_ASSERT(NS_GetCurrentThread() == mCameraThread);
  RETURN_IF_NO_CAMERA_HW();

  DOM_CAMERA_LOGI("Starting auto focus\n");

  if (mCameraHw->AutoFocus() != OK) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult
nsGonkCameraControl::StartFaceDetectionImpl()
{
  MOZ_ASSERT(NS_GetCurrentThread() == mCameraThread);
  RETURN_IF_NO_CAMERA_HW();

  DOM_CAMERA_LOGI("Starting face detection\n");

  if (mCameraHw->StartFaceDetection() != OK) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult
nsGonkCameraControl::StopFaceDetectionImpl()
{
  MOZ_ASSERT(NS_GetCurrentThread() == mCameraThread);
  RETURN_IF_NO_CAMERA_HW();

  DOM_CAMERA_LOGI("Stopping face detection\n");

  if (mCameraHw->StopFaceDetection() != OK) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult
nsGonkCameraControl::SetThumbnailSizeImpl(const Size& aSize)
{
  MOZ_ASSERT(NS_GetCurrentThread() == mCameraThread);

  




  mLastThumbnailSize = aSize;

  



  if (!aSize.width || !aSize.height) {
    DOM_CAMERA_LOGW("Requested thumbnail size %ux%u, disabling thumbnail\n",
      aSize.width, aSize.height);
    Size size = { 0, 0 };
    return SetAndPush(CAMERA_PARAM_THUMBNAILSIZE, size);
  }

  





  int smallestDelta = INT_MAX;
  uint32_t smallestDeltaIndex = UINT32_MAX;
  int targetArea = aSize.width * aSize.height;

  nsAutoTArray<Size, 8> supportedSizes;
  Get(CAMERA_PARAM_SUPPORTED_JPEG_THUMBNAIL_SIZES, supportedSizes);

  for (uint32_t i = 0; i < supportedSizes.Length(); ++i) {
    int area = supportedSizes[i].width * supportedSizes[i].height;
    int delta = abs(area - targetArea);

    if (area != 0
      && delta < smallestDelta
      && supportedSizes[i].width * mLastPictureSize.height /
         supportedSizes[i].height == mLastPictureSize.width
    ) {
      smallestDelta = delta;
      smallestDeltaIndex = i;
    }
  }

  if (smallestDeltaIndex == UINT32_MAX) {
    DOM_CAMERA_LOGW("Unable to find a thumbnail size close to %ux%u, disabling thumbnail\n",
      aSize.width, aSize.height);
    
    
    Size size = { 0, 0 };
    return SetAndPush(CAMERA_PARAM_THUMBNAILSIZE, size);
  }

  Size size = supportedSizes[smallestDeltaIndex];
  DOM_CAMERA_LOGI("camera-param set thumbnail-size = %ux%u (requested %ux%u)\n",
    size.width, size.height, aSize.width, aSize.height);
  if (size.width > INT32_MAX || size.height > INT32_MAX) {
    DOM_CAMERA_LOGE("Supported thumbnail size is too big, no change\n");
    return NS_ERROR_FAILURE;
  }

  return SetAndPush(CAMERA_PARAM_THUMBNAILSIZE, size);
}

nsresult
nsGonkCameraControl::SetThumbnailSize(const Size& aSize)
{
  class SetThumbnailSize : public nsRunnable
  {
  public:
    SetThumbnailSize(nsGonkCameraControl* aCameraControl, const Size& aSize)
      : mCameraControl(aCameraControl)
      , mSize(aSize)
    {
      MOZ_COUNT_CTOR(SetThumbnailSize);
    }
    ~SetThumbnailSize() { MOZ_COUNT_DTOR(SetThumbnailSize); }

    NS_IMETHODIMP
    Run() MOZ_OVERRIDE
    {
      nsresult rv = mCameraControl->SetThumbnailSizeImpl(mSize);
      if (NS_FAILED(rv)) {
        mCameraControl->OnUserError(CameraControlListener::kInSetThumbnailSize, rv);
      }
      return NS_OK;
    }

  protected:
    nsRefPtr<nsGonkCameraControl> mCameraControl;
    Size mSize;
  };

  if (NS_GetCurrentThread() == mCameraThread) {
    return SetThumbnailSizeImpl(aSize);
  }

  return mCameraThread->Dispatch(new SetThumbnailSize(this, aSize), NS_DISPATCH_NORMAL);
}

nsresult
nsGonkCameraControl::UpdateThumbnailSize()
{
  return SetThumbnailSize(mLastThumbnailSize);
}

nsresult
nsGonkCameraControl::SetPictureSizeImpl(const Size& aSize)
{
  MOZ_ASSERT(NS_GetCurrentThread() == mCameraThread);

  




  if (!aSize.width || !aSize.height) {
    DOM_CAMERA_LOGW("Ignoring requested picture size of %ux%u\n", aSize.width, aSize.height);
    return NS_ERROR_INVALID_ARG;
  }

  if (aSize.width == mLastPictureSize.width && aSize.height == mLastPictureSize.height) {
    DOM_CAMERA_LOGI("Requested picture size %ux%u unchanged\n", aSize.width, aSize.height);
    return NS_OK;
  }

  nsAutoTArray<Size, 8> supportedSizes;
  Get(CAMERA_PARAM_SUPPORTED_PICTURESIZES, supportedSizes);

  Size best;
  nsresult rv = GetSupportedSize(aSize, supportedSizes, best);
  if (NS_FAILED(rv)) {
    DOM_CAMERA_LOGW("Unable to find a picture size close to %ux%u\n",
      aSize.width, aSize.height);
    return NS_ERROR_INVALID_ARG;
  }

  DOM_CAMERA_LOGI("camera-param set picture-size = %ux%u (requested %ux%u)\n",
    best.width, best.height, aSize.width, aSize.height);
  if (best.width > INT32_MAX || best.height > INT32_MAX) {
    DOM_CAMERA_LOGE("Supported picture size is too big, no change\n");
    return NS_ERROR_FAILURE;
  }

  rv = mParams.Set(CAMERA_PARAM_PICTURE_SIZE, best);
  if (NS_FAILED(rv)) {
    return rv;
  }

  mLastPictureSize = best;

  
  
  
  return UpdateThumbnailSize();
}

int32_t
nsGonkCameraControl::RationalizeRotation(int32_t aRotation)
{
  int32_t r = aRotation;

  
  
  
  
  if (r >= 0) {
    r += 45;
  } else {
    r -= 45;
  }
  r /= 90;
  r %= 4;
  r *= 90;
  if (r < 0) {
    r += 360;
  }

  return r;
}

nsresult
nsGonkCameraControl::SetPictureSize(const Size& aSize)
{
  class SetPictureSize : public nsRunnable
  {
  public:
    SetPictureSize(nsGonkCameraControl* aCameraControl, const Size& aSize)
      : mCameraControl(aCameraControl)
      , mSize(aSize)
    {
      MOZ_COUNT_CTOR(SetPictureSize);
    }
    ~SetPictureSize() { MOZ_COUNT_DTOR(SetPictureSize); }

    NS_IMETHODIMP
    Run() MOZ_OVERRIDE
    {
      nsresult rv = mCameraControl->SetPictureSizeImpl(mSize);
      if (NS_FAILED(rv)) {
        mCameraControl->OnUserError(CameraControlListener::kInSetPictureSize, rv);
      }
      return NS_OK;
    }

  protected:
    nsRefPtr<nsGonkCameraControl> mCameraControl;
    Size mSize;
  };

  if (NS_GetCurrentThread() == mCameraThread) {
    return SetPictureSizeImpl(aSize);
  }

  return mCameraThread->Dispatch(new SetPictureSize(this, aSize), NS_DISPATCH_NORMAL);
}

nsresult
nsGonkCameraControl::TakePictureImpl()
{
  MOZ_ASSERT(NS_GetCurrentThread() == mCameraThread);
  RETURN_IF_NO_CAMERA_HW();

  if (mCameraHw->TakePicture() != OK) {
    return NS_ERROR_FAILURE;
  }

  
  
  OnPreviewStateChange(CameraControlListener::kPreviewPaused);
  return NS_OK;
}

nsresult
nsGonkCameraControl::PushParametersImpl()
{
  MOZ_ASSERT(NS_GetCurrentThread() == mCameraThread);
  DOM_CAMERA_LOGI("Pushing camera parameters\n");
  RETURN_IF_NO_CAMERA_HW();

  if (mCameraHw->PushParameters(mParams) != OK) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
nsGonkCameraControl::PullParametersImpl()
{
  MOZ_ASSERT(NS_GetCurrentThread() == mCameraThread);
  DOM_CAMERA_LOGI("Pulling camera parameters\n");
  RETURN_IF_NO_CAMERA_HW();

  return mCameraHw->PullParameters(mParams);
}

nsresult
nsGonkCameraControl::SetupRecordingFlash(bool aAutoEnableLowLightTorch)
{
  mAutoFlashModeOverridden = false;

  if (!aAutoEnableLowLightTorch || !mLuminanceSupported || !mFlashSupported) {
    return NS_OK;
  }

  DOM_CAMERA_LOGI("Luminance reporting and flash supported\n");

  nsresult rv = PullParametersImpl();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsString luminance;
  rv = mParams.Get(CAMERA_PARAM_LUMINANCE, luminance);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    
    return NS_OK;
  }

  nsString flashMode;
  rv = mParams.Get(CAMERA_PARAM_FLASHMODE, flashMode);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    
    return NS_OK;
  }

  if (luminance.EqualsASCII("low") && flashMode.EqualsASCII("auto")) {
    DOM_CAMERA_LOGI("Low luminance detected, turning on flash\n");
    rv = SetAndPush(CAMERA_PARAM_FLASHMODE, NS_LITERAL_STRING("torch"));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      
      return NS_OK;
    }

    mAutoFlashModeOverridden = true;
  }

  return NS_OK;
}

nsresult
nsGonkCameraControl::StartRecordingImpl(DeviceStorageFileDescriptor* aFileDescriptor,
                                        const StartRecordingOptions* aOptions)
{
  MOZ_ASSERT(NS_GetCurrentThread() == mCameraThread);

  ReentrantMonitorAutoEnter mon(mRecorderMonitor);

  NS_ENSURE_TRUE(!mCurrentConfiguration.mRecorderProfile.IsEmpty(), NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mRecorder, NS_ERROR_FAILURE);

  







  if (NS_WARN_IF(!aFileDescriptor)) {
    return NS_ERROR_INVALID_ARG;
  }
  nsAutoString fullPath;
  mVideoFile = aFileDescriptor->mDSFile;
  mVideoFile->GetFullPath(fullPath);
  DOM_CAMERA_LOGI("Video filename is '%s'\n",
                  NS_LossyConvertUTF16toASCII(fullPath).get());

  if (!mVideoFile->IsSafePath()) {
    DOM_CAMERA_LOGE("Invalid video file name\n");
    return NS_ERROR_INVALID_ARG;
  }

  
  
  
  
  nsRefPtr<CloseFileRunnable> closer;
  if (aFileDescriptor->mFileDescriptor.IsValid()) {
    closer = new CloseFileRunnable(aFileDescriptor->mFileDescriptor);
  }
  nsresult rv;
  int fd = aFileDescriptor->mFileDescriptor.PlatformHandle();
  if (aOptions) {
    rv = SetupRecording(fd, aOptions->rotation, aOptions->maxFileSizeBytes,
                        aOptions->maxVideoLengthMs);
    if (NS_SUCCEEDED(rv)) {
      rv = SetupRecordingFlash(aOptions->autoEnableLowLightTorch);
    }
  } else {
    rv = SetupRecording(fd, 0, 0, 0);
  }
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (mRecorder->start() != OK) {
    DOM_CAMERA_LOGE("mRecorder->start() failed\n");
    
    mRecorder = nullptr;
    
    if (mAutoFlashModeOverridden) {
      SetAndPush(CAMERA_PARAM_FLASHMODE, NS_LITERAL_STRING("auto"));
    }
    return NS_ERROR_FAILURE;
  }

  OnRecorderStateChange(CameraControlListener::kRecorderStarted);
  return NS_OK;
}

nsresult
nsGonkCameraControl::StopRecordingImpl()
{
  class RecordingComplete : public nsRunnable
  {
  public:
    RecordingComplete(DeviceStorageFile* aFile)
      : mFile(aFile)
    { }

    ~RecordingComplete() { }

    NS_IMETHODIMP
    Run()
    {
      MOZ_ASSERT(NS_IsMainThread());

      nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
      obs->NotifyObservers(mFile, "file-watcher-notify", NS_LITERAL_STRING("modified").get());
      return NS_OK;
    }

  private:
    nsRefPtr<DeviceStorageFile> mFile;
  };

  ReentrantMonitorAutoEnter mon(mRecorderMonitor);

  
  NS_ENSURE_TRUE(mRecorder, NS_OK);

  mRecorder->stop();
  mRecorder = nullptr;
  OnRecorderStateChange(CameraControlListener::kRecorderStopped);

  {
    ICameraControlParameterSetAutoEnter set(this);

    if (mAutoFlashModeOverridden) {
      nsresult rv = Set(CAMERA_PARAM_FLASHMODE, NS_LITERAL_STRING("auto"));
      if (NS_FAILED(rv)) {
        DOM_CAMERA_LOGE("Failed to set flash mode (0x%x)\n", rv);
      }
    }
  }

  
  return NS_DispatchToMainThread(new RecordingComplete(mVideoFile));
}

nsresult
nsGonkCameraControl::ResumeContinuousFocusImpl()
{
  MOZ_ASSERT(NS_GetCurrentThread() == mCameraThread);
  RETURN_IF_NO_CAMERA_HW();

  DOM_CAMERA_LOGI("Resuming continuous autofocus\n");

  
  
  if (NS_WARN_IF(mCameraHw->CancelAutoFocus() != OK)) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

void
nsGonkCameraControl::OnAutoFocusComplete(bool aSuccess)
{
  class AutoFocusComplete : public nsRunnable
  {
  public:
    AutoFocusComplete(nsGonkCameraControl* aCameraControl, bool aSuccess)
      : mCameraControl(aCameraControl)
      , mSuccess(aSuccess)
    { }

    NS_IMETHODIMP
    Run() MOZ_OVERRIDE
    {
      mCameraControl->OnAutoFocusComplete(mSuccess);
      return NS_OK;
    }

  protected:
    nsRefPtr<nsGonkCameraControl> mCameraControl;
    bool mSuccess;
  };

  if (NS_GetCurrentThread() == mCameraThread) {
    



    PullParametersImpl();
    CameraControlImpl::OnAutoFocusComplete(aSuccess);
    return;
  }

  



  mCameraThread->Dispatch(new AutoFocusComplete(this, aSuccess), NS_DISPATCH_NORMAL);
}

bool
FeatureDetected(int32_t feature[])
{
  








  const int32_t kLowerFeatureBound = -1000;
  const int32_t kUpperFeatureBound = 1000;
  return (feature[0] >= kLowerFeatureBound && feature[0] <= kUpperFeatureBound) ||
         (feature[1] >= kLowerFeatureBound && feature[1] <= kUpperFeatureBound);
}

void
nsGonkCameraControl::OnFacesDetected(camera_frame_metadata_t* aMetaData)
{
  NS_ENSURE_TRUE_VOID(aMetaData);

  nsTArray<Face> faces;
  uint32_t numFaces = aMetaData->number_of_faces;
  DOM_CAMERA_LOGI("Camera detected %d face(s)", numFaces);

  faces.SetCapacity(numFaces);

  for (uint32_t i = 0; i < numFaces; ++i) {
    Face* f = faces.AppendElement();

    f->id = aMetaData->faces[i].id;
    f->score = aMetaData->faces[i].score;
    if (f->score > 100) {
      f->score = 100;
    }
    f->bound.left = aMetaData->faces[i].rect[0];
    f->bound.top = aMetaData->faces[i].rect[1];
    f->bound.right = aMetaData->faces[i].rect[2];
    f->bound.bottom = aMetaData->faces[i].rect[3];
    DOM_CAMERA_LOGI("Camera face[%u] appended: id=%d, score=%d, bound=(%d, %d)-(%d, %d)\n",
      i, f->id, f->score, f->bound.left, f->bound.top, f->bound.right, f->bound.bottom);

    f->hasLeftEye = FeatureDetected(aMetaData->faces[i].left_eye);
    if (f->hasLeftEye) {
      f->leftEye.x = aMetaData->faces[i].left_eye[0];
      f->leftEye.y = aMetaData->faces[i].left_eye[1];
      DOM_CAMERA_LOGI("    Left eye detected at (%d, %d)\n",
        f->leftEye.x, f->leftEye.y);
    } else {
      DOM_CAMERA_LOGI("    No left eye detected\n");
    }

    f->hasRightEye = FeatureDetected(aMetaData->faces[i].right_eye);
    if (f->hasRightEye) {
      f->rightEye.x = aMetaData->faces[i].right_eye[0];
      f->rightEye.y = aMetaData->faces[i].right_eye[1];
      DOM_CAMERA_LOGI("    Right eye detected at (%d, %d)\n",
        f->rightEye.x, f->rightEye.y);
    } else {
      DOM_CAMERA_LOGI("    No right eye detected\n");
    }

    f->hasMouth = FeatureDetected(aMetaData->faces[i].mouth);
    if (f->hasMouth) {
      f->mouth.x = aMetaData->faces[i].mouth[0];
      f->mouth.y = aMetaData->faces[i].mouth[1];
      DOM_CAMERA_LOGI("    Mouth detected at (%d, %d)\n", f->mouth.x, f->mouth.y);
    } else {
      DOM_CAMERA_LOGI("    No mouth detected\n");
    }
  }

  CameraControlImpl::OnFacesDetected(faces);
}

void
nsGonkCameraControl::OnTakePictureComplete(uint8_t* aData, uint32_t aLength)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  uint8_t* data = new uint8_t[aLength];

  memcpy(data, aData, aLength);

  nsString s(NS_LITERAL_STRING("image/"));
  s.Append(mFileFormat);
  DOM_CAMERA_LOGI("Got picture, type '%s', %u bytes\n", NS_ConvertUTF16toUTF8(s).get(), aLength);
  OnTakePictureComplete(data, aLength, s);

  if (mResumePreviewAfterTakingPicture) {
    nsresult rv = StartPreview();
    if (NS_FAILED(rv)) {
      DOM_CAMERA_LOGE("Failed to restart camera preview (%x)\n", rv);
      OnPreviewStateChange(CameraControlListener::kPreviewStopped);
    }
  }

  DOM_CAMERA_LOGI("nsGonkCameraControl::OnTakePictureComplete() done\n");
}

void
nsGonkCameraControl::OnTakePictureError()
{
  CameraControlImpl::OnUserError(CameraControlListener::kInTakePicture,
                                 NS_ERROR_FAILURE);
}

nsresult
nsGonkCameraControl::SetPreviewSize(const Size& aSize)
{
  MOZ_ASSERT(NS_GetCurrentThread() == mCameraThread);

  nsTArray<Size> previewSizes;
  nsresult rv = Get(CAMERA_PARAM_SUPPORTED_PREVIEWSIZES, previewSizes);
  if (NS_FAILED(rv)) {
    DOM_CAMERA_LOGE("Camera failed to return any preview sizes (0x%x)\n", rv);
    return rv;
  }

  Size best;
  rv = GetSupportedSize(aSize, previewSizes, best);
  if (NS_FAILED(rv)) {
    DOM_CAMERA_LOGE("Failed to find a supported preview size, requested size %dx%d",
        aSize.width, aSize.height);
    return rv;
  }

  if (mSeparateVideoAndPreviewSizesSupported) {
    
    
    
    if (best.width > mLastRecorderSize.width || best.height > mLastRecorderSize.height) {
      SetVideoSize(best);
    }
  } else {
    mLastRecorderSize = best;
  }
  mCurrentConfiguration.mPreviewSize = best;
  return Set(CAMERA_PARAM_PREVIEWSIZE, best);
}

nsresult
nsGonkCameraControl::SetVideoSize(const Size& aSize)
{
  MOZ_ASSERT(NS_GetCurrentThread() == mCameraThread);

  if (!mSeparateVideoAndPreviewSizesSupported) {
    DOM_CAMERA_LOGE("Camera does not support setting separate video size\n");
    return NS_ERROR_NOT_AVAILABLE;
  }
  
  nsTArray<Size> videoSizes;
  nsresult rv = Get(CAMERA_PARAM_SUPPORTED_VIDEOSIZES, videoSizes);
  if (NS_FAILED(rv)) {
    DOM_CAMERA_LOGE("Camera failed to return any video sizes (0x%x)\n", rv);
    return rv;
  }

  Size best;
  rv = GetSupportedSize(aSize, videoSizes, best);
  if (NS_FAILED(rv)) {
    DOM_CAMERA_LOGE("Failed to find a supported video size, requested size %dx%d",
        aSize.width, aSize.height);
    return rv;
  }
  mLastRecorderSize = best;
  return Set(CAMERA_PARAM_VIDEOSIZE, best);
}

nsresult
nsGonkCameraControl::GetSupportedSize(const Size& aSize,
                                      const nsTArray<Size>& supportedSizes,
                                      Size& best)
{
  nsresult rv = NS_ERROR_INVALID_ARG;
  best = aSize;
  uint32_t minSizeDelta = UINT32_MAX;
  uint32_t delta;

  if (!aSize.width && !aSize.height) {
    
    best = supportedSizes[0];
    return NS_OK;
  } else if (aSize.width && aSize.height) {
    
    
    for (nsTArray<Size>::index_type i = 0; i < supportedSizes.Length(); i++) {
      Size size = supportedSizes[i];
      if (size.width == aSize.width && size.height == aSize.height) {
        best = size;
        return NS_OK;
      }
    }

    
    uint32_t targetArea = aSize.width * aSize.height;
    for (nsTArray<Size>::index_type i = 0; i < supportedSizes.Length(); i++) {
      Size size = supportedSizes[i];
      uint32_t delta = abs((long int)(size.width * size.height - targetArea));
      if (delta < minSizeDelta) {
        minSizeDelta = delta;
        best = size;
        rv = NS_OK;
      }
    }
  } else if (!aSize.width) {
    
    for (nsTArray<Size>::index_type i = 0; i < supportedSizes.Length(); i++) {
      Size size = supportedSizes[i];
      delta = abs((long int)(size.height - aSize.height));
      if (delta < minSizeDelta) {
        minSizeDelta = delta;
        best = size;
        rv = NS_OK;
      }
    }
  } else if (!aSize.height) {
    
    for (nsTArray<Size>::index_type i = 0; i < supportedSizes.Length(); i++) {
      Size size = supportedSizes[i];
      delta = abs((long int)(size.width - aSize.width));
      if (delta < minSizeDelta) {
        minSizeDelta = delta;
        best = size;
        rv = NS_OK;
      }
    }
  }
  return rv;
}

nsresult
nsGonkCameraControl::SetVideoConfiguration(const Configuration& aConfig)
{
  DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);

  RecorderProfile* profile;
  if (!mRecorderProfiles.Get(aConfig.mRecorderProfile, &profile)) {
    DOM_CAMERA_LOGE("Recorder profile '%s' is not supported\n",
      NS_ConvertUTF16toUTF8(aConfig.mRecorderProfile).get());
    return NS_ERROR_INVALID_ARG;
  }

  mCurrentConfiguration.mRecorderProfile = aConfig.mRecorderProfile;
  const RecorderProfile::Video& video(profile->GetVideo());
  const Size& size = video.GetSize();
  int fps = video.GetFramesPerSecond();
  if (fps <= 0 || size.width <= 0 || size.height <= 0) {
    DOM_CAMERA_LOGE("Can't configure video with fps=%d, width=%d, height=%d\n",
      fps, size.width, size.height);
    return NS_ERROR_FAILURE;
  }

  PullParametersImpl();

  {
    ICameraControlParameterSetAutoEnter set(this);
    nsresult rv;

    if (mSeparateVideoAndPreviewSizesSupported) {
      
      
      rv = SetVideoSize(size);
      if (NS_FAILED(rv)) {
        DOM_CAMERA_LOGE("Failed to set video mode video size (0x%x)\n", rv);
        return rv;
      }

      
      
      rv = SetPreviewSize(aConfig.mPreviewSize);
      if (NS_FAILED(rv)) {
        DOM_CAMERA_LOGE("Failed to set video mode preview size (0x%x)\n", rv);
        return rv;
      }
    } else {
      
      
      
      rv = SetPreviewSize(size);
      if (NS_FAILED(rv)) {
        DOM_CAMERA_LOGE("Failed to set video mode preview size (0x%x)\n", rv);
        return rv;
      }
    }

    rv = Set(CAMERA_PARAM_PREVIEWFRAMERATE, fps);
    if (NS_FAILED(rv)) {
      DOM_CAMERA_LOGE("Failed to set video mode frame rate (0x%x)\n", rv);
      return rv;
    }
  }

  mPreviewFps = fps;
  return NS_OK;
}

class GonkRecorderListener : public IMediaRecorderClient
{
public:
  GonkRecorderListener(nsGonkCameraControl* aCameraControl)
    : mCameraControl(aCameraControl)
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p, aCameraControl=%p\n",
      __func__, __LINE__, this, mCameraControl.get());
  }

  void notify(int msg, int ext1, int ext2)
  {
    if (mCameraControl) {
      mCameraControl->OnRecorderEvent(msg, ext1, ext2);
    }
  }

  IBinder* onAsBinder()
  {
    DOM_CAMERA_LOGE("onAsBinder() called, should NEVER get called!\n");
    return nullptr;
  }

protected:
  ~GonkRecorderListener() { }
  nsRefPtr<nsGonkCameraControl> mCameraControl;
};

void
nsGonkCameraControl::OnRecorderEvent(int msg, int ext1, int ext2)
{
  













































  int trackNum = CameraControlListener::kNoTrackNumber;

  switch (msg) {
    
    case MEDIA_RECORDER_EVENT_INFO:
      switch (ext1) {
        case MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED:
          DOM_CAMERA_LOGI("recorder-event : info: maximum file size reached\n");
          OnRecorderStateChange(CameraControlListener::kFileSizeLimitReached, ext2, trackNum);
          return;

        case MEDIA_RECORDER_INFO_MAX_DURATION_REACHED:
          DOM_CAMERA_LOGI("recorder-event : info: maximum video duration reached\n");
          OnRecorderStateChange(CameraControlListener::kVideoLengthLimitReached, ext2, trackNum);
          return;

        case MEDIA_RECORDER_TRACK_INFO_COMPLETION_STATUS:
          DOM_CAMERA_LOGI("recorder-event : info: track completed\n");
          OnRecorderStateChange(CameraControlListener::kTrackCompleted, ext2, trackNum);
          return;
      }
      break;

    case MEDIA_RECORDER_EVENT_ERROR:
      switch (ext1) {
        case MEDIA_RECORDER_ERROR_UNKNOWN:
          DOM_CAMERA_LOGE("recorder-event : recorder-error: %d (0x%08x)\n", ext2, ext2);
          OnRecorderStateChange(CameraControlListener::kMediaRecorderFailed, ext2, trackNum);
          return;

        case MEDIA_ERROR_SERVER_DIED:
          DOM_CAMERA_LOGE("recorder-event : recorder-error: server died\n");
          OnRecorderStateChange(CameraControlListener::kMediaServerFailed, ext2, trackNum);
          return;
      }
      break;

    
    case MEDIA_RECORDER_TRACK_EVENT_INFO:
      trackNum = (ext1 & 0xF0000000) >> 28;
      ext1 &= 0xFFFF;
      switch (ext1) {
        case MEDIA_RECORDER_TRACK_INFO_COMPLETION_STATUS:
          if (ext2 == OK) {
            DOM_CAMERA_LOGI("recorder-event : track-complete: track %d, %d (0x%08x)\n", trackNum, ext2, ext2);
            OnRecorderStateChange(CameraControlListener::kTrackCompleted, ext2, trackNum);
            return;
          }
          DOM_CAMERA_LOGE("recorder-event : track-error: track %d, %d (0x%08x)\n", trackNum, ext2, ext2);
          OnRecorderStateChange(CameraControlListener::kTrackFailed, ext2, trackNum);
          return;

        case MEDIA_RECORDER_TRACK_INFO_PROGRESS_IN_TIME:
          DOM_CAMERA_LOGI("recorder-event : track-info: progress in time: %d ms\n", ext2);
          return;
      }
      break;

    case MEDIA_RECORDER_TRACK_EVENT_ERROR:
      trackNum = (ext1 & 0xF0000000) >> 28;
      ext1 &= 0xFFFF;
      DOM_CAMERA_LOGE("recorder-event : track-error: track %d, %d (0x%08x)\n", trackNum, ext2, ext2);
      OnRecorderStateChange(CameraControlListener::kTrackFailed, ext2, trackNum);
      return;
  }

  
  DOM_CAMERA_LOGW("recorder-event : unhandled: msg=%d, ext1=%d, ext2=%d\n", msg, ext1, ext2);
}

nsresult
nsGonkCameraControl::SetupRecording(int aFd, int aRotation,
                                    uint64_t aMaxFileSizeBytes,
                                    uint64_t aMaxVideoLengthMs)
{
  RETURN_IF_NO_CAMERA_HW();

  
  const size_t SIZE = 256;
  char buffer[SIZE];

  ReentrantMonitorAutoEnter mon(mRecorderMonitor);

  mRecorder = new GonkRecorder();
  CHECK_SETARG_RETURN(mRecorder->init(), NS_ERROR_FAILURE);

  nsresult rv =
    GonkRecorderProfile::ConfigureRecorder(*mRecorder, mCameraId,
                                           mCurrentConfiguration.mRecorderProfile);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  CHECK_SETARG_RETURN(mRecorder->setCamera(mCameraHw), NS_ERROR_FAILURE);

  DOM_CAMERA_LOGI("maxVideoLengthMs=%llu\n", aMaxVideoLengthMs);
  const uint64_t kMaxVideoLengthMs = INT64_MAX / 1000;
  if (aMaxVideoLengthMs == 0) {
    aMaxVideoLengthMs = -1;
  } else if (aMaxVideoLengthMs > kMaxVideoLengthMs) {
    
    
    
    DOM_CAMERA_LOGW("maxVideoLengthMs capped to %lld\n", kMaxVideoLengthMs);
    aMaxVideoLengthMs = kMaxVideoLengthMs;
  }
  snprintf(buffer, SIZE, "max-duration=%lld", aMaxVideoLengthMs);
  CHECK_SETARG_RETURN(mRecorder->setParameters(String8(buffer)),
                      NS_ERROR_INVALID_ARG);

  DOM_CAMERA_LOGI("maxFileSizeBytes=%llu\n", aMaxFileSizeBytes);
  if (aMaxFileSizeBytes == 0) {
    aMaxFileSizeBytes = -1;
  } else if (aMaxFileSizeBytes > INT64_MAX) {
    
    DOM_CAMERA_LOGW("maxFileSizeBytes capped to INT64_MAX\n");
    aMaxFileSizeBytes = INT64_MAX;
  }
  snprintf(buffer, SIZE, "max-filesize=%lld", aMaxFileSizeBytes);
  CHECK_SETARG_RETURN(mRecorder->setParameters(String8(buffer)),
                      NS_ERROR_INVALID_ARG);

  
  int r = aRotation;
  r += mCameraHw->GetSensorOrientation();
  r = RationalizeRotation(r);
  DOM_CAMERA_LOGI("setting video rotation to %d degrees (mapped from %d)\n", r, aRotation);
  snprintf(buffer, SIZE, "video-param-rotation-angle-degrees=%d", r);
  CHECK_SETARG_RETURN(mRecorder->setParameters(String8(buffer)),
                      NS_ERROR_INVALID_ARG);

  CHECK_SETARG_RETURN(mRecorder->setListener(new GonkRecorderListener(this)),
                      NS_ERROR_FAILURE);

  
  CHECK_SETARG_RETURN(mRecorder->setOutputFile(aFd, 0, 0), NS_ERROR_FAILURE);
  CHECK_SETARG_RETURN(mRecorder->prepare(), NS_ERROR_FAILURE);

  return NS_OK;
}

nsresult
nsGonkCameraControl::StopImpl()
{
  DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);

  
  StopRecordingImpl();

  
  StopPreviewImpl();

  
  if (mCameraHw.get()){
     mCameraHw->Close();
     mCameraHw.clear();
  }

  OnHardwareStateChange(CameraControlListener::kHardwareClosed);
  return NS_OK;
}

nsresult
nsGonkCameraControl::LoadRecorderProfiles()
{
  if (mRecorderProfiles.Count() == 0) {
    nsTArray<nsRefPtr<RecorderProfile>> profiles;
    nsresult rv = GonkRecorderProfile::GetAll(mCameraId, profiles);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    nsTArray<Size> sizes;
    rv = Get(CAMERA_PARAM_SUPPORTED_VIDEOSIZES, sizes);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    
    for (nsTArray<RecorderProfile>::size_type i = 0; i < profiles.Length(); ++i) {
      int width = profiles[i]->GetVideo().GetSize().width; 
      int height = profiles[i]->GetVideo().GetSize().height;
      if (width < 0 || height < 0) {
        DOM_CAMERA_LOGW("Ignoring weird profile '%s' with width and/or height < 0\n",
          NS_ConvertUTF16toUTF8(profiles[i]->GetName()).get());
        continue;
      }
      for (nsTArray<Size>::size_type n = 0; n < sizes.Length(); ++n) {
        if (static_cast<uint32_t>(width) == sizes[n].width &&
            static_cast<uint32_t>(height) == sizes[n].height) {
          mRecorderProfiles.Put(profiles[i]->GetName(), profiles[i]);
          break;
        }
      }
    }
  }

  return NS_OK;
}

 PLDHashOperator
nsGonkCameraControl::Enumerate(const nsAString& aProfileName,
                               RecorderProfile* aProfile,
                               void* aUserArg)
{
  nsTArray<nsString>* profiles = static_cast<nsTArray<nsString>*>(aUserArg);
  MOZ_ASSERT(profiles);
  profiles->AppendElement(aProfileName);
  return PL_DHASH_NEXT;
}

nsresult
nsGonkCameraControl::GetRecorderProfiles(nsTArray<nsString>& aProfiles)
{
  nsresult rv = LoadRecorderProfiles();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  aProfiles.Clear();
  mRecorderProfiles.EnumerateRead(Enumerate, static_cast<void*>(&aProfiles));
  return NS_OK;
}

ICameraControl::RecorderProfile*
nsGonkCameraControl::GetProfileInfo(const nsAString& aProfile)
{
  RecorderProfile* profile;
  if (!mRecorderProfiles.Get(aProfile, &profile)) {
    return nullptr;
  }
  return profile;
}

void
nsGonkCameraControl::OnRateLimitPreview(bool aLimit)
{
  CameraControlImpl::OnRateLimitPreview(aLimit);
}

void
nsGonkCameraControl::OnNewPreviewFrame(layers::TextureClient* aBuffer)
{
  nsRefPtr<Image> frame = mImageContainer->CreateImage(ImageFormat::GRALLOC_PLANAR_YCBCR);

  GrallocImage* videoImage = static_cast<GrallocImage*>(frame.get());

  GrallocImage::GrallocData data;
  data.mGraphicBuffer = aBuffer;
  data.mPicSize = IntSize(mCurrentConfiguration.mPreviewSize.width,
                          mCurrentConfiguration.mPreviewSize.height);
  videoImage->SetData(data);

  OnNewPreviewFrame(frame, mCurrentConfiguration.mPreviewSize.width,
                    mCurrentConfiguration.mPreviewSize.height);
}

void
nsGonkCameraControl::OnSystemError(CameraControlListener::SystemContext aWhere,
                                   nsresult aError)
{
  if (aWhere == CameraControlListener::kSystemService) {
    OnPreviewStateChange(CameraControlListener::kPreviewStopped);
    OnHardwareStateChange(CameraControlListener::kHardwareClosed);
  }

  CameraControlImpl::OnSystemError(aWhere, aError);
}


namespace mozilla {

void
OnTakePictureComplete(nsGonkCameraControl* gc, uint8_t* aData, uint32_t aLength)
{
  gc->OnTakePictureComplete(aData, aLength);
}

void
OnTakePictureError(nsGonkCameraControl* gc)
{
  gc->OnTakePictureError();
}

void
OnAutoFocusComplete(nsGonkCameraControl* gc, bool aSuccess)
{
  gc->OnAutoFocusComplete(aSuccess);
}

void
OnAutoFocusMoving(nsGonkCameraControl* gc, bool aIsMoving)
{
  gc->OnAutoFocusMoving(aIsMoving);
}

void
OnFacesDetected(nsGonkCameraControl* gc, camera_frame_metadata_t* aMetaData)
{
  gc->OnFacesDetected(aMetaData);
}

void
OnRateLimitPreview(nsGonkCameraControl* gc, bool aLimit)
{
  gc->OnRateLimitPreview(aLimit);
}

void
OnNewPreviewFrame(nsGonkCameraControl* gc, layers::TextureClient* aBuffer)
{
  gc->OnNewPreviewFrame(aBuffer);
}

void
OnShutter(nsGonkCameraControl* gc)
{
  gc->OnShutter();
}

void
OnClosed(nsGonkCameraControl* gc)
{
  gc->OnClosed();
}

void
OnSystemError(nsGonkCameraControl* gc,
              CameraControlListener::SystemContext aWhere,
              int32_t aArg1, int32_t aArg2)
{
#ifdef PR_LOGGING
  DOM_CAMERA_LOGE("OnSystemError : aWhere=%d, aArg1=%d, aArg2=%d\n", aWhere, aArg1, aArg2);
#else
  unused << aArg1;
  unused << aArg2;
#endif
  gc->OnSystemError(aWhere, NS_ERROR_FAILURE);
}

} 
