





#include "MP4Reader.h"
#include "MP4Stream.h"
#include "MediaResource.h"
#include "nsPrintfCString.h"
#include "nsSize.h"
#include "VideoUtils.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "ImageContainer.h"
#include "Layers.h"
#include "SharedThreadPool.h"
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"
#include "mozilla/dom/TimeRanges.h"
#include "mp4_demuxer/AnnexB.h"
#include "mp4_demuxer/H264.h"
#include "SharedDecoderManager.h"
#include "mp4_demuxer/MP4TrackDemuxer.h"
#include <algorithm>

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
#define LOG(arg, ...) PR_LOG(GetDemuxerLog(), PR_LOG_DEBUG, ("MP4Reader(%p)::%s: " arg, this, __func__, ##__VA_ARGS__))
#define VLOG(arg, ...) PR_LOG(GetDemuxerLog(), PR_LOG_DEBUG, ("MP4Reader(%p)::%s: " arg, this, __func__, ##__VA_ARGS__))
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

bool
AccumulateSPSTelemetry(const ByteBuffer* aExtradata)
{
  SPSData spsdata;
  if (H264::DecodeSPSFromExtraData(aExtradata, spsdata)) {
   uint8_t constraints = (spsdata.constraint_set0_flag ? (1 << 0) : 0) |
                         (spsdata.constraint_set1_flag ? (1 << 1) : 0) |
                         (spsdata.constraint_set2_flag ? (1 << 2) : 0) |
                         (spsdata.constraint_set3_flag ? (1 << 3) : 0) |
                         (spsdata.constraint_set4_flag ? (1 << 4) : 0) |
                         (spsdata.constraint_set5_flag ? (1 << 5) : 0);
    Telemetry::Accumulate(Telemetry::VIDEO_DECODED_H264_SPS_CONSTRAINT_SET_FLAG,
                          constraints);

    
    Telemetry::Accumulate(Telemetry::VIDEO_DECODED_H264_SPS_PROFILE,
                          spsdata.profile_idc <= 244 ? spsdata.profile_idc : 0);

    
    
    Telemetry::Accumulate(Telemetry::VIDEO_DECODED_H264_SPS_LEVEL,
                          (spsdata.level_idc >= 10 && spsdata.level_idc <= 52) ?
                          spsdata.level_idc : 0);

    
    
    Telemetry::Accumulate(Telemetry::VIDEO_H264_SPS_MAX_NUM_REF_FRAMES,
                          std::min(spsdata.max_num_ref_frames, 17u));

    return true;
  }

  return false;
}














template<typename ThisType, typename ReturnType>
ReturnType
InvokeAndRetry(ThisType* aThisVal, ReturnType(ThisType::*aMethod)(), MP4Stream* aStream, Monitor* aMonitor)
{
  AutoPinned<MP4Stream> stream(aStream);
  MP4Stream::ReadRecord prevFailure(-1, 0);
  while (true) {
    ReturnType result = ((*aThisVal).*aMethod)();
    if (result) {
      return result;
    }
    MP4Stream::ReadRecord failure(-1, 0);
    if (NS_WARN_IF(!stream->LastReadFailed(&failure))) {
      return result;
    }
    stream->ClearFailedRead();

    if (NS_WARN_IF(failure == prevFailure)) {
      NS_WARNING(nsPrintfCString("Failed reading the same block twice: offset=%lld, count=%lu",
                                 failure.mOffset, failure.mCount).get());
      return result;
    }

    prevFailure = failure;
    if (NS_WARN_IF(!stream->BlockingReadIntoCache(failure.mOffset, failure.mCount, aMonitor))) {
      return result;
    }
  }
}

MP4Reader::MP4Reader(AbstractMediaDecoder* aDecoder)
  : MediaDecoderReader(aDecoder)
  , mAudio(MediaData::AUDIO_DATA, Preferences::GetUint("media.mp4-audio-decode-ahead", 2))
  , mVideo(MediaData::VIDEO_DATA, Preferences::GetUint("media.mp4-video-decode-ahead", 2))
  , mLastReportedNumDecodedFrames(0)
  , mLayersBackendType(layers::LayersBackend::LAYERS_NONE)
  , mDemuxerInitialized(false)
  , mFoundSPSForTelemetry(false)
  , mIsEncrypted(false)
  , mAreDecodersSetup(false)
  , mIndexReady(false)
  , mLastSeenEnd(-1)
  , mDemuxerMonitor("MP4 Demuxer")
#if defined(MP4_READER_DORMANT_HEURISTIC)
  , mDormantEnabled(Preferences::GetBool("media.decoder.heuristic.dormant.enabled", false))
#endif
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

  mPlatform = nullptr;

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
static bool sDemuxSkipToNextKeyframe = true;

nsresult
MP4Reader::Init(MediaDecoderReader* aCloneDonor)
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
  PlatformDecoderModule::Init();
  mStream = new MP4Stream(mDecoder->GetResource());

  InitLayersBackendType();

  mAudio.mTaskQueue = new FlushableMediaTaskQueue(GetMediaThreadPool());
  NS_ENSURE_TRUE(mAudio.mTaskQueue, NS_ERROR_FAILURE);

  mVideo.mTaskQueue = new FlushableMediaTaskQueue(GetMediaThreadPool());
  NS_ENSURE_TRUE(mVideo.mTaskQueue, NS_ERROR_FAILURE);

  static bool sSetupPrefCache = false;
  if (!sSetupPrefCache) {
    sSetupPrefCache = true;
    Preferences::AddBoolVarCache(&sIsEMEEnabled, "media.eme.enabled", false);
    Preferences::AddBoolVarCache(&sDemuxSkipToNextKeyframe, "media.fmp4.demux-skip", true);
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
  if (mVideo.mDecoder) {
    mVideo.mDecoder->AllocateMediaResources();
  }
}

bool MP4Reader::IsWaitingMediaResources() {
  return mVideo.mDecoder && mVideo.mDecoder->IsWaitingMediaResources();
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
    LOG("capsKnown=%d", caps.AreCapsKnown());
    return !caps.AreCapsKnown();
  }
#else
  return false;
#endif
}

void
MP4Reader::ExtractCryptoInitData(nsTArray<uint8_t>& aInitData)
{
  MOZ_ASSERT(mCrypto.valid);
  const nsTArray<mp4_demuxer::PsshInfo>& psshs = mCrypto.pssh;
  for (uint32_t i = 0; i < psshs.Length(); i++) {
    aInitData.AppendElements(psshs[i].data);
  }
}

bool
MP4Reader::IsSupportedAudioMimeType(const nsACString& aMimeType)
{
  return (aMimeType.EqualsLiteral("audio/mpeg") ||
          aMimeType.EqualsLiteral("audio/mp4a-latm")) &&
         mPlatform->SupportsAudioMimeType(aMimeType);
}

bool
MP4Reader::IsSupportedVideoMimeType(const nsACString& aMimeType)
{
  return (aMimeType.EqualsLiteral("video/mp4") ||
          aMimeType.EqualsLiteral("video/avc") ||
          aMimeType.EqualsLiteral("video/x-vnd.on2.vp6")) &&
         mPlatform->SupportsVideoMimeType(aMimeType);
}

void
MP4Reader::PreReadMetadata()
{
  if (mPlatform) {
    RequestCodecResource();
  }
}

bool
MP4Reader::InitDemuxer()
{
  mDemuxer = new MP4Demuxer(mStream, &mDemuxerMonitor);
  return mDemuxer->Init();
}

nsresult
MP4Reader::ReadMetadata(MediaInfo* aInfo,
                        MetadataTags** aTags)
{
  if (!mDemuxerInitialized) {
    MonitorAutoLock mon(mDemuxerMonitor);
    bool ok = InvokeAndRetry(this, &MP4Reader::InitDemuxer, mStream, &mDemuxerMonitor);
    NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);
    mIndexReady = true;

    
    mInfo.mVideo.mHasVideo = mVideo.mActive = mDemuxer->HasValidVideo() &&
                                              mDecoder->GetImageContainer();
    if (mVideo.mActive) {
      mVideo.mTrackDemuxer = new MP4VideoDemuxer(mDemuxer);
    }

    mInfo.mAudio.mHasAudio = mAudio.mActive = mDemuxer->HasValidAudio();
    if (mAudio.mActive) {
      mAudio.mTrackDemuxer = new MP4AudioDemuxer(mDemuxer);
    }
    mCrypto = mDemuxer->Crypto();

    {
      MonitorAutoUnlock unlock(mDemuxerMonitor);
      ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
      mIsEncrypted = mCrypto.valid;
    }

    
    
    
    mDemuxerInitialized = true;
  } else if (mPlatform && !IsWaitingMediaResources()) {
    *aInfo = mInfo;
    *aTags = nullptr;
    return NS_OK;
  }

  if (HasAudio()) {
    const AudioDecoderConfig& audio = mDemuxer->AudioConfig();
    mInfo.mAudio.mRate = audio.samples_per_second;
    mInfo.mAudio.mChannels = audio.channel_count;
    mAudio.mCallback = new DecoderCallback(this, kAudio);
  }

  if (HasVideo()) {
    const VideoDecoderConfig& video = mDemuxer->VideoConfig();
    mInfo.mVideo.mDisplay =
      nsIntSize(video.display_width, video.display_height);
    mVideo.mCallback = new DecoderCallback(this, kVideo);

    
    if (!mFoundSPSForTelemetry) {
      mFoundSPSForTelemetry = AccumulateSPSTelemetry(video.extra_data);
    }
  }

  if (mCrypto.valid) {
    nsTArray<uint8_t> initData;
    ExtractCryptoInitData(initData);
    if (initData.Length() == 0) {
      return NS_ERROR_FAILURE;
    }

#ifdef MOZ_EME
    
    NS_DispatchToMainThread(
      new DispatchKeyNeededEvent(mDecoder, initData, NS_LITERAL_STRING("cenc")));
#endif 
    
    
    mInfo.mCrypto.AddInitData(NS_LITERAL_STRING("cenc"), Move(initData));
  }

  
  Microseconds duration;
  {
    MonitorAutoLock lock(mDemuxerMonitor);
    duration = mDemuxer->Duration();
  }
  if (duration != -1) {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mDecoder->SetMediaDuration(duration);
  }

  *aInfo = mInfo;
  *aTags = nullptr;

  if (!IsWaitingMediaResources() && !IsWaitingOnCDMResource()) {
    NS_ENSURE_TRUE(EnsureDecodersSetup(), NS_ERROR_FAILURE);
  }

  MonitorAutoLock mon(mDemuxerMonitor);
  UpdateIndex();

  return NS_OK;
}

bool
MP4Reader::EnsureDecodersSetup()
{
  if (mAreDecodersSetup) {
    return !!mPlatform;
  }

  if (mIsEncrypted) {
#ifdef MOZ_EME
    
    
    
    
    
    
    nsRefPtr<CDMProxy> proxy;
    if (IsWaitingMediaResources()) {
      return true;
    }
    MOZ_ASSERT(!IsWaitingMediaResources());

    {
      ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
      proxy = mDecoder->GetCDMProxy();
    }
    MOZ_ASSERT(proxy);

    mPlatform = PlatformDecoderModule::CreateCDMWrapper(proxy,
                                                        HasAudio(),
                                                        HasVideo());
    NS_ENSURE_TRUE(mPlatform, false);
#else
    
    return false;
#endif
  } else {
    mPlatform = PlatformDecoderModule::Create();
    NS_ENSURE_TRUE(mPlatform, false);
  }

  if (HasAudio()) {
    NS_ENSURE_TRUE(IsSupportedAudioMimeType(mDemuxer->AudioConfig().mime_type),
                   false);

    mAudio.mDecoder = mPlatform->CreateAudioDecoder(mDemuxer->AudioConfig(),
                                                    mAudio.mTaskQueue,
                                                    mAudio.mCallback);
    NS_ENSURE_TRUE(mAudio.mDecoder != nullptr, false);
    nsresult rv = mAudio.mDecoder->Init();
    NS_ENSURE_SUCCESS(rv, false);
  }

  if (HasVideo()) {
    NS_ENSURE_TRUE(IsSupportedVideoMimeType(mDemuxer->VideoConfig().mime_type),
                   false);

    if (mSharedDecoderManager && mPlatform->SupportsSharedDecoders(mDemuxer->VideoConfig())) {
      mVideo.mDecoder =
        mSharedDecoderManager->CreateVideoDecoder(mPlatform,
                                                  mDemuxer->VideoConfig(),
                                                  mLayersBackendType,
                                                  mDecoder->GetImageContainer(),
                                                  mVideo.mTaskQueue,
                                                  mVideo.mCallback);
    } else {
      mVideo.mDecoder = mPlatform->CreateVideoDecoder(mDemuxer->VideoConfig(),
                                                      mLayersBackendType,
                                                      mDecoder->GetImageContainer(),
                                                      mVideo.mTaskQueue,
                                                      mVideo.mCallback);
    }
    NS_ENSURE_TRUE(mVideo.mDecoder != nullptr, false);
    nsresult rv = mVideo.mDecoder->Init();
    NS_ENSURE_SUCCESS(rv, false);
  }

  mAreDecodersSetup = true;
  return true;
}

void
MP4Reader::ReadUpdatedMetadata(MediaInfo* aInfo)
{
  *aInfo = mInfo;
}

bool
MP4Reader::IsMediaSeekable()
{
  
  
  MonitorAutoLock mon(mDemuxerMonitor);
  return mDecoder->GetResource()->IsTransportSeekable();
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

Microseconds
MP4Reader::GetNextKeyframeTime()
{
  MonitorAutoLock mon(mDemuxerMonitor);
  return mVideo.mTrackDemuxer->GetNextKeyframeTime();
}

void
MP4Reader::DisableHardwareAcceleration()
{
  if (HasVideo() && mSharedDecoderManager) {
    mSharedDecoderManager->DisableHardwareAcceleration();

    const VideoDecoderConfig& video = mDemuxer->VideoConfig();
    if (!mSharedDecoderManager->Recreate(video, mLayersBackendType, mDecoder->GetImageContainer())) {
      MonitorAutoLock mon(mVideo.mMonitor);
      mVideo.mError = true;
      if (mVideo.HasPromise()) {
        mVideo.RejectPromise(DECODE_ERROR, __func__);
      }
    } else {
      MonitorAutoLock lock(mVideo.mMonitor);
      ScheduleUpdate(kVideo);
    }
  }
}

bool
MP4Reader::ShouldSkip(bool aSkipToNextKeyframe, int64_t aTimeThreshold)
{
  
  
  
  
  
  Microseconds nextKeyframe = -1;
  if (!sDemuxSkipToNextKeyframe ||
      (nextKeyframe = GetNextKeyframeTime()) == -1) {
    return aSkipToNextKeyframe;
  }
  return nextKeyframe < aTimeThreshold;
}

nsRefPtr<MediaDecoderReader::VideoDataPromise>
MP4Reader::RequestVideoData(bool aSkipToNextKeyframe,
                            int64_t aTimeThreshold)
{
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());
  VLOG("skip=%d time=%lld", aSkipToNextKeyframe, aTimeThreshold);

  if (!EnsureDecodersSetup()) {
    NS_WARNING("Error constructing MP4 decoders");
    return VideoDataPromise::CreateAndReject(DECODE_ERROR, __func__);
  }

  if (mShutdown) {
    NS_WARNING("RequestVideoData on shutdown MP4Reader!");
    return VideoDataPromise::CreateAndReject(CANCELED, __func__);
  }

  MOZ_ASSERT(HasVideo() && mPlatform && mVideo.mDecoder);

  bool eos = false;
  if (ShouldSkip(aSkipToNextKeyframe, aTimeThreshold)) {
    uint32_t parsed = 0;
    eos = !SkipVideoDemuxToNextKeyFrame(aTimeThreshold, parsed);
    if (!eos && NS_FAILED(mVideo.mDecoder->Flush())) {
      NS_WARNING("Failed to skip/flush video when skipping-to-next-keyframe.");
    }
    mDecoder->NotifyDecodedFrames(parsed, 0, parsed);
  }

  MonitorAutoLock lock(mVideo.mMonitor);
  nsRefPtr<VideoDataPromise> p = mVideo.mPromise.Ensure(__func__);
  if (mVideo.mError) {
    mVideo.mPromise.Reject(DECODE_ERROR, __func__);
  } else if (eos) {
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
  VLOG("");

  if (!EnsureDecodersSetup()) {
    NS_WARNING("Error constructing MP4 decoders");
    return AudioDataPromise::CreateAndReject(DECODE_ERROR, __func__);
  }

  if (mShutdown) {
    NS_WARNING("RequestAudioData on shutdown MP4Reader!");
    return AudioDataPromise::CreateAndReject(CANCELED, __func__);
  }

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

  if (mShutdown) {
    return;
  }

  
  
  AbstractMediaDecoder::AutoNotifyDecoded a(mDecoder);

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
      a.mDecoded = static_cast<uint32_t>(delta);
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
    nsAutoPtr<MediaSample> sample(PopSample(aTrack));

    
    if (!mFoundSPSForTelemetry && sample && AnnexB::HasSPS(sample->mMp4Sample)) {
      nsRefPtr<ByteBuffer> extradata = AnnexB::ExtractExtraData(sample->mMp4Sample);
      mFoundSPSForTelemetry = AccumulateSPSTelemetry(extradata);
    }

    if (sample) {
      decoder.mDecoder->Input(sample->mMp4Sample.forget());
      if (aTrack == kVideo) {
        a.mParsed++;
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
      LOG("change of sampling rate:%d->%d",
          mInfo.mAudio.mRate, audioData->mRate);
      mInfo.mAudio.mRate = audioData->mRate;
      mInfo.mAudio.mChannels = audioData->mChannels;
    }

    mAudio.mPromise.Resolve(audioData, __func__);
  } else if (aTrack == kVideo) {
    mVideo.mPromise.Resolve(static_cast<VideoData*>(aData), __func__);
  }
}

MediaSample*
MP4Reader::PopSample(TrackType aTrack)
{
  MonitorAutoLock mon(mDemuxerMonitor);
  return PopSampleLocked(aTrack);
}

MediaSample*
MP4Reader::PopSampleLocked(TrackType aTrack)
{
  mDemuxerMonitor.AssertCurrentThreadOwns();
  switch (aTrack) {
    case kAudio:
      return InvokeAndRetry(mAudio.mTrackDemuxer.get(), &TrackDemuxer::DemuxSample, mStream, &mDemuxerMonitor);
    case kVideo:
      if (mQueuedVideoSample) {
        return mQueuedVideoSample.forget();
      }
      return InvokeAndRetry(mVideo.mTrackDemuxer.get(), &TrackDemuxer::DemuxSample, mStream, &mDemuxerMonitor);

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
  {
    MonitorAutoLock mon(mDemuxerMonitor);
    if (mVideo.mTrackDemuxer) {
      mVideo.mTrackDemuxer->Seek(0);
    }
  }
  Flush(kAudio);
  {
    MonitorAutoLock mon(mDemuxerMonitor);
    if (mAudio.mTrackDemuxer) {
      mAudio.mTrackDemuxer->Seek(0);
    }
  }
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
    nsAutoPtr<MediaSample> compressed(PopSample(kVideo));
    if (!compressed) {
      
      MonitorAutoLock mon(mVideo.mMonitor);
      mVideo.mDemuxEOS = true;
      return false;
    }
    parsed++;
    if (!compressed->mMp4Sample->is_sync_point ||
        compressed->mMp4Sample->composition_timestamp < aTimeThreshold) {
      continue;
    }
    mQueuedVideoSample = compressed;
    break;
  }

  return true;
}

nsRefPtr<MediaDecoderReader::SeekPromise>
MP4Reader::Seek(int64_t aTime, int64_t aEndTime)
{
  LOG("aTime=(%lld)", aTime);
  MOZ_ASSERT(GetTaskQueue()->IsCurrentThreadIn());
  MonitorAutoLock mon(mDemuxerMonitor);
  if (!mDecoder->GetResource()->IsTransportSeekable() || !mDemuxer->CanSeek()) {
    VLOG("Seek() END (Unseekable)");
    return SeekPromise::CreateAndReject(NS_ERROR_FAILURE, __func__);
  }

  int64_t seekTime = aTime;
  mQueuedVideoSample = nullptr;
  if (mDemuxer->HasValidVideo()) {
    mVideo.mTrackDemuxer->Seek(seekTime);
    mQueuedVideoSample = PopSampleLocked(kVideo);
    if (mQueuedVideoSample) {
      seekTime = mQueuedVideoSample->mMp4Sample->composition_timestamp;
    }
  }
  if (mDemuxer->HasValidAudio()) {
    mAudio.mTrackDemuxer->Seek(seekTime);
  }
  LOG("aTime=%lld exit", aTime);
  return SeekPromise::CreateAndResolve(seekTime, __func__);
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
  MonitorAutoLock mon(mDemuxerMonitor);
  if (!mIndexReady) {
    return 0;
  }

  return mDemuxer->GetEvictionOffset(aTime * 1000000.0);
}

nsresult
MP4Reader::GetBuffered(dom::TimeRanges* aBuffered)
{
  MonitorAutoLock mon(mDemuxerMonitor);
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
#if defined(MP4_READER_DORMANT)
  return
#if defined(MP4_READER_DORMANT_HEURISTIC)
        mDormantEnabled &&
#endif
        mVideo.mDecoder &&
        mVideo.mDecoder->IsDormantNeeded();
#else
  return false;
#endif
}

void MP4Reader::ReleaseMediaResources()
{
  
  
  VideoFrameContainer* container = mDecoder->GetVideoFrameContainer();
  if (container) {
    container->ClearCurrentFrame();
  }
  if (mVideo.mDecoder) {
    mVideo.mDecoder->ReleaseMediaResources();
  }
}

void MP4Reader::NotifyResourcesStatusChanged()
{
  if (mDecoder) {
    mDecoder->NotifyWaitingForResourcesStatusChanged();
  }
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
#if !defined(MOZ_WIDGET_ANDROID)
  mSharedDecoderManager = aManager;
#endif
}

bool
MP4Reader::VideoIsHardwareAccelerated() const
{
  return mVideo.mDecoder && mVideo.mDecoder->IsHardwareAccelerated();
}

void
MP4Reader::NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mShutdown) {
    return;
  }

  if (mLastSeenEnd < 0) {
    MonitorAutoLock mon(mDemuxerMonitor);
    mLastSeenEnd = mDecoder->GetResource()->GetLength();
    if (mLastSeenEnd < 0) {
      
      return;
    }
  }
  int64_t end = aOffset + aLength;
  if (end <= mLastSeenEnd) {
    return;
  }
  mLastSeenEnd = end;

  if (HasVideo()) {
    auto& decoder = GetDecoderData(kVideo);
    MonitorAutoLock lock(decoder.mMonitor);
    decoder.mDemuxEOS = false;
  }
  if (HasAudio()) {
    auto& decoder = GetDecoderData(kAudio);
    MonitorAutoLock lock(decoder.mMonitor);
    decoder.mDemuxEOS = false;
  }
}

} 
