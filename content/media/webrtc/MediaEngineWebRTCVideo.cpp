



#include "MediaEngineWebRTC.h"
#include "Layers.h"
#include "ImageTypes.h"
#include "ImageContainer.h"

namespace mozilla {

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaManagerLog;
#define LOG(msg) PR_LOG(gMediaManagerLog, PR_LOG_DEBUG, msg)
#else
#define LOG(msg)
#endif




NS_IMPL_THREADSAFE_ISUPPORTS1(MediaEngineWebRTCVideoSource, nsIRunnable)


int
MediaEngineWebRTCVideoSource::FrameSizeChange(
   unsigned int w, unsigned int h, unsigned int streams)
{
  mWidth = w;
  mHeight = h;
  return 0;
}


int
MediaEngineWebRTCVideoSource::DeliverFrame(
   unsigned char* buffer, int size, uint32_t time_stamp, int64_t render_time)
{
  ReentrantMonitorAutoEnter enter(mMonitor);

  if (mInSnapshotMode) {
    
    PR_Lock(mSnapshotLock);
    mInSnapshotMode = false;
    PR_NotifyCondVar(mSnapshotCondVar);
    PR_Unlock(mSnapshotLock);
    return 0;
  }

  
  if (mState != kStarted) {
    LOG(("DeliverFrame: video not started"));
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

  VideoSegment segment;
  segment.AppendFrame(image.forget(), 1, gfxIntSize(mWidth, mHeight));
  mSource->AppendToTrack(mTrackID, &(segment));
  return 0;
}

void
MediaEngineWebRTCVideoSource::ChooseCapability(uint32_t aWidth, uint32_t aHeight, uint32_t aMinFPS)
{
  int num = mViECapture->NumberOfCapabilities(mUniqueId, KMaxUniqueIdLength);

  NS_WARN_IF_FALSE(!mCapabilityChosen,"Shouldn't select capability of a device twice");

  if (num <= 0) {
    
    mCapability.width  = mOpts.mWidth  = aWidth;
    mCapability.height = mOpts.mHeight = aHeight;
    mCapability.maxFPS = mOpts.mMaxFPS = DEFAULT_VIDEO_FPS;
    mOpts.codecType = kVideoCodecI420;

    
    mCapabilityChosen = true;
    return;
  }

  
  
  
  webrtc::CaptureCapability cap;
  bool higher = true;
  for (int i = 0; i < num; i++) {
    mViECapture->GetCaptureCapability(mUniqueId, KMaxUniqueIdLength, i, cap);
    if (higher) {
      if (i == 0 ||
          (mOpts.mWidth > cap.width && mOpts.mHeight > cap.height)) {
        mOpts.mWidth = cap.width;
        mOpts.mHeight = cap.height;
        mOpts.mMaxFPS = cap.maxFPS;
        mCapability = cap;
        
      }
      if (cap.width <= aWidth && cap.height <= aHeight) {
        higher = false;
      }
    } else {
      if (cap.width > aWidth || cap.height > aHeight || cap.maxFPS < aMinFPS) {
        continue;
      }
      if (mOpts.mWidth < cap.width && mOpts.mHeight < cap.height) {
        mOpts.mWidth = cap.width;
        mOpts.mHeight = cap.height;
        mOpts.mMaxFPS = cap.maxFPS;
        mCapability = cap;
        
      }
    }
  }
  mCapabilityChosen = true;
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
MediaEngineWebRTCVideoSource::Allocate()
{
  if (mState != kReleased) {
    return NS_ERROR_FAILURE;
  }

  if (!mCapabilityChosen) {
    
    ChooseCapability(mWidth, mHeight, mMinFps);
  }

  if (mViECapture->AllocateCaptureDevice(mUniqueId, KMaxUniqueIdLength, mCaptureIndex)) {
    return NS_ERROR_FAILURE;
  }

  mState = kAllocated;
  return NS_OK;
}

nsresult
MediaEngineWebRTCVideoSource::Deallocate()
{
  if (mState != kStopped && mState != kAllocated) {
    return NS_ERROR_FAILURE;
  }

  mViECapture->ReleaseCaptureDevice(mCaptureIndex);
  mState = kReleased;
  return NS_OK;
}

const MediaEngineVideoOptions*
MediaEngineWebRTCVideoSource::GetOptions()
{
  if (!mCapabilityChosen) {
    ChooseCapability(mWidth, mHeight, mMinFps);
  }
  return &mOpts;
}

nsresult
MediaEngineWebRTCVideoSource::Start(SourceMediaStream* aStream, TrackID aID)
{
  int error = 0;
  if (!mInitDone || mState != kAllocated) {
    return NS_ERROR_FAILURE;
  }

  if (!aStream) {
    return NS_ERROR_FAILURE;
  }

  if (mState == kStarted) {
    return NS_OK;
  }

  mSource = aStream;
  mTrackID = aID;

  mImageContainer = layers::LayerManager::CreateImageContainer();
  mSource->AddTrack(aID, mFps, 0, new VideoSegment());
  mSource->AdvanceKnownTracksTime(STREAM_TIME_MAX);

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

  mState = kStarted;
  return NS_OK;
}

nsresult
MediaEngineWebRTCVideoSource::Stop()
{
  if (mState != kStarted) {
    return NS_ERROR_FAILURE;
  }

  mSource->EndTrack(mTrackID);
  mSource->Finish();

  mViERender->StopRender(mCaptureIndex);
  mViERender->RemoveRenderer(mCaptureIndex);
  mViECapture->StopCapture(mCaptureIndex);

  mState = kStopped;
  return NS_OK;
}

nsresult
MediaEngineWebRTCVideoSource::Snapshot(uint32_t aDuration, nsIDOMFile** aFile)
{
  















  *aFile = nullptr;
  if (!mInitDone || mState != kAllocated) {
    return NS_ERROR_FAILURE;
  }

  mSnapshotLock = PR_NewLock();
  mSnapshotCondVar = PR_NewCondVar(mSnapshotLock);

  PR_Lock(mSnapshotLock);
  mInSnapshotMode = true;

  
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

  
  
  
  while (mInSnapshotMode) {
    PR_WaitCondVar(mSnapshotCondVar, PR_INTERVAL_NO_TIMEOUT);
  }

  
  PR_Unlock(mSnapshotLock);
  PR_DestroyCondVar(mSnapshotCondVar);
  PR_DestroyLock(mSnapshotLock);

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
  bool continueShutdown = false;

  if (!mInitDone) {
    return;
  }

  if (mState == kStarted) {
    mViERender->StopRender(mCaptureIndex);
    mViERender->RemoveRenderer(mCaptureIndex);
    continueShutdown = true;
  }

  if (mState == kAllocated || continueShutdown) {
    mViECapture->StopCapture(mCaptureIndex);
    mViECapture->ReleaseCaptureDevice(mCaptureIndex);
    continueShutdown = false;
  }

  mViECapture->Release();
  mViERender->Release();
  mViEBase->Release();
  mState = kReleased;
  mInitDone = false;
}

}
