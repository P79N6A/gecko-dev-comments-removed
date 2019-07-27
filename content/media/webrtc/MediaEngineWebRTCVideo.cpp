



#include "MediaEngineWebRTC.h"
#include "Layers.h"
#include "ImageTypes.h"
#include "ImageContainer.h"
#include "mozilla/layers/GrallocTextureClient.h"
#include "nsMemory.h"
#include "mtransport/runnable_utils.h"
#include "MediaTrackConstraints.h"

#ifdef MOZ_B2G_CAMERA
#include "GrallocImages.h"
#include "libyuv.h"
#include "mozilla/Hal.h"
#include "ScreenOrientation.h"
using namespace mozilla::dom;
#endif
namespace mozilla {

using namespace mozilla::gfx;
using dom::ConstrainLongRange;
using dom::ConstrainDoubleRange;
using dom::MediaTrackConstraintSet;

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetMediaManagerLog();
#define LOG(msg) PR_LOG(GetMediaManagerLog(), PR_LOG_DEBUG, msg)
#define LOGFRAME(msg) PR_LOG(GetMediaManagerLog(), 6, msg)
#else
#define LOG(msg)
#define LOGFRAME(msg)
#endif




#ifndef MOZ_B2G_CAMERA
NS_IMPL_ISUPPORTS(MediaEngineWebRTCVideoSource, nsIRunnable)
#else
NS_IMPL_QUERY_INTERFACE(MediaEngineWebRTCVideoSource, nsIRunnable)
NS_IMPL_ADDREF_INHERITED(MediaEngineWebRTCVideoSource, CameraControlListener)
NS_IMPL_RELEASE_INHERITED(MediaEngineWebRTCVideoSource, CameraControlListener)
#endif


#ifndef MOZ_B2G_CAMERA
int
MediaEngineWebRTCVideoSource::FrameSizeChange(
   unsigned int w, unsigned int h, unsigned int streams)
{
  mWidth = w;
  mHeight = h;
  LOG(("Video FrameSizeChange: %ux%u", w, h));
  return 0;
}


int
MediaEngineWebRTCVideoSource::DeliverFrame(
   unsigned char* buffer, int size, uint32_t time_stamp, int64_t render_time,
   void *handle)
{
  
  
  if (mInSnapshotMode) {
    
    MonitorAutoLock lock(mMonitor);
    mInSnapshotMode = false;
    lock.Notify();
    return 0;
  }

  
  if (mState != kStarted) {
    LOG(("DeliverFrame: video not started"));
    return 0;
  }

  if (mWidth*mHeight + 2*(((mWidth+1)/2)*((mHeight+1)/2)) != size) {
    MOZ_ASSERT(false, "Wrong size frame in DeliverFrame!");
    return 0;
  }

  
  nsRefPtr<layers::Image> image = mImageContainer->CreateImage(ImageFormat::PLANAR_YCBCR);

  layers::PlanarYCbCrImage* videoImage = static_cast<layers::PlanarYCbCrImage*>(image.get());

  uint8_t* frame = static_cast<uint8_t*> (buffer);
  const uint8_t lumaBpp = 8;
  const uint8_t chromaBpp = 4;

  
  layers::PlanarYCbCrData data;
  data.mYChannel = frame;
  data.mYSize = IntSize(mWidth, mHeight);
  data.mYStride = (mWidth * lumaBpp + 7)/ 8;
  data.mCbCrStride = (mWidth * chromaBpp + 7) / 8;
  data.mCbChannel = frame + mHeight * data.mYStride;
  data.mCrChannel = data.mCbChannel + ((mHeight+1)/2) * data.mCbCrStride;
  data.mCbCrSize = IntSize((mWidth+1)/ 2, (mHeight+1)/ 2);
  data.mPicX = 0;
  data.mPicY = 0;
  data.mPicSize = IntSize(mWidth, mHeight);
  data.mStereoMode = StereoMode::MONO;

  videoImage->SetData(data);

#ifdef DEBUG
  static uint32_t frame_num = 0;
  LOGFRAME(("frame %d (%dx%d); timestamp %u, render_time %lu", frame_num++,
            mWidth, mHeight, time_stamp, render_time));
#endif

  
  
  MonitorAutoLock lock(mMonitor);

  
  mImage = image.forget();

  return 0;
}
#endif




void
MediaEngineWebRTCVideoSource::NotifyPull(MediaStreamGraph* aGraph,
                                         SourceMediaStream *aSource,
                                         TrackID aID,
                                         StreamTime aDesiredTime,
                                         TrackTicks &aLastEndTime)
{
  VideoSegment segment;

  MonitorAutoLock lock(mMonitor);
  
  
  

  
  nsRefPtr<layers::Image> image = mImage;
  TrackTicks target = aSource->TimeToTicksRoundUp(USECS_PER_S, aDesiredTime);
  TrackTicks delta = target - aLastEndTime;
  LOGFRAME(("NotifyPull, desired = %ld, target = %ld, delta = %ld %s", (int64_t) aDesiredTime,
            (int64_t) target, (int64_t) delta, image ? "" : "<null>"));

  
  
  
  
  
  
  

  
  
  if (delta > 0) {
    
    IntSize size(image ? mWidth : 0, image ? mHeight : 0);
    segment.AppendFrame(image.forget(), delta, size);
    
    
    if (aSource->AppendToTrack(aID, &(segment))) {
      aLastEndTime = target;
    }
  }
}

static bool IsWithin(int32_t n, const ConstrainLongRange& aRange) {
  return aRange.mMin <= n && n <= aRange.mMax;
}

static bool IsWithin(double n, const ConstrainDoubleRange& aRange) {
  return aRange.mMin <= n && n <= aRange.mMax;
}

static int32_t Clamp(int32_t n, const ConstrainLongRange& aRange) {
  return std::max(aRange.mMin, std::min(n, aRange.mMax));
}

static bool
AreIntersecting(const ConstrainLongRange& aA, const ConstrainLongRange& aB) {
  return aA.mMax >= aB.mMin && aA.mMin <= aB.mMax;
}

static bool
Intersect(ConstrainLongRange& aA, const ConstrainLongRange& aB) {
  MOZ_ASSERT(AreIntersecting(aA, aB));
  aA.mMin = std::max(aA.mMin, aB.mMin);
  aA.mMax = std::min(aA.mMax, aB.mMax);
  return true;
}

static bool SatisfyConstraintSet(const MediaTrackConstraintSet &aConstraints,
                                 const webrtc::CaptureCapability& aCandidate) {
  if (!IsWithin(aCandidate.width, aConstraints.mWidth) ||
      !IsWithin(aCandidate.height, aConstraints.mHeight)) {
    return false;
  }
  if (!IsWithin(aCandidate.maxFPS, aConstraints.mFrameRate)) {
    return false;
  }
  return true;
}

void
MediaEngineWebRTCVideoSource::ChooseCapability(
    const VideoTrackConstraintsN &aConstraints,
    const MediaEnginePrefs &aPrefs)
{
#ifdef MOZ_B2G_CAMERA
  return GuessCapability(aConstraints, aPrefs);
#else
  NS_ConvertUTF16toUTF8 uniqueId(mUniqueId);
  int num = mViECapture->NumberOfCapabilities(uniqueId.get(), kMaxUniqueIdLength);
  if (num <= 0) {
    
    return GuessCapability(aConstraints, aPrefs);
  }

  

  LOG(("ChooseCapability: prefs: %dx%d @%d-%dfps",
       aPrefs.mWidth, aPrefs.mHeight, aPrefs.mFPS, aPrefs.mMinFPS));

  typedef nsTArray<uint8_t> SourceSet;

  SourceSet candidateSet;
  for (int i = 0; i < num; i++) {
    candidateSet.AppendElement(i);
  }

  

  for (uint32_t i = 0; i < candidateSet.Length();) {
    webrtc::CaptureCapability cap;
    mViECapture->GetCaptureCapability(uniqueId.get(), kMaxUniqueIdLength,
                                      candidateSet[i], cap);
    if (!SatisfyConstraintSet(aConstraints.mRequired, cap)) {
      candidateSet.RemoveElementAt(i);
    } else {
      ++i;
    }
  }

  SourceSet tailSet;

  

  if (aConstraints.mAdvanced.WasPassed()) {
    auto &array = aConstraints.mAdvanced.Value();

    for (uint32_t i = 0; i < array.Length(); i++) {
      SourceSet rejects;
      for (uint32_t j = 0; j < candidateSet.Length();) {
        webrtc::CaptureCapability cap;
        mViECapture->GetCaptureCapability(uniqueId.get(), kMaxUniqueIdLength,
                                          candidateSet[j], cap);
        if (!SatisfyConstraintSet(array[i], cap)) {
          rejects.AppendElement(candidateSet[j]);
          candidateSet.RemoveElementAt(j);
        } else {
          ++j;
        }
      }
      (candidateSet.Length()? tailSet : candidateSet).MoveElementsFrom(rejects);
    }
  }

  if (!candidateSet.Length()) {
    candidateSet.AppendElement(0);
  }

  int prefWidth = aPrefs.GetWidth();
  int prefHeight = aPrefs.GetHeight();

  
  
  

  webrtc::CaptureCapability cap;
  bool higher = true;
  for (uint32_t i = 0; i < candidateSet.Length(); i++) {
    mViECapture->GetCaptureCapability(NS_ConvertUTF16toUTF8(mUniqueId).get(),
                                      kMaxUniqueIdLength, candidateSet[i], cap);
    if (higher) {
      if (i == 0 ||
          (mCapability.width > cap.width && mCapability.height > cap.height)) {
        
        mCapability = cap;
        
      }
      if (cap.width <= (uint32_t) prefWidth && cap.height <= (uint32_t) prefHeight) {
        higher = false;
      }
    } else {
      if (cap.width > (uint32_t) prefWidth || cap.height > (uint32_t) prefHeight ||
          cap.maxFPS < (uint32_t) aPrefs.mMinFPS) {
        continue;
      }
      if (mCapability.width < cap.width && mCapability.height < cap.height) {
        mCapability = cap;
        
      }
    }
    
    if (mCapability.width == cap.width && mCapability.height == cap.height) {
      
      if (cap.maxFPS < (uint32_t) aPrefs.mMinFPS) {
        continue;
      }
      
      if (cap.maxFPS < mCapability.maxFPS) {
        mCapability = cap;
      } else if (cap.maxFPS == mCapability.maxFPS) {
        
        if (cap.rawType == webrtc::RawVideoType::kVideoI420
          || cap.rawType == webrtc::RawVideoType::kVideoYUY2
          || cap.rawType == webrtc::RawVideoType::kVideoYV12) {
          mCapability = cap;
        }
      }
    }
  }
  LOG(("chose cap %dx%d @%dfps codec %d raw %d",
       mCapability.width, mCapability.height, mCapability.maxFPS,
       mCapability.codecType, mCapability.rawType));
#endif
}



void
MediaEngineWebRTCVideoSource::GuessCapability(
    const VideoTrackConstraintsN &aConstraints,
    const MediaEnginePrefs &aPrefs)
{
  LOG(("GuessCapability: prefs: %dx%d @%d-%dfps",
       aPrefs.mWidth, aPrefs.mHeight, aPrefs.mFPS, aPrefs.mMinFPS));

  

  ConstrainLongRange cWidth(aConstraints.mRequired.mWidth);
  ConstrainLongRange cHeight(aConstraints.mRequired.mHeight);

  if (aConstraints.mAdvanced.WasPassed()) {
    const auto& advanced = aConstraints.mAdvanced.Value();
    for (uint32_t i = 0; i < advanced.Length(); i++) {
      if (AreIntersecting(cWidth, advanced[i].mWidth) &&
          AreIntersecting(cHeight, advanced[i].mHeight)) {
        Intersect(cWidth, advanced[i].mWidth);
        Intersect(cHeight, advanced[i].mHeight);
      }
    }
  }
  
  
  
  
  

  bool macHD = ((!aPrefs.mWidth || !aPrefs.mHeight) &&
                mDeviceName.EqualsASCII("FaceTime HD Camera (Built-in)") &&
                (aPrefs.GetWidth() < cWidth.mMin ||
                 aPrefs.GetHeight() < cHeight.mMin) &&
                !(aPrefs.GetWidth(true) > cWidth.mMax ||
                  aPrefs.GetHeight(true) > cHeight.mMax));
  int prefWidth = aPrefs.GetWidth(macHD);
  int prefHeight = aPrefs.GetHeight(macHD);

  

  if (IsWithin(prefWidth, cWidth) == IsWithin(prefHeight, cHeight)) {
    
    
    
    mCapability.width = Clamp(prefWidth, cWidth);
    mCapability.height = Clamp(prefHeight, cHeight);
  } else {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (IsWithin(prefWidth, cWidth)) {
      mCapability.height = Clamp(prefHeight, cHeight);
      mCapability.width = Clamp((mCapability.height * prefWidth) /
                                prefHeight, cWidth);
    } else {
      mCapability.width = Clamp(prefWidth, cWidth);
      mCapability.height = Clamp((mCapability.width * prefHeight) /
                                 prefWidth, cHeight);
    }
  }
  mCapability.maxFPS = MediaEngine::DEFAULT_VIDEO_FPS;
  LOG(("chose cap %dx%d @%dfps",
       mCapability.width, mCapability.height, mCapability.maxFPS));
}

void
MediaEngineWebRTCVideoSource::GetName(nsAString& aName)
{
  aName = mDeviceName;
}

void
MediaEngineWebRTCVideoSource::GetUUID(nsAString& aUUID)
{
  aUUID = mUniqueId;
}

nsresult
MediaEngineWebRTCVideoSource::Allocate(const VideoTrackConstraintsN &aConstraints,
                                       const MediaEnginePrefs &aPrefs)
{
  LOG((__FUNCTION__));
#ifdef MOZ_B2G_CAMERA
  ReentrantMonitorAutoEnter sync(mCallbackMonitor);
  if (mState == kReleased && mInitDone) {
    ChooseCapability(aConstraints, aPrefs);
    NS_DispatchToMainThread(WrapRunnable(nsRefPtr<MediaEngineWebRTCVideoSource>(this),
                                         &MediaEngineWebRTCVideoSource::AllocImpl));
    mCallbackMonitor.Wait();
    if (mState != kAllocated) {
      return NS_ERROR_FAILURE;
    }
  }
#else
  if (mState == kReleased && mInitDone) {
    
    

    ChooseCapability(aConstraints, aPrefs);

    if (mViECapture->AllocateCaptureDevice(NS_ConvertUTF16toUTF8(mUniqueId).get(),
                                           kMaxUniqueIdLength, mCaptureIndex)) {
      return NS_ERROR_FAILURE;
    }
    mState = kAllocated;
    LOG(("Video device %d allocated", mCaptureIndex));
  } else if (mSources.IsEmpty()) {
    LOG(("Video device %d reallocated", mCaptureIndex));
  } else {
    LOG(("Video device %d allocated shared", mCaptureIndex));
  }
#endif

  return NS_OK;
}

nsresult
MediaEngineWebRTCVideoSource::Deallocate()
{
  LOG((__FUNCTION__));
  if (mSources.IsEmpty()) {
#ifdef MOZ_B2G_CAMERA
    ReentrantMonitorAutoEnter sync(mCallbackMonitor);
#endif
    if (mState != kStopped && mState != kAllocated) {
      return NS_ERROR_FAILURE;
    }
#ifdef MOZ_B2G_CAMERA
    

    NS_DispatchToMainThread(WrapRunnable(nsRefPtr<MediaEngineWebRTCVideoSource>(this),
                                         &MediaEngineWebRTCVideoSource::DeallocImpl));
    mCallbackMonitor.Wait();
    if (mState != kReleased) {
      return NS_ERROR_FAILURE;
    }
#elif XP_MACOSX
    
    
    
    
    
    
    
    
    
    
    
    
    NS_DispatchToMainThread(WrapRunnable(mViECapture,
                                         &webrtc::ViECapture::ReleaseCaptureDevice,
                                         mCaptureIndex),
                            NS_DISPATCH_SYNC);
#else
    mViECapture->ReleaseCaptureDevice(mCaptureIndex);
#endif
    mState = kReleased;
    LOG(("Video device %d deallocated", mCaptureIndex));
  } else {
    LOG(("Video device %d deallocated but still in use", mCaptureIndex));
  }
  return NS_OK;
}

nsresult
MediaEngineWebRTCVideoSource::Start(SourceMediaStream* aStream, TrackID aID)
{
  LOG((__FUNCTION__));
#ifndef MOZ_B2G_CAMERA
  int error = 0;
#endif
  if (!mInitDone || !aStream) {
    return NS_ERROR_FAILURE;
  }

  mSources.AppendElement(aStream);

  aStream->AddTrack(aID, USECS_PER_S, 0, new VideoSegment());
  aStream->AdvanceKnownTracksTime(STREAM_TIME_MAX);

#ifdef MOZ_B2G_CAMERA
  ReentrantMonitorAutoEnter sync(mCallbackMonitor);
#endif

  if (mState == kStarted) {
    return NS_OK;
  }
  mImageContainer = layers::LayerManager::CreateImageContainer();

#ifdef MOZ_B2G_CAMERA
  NS_DispatchToMainThread(WrapRunnable(nsRefPtr<MediaEngineWebRTCVideoSource>(this),
                                       &MediaEngineWebRTCVideoSource::StartImpl,
                                       mCapability));
  mCallbackMonitor.Wait();
  if (mState != kStarted) {
    return NS_ERROR_FAILURE;
  }
#else
  mState = kStarted;
  error = mViERender->AddRenderer(mCaptureIndex, webrtc::kVideoI420, (webrtc::ExternalRenderer*)this);
  if (error == -1) {
    return NS_ERROR_FAILURE;
  }

  error = mViERender->StartRender(mCaptureIndex);
  if (error == -1) {
    return NS_ERROR_FAILURE;
  }

  if (mViECapture->StartCapture(mCaptureIndex, mCapability) < 0) {
    return NS_ERROR_FAILURE;
  }
#endif

  return NS_OK;
}

nsresult
MediaEngineWebRTCVideoSource::Stop(SourceMediaStream *aSource, TrackID aID)
{
  LOG((__FUNCTION__));
  if (!mSources.RemoveElement(aSource)) {
    
    return NS_OK;
  }
  if (!mSources.IsEmpty()) {
    return NS_OK;
  }
#ifdef MOZ_B2G_CAMERA
  ReentrantMonitorAutoEnter sync(mCallbackMonitor);
#endif
  if (mState != kStarted) {
    return NS_ERROR_FAILURE;
  }

  {
    MonitorAutoLock lock(mMonitor);
    mState = kStopped;
    aSource->EndTrack(aID);
    
    
    mImage = nullptr;
  }
#ifdef MOZ_B2G_CAMERA
  NS_DispatchToMainThread(WrapRunnable(nsRefPtr<MediaEngineWebRTCVideoSource>(this),
                                       &MediaEngineWebRTCVideoSource::StopImpl));
#else
  mViERender->StopRender(mCaptureIndex);
  mViERender->RemoveRenderer(mCaptureIndex);
  mViECapture->StopCapture(mCaptureIndex);
#endif

  return NS_OK;
}

void
MediaEngineWebRTCVideoSource::SetDirectListeners(bool aHasDirectListeners)
{
  LOG((__FUNCTION__));
  mHasDirectListeners = aHasDirectListeners;
}

nsresult
MediaEngineWebRTCVideoSource::Snapshot(uint32_t aDuration, nsIDOMFile** aFile)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}






void
MediaEngineWebRTCVideoSource::Init()
{
#ifdef MOZ_B2G_CAMERA
  nsAutoCString deviceName;
  ICameraControl::GetCameraName(mCaptureIndex, deviceName);
  CopyUTF8toUTF16(deviceName, mDeviceName);
  CopyUTF8toUTF16(deviceName, mUniqueId);
#else
  
  (void) mFps;
  (void) mMinFps;

  LOG((__FUNCTION__));
  if (mVideoEngine == nullptr) {
    return;
  }

  mViEBase = webrtc::ViEBase::GetInterface(mVideoEngine);
  if (mViEBase == nullptr) {
    return;
  }

  
  mViECapture = webrtc::ViECapture::GetInterface(mVideoEngine);
  mViERender = webrtc::ViERender::GetInterface(mVideoEngine);

  if (mViECapture == nullptr || mViERender == nullptr) {
    return;
  }

  char deviceName[kMaxDeviceNameLength];
  char uniqueId[kMaxUniqueIdLength];
  if (mViECapture->GetCaptureDevice(mCaptureIndex,
                                    deviceName, kMaxDeviceNameLength,
                                    uniqueId, kMaxUniqueIdLength)) {
    return;
  }

  CopyUTF8toUTF16(deviceName, mDeviceName);
  CopyUTF8toUTF16(uniqueId, mUniqueId);
#endif

  mInitDone = true;
}

void
MediaEngineWebRTCVideoSource::Shutdown()
{
  LOG((__FUNCTION__));
  if (!mInitDone) {
    return;
  }
#ifdef MOZ_B2G_CAMERA
  ReentrantMonitorAutoEnter sync(mCallbackMonitor);
#endif
  if (mState == kStarted) {
    while (!mSources.IsEmpty()) {
      Stop(mSources[0], kVideoTrack); 
    }
    MOZ_ASSERT(mState == kStopped);
  }

  if (mState == kAllocated || mState == kStopped) {
    Deallocate();
  }
#ifndef MOZ_B2G_CAMERA
  mViECapture->Release();
  mViERender->Release();
  mViEBase->Release();
#endif
  mState = kReleased;
  mInitDone = false;
}

void MediaEngineWebRTCVideoSource::Refresh(int aIndex) {
  
  
#ifdef MOZ_B2G_CAMERA
  
#else
  
  char deviceName[kMaxDeviceNameLength];
  char uniqueId[kMaxUniqueIdLength];

  if (mViECapture->GetCaptureDevice(aIndex,
                                    deviceName, sizeof(deviceName),
                                    uniqueId, sizeof(uniqueId))) {
    return;
  }

  CopyUTF8toUTF16(deviceName, mDeviceName);
#ifdef DEBUG
  nsString temp;
  CopyUTF8toUTF16(uniqueId, temp);
  MOZ_ASSERT(temp.Equals(mUniqueId));
#endif
#endif
}

#ifdef MOZ_B2G_CAMERA


void
MediaEngineWebRTCVideoSource::AllocImpl() {
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
MediaEngineWebRTCVideoSource::DeallocImpl() {
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
MediaEngineWebRTCVideoSource::Notify(const hal::ScreenConfiguration& aConfiguration) {
#ifdef DYNAMIC_GUM_ROTATION
  if (mHasDirectListeners) {
    
    MonitorAutoLock enter(mMonitor);
    mRotation = GetRotateAmount(aConfiguration.orientation(), mCameraAngle, mBackCamera);

    LOG(("*** New orientation: %d (Camera %d Back %d MountAngle: %d)",
         mRotation, mCaptureIndex, mBackCamera, mCameraAngle));
  }
#endif
}

void
MediaEngineWebRTCVideoSource::StartImpl(webrtc::CaptureCapability aCapability) {
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
MediaEngineWebRTCVideoSource::StopImpl() {
  MOZ_ASSERT(NS_IsMainThread());

  hal::UnregisterScreenConfigurationObserver(this);
  mCameraControl->Stop();
}

void
MediaEngineWebRTCVideoSource::SnapshotImpl() {
  MOZ_ASSERT(NS_IsMainThread());
  mCameraControl->TakePicture();
}

void
MediaEngineWebRTCVideoSource::OnHardwareStateChange(HardwareState aState)
{
  ReentrantMonitorAutoEnter sync(mCallbackMonitor);
  if (aState == CameraControlListener::kHardwareClosed) {
    
    
    
    
    if (mState != kAllocated) {
      mState = kReleased;
      mCallbackMonitor.Notify();
    }
  } else {
    
    NS_DispatchToMainThread(WrapRunnable(nsRefPtr<MediaEngineWebRTCVideoSource>(this),
                                         &MediaEngineWebRTCVideoSource::GetRotation));
    mState = kStarted;
    mCallbackMonitor.Notify();
  }
}

void
MediaEngineWebRTCVideoSource::GetRotation()
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
MediaEngineWebRTCVideoSource::OnUserError(UserContext aContext, nsresult aError)
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
MediaEngineWebRTCVideoSource::OnTakePictureComplete(uint8_t* aData, uint32_t aLength, const nsAString& aMimeType)
{
  
  
  mCameraControl->StartPreview();

  
  
  class GenerateBlobRunnable : public nsRunnable {
  public:
    GenerateBlobRunnable(nsTArray<nsRefPtr<PhotoCallback>>& aCallbacks,
                         uint8_t* aData,
                         uint32_t aLength,
                         const nsAString& aMimeType)
    {
      mCallbacks.SwapElements(aCallbacks);
      mPhoto.AppendElements(aData, aLength);
      mMimeType = aMimeType;
    }

    NS_IMETHOD Run()
    {
      nsRefPtr<dom::DOMFile> blob =
        dom::DOMFile::CreateMemoryFile(mPhoto.Elements(), mPhoto.Length(), mMimeType);
      uint32_t callbackCounts = mCallbacks.Length();
      for (uint8_t i = 0; i < callbackCounts; i++) {
        nsRefPtr<dom::DOMFile> tempBlob = blob;
        mCallbacks[i]->PhotoComplete(tempBlob.forget());
      }
      
      mCallbacks.Clear();
      return NS_OK;
    }

    nsTArray<nsRefPtr<PhotoCallback>> mCallbacks;
    nsTArray<uint8_t> mPhoto;
    nsString mMimeType;
  };

  
  
  
  MonitorAutoLock lock(mMonitor);
  if (mPhotoCallbacks.Length()) {
    NS_DispatchToMainThread(
      new GenerateBlobRunnable(mPhotoCallbacks, aData, aLength, aMimeType));
  }
}

void
MediaEngineWebRTCVideoSource::RotateImage(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight) {
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
                        libyuv::FOURCC_NV21);
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
}

bool
MediaEngineWebRTCVideoSource::OnNewPreviewFrame(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight) {
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

nsresult
MediaEngineWebRTCVideoSource::TakePhoto(PhotoCallback* aCallback)
{
  MOZ_ASSERT(NS_IsMainThread());

  MonitorAutoLock lock(mMonitor);

  
  
  if (!mPhotoCallbacks.Length()) {
    nsresult rv = mCameraControl->TakePicture();
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  mPhotoCallbacks.AppendElement(aCallback);

  return NS_OK;
}

#endif

}
