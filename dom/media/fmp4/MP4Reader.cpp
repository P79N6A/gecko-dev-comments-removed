





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
#include "SharedDecoderManager.h"

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
#define VLOG(...) PR_LOG(GetDemuxerLog(), PR_LOG_DEBUG+1, (__VA_ARGS__))
#else
#define LOG(...)
#define VLOG(...)
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

  explicit MP4Stream(MediaResource* aResource)
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

  virtual bool CachedReadAt(int64_t aOffset, void* aBuffer, size_t aCount,
                            size_t* aBytesRead) MOZ_OVERRIDE
  {
    nsresult rv = mResource->ReadFromCache(reinterpret_cast<char*>(aBuffer),
                                           aOffset, aCount);
    if (NS_FAILED(rv)) {
      *aBytesRead = 0;
      return false;
    }
    *aBytesRead = aCount;
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
  MOZ_COUNT_DTOR(MP4Reader);
}

void
MP4Reader::Shutdown()
{
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());

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

  mAudio.mTaskQueue = new MediaTaskQueue(GetMediaDecodeThreadPool());
  NS_ENSURE_TRUE(mAudio.mTaskQueue, NS_ERROR_FAILURE);

  mVideo.mTaskQueue = new MediaTaskQueue(GetMediaDecodeThreadPool());
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

bool
MP4Reader::IsSupportedVideoMimeType(const char* aMimeType)
{
  return (!strcmp(aMimeType, "video/mp4") ||
          !strcmp(aMimeType, "video/avc")) &&
         mPlatform->SupportsVideoMimeType(aMimeType);
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

    
    mInfo.mVideo.mHasVideo = mVideo.mActive = mDemuxer->HasValidVideo() &&
                                              mDecoder->GetImageContainer();
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
  }

  if (HasVideo()) {
    const VideoDecoderConfig& video = mDemuxer->VideoConfig();
    if (mInfo.mVideo.mHasVideo && !IsSupportedVideoMimeType(video.mime_type)) {
      return NS_ERROR_FAILURE;
    }
    mInfo.mVideo.mDisplay =
      nsIntSize(video.display_width, video.display_height);
    mVideo.mCallback = new DecoderCallback(this, kVideo);
    if (mSharedDecoderManager) {
      mVideo.mDecoder =
        mSharedDecoderManager->CreateVideoDecoder(video,
                                                  mLayersBackendType,
                                                  mDecoder->GetImageContainer(),
                                                  mVideo.mTaskQueue,
                                                  mVideo.mCallback);
    } else {
      mVideo.mDecoder = mPlatform->CreateVideoDecoder(video,
                                                      mLayersBackendType,
                                                      mDecoder->GetImageContainer(),
                                                      mVideo.mTaskQueue,
                                                      mVideo.mCallback);
    }
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

void
MP4Reader::ReadUpdatedMetadata(MediaInfo* aInfo)
{
  *aInfo = mInfo;
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

void
MP4Reader::RequestVideoData(bool aSkipToNextKeyframe,
                            int64_t aTimeThreshold)
{
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());
  VLOG("RequestVideoData skip=%d time=%lld", aSkipToNextKeyframe, aTimeThreshold);

  
  
  uint32_t parsed = 0, decoded = 0;
  AbstractMediaDecoder::AutoNotifyDecoded autoNotify(mDecoder, parsed, decoded);

  MOZ_ASSERT(HasVideo() && mPlatform && mVideo.mDecoder);

  if (aSkipToNextKeyframe) {
    if (!SkipVideoDemuxToNextKeyFrame(aTimeThreshold, parsed) ||
        NS_FAILED(mVideo.mDecoder->Flush())) {
      NS_WARNING("Failed to skip/flush video when skipping-to-next-keyframe.");
    }
  }

  auto& decoder = GetDecoderData(kVideo);
  MonitorAutoLock lock(decoder.mMonitor);
  decoder.mOutputRequested = true;
  ScheduleUpdate(kVideo);

  
  
  uint64_t delta = mVideo.mNumSamplesOutput - mLastReportedNumDecodedFrames;
  decoded = static_cast<uint32_t>(delta);
  mLastReportedNumDecodedFrames = mVideo.mNumSamplesOutput;
}

void
MP4Reader::RequestAudioData()
{
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());
  VLOG("RequestAudioData");
  auto& decoder = GetDecoderData(kAudio);
  MonitorAutoLock lock(decoder.mMonitor);
  decoder.mOutputRequested = true;
  ScheduleUpdate(kAudio);
}

void
MP4Reader::ScheduleUpdate(TrackType aTrack)
{
  auto& decoder = GetDecoderData(aTrack);
  decoder.mMonitor.AssertCurrentThreadOwns();
  if (decoder.mUpdateScheduled) {
    return;
  }
  VLOG("SchedulingUpdate(%s)", TrackTypeToStr(aTrack));
  decoder.mUpdateScheduled = true;
  RefPtr<nsIRunnable> task(
    NS_NewRunnableMethodWithArg<TrackType>(this, &MP4Reader::Update, aTrack));
  GetTaskQueue()->Dispatch(task.forget());
}

bool
MP4Reader::NeedInput(DecoderData& aDecoder)
{
  aDecoder.mMonitor.AssertCurrentThreadOwns();
  
  
  
  
  
  return
    !aDecoder.mError &&
    !aDecoder.mEOS &&
    aDecoder.mOutputRequested &&
    aDecoder.mOutput.IsEmpty() &&
    (aDecoder.mInputExhausted ||
     aDecoder.mNumSamplesInput - aDecoder.mNumSamplesOutput < aDecoder.mDecodeAhead);
}

void
MP4Reader::Update(TrackType aTrack)
{
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());

  bool needInput = false;
  bool needOutput = false;
  bool eos = false;
  auto& decoder = GetDecoderData(aTrack);
  nsRefPtr<MediaData> output;
  {
    MonitorAutoLock lock(decoder.mMonitor);
    decoder.mUpdateScheduled = false;
    if (NeedInput(decoder)) {
      needInput = true;
      decoder.mInputExhausted = false;
      decoder.mNumSamplesInput++;
    }
    needOutput = decoder.mOutputRequested;
    if (needOutput && !decoder.mOutput.IsEmpty()) {
      output = decoder.mOutput[0];
      decoder.mOutput.RemoveElementAt(0);
    }
    eos = decoder.mEOS;
  }
  VLOG("Update(%s) ni=%d no=%d iex=%d or=%d fl=%d",
       TrackTypeToStr(aTrack),
       needInput,
       needOutput,
       decoder.mInputExhausted,
       decoder.mOutputRequested,
       decoder.mIsFlushing);

  if (needInput) {
    MP4Sample* sample = PopSample(aTrack);
    if (sample) {
      decoder.mDecoder->Input(sample);
    } else {
      {
        MonitorAutoLock lock(decoder.mMonitor);
        MOZ_ASSERT(!decoder.mEOS);
        eos = decoder.mEOS = true;
      }
      decoder.mDecoder->Drain();
    }
  }
  if (needOutput) {
    if (output) {
      ReturnOutput(output, aTrack);
    } else if (eos) {
      ReturnEOS(aTrack);
    }
  }
}

void
MP4Reader::ReturnOutput(MediaData* aData, TrackType aTrack)
{
  auto& decoder = GetDecoderData(aTrack);
  {
    MonitorAutoLock lock(decoder.mMonitor);
    MOZ_ASSERT(decoder.mOutputRequested);
    decoder.mOutputRequested = false;
    if (decoder.mDiscontinuity) {
      decoder.mDiscontinuity = false;
      aData->mDiscontinuity = true;
    }
  }

  if (aTrack == kAudio) {
    AudioData* audioData = static_cast<AudioData*>(aData);

    if (audioData->mChannels != mInfo.mAudio.mChannels ||
        audioData->mRate != mInfo.mAudio.mRate) {
      LOG("MP4Reader::ReturnOutput change of sampling rate:%d->%d",
          mInfo.mAudio.mRate, audioData->mRate);
      mInfo.mAudio.mRate = audioData->mRate;
      mInfo.mAudio.mChannels = audioData->mChannels;
    }

    GetCallback()->OnAudioDecoded(audioData);
  } else if (aTrack == kVideo) {
    GetCallback()->OnVideoDecoded(static_cast<VideoData*>(aData));
  }
}

void
MP4Reader::ReturnEOS(TrackType aTrack)
{
  GetCallback()->OnNotDecoded(aTrack == kAudio ? MediaData::AUDIO_DATA : MediaData::VIDEO_DATA,
                              RequestSampleCallback::END_OF_STREAM);
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

nsresult
MP4Reader::ResetDecode()
{
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());
  Flush(kVideo);
  Flush(kAudio);
  return MediaDecoderReader::ResetDecode();
}

void
MP4Reader::Output(TrackType aTrack, MediaData* aSample)
{
#ifdef LOG_SAMPLE_DECODE
  VLOG("Decoded %s sample time=%lld dur=%lld",
      TrackTypeToStr(aTrack), aSample->mTime, aSample->mDuration);
#endif

  if (!aSample) {
    NS_WARNING("MP4Reader::Output() passed a null sample");
    Error(aTrack);
    return;
  }

  auto& decoder = GetDecoderData(aTrack);
  
  MonitorAutoLock mon(decoder.mMonitor);
  if (decoder.mIsFlushing) {
    LOG("MP4Reader produced output while flushing, discarding.");
    mon.NotifyAll();
    return;
  }

  decoder.mOutput.AppendElement(aSample);
  decoder.mNumSamplesOutput++;
  if (NeedInput(decoder) || decoder.mOutputRequested) {
    ScheduleUpdate(aTrack);
  }
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
  ScheduleUpdate(aTrack);
}

void
MP4Reader::Error(TrackType aTrack)
{
  DecoderData& data = GetDecoderData(aTrack);
  {
    MonitorAutoLock mon(data.mMonitor);
    data.mError = true;
  }
  GetCallback()->OnNotDecoded(aTrack == kVideo ? MediaData::VIDEO_DATA : MediaData::AUDIO_DATA,
                              RequestSampleCallback::DECODE_ERROR);
}

void
MP4Reader::Flush(TrackType aTrack)
{
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());
  VLOG("Flush(%s) BEGIN", TrackTypeToStr(aTrack));
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
    data.mOutput.Clear();
    data.mNumSamplesInput = 0;
    data.mNumSamplesOutput = 0;
    data.mInputExhausted = false;
    if (data.mOutputRequested) {
      GetCallback()->OnNotDecoded(aTrack == kVideo ? MediaData::VIDEO_DATA : MediaData::AUDIO_DATA,
                                  RequestSampleCallback::CANCELED);
    }
    data.mOutputRequested = false;
    data.mDiscontinuity = true;
  }
  if (aTrack == kVideo) {
    mQueuedVideoSample = nullptr;
  }
  VLOG("Flush(%s) END", TrackTypeToStr(aTrack));
}

bool
MP4Reader::SkipVideoDemuxToNextKeyFrame(int64_t aTimeThreshold, uint32_t& parsed)
{
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());

  MOZ_ASSERT(mVideo.mDecoder);

  Flush(kVideo);

  
  while (true) {
    nsAutoPtr<MP4Sample> compressed(PopSample(kVideo));
    if (!compressed) {
      
      GetCallback()->OnNotDecoded(MediaData::VIDEO_DATA,
                                  RequestSampleCallback::END_OF_STREAM);
      {
        MonitorAutoLock mon(mVideo.mMonitor);
        mVideo.mEOS = true;
      }
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

void
MP4Reader::Seek(int64_t aTime,
                int64_t aStartTime,
                int64_t aEndTime,
                int64_t aCurrentTime)
{
  LOG("MP4Reader::Seek(%lld)", aTime);
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());
  if (!mDecoder->GetResource()->IsTransportSeekable() || !mDemuxer->CanSeek()) {
    VLOG("Seek() END (Unseekable)");
    GetCallback()->OnSeekCompleted(NS_ERROR_FAILURE);
    return;
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
  LOG("MP4Reader::Seek(%lld) exit", aTime);
  GetCallback()->OnSeekCompleted(NS_OK);
}

void
MP4Reader::NotifyDataArrived(const char* aBuffer, uint32_t aLength,
                             int64_t aOffset)
{
  UpdateIndex();
}

void
MP4Reader::UpdateIndex()
{
  MonitorAutoLock mon(mIndexMonitor);
  if (!mIndexReady) {
    return;
  }

  AutoPinned<MediaResource> resource(mDecoder->GetResource());
  nsTArray<MediaByteRange> ranges;
  if (NS_SUCCEEDED(resource->GetCachedRanges(ranges))) {
    mDemuxer->UpdateIndex(ranges);
  }
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
MP4Reader::GetBuffered(dom::TimeRanges* aBuffered)
{
  MonitorAutoLock mon(mIndexMonitor);
  if (!mIndexReady) {
    return NS_OK;
  }
  MOZ_ASSERT(mStartTime != -1, "Need to finish metadata decode first");

  AutoPinned<MediaResource> resource(mDecoder->GetResource());
  nsTArray<MediaByteRange> ranges;
  nsresult rv = resource->GetCachedRanges(ranges);

  if (NS_SUCCEEDED(rv)) {
    nsTArray<Interval<Microseconds>> timeRanges;
    mDemuxer->ConvertByteRangesToTime(ranges, &timeRanges);
    for (size_t i = 0; i < timeRanges.Length(); i++) {
      aBuffered->Add((timeRanges[i].start - mStartTime) / 1000000.0,
                     (timeRanges[i].end - mStartTime) / 1000000.0);
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

void
MP4Reader::SetIdle()
{
  if (mSharedDecoderManager && mVideo.mDecoder) {
    mSharedDecoderManager->SetIdle(mVideo.mDecoder);
    NotifyResourcesStatusChanged();
  }
}

void
MP4Reader::SetSharedDecoderManager(SharedDecoderManager* aManager)
{
#ifdef MOZ_GONK_MEDIACODEC
  mSharedDecoderManager = aManager;
#endif
}

} 
