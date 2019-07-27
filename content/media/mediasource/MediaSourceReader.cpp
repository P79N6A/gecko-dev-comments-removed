




#include "MediaSourceReader.h"

#include "prlog.h"
#include "mozilla/dom/TimeRanges.h"
#include "DecoderTraits.h"
#include "MediaDataDecodedListener.h"
#include "MediaDecoderOwner.h"
#include "MediaSource.h"
#include "MediaSourceDecoder.h"
#include "MediaSourceUtils.h"
#include "SourceBufferDecoder.h"
#include "TrackBuffer.h"

#ifdef MOZ_FMP4
#include "MP4Decoder.h"
#include "MP4Reader.h"
#endif

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetMediaSourceLog();
extern PRLogModuleInfo* GetMediaSourceAPILog();

#define MSE_DEBUG(...) PR_LOG(GetMediaSourceLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#define MSE_DEBUGV(...) PR_LOG(GetMediaSourceLog(), PR_LOG_DEBUG+1, (__VA_ARGS__))
#define MSE_API(...) PR_LOG(GetMediaSourceAPILog(), PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define MSE_DEBUG(...)
#define MSE_DEBUGV(...)
#define MSE_API(...)
#endif

namespace mozilla {

MediaSourceReader::MediaSourceReader(MediaSourceDecoder* aDecoder)
  : MediaDecoderReader(aDecoder)
  , mLastAudioTime(-1)
  , mLastVideoTime(-1)
  , mTimeThreshold(-1)
  , mDropAudioBeforeThreshold(false)
  , mDropVideoBeforeThreshold(false)
  , mEnded(false)
  , mAudioIsSeeking(false)
  , mVideoIsSeeking(false)
  , mHasEssentialTrackBuffers(false)
{
}

void
MediaSourceReader::PrepareInitialization()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  MSE_DEBUG("MediaSourceReader(%p)::PrepareInitialization trackBuffers=%u",
            this, mTrackBuffers.Length());
  mEssentialTrackBuffers.AppendElements(mTrackBuffers);
  mHasEssentialTrackBuffers = true;
  mDecoder->NotifyWaitingForResourcesStatusChanged();
}

bool
MediaSourceReader::IsWaitingMediaResources()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  for (uint32_t i = 0; i < mEssentialTrackBuffers.Length(); ++i) {
    if (!mEssentialTrackBuffers[i]->IsReady()) {
      return true;
    }
  }

  return !mHasEssentialTrackBuffers;
}

void
MediaSourceReader::RequestAudioData()
{
  MSE_DEBUGV("MediaSourceReader(%p)::RequestAudioData", this);
  if (!mAudioReader) {
    MSE_DEBUG("MediaSourceReader(%p)::RequestAudioData called with no audio reader", this);
    GetCallback()->OnDecodeError();
    return;
  }
  if (SwitchAudioReader(double(mLastAudioTime) / USECS_PER_S)) {
    MSE_DEBUGV("MediaSourceReader(%p)::RequestAudioData switching audio reader", this);
  }
  mAudioReader->RequestAudioData();
}

void
MediaSourceReader::OnAudioDecoded(AudioData* aSample)
{
  MSE_DEBUGV("MediaSourceReader(%p)::OnAudioDecoded mTime=%lld mDuration=%lld d=%d",
             this, aSample->mTime, aSample->mDuration, aSample->mDiscontinuity);
  if (mDropAudioBeforeThreshold) {
    if (aSample->mTime < mTimeThreshold) {
      MSE_DEBUG("MediaSourceReader(%p)::OnAudioDecoded mTime=%lld < mTimeThreshold=%lld",
                this, aSample->mTime, mTimeThreshold);
      delete aSample;
      mAudioReader->RequestAudioData();
      return;
    }
    mDropAudioBeforeThreshold = false;
  }

  
  
  
  if (mAudioIsSeeking) {
    mAudioIsSeeking = false;
    aSample->mDiscontinuity = true;
  }
  mLastAudioTime = aSample->mTime + aSample->mDuration;
  GetCallback()->OnAudioDecoded(aSample);
}

void
MediaSourceReader::OnAudioEOS()
{
  MSE_DEBUG("MediaSourceReader(%p)::OnAudioEOS reader=%p (decoders=%u)",
            this, mAudioReader.get(), mAudioTrack->Decoders().Length());
  if (SwitchAudioReader(double(mLastAudioTime) / USECS_PER_S)) {
    
    RequestAudioData();
  } else if (IsEnded()) {
    
    MSE_DEBUG("MediaSourceReader(%p)::OnAudioEOS reader=%p EOS (decoders=%u)",
              this, mAudioReader.get(), mAudioTrack->Decoders().Length());
    GetCallback()->OnAudioEOS();
  }
}

void
MediaSourceReader::RequestVideoData(bool aSkipToNextKeyframe, int64_t aTimeThreshold)
{
  MSE_DEBUGV("MediaSourceReader(%p)::RequestVideoData(%d, %lld)",
             this, aSkipToNextKeyframe, aTimeThreshold);
  if (!mVideoReader) {
    MSE_DEBUG("MediaSourceReader(%p)::RequestVideoData called with no video reader", this);
    GetCallback()->OnDecodeError();
    return;
  }
  if (aSkipToNextKeyframe) {
    mTimeThreshold = aTimeThreshold;
    mDropAudioBeforeThreshold = true;
    mDropVideoBeforeThreshold = true;
  }
  if (SwitchVideoReader(double(mLastVideoTime) / USECS_PER_S)) {
    MSE_DEBUGV("MediaSourceReader(%p)::RequestVideoData switching video reader", this);
  }
  mVideoReader->RequestVideoData(aSkipToNextKeyframe, aTimeThreshold);
}

void
MediaSourceReader::OnVideoDecoded(VideoData* aSample)
{
  MSE_DEBUGV("MediaSourceReader(%p)::OnVideoDecoded mTime=%lld mDuration=%lld d=%d",
             this, aSample->mTime, aSample->mDuration, aSample->mDiscontinuity);
  if (mDropVideoBeforeThreshold) {
    if (aSample->mTime < mTimeThreshold) {
      MSE_DEBUG("MediaSourceReader(%p)::OnVideoDecoded mTime=%lld < mTimeThreshold=%lld",
                this, aSample->mTime, mTimeThreshold);
      delete aSample;
      mVideoReader->RequestVideoData(false, 0);
      return;
    }
    mDropVideoBeforeThreshold = false;
  }

  
  
  
  if (mVideoIsSeeking) {
    mVideoIsSeeking = false;
    aSample->mDiscontinuity = true;
  }
  mLastVideoTime = aSample->mTime + aSample->mDuration;
  GetCallback()->OnVideoDecoded(aSample);
}

void
MediaSourceReader::OnVideoEOS()
{
  
  MSE_DEBUG("MediaSourceReader(%p)::OnVideoEOS reader=%p (decoders=%u)",
            this, mVideoReader.get(), mVideoTrack->Decoders().Length());
  if (SwitchVideoReader(double(mLastVideoTime) / USECS_PER_S)) {
    
    RequestVideoData(false, 0);
  } else if (IsEnded()) {
    
    MSE_DEBUG("MediaSourceReader(%p)::OnVideoEOS reader=%p EOS (decoders=%u)",
              this, mVideoReader.get(), mVideoTrack->Decoders().Length());
    GetCallback()->OnVideoEOS();
  }
}

void
MediaSourceReader::OnDecodeError()
{
  MSE_DEBUG("MediaSourceReader(%p)::OnDecodeError", this);
  GetCallback()->OnDecodeError();
}

void
MediaSourceReader::Shutdown()
{
  MediaDecoderReader::Shutdown();
  mAudioTrack = nullptr;
  mAudioReader = nullptr;
  mVideoTrack = nullptr;
  mVideoReader = nullptr;
  for (uint32_t i = 0; i < mTrackBuffers.Length(); ++i) {
    mTrackBuffers[i]->Shutdown();
  }
  mTrackBuffers.Clear();
}

void
MediaSourceReader::BreakCycles()
{
  MediaDecoderReader::BreakCycles();
  mAudioTrack = nullptr;
  mAudioReader = nullptr;
  mVideoTrack = nullptr;
  mVideoReader = nullptr;
  for (uint32_t i = 0; i < mTrackBuffers.Length(); ++i) {
    mTrackBuffers[i]->BreakCycles();
  }
  mTrackBuffers.Clear();
}

bool
MediaSourceReader::CanSelectAudioReader(MediaDecoderReader* aNewReader)
{
  AudioInfo currentInfo = mAudioReader->GetMediaInfo().mAudio;
  AudioInfo newInfo = aNewReader->GetMediaInfo().mAudio;

  
  if (currentInfo.mRate != newInfo.mRate ||
      currentInfo.mChannels != newInfo.mChannels) {
    MSE_DEBUGV("MediaSourceReader(%p)::CanSelectAudioReader(%p) skip reader due to format mismatch",
               this, aNewReader);
    return false;
  }

  if (aNewReader->AudioQueue().AtEndOfStream()) {
    MSE_DEBUGV("MediaSourceReader(%p)::CanSelectAudioReader(%p) skip reader due to queue EOS",
               this, aNewReader);
    return false;
  }

  return true;
}

bool
MediaSourceReader::CanSelectVideoReader(MediaDecoderReader* aNewReader)
{
  if (aNewReader->VideoQueue().AtEndOfStream()) {
    MSE_DEBUGV("MediaSourceReader(%p)::CanSelectVideoReader(%p) skip reader due to queue EOS",
               this, aNewReader);
    return false;
  }

  return true;
}

already_AddRefed<MediaDecoderReader>
MediaSourceReader::SelectReader(double aTarget,
                                bool (MediaSourceReader::*aCanSelectReader)(MediaDecoderReader*),
                                const nsTArray<nsRefPtr<SourceBufferDecoder>>& aTrackDecoders)
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  
  
  for (int32_t i = aTrackDecoders.Length() - 1; i >= 0; --i) {
    nsRefPtr<MediaDecoderReader> newReader = aTrackDecoders[i]->GetReader();

    
    
    
    if (!(this->*aCanSelectReader)(newReader)) {
      continue;
    }

    nsRefPtr<dom::TimeRanges> ranges = new dom::TimeRanges();
    aTrackDecoders[i]->GetBuffered(ranges);
    if (ranges->Find(aTarget) == dom::TimeRanges::NoIndex) {
      MSE_DEBUGV("MediaSourceReader(%p)::SelectReader(%f) newReader=%p target not in ranges=%s",
                 this, aTarget, newReader.get(), DumpTimeRanges(ranges).get());
      continue;
    }

    return newReader.forget();
  }

  return nullptr;
}

bool
MediaSourceReader::SwitchAudioReader(double aTarget)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  
  if (!mAudioTrack) {
    return false;
  }
  nsRefPtr<MediaDecoderReader> newReader = SelectReader(aTarget,
                                                        &MediaSourceReader::CanSelectAudioReader,
                                                        mAudioTrack->Decoders());
  if (newReader) {
    mAudioReader->SetIdle();
    mAudioReader = newReader;
  }
  return newReader != nullptr;
}

bool
MediaSourceReader::SwitchVideoReader(double aTarget)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  
  if (!mVideoTrack) {
    return false;
  }
  nsRefPtr<MediaDecoderReader> newReader = SelectReader(aTarget,
                                                        &MediaSourceReader::CanSelectVideoReader,
                                                        mVideoTrack->Decoders());
  if (newReader) {
    mVideoReader->SetIdle();
    mVideoReader = newReader;
  }
  return newReader != nullptr;
}

MediaDecoderReader*
CreateReaderForType(const nsACString& aType, AbstractMediaDecoder* aDecoder)
{
#ifdef MOZ_FMP4
  
  
  
  
  if ((aType.LowerCaseEqualsLiteral("video/mp4") ||
       aType.LowerCaseEqualsLiteral("audio/mp4")) &&
      MP4Decoder::IsEnabled()) {
    return new MP4Reader(aDecoder);
  }
#endif
  return DecoderTraits::CreateReader(aType, aDecoder);
}

already_AddRefed<SourceBufferDecoder>
MediaSourceReader::CreateSubDecoder(const nsACString& aType)
{
  if (IsShutdown()) {
    return nullptr;
  }
  MOZ_ASSERT(GetTaskQueue());
  nsRefPtr<SourceBufferDecoder> decoder =
    new SourceBufferDecoder(new SourceBufferResource(aType), mDecoder);
  nsRefPtr<MediaDecoderReader> reader(CreateReaderForType(aType, decoder));
  if (!reader) {
    return nullptr;
  }
  
  
  
  RefPtr<MediaDataDecodedListener<MediaSourceReader>> callback =
    new MediaDataDecodedListener<MediaSourceReader>(this, GetTaskQueue());
  reader->SetCallback(callback);
  reader->SetTaskQueue(GetTaskQueue());
  reader->Init(nullptr);

  MSE_DEBUG("MediaSourceReader(%p)::CreateSubDecoder subdecoder %p subreader %p",
            this, decoder.get(), reader.get());
  decoder->SetReader(reader);
  return decoder.forget();
}

void
MediaSourceReader::AddTrackBuffer(TrackBuffer* aTrackBuffer)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  MSE_DEBUG("MediaSourceReader(%p)::AddTrackBuffer %p", this, aTrackBuffer);
  mTrackBuffers.AppendElement(aTrackBuffer);
}

void
MediaSourceReader::RemoveTrackBuffer(TrackBuffer* aTrackBuffer)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  MSE_DEBUG("MediaSourceReader(%p)::RemoveTrackBuffer %p", this, aTrackBuffer);
  mTrackBuffers.RemoveElement(aTrackBuffer);
  if (mAudioTrack == aTrackBuffer) {
    mAudioTrack = nullptr;
  }
  if (mVideoTrack == aTrackBuffer) {
    mVideoTrack = nullptr;
  }
}

void
MediaSourceReader::OnTrackBufferConfigured(TrackBuffer* aTrackBuffer, const MediaInfo& aInfo)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  MOZ_ASSERT(aTrackBuffer->IsReady());
  MOZ_ASSERT(mTrackBuffers.Contains(aTrackBuffer));
  if (aInfo.HasAudio() && !mAudioTrack) {
    MSE_DEBUG("MediaSourceReader(%p)::OnTrackBufferConfigured %p audio", this, aTrackBuffer);
    mAudioTrack = aTrackBuffer;
  }
  if (aInfo.HasVideo() && !mVideoTrack) {
    MSE_DEBUG("MediaSourceReader(%p)::OnTrackBufferConfigured %p video", this, aTrackBuffer);
    mVideoTrack = aTrackBuffer;
  }
  mDecoder->NotifyWaitingForResourcesStatusChanged();
}

class ChangeToHaveMetadata : public nsRunnable {
public:
  explicit ChangeToHaveMetadata(AbstractMediaDecoder* aDecoder) :
    mDecoder(aDecoder)
  {
  }

  NS_IMETHOD Run() MOZ_OVERRIDE MOZ_FINAL {
    auto owner = mDecoder->GetOwner();
    if (owner) {
      owner->UpdateReadyStateForData(MediaDecoderOwner::NEXT_FRAME_WAIT_FOR_MSE_DATA);
    }
    return NS_OK;
  }

private:
  nsRefPtr<AbstractMediaDecoder> mDecoder;
};

bool
MediaSourceReader::TrackBuffersContainTime(double aTime)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (mAudioTrack && !mAudioTrack->ContainsTime(aTime)) {
    return false;
  }
  if (mVideoTrack && !mVideoTrack->ContainsTime(aTime)) {
    return false;
  }
  return true;
}

nsresult
MediaSourceReader::Seek(int64_t aTime, int64_t aStartTime, int64_t aEndTime,
                        int64_t aCurrentTime)
{
  MSE_DEBUG("MediaSourceReader(%p)::Seek(aTime=%lld, aStart=%lld, aEnd=%lld, aCurrent=%lld)",
            this, aTime, aStartTime, aEndTime, aCurrentTime);

  ResetDecode();
  for (uint32_t i = 0; i < mTrackBuffers.Length(); ++i) {
    mTrackBuffers[i]->ResetDecode();
  }

  
  mLastAudioTime = aTime;
  mLastVideoTime = aTime;

  double target = static_cast<double>(aTime) / USECS_PER_S;
  if (!TrackBuffersContainTime(target)) {
    MSE_DEBUG("MediaSourceReader(%p)::Seek no active buffer contains target=%f", this, target);
    NS_DispatchToMainThread(new ChangeToHaveMetadata(mDecoder));
  }

  
  
  
  
  while (!TrackBuffersContainTime(target) && !IsShutdown() && !IsEnded()) {
    MSE_DEBUG("MediaSourceReader(%p)::Seek waiting for target=%f", this, target);
    static_cast<MediaSourceDecoder*>(mDecoder)->WaitForData();
  }

  if (IsShutdown()) {
    return NS_ERROR_FAILURE;
  }

  if (mAudioTrack) {
    mAudioIsSeeking = true;
    DebugOnly<bool> ok = SwitchAudioReader(target);
    MOZ_ASSERT(ok && static_cast<SourceBufferDecoder*>(mAudioReader->GetDecoder())->ContainsTime(target));
    nsresult rv = mAudioReader->Seek(aTime, aStartTime, aEndTime, aCurrentTime);
    MSE_DEBUG("MediaSourceReader(%p)::Seek audio reader=%p rv=%x", this, mAudioReader.get(), rv);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }
  if (mVideoTrack) {
    mVideoIsSeeking = true;
    DebugOnly<bool> ok = SwitchVideoReader(target);
    MOZ_ASSERT(ok && static_cast<SourceBufferDecoder*>(mVideoReader->GetDecoder())->ContainsTime(target));
    nsresult rv = mVideoReader->Seek(aTime, aStartTime, aEndTime, aCurrentTime);
    MSE_DEBUG("MediaSourceReader(%p)::Seek video reader=%p rv=%x", this, mVideoReader.get(), rv);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }
  return NS_OK;
}

nsresult
MediaSourceReader::ReadMetadata(MediaInfo* aInfo, MetadataTags** aTags)
{
  bool waiting = IsWaitingMediaResources();
  MSE_DEBUG("MediaSourceReader(%p)::ReadMetadata waiting=%d tracks=%u/%u audio=%p video=%p",
            this, waiting, mEssentialTrackBuffers.Length(), mTrackBuffers.Length(),
            mAudioTrack.get(), mVideoTrack.get());
  
  if (waiting) {
    return NS_OK;
  }
  mEssentialTrackBuffers.Clear();
  if (!mAudioTrack && !mVideoTrack) {
    MSE_DEBUG("MediaSourceReader(%p)::ReadMetadata missing track: mAudioTrack=%p mVideoTrack=%p",
              this, mAudioTrack.get(), mVideoTrack.get());
    return NS_ERROR_FAILURE;
  }

  int64_t maxDuration = -1;

  if (mAudioTrack) {
    MOZ_ASSERT(mAudioTrack->IsReady());
    mAudioReader = mAudioTrack->Decoders()[0]->GetReader();

    const MediaInfo& info = mAudioReader->GetMediaInfo();
    MOZ_ASSERT(info.HasAudio());
    mInfo.mAudio = info.mAudio;
    maxDuration = std::max(maxDuration, mAudioReader->GetDecoder()->GetMediaDuration());
    MSE_DEBUG("MediaSourceReader(%p)::ReadMetadata audio reader=%p maxDuration=%lld",
              this, mAudioReader.get(), maxDuration);
  }

  if (mVideoTrack) {
    MOZ_ASSERT(mVideoTrack->IsReady());
    mVideoReader = mVideoTrack->Decoders()[0]->GetReader();

    const MediaInfo& info = mVideoReader->GetMediaInfo();
    MOZ_ASSERT(info.HasVideo());
    mInfo.mVideo = info.mVideo;
    maxDuration = std::max(maxDuration, mVideoReader->GetDecoder()->GetMediaDuration());
    MSE_DEBUG("MediaSourceReader(%p)::ReadMetadata video reader=%p maxDuration=%lld",
              this, mVideoReader.get(), maxDuration);
  }

  if (maxDuration != -1) {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mDecoder->SetMediaDuration(maxDuration);
    nsRefPtr<nsIRunnable> task (
      NS_NewRunnableMethodWithArg<double>(static_cast<MediaSourceDecoder*>(mDecoder),
                                          &MediaSourceDecoder::SetMediaSourceDuration,
                                          static_cast<double>(maxDuration) / USECS_PER_S));
    NS_DispatchToMainThread(task);
  }

  *aInfo = mInfo;
  *aTags = nullptr; 

  return NS_OK;
}

void
MediaSourceReader::Ended()
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  mEnded = true;
}

bool
MediaSourceReader::IsEnded()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  return mEnded;
}

} 
