





#include "MP4Reader.h"
#include "MediaResource.h"
#include "nsSize.h"
#include "VideoUtils.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "ImageContainer.h"
#include "Layers.h"
#include "SharedThreadPool.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/TimeRanges.h"

#ifdef MOZ_EME
#include "mozilla/CDMProxy.h"
#endif

using mozilla::layers::Image;
using mozilla::layers::LayerManager;
using mozilla::layers::LayersBackend;

#ifdef PR_LOGGING
PRLogModuleInfo* GetDemuxerLog() {
  static PRLogModuleInfo* log = nullptr;
  if (!log) {
    log = PR_NewLogModule("MP4Demuxer");
  }
  return log;
}
#define LOG(...) PR_LOG(GetDemuxerLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define LOG(...)
#endif

using namespace mp4_demuxer;

namespace mozilla {




#ifdef PR_LOGGING
static const char*
TrackTypeToStr(TrackType aTrack)
{
  MOZ_ASSERT(aTrack == kAudio || aTrack == kVideo);
  switch (aTrack) {
  case kAudio:
    return "Audio";
  case kVideo:
    return "Video";
  default:
    return "Unknown";
  }
}
#endif

class MP4Stream : public Stream {
public:

  MP4Stream(MediaResource* aResource)
    : mResource(aResource)
  {
    MOZ_COUNT_CTOR(MP4Stream);
    MOZ_ASSERT(aResource);
  }
  virtual ~MP4Stream() {
    MOZ_COUNT_DTOR(MP4Stream);
  }

  virtual bool ReadAt(int64_t aOffset, void* aBuffer, size_t aCount,
                      size_t* aBytesRead) MOZ_OVERRIDE
  {
    uint32_t sum = 0;
    uint32_t bytesRead = 0;
    do {
      uint64_t offset = aOffset + sum;
      char* buffer = reinterpret_cast<char*>(aBuffer) + sum;
      uint32_t toRead = aCount - sum;
      nsresult rv = mResource->ReadAt(offset, buffer, toRead, &bytesRead);
      if (NS_FAILED(rv)) {
        return false;
      }
      sum += bytesRead;
    } while (sum < aCount && bytesRead > 0);
    *aBytesRead = sum;
    return true;
  }

  virtual bool Length(int64_t* aSize) MOZ_OVERRIDE
  {
    if (mResource->GetLength() < 0)
      return false;
    *aSize = mResource->GetLength();
    return true;
  }

private:
  RefPtr<MediaResource> mResource;
};

MP4Reader::MP4Reader(AbstractMediaDecoder* aDecoder)
  : MediaDecoderReader(aDecoder)
  , mAudio("MP4 audio decoder data", Preferences::GetUint("media.mp4-audio-decode-ahead", 2))
  , mVideo("MP4 video decoder data", Preferences::GetUint("media.mp4-video-decode-ahead", 2))
  , mLastReportedNumDecodedFrames(0)
  , mLayersBackendType(layers::LayersBackend::LAYERS_NONE)
  , mDemuxerInitialized(false)
  , mIsEncrypted(false)
  , mIndexReady(false)
  , mIndexMonitor("MP4 index")
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
  MOZ_COUNT_CTOR(MP4Reader);
}

MP4Reader::~MP4Reader()
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
  MOZ_COUNT_DTOR(MP4Reader);
  Shutdown();
}

void
MP4Reader::Shutdown()
{
  if (mAudio.mDecoder) {
    Flush(kAudio);
    mAudio.mDecoder->Shutdown();
    mAudio.mDecoder = nullptr;
  }
  if (mAudio.mTaskQueue) {
    mAudio.mTaskQueue->Shutdown();
    mAudio.mTaskQueue = nullptr;
  }
  if (mVideo.mDecoder) {
    Flush(kVideo);
    mVideo.mDecoder->Shutdown();
    mVideo.mDecoder = nullptr;
  }
  if (mVideo.mTaskQueue) {
    mVideo.mTaskQueue->Shutdown();
    mVideo.mTaskQueue = nullptr;
  }
  
  mQueuedVideoSample = nullptr;

  if (mPlatform) {
    mPlatform->Shutdown();
    mPlatform = nullptr;
  }
}

void
MP4Reader::InitLayersBackendType()
{
  if (!IsVideoContentType(mDecoder->GetResource()->GetContentType())) {
    
    return;
  }
  
  
  
  MediaDecoderOwner* owner = mDecoder->GetOwner();
  if (!owner) {
    NS_WARNING("MP4Reader without a decoder owner, can't get HWAccel");
    return;
  }

  dom::HTMLMediaElement* element = owner->GetMediaElement();
  NS_ENSURE_TRUE_VOID(element);

  nsRefPtr<LayerManager> layerManager =
    nsContentUtils::LayerManagerForDocument(element->OwnerDoc());
  NS_ENSURE_TRUE_VOID(layerManager);

  mLayersBackendType = layerManager->GetCompositorBackendType();
}

static bool sIsEMEEnabled = false;

nsresult
MP4Reader::Init(MediaDecoderReader* aCloneDonor)
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
  PlatformDecoderModule::Init();
  mDemuxer = new MP4Demuxer(new MP4Stream(mDecoder->GetResource()));

  InitLayersBackendType();

  mAudio.mTaskQueue = new MediaTaskQueue(
    SharedThreadPool::Get(NS_LITERAL_CSTRING("MP4 Audio Decode")));
  NS_ENSURE_TRUE(mAudio.mTaskQueue, NS_ERROR_FAILURE);

  mVideo.mTaskQueue = new MediaTaskQueue(
    SharedThreadPool::Get(NS_LITERAL_CSTRING("MP4 Video Decode")));
  NS_ENSURE_TRUE(mVideo.mTaskQueue, NS_ERROR_FAILURE);

  static bool sSetupPrefCache = false;
  if (!sSetupPrefCache) {
    sSetupPrefCache = true;
    Preferences::AddBoolVarCache(&sIsEMEEnabled, "media.eme.enabled", false);
  }

  return NS_OK;
}

#ifdef MOZ_EME
class DispatchKeyNeededEvent : public nsRunnable {
public:
  DispatchKeyNeededEvent(AbstractMediaDecoder* aDecoder,
                         nsTArray<uint8_t>& aInitData,
                         const nsString& aInitDataType)
    : mDecoder(aDecoder)
    , mInitData(aInitData)
    , mInitDataType(aInitDataType)
  {
  }
  NS_IMETHOD Run() {
    
    
    MediaDecoderOwner* owner = mDecoder->GetOwner();
    if (owner) {
      owner->DispatchEncrypted(mInitData, mInitDataType);
    }
    mDecoder = nullptr;
    return NS_OK;
  }
private:
  nsRefPtr<AbstractMediaDecoder> mDecoder;
  nsTArray<uint8_t> mInitData;
  nsString mInitDataType;
};
#endif

bool MP4Reader::IsWaitingOnCodecResource() {
#ifdef MOZ_GONK_MEDIACODEC
  return mVideo.mDecoder && mVideo.mDecoder->IsWaitingMediaResources();
#endif
  return false;
}

bool MP4Reader::IsWaitingOnCDMResource() {
#ifdef MOZ_EME
  nsRefPtr<CDMProxy> proxy;
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    if (!mIsEncrypted) {
      
      return false;
    }
    proxy = mDecoder->GetCDMProxy();
    if (!proxy) {
      
      return true;
    }
  }
  
  {
    CDMCaps::AutoLock caps(proxy->Capabilites());
    LOG("MP4Reader::IsWaitingMediaResources() capsKnown=%d", caps.AreCapsKnown());
    return !caps.AreCapsKnown();
  }
#else
  return false;
#endif
}

bool MP4Reader::IsWaitingMediaResources()
{
  
  
  
  
  return IsWaitingOnCDMResource() || IsWaitingOnCodecResource();
}

void
MP4Reader::ExtractCryptoInitData(nsTArray<uint8_t>& aInitData)
{
  MOZ_ASSERT(mDemuxer->Crypto().valid);
  const nsTArray<mp4_demuxer::PsshInfo>& psshs = mDemuxer->Crypto().pssh;
  for (uint32_t i = 0; i < psshs.Length(); i++) {
    aInitData.AppendElements(psshs[i].data);
  }
}

bool
MP4Reader::IsSupportedAudioMimeType(const char* aMimeType)
{
  return (!strcmp(aMimeType, "audio/mpeg") ||
          !strcmp(aMimeType, "audio/mp4a-latm")) &&
         mPlatform->SupportsAudioMimeType(aMimeType);
}

nsresult
MP4Reader::ReadMetadata(MediaInfo* aInfo,
                        MetadataTags** aTags)
{
  if (!mDemuxerInitialized) {
    bool ok = mDemuxer->Init();
    NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);

    {
      MonitorAutoLock mon(mIndexMonitor);
      mIndexReady = true;
    }

    mInfo.mVideo.mHasVideo = mVideo.mActive = mDemuxer->HasValidVideo();
    const VideoDecoderConfig& video = mDemuxer->VideoConfig();
    
    if (mInfo.mVideo.mHasVideo && strcmp(video.mime_type, "video/avc")) {
      return NS_ERROR_FAILURE;
    }

    mInfo.mAudio.mHasAudio = mAudio.mActive = mDemuxer->HasValidAudio();

    {
      ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
      mIsEncrypted = mDemuxer->Crypto().valid;
    }

    
    
    
    mDemuxerInitialized = true;
  } else if (mPlatform && !IsWaitingMediaResources()) {
    *aInfo = mInfo;
    *aTags = nullptr;
    return NS_OK;
  }

  if (mDemuxer->Crypto().valid) {
#ifdef MOZ_EME
    if (!sIsEMEEnabled) {
      
      return NS_ERROR_FAILURE;
    }

    
    
    
    nsRefPtr<CDMProxy> proxy;
    nsTArray<uint8_t> initData;
    ExtractCryptoInitData(initData);
    if (initData.Length() == 0) {
      return NS_ERROR_FAILURE;
    }
    if (!mInitDataEncountered.Contains(initData)) {
      mInitDataEncountered.AppendElement(initData);
      NS_DispatchToMainThread(new DispatchKeyNeededEvent(mDecoder, initData, NS_LITERAL_STRING("cenc")));
    }
    if (IsWaitingMediaResources()) {
      return NS_OK;
    }
    MOZ_ASSERT(!IsWaitingMediaResources());

    {
      ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
      proxy = mDecoder->GetCDMProxy();
    }
    MOZ_ASSERT(proxy);

    mPlatform = PlatformDecoderModule::CreateCDMWrapper(proxy,
                                                        HasAudio(),
                                                        HasVideo(),
                                                        GetTaskQueue());
    NS_ENSURE_TRUE(mPlatform, NS_ERROR_FAILURE);
#else
    
    return NS_ERROR_FAILURE;
#endif
  } else {
    mPlatform = PlatformDecoderModule::Create();
    NS_ENSURE_TRUE(mPlatform, NS_ERROR_FAILURE);
  }

  if (HasAudio()) {
    const AudioDecoderConfig& audio = mDemuxer->AudioConfig();
    if (mInfo.mAudio.mHasAudio && !IsSupportedAudioMimeType(audio.mime_type)) {
      return NS_ERROR_FAILURE;
    }
    mInfo.mAudio.mRate = audio.samples_per_second;
    mInfo.mAudio.mChannels = audio.channel_count;
    mAudio.mCallback = new DecoderCallback(this, kAudio);
    mAudio.mDecoder = mPlatform->CreateAudioDecoder(audio,
                                                    mAudio.mTaskQueue,
                                                    mAudio.mCallback);
    NS_ENSURE_TRUE(mAudio.mDecoder != nullptr, NS_ERROR_FAILURE);
    nsresult rv = mAudio.mDecoder->Init();
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    Decode(kAudio);
  }

  if (HasVideo()) {
    const VideoDecoderConfig& video = mDemuxer->VideoConfig();
    mInfo.mVideo.mDisplay =
      nsIntSize(video.display_width, video.display_height);
    mVideo.mCallback = new  DecoderCallback(this, kVideo);
    mVideo.mDecoder = mPlatform->CreateH264Decoder(video,
                                                   mLayersBackendType,
                                                   mDecoder->GetImageContainer(),
                                                   mVideo.mTaskQueue,
                                                   mVideo.mCallback);
    NS_ENSURE_TRUE(mVideo.mDecoder != nullptr, NS_ERROR_FAILURE);
    nsresult rv = mVideo.mDecoder->Init();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  Microseconds duration = mDemuxer->Duration();
  if (duration != -1) {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mDecoder->SetMediaDuration(duration);
  }

  *aInfo = mInfo;
  *aTags = nullptr;

  UpdateIndex();

  return NS_OK;
}

bool
MP4Reader::IsMediaSeekable()
{
  
  
  return mDecoder->GetResource()->IsTransportSeekable() && mDemuxer->CanSeek();
}

bool
MP4Reader::HasAudio()
{
  return mAudio.mActive;
}

bool
MP4Reader::HasVideo()
{
  return mVideo.mActive;
}

MP4Reader::DecoderData&
MP4Reader::GetDecoderData(TrackType aTrack)
{
  MOZ_ASSERT(aTrack == kAudio || aTrack == kVideo);
  return (aTrack == kAudio) ? mAudio : mVideo;
}

MediaDataDecoder*
MP4Reader::Decoder(TrackType aTrack)
{
  return GetDecoderData(aTrack).mDecoder;
}

MP4Sample*
MP4Reader::PopSample(TrackType aTrack)
{
  switch (aTrack) {
    case kAudio:
      return mDemuxer->DemuxAudioSample();

    case kVideo:
      if (mQueuedVideoSample) {
        return mQueuedVideoSample.forget();
      }
      return mDemuxer->DemuxVideoSample();

    default:
      return nullptr;
  }
}




















bool
MP4Reader::Decode(TrackType aTrack)
{
  DecoderData& data = GetDecoderData(aTrack);
  MOZ_ASSERT(data.mDecoder);

  data.mMonitor.Lock();
  uint64_t prevNumFramesOutput = data.mNumSamplesOutput;
  while (prevNumFramesOutput == data.mNumSamplesOutput) {
    data.mMonitor.AssertCurrentThreadOwns();
    if (data.mError) {
      
      data.mMonitor.Unlock();
      return false;
    }
    
    
    
    
    
    
    while (prevNumFramesOutput == data.mNumSamplesOutput &&
           (data.mInputExhausted ||
           (data.mNumSamplesInput - data.mNumSamplesOutput) < data.mDecodeAhead) &&
           !data.mEOS) {
      data.mMonitor.AssertCurrentThreadOwns();
      data.mMonitor.Unlock();
      nsAutoPtr<MP4Sample> compressed(PopSample(aTrack));
      if (!compressed) {
        
        LOG("Draining %s", TrackTypeToStr(aTrack));
        data.mMonitor.Lock();
        MOZ_ASSERT(!data.mEOS);
        data.mEOS = true;
        MOZ_ASSERT(!data.mDrainComplete);
        data.mDrainComplete = false;
        data.mMonitor.Unlock();
        data.mDecoder->Drain();
      } else {
#ifdef LOG_SAMPLE_DECODE
        LOG("PopSample %s time=%lld dur=%lld", TrackTypeToStr(aTrack),
            compressed->composition_timestamp, compressed->duration);
#endif
        data.mMonitor.Lock();
        data.mDrainComplete = false;
        data.mInputExhausted = false;
        data.mNumSamplesInput++;
        data.mMonitor.Unlock();

        if (NS_FAILED(data.mDecoder->Input(compressed))) {
          return false;
        }
        
        
        
        compressed.forget();
      }
      data.mMonitor.Lock();
    }
    data.mMonitor.AssertCurrentThreadOwns();
    while (!data.mError &&
           prevNumFramesOutput == data.mNumSamplesOutput &&
           (!data.mInputExhausted || data.mEOS) &&
           !data.mDrainComplete) {
      data.mMonitor.Wait();
    }
    if (data.mError ||
        (data.mEOS && data.mDrainComplete)) {
      break;
    }
  }
  data.mMonitor.AssertCurrentThreadOwns();
  bool rv = !(data.mDrainComplete || data.mError);
  data.mMonitor.Unlock();
  return rv;
}

nsresult
MP4Reader::ResetDecode()
{
  Flush(kAudio);
  Flush(kVideo);
  return MediaDecoderReader::ResetDecode();
}

void
MP4Reader::Output(TrackType aTrack, MediaData* aSample)
{
#ifdef LOG_SAMPLE_DECODE
  LOG("Decoded %s sample time=%lld dur=%lld",
      TrackTypeToStr(aTrack), aSample->mTime, aSample->mDuration);
#endif

  DecoderData& data = GetDecoderData(aTrack);
  
  MonitorAutoLock mon(data.mMonitor);
  if (data.mIsFlushing) {
    delete aSample;
    LOG("MP4Reader produced output while flushing, discarding.");
    mon.NotifyAll();
    return;
  }

  switch (aTrack) {
    case kAudio: {
      MOZ_ASSERT(aSample->mType == MediaData::AUDIO_SAMPLES);
      AudioData* audioData = static_cast<AudioData*>(aSample);
      AudioQueue().Push(audioData);
      if (audioData->mChannels != mInfo.mAudio.mChannels ||
          audioData->mRate != mInfo.mAudio.mRate) {
        LOG("MP4Reader::Output change of sampling rate:%d->%d",
            mInfo.mAudio.mRate, audioData->mRate);
        mInfo.mAudio.mRate = audioData->mRate;
        mInfo.mAudio.mChannels = audioData->mChannels;
      }
      break;
    }
    case kVideo: {
      MOZ_ASSERT(aSample->mType == MediaData::VIDEO_FRAME);
      VideoQueue().Push(static_cast<VideoData*>(aSample));
      break;
    }
    default:
      break;
  }

  data.mNumSamplesOutput++;
  mon.NotifyAll();
}

void
MP4Reader::DrainComplete(TrackType aTrack)
{
  DecoderData& data = GetDecoderData(aTrack);
  MonitorAutoLock mon(data.mMonitor);
  data.mDrainComplete = true;
  mon.NotifyAll();
}

void
MP4Reader::InputExhausted(TrackType aTrack)
{
  DecoderData& data = GetDecoderData(aTrack);
  MonitorAutoLock mon(data.mMonitor);
  data.mInputExhausted = true;
  mon.NotifyAll();
}

void
MP4Reader::Error(TrackType aTrack)
{
  DecoderData& data = GetDecoderData(aTrack);
  MonitorAutoLock mon(data.mMonitor);
  data.mError = true;
  mon.NotifyAll();
}

bool
MP4Reader::DecodeAudioData()
{
  MOZ_ASSERT(HasAudio() && mPlatform && mAudio.mDecoder);
  return Decode(kAudio);
}

void
MP4Reader::Flush(TrackType aTrack)
{
  DecoderData& data = GetDecoderData(aTrack);
  if (!data.mDecoder) {
    return;
  }
  
  
  
  {
    MonitorAutoLock mon(data.mMonitor);
    data.mIsFlushing = true;
    data.mDrainComplete = false;
    data.mEOS = false;
  }
  data.mDecoder->Flush();
  {
    MonitorAutoLock mon(data.mMonitor);
    data.mIsFlushing = false;
  }
}

bool
MP4Reader::SkipVideoDemuxToNextKeyFrame(int64_t aTimeThreshold, uint32_t& parsed)
{
  MOZ_ASSERT(mVideo.mDecoder);

  Flush(kVideo);

  
  while (true) {
    nsAutoPtr<MP4Sample> compressed(PopSample(kVideo));
    if (!compressed) {
      
      return false;
    }
    parsed++;
    if (!compressed->is_sync_point ||
        compressed->composition_timestamp < aTimeThreshold) {
      continue;
    }
    mQueuedVideoSample = compressed;
    break;
  }

  return true;
}

bool
MP4Reader::DecodeVideoFrame(bool &aKeyframeSkip,
                            int64_t aTimeThreshold)
{
  
  
  uint32_t parsed = 0, decoded = 0;
  AbstractMediaDecoder::AutoNotifyDecoded autoNotify(mDecoder, parsed, decoded);

  MOZ_ASSERT(HasVideo() && mPlatform && mVideo.mDecoder);

  if (aKeyframeSkip) {
    bool ok = SkipVideoDemuxToNextKeyFrame(aTimeThreshold, parsed);
    if (!ok) {
      NS_WARNING("Failed to skip demux up to next keyframe");
      return false;
    }
    aKeyframeSkip = false;
    nsresult rv = mVideo.mDecoder->Flush();
    NS_ENSURE_SUCCESS(rv, false);
  }

  bool rv = Decode(kVideo);
  {
    
    
    MonitorAutoLock mon(mVideo.mMonitor);
    uint64_t delta = mVideo.mNumSamplesOutput - mLastReportedNumDecodedFrames;
    decoded = static_cast<uint32_t>(delta);
    mLastReportedNumDecodedFrames = mVideo.mNumSamplesOutput;
  }
  return rv;
}

nsresult
MP4Reader::Seek(int64_t aTime,
                int64_t aStartTime,
                int64_t aEndTime,
                int64_t aCurrentTime)
{
  if (!mDecoder->GetResource()->IsTransportSeekable() || !mDemuxer->CanSeek()) {
    return NS_ERROR_FAILURE;
  }

  mQueuedVideoSample = nullptr;
  if (mDemuxer->HasValidVideo()) {
    mDemuxer->SeekVideo(aTime);
    mQueuedVideoSample = PopSample(kVideo);
  }
  if (mDemuxer->HasValidAudio()) {
    mDemuxer->SeekAudio(
      mQueuedVideoSample ? mQueuedVideoSample->composition_timestamp : aTime);
  }

  return NS_OK;
}

void
MP4Reader::NotifyDataArrived(const char* aBuffer, uint32_t aLength,
                             int64_t aOffset)
{
  if (NS_IsMainThread()) {
    if (GetTaskQueue()) {
      GetTaskQueue()->Dispatch(
        NS_NewRunnableMethod(this, &MP4Reader::UpdateIndex));
    }
  } else {
    UpdateIndex();
  }
}

void
MP4Reader::UpdateIndex()
{
  MonitorAutoLock mon(mIndexMonitor);
  if (!mIndexReady) {
    return;
  }

  MediaResource* resource = mDecoder->GetResource();
  resource->Pin();
  nsTArray<MediaByteRange> ranges;
  if (NS_SUCCEEDED(resource->GetCachedRanges(ranges))) {
    mDemuxer->UpdateIndex(ranges);
  }
  resource->Unpin();
}

int64_t
MP4Reader::GetEvictionOffset(double aTime)
{
  MonitorAutoLock mon(mIndexMonitor);
  if (!mIndexReady) {
    return 0;
  }

  return mDemuxer->GetEvictionOffset(aTime * 1000000.0);
}

nsresult
MP4Reader::GetBuffered(dom::TimeRanges* aBuffered, int64_t aStartTime)
{
  MonitorAutoLock mon(mIndexMonitor);
  if (!mIndexReady) {
    return NS_OK;
  }

  MediaResource* resource = mDecoder->GetResource();
  nsTArray<MediaByteRange> ranges;
  resource->Pin();
  nsresult rv = resource->GetCachedRanges(ranges);
  resource->Unpin();

  if (NS_SUCCEEDED(rv)) {
    nsTArray<Interval<Microseconds>> timeRanges;
    mDemuxer->ConvertByteRangesToTime(ranges, &timeRanges);
    for (size_t i = 0; i < timeRanges.Length(); i++) {
      aBuffered->Add((timeRanges[i].start - aStartTime) / 1000000.0,
                     (timeRanges[i].end - aStartTime) / 1000000.0);
    }
  }

  return NS_OK;
}

bool MP4Reader::IsDormantNeeded()
{
#ifdef MOZ_GONK_MEDIACODEC
  return mVideo.mDecoder && mVideo.mDecoder->IsDormantNeeded();
#endif
  return false;
}

void MP4Reader::ReleaseMediaResources()
{
#ifdef MOZ_GONK_MEDIACODEC
  
  
  VideoFrameContainer* container = mDecoder->GetVideoFrameContainer();
  if (container) {
    container->ClearCurrentFrame();
  }
  if (mVideo.mDecoder) {
    mVideo.mDecoder->ReleaseMediaResources();
  }
#endif
}

void MP4Reader::NotifyResourcesStatusChanged()
{
#ifdef MOZ_GONK_MEDIACODEC
  if (mDecoder) {
    mDecoder->NotifyWaitingForResourcesStatusChanged();
  }
#endif
}

} 
