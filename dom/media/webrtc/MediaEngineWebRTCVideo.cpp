



#include "MediaEngineWebRTC.h"
#include "Layers.h"
#include "ImageTypes.h"
#include "ImageContainer.h"
#include "mozilla/layers/GrallocTextureClient.h"
#include "nsMemory.h"
#include "mtransport/runnable_utils.h"
#include "MediaTrackConstraints.h"

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





NS_IMPL_ISUPPORTS0(MediaEngineWebRTCVideoSource)

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
   unsigned char* buffer, int size, uint32_t time_stamp,
   int64_t ntp_time_ms, int64_t render_time, void *handle)
{
  
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
  LOGFRAME(("frame %d (%dx%d); timestamp %u, ntp_time %lu, render_time %lu", frame_num++,
            mWidth, mHeight, time_stamp, ntp_time_ms, render_time));
#endif

  
  
  MonitorAutoLock lock(mMonitor);

  
  mImage = image.forget();

  
  
  
  

  
  
  
  
  uint32_t len = mSources.Length();
  for (uint32_t i = 0; i < len; i++) {
    if (mSources[i]) {
      AppendToTrack(mSources[i], mImage, mTrackID, 1); 
    }
  }

  return 0;
}




void
MediaEngineWebRTCVideoSource::NotifyPull(MediaStreamGraph* aGraph,
                                         SourceMediaStream* aSource,
                                         TrackID aID,
                                         StreamTime aDesiredTime)
{
  VideoSegment segment;

  MonitorAutoLock lock(mMonitor);
  
  
  

  StreamTime delta = aDesiredTime - aSource->GetEndOfAppendedData(aID);
  LOGFRAME(("NotifyPull, desired = %ld, delta = %ld %s", (int64_t) aDesiredTime,
            (int64_t) delta, mImage.get() ? "" : "<null>"));

  
  
  
  
  
  
  

  
  
  if (delta > 0) {
    
    AppendToTrack(aSource, mImage, aID, delta);
  }
}


bool
MediaEngineWebRTCVideoSource::SatisfiesConstraintSet(const MediaTrackConstraintSet &aConstraints,
                                                     const webrtc::CaptureCapability& aCandidate) {
  if (!MediaEngineCameraVideoSource::IsWithin(aCandidate.width, aConstraints.mWidth) ||
      !MediaEngineCameraVideoSource::IsWithin(aCandidate.height, aConstraints.mHeight)) {
    return false;
  }
  if (!MediaEngineCameraVideoSource::IsWithin(aCandidate.maxFPS, aConstraints.mFrameRate)) {
    return false;
  }
  return true;
}

typedef nsTArray<uint8_t> CapabilitySet;




bool
MediaEngineWebRTCVideoSource::SatisfiesConstraintSets(
    const nsTArray<const MediaTrackConstraintSet*>& aConstraintSets)
{
  NS_ConvertUTF16toUTF8 uniqueId(mUniqueId);
  int num = mViECapture->NumberOfCapabilities(uniqueId.get(), kMaxUniqueIdLength);
  if (num <= 0) {
    return true;
  }

  CapabilitySet candidateSet;
  for (int i = 0; i < num; i++) {
    candidateSet.AppendElement(i);
  }

  for (size_t j = 0; j < aConstraintSets.Length(); j++) {
    for (size_t i = 0; i < candidateSet.Length();  ) {
      webrtc::CaptureCapability cap;
      mViECapture->GetCaptureCapability(uniqueId.get(), kMaxUniqueIdLength,
                                        candidateSet[i], cap);
      if (!SatisfiesConstraintSet(*aConstraintSets[j], cap)) {
        candidateSet.RemoveElementAt(i);
      } else {
        ++i;
      }
    }
  }
  return !!candidateSet.Length();
}

void
MediaEngineWebRTCVideoSource::ChooseCapability(
    const VideoTrackConstraintsN &aConstraints,
    const MediaEnginePrefs &aPrefs)
{
  NS_ConvertUTF16toUTF8 uniqueId(mUniqueId);
  int num = mViECapture->NumberOfCapabilities(uniqueId.get(), kMaxUniqueIdLength);
  if (num <= 0) {
    
    return GuessCapability(aConstraints, aPrefs);
  }

  

  LOG(("ChooseCapability: prefs: %dx%d @%d-%dfps",
       aPrefs.mWidth, aPrefs.mHeight, aPrefs.mFPS, aPrefs.mMinFPS));

  CapabilitySet candidateSet;
  for (int i = 0; i < num; i++) {
    candidateSet.AppendElement(i);
  }

  

  for (uint32_t i = 0; i < candidateSet.Length();) {
    webrtc::CaptureCapability cap;
    mViECapture->GetCaptureCapability(uniqueId.get(), kMaxUniqueIdLength,
                                      candidateSet[i], cap);
    if (!SatisfiesConstraintSet(aConstraints.mRequired, cap)) {
      candidateSet.RemoveElementAt(i);
    } else {
      ++i;
    }
  }

  CapabilitySet tailSet;

  

  if (aConstraints.mAdvanced.WasPassed()) {
    auto &array = aConstraints.mAdvanced.Value();

    for (uint32_t i = 0; i < array.Length(); i++) {
      CapabilitySet rejects;
      for (uint32_t j = 0; j < candidateSet.Length();) {
        webrtc::CaptureCapability cap;
        mViECapture->GetCaptureCapability(uniqueId.get(), kMaxUniqueIdLength,
                                          candidateSet[j], cap);
        if (!SatisfiesConstraintSet(array[i], cap)) {
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
}

nsresult
MediaEngineWebRTCVideoSource::Allocate(const VideoTrackConstraintsN &aConstraints,
                                       const MediaEnginePrefs &aPrefs)
{
  LOG((__FUNCTION__));
  if (mState == kReleased && mInitDone) {
    
    

    ChooseCapability(aConstraints, aPrefs);

    if (mViECapture->AllocateCaptureDevice(NS_ConvertUTF16toUTF8(mUniqueId).get(),
                                           kMaxUniqueIdLength, mCaptureIndex)) {
      return NS_ERROR_FAILURE;
    }
    mState = kAllocated;
    LOG(("Video device %d allocated", mCaptureIndex));
  } else {
#ifdef PR_LOGGING
    MonitorAutoLock lock(mMonitor);
    if (mSources.IsEmpty()) {
      LOG(("Video device %d reallocated", mCaptureIndex));
    } else {
      LOG(("Video device %d allocated shared", mCaptureIndex));
    }
#endif
  }

  return NS_OK;
}

nsresult
MediaEngineWebRTCVideoSource::Deallocate()
{
  LOG((__FUNCTION__));
  bool empty;
  {
    MonitorAutoLock lock(mMonitor);
    empty = mSources.IsEmpty();
  }
  if (empty) {
    
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

  {
    MonitorAutoLock lock(mMonitor);
    mSources.AppendElement(aStream);
  }

  aStream->AddTrack(aID, 0, new VideoSegment(), SourceMediaStream::ADDTRACK_QUEUED);

  if (mState == kStarted) {
    return NS_OK;
  }
  mImageContainer = layers::LayerManager::CreateImageContainer();

  mState = kStarted;
  mTrackID = aID;

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
  {
    MonitorAutoLock lock(mMonitor);

    if (!mSources.RemoveElement(aSource)) {
      
      return NS_OK;
    }

    aSource->EndTrack(aID);

    if (!mSources.IsEmpty()) {
      return NS_OK;
    }
    if (mState != kStarted) {
      return NS_ERROR_FAILURE;
    }

    mState = kStopped;
    
    
    mImage = nullptr;
  }
  mViERender->StopRender(mCaptureIndex);
  mViERender->RemoveRenderer(mCaptureIndex);
  mViECapture->StopCapture(mCaptureIndex);

  return NS_OK;
}

void
MediaEngineWebRTCVideoSource::Init()
{
  
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
    SourceMediaStream *source;
    bool empty;

    while (1) {
      {
        MonitorAutoLock lock(mMonitor);
        empty = mSources.IsEmpty();
        if (empty) {
          break;
        }
        source = mSources[0];
      }
      Stop(source, kVideoTrack); 
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

void MediaEngineWebRTCVideoSource::Refresh(int aIndex) {
  
  
  
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
}

} 
