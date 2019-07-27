


#include "MediaEngineGonkVideoSource.h"

#define LOG_TAG "MediaEngineGonkVideoSource"

#include <utils/Log.h>

#include "GrallocImages.h"
#include "VideoUtils.h"
#include "ScreenOrientation.h"

#include "libyuv.h"
#include "mtransport/runnable_utils.h"

namespace mozilla {

using namespace mozilla::dom;
using namespace mozilla::gfx;

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetMediaManagerLog();
#define LOG(msg) PR_LOG(GetMediaManagerLog(), PR_LOG_DEBUG, msg)
#define LOGFRAME(msg) PR_LOG(GetMediaManagerLog(), 6, msg)
#else
#define LOG(msg)
#define LOGFRAME(msg)
#endif



NS_IMPL_QUERY_INTERFACE(MediaEngineGonkVideoSource, nsISupports)
NS_IMPL_ADDREF_INHERITED(MediaEngineGonkVideoSource, CameraControlListener)
NS_IMPL_RELEASE_INHERITED(MediaEngineGonkVideoSource, CameraControlListener)




void
MediaEngineGonkVideoSource::NotifyPull(MediaStreamGraph* aGraph,
                                       SourceMediaStream* aSource,
                                       TrackID aID,
                                       StreamTime aDesiredTime,
                                       TrackTicks& aLastEndTime)
{
  VideoSegment segment;

  MonitorAutoLock lock(mMonitor);
  
  
  

  
  nsRefPtr<layers::Image> image = mImage;
  TrackTicks delta = aDesiredTime - aLastEndTime;
  LOGFRAME(("NotifyPull, desired = %ld, delta = %ld %s", (int64_t) aDesiredTime,
            (int64_t) delta, image ? "" : "<null>"));

  
  
  
  
  
  
  

  
  
  if (delta > 0) {
    
    IntSize size(image ? mWidth : 0, image ? mHeight : 0);
    segment.AppendFrame(image.forget(), delta, size);
    
    
    if (aSource->AppendToTrack(aID, &(segment))) {
      aLastEndTime = aDesiredTime;
    }
  }
}

void
MediaEngineGonkVideoSource::ChooseCapability(const VideoTrackConstraintsN& aConstraints,
                                             const MediaEnginePrefs& aPrefs)
{
  return GuessCapability(aConstraints, aPrefs);
}

nsresult
MediaEngineGonkVideoSource::Allocate(const VideoTrackConstraintsN& aConstraints,
                                     const MediaEnginePrefs& aPrefs)
{
  LOG((__FUNCTION__));

  ReentrantMonitorAutoEnter sync(mCallbackMonitor);
  if (mState == kReleased && mInitDone) {
    ChooseCapability(aConstraints, aPrefs);
    NS_DispatchToMainThread(WrapRunnable(nsRefPtr<MediaEngineGonkVideoSource>(this),
                                         &MediaEngineGonkVideoSource::AllocImpl));
    mCallbackMonitor.Wait();
    if (mState != kAllocated) {
      return NS_ERROR_FAILURE;
    }
  }

  return NS_OK;
}

nsresult
MediaEngineGonkVideoSource::Deallocate()
{
  LOG((__FUNCTION__));
  if (mSources.IsEmpty()) {

    ReentrantMonitorAutoEnter sync(mCallbackMonitor);

    if (mState != kStopped && mState != kAllocated) {
      return NS_ERROR_FAILURE;
    }

    

    NS_DispatchToMainThread(WrapRunnable(nsRefPtr<MediaEngineGonkVideoSource>(this),
                                         &MediaEngineGonkVideoSource::DeallocImpl));
    mCallbackMonitor.Wait();
    if (mState != kReleased) {
      return NS_ERROR_FAILURE;
    }

    mState = kReleased;
    LOG(("Video device %d deallocated", mCaptureIndex));
  } else {
    LOG(("Video device %d deallocated but still in use", mCaptureIndex));
  }
  return NS_OK;
}

nsresult
MediaEngineGonkVideoSource::Start(SourceMediaStream* aStream, TrackID aID)
{
  LOG((__FUNCTION__));
  if (!mInitDone || !aStream) {
    return NS_ERROR_FAILURE;
  }

  mSources.AppendElement(aStream);

  aStream->AddTrack(aID, 0, new VideoSegment());
  aStream->AdvanceKnownTracksTime(STREAM_TIME_MAX);

  ReentrantMonitorAutoEnter sync(mCallbackMonitor);

  if (mState == kStarted) {
    return NS_OK;
  }
  mTrackID = aID;
  mImageContainer = layers::LayerManager::CreateImageContainer();

  NS_DispatchToMainThread(WrapRunnable(nsRefPtr<MediaEngineGonkVideoSource>(this),
                                       &MediaEngineGonkVideoSource::StartImpl,
                                       mCapability));
  mCallbackMonitor.Wait();
  if (mState != kStarted) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
MediaEngineGonkVideoSource::Stop(SourceMediaStream* aSource, TrackID aID)
{
  LOG((__FUNCTION__));
  if (!mSources.RemoveElement(aSource)) {
    
    return NS_OK;
  }
  if (!mSources.IsEmpty()) {
    return NS_OK;
  }

  ReentrantMonitorAutoEnter sync(mCallbackMonitor);

  if (mState != kStarted) {
    return NS_ERROR_FAILURE;
  }

  {
    MonitorAutoLock lock(mMonitor);
    mState = kStopped;
    aSource->EndTrack(aID);
    
    
    mImage = nullptr;
  }

  NS_DispatchToMainThread(WrapRunnable(nsRefPtr<MediaEngineGonkVideoSource>(this),
                                       &MediaEngineGonkVideoSource::StopImpl));

  return NS_OK;
}






void
MediaEngineGonkVideoSource::Init()
{
  nsAutoCString deviceName;
  ICameraControl::GetCameraName(mCaptureIndex, deviceName);
  CopyUTF8toUTF16(deviceName, mDeviceName);
  CopyUTF8toUTF16(deviceName, mUniqueId);

  mInitDone = true;
}

void
MediaEngineGonkVideoSource::Shutdown()
{
  LOG((__FUNCTION__));
  if (!mInitDone) {
    return;
  }

  ReentrantMonitorAutoEnter sync(mCallbackMonitor);

  if (mState == kStarted) {
    while (!mSources.IsEmpty()) {
      Stop(mSources[0], kVideoTrack); 
    }
    MOZ_ASSERT(mState == kStopped);
  }

  if (mState == kAllocated || mState == kStopped) {
    Deallocate();
  }

  mState = kReleased;
  mInitDone = false;
}


void
MediaEngineGonkVideoSource::AllocImpl() {
  MOZ_ASSERT(NS_IsMainThread());
  ReentrantMonitorAutoEnter sync(mCallbackMonitor);

  mCameraControl = ICameraControl::Create(mCaptureIndex);
  if (mCameraControl) {
    mState = kAllocated;
    
    
    
    mCameraControl->AddListener(this);
  }
  mCallbackMonitor.Notify();
}

void
MediaEngineGonkVideoSource::DeallocImpl() {
  MOZ_ASSERT(NS_IsMainThread());

  mCameraControl = nullptr;
}


static int
GetRotateAmount(ScreenOrientation aScreen, int aCameraMountAngle, bool aBackCamera) {
  int screenAngle = 0;
  switch (aScreen) {
    case eScreenOrientation_PortraitPrimary:
      screenAngle = 0;
      break;
    case eScreenOrientation_PortraitSecondary:
      screenAngle = 180;
      break;
   case eScreenOrientation_LandscapePrimary:
      screenAngle = 90;
      break;
   case eScreenOrientation_LandscapeSecondary:
      screenAngle = 270;
      break;
   default:
      MOZ_ASSERT(false);
      break;
  }

  int result;

  if (aBackCamera) {
    
    result = (aCameraMountAngle - screenAngle + 360) % 360;
  } else {
    
    result = (aCameraMountAngle + screenAngle) % 360;
  }
  return result;
}


#define DYNAMIC_GUM_ROTATION

void
MediaEngineGonkVideoSource::Notify(const hal::ScreenConfiguration& aConfiguration) {
#ifdef DYNAMIC_GUM_ROTATION
  if (mHasDirectListeners) {
    
    MonitorAutoLock enter(mMonitor);
    mRotation = GetRotateAmount(aConfiguration.orientation(), mCameraAngle, mBackCamera);

    LOG(("*** New orientation: %d (Camera %d Back %d MountAngle: %d)",
         mRotation, mCaptureIndex, mBackCamera, mCameraAngle));
  }
#endif

  mOrientationChanged = true;
}

void
MediaEngineGonkVideoSource::StartImpl(webrtc::CaptureCapability aCapability) {
  MOZ_ASSERT(NS_IsMainThread());

  ICameraControl::Configuration config;
  config.mMode = ICameraControl::kPictureMode;
  config.mPreviewSize.width = aCapability.width;
  config.mPreviewSize.height = aCapability.height;
  mCameraControl->Start(&config);
  mCameraControl->Set(CAMERA_PARAM_PICTURE_SIZE, config.mPreviewSize);

  hal::RegisterScreenConfigurationObserver(this);
}

void
MediaEngineGonkVideoSource::StopImpl() {
  MOZ_ASSERT(NS_IsMainThread());

  hal::UnregisterScreenConfigurationObserver(this);
  mCameraControl->Stop();
}

void
MediaEngineGonkVideoSource::OnHardwareStateChange(HardwareState aState)
{
  ReentrantMonitorAutoEnter sync(mCallbackMonitor);
  if (aState == CameraControlListener::kHardwareClosed) {
    
    
    
    
    if (mState != kAllocated) {
      mState = kReleased;
      mCallbackMonitor.Notify();
    }
  } else {
    
    NS_DispatchToMainThread(WrapRunnable(nsRefPtr<MediaEngineGonkVideoSource>(this),
                                         &MediaEngineGonkVideoSource::GetRotation));
    mState = kStarted;
    mCallbackMonitor.Notify();
  }
}


void
MediaEngineGonkVideoSource::GetRotation()
{
  MOZ_ASSERT(NS_IsMainThread());
  MonitorAutoLock enter(mMonitor);

  mCameraControl->Get(CAMERA_PARAM_SENSORANGLE, mCameraAngle);
  MOZ_ASSERT(mCameraAngle == 0 || mCameraAngle == 90 || mCameraAngle == 180 ||
             mCameraAngle == 270);
  hal::ScreenConfiguration config;
  hal::GetCurrentScreenConfiguration(&config);

  nsCString deviceName;
  ICameraControl::GetCameraName(mCaptureIndex, deviceName);
  if (deviceName.EqualsASCII("back")) {
    mBackCamera = true;
  }

  mRotation = GetRotateAmount(config.orientation(), mCameraAngle, mBackCamera);
  LOG(("*** Initial orientation: %d (Camera %d Back %d MountAngle: %d)",
       mRotation, mCaptureIndex, mBackCamera, mCameraAngle));
}

void
MediaEngineGonkVideoSource::OnUserError(UserContext aContext, nsresult aError)
{
  {
    
    
    ReentrantMonitorAutoEnter sync(mCallbackMonitor);
    mCallbackMonitor.Notify();
  }

  
  class TakePhotoError : public nsRunnable {
  public:
    TakePhotoError(nsTArray<nsRefPtr<PhotoCallback>>& aCallbacks,
                   nsresult aRv)
      : mRv(aRv)
    {
      mCallbacks.SwapElements(aCallbacks);
    }

    NS_IMETHOD Run()
    {
      uint32_t callbackNumbers = mCallbacks.Length();
      for (uint8_t i = 0; i < callbackNumbers; i++) {
        mCallbacks[i]->PhotoError(mRv);
      }
      
      mCallbacks.Clear();
      return NS_OK;
    }

  protected:
    nsTArray<nsRefPtr<PhotoCallback>> mCallbacks;
    nsresult mRv;
  };

  if (aContext == UserContext::kInTakePicture) {
    MonitorAutoLock lock(mMonitor);
    if (mPhotoCallbacks.Length()) {
      NS_DispatchToMainThread(new TakePhotoError(mPhotoCallbacks, aError));
    }
  }
}

void
MediaEngineGonkVideoSource::OnTakePictureComplete(uint8_t* aData, uint32_t aLength, const nsAString& aMimeType)
{
  
  
  mCameraControl->StartPreview();

  
  
  class GenerateBlobRunnable : public nsRunnable {
  public:
    GenerateBlobRunnable(nsTArray<nsRefPtr<PhotoCallback>>& aCallbacks,
                         uint8_t* aData,
                         uint32_t aLength,
                         const nsAString& aMimeType)
      : mPhotoDataLength(aLength)
    {
      mCallbacks.SwapElements(aCallbacks);
      mPhotoData = (uint8_t*) moz_malloc(aLength);
      memcpy(mPhotoData, aData, mPhotoDataLength);
      mMimeType = aMimeType;
    }

    NS_IMETHOD Run()
    {
      nsRefPtr<dom::File> blob =
        dom::File::CreateMemoryFile(nullptr, mPhotoData, mPhotoDataLength, mMimeType);
      uint32_t callbackCounts = mCallbacks.Length();
      for (uint8_t i = 0; i < callbackCounts; i++) {
        nsRefPtr<dom::File> tempBlob = blob;
        mCallbacks[i]->PhotoComplete(tempBlob.forget());
      }
      
      mCallbacks.Clear();
      return NS_OK;
    }

    nsTArray<nsRefPtr<PhotoCallback>> mCallbacks;
    uint8_t* mPhotoData;
    nsString mMimeType;
    uint32_t mPhotoDataLength;
  };

  
  
  
  MonitorAutoLock lock(mMonitor);
  if (mPhotoCallbacks.Length()) {
    NS_DispatchToMainThread(
      new GenerateBlobRunnable(mPhotoCallbacks, aData, aLength, aMimeType));
  }
}

nsresult
MediaEngineGonkVideoSource::TakePhoto(PhotoCallback* aCallback)
{
  MOZ_ASSERT(NS_IsMainThread());

  MonitorAutoLock lock(mMonitor);

  
  
  if (!mPhotoCallbacks.Length()) {
    nsresult rv;
    if (mOrientationChanged) {
      UpdatePhotoOrientation();
    }
    rv = mCameraControl->TakePicture();
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  mPhotoCallbacks.AppendElement(aCallback);

  return NS_OK;
}

nsresult
MediaEngineGonkVideoSource::UpdatePhotoOrientation()
{
  MOZ_ASSERT(NS_IsMainThread());

  hal::ScreenConfiguration config;
  hal::GetCurrentScreenConfiguration(&config);

  
  int orientation = 0;
  switch (config.orientation()) {
    case eScreenOrientation_PortraitPrimary:
      orientation = 0;
      break;
    case eScreenOrientation_PortraitSecondary:
      orientation = 180;
      break;
   case eScreenOrientation_LandscapePrimary:
      orientation = 270;
      break;
   case eScreenOrientation_LandscapeSecondary:
      orientation = 90;
      break;
  }

  
  orientation = (mBackCamera ? orientation : (-orientation));

  ICameraControlParameterSetAutoEnter batch(mCameraControl);
  
  mCameraControl->Set(CAMERA_PARAM_PICTURE_ROTATION, orientation);

  mOrientationChanged = false;

  return NS_OK;
}

uint32_t
MediaEngineGonkVideoSource::ConvertPixelFormatToFOURCC(int aFormat)
{
  switch (aFormat) {
  case HAL_PIXEL_FORMAT_RGBA_8888:
    return libyuv::FOURCC_BGRA;
  case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    return libyuv::FOURCC_NV21;
  case HAL_PIXEL_FORMAT_YV12:
    return libyuv::FOURCC_YV12;
  default: {
    LOG((" xxxxx Unknown pixel format %d", aFormat));
    MOZ_ASSERT(false, "Unknown pixel format.");
    return libyuv::FOURCC_ANY;
    }
  }
}

void
MediaEngineGonkVideoSource::RotateImage(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight) {
  layers::GrallocImage *nativeImage = static_cast<layers::GrallocImage*>(aImage);
  android::sp<android::GraphicBuffer> graphicBuffer = nativeImage->GetGraphicBuffer();
  void *pMem = nullptr;
  uint32_t size = aWidth * aHeight * 3 / 2;

  graphicBuffer->lock(android::GraphicBuffer::USAGE_SW_READ_MASK, &pMem);

  uint8_t* srcPtr = static_cast<uint8_t*>(pMem);
  
  nsRefPtr<layers::Image> image = mImageContainer->CreateImage(ImageFormat::PLANAR_YCBCR);
  layers::PlanarYCbCrImage* videoImage = static_cast<layers::PlanarYCbCrImage*>(image.get());

  uint32_t dstWidth;
  uint32_t dstHeight;

  if (mRotation == 90 || mRotation == 270) {
    dstWidth = aHeight;
    dstHeight = aWidth;
  } else {
    dstWidth = aWidth;
    dstHeight = aHeight;
  }

  uint32_t half_width = dstWidth / 2;
  uint8_t* dstPtr = videoImage->AllocateAndGetNewBuffer(size);
  libyuv::ConvertToI420(srcPtr, size,
                        dstPtr, dstWidth,
                        dstPtr + (dstWidth * dstHeight), half_width,
                        dstPtr + (dstWidth * dstHeight * 5 / 4), half_width,
                        0, 0,
                        aWidth, aHeight,
                        aWidth, aHeight,
                        static_cast<libyuv::RotationMode>(mRotation),
                        ConvertPixelFormatToFOURCC(graphicBuffer->getPixelFormat()));
  graphicBuffer->unlock();

  const uint8_t lumaBpp = 8;
  const uint8_t chromaBpp = 4;

  layers::PlanarYCbCrData data;
  data.mYChannel = dstPtr;
  data.mYSize = IntSize(dstWidth, dstHeight);
  data.mYStride = dstWidth * lumaBpp / 8;
  data.mCbCrStride = dstWidth * chromaBpp / 8;
  data.mCbChannel = dstPtr + dstHeight * data.mYStride;
  data.mCrChannel = data.mCbChannel +( dstHeight * data.mCbCrStride / 2);
  data.mCbCrSize = IntSize(dstWidth / 2, dstHeight / 2);
  data.mPicX = 0;
  data.mPicY = 0;
  data.mPicSize = IntSize(dstWidth, dstHeight);
  data.mStereoMode = StereoMode::MONO;

  videoImage->SetDataNoCopy(data);

  
  mImage = image.forget();

  
  
  
  

  
  
  
  uint32_t len = mSources.Length();
  for (uint32_t i = 0; i < len; i++) {
    if (mSources[i]) {
      AppendToTrack(mSources[i], mImage, mTrackID, 1); 
    }
  }
}

bool
MediaEngineGonkVideoSource::OnNewPreviewFrame(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight) {
  {
    ReentrantMonitorAutoEnter sync(mCallbackMonitor);
    if (mState == kStopped) {
      return false;
    }
  }

  MonitorAutoLock enter(mMonitor);
  
  RotateImage(aImage, aWidth, aHeight);
  if (mRotation != 0 && mRotation != 180) {
    uint32_t temp = aWidth;
    aWidth = aHeight;
    aHeight = temp;
  }
  if (mWidth != static_cast<int>(aWidth) || mHeight != static_cast<int>(aHeight)) {
    mWidth = aWidth;
    mHeight = aHeight;
    LOG(("Video FrameSizeChange: %ux%u", mWidth, mHeight));
  }

  return true; 
}

} 
