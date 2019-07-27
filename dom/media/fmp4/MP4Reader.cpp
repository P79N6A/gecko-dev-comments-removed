





#include "MP4Reader.h"
#include "MP4Stream.h"
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

MP4Reader::MP4Reader(AbstractMediaDecoder* aDecoder)
  : MediaDecoderReader(aDecoder)
  , mAudio(MediaData::AUDIO_DATA, Preferences::GetUint("media.mp4-audio-decode-ahead", 2))
  , mVideo(MediaData::VIDEO_DATA, Preferences::GetUint("media.mp4-video-decode-ahead", 2))
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

nsRefPtr<ShutdownPromise>
MP4Reader::Shutdown()
{
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());

  if (mAudio.mDecoder) {
    Flush(kAudio);
    mAudio.mDecoder->Shutdown();
    mAudio.mDecoder = nullptr;
  }
  if (mAudio.mTaskQueue) {
    mAudio.mTaskQueue->BeginShutdown();
    mAudio.mTaskQueue->AwaitShutdownAndIdle();
    mAudio.mTaskQueue = nullptr;
  }
  mAudio.mPromise.SetMonitor(nullptr);
  MOZ_ASSERT(mAudio.mPromise.IsEmpty());

  if (mVideo.mDecoder) {
    Flush(kVideo);
    mVideo.mDecoder->Shutdown();
    mVideo.mDecoder = nullptr;
  }
  if (mVideo.mTaskQueue) {
    mVideo.mTaskQueue->BeginShutdown();
    mVideo.mTaskQueue->AwaitShutdownAndIdle();
    mVideo.mTaskQueue = nullptr;
  }
  mVideo.mPromise.SetMonitor(nullptr);
  MOZ_ASSERT(mVideo.mPromise.IsEmpty());
  
  mQueuedVideoSample = nullptr;

  if (mPlatform) {
    mPlatform->Shutdown();
    mPlatform = nullptr;
  }

  return MediaDecoderReader::Shutdown();
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
  mDemuxer = new MP4Demuxer(new MP4Stream(mDecoder->GetResource(), &mIndexMonitor), &mIndexMonitor);

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

void MP4Reader::RequestCodecResource() {
#ifdef MOZ_GONK_MEDIACODEC
  if(mVideo.mDecoder) {
    mVideo.mDecoder->AllocateMediaResources();
  }
#endif
}

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
          !strcmp(aMimeType, "video/avc") ||
          !strcmp(aMimeType, "video/x-vnd.on2.vp6")) &&
         mPlatform->SupportsVideoMimeType(aMimeType);
}

void
MP4Reader::PreReadMetadata()
{
  if (mPlatform) {
    RequestCodecResource();
  }
}

nsresult
MP4Reader::ReadMetadata(MediaInfo* aInfo,
                        MetadataTags** aTags)
{
  if (!mDemuxerInitialized) {
    {
      MonitorAutoLock mon(mIndexMonitor);
      bool ok = mDemuxer->Init();
      NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);
      mIndexReady = true;
    }

    
    mInfo.mVideo.mHasVideo = mVideo.mActive = mDemuxer->HasValidVideo() &&
                                              mDecoder->GetImageContainer();

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

  MonitorAutoLock mon(mIndexMonitor);
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
  if (aTrack == kAudio) {
    return mAudio;
  }
  return mVideo;
}

nsRefPtr<MediaDecoderReader::VideoDataPromise>
MP4Reader::RequestVideoData(bool aSkipToNextKeyframe,
                            int64_t aTimeThreshold)
{
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());
  VLOG("RequestVideoData skip=%d time=%lld", aSkipToNextKeyframe, aTimeThreshold);

  MOZ_ASSERT(HasVideo() && mPlatform && mVideo.mDecoder);

  bool eos = false;
  if (aSkipToNextKeyframe) {
    uint32_t parsed = 0;
    eos = !SkipVideoDemuxToNextKeyFrame(aTimeThreshold, parsed);
    if (!eos && NS_FAILED(mVideo.mDecoder->Flush())) {
      NS_WARNING("Failed to skip/flush video when skipping-to-next-keyframe.");
    }
    mDecoder->NotifyDecodedFrames(parsed, 0);
  }

  MonitorAutoLock lock(mVideo.mMonitor);
  nsRefPtr<VideoDataPromise> p = mVideo.mPromise.Ensure(__func__);
  if (eos) {
    mVideo.mPromise.Reject(END_OF_STREAM, __func__);
  } else {
    ScheduleUpdate(kVideo);
  }

  return p;
}

nsRefPtr<MediaDecoderReader::AudioDataPromise>
MP4Reader::RequestAudioData()
{
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());
  VLOG("RequestAudioData");
  MonitorAutoLock lock(mAudio.mMonitor);
  nsRefPtr<AudioDataPromise> p = mAudio.mPromise.Ensure(__func__);
  ScheduleUpdate(kAudio);
  return p;
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
    !aDecoder.mDemuxEOS &&
    aDecoder.HasPromise() &&
    aDecoder.mOutput.IsEmpty() &&
    (aDecoder.mInputExhausted ||
     aDecoder.mNumSamplesInput - aDecoder.mNumSamplesOutput < aDecoder.mDecodeAhead);
}

void
MP4Reader::Update(TrackType aTrack)
{
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());

  
  
  uint32_t parsed = 0, decoded = 0;
  AbstractMediaDecoder::AutoNotifyDecoded autoNotify(mDecoder, parsed, decoded);

  bool needInput = false;
  bool needOutput = false;
  auto& decoder = GetDecoderData(aTrack);
  {
    MonitorAutoLock lock(decoder.mMonitor);
    decoder.mUpdateScheduled = false;
    if (NeedInput(decoder)) {
      needInput = true;
      decoder.mInputExhausted = false;
      decoder.mNumSamplesInput++;
    }
    if (aTrack == kVideo) {
      uint64_t delta = decoder.mNumSamplesOutput - mLastReportedNumDecodedFrames;
      decoded = static_cast<uint32_t>(delta);
      mLastReportedNumDecodedFrames = decoder.mNumSamplesOutput;
    }
    if (decoder.HasPromise()) {
      needOutput = true;
      if (!decoder.mOutput.IsEmpty()) {
        nsRefPtr<MediaData> output = decoder.mOutput[0];
        decoder.mOutput.RemoveElementAt(0);
        ReturnOutput(output, aTrack);
      } else if (decoder.mDrainComplete) {
        decoder.RejectPromise(END_OF_STREAM, __func__);
      }
    }
  }

  VLOG("Update(%s) ni=%d no=%d iex=%d fl=%d",
       TrackTypeToStr(aTrack),
       needInput,
       needOutput,
       decoder.mInputExhausted,
       decoder.mIsFlushing);

  if (needInput) {
    MP4Sample* sample = PopSample(aTrack);
    if (sample) {
      decoder.mDecoder->Input(sample);
      if (aTrack == kVideo) {
        parsed++;
      }
    } else {
      {
        MonitorAutoLock lock(decoder.mMonitor);
        MOZ_ASSERT(!decoder.mDemuxEOS);
        decoder.mDemuxEOS = true;
      }
      
      decoder.mDecoder->Drain();
    }
  }
}

void
MP4Reader::ReturnOutput(MediaData* aData, TrackType aTrack)
{
  auto& decoder = GetDecoderData(aTrack);
  decoder.mMonitor.AssertCurrentThreadOwns();
  MOZ_ASSERT(decoder.HasPromise());
  if (decoder.mDiscontinuity) {
    decoder.mDiscontinuity = false;
    aData->mDiscontinuity = true;
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

    mAudio.mPromise.Resolve(audioData, __func__);
  } else if (aTrack == kVideo) {
    mVideo.mPromise.Resolve(static_cast<VideoData*>(aData), __func__);
  }
}

MP4Sample*
MP4Reader::PopSample(TrackType aTrack)
{
  MonitorAutoLock mon(mIndexMonitor);
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

size_t
MP4Reader::SizeOfVideoQueueInFrames()
{
  return SizeOfQueue(kVideo);
}

size_t
MP4Reader::SizeOfAudioQueueInFrames()
{
  return SizeOfQueue(kAudio);
}

size_t
MP4Reader::SizeOfQueue(TrackType aTrack)
{
  auto& decoder = GetDecoderData(aTrack);
  MonitorAutoLock lock(decoder.mMonitor);
  return decoder.mOutput.Length() + (decoder.mNumSamplesInput - decoder.mNumSamplesOutput);
}

nsresult
MP4Reader::ResetDecode()
{
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());
  Flush(kVideo);
  mDemuxer->SeekVideo(0);
  Flush(kAudio);
  mDemuxer->SeekAudio(0);
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
  if (NeedInput(decoder) || decoder.HasPromise()) {
    ScheduleUpdate(aTrack);
  }
}

void
MP4Reader::DrainComplete(TrackType aTrack)
{
  DecoderData& data = GetDecoderData(aTrack);
  MonitorAutoLock mon(data.mMonitor);
  data.mDrainComplete = true;
  ScheduleUpdate(aTrack);
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
    if (data.HasPromise()) {
      data.RejectPromise(DECODE_ERROR, __func__);
    }
  }
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
    data.mDemuxEOS = false;
    data.mDrainComplete = false;
  }
  data.mDecoder->Flush();
  {
    MonitorAutoLock mon(data.mMonitor);
    data.mIsFlushing = false;
    data.mOutput.Clear();
    data.mNumSamplesInput = 0;
    data.mNumSamplesOutput = 0;
    data.mInputExhausted = false;
    if (data.HasPromise()) {
      data.RejectPromise(CANCELED, __func__);
    }
    data.mDiscontinuity = true;
    data.mUpdateScheduled = false;
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
      
      MonitorAutoLock mon(mVideo.mMonitor);
      mVideo.mDemuxEOS = true;
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

nsRefPtr<MediaDecoderReader::SeekPromise>
MP4Reader::Seek(int64_t aTime,
                int64_t aStartTime,
                int64_t aEndTime,
                int64_t aCurrentTime)
{
  LOG("MP4Reader::Seek(%lld)", aTime);
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());
  if (!mDecoder->GetResource()->IsTransportSeekable() || !mDemuxer->CanSeek()) {
    VLOG("Seek() END (Unseekable)");
    return SeekPromise::CreateAndReject(NS_ERROR_FAILURE, __func__);
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
  return SeekPromise::CreateAndResolve(true, __func__);
}

void
MP4Reader::UpdateIndex()
{
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
  UpdateIndex();
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
