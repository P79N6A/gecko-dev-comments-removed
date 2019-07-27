




#include "MediaSourceReader.h"

#include "prlog.h"
#include "mozilla/dom/TimeRanges.h"
#include "DecoderTraits.h"
#include "MediaDecoderOwner.h"
#include "MediaSourceDecoder.h"
#include "MediaSourceUtils.h"
#include "SourceBufferDecoder.h"
#include "TrackBuffer.h"

#ifdef MOZ_FMP4
#include "SharedDecoderManager.h"
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






#define EOS_FUZZ_US 125000

using mozilla::dom::TimeRanges;

namespace mozilla {

MediaSourceReader::MediaSourceReader(MediaSourceDecoder* aDecoder)
  : MediaDecoderReader(aDecoder)
  , mLastAudioTime(-1)
  , mLastVideoTime(-1)
  , mPendingSeekTime(-1)
  , mPendingStartTime(-1)
  , mPendingEndTime(-1)
  , mPendingCurrentTime(-1)
  , mWaitingForSeekData(false)
  , mPendingSeeks(0)
  , mSeekResult(NS_OK)
  , mTimeThreshold(-1)
  , mDropAudioBeforeThreshold(false)
  , mDropVideoBeforeThreshold(false)
  , mEnded(false)
  , mAudioIsSeeking(false)
  , mVideoIsSeeking(false)
  , mHasEssentialTrackBuffers(false)
#ifdef MOZ_FMP4
  , mSharedDecoderManager(new SharedDecoderManager())
#endif
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

size_t
MediaSourceReader::SizeOfVideoQueueInFrames()
{
  if (!mVideoReader) {
    MSE_DEBUG("MediaSourceReader(%p)::SizeOfVideoQueue called with no video reader", this);
    return 0;
  }
  return mVideoReader->SizeOfVideoQueueInFrames();
}

size_t
MediaSourceReader::SizeOfAudioQueueInFrames()
{
  if (!mAudioReader) {
    MSE_DEBUG("MediaSourceReader(%p)::SizeOfAudioQueue called with no audio reader", this);
    return 0;
  }
  return mAudioReader->SizeOfAudioQueueInFrames();
}

nsRefPtr<MediaDecoderReader::AudioDataPromise>
MediaSourceReader::RequestAudioData()
{
  nsRefPtr<AudioDataPromise> p = mAudioPromise.Ensure(__func__);
  MSE_DEBUGV("MediaSourceReader(%p)::RequestAudioData", this);
  if (!mAudioReader) {
    MSE_DEBUG("MediaSourceReader(%p)::RequestAudioData called with no audio reader", this);
    mAudioPromise.Reject(DECODE_ERROR, __func__);
    return p;
  }
  mAudioIsSeeking = false;
  SwitchAudioReader(mLastAudioTime);
  mAudioReader->RequestAudioData()->Then(GetTaskQueue(), __func__, this,
                                         &MediaSourceReader::OnAudioDecoded,
                                         &MediaSourceReader::OnAudioNotDecoded);
  return p;
}

void
MediaSourceReader::OnAudioDecoded(AudioData* aSample)
{
  MSE_DEBUGV("MediaSourceReader(%p)::OnAudioDecoded [mTime=%lld mDuration=%lld mDiscontinuity=%d]",
             this, aSample->mTime, aSample->mDuration, aSample->mDiscontinuity);
  if (mDropAudioBeforeThreshold) {
    if (aSample->mTime < mTimeThreshold) {
      MSE_DEBUG("MediaSourceReader(%p)::OnAudioDecoded mTime=%lld < mTimeThreshold=%lld",
                this, aSample->mTime, mTimeThreshold);
      mAudioReader->RequestAudioData()->Then(GetTaskQueue(), __func__, this,
                                             &MediaSourceReader::OnAudioDecoded,
                                             &MediaSourceReader::OnAudioNotDecoded);
      return;
    }
    mDropAudioBeforeThreshold = false;
  }

  
  
  
  if (!mAudioIsSeeking) {
    mLastAudioTime = aSample->mTime + aSample->mDuration;
  }

  mAudioPromise.Resolve(aSample, __func__);
}













static void
AdjustEndTime(int64_t* aEndTime, MediaDecoderReader* aReader)
{
  if (aReader) {
    nsRefPtr<dom::TimeRanges> ranges = new dom::TimeRanges();
    aReader->GetBuffered(ranges);
    if (ranges->Length() > 0) {
      
      int64_t end = ranges->GetEndTime() * USECS_PER_S + 0.5;
      *aEndTime = std::max(*aEndTime, end);
    }
  }
}

void
MediaSourceReader::OnAudioNotDecoded(NotDecodedReason aReason)
{
  MSE_DEBUG("MediaSourceReader(%p)::OnAudioNotDecoded aReason=%u IsEnded: %d", this, aReason, IsEnded());
  if (aReason == DECODE_ERROR || aReason == CANCELED) {
    mAudioPromise.Reject(aReason, __func__);
    return;
  }

  
  
  MOZ_ASSERT(aReason == END_OF_STREAM);
  if (mAudioReader) {
    AdjustEndTime(&mLastAudioTime, mAudioReader);
  }

  
  
  
  if (SwitchAudioReader(mLastAudioTime + EOS_FUZZ_US)) {
    mAudioReader->RequestAudioData()->Then(GetTaskQueue(), __func__, this,
                                           &MediaSourceReader::OnAudioDecoded,
                                           &MediaSourceReader::OnAudioNotDecoded);
    return;
  }

  
  if (IsEnded()) {
    mAudioPromise.Reject(END_OF_STREAM, __func__);
    return;
  }

  
  
  mAudioPromise.Reject(WAITING_FOR_DATA, __func__);
}


nsRefPtr<MediaDecoderReader::VideoDataPromise>
MediaSourceReader::RequestVideoData(bool aSkipToNextKeyframe, int64_t aTimeThreshold)
{
  nsRefPtr<VideoDataPromise> p = mVideoPromise.Ensure(__func__);
  MSE_DEBUGV("MediaSourceReader(%p)::RequestVideoData(%d, %lld)",
             this, aSkipToNextKeyframe, aTimeThreshold);
  if (!mVideoReader) {
    MSE_DEBUG("MediaSourceReader(%p)::RequestVideoData called with no video reader", this);
    mVideoPromise.Reject(DECODE_ERROR, __func__);
    return p;
  }
  if (aSkipToNextKeyframe) {
    mTimeThreshold = aTimeThreshold;
    mDropAudioBeforeThreshold = true;
    mDropVideoBeforeThreshold = true;
  }
  mVideoIsSeeking = false;
  SwitchVideoReader(mLastVideoTime);

  mVideoReader->RequestVideoData(aSkipToNextKeyframe, aTimeThreshold)
              ->Then(GetTaskQueue(), __func__, this,
                     &MediaSourceReader::OnVideoDecoded, &MediaSourceReader::OnVideoNotDecoded);
  return p;
}

void
MediaSourceReader::OnVideoDecoded(VideoData* aSample)
{
  MSE_DEBUGV("MediaSourceReader(%p)::OnVideoDecoded [mTime=%lld mDuration=%lld mDiscontinuity=%d]",
             this, aSample->mTime, aSample->mDuration, aSample->mDiscontinuity);
  if (mDropVideoBeforeThreshold) {
    if (aSample->mTime < mTimeThreshold) {
      MSE_DEBUG("MediaSourceReader(%p)::OnVideoDecoded mTime=%lld < mTimeThreshold=%lld",
                this, aSample->mTime, mTimeThreshold);
      mVideoReader->RequestVideoData(false, 0)->Then(GetTaskQueue(), __func__, this,
                                                     &MediaSourceReader::OnVideoDecoded,
                                                     &MediaSourceReader::OnVideoNotDecoded);
      return;
    }
    mDropVideoBeforeThreshold = false;
  }

  
  
  
  if (!mVideoIsSeeking) {
    mLastVideoTime = aSample->mTime + aSample->mDuration;
  }

  mVideoPromise.Resolve(aSample, __func__);
}

void
MediaSourceReader::OnVideoNotDecoded(NotDecodedReason aReason)
{
  MSE_DEBUG("MediaSourceReader(%p)::OnVideoNotDecoded aReason=%u IsEnded: %d", this, aReason, IsEnded());
  if (aReason == DECODE_ERROR || aReason == CANCELED) {
    mVideoPromise.Reject(aReason, __func__);
    return;
  }

  
  
  MOZ_ASSERT(aReason == END_OF_STREAM);
  if (mVideoReader) {
    AdjustEndTime(&mLastVideoTime, mVideoReader);
  }

  
  
  
  if (SwitchVideoReader(mLastVideoTime + EOS_FUZZ_US)) {
    mVideoReader->RequestVideoData(false, 0)
                ->Then(GetTaskQueue(), __func__, this,
                       &MediaSourceReader::OnVideoDecoded,
                       &MediaSourceReader::OnVideoNotDecoded);
    return;
  }

  
  if (IsEnded()) {
    mVideoPromise.Reject(END_OF_STREAM, __func__);
    return;
  }

  
  
  mVideoPromise.Reject(WAITING_FOR_DATA, __func__);
}

nsRefPtr<ShutdownPromise>
MediaSourceReader::Shutdown()
{
  mSeekPromise.RejectIfExists(NS_ERROR_FAILURE, __func__);

  MOZ_ASSERT(mMediaSourceShutdownPromise.IsEmpty());
  nsRefPtr<ShutdownPromise> p = mMediaSourceShutdownPromise.Ensure(__func__);

  ContinueShutdown();
  return p;
}

void
MediaSourceReader::ContinueShutdown()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (mTrackBuffers.Length()) {
    mTrackBuffers[0]->Shutdown()->Then(GetTaskQueue(), __func__, this,
                                       &MediaSourceReader::ContinueShutdown,
                                       &MediaSourceReader::ContinueShutdown);
    mShutdownTrackBuffers.AppendElement(mTrackBuffers[0]);
    mTrackBuffers.RemoveElementAt(0);
    return;
  }

  mAudioTrack = nullptr;
  mAudioReader = nullptr;
  mVideoTrack = nullptr;
  mVideoReader = nullptr;

  MOZ_ASSERT(mAudioPromise.IsEmpty());
  MOZ_ASSERT(mVideoPromise.IsEmpty());

  mAudioWaitPromise.RejectIfExists(WaitForDataRejectValue(MediaData::AUDIO_DATA, WaitForDataRejectValue::SHUTDOWN), __func__);
  mVideoWaitPromise.RejectIfExists(WaitForDataRejectValue(MediaData::VIDEO_DATA, WaitForDataRejectValue::SHUTDOWN), __func__);

  MediaDecoderReader::Shutdown()->ChainTo(mMediaSourceShutdownPromise.Steal(), __func__);
}

void
MediaSourceReader::BreakCycles()
{
  MediaDecoderReader::BreakCycles();

  
  MOZ_ASSERT(!mAudioTrack);
  MOZ_ASSERT(!mAudioReader);
  MOZ_ASSERT(!mVideoTrack);
  MOZ_ASSERT(!mVideoReader);
  MOZ_ASSERT(!mTrackBuffers.Length());

  for (uint32_t i = 0; i < mShutdownTrackBuffers.Length(); ++i) {
    mShutdownTrackBuffers[i]->BreakCycles();
  }
  mShutdownTrackBuffers.Clear();
}

already_AddRefed<MediaDecoderReader>
MediaSourceReader::SelectReader(int64_t aTarget,
                                const nsTArray<nsRefPtr<SourceBufferDecoder>>& aTrackDecoders)
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  
  
  for (int32_t i = aTrackDecoders.Length() - 1; i >= 0; --i) {
    nsRefPtr<MediaDecoderReader> newReader = aTrackDecoders[i]->GetReader();

    nsRefPtr<dom::TimeRanges> ranges = new dom::TimeRanges();
    aTrackDecoders[i]->GetBuffered(ranges);
    if (ranges->Find(double(aTarget) / USECS_PER_S) == dom::TimeRanges::NoIndex) {
      MSE_DEBUGV("MediaSourceReader(%p)::SelectReader(%lld) newReader=%p target not in ranges=%s",
                 this, aTarget, newReader.get(), DumpTimeRanges(ranges).get());
      continue;
    }

    return newReader.forget();
  }

  return nullptr;
}

bool
MediaSourceReader::HaveData(int64_t aTarget, MediaData::Type aType)
{
  TrackBuffer* trackBuffer = aType == MediaData::AUDIO_DATA ? mAudioTrack : mVideoTrack;
  MOZ_ASSERT(trackBuffer);
  nsRefPtr<MediaDecoderReader> reader = SelectReader(aTarget, trackBuffer->Decoders());
  return !!reader;
}

bool
MediaSourceReader::SwitchAudioReader(int64_t aTarget)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  
  if (!mAudioTrack) {
    return false;
  }
  nsRefPtr<MediaDecoderReader> newReader = SelectReader(aTarget, mAudioTrack->Decoders());
  if (newReader && newReader != mAudioReader) {
    mAudioReader->SetIdle();
    mAudioReader = newReader;
    MSE_DEBUGV("MediaSourceReader(%p)::SwitchAudioReader switched reader to %p", this, mAudioReader.get());
    return true;
  }
  return false;
}

bool
MediaSourceReader::SwitchVideoReader(int64_t aTarget)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  
  if (!mVideoTrack) {
    return false;
  }
  nsRefPtr<MediaDecoderReader> newReader = SelectReader(aTarget, mVideoTrack->Decoders());
  if (newReader && newReader != mVideoReader) {
    mVideoReader->SetIdle();
    mVideoReader = newReader;
    MSE_DEBUGV("MediaSourceReader(%p)::SwitchVideoReader switched reader to %p", this, mVideoReader.get());
    return true;
  }
  return false;
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

  
  
  
  {
    ReentrantMonitorAutoEnter mon(decoder->GetReentrantMonitor());
    reader->SetStartTime(0);
  }

  
  
  
  
  
  
  
  reader->SetBorrowedTaskQueue(GetTaskQueue());

#ifdef MOZ_FMP4
  reader->SetSharedDecoderManager(mSharedDecoderManager);
#endif
  reader->Init(nullptr);

  MSE_DEBUG("MediaSourceReader(%p)::CreateSubDecoder subdecoder %p subreader %p",
            this, decoder.get(), reader.get());
  decoder->SetReader(reader);
#ifdef MOZ_EME
  decoder->SetCDMProxy(mCDMProxy);
#endif
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

bool
MediaSourceReader::TrackBuffersContainTime(int64_t aTime)
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

void
MediaSourceReader::NotifyTimeRangesChanged()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (mWaitingForSeekData) {
    
    RefPtr<nsIRunnable> task(NS_NewRunnableMethod(
        this, &MediaSourceReader::AttemptSeek));
    GetTaskQueue()->Dispatch(task.forget());
  }
}

nsRefPtr<MediaDecoderReader::SeekPromise>
MediaSourceReader::Seek(int64_t aTime, int64_t aStartTime, int64_t aEndTime,
                        int64_t aCurrentTime)
{
  MSE_DEBUG("MediaSourceReader(%p)::Seek(aTime=%lld, aStart=%lld, aEnd=%lld, aCurrent=%lld)",
            this, aTime, aStartTime, aEndTime, aCurrentTime);

  mSeekPromise.RejectIfExists(NS_OK, __func__);
  nsRefPtr<SeekPromise> p = mSeekPromise.Ensure(__func__);

  if (IsShutdown()) {
    mSeekPromise.Reject(NS_ERROR_FAILURE, __func__);
    return p;
  }

  
  
  mPendingSeekTime = aTime;
  mPendingStartTime = aStartTime;
  mPendingEndTime = aEndTime;
  mPendingCurrentTime = aCurrentTime;

  
  
  
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

    if (!mWaitingForSeekData) {
      mWaitingForSeekData = true;
      if (mAudioTrack) {
        mPendingSeeks++;
      }
      if (mVideoTrack) {
        mPendingSeeks++;
      }
    }
  }

  AttemptSeek();
  return p;
}

void
MediaSourceReader::OnSeekCompleted()
{
  mPendingSeeks--;
  FinalizeSeek();
}

void
MediaSourceReader::OnSeekFailed(nsresult aResult)
{
  mPendingSeeks--;
  
  if (NS_FAILED(aResult)) {
    mSeekResult = aResult;
  }
  FinalizeSeek();
}

void
MediaSourceReader::FinalizeSeek()
{
  
  
  if (!mPendingSeeks) {
    if (NS_FAILED(mSeekResult)) {
      mSeekPromise.Reject(mSeekResult, __func__);
    } else {
      mSeekPromise.Resolve(true, __func__);
    }
    mSeekResult = NS_OK;
  }
}

void
MediaSourceReader::AttemptSeek()
{
  
  
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    if (!mWaitingForSeekData || !TrackBuffersContainTime(mPendingSeekTime)) {
      return;
    }
  }

  ResetDecode();
  for (uint32_t i = 0; i < mTrackBuffers.Length(); ++i) {
    mTrackBuffers[i]->ResetDecode();
  }

  
  mLastAudioTime = mPendingSeekTime;
  mLastVideoTime = mPendingSeekTime;

  if (mAudioTrack) {
    mAudioIsSeeking = true;
    SwitchAudioReader(mPendingSeekTime);
    mAudioReader->Seek(mPendingSeekTime,
                       mPendingStartTime,
                       mPendingEndTime,
                       mPendingCurrentTime)
                ->Then(GetTaskQueue(), __func__, this,
                       &MediaSourceReader::OnSeekCompleted,
                       &MediaSourceReader::OnSeekFailed);
    MSE_DEBUG("MediaSourceReader(%p)::Seek audio reader=%p", this, mAudioReader.get());
  }
  if (mVideoTrack) {
    mVideoIsSeeking = true;
    SwitchVideoReader(mPendingSeekTime);
    mVideoReader->Seek(mPendingSeekTime,
                       mPendingStartTime,
                       mPendingEndTime,
                       mPendingCurrentTime)
                ->Then(GetTaskQueue(), __func__, this,
                       &MediaSourceReader::OnSeekCompleted,
                       &MediaSourceReader::OnSeekFailed);
    MSE_DEBUG("MediaSourceReader(%p)::Seek video reader=%p", this, mVideoReader.get());
  }
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mWaitingForSeekData = false;
  }
}

nsresult
MediaSourceReader::GetBuffered(dom::TimeRanges* aBuffered)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  MOZ_ASSERT(aBuffered->Length() == 0);
  if (mTrackBuffers.IsEmpty()) {
    return NS_OK;
  }

  double highestEndTime = 0;

  nsTArray<nsRefPtr<TimeRanges>> activeRanges;
  for (uint32_t i = 0; i < mTrackBuffers.Length(); ++i) {
    nsRefPtr<TimeRanges> r = new TimeRanges();
    mTrackBuffers[i]->Buffered(r);
    activeRanges.AppendElement(r);
    highestEndTime = std::max(highestEndTime, activeRanges.LastElement()->GetEndTime());
  }

  TimeRanges* intersectionRanges = aBuffered;
  intersectionRanges->Add(0, highestEndTime);

  for (uint32_t i = 0; i < activeRanges.Length(); ++i) {
    TimeRanges* sourceRanges = activeRanges[i];

    if (IsEnded()) {
      
      
      
      sourceRanges->Add(sourceRanges->GetEndTime(), highestEndTime);
      sourceRanges->Normalize();
    }

    intersectionRanges->Intersection(sourceRanges);
  }

  MSE_DEBUG("MediaSourceReader(%p)::GetBuffered ranges=%s", this, DumpTimeRanges(intersectionRanges).get());
  return NS_OK;
}

nsRefPtr<MediaDecoderReader::WaitForDataPromise>
MediaSourceReader::WaitForData(MediaData::Type aType)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  nsRefPtr<WaitForDataPromise> p = WaitPromise(aType).Ensure(__func__);
  MaybeNotifyHaveData();
  return p;
}

void
MediaSourceReader::MaybeNotifyHaveData()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  bool haveAudio = false, haveVideo = false;
  if (!mAudioIsSeeking && mAudioTrack && HaveData(mLastAudioTime, MediaData::AUDIO_DATA)) {
    haveAudio = true;
    WaitPromise(MediaData::AUDIO_DATA).ResolveIfExists(MediaData::AUDIO_DATA, __func__);
  }
  if (!mVideoIsSeeking && mVideoTrack && HaveData(mLastVideoTime, MediaData::VIDEO_DATA)) {
    haveVideo = true;
    WaitPromise(MediaData::VIDEO_DATA).ResolveIfExists(MediaData::VIDEO_DATA, __func__);
  }
  MSE_DEBUG("MediaSourceReader(%p)::MaybeNotifyHaveData haveAudio=%d, haveVideo=%d", this,
            haveAudio, haveVideo);
}

nsresult
MediaSourceReader::ReadMetadata(MediaInfo* aInfo, MetadataTags** aTags)
{
  MSE_DEBUG("MediaSourceReader(%p)::ReadMetadata tracks=%u/%u audio=%p video=%p",
            this, mEssentialTrackBuffers.Length(), mTrackBuffers.Length(),
            mAudioTrack.get(), mVideoTrack.get());

  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mEssentialTrackBuffers.Clear();
  }
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
    static_cast<MediaSourceDecoder*>(mDecoder)->SetDecodedDuration(maxDuration);
  }

  *aInfo = mInfo;
  *aTags = nullptr; 

  return NS_OK;
}

void
MediaSourceReader::ReadUpdatedMetadata(MediaInfo* aInfo)
{
  if (mAudioTrack) {
    MOZ_ASSERT(mAudioTrack->IsReady());
    mAudioReader = mAudioTrack->Decoders()[0]->GetReader();

    const MediaInfo& info = mAudioReader->GetMediaInfo();
    MOZ_ASSERT(info.HasAudio());
    mInfo.mAudio = info.mAudio;
  }

  if (mVideoTrack) {
    MOZ_ASSERT(mVideoTrack->IsReady());
    mVideoReader = mVideoTrack->Decoders()[0]->GetReader();

    const MediaInfo& info = mVideoReader->GetMediaInfo();
    MOZ_ASSERT(info.HasVideo());
    mInfo.mVideo = info.mVideo;
  }
  *aInfo = mInfo;
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

#ifdef MOZ_EME
nsresult
MediaSourceReader::SetCDMProxy(CDMProxy* aProxy)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  mCDMProxy = aProxy;
  for (size_t i = 0; i < mTrackBuffers.Length(); i++) {
    nsresult rv = mTrackBuffers[i]->SetCDMProxy(aProxy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}
#endif

} 
