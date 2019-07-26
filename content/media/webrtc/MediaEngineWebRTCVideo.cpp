



#include "MediaEngineWebRTC.h"
#include "Layers.h"
#include "ImageTypes.h"
#include "ImageContainer.h"
#include "mtransport/runnable_utils.h"

namespace mozilla {

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetMediaManagerLog();
#define LOG(msg) PR_LOG(GetMediaManagerLog(), PR_LOG_DEBUG, msg)
#define LOGFRAME(msg) PR_LOG(GetMediaManagerLog(), 6, msg)
#else
#define LOG(msg)
#define LOGFRAME(msg)
#endif




NS_IMPL_THREADSAFE_ISUPPORTS1(MediaEngineWebRTCVideoSource, nsIRunnable)


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
   unsigned char* buffer, int size, uint32_t time_stamp, int64_t render_time)
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

  
  ImageFormat format = PLANAR_YCBCR;

  nsRefPtr<layers::Image> image = mImageContainer->CreateImage(&format, 1);

  layers::PlanarYCbCrImage* videoImage = static_cast<layers::PlanarYCbCrImage*>(image.get());

  uint8_t* frame = static_cast<uint8_t*> (buffer);
  const uint8_t lumaBpp = 8;
  const uint8_t chromaBpp = 4;

  layers::PlanarYCbCrImage::Data data;
  data.mYChannel = frame;
  data.mYSize = gfxIntSize(mWidth, mHeight);
  data.mYStride = mWidth * lumaBpp/ 8;
  data.mCbCrStride = mWidth * chromaBpp / 8;
  data.mCbChannel = frame + mHeight * data.mYStride;
  data.mCrChannel = data.mCbChannel + mHeight * data.mCbCrStride / 2;
  data.mCbCrSize = gfxIntSize(mWidth/ 2, mHeight/ 2);
  data.mPicX = 0;
  data.mPicY = 0;
  data.mPicSize = gfxIntSize(mWidth, mHeight);
  data.mStereoMode = STEREO_MODE_MONO;

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
      segment.AppendFrame(image.forget(), delta, gfxIntSize(mWidth, mHeight));
    } else {
      segment.AppendFrame(nullptr, delta, gfxIntSize(0,0));
    }
    
    
    if (aSource->AppendToTrack(aID, &(segment))) {
      aLastEndTime = target;
    }
  }
}

void
MediaEngineWebRTCVideoSource::ChooseCapability(const MediaEnginePrefs &aPrefs)
{
  int num = mViECapture->NumberOfCapabilities(mUniqueId, KMaxUniqueIdLength);

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
    mViECapture->GetCaptureCapability(mUniqueId, KMaxUniqueIdLength, i, cap);
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
}

void
MediaEngineWebRTCVideoSource::GetName(nsAString& aName)
{
  
  CopyUTF8toUTF16(mDeviceName, aName);
}

void
MediaEngineWebRTCVideoSource::GetUUID(nsAString& aUUID)
{
  
  CopyUTF8toUTF16(mUniqueId, aUUID);
}

nsresult
MediaEngineWebRTCVideoSource::Allocate(const MediaEnginePrefs &aPrefs)
{
  LOG((__FUNCTION__));
  if (mState == kReleased && mInitDone) {
    
    

    ChooseCapability(aPrefs);

    if (mViECapture->AllocateCaptureDevice(mUniqueId, KMaxUniqueIdLength, mCaptureIndex)) {
      return NS_ERROR_FAILURE;
    }
    mState = kAllocated;
    LOG(("Video device %d allocated", mCaptureIndex));
  } else if (mSources.IsEmpty()) {
    LOG(("Video device %d reallocated", mCaptureIndex));
  } else {
    LOG(("Video device %d allocated shared", mCaptureIndex));
  }

  return NS_OK;
}

nsresult
MediaEngineWebRTCVideoSource::Deallocate()
{
  LOG((__FUNCTION__));
  if (mSources.IsEmpty()) {
    if (mState != kStopped && mState != kAllocated) {
      return NS_ERROR_FAILURE;
    }

#ifdef XP_MACOSX
    
    
    
    
    
    
    
    
    
    
    
    
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
  int error = 0;
  if (!mInitDone || !aStream) {
    return NS_ERROR_FAILURE;
  }

  mSources.AppendElement(aStream);

  aStream->AddTrack(aID, USECS_PER_S, 0, new VideoSegment());
  aStream->AdvanceKnownTracksTime(STREAM_TIME_MAX);

  if (mState == kStarted) {
    return NS_OK;
  }
  mState = kStarted;

  mImageContainer = layers::LayerManager::CreateImageContainer();

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

  if (mState != kStarted) {
    return NS_ERROR_FAILURE;
  }

  {
    MonitorAutoLock lock(mMonitor);
    mState = kStopped;
    aSource->EndTrack(aID);
    
    
    mImage = nullptr;
  }

  mViERender->StopRender(mCaptureIndex);
  mViERender->RemoveRenderer(mCaptureIndex);
  mViECapture->StopCapture(mCaptureIndex);

  return NS_OK;
}

nsresult
MediaEngineWebRTCVideoSource::Snapshot(uint32_t aDuration, nsIDOMFile** aFile)
{
  















  *aFile = nullptr;
  if (!mInitDone || mState != kAllocated) {
    return NS_ERROR_FAILURE;
  }

  {
    MonitorAutoLock lock(mMonitor);
    mInSnapshotMode = true;
  }

  
  int error = 0;
  if (!mInitDone || mState != kAllocated) {
    return NS_ERROR_FAILURE;
  }
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

  
  
  
  
  
  {
    MonitorAutoLock lock(mMonitor);
    while (mInSnapshotMode) {
      lock.Wait();
    }
  }

  
  webrtc::ViEFile* vieFile = webrtc::ViEFile::GetInterface(mVideoEngine);
  if (!vieFile) {
    return NS_ERROR_FAILURE;
  }

  
  
  NS_DispatchToMainThread(this, NS_DISPATCH_SYNC);

  if (!mSnapshotPath) {
    return NS_ERROR_FAILURE;
  }

  NS_ConvertUTF16toUTF8 path(*mSnapshotPath);
  if (vieFile->GetCaptureDeviceSnapshot(mCaptureIndex, path.get()) < 0) {
    delete mSnapshotPath;
    mSnapshotPath = NULL;
    return NS_ERROR_FAILURE;
  }

  
  mViERender->StopRender(mCaptureIndex);
  mViERender->RemoveRenderer(mCaptureIndex);

  nsCOMPtr<nsIFile> file;
  nsresult rv = NS_NewLocalFile(*mSnapshotPath, false, getter_AddRefs(file));

  delete mSnapshotPath;
  mSnapshotPath = NULL;

  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aFile = new nsDOMFileFile(file));

  return NS_OK;
}






void
MediaEngineWebRTCVideoSource::Init()
{
  mDeviceName[0] = '\0'; 
  mUniqueId[0] = '\0';

  
  (void) mFps;
  (void) mMinFps;

  LOG((__FUNCTION__));
  if (mVideoEngine == NULL) {
    return;
  }

  mViEBase = webrtc::ViEBase::GetInterface(mVideoEngine);
  if (mViEBase == NULL) {
    return;
  }

  
  mViECapture = webrtc::ViECapture::GetInterface(mVideoEngine);
  mViERender = webrtc::ViERender::GetInterface(mVideoEngine);

  if (mViECapture == NULL || mViERender == NULL) {
    return;
  }

  if (mViECapture->GetCaptureDevice(mCaptureIndex,
                                    mDeviceName, sizeof(mDeviceName),
                                    mUniqueId, sizeof(mUniqueId))) {
    return;
  }

  mInitDone = true;
}

void
MediaEngineWebRTCVideoSource::Shutdown()
{
  LOG((__FUNCTION__));
  if (!mInitDone) {
    return;
  }

  if (mState == kStarted) {
    while (!mSources.IsEmpty()) {
      Stop(mSources[0], kVideoTrack); 
    }
    MOZ_ASSERT(mState == kStopped);
  }

  if (mState == kAllocated || mState == kStopped) {
    Deallocate();
  }

  mViECapture->Release();
  mViERender->Release();
  mViEBase->Release();
  mState = kReleased;
  mInitDone = false;
}

}
