





#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/TimeRanges.h"
#include "mozilla/Preferences.h"
#include "nsPrintfCString.h"
#include "nsSize.h"
#include "ImageContainer.h"
#include "Layers.h"
#include "MediaData.h"
#include "MediaInfo.h"
#include "MediaFormatReader.h"
#include "MediaResource.h"
#include "SharedDecoderManager.h"
#include "SharedThreadPool.h"
#include "VideoUtils.h"

#include <algorithm>

#ifdef MOZ_EME
#include "mozilla/CDMProxy.h"
#endif

using mozilla::layers::Image;
using mozilla::layers::LayerManager;
using mozilla::layers::LayersBackend;

PRLogModuleInfo* GetFormatDecoderLog() {
  static PRLogModuleInfo* log = nullptr;
  if (!log) {
    log = PR_NewLogModule("MediaFormatReader");
  }
  return log;
}
#define LOG(arg, ...) PR_LOG(GetFormatDecoderLog(), PR_LOG_DEBUG, ("MediaFormatReader(%p)::%s: " arg, this, __func__, ##__VA_ARGS__))
#define LOGV(arg, ...) PR_LOG(GetFormatDecoderLog(), PR_LOG_DEBUG+1, ("MediaFormatReader(%p)::%s: " arg, this, __func__, ##__VA_ARGS__))

namespace mozilla {

static const char*
TrackTypeToStr(TrackInfo::TrackType aTrack)
{
  MOZ_ASSERT(aTrack == TrackInfo::kAudioTrack ||
             aTrack == TrackInfo::kVideoTrack ||
             aTrack == TrackInfo::kTextTrack);
  switch (aTrack) {
  case TrackInfo::kAudioTrack:
    return "Audio";
  case TrackInfo::kVideoTrack:
    return "Video";
  case TrackInfo::kTextTrack:
    return "Text";
  default:
    return "Unknown";
  }
}

MediaFormatReader::MediaFormatReader(AbstractMediaDecoder* aDecoder,
                                       MediaDataDemuxer* aDemuxer)
  : MediaDecoderReader(aDecoder)
  , mDemuxer(aDemuxer)
  , mAudio(this, MediaData::AUDIO_DATA, Preferences::GetUint("media.audio-decode-ahead", 2))
  , mVideo(this, MediaData::VIDEO_DATA, Preferences::GetUint("media.video-decode-ahead", 2))
  , mLastReportedNumDecodedFrames(0)
  , mLayersBackendType(layers::LayersBackend::LAYERS_NONE)
  , mInitDone(false)
  , mSeekable(false)
  , mIsEncrypted(false)
#if defined(READER_DORMANT_HEURISTIC)
  , mDormantEnabled(Preferences::GetBool("media.decoder.heuristic.dormant.enabled", false))
#endif
{
  MOZ_ASSERT(aDemuxer);
  MOZ_COUNT_CTOR(MediaFormatReader);
}

MediaFormatReader::~MediaFormatReader()
{
  MOZ_COUNT_DTOR(MediaFormatReader);
  
  if (mAudioTrackDemuxer) {
    mAudioTrackDemuxer->BreakCycles();
    mAudioTrackDemuxer = nullptr;
  }
  if (mVideoTrackDemuxer) {
    mVideoTrackDemuxer->BreakCycles();
    mVideoTrackDemuxer = nullptr;
  }
  mMainThreadDemuxer = nullptr;
}

nsRefPtr<ShutdownPromise>
MediaFormatReader::Shutdown()
{
  MOZ_ASSERT(OnTaskQueue());

  mDemuxerInitRequest.DisconnectIfExists();
  mSeekPromise.RejectIfExists(NS_ERROR_FAILURE, __func__);
  mSkipRequest.DisconnectIfExists();

  if (mAudio.mDecoder) {
    Flush(TrackInfo::kAudioTrack);
    mAudio.mDecoder->Shutdown();
    mAudio.mDecoder = nullptr;
  }
  if (mAudio.mTrackDemuxer) {
    mAudio.mTrackDemuxer->BreakCycles();
    mAudio.mTrackDemuxer = nullptr;
  }
  if (mAudio.mTaskQueue) {
    mAudio.mTaskQueue->BeginShutdown();
    mAudio.mTaskQueue->AwaitShutdownAndIdle();
    mAudio.mTaskQueue = nullptr;
  }
  MOZ_ASSERT(mAudio.mPromise.IsEmpty());

  if (mVideo.mDecoder) {
    Flush(TrackInfo::kVideoTrack);
    mVideo.mDecoder->Shutdown();
    mVideo.mDecoder = nullptr;
  }
  if (mVideo.mTrackDemuxer) {
    mVideo.mTrackDemuxer->BreakCycles();
    mVideo.mTrackDemuxer = nullptr;
  }
  if (mVideo.mTaskQueue) {
    mVideo.mTaskQueue->BeginShutdown();
    mVideo.mTaskQueue->AwaitShutdownAndIdle();
    mVideo.mTaskQueue = nullptr;
  }
  MOZ_ASSERT(mVideo.mPromise.IsEmpty());

  mDemuxer = nullptr;

  mPlatform = nullptr;

  return MediaDecoderReader::Shutdown();
}

void
MediaFormatReader::InitLayersBackendType()
{
  if (!IsVideoContentType(mDecoder->GetResource()->GetContentType())) {
    
    return;
  }
  
  
  
  MediaDecoderOwner* owner = mDecoder->GetOwner();
  if (!owner) {
    NS_WARNING("MediaFormatReader without a decoder owner, can't get HWAccel");
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
MediaFormatReader::Init(MediaDecoderReader* aCloneDonor)
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
  PlatformDecoderModule::Init();

  InitLayersBackendType();

  mAudio.mTaskQueue =
    new FlushableMediaTaskQueue(GetMediaThreadPool(MediaThreadType::PLATFORM_DECODER));
  mVideo.mTaskQueue =
    new FlushableMediaTaskQueue(GetMediaThreadPool(MediaThreadType::PLATFORM_DECODER));

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

bool MediaFormatReader::IsWaitingOnCDMResource() {
#ifdef MOZ_EME
  nsRefPtr<CDMProxy> proxy;
  {
    if (!IsEncrypted()) {
      
      return false;
    }
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
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

bool
MediaFormatReader::IsSupportedAudioMimeType(const nsACString& aMimeType)
{
  return mPlatform && mPlatform->SupportsMimeType(aMimeType);
}

bool
MediaFormatReader::IsSupportedVideoMimeType(const nsACString& aMimeType)
{
  return mPlatform && mPlatform->SupportsMimeType(aMimeType);
}

nsRefPtr<MediaDecoderReader::MetadataPromise>
MediaFormatReader::AsyncReadMetadata()
{
  MOZ_ASSERT(OnTaskQueue());

  if (mInitDone) {
    
    if (!EnsureDecodersSetup()) {
      return MetadataPromise::CreateAndReject(ReadMetadataFailureReason::METADATA_ERROR, __func__);
    }
    nsRefPtr<MetadataHolder> metadata = new MetadataHolder();
    metadata->mInfo = mInfo;
    metadata->mTags = nullptr;
    return MetadataPromise::CreateAndResolve(metadata, __func__);
  }

  nsRefPtr<MetadataPromise> p = mMetadataPromise.Ensure(__func__);

  mDemuxerInitRequest.Begin(mDemuxer->Init()
                       ->RefableThen(GetTaskQueue(), __func__,
                                     this,
                                     &MediaFormatReader::OnDemuxerInitDone,
                                     &MediaFormatReader::OnDemuxerInitFailed));
  return p;
}

void
MediaFormatReader::OnDemuxerInitDone(nsresult)
{
  MOZ_ASSERT(OnTaskQueue());
  mDemuxerInitRequest.Complete();

  
  bool videoActive = !!mDemuxer->GetNumberTracks(TrackInfo::kVideoTrack) &&
    mDecoder->GetImageContainer();

  if (videoActive) {
    
    mVideo.mTrackDemuxer = mDemuxer->GetTrackDemuxer(TrackInfo::kVideoTrack, 0);
    MOZ_ASSERT(mVideo.mTrackDemuxer);
    mInfo.mVideo = *mVideo.mTrackDemuxer->GetInfo()->GetAsVideoInfo();
    mVideo.mCallback = new DecoderCallback(this, TrackInfo::kVideoTrack);
    mVideo.mTimeRanges = mVideo.mTrackDemuxer->GetBuffered();
  }

  bool audioActive = !!mDemuxer->GetNumberTracks(TrackInfo::kAudioTrack);
  if (audioActive) {
    mAudio.mTrackDemuxer = mDemuxer->GetTrackDemuxer(TrackInfo::kAudioTrack, 0);
    MOZ_ASSERT(mAudio.mTrackDemuxer);
    mInfo.mAudio = *mAudio.mTrackDemuxer->GetInfo()->GetAsAudioInfo();
    mAudio.mCallback = new DecoderCallback(this, TrackInfo::kAudioTrack);
    mAudio.mTimeRanges = mAudio.mTrackDemuxer->GetBuffered();
  }

  UniquePtr<EncryptionInfo> crypto = mDemuxer->GetCrypto();

  mIsEncrypted = crypto && crypto->IsEncrypted();

  if (crypto && crypto->IsEncrypted()) {
#ifdef MOZ_EME
    
    for (uint32_t i = 0; i < crypto->mInitDatas.Length(); i++) {
      NS_DispatchToMainThread(
        new DispatchKeyNeededEvent(mDecoder, crypto->mInitDatas[i].mInitData, NS_LITERAL_STRING("cenc")));
    }
#endif 
    mInfo.mCrypto = *crypto;
  }

  int64_t videoDuration = HasVideo() ? mInfo.mVideo.mDuration : 0;
  int64_t audioDuration = HasAudio() ? mInfo.mAudio.mDuration : 0;

  int64_t duration = std::max(videoDuration, audioDuration);
  if (duration != -1) {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mDecoder->SetMediaDuration(duration);
  }

  mSeekable = mDemuxer->IsSeekable();
  mInitDone = true;

  
  mMainThreadDemuxer = mDemuxer->Clone();
  if (!mMainThreadDemuxer) {
    mMetadataPromise.Reject(ReadMetadataFailureReason::METADATA_ERROR, __func__);
    NS_WARNING("Unable to clone current MediaDataDemuxer");
    return;
  }
  if (videoActive) {
    mVideoTrackDemuxer =
      mMainThreadDemuxer->GetTrackDemuxer(TrackInfo::kVideoTrack, 0);
    MOZ_ASSERT(mVideoTrackDemuxer);
  }
  if (audioActive) {
    mAudioTrackDemuxer =
      mMainThreadDemuxer->GetTrackDemuxer(TrackInfo::kAudioTrack, 0);
    MOZ_ASSERT(mAudioTrackDemuxer);
  }

  if (!IsWaitingOnCDMResource() && !EnsureDecodersSetup()) {
    mMetadataPromise.Reject(ReadMetadataFailureReason::METADATA_ERROR, __func__);
  } else {
    nsRefPtr<MetadataHolder> metadata = new MetadataHolder();
    metadata->mInfo = mInfo;
    metadata->mTags = nullptr;
    mMetadataPromise.Resolve(metadata, __func__);
  }
}

void
MediaFormatReader::OnDemuxerInitFailed(DemuxerFailureReason aFailure)
{
  mDemuxerInitRequest.Complete();
  if (aFailure == DemuxerFailureReason::WAITING_FOR_DATA) {
    mMetadataPromise.Reject(ReadMetadataFailureReason::WAITING_FOR_RESOURCES, __func__);
  } else {
    mMetadataPromise.Reject(ReadMetadataFailureReason::METADATA_ERROR, __func__);
  }
}

bool
MediaFormatReader::EnsureDecodersSetup()
{
  MOZ_ASSERT(OnTaskQueue());
  MOZ_ASSERT(mInitDone);

  if (!mPlatform) {
    if (IsEncrypted()) {
#ifdef MOZ_EME
      
      
      
      
      
      
      nsRefPtr<CDMProxy> proxy;
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
  }

  MOZ_ASSERT(mPlatform);

  if (HasAudio() && !mAudio.mDecoder) {
    NS_ENSURE_TRUE(IsSupportedAudioMimeType(mInfo.mAudio.mMimeType),
                   false);

    mAudio.mDecoder =
      mPlatform->CreateDecoder(mInfo.mAudio,
                               mAudio.mTaskQueue,
                               mAudio.mCallback);
    NS_ENSURE_TRUE(mAudio.mDecoder != nullptr, false);
    nsresult rv = mAudio.mDecoder->Init();
    NS_ENSURE_SUCCESS(rv, false);
  }

  if (HasVideo() && !mVideo.mDecoder) {
    NS_ENSURE_TRUE(IsSupportedVideoMimeType(mInfo.mVideo.mMimeType),
                   false);

    if (mSharedDecoderManager &&
        mPlatform->SupportsSharedDecoders(mInfo.mVideo)) {
      mVideo.mDecoder =
        mSharedDecoderManager->CreateVideoDecoder(mPlatform,
                                                  mInfo.mVideo,
                                                  mLayersBackendType,
                                                  mDecoder->GetImageContainer(),
                                                  mVideo.mTaskQueue,
                                                  mVideo.mCallback);
    } else {
      mVideo.mDecoder =
        mPlatform->CreateDecoder(mInfo.mVideo,
                                 mVideo.mTaskQueue,
                                 mVideo.mCallback,
                                 mLayersBackendType,
                                 mDecoder->GetImageContainer());
    }
    NS_ENSURE_TRUE(mVideo.mDecoder != nullptr, false);
    nsresult rv = mVideo.mDecoder->Init();
    NS_ENSURE_SUCCESS(rv, false);
  }

  return true;
}

void
MediaFormatReader::ReadUpdatedMetadata(MediaInfo* aInfo)
{
  *aInfo = mInfo;
}

MediaFormatReader::DecoderData&
MediaFormatReader::GetDecoderData(TrackType aTrack)
{
  MOZ_ASSERT(aTrack == TrackInfo::kAudioTrack ||
             aTrack == TrackInfo::kVideoTrack);
  if (aTrack == TrackInfo::kAudioTrack) {
    return mAudio;
  }
  return mVideo;
}

void
MediaFormatReader::DisableHardwareAcceleration()
{
  MOZ_ASSERT(OnTaskQueue());
  if (HasVideo() && mSharedDecoderManager) {
    mSharedDecoderManager->DisableHardwareAcceleration();

    bool ok = mSharedDecoderManager->Recreate(mInfo.mVideo);
    MonitorAutoLock mon(mVideo.mMonitor);
    if (!ok) {
      mVideo.mError = true;
    }
    ScheduleUpdate(TrackInfo::kVideoTrack);
  }
}

bool
MediaFormatReader::ShouldSkip(bool aSkipToNextKeyframe, media::TimeUnit aTimeThreshold)
{
  MOZ_ASSERT(HasVideo());
  media::TimeUnit nextKeyframe;
  nsresult rv = mVideo.mTrackDemuxer->GetNextRandomAccessPoint(&nextKeyframe);
  if (NS_FAILED(rv)) {
    return aSkipToNextKeyframe;
  }
  return nextKeyframe < aTimeThreshold;
}

nsRefPtr<MediaDecoderReader::VideoDataPromise>
MediaFormatReader::RequestVideoData(bool aSkipToNextKeyframe,
                                     int64_t aTimeThreshold)
{
  MOZ_ASSERT(OnTaskQueue());
  MOZ_DIAGNOSTIC_ASSERT(mSeekPromise.IsEmpty(), "No sample requests allowed while seeking");
  MOZ_DIAGNOSTIC_ASSERT(!mVideo.HasPromise(), "No duplicate sample requests");
  MOZ_DIAGNOSTIC_ASSERT(!mVideoSeekRequest.Exists());
  MOZ_DIAGNOSTIC_ASSERT(!mAudioSeekRequest.Exists());
  MOZ_DIAGNOSTIC_ASSERT(!IsSeeking(), "called mid-seek");
  LOGV("RequestVideoData(%d, %lld)", aSkipToNextKeyframe, aTimeThreshold);

  if (!HasVideo()) {
    LOG("called with no video track");
    return VideoDataPromise::CreateAndReject(DECODE_ERROR, __func__);
  }

  if (IsSeeking()) {
    LOG("called mid-seek. Rejecting.");
    return VideoDataPromise::CreateAndReject(CANCELED, __func__);
  }

  if (mShutdown) {
    NS_WARNING("RequestVideoData on shutdown MediaFormatReader!");
    return VideoDataPromise::CreateAndReject(CANCELED, __func__);
  }

  if (!EnsureDecodersSetup()) {
    NS_WARNING("Error constructing decoders");
    return VideoDataPromise::CreateAndReject(DECODE_ERROR, __func__);
  }

  MOZ_ASSERT(HasVideo() && mPlatform && mVideo.mDecoder);

  media::TimeUnit timeThreshold{media::TimeUnit::FromMicroseconds(aTimeThreshold)};
  if (ShouldSkip(aSkipToNextKeyframe, timeThreshold)) {
    Flush(TrackInfo::kVideoTrack);
    nsRefPtr<VideoDataPromise> p = mVideo.mPromise.Ensure(__func__);
    SkipVideoDemuxToNextKeyFrame(timeThreshold);
    return p;
  }

  nsRefPtr<VideoDataPromise> p = mVideo.mPromise.Ensure(__func__);
  MonitorAutoLock lock(mVideo.mMonitor);
  ScheduleUpdate(TrackInfo::kVideoTrack);
  return p;
}

void
MediaFormatReader::OnDemuxFailed(TrackType aTrack, DemuxerFailureReason aFailure)
{
  MOZ_ASSERT(OnTaskQueue());
  LOG("Failed to demux %s, failure = %d",
      aTrack == TrackType::kVideoTrack ? "video" : "audio", aFailure);
  auto& decoder = GetDecoderData(aTrack);
  decoder.mDemuxRequest.Complete();
  MonitorAutoLock lock(decoder.mMonitor);
  switch (aFailure) {
    case DemuxerFailureReason::END_OF_STREAM:
      decoder.mDemuxEOS = true;
      ScheduleUpdate(aTrack);
      break;
    case DemuxerFailureReason::DEMUXER_ERROR:
      decoder.mError = true;
      ScheduleUpdate(aTrack);
      break;
    case DemuxerFailureReason::WAITING_FOR_DATA:
      decoder.mWaitingForData = true;
      ScheduleUpdate(aTrack);
      break;
    case DemuxerFailureReason::CANCELED:
    case DemuxerFailureReason::SHUTDOWN:
      if (decoder.HasPromise()) {
        decoder.RejectPromise(CANCELED, __func__);
      }
      break;
    default:
      MOZ_ASSERT(false);
      break;
  }
}

void
MediaFormatReader::DoDemuxVideo()
{
  
  mVideo.mDemuxRequest.Begin(mVideo.mTrackDemuxer->GetSamples(1)
                      ->RefableThen(GetTaskQueue(), __func__, this,
                                    &MediaFormatReader::OnVideoDemuxCompleted,
                                    &MediaFormatReader::OnVideoDemuxFailed));
}

void
MediaFormatReader::OnVideoDemuxCompleted(nsRefPtr<MediaTrackDemuxer::SamplesHolder> aSamples)
{
  LOGV("%d video samples demuxed", aSamples->mSamples.Length());
  mVideo.mDemuxRequest.Complete();
  mVideo.mQueuedSamples.AppendElements(aSamples->mSamples);
  MonitorAutoLock lock(mVideo.mMonitor);
  ScheduleUpdate(TrackInfo::kVideoTrack);
}

nsRefPtr<MediaDecoderReader::AudioDataPromise>
MediaFormatReader::RequestAudioData()
{
  MOZ_ASSERT(OnTaskQueue());
  MOZ_DIAGNOSTIC_ASSERT(mSeekPromise.IsEmpty(), "No sample requests allowed while seeking");
  MOZ_DIAGNOSTIC_ASSERT(!mVideoSeekRequest.Exists());
  MOZ_DIAGNOSTIC_ASSERT(!mAudioSeekRequest.Exists());
  MOZ_DIAGNOSTIC_ASSERT(!mAudio.HasPromise(), "No duplicate sample requests");
  MOZ_DIAGNOSTIC_ASSERT(!IsSeeking(), "called mid-seek");
  LOGV("");

  if (!HasAudio()) {
    LOG("called with no audio track");
    return AudioDataPromise::CreateAndReject(DECODE_ERROR, __func__);
  }

  if (IsSeeking()) {
    LOG("called mid-seek. Rejecting.");
    return AudioDataPromise::CreateAndReject(CANCELED, __func__);
  }

  if (mShutdown) {
    NS_WARNING("RequestAudioData on shutdown MediaFormatReader!");
    return AudioDataPromise::CreateAndReject(CANCELED, __func__);
  }

  if (!EnsureDecodersSetup()) {
    NS_WARNING("Error constructing decoders");
    return AudioDataPromise::CreateAndReject(DECODE_ERROR, __func__);
  }

  nsRefPtr<AudioDataPromise> p = mAudio.mPromise.Ensure(__func__);
  MonitorAutoLock lock(mAudio.mMonitor);
  ScheduleUpdate(TrackInfo::kAudioTrack);
  return p;
}

void
MediaFormatReader::DoDemuxAudio()
{
  
  mAudio.mDemuxRequest.Begin(mAudio.mTrackDemuxer->GetSamples(1)
                      ->RefableThen(GetTaskQueue(), __func__, this,
                                    &MediaFormatReader::OnAudioDemuxCompleted,
                                    &MediaFormatReader::OnAudioDemuxFailed));
}

void
MediaFormatReader::OnAudioDemuxCompleted(nsRefPtr<MediaTrackDemuxer::SamplesHolder> aSamples)
{
  LOGV("%d audio samples demuxed", aSamples->mSamples.Length());
  mAudio.mDemuxRequest.Complete();
  mAudio.mQueuedSamples.AppendElements(aSamples->mSamples);
  MonitorAutoLock lock(mAudio.mMonitor);
  ScheduleUpdate(TrackInfo::kAudioTrack);
}

bool
MediaFormatReader::NeedInput(DecoderData& aDecoder)
{
  aDecoder.mMonitor.AssertCurrentThreadOwns();
  
  
  
  
  
  return
    !aDecoder.mError &&
    aDecoder.HasPromise() &&
    aDecoder.mOutput.IsEmpty() &&
    (aDecoder.mInputExhausted || !aDecoder.mQueuedSamples.IsEmpty() ||
     aDecoder.mNumSamplesInput - aDecoder.mNumSamplesOutput < aDecoder.mDecodeAhead);
}

void
MediaFormatReader::ScheduleUpdate(TrackType aTrack)
{
  auto& decoder = GetDecoderData(aTrack);
  decoder.mMonitor.AssertCurrentThreadOwns();
  if (decoder.mUpdateScheduled) {
    return;
  }
  LOGV("SchedulingUpdate(%s)", TrackTypeToStr(aTrack));
  decoder.mUpdateScheduled = true;
  RefPtr<nsIRunnable> task(
    NS_NewRunnableMethodWithArg<TrackType>(this, &MediaFormatReader::Update, aTrack));
  GetTaskQueue()->Dispatch(task.forget());
}

bool
MediaFormatReader::UpdateReceivedNewData(TrackType aTrack)
{
  MOZ_ASSERT(OnTaskQueue());
  auto& decoder = GetDecoderData(aTrack);

  if (!decoder.mReceivedNewData) {
    return false;
  }
  decoder.mWaitingForData = false;
  bool hasLastEnd;
  media::TimeUnit lastEnd = decoder.mTimeRanges.GetEnd(&hasLastEnd);
  
  decoder.mTimeRanges = decoder.mTrackDemuxer->GetBuffered();
  if (decoder.mTimeRanges.Length() &&
      (!hasLastEnd || decoder.mTimeRanges.GetEnd() > lastEnd)) {
    
    decoder.mDemuxEOS = false;
    decoder.mDemuxEOSServiced = false;
  }
  decoder.mReceivedNewData = false;

  if (!decoder.mError && decoder.HasWaitingPromise()) {
    MOZ_ASSERT(!decoder.HasPromise());
    LOG("We have new data. Resolving WaitingPromise");
    decoder.mWaitingPromise.Resolve(decoder.mType, __func__);
    return true;
  }
  if (!decoder.mError && !mSeekPromise.IsEmpty()) {
    MOZ_ASSERT(!decoder.HasPromise());
    if (mVideoSeekRequest.Exists() || mAudioSeekRequest.Exists()) {
      
      return true;
    }
    LOG("Attempting Seek");
    AttemptSeek();
    return true;
  }
  return false;
}

void
MediaFormatReader::RequestDemuxSamples(TrackType aTrack)
{
  MOZ_ASSERT(OnTaskQueue());
  auto& decoder = GetDecoderData(aTrack);

  if (!decoder.mQueuedSamples.IsEmpty()) {
    
    return;
  }

  if (decoder.mDemuxRequest.Exists()) {
    
    return;
  }
  if (decoder.mDemuxEOS) {
    
    return;
  }
  if (aTrack == TrackInfo::kVideoTrack) {
    DoDemuxVideo();
  } else {
    DoDemuxAudio();
  }
}

void
MediaFormatReader::DecodeDemuxedSamples(TrackType aTrack,
                                        AbstractMediaDecoder::AutoNotifyDecoded& aA)
{
  MOZ_ASSERT(OnTaskQueue());
  auto& decoder = GetDecoderData(aTrack);

  if (decoder.mQueuedSamples.IsEmpty()) {
    return;
  }

  
  for (auto& samples : decoder.mQueuedSamples) {
    decoder.mDecoder->Input(samples);
  }
  {
    MonitorAutoLock lock(decoder.mMonitor);
    decoder.mNumSamplesInput += decoder.mQueuedSamples.Length();
  }
  if (aTrack == TrackInfo::kVideoTrack) {
    aA.mParsed += decoder.mQueuedSamples.Length();
  }
  decoder.mQueuedSamples.Clear();
}

void
MediaFormatReader::Update(TrackType aTrack)
{
  MOZ_ASSERT(OnTaskQueue());

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

    if (UpdateReceivedNewData(aTrack)) {
      return;
    }

    if (decoder.HasPromise()) {
      
      if (decoder.mError) {
        LOG("Decoding Error");
        decoder.RejectPromise(DECODE_ERROR, __func__);
        return;
      }
      if (decoder.mWaitingForData) {
        LOG("Waiting For Data");
        decoder.RejectPromise(WAITING_FOR_DATA, __func__);
      }
    }

    if (NeedInput(decoder)) {
      needInput = true;
      decoder.mInputExhausted = false;
    }
    if (aTrack == TrackInfo::kVideoTrack) {
      uint64_t delta =
        decoder.mNumSamplesOutput - mLastReportedNumDecodedFrames;
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
        decoder.mDrainComplete = false;
      }
    }
  }

  LOGV("Update(%s) ni=%d no=%d iex=%d fl=%d",
       TrackTypeToStr(aTrack),
       needInput,
       needOutput,
       decoder.mInputExhausted,
       decoder.mIsFlushing);

  if (decoder.mDemuxEOS && !decoder.mDemuxEOSServiced) {
    decoder.mDecoder->Drain();
    decoder.mDemuxEOSServiced = true;
  }

  if (!needInput) {
    return;
  }

  
  RequestDemuxSamples(aTrack);
  
  DecodeDemuxedSamples(aTrack, a);
}

void
MediaFormatReader::ReturnOutput(MediaData* aData, TrackType aTrack)
{
  auto& decoder = GetDecoderData(aTrack);
  decoder.mMonitor.AssertCurrentThreadOwns();
  MOZ_ASSERT(decoder.HasPromise());
  if (decoder.mDiscontinuity) {
    decoder.mDiscontinuity = false;
    aData->mDiscontinuity = true;
  }

  if (aTrack == TrackInfo::kAudioTrack) {
    AudioData* audioData = static_cast<AudioData*>(aData);

    if (audioData->mChannels != mInfo.mAudio.mChannels ||
        audioData->mRate != mInfo.mAudio.mRate) {
      LOG("change of audio format (rate:%d->%d). "
          "This is an unsupported configuration",
          mInfo.mAudio.mRate, audioData->mRate);
      mInfo.mAudio.mRate = audioData->mRate;
      mInfo.mAudio.mChannels = audioData->mChannels;
    }
    mAudio.mPromise.Resolve(audioData, __func__);
  } else if (aTrack == TrackInfo::kVideoTrack) {
    mVideo.mPromise.Resolve(static_cast<VideoData*>(aData), __func__);
  }
}

size_t
MediaFormatReader::SizeOfVideoQueueInFrames()
{
  return SizeOfQueue(TrackInfo::kVideoTrack);
}

size_t
MediaFormatReader::SizeOfAudioQueueInFrames()
{
  return SizeOfQueue(TrackInfo::kAudioTrack);
}

size_t
MediaFormatReader::SizeOfQueue(TrackType aTrack)
{
  auto& decoder = GetDecoderData(aTrack);
  MonitorAutoLock lock(decoder.mMonitor);
  return decoder.mOutput.Length() + (decoder.mNumSamplesInput - decoder.mNumSamplesOutput);
}

nsRefPtr<MediaDecoderReader::WaitForDataPromise>
MediaFormatReader::WaitForData(MediaData::Type aType)
{
  MOZ_ASSERT(OnTaskQueue());
  TrackType trackType = aType == MediaData::VIDEO_DATA ?
    TrackType::kVideoTrack : TrackType::kAudioTrack;
  auto& decoder = GetDecoderData(trackType);
  nsRefPtr<WaitForDataPromise> p = decoder.mWaitingPromise.Ensure(__func__);
  MonitorAutoLock lock(decoder.mMonitor);
  ScheduleUpdate(trackType);
  return p;
}

nsresult
MediaFormatReader::ResetDecode()
{
  MOZ_ASSERT(OnTaskQueue());

  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  mAudioSeekRequest.DisconnectIfExists();
  mVideoSeekRequest.DisconnectIfExists();
  mSeekPromise.RejectIfExists(NS_OK, __func__);
  mSkipRequest.DisconnectIfExists();

  
  mAudio.mWaitingPromise.RejectIfExists(WaitForDataRejectValue(MediaData::AUDIO_DATA, WaitForDataRejectValue::CANCELED), __func__);
  mVideo.mWaitingPromise.RejectIfExists(WaitForDataRejectValue(MediaData::VIDEO_DATA, WaitForDataRejectValue::CANCELED), __func__);

  
  mPendingSeekTime.reset();

  if (HasVideo()) {
    Flush(TrackInfo::kVideoTrack);
  }
  if (HasAudio()) {
    Flush(TrackInfo::kAudioTrack);
  }
  return MediaDecoderReader::ResetDecode();
}

void
MediaFormatReader::Output(TrackType aTrack, MediaData* aSample)
{
  LOGV("Decoded %s sample time=%lld dur=%lld",
       TrackTypeToStr(aTrack), aSample->mTime, aSample->mDuration);

  if (!aSample) {
    NS_WARNING("MediaFormatReader::Output() passed a null sample");
    Error(aTrack);
    return;
  }

  auto& decoder = GetDecoderData(aTrack);
  
  MonitorAutoLock mon(decoder.mMonitor);
  if (decoder.mIsFlushing) {
    LOG("MediaFormatReader produced output while flushing, discarding.");
    mon.NotifyAll();
    return;
  }

  decoder.mOutput.AppendElement(aSample);
  decoder.mNumSamplesOutput++;
  ScheduleUpdate(aTrack);
}

void
MediaFormatReader::DrainComplete(TrackType aTrack)
{
  DecoderData& data = GetDecoderData(aTrack);
  MonitorAutoLock mon(data.mMonitor);
  data.mDrainComplete = true;
  ScheduleUpdate(aTrack);
}

void
MediaFormatReader::InputExhausted(TrackType aTrack)
{
  DecoderData& data = GetDecoderData(aTrack);
  MonitorAutoLock mon(data.mMonitor);
  data.mInputExhausted = true;
  ScheduleUpdate(aTrack);
}

void
MediaFormatReader::Error(TrackType aTrack)
{
  DecoderData& data = GetDecoderData(aTrack);
  {
    MonitorAutoLock mon(data.mMonitor);
    data.mError = true;
    ScheduleUpdate(aTrack);
  }
}

void
MediaFormatReader::Flush(TrackType aTrack)
{
  MOZ_ASSERT(OnTaskQueue());
  LOG("Flush(%s) BEGIN", TrackTypeToStr(aTrack));

  auto& decoder = GetDecoderData(aTrack);
  if (!decoder.mDecoder) {
    return;
  }

  
  decoder.mDemuxRequest.DisconnectIfExists();
  decoder.mTrackDemuxer->Reset();

  
  
  
  {
    MonitorAutoLock mon(decoder.mMonitor);
    decoder.mIsFlushing = true;
  }
  decoder.mDecoder->Flush();
  decoder.ResetState();
  if (decoder.HasPromise()) {
    decoder.RejectPromise(CANCELED, __func__);
  }
  LOG("Flush(%s) END", TrackTypeToStr(aTrack));
}

void
MediaFormatReader::SkipVideoDemuxToNextKeyFrame(media::TimeUnit aTimeThreshold)
{
  MOZ_ASSERT(OnTaskQueue());

  MOZ_ASSERT(mVideo.mDecoder);
  MOZ_ASSERT(mVideo.HasPromise());
  LOG("Skipping up to %lld", aTimeThreshold.ToMicroseconds());

  if (mVideo.mError) {
    mVideo.RejectPromise(DECODE_ERROR, __func__);
    return;
  }

  mSkipRequest.Begin(mVideo.mTrackDemuxer->SkipToNextRandomAccessPoint(aTimeThreshold)
                          ->RefableThen(GetTaskQueue(), __func__, this,
                                        &MediaFormatReader::OnVideoSkipCompleted,
                                        &MediaFormatReader::OnVideoSkipFailed));
  return;
}

void
MediaFormatReader::OnVideoSkipCompleted(uint32_t aSkipped)
{
  MOZ_ASSERT(OnTaskQueue());
  LOG("Skipping succeeded, skipped %u frames", aSkipped);
  mSkipRequest.Complete();
  mDecoder->NotifyDecodedFrames(aSkipped, 0, aSkipped);
  MonitorAutoLock lock(mVideo.mMonitor);
  MOZ_ASSERT(!mVideo.mError); 
                              
  ScheduleUpdate(TrackInfo::kVideoTrack);
}

void
MediaFormatReader::OnVideoSkipFailed(MediaTrackDemuxer::SkipFailureHolder aFailure)
{
  MOZ_ASSERT(OnTaskQueue());
  LOG("Skipping failed, skipped %u frames", aFailure.mSkipped);
  mSkipRequest.Complete();
  mDecoder->NotifyDecodedFrames(aFailure.mSkipped, 0, aFailure.mSkipped);
  MOZ_ASSERT(mVideo.HasPromise());
  if (aFailure.mFailure == DemuxerFailureReason::END_OF_STREAM) {
    mVideo.RejectPromise(END_OF_STREAM, __func__);
  } else {
    mVideo.RejectPromise(DECODE_ERROR, __func__);
  }
}

nsRefPtr<MediaDecoderReader::SeekPromise>
MediaFormatReader::Seek(int64_t aTime, int64_t aUnused)
{
  MOZ_ASSERT(OnTaskQueue());

  LOG("aTime=(%lld)", aTime);

  MOZ_DIAGNOSTIC_ASSERT(mSeekPromise.IsEmpty());
  MOZ_DIAGNOSTIC_ASSERT(!mVideo.HasPromise());
  MOZ_DIAGNOSTIC_ASSERT(!mAudio.HasPromise());
  MOZ_DIAGNOSTIC_ASSERT(mPendingSeekTime.isNothing());

  if (!mSeekable) {
    LOG("Seek() END (Unseekable)");
    return SeekPromise::CreateAndReject(NS_ERROR_FAILURE, __func__);
  }

  if (mShutdown) {
    return SeekPromise::CreateAndReject(NS_ERROR_FAILURE, __func__);
  }

  mPendingSeekTime.emplace(media::TimeUnit::FromMicroseconds(aTime));

  nsRefPtr<SeekPromise> p = mSeekPromise.Ensure(__func__);

  AttemptSeek();

  return p;
}

void
MediaFormatReader::AttemptSeek()
{
  MOZ_ASSERT(OnTaskQueue());
  if (HasVideo()) {
    DoVideoSeek();
  } else if (HasAudio()) {
    DoAudioSeek();
  } else {
    MOZ_CRASH();
  }
}

void
MediaFormatReader::OnSeekFailed(TrackType aTrack, DemuxerFailureReason aResult)
{
  MOZ_ASSERT(OnTaskQueue());
  if (aTrack == TrackType::kVideoTrack) {
    mVideoSeekRequest.Complete();
  } else {
    mAudioSeekRequest.Complete();
  }
  auto& decoder = GetDecoderData(aTrack);

  if (aResult == DemuxerFailureReason::WAITING_FOR_DATA) {
    MonitorAutoLock lock(decoder.mMonitor);
    decoder.mWaitingForData = true;
    return;
  }
  MOZ_ASSERT(!mVideoSeekRequest.Exists() && !mAudioSeekRequest.Exists());
  mPendingSeekTime.reset();
  mSeekPromise.Reject(NS_ERROR_FAILURE, __func__);
}

void
MediaFormatReader::DoVideoSeek()
{
  MOZ_ASSERT(mPendingSeekTime.isSome());
  media::TimeUnit seekTime = mPendingSeekTime.ref();
  mVideoSeekRequest.Begin(mVideo.mTrackDemuxer->Seek(seekTime)
                          ->RefableThen(GetTaskQueue(), __func__, this,
                                        &MediaFormatReader::OnVideoSeekCompleted,
                                        &MediaFormatReader::OnVideoSeekFailed));
}

void
MediaFormatReader::OnVideoSeekCompleted(media::TimeUnit aTime)
{
  MOZ_ASSERT(OnTaskQueue());
  mVideoSeekRequest.Complete();

  if (HasAudio()) {
    MOZ_ASSERT(mPendingSeekTime.isSome());
    mPendingSeekTime.ref() = aTime;
    DoAudioSeek();
  } else {
    mPendingSeekTime.reset();
    mSeekPromise.Resolve(aTime.ToMicroseconds(), __func__);
  }
}

void
MediaFormatReader::DoAudioSeek()
{
  MOZ_ASSERT(mPendingSeekTime.isSome());
  media::TimeUnit seekTime = mPendingSeekTime.ref();
  mAudioSeekRequest.Begin(mAudio.mTrackDemuxer->Seek(seekTime)
                         ->RefableThen(GetTaskQueue(), __func__, this,
                                       &MediaFormatReader::OnAudioSeekCompleted,
                                       &MediaFormatReader::OnAudioSeekFailed));
}

void
MediaFormatReader::OnAudioSeekCompleted(media::TimeUnit aTime)
{
  MOZ_ASSERT(OnTaskQueue());
  mAudioSeekRequest.Complete();
  mPendingSeekTime.reset();
  mSeekPromise.Resolve(aTime.ToMicroseconds(), __func__);
}

int64_t
MediaFormatReader::GetEvictionOffset(double aTime)
{
  int64_t audioOffset;
  int64_t videoOffset;
  if (NS_IsMainThread()) {
    audioOffset = HasAudio() ? mAudioTrackDemuxer->GetEvictionOffset(media::TimeUnit::FromSeconds(aTime)) : INT64_MAX;
    videoOffset = HasVideo() ? mVideoTrackDemuxer->GetEvictionOffset(media::TimeUnit::FromSeconds(aTime)) : INT64_MAX;
  } else {
    MOZ_ASSERT(OnTaskQueue());
    audioOffset = HasAudio() ? mAudio.mTrackDemuxer->GetEvictionOffset(media::TimeUnit::FromSeconds(aTime)) : INT64_MAX;
    videoOffset = HasVideo() ? mVideo.mTrackDemuxer->GetEvictionOffset(media::TimeUnit::FromSeconds(aTime)) : INT64_MAX;
  }
  return std::min(audioOffset, videoOffset);
}

nsresult
MediaFormatReader::GetBuffered(dom::TimeRanges* aBuffered)
{
  media::TimeIntervals videoti;
  media::TimeIntervals audioti;

  if (NS_IsMainThread()) {
    if (mVideoTrackDemuxer) {
      videoti = mVideoTrackDemuxer->GetBuffered();
    }
    if (mAudioTrackDemuxer) {
      audioti = mAudioTrackDemuxer->GetBuffered();
    }
  } else {
    if (HasVideo()) {
      MonitorAutoLock lock(mVideo.mMonitor);
      videoti = mVideo.mTimeRanges;
    }
    if (HasAudio()) {
      MonitorAutoLock lock(mAudio.mMonitor);
      audioti = mAudio.mTimeRanges;
    }
  }
  if (HasAudio() && HasVideo()) {
    videoti.Intersection(audioti);
    videoti.ToTimeRanges(aBuffered);
  } else if (HasAudio()) {
    audioti.ToTimeRanges(aBuffered);
  } else if (HasVideo()) {
    videoti.ToTimeRanges(aBuffered);
  }

  return NS_OK;
}

bool MediaFormatReader::IsDormantNeeded()
{
#if defined(READER_DORMANT_HEURISTIC)
  return mDormantEnabled;
#else
  return false;
#endif
}

void MediaFormatReader::ReleaseMediaResources()
{
  
  
  VideoFrameContainer* container = mDecoder->GetVideoFrameContainer();
  if (container) {
    container->ClearCurrentFrame();
  }
  if (mVideo.mDecoder) {
    mVideo.mDecoder->Shutdown();
    mVideo.mDecoder = nullptr;
  }
}

void MediaFormatReader::NotifyResourcesStatusChanged()
{
  if (mDecoder) {
    mDecoder->NotifyWaitingForResourcesStatusChanged();
  }
}

void
MediaFormatReader::SetIdle()
{
  if (mSharedDecoderManager && mVideo.mDecoder) {
    mSharedDecoderManager->SetIdle(mVideo.mDecoder);
    NotifyResourcesStatusChanged();
  }
}

void
MediaFormatReader::SetSharedDecoderManager(SharedDecoderManager* aManager)
{
#if !defined(MOZ_WIDGET_ANDROID)
  mSharedDecoderManager = aManager;
#endif
}

bool
MediaFormatReader::VideoIsHardwareAccelerated() const
{
  return mVideo.mDecoder && mVideo.mDecoder->IsHardwareAccelerated();
}

void
MediaFormatReader::NotifyDemuxer(uint32_t aLength, int64_t aOffset)
{
  MOZ_ASSERT(OnTaskQueue());

  if (mShutdown) {
    return;
  }

  mDemuxer->NotifyDataArrived(aLength, aOffset);
  if (HasVideo()) {
    mVideo.mReceivedNewData = true;
    MonitorAutoLock lock(mVideo.mMonitor);
    ScheduleUpdate(TrackType::kVideoTrack);
  }
  if (HasAudio()) {
    mAudio.mReceivedNewData = true;
    MonitorAutoLock lock(mAudio.mMonitor);
    ScheduleUpdate(TrackType::kAudioTrack);
  }
}

void
MediaFormatReader::NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mInitDone) {
    return;
  }

  mMainThreadDemuxer->NotifyDataArrived(aLength, aOffset);

  
  RefPtr<nsIRunnable> task =
    NS_NewRunnableMethodWithArgs<int32_t, uint64_t>(
      this, &MediaFormatReader::NotifyDemuxer,
      aLength, aOffset);
  GetTaskQueue()->Dispatch(task);
}

} 
