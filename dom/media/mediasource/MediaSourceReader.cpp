




#include "MediaSourceReader.h"

#include <cmath>
#include "prlog.h"
#include "mozilla/dom/TimeRanges.h"
#include "DecoderTraits.h"
#include "MediaDecoderOwner.h"
#include "MediaSourceDecoder.h"
#include "MediaSourceUtils.h"
#include "SourceBufferDecoder.h"
#include "TrackBuffer.h"
#include "nsPrintfCString.h"

#ifdef MOZ_FMP4
#include "SharedDecoderManager.h"
#include "MP4Decoder.h"
#include "MP4Reader.h"
#endif

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetMediaSourceLog();

#define MSE_DEBUG(arg, ...) PR_LOG(GetMediaSourceLog(), PR_LOG_DEBUG, ("MediaSourceReader(%p)::%s: " arg, this, __func__, ##__VA_ARGS__))
#define MSE_DEBUGV(arg, ...) PR_LOG(GetMediaSourceLog(), PR_LOG_DEBUG + 1, ("MediaSourceReader(%p)::%s: " arg, this, __func__, ##__VA_ARGS__))
#else
#define MSE_DEBUG(...)
#define MSE_DEBUGV(...)
#endif






#define EOS_FUZZ_US 125000

using mozilla::dom::TimeRanges;

namespace mozilla {

MediaSourceReader::MediaSourceReader(MediaSourceDecoder* aDecoder)
  : MediaDecoderReader(aDecoder)
  , mLastAudioTime(0)
  , mLastVideoTime(0)
  , mPendingSeekTime(-1)
  , mWaitingForSeekData(false)
  , mSeekToEnd(false)
  , mTimeThreshold(0)
  , mDropAudioBeforeThreshold(false)
  , mDropVideoBeforeThreshold(false)
  , mAudioDiscontinuity(false)
  , mVideoDiscontinuity(false)
  , mEnded(false)
  , mMediaSourceDuration(0)
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
  MSE_DEBUG("trackBuffers=%u", mTrackBuffers.Length());
  mEssentialTrackBuffers.AppendElements(mTrackBuffers);
  mHasEssentialTrackBuffers = true;
  if (!IsWaitingMediaResources()) {
    mDecoder->NotifyWaitingForResourcesStatusChanged();
  }
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

bool
MediaSourceReader::IsWaitingOnCDMResource()
{
#ifdef MOZ_EME
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  MOZ_ASSERT(!IsWaitingMediaResources());

  if (!mInfo.IsEncrypted()) {
    return false;
  }

  
  
  
  if (!mCDMProxy) {
    return true;
  }

  {
    CDMCaps::AutoLock caps(mCDMProxy->Capabilites());
    return !caps.AreCapsKnown();
  }

#else
  return false;
#endif
}

size_t
MediaSourceReader::SizeOfVideoQueueInFrames()
{
  if (!GetVideoReader()) {
    MSE_DEBUG("called with no video reader");
    return 0;
  }
  return GetVideoReader()->SizeOfVideoQueueInFrames();
}

size_t
MediaSourceReader::SizeOfAudioQueueInFrames()
{
  if (!GetAudioReader()) {
    MSE_DEBUG("called with no audio reader");
    return 0;
  }
  return GetAudioReader()->SizeOfAudioQueueInFrames();
}

nsRefPtr<MediaDecoderReader::AudioDataPromise>
MediaSourceReader::RequestAudioData()
{
  MOZ_ASSERT(OnTaskQueue());
  MOZ_DIAGNOSTIC_ASSERT(mSeekPromise.IsEmpty(), "No sample requests allowed while seeking");
  MOZ_DIAGNOSTIC_ASSERT(mAudioPromise.IsEmpty(), "No duplicate sample requests");
  nsRefPtr<AudioDataPromise> p = mAudioPromise.Ensure(__func__);
  MSE_DEBUGV("mLastAudioTime=%lld", mLastAudioTime);
  if (!mAudioTrack) {
    MSE_DEBUG("called with no audio track");
    mAudioPromise.Reject(DECODE_ERROR, __func__);
    return p;
  }
  if (IsSeeking()) {
    MSE_DEBUG("called mid-seek. Rejecting.");
    mAudioPromise.Reject(CANCELED, __func__);
    return p;
  }
  MOZ_DIAGNOSTIC_ASSERT(!mAudioSeekRequest.Exists());

  SwitchSourceResult ret = SwitchAudioSource(&mLastAudioTime);
  switch (ret) {
    case SOURCE_NEW:
      GetAudioReader()->ResetDecode();
      mAudioSeekRequest.Begin(GetAudioReader()->Seek(GetReaderAudioTime(mLastAudioTime), 0)
                              ->RefableThen(GetTaskQueue(), __func__, this,
                                            &MediaSourceReader::CompleteAudioSeekAndDoRequest,
                                            &MediaSourceReader::CompleteAudioSeekAndRejectPromise));
      break;
    case SOURCE_NONE:
      if (!mLastAudioTime) {
        
        
        mAudioSourceDecoder = FirstDecoder(MediaData::AUDIO_DATA);
      }
      if (mLastAudioTime || !mAudioSourceDecoder) {
        CheckForWaitOrEndOfStream(MediaData::AUDIO_DATA, mLastAudioTime);
        break;
      }
      
    default:
      DoAudioRequest();
      break;
  }
  return p;
}

void MediaSourceReader::DoAudioRequest()
{
  mAudioRequest.Begin(GetAudioReader()->RequestAudioData()
                      ->RefableThen(GetTaskQueue(), __func__, this,
                                    &MediaSourceReader::OnAudioDecoded,
                                    &MediaSourceReader::OnAudioNotDecoded));
}

void
MediaSourceReader::OnAudioDecoded(AudioData* aSample)
{
  MOZ_DIAGNOSTIC_ASSERT(!IsSeeking());
  mAudioRequest.Complete();

  int64_t ourTime = aSample->mTime + mAudioSourceDecoder->GetTimestampOffset();
  if (aSample->mDiscontinuity) {
    mAudioDiscontinuity = true;
  }

  MSE_DEBUGV("[mTime=%lld mDuration=%lld mDiscontinuity=%d]",
             ourTime, aSample->mDuration, aSample->mDiscontinuity);
  if (mDropAudioBeforeThreshold) {
    if (ourTime < mTimeThreshold) {
      MSE_DEBUG("mTime=%lld < mTimeThreshold=%lld",
                ourTime, mTimeThreshold);
      mAudioRequest.Begin(GetAudioReader()->RequestAudioData()
                          ->RefableThen(GetTaskQueue(), __func__, this,
                                        &MediaSourceReader::OnAudioDecoded,
                                        &MediaSourceReader::OnAudioNotDecoded));
      return;
    }
    mDropAudioBeforeThreshold = false;
  }

  
  nsRefPtr<AudioData> newSample =
    AudioData::TransferAndUpdateTimestampAndDuration(aSample,
                                                     ourTime,
                                                     aSample->mDuration);
  mLastAudioTime = newSample->GetEndTime();
  if (mAudioDiscontinuity) {
    newSample->mDiscontinuity = true;
    mAudioDiscontinuity = false;
  }

  mAudioPromise.Resolve(newSample, __func__);
}













static void
AdjustEndTime(int64_t* aEndTime, SourceBufferDecoder* aDecoder)
{
  if (aDecoder) {
    nsRefPtr<dom::TimeRanges> ranges = new dom::TimeRanges();
    aDecoder->GetBuffered(ranges);
    if (ranges->Length() > 0) {
      int64_t end = std::ceil(ranges->GetEndTime() * USECS_PER_S);
      *aEndTime = std::max(*aEndTime, end);
    }
  }
}

void
MediaSourceReader::OnAudioNotDecoded(NotDecodedReason aReason)
{
  MOZ_DIAGNOSTIC_ASSERT(!IsSeeking());
  mAudioRequest.Complete();

  MSE_DEBUG("aReason=%u IsEnded: %d", aReason, IsEnded());
  if (aReason == CANCELED) {
    mAudioPromise.Reject(CANCELED, __func__);
    return;
  }

  
  
  int64_t lastAudioTime = mLastAudioTime;
  if (aReason == END_OF_STREAM && mAudioSourceDecoder) {
    AdjustEndTime(&mLastAudioTime, mAudioSourceDecoder);
  }

  SwitchSourceResult result = SwitchAudioSource(&mLastAudioTime);
  
  if (result == SOURCE_NEW) {
    GetAudioReader()->ResetDecode();
    mAudioSeekRequest.Begin(GetAudioReader()->Seek(GetReaderAudioTime(mLastAudioTime), 0)
                            ->RefableThen(GetTaskQueue(), __func__, this,
                                          &MediaSourceReader::CompleteAudioSeekAndDoRequest,
                                          &MediaSourceReader::CompleteAudioSeekAndRejectPromise));
    return;
  }

  
  
  
  
  if (aReason == DECODE_ERROR && result != SOURCE_NONE) {
    mAudioPromise.Reject(DECODE_ERROR, __func__);
    return;
  }

  CheckForWaitOrEndOfStream(MediaData::AUDIO_DATA, mLastAudioTime);

  if (mLastAudioTime - lastAudioTime >= EOS_FUZZ_US) {
    
    
    mLastAudioTime = lastAudioTime;
  }
}

nsRefPtr<MediaDecoderReader::VideoDataPromise>
MediaSourceReader::RequestVideoData(bool aSkipToNextKeyframe, int64_t aTimeThreshold)
{
  MOZ_ASSERT(OnTaskQueue());
  MOZ_DIAGNOSTIC_ASSERT(mSeekPromise.IsEmpty(), "No sample requests allowed while seeking");
  MOZ_DIAGNOSTIC_ASSERT(mVideoPromise.IsEmpty(), "No duplicate sample requests");
  nsRefPtr<VideoDataPromise> p = mVideoPromise.Ensure(__func__);
  MSE_DEBUGV("RequestVideoData(%d, %lld), mLastVideoTime=%lld",
             aSkipToNextKeyframe, aTimeThreshold, mLastVideoTime);
  if (!mVideoTrack) {
    MSE_DEBUG("called with no video track");
    mVideoPromise.Reject(DECODE_ERROR, __func__);
    return p;
  }
  if (aSkipToNextKeyframe) {
    mTimeThreshold = aTimeThreshold;
    mDropAudioBeforeThreshold = true;
    mDropVideoBeforeThreshold = true;
  }
  if (IsSeeking()) {
    MSE_DEBUG("called mid-seek. Rejecting.");
    mVideoPromise.Reject(CANCELED, __func__);
    return p;
  }
  MOZ_DIAGNOSTIC_ASSERT(!mVideoSeekRequest.Exists());

  SwitchSourceResult ret = SwitchVideoSource(&mLastVideoTime);
  switch (ret) {
    case SOURCE_NEW:
      GetVideoReader()->ResetDecode();
      mVideoSeekRequest.Begin(GetVideoReader()->Seek(GetReaderVideoTime(mLastVideoTime), 0)
                             ->RefableThen(GetTaskQueue(), __func__, this,
                                           &MediaSourceReader::CompleteVideoSeekAndDoRequest,
                                           &MediaSourceReader::CompleteVideoSeekAndRejectPromise));
      break;
    case SOURCE_NONE:
      if (!mLastVideoTime) {
        
        
        mVideoSourceDecoder = FirstDecoder(MediaData::VIDEO_DATA);
      }
      if (mLastVideoTime || !mVideoSourceDecoder) {
        CheckForWaitOrEndOfStream(MediaData::VIDEO_DATA, mLastVideoTime);
        break;
      }
      
    default:
      DoVideoRequest();
      break;
  }

  return p;
}

void
MediaSourceReader::DoVideoRequest()
{
  mVideoRequest.Begin(GetVideoReader()->RequestVideoData(mDropVideoBeforeThreshold, GetReaderVideoTime(mTimeThreshold))
                      ->RefableThen(GetTaskQueue(), __func__, this,
                                    &MediaSourceReader::OnVideoDecoded,
                                    &MediaSourceReader::OnVideoNotDecoded));
}

void
MediaSourceReader::OnVideoDecoded(VideoData* aSample)
{
  MOZ_DIAGNOSTIC_ASSERT(!IsSeeking());
  mVideoRequest.Complete();

  
  int64_t ourTime = aSample->mTime + mVideoSourceDecoder->GetTimestampOffset();
  if (aSample->mDiscontinuity) {
    mVideoDiscontinuity = true;
  }

  MSE_DEBUGV("[mTime=%lld mDuration=%lld mDiscontinuity=%d]",
             ourTime, aSample->mDuration, aSample->mDiscontinuity);
  if (mDropVideoBeforeThreshold) {
    if (ourTime < mTimeThreshold) {
      MSE_DEBUG("mTime=%lld < mTimeThreshold=%lld",
                ourTime, mTimeThreshold);
      DoVideoRequest();
      return;
    }
    mDropVideoBeforeThreshold = false;
    mTimeThreshold = 0;
  }

  
  nsRefPtr<VideoData> newSample =
    VideoData::ShallowCopyUpdateTimestampAndDuration(aSample,
                                                     ourTime,
                                                     aSample->mDuration);

  mLastVideoTime = newSample->GetEndTime();
  if (mVideoDiscontinuity) {
    newSample->mDiscontinuity = true;
    mVideoDiscontinuity = false;
  }

  mVideoPromise.Resolve(newSample, __func__);
}

void
MediaSourceReader::OnVideoNotDecoded(NotDecodedReason aReason)
{
  MOZ_DIAGNOSTIC_ASSERT(!IsSeeking());
  mVideoRequest.Complete();

  MSE_DEBUG("aReason=%u IsEnded: %d", aReason, IsEnded());

  if (aReason == CANCELED) {
    mVideoPromise.Reject(CANCELED, __func__);
    return;
  }

  
  
  int64_t lastVideoTime = mLastVideoTime;
  if (aReason == END_OF_STREAM && mVideoSourceDecoder) {
    AdjustEndTime(&mLastVideoTime, mVideoSourceDecoder);
  }

  
  SwitchSourceResult result = SwitchVideoSource(&mLastVideoTime);
  if (result == SOURCE_NEW) {
    GetVideoReader()->ResetDecode();
    mVideoSeekRequest.Begin(GetVideoReader()->Seek(GetReaderVideoTime(mLastVideoTime), 0)
                           ->RefableThen(GetTaskQueue(), __func__, this,
                                         &MediaSourceReader::CompleteVideoSeekAndDoRequest,
                                         &MediaSourceReader::CompleteVideoSeekAndRejectPromise));
    return;
  }

  
  
  
  
  if (aReason == DECODE_ERROR && result != SOURCE_NONE) {
    mVideoPromise.Reject(DECODE_ERROR, __func__);
    return;
  }

  CheckForWaitOrEndOfStream(MediaData::VIDEO_DATA, mLastVideoTime);

  if (mLastVideoTime - lastVideoTime >= EOS_FUZZ_US) {
    
    
    mLastVideoTime = lastVideoTime;
  }
}

void
MediaSourceReader::CheckForWaitOrEndOfStream(MediaData::Type aType, int64_t aTime)
{
  
  if (IsNearEnd(aType, aTime)) {
    if (aType == MediaData::AUDIO_DATA) {
      mAudioPromise.Reject(END_OF_STREAM, __func__);
    } else {
      mVideoPromise.Reject(END_OF_STREAM, __func__);
    }
    return;
  }

  if (aType == MediaData::AUDIO_DATA) {
    
    
    mAudioPromise.Reject(WAITING_FOR_DATA, __func__);
  } else {
    mVideoPromise.Reject(WAITING_FOR_DATA, __func__);
  }
}

nsRefPtr<ShutdownPromise>
MediaSourceReader::Shutdown()
{
  mSeekPromise.RejectIfExists(NS_ERROR_FAILURE, __func__);
  
  mAudioRequest.DisconnectIfExists();
  mVideoRequest.DisconnectIfExists();

  
  
  mAudioPromise.RejectIfExists(CANCELED, __func__);
  mVideoPromise.RejectIfExists(CANCELED, __func__);

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
  mAudioSourceDecoder = nullptr;
  mVideoTrack = nullptr;
  mVideoSourceDecoder = nullptr;

#ifdef MOZ_FMP4
  if (mSharedDecoderManager) {
    mSharedDecoderManager->Shutdown();
    mSharedDecoderManager = nullptr;
  }
#endif

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
  MOZ_ASSERT(!mAudioSourceDecoder);
  MOZ_ASSERT(!mVideoTrack);
  MOZ_ASSERT(!mVideoSourceDecoder);
  MOZ_ASSERT(!mTrackBuffers.Length());

  for (uint32_t i = 0; i < mShutdownTrackBuffers.Length(); ++i) {
    mShutdownTrackBuffers[i]->BreakCycles();
  }
  mShutdownTrackBuffers.Clear();
}

already_AddRefed<SourceBufferDecoder>
MediaSourceReader::SelectDecoder(int64_t aTarget,
                                 int64_t aTolerance,
                                 const nsTArray<nsRefPtr<SourceBufferDecoder>>& aTrackDecoders)
{
  return static_cast<MediaSourceDecoder*>(mDecoder)
      ->SelectDecoder(aTarget, aTolerance, aTrackDecoders);
}

bool
MediaSourceReader::HaveData(int64_t aTarget, MediaData::Type aType)
{
  TrackBuffer* trackBuffer = aType == MediaData::AUDIO_DATA ? mAudioTrack : mVideoTrack;
  MOZ_ASSERT(trackBuffer);
  nsRefPtr<SourceBufferDecoder> decoder = SelectDecoder(aTarget, EOS_FUZZ_US, trackBuffer->Decoders());
  return !!decoder;
}

MediaSourceReader::SwitchSourceResult
MediaSourceReader::SwitchAudioSource(int64_t* aTarget)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  
  if (!mAudioTrack) {
    return SOURCE_NONE;
  }

  
  
  
  bool usedFuzz = false;
  nsRefPtr<SourceBufferDecoder> newDecoder =
    SelectDecoder(*aTarget,  0, mAudioTrack->Decoders());
  if (!newDecoder) {
    newDecoder = SelectDecoder(*aTarget, EOS_FUZZ_US, mAudioTrack->Decoders());
    usedFuzz = true;
  }
  if (GetAudioReader() && mAudioSourceDecoder != newDecoder) {
    GetAudioReader()->SetIdle();
  }
  if (!newDecoder) {
    mAudioSourceDecoder = nullptr;
    return SOURCE_NONE;
  }
  if (newDecoder == mAudioSourceDecoder) {
    return SOURCE_EXISTING;
  }
  mAudioSourceDecoder = newDecoder;
  if (usedFuzz) {
    
    
    
    nsRefPtr<dom::TimeRanges> ranges = new dom::TimeRanges();
    newDecoder->GetBuffered(ranges);
    int64_t startTime = ranges->GetStartTime() * USECS_PER_S;
    if (*aTarget < startTime) {
      *aTarget = startTime;
    }
  }
  MSE_DEBUGV("switched decoder to %p (fuzz:%d)",
             mAudioSourceDecoder.get(), usedFuzz);
  return SOURCE_NEW;
}

MediaSourceReader::SwitchSourceResult
MediaSourceReader::SwitchVideoSource(int64_t* aTarget)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  
  if (!mVideoTrack) {
    return SOURCE_NONE;
  }

  
  
  
  bool usedFuzz = false;
  nsRefPtr<SourceBufferDecoder> newDecoder =
    SelectDecoder(*aTarget,  0, mVideoTrack->Decoders());
  if (!newDecoder) {
    newDecoder = SelectDecoder(*aTarget, EOS_FUZZ_US, mVideoTrack->Decoders());
    usedFuzz = true;
  }
  if (GetVideoReader() && mVideoSourceDecoder != newDecoder) {
    GetVideoReader()->SetIdle();
  }
  if (!newDecoder) {
    mVideoSourceDecoder = nullptr;
    return SOURCE_NONE;
  }
  if (newDecoder == mVideoSourceDecoder) {
    return SOURCE_EXISTING;
  }
  mVideoSourceDecoder = newDecoder;
  if (usedFuzz) {
    
    
    
    nsRefPtr<dom::TimeRanges> ranges = new dom::TimeRanges();
    newDecoder->GetBuffered(ranges);
    int64_t startTime = ranges->GetStartTime() * USECS_PER_S;
    if (*aTarget < startTime) {
      *aTarget = startTime;
    }
  }
  MSE_DEBUGV("switched decoder to %p (fuzz:%d)",
             mVideoSourceDecoder.get(), usedFuzz);
  return SOURCE_NEW;
}

bool
MediaSourceReader::IsDormantNeeded()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (GetVideoReader()) {
    return GetVideoReader()->IsDormantNeeded();
  }

  return false;
}

void
MediaSourceReader::ReleaseMediaResources()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (GetVideoReader()) {
    GetVideoReader()->ReleaseMediaResources();
  }
}

MediaDecoderReader*
CreateReaderForType(const nsACString& aType, AbstractMediaDecoder* aDecoder)
{
#ifdef MOZ_FMP4
  
  
  
  
  if ((aType.LowerCaseEqualsLiteral("video/mp4") ||
       aType.LowerCaseEqualsLiteral("audio/mp4")) &&
      MP4Decoder::IsEnabled() && aDecoder) {
    return new MP4Reader(aDecoder);
  }
#endif
  return DecoderTraits::CreateReader(aType, aDecoder);
}

already_AddRefed<SourceBufferDecoder>
MediaSourceReader::CreateSubDecoder(const nsACString& aType, int64_t aTimestampOffset)
{
  if (IsShutdown()) {
    return nullptr;
  }
  MOZ_ASSERT(GetTaskQueue());
  nsRefPtr<SourceBufferDecoder> decoder =
    new SourceBufferDecoder(new SourceBufferResource(aType), mDecoder, aTimestampOffset);
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

  MSE_DEBUG("subdecoder %p subreader %p",
            decoder.get(), reader.get());
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
  MSE_DEBUG("AddTrackBuffer(%p)", aTrackBuffer);
  mTrackBuffers.AppendElement(aTrackBuffer);
}

void
MediaSourceReader::RemoveTrackBuffer(TrackBuffer* aTrackBuffer)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  MSE_DEBUG("RemoveTrackBuffer(%p)", aTrackBuffer);
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
    MSE_DEBUG("%p audio", aTrackBuffer);
    mAudioTrack = aTrackBuffer;
  }
  if (aInfo.HasVideo() && !mVideoTrack) {
    MSE_DEBUG("%p video", aTrackBuffer);
    mVideoTrack = aTrackBuffer;
  }

  if (!IsWaitingMediaResources()) {
    mDecoder->NotifyWaitingForResourcesStatusChanged();
  }
}

bool
MediaSourceReader::TrackBuffersContainTime(int64_t aTime)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (mAudioTrack && !mAudioTrack->ContainsTime(aTime, EOS_FUZZ_US)) {
    return false;
  }
  if (mVideoTrack && !mVideoTrack->ContainsTime(aTime, EOS_FUZZ_US)) {
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
  } else {
    MaybeNotifyHaveData();
  }
}

nsRefPtr<MediaDecoderReader::SeekPromise>
MediaSourceReader::Seek(int64_t aTime, int64_t aIgnored )
{
  MSE_DEBUG("Seek(aTime=%lld, aEnd=%lld, aCurrent=%lld)",
            aTime);

  MOZ_DIAGNOSTIC_ASSERT(mSeekPromise.IsEmpty());
  MOZ_DIAGNOSTIC_ASSERT(mAudioPromise.IsEmpty());
  MOZ_DIAGNOSTIC_ASSERT(mVideoPromise.IsEmpty());
  nsRefPtr<SeekPromise> p = mSeekPromise.Ensure(__func__);

  if (IsShutdown()) {
    mSeekPromise.Reject(NS_ERROR_FAILURE, __func__);
    return p;
  }

  
  
  mPendingSeekTime = aTime;

  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mWaitingForSeekData = true;
    mDropAudioBeforeThreshold = false;
    mDropVideoBeforeThreshold = false;
    mTimeThreshold = 0;
  }

  AttemptSeek();
  return p;
}

nsresult
MediaSourceReader::ResetDecode()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  MSE_DEBUG("");

  
  mAudioRequest.DisconnectIfExists();
  mVideoRequest.DisconnectIfExists();
  mAudioSeekRequest.DisconnectIfExists();
  mVideoSeekRequest.DisconnectIfExists();

  
  
  mAudioPromise.RejectIfExists(CANCELED, __func__);
  mVideoPromise.RejectIfExists(CANCELED, __func__);
  mSeekPromise.RejectIfExists(NS_OK, __func__);

  
  mAudioWaitPromise.RejectIfExists(WaitForDataRejectValue(MediaData::AUDIO_DATA, WaitForDataRejectValue::CANCELED), __func__);
  mVideoWaitPromise.RejectIfExists(WaitForDataRejectValue(MediaData::VIDEO_DATA, WaitForDataRejectValue::CANCELED), __func__);

  
  mWaitingForSeekData = false;
  mPendingSeekTime = -1;

  
  if (GetAudioReader()) {
    GetAudioReader()->ResetDecode();
  }
  if (GetVideoReader()) {
    GetVideoReader()->ResetDecode();
  }

  return MediaDecoderReader::ResetDecode();
}

void
MediaSourceReader::OnVideoSeekCompleted(int64_t aTime)
{
  mVideoSeekRequest.Complete();

  
  int64_t ourTime = aTime + mVideoSourceDecoder->GetTimestampOffset();

  if (mAudioTrack) {
    mPendingSeekTime = ourTime;
    DoAudioSeek();
  } else {
    mPendingSeekTime = -1;
    mSeekPromise.Resolve(ourTime, __func__);
  }
}

void
MediaSourceReader::OnVideoSeekFailed(nsresult aResult)
{
  mVideoSeekRequest.Complete();
  mPendingSeekTime = -1;
  mSeekPromise.Reject(aResult, __func__);
}

void
MediaSourceReader::DoAudioSeek()
{
  int64_t seekTime = mPendingSeekTime;
  if (mSeekToEnd) {
    seekTime = LastSampleTime(MediaData::AUDIO_DATA);
  }
  if (SwitchAudioSource(&seekTime) == SOURCE_NONE) {
    
    
    mWaitingForSeekData = true;
    return;
  }
  GetAudioReader()->ResetDecode();
  mAudioSeekRequest.Begin(GetAudioReader()->Seek(GetReaderAudioTime(seekTime), 0)
                         ->RefableThen(GetTaskQueue(), __func__, this,
                                       &MediaSourceReader::OnAudioSeekCompleted,
                                       &MediaSourceReader::OnAudioSeekFailed));
  MSE_DEBUG("reader=%p", GetAudioReader());
}

void
MediaSourceReader::OnAudioSeekCompleted(int64_t aTime)
{
  mAudioSeekRequest.Complete();
  mPendingSeekTime = -1;
  
  mSeekPromise.Resolve(aTime + mAudioSourceDecoder->GetTimestampOffset(),
                       __func__);
}

void
MediaSourceReader::OnAudioSeekFailed(nsresult aResult)
{
  mAudioSeekRequest.Complete();
  mPendingSeekTime = -1;
  mSeekPromise.Reject(aResult, __func__);
}

void
MediaSourceReader::AttemptSeek()
{
  
  
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    if (!mWaitingForSeekData) {
      return;
    }

    mSeekToEnd = IsEnded() && mPendingSeekTime >= mMediaSourceDuration * USECS_PER_S;
    if (!mSeekToEnd && !TrackBuffersContainTime(mPendingSeekTime)) {
      mVideoSourceDecoder = nullptr;
      mAudioSourceDecoder = nullptr;
      return;
    }
    mWaitingForSeekData = false;
  }

  
  mLastAudioTime = mPendingSeekTime;
  mLastVideoTime = mPendingSeekTime;

  if (mVideoTrack) {
    DoVideoSeek();
  } else if (mAudioTrack) {
    DoAudioSeek();
  } else {
    MOZ_CRASH();
  }
}

void
MediaSourceReader::DoVideoSeek()
{
  int64_t seekTime = mPendingSeekTime;
  if (mSeekToEnd) {
    seekTime = LastSampleTime(MediaData::VIDEO_DATA);
  }
  if (SwitchVideoSource(&seekTime) == SOURCE_NONE) {
    
    
    mWaitingForSeekData = true;
    return;
  }
  GetVideoReader()->ResetDecode();
  mVideoSeekRequest.Begin(GetVideoReader()->Seek(GetReaderVideoTime(seekTime), 0)
                          ->RefableThen(GetTaskQueue(), __func__, this,
                                        &MediaSourceReader::OnVideoSeekCompleted,
                                        &MediaSourceReader::OnVideoSeekFailed));
  MSE_DEBUG("reader=%p", GetVideoReader());
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

  MSE_DEBUG("ranges=%s", DumpTimeRanges(intersectionRanges).get());
  return NS_OK;
}

already_AddRefed<SourceBufferDecoder>
MediaSourceReader::FirstDecoder(MediaData::Type aType)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  TrackBuffer* trackBuffer =
    aType == MediaData::AUDIO_DATA ? mAudioTrack : mVideoTrack;
  MOZ_ASSERT(trackBuffer);
  const nsTArray<nsRefPtr<SourceBufferDecoder>>& decoders = trackBuffer->Decoders();
  if (decoders.IsEmpty()) {
    return nullptr;
  }

  nsRefPtr<SourceBufferDecoder> firstDecoder;
  double lowestStartTime = PositiveInfinity<double>();

  for (uint32_t i = 0; i < decoders.Length(); ++i) {
    nsRefPtr<TimeRanges> r = new TimeRanges();
    decoders[i]->GetBuffered(r);
    double start = r->GetStartTime();
    if (start < 0) {
      continue;
    }
    if (start < lowestStartTime) {
      firstDecoder = decoders[i];
      lowestStartTime = start;
    }
  }
  return firstDecoder.forget();
}

nsRefPtr<MediaDecoderReader::WaitForDataPromise>
MediaSourceReader::WaitForData(MediaData::Type aType)
{
  MOZ_ASSERT(OnTaskQueue());
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
  bool ended = IsEnded();
  
  
  
  if (!IsSeeking() && mAudioTrack) {
    if (!mLastAudioTime) {
      nsRefPtr<SourceBufferDecoder> d = FirstDecoder(MediaData::AUDIO_DATA);
      haveAudio = !!d;
    } else {
      haveAudio = HaveData(mLastAudioTime, MediaData::AUDIO_DATA);
    }
    if (ended || haveAudio) {
      WaitPromise(MediaData::AUDIO_DATA).ResolveIfExists(MediaData::AUDIO_DATA, __func__);
    }
  }
  if (!IsSeeking() && mVideoTrack) {
    if (!mLastVideoTime) {
      nsRefPtr<SourceBufferDecoder> d = FirstDecoder(MediaData::VIDEO_DATA);
      haveVideo = !!d;
    } else {
      haveVideo = HaveData(mLastVideoTime, MediaData::VIDEO_DATA);
    }
    if (ended || haveVideo) {
      WaitPromise(MediaData::VIDEO_DATA).ResolveIfExists(MediaData::VIDEO_DATA, __func__);
    }
  }
  MSE_DEBUG("isSeeking=%d haveAudio=%d, haveVideo=%d ended=%d",
            IsSeeking(), haveAudio, haveVideo, ended);
}

nsresult
MediaSourceReader::ReadMetadata(MediaInfo* aInfo, MetadataTags** aTags)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  MSE_DEBUG("tracks=%u/%u audio=%p video=%p",
            mEssentialTrackBuffers.Length(), mTrackBuffers.Length(),
            mAudioTrack.get(), mVideoTrack.get());

  mEssentialTrackBuffers.Clear();
  if (!mAudioTrack && !mVideoTrack) {
    MSE_DEBUG("missing track: mAudioTrack=%p mVideoTrack=%p",
              mAudioTrack.get(), mVideoTrack.get());
    return NS_ERROR_FAILURE;
  }

  if (mAudioTrack) {
    MOZ_ASSERT(mAudioTrack->IsReady());
    mAudioSourceDecoder = mAudioTrack->Decoders()[0];

    const MediaInfo& info = GetAudioReader()->GetMediaInfo();
    MOZ_ASSERT(info.HasAudio());
    mInfo.mAudio = info.mAudio;
    mInfo.mCrypto.AddInitData(info.mCrypto);
    MSE_DEBUG("audio reader=%p duration=%lld",
              mAudioSourceDecoder.get(),
              mAudioSourceDecoder->GetReader()->GetDecoder()->GetMediaDuration());
  }

  if (mVideoTrack) {
    MOZ_ASSERT(mVideoTrack->IsReady());
    mVideoSourceDecoder = mVideoTrack->Decoders()[0];

    const MediaInfo& info = GetVideoReader()->GetMediaInfo();
    MOZ_ASSERT(info.HasVideo());
    mInfo.mVideo = info.mVideo;
    mInfo.mCrypto.AddInitData(info.mCrypto);
    MSE_DEBUG("video reader=%p duration=%lld",
              GetVideoReader(),
              GetVideoReader()->GetDecoder()->GetMediaDuration());
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
    mAudioSourceDecoder = mAudioTrack->Decoders()[0];

    const MediaInfo& info = GetAudioReader()->GetMediaInfo();
    MOZ_ASSERT(info.HasAudio());
    mInfo.mAudio = info.mAudio;
  }

  if (mVideoTrack) {
    MOZ_ASSERT(mVideoTrack->IsReady());
    mVideoSourceDecoder = mVideoTrack->Decoders()[0];

    const MediaInfo& info = GetVideoReader()->GetMediaInfo();
    MOZ_ASSERT(info.HasVideo());
    mInfo.mVideo = info.mVideo;
  }
  *aInfo = mInfo;
}

void
MediaSourceReader::Ended(bool aEnded)
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  mEnded = aEnded;
  if (aEnded) {
    
    
    RefPtr<nsIRunnable> task(NS_NewRunnableMethod(
        this, &MediaSourceReader::NotifyTimeRangesChanged));
    GetTaskQueue()->Dispatch(task.forget());
  }
}

bool
MediaSourceReader::IsEnded()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  return mEnded;
}

bool
MediaSourceReader::IsNearEnd(MediaData::Type aType, int64_t aTime)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (!mEnded) {
    return false;
  }
  TrackBuffer* trackBuffer =
    aType == MediaData::AUDIO_DATA ? mAudioTrack : mVideoTrack;
  nsRefPtr<dom::TimeRanges> buffered = new dom::TimeRanges();
  trackBuffer->Buffered(buffered);
  return aTime >= (buffered->GetEndTime() * USECS_PER_S - EOS_FUZZ_US);
}

int64_t
MediaSourceReader::LastSampleTime(MediaData::Type aType)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  TrackBuffer* trackBuffer =
  aType == MediaData::AUDIO_DATA ? mAudioTrack : mVideoTrack;
  nsRefPtr<dom::TimeRanges> buffered = new dom::TimeRanges();
  trackBuffer->Buffered(buffered);
  return buffered->GetEndTime() * USECS_PER_S - 1;
}

void
MediaSourceReader::SetMediaSourceDuration(double aDuration)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  mMediaSourceDuration = aDuration;
}

void
MediaSourceReader::GetMozDebugReaderData(nsAString& aString)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  nsAutoCString result;
  result += nsPrintfCString("Dumping data for reader %p:\n", this);
  if (mAudioTrack) {
    result += nsPrintfCString("\tDumping Audio Track Decoders: - mLastAudioTime: %f\n", double(mLastAudioTime) / USECS_PER_S);
    for (int32_t i = mAudioTrack->Decoders().Length() - 1; i >= 0; --i) {
      nsRefPtr<MediaDecoderReader> newReader = mAudioTrack->Decoders()[i]->GetReader();

      nsRefPtr<dom::TimeRanges> ranges = new dom::TimeRanges();
      mAudioTrack->Decoders()[i]->GetBuffered(ranges);
      result += nsPrintfCString("\t\tReader %d: %p ranges=%s active=%s size=%lld\n",
                                i, newReader.get(), DumpTimeRanges(ranges).get(),
                                newReader.get() == GetAudioReader() ? "true" : "false",
                                mAudioTrack->Decoders()[i]->GetResource()->GetSize());
    }
  }

  if (mVideoTrack) {
    result += nsPrintfCString("\tDumping Video Track Decoders - mLastVideoTime: %f\n", double(mLastVideoTime) / USECS_PER_S);
    for (int32_t i = mVideoTrack->Decoders().Length() - 1; i >= 0; --i) {
      nsRefPtr<MediaDecoderReader> newReader = mVideoTrack->Decoders()[i]->GetReader();

      nsRefPtr<dom::TimeRanges> ranges = new dom::TimeRanges();
      mVideoTrack->Decoders()[i]->GetBuffered(ranges);
      result += nsPrintfCString("\t\tReader %d: %p ranges=%s active=%s size=%lld\n",
                                i, newReader.get(), DumpTimeRanges(ranges).get(),
                                newReader.get() == GetVideoReader() ? "true" : "false",
                                mVideoTrack->Decoders()[i]->GetResource()->GetSize());
    }
  }
  aString += NS_ConvertUTF8toUTF16(result);
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

bool
MediaSourceReader::IsActiveReader(MediaDecoderReader* aReader)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  return aReader && (aReader == GetVideoReader() || aReader == GetAudioReader());
}

MediaDecoderReader*
MediaSourceReader::GetAudioReader() const
{
  return mAudioSourceDecoder ? mAudioSourceDecoder->GetReader() : nullptr;
}

MediaDecoderReader*
MediaSourceReader::GetVideoReader() const
{
  return mVideoSourceDecoder ? mVideoSourceDecoder->GetReader() : nullptr;
}

int64_t
MediaSourceReader::GetReaderAudioTime(int64_t aTime) const
{
  return aTime - mAudioSourceDecoder->GetTimestampOffset();
}

int64_t
MediaSourceReader::GetReaderVideoTime(int64_t aTime) const
{
  return aTime - mVideoSourceDecoder->GetTimestampOffset();
}

#undef MSE_DEBUG
#undef MSE_DEBUGV
} 
