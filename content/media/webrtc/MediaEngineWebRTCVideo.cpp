



#include "MediaEngineWebRTC.h"
#include "Layers.h"
#include "ImageTypes.h"
#include "ImageContainer.h"
#include "nsMemory.h"
#include "mtransport/runnable_utils.h"

#ifdef MOZ_B2G_CAMERA
#include "GrallocImages.h"
#include "libyuv.h"
#endif
namespace mozilla {

using namespace mozilla::gfx;

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetMediaManagerLog();
#define LOG(msg) PR_LOG(GetMediaManagerLog(), PR_LOG_DEBUG, msg)
#define LOGFRAME(msg) PR_LOG(GetMediaManagerLog(), 6, msg)
#else
#define LOG(msg)
#define LOGFRAME(msg)
#endif




#ifndef MOZ_B2G_CAMERA
NS_IMPL_ISUPPORTS1(MediaEngineWebRTCVideoSource, nsIRunnable)
#else
NS_IMPL_QUERY_INTERFACE1(MediaEngineWebRTCVideoSource, nsIRunnable)
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

  MOZ_ASSERT(mWidth*mHeight*3/2 == size);
  if (mWidth*mHeight*3/2 != size) {
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
  data.mYStride = mWidth * lumaBpp/ 8;
  data.mCbCrStride = mWidth * chromaBpp / 8;
  data.mCbChannel = frame + mHeight * data.mYStride;
  data.mCrChannel = data.mCbChannel + mHeight * data.mCbCrStride / 2;
  data.mCbCrSize = IntSize(mWidth/ 2, mHeight/ 2);
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
  if (mState != kStarted)
    return;

  
  nsRefPtr<layers::Image> image = mImage;
  TrackTicks target = TimeToTicksRoundUp(USECS_PER_S, aDesiredTime);
  TrackTicks delta = target - aLastEndTime;
  LOGFRAME(("NotifyPull, desired = %ld, target = %ld, delta = %ld %s", (int64_t) aDesiredTime, 
            (int64_t) target, (int64_t) delta, image ? "" : "<null>"));

  
  
  
  
  
  
  

  
  
  if (delta > 0) {
    
    if (image) {
      segment.AppendFrame(image.forget(), delta, IntSize(mWidth, mHeight));
    } else {
      segment.AppendFrame(nullptr, delta, IntSize(0, 0));
    }
    
    
    if (aSource->AppendToTrack(aID, &(segment))) {
      aLastEndTime = target;
    }
  }
}

void
MediaEngineWebRTCVideoSource::ChooseCapability(const MediaEnginePrefs &aPrefs)
{
#ifdef MOZ_B2G_CAMERA
  mCapability.width  = aPrefs.mWidth;
  mCapability.height = aPrefs.mHeight;
#else
  int num = mViECapture->NumberOfCapabilities(NS_ConvertUTF16toUTF8(mUniqueId).get(),
                                              KMaxUniqueIdLength);

  LOG(("ChooseCapability: prefs: %dx%d @%d-%dfps", aPrefs.mWidth, aPrefs.mHeight, aPrefs.mFPS, aPrefs.mMinFPS));

  if (num <= 0) {
    
    mCapability.width  = aPrefs.mWidth;
    mCapability.height = aPrefs.mHeight;
    mCapability.maxFPS = MediaEngine::DEFAULT_VIDEO_FPS;

    
    return;
  }

  
  
  
  webrtc::CaptureCapability cap;
  bool higher = true;
  for (int i = 0; i < num; i++) {
    mViECapture->GetCaptureCapability(NS_ConvertUTF16toUTF8(mUniqueId).get(),
                                      KMaxUniqueIdLength, i, cap);
    if (higher) {
      if (i == 0 ||
          (mCapability.width > cap.width && mCapability.height > cap.height)) {
        
        mCapability = cap;
        
      }
      if (cap.width <= (uint32_t) aPrefs.mWidth && cap.height <= (uint32_t) aPrefs.mHeight) {
        higher = false;
      }
    } else {
      if (cap.width > (uint32_t) aPrefs.mWidth || cap.height > (uint32_t) aPrefs.mHeight ||
          cap.maxFPS < (uint32_t) aPrefs.mMinFPS) {
        continue;
      }
      if (mCapability.width < cap.width && mCapability.height < cap.height) {
        mCapability = cap;
        
      }
    }
  }
  LOG(("chose cap %dx%d @%dfps", mCapability.width, mCapability.height, mCapability.maxFPS));
#endif
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
MediaEngineWebRTCVideoSource::Allocate(const MediaEnginePrefs &aPrefs)
{
  LOG((__FUNCTION__));
#ifdef MOZ_B2G_CAMERA
  ReentrantMonitorAutoEnter sync(mCallbackMonitor);
  if (mState == kReleased && mInitDone) {
    ChooseCapability(aPrefs);
    NS_DispatchToMainThread(WrapRunnable(this,
                                         &MediaEngineWebRTCVideoSource::AllocImpl));
    mCallbackMonitor.Wait();
    if (mState != kAllocated) {
      return NS_ERROR_FAILURE;
    }
  }
#else
  if (mState == kReleased && mInitDone) {
    
    

    ChooseCapability(aPrefs);

    if (mViECapture->AllocateCaptureDevice(NS_ConvertUTF16toUTF8(mUniqueId).get(),
                                           KMaxUniqueIdLength, mCaptureIndex)) {
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
    

    NS_DispatchToMainThread(WrapRunnable(this,
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
  NS_DispatchToMainThread(WrapRunnable(this,
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
  NS_DispatchToMainThread(WrapRunnable(this,
                                       &MediaEngineWebRTCVideoSource::StopImpl));
#else
  mViERender->StopRender(mCaptureIndex);
  mViERender->RemoveRenderer(mCaptureIndex);
  mViECapture->StopCapture(mCaptureIndex);
#endif

  return NS_OK;
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

  const uint32_t KMaxDeviceNameLength = 128;
  const uint32_t KMaxUniqueIdLength = 256;
  char deviceName[KMaxDeviceNameLength];
  char uniqueId[KMaxUniqueIdLength];
  if (mViECapture->GetCaptureDevice(mCaptureIndex,
                                    deviceName, KMaxDeviceNameLength,
                                    uniqueId, KMaxUniqueIdLength)) {
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

#ifdef MOZ_B2G_CAMERA


void
MediaEngineWebRTCVideoSource::AllocImpl() {
  MOZ_ASSERT(NS_IsMainThread());

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

void
MediaEngineWebRTCVideoSource::StartImpl(webrtc::CaptureCapability aCapability) {
  MOZ_ASSERT(NS_IsMainThread());

  ICameraControl::Configuration config;
  config.mMode = ICameraControl::kPictureMode;
  config.mPreviewSize.width = aCapability.width;
  config.mPreviewSize.height = aCapability.height;
  mCameraControl->Start(&config);
  mCameraControl->Set(CAMERA_PARAM_PICTURESIZE, config.mPreviewSize);
  mCameraControl->Get(CAMERA_PARAM_SENSORANGLE, mSensorAngle);
  MOZ_ASSERT(mSensorAngle >= 0 && mSensorAngle < 360);
}

void
MediaEngineWebRTCVideoSource::StopImpl() {
  MOZ_ASSERT(NS_IsMainThread());

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
    mState = kStarted;
    mCallbackMonitor.Notify();
  }
}

void
MediaEngineWebRTCVideoSource::OnError(CameraErrorContext aContext, CameraError aError)
{
  ReentrantMonitorAutoEnter sync(mCallbackMonitor);
  mCallbackMonitor.Notify();
}

void
MediaEngineWebRTCVideoSource::OnTakePictureComplete(uint8_t* aData, uint32_t aLength, const nsAString& aMimeType)
{
  ReentrantMonitorAutoEnter sync(mCallbackMonitor);
  mLastCapture =
    static_cast<nsIDOMFile*>(new nsDOMMemoryFile(static_cast<void*>(aData),
                                                 static_cast<uint64_t>(aLength),
                                                 aMimeType));
  mCallbackMonitor.Notify();
}

void
MediaEngineWebRTCVideoSource::RotateImage(layers::Image* aImage) {
  layers::GrallocImage *nativeImage = static_cast<layers::GrallocImage*>(aImage);
  layers::SurfaceDescriptor handle = nativeImage->GetSurfaceDescriptor();
  layers::SurfaceDescriptorGralloc grallocHandle = handle.get_SurfaceDescriptorGralloc();
  android::sp<android::GraphicBuffer> graphicBuffer = layers::GrallocBufferActor::GetFrom(grallocHandle);
  void *pMem = nullptr;
  uint32_t size = mWidth * mHeight * 3 / 2;

  graphicBuffer->lock(android::GraphicBuffer::USAGE_SW_READ_MASK, &pMem);

  uint8_t* srcPtr = static_cast<uint8_t*>(pMem);
  
  nsRefPtr<layers::Image> image = mImageContainer->CreateImage(ImageFormat::PLANAR_YCBCR);
  layers::PlanarYCbCrImage* videoImage = static_cast<layers::PlanarYCbCrImage*>(image.get());

  uint32_t dstWidth = mWidth;
  uint32_t dstHeight = mHeight;

  if (mSensorAngle == 90 || mSensorAngle == 270) {
    dstWidth = mHeight;
    dstHeight = mWidth;
  }

  uint32_t half_width = dstWidth / 2;
  uint8_t* dstPtr = videoImage->AllocateAndGetNewBuffer(size);
  libyuv::ConvertToI420(srcPtr, size,
                        dstPtr, dstWidth,
                        dstPtr + (dstWidth * dstHeight), half_width,
                        dstPtr + (dstWidth * dstHeight * 5 / 4), half_width,
                        0, 0,
                        mWidth, mHeight,
                        mWidth, mHeight,
                        static_cast<libyuv::RotationMode>(mSensorAngle),
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
  MonitorAutoLock enter(mMonitor);
  if (mState == kStopped) {
    return false;
  }
  if (mWidth != static_cast<int>(aWidth) || mHeight != static_cast<int>(aHeight)) {
    mWidth = aWidth;
    mHeight = aHeight;
    LOG(("Video FrameSizeChange: %ux%u", mWidth, mHeight));
  }

  if (mSensorAngle == 0) {
    mImage = aImage;
  } else {
    RotateImage(aImage);
  }
  return true; 
}
#endif

}
