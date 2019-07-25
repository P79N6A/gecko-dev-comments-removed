



#include "MediaEngineWebRTC.h"
#include "Layers.h"

namespace mozilla {




NS_IMPL_THREADSAFE_ISUPPORTS1(MediaEngineWebRTCVideoSource, nsIRunnable)


const unsigned int MediaEngineWebRTCVideoSource::KMaxDeviceNameLength = 128;
const unsigned int MediaEngineWebRTCVideoSource::KMaxUniqueIdLength = 256;


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
    return 0;
  }

  
  ImageFormat format = PLANAR_YCBCR;
  nsRefPtr<layers::Image> image = mImageContainer->CreateImage(&format, 1);

  layers::PlanarYCbCrImage* videoImage = static_cast<layers::PlanarYCbCrImage*>(image.get());

  PRUint8* frame = static_cast<PRUint8*> (buffer);
  const PRUint8 lumaBpp = 8;
  const PRUint8 chromaBpp = 4;

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
MediaEngineWebRTCVideoSource::GetName(nsAString& aName)
{
  char deviceName[KMaxDeviceNameLength];
  memset(deviceName, 0, KMaxDeviceNameLength);

  char uniqueId[KMaxUniqueIdLength];
  memset(uniqueId, 0, KMaxUniqueIdLength);

  if (mInitDone) {
    mViECapture->GetCaptureDevice(
      mCapIndex, deviceName, KMaxDeviceNameLength, uniqueId, KMaxUniqueIdLength
    );
    aName.Assign(NS_ConvertASCIItoUTF16(deviceName));
  }
}

void
MediaEngineWebRTCVideoSource::GetUUID(nsAString& aUUID)
{
  char deviceName[KMaxDeviceNameLength];
  memset(deviceName, 0, KMaxDeviceNameLength);

  char uniqueId[KMaxUniqueIdLength];
  memset(uniqueId, 0, KMaxUniqueIdLength);

  if (mInitDone) {
    mViECapture->GetCaptureDevice(
      mCapIndex, deviceName, KMaxDeviceNameLength, uniqueId, KMaxUniqueIdLength
    );
    aUUID.Assign(NS_ConvertASCIItoUTF16(uniqueId));
  }
}

nsresult
MediaEngineWebRTCVideoSource::Allocate()
{
  if (mState != kReleased) {
    return NS_ERROR_FAILURE;
  }

  char deviceName[KMaxDeviceNameLength];
  memset(deviceName, 0, KMaxDeviceNameLength);

  char uniqueId[KMaxUniqueIdLength];
  memset(uniqueId, 0, KMaxUniqueIdLength);

  mViECapture->GetCaptureDevice(
    mCapIndex, deviceName, KMaxDeviceNameLength, uniqueId, KMaxUniqueIdLength
  );

  if (mViECapture->AllocateCaptureDevice(uniqueId, KMaxUniqueIdLength, mCapIndex)) {
    return NS_ERROR_FAILURE;
  }

  if (mViECapture->StartCapture(mCapIndex) < 0) {
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

  mViECapture->StopCapture(mCapIndex);
  mViECapture->ReleaseCaptureDevice(mCapIndex);
  mState = kReleased;
  return NS_OK;
}

MediaEngineVideoOptions
MediaEngineWebRTCVideoSource::GetOptions()
{
  MediaEngineVideoOptions aOpts;
  aOpts.mWidth = mWidth;
  aOpts.mHeight = mHeight;
  aOpts.mMaxFPS = mFps;
  aOpts.codecType = kVideoCodecI420;
  return aOpts;
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

  error = mViERender->AddRenderer(mCapIndex, webrtc::kVideoI420, (webrtc::ExternalRenderer*)this);
  if (error == -1) {
    return NS_ERROR_FAILURE;
  }

  error = mViERender->StartRender(mCapIndex);
  if (error == -1) {
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

  mViERender->StopRender(mCapIndex);
  mViERender->RemoveRenderer(mCapIndex);

  mState = kStopped;
  return NS_OK;
}

nsresult
MediaEngineWebRTCVideoSource::Snapshot(PRUint32 aDuration, nsIDOMFile** aFile)
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
  error = mViERender->AddRenderer(mCapIndex, webrtc::kVideoI420, (webrtc::ExternalRenderer*)this);
  if (error == -1) {
    return NS_ERROR_FAILURE;
  }
  error = mViERender->StartRender(mCapIndex);
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
  if (vieFile->GetCaptureDeviceSnapshot(mCapIndex, path.get()) < 0) {
    delete mSnapshotPath;
    mSnapshotPath = NULL;
    return NS_ERROR_FAILURE;
  }

  
  mViERender->StopRender(mCapIndex);
  mViERender->RemoveRenderer(mCapIndex);

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
    mViERender->StopRender(mCapIndex);
    mViERender->RemoveRenderer(mCapIndex);
    continueShutdown = true;
  }

  if (mState == kAllocated || continueShutdown) {
    mViECapture->StopCapture(mCapIndex);
    mViECapture->ReleaseCaptureDevice(mCapIndex);
    continueShutdown = false;
  }

  mViECapture->Release();
  mViERender->Release();
  mViEBase->Release();
  mState = kReleased;
  mInitDone = false;
}

}
