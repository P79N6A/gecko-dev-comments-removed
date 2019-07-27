





#include "TrackBuffersManager.h"
#include "SourceBufferResource.h"
#include "SourceBuffer.h"

#ifdef MOZ_FMP4
#include "MP4Demuxer.h"
#endif

#include <limits>

extern PRLogModuleInfo* GetMediaSourceLog();

#define MSE_DEBUG(arg, ...) MOZ_LOG(GetMediaSourceLog(), mozilla::LogLevel::Debug, ("TrackBuffersManager(%p:%s)::%s: " arg, this, mType.get(), __func__, ##__VA_ARGS__))
#define MSE_DEBUGV(arg, ...) MOZ_LOG(GetMediaSourceLog(), mozilla::LogLevel::Verbose, ("TrackBuffersManager(%p:%s)::%s: " arg, this, mType.get(), __func__, ##__VA_ARGS__))

namespace mozilla {

static const char*
AppendStateToStr(TrackBuffersManager::AppendState aState)
{
  switch (aState) {
    case TrackBuffersManager::AppendState::WAITING_FOR_SEGMENT:
      return "WAITING_FOR_SEGMENT";
    case TrackBuffersManager::AppendState::PARSING_INIT_SEGMENT:
      return "PARSING_INIT_SEGMENT";
    case TrackBuffersManager::AppendState::PARSING_MEDIA_SEGMENT:
      return "PARSING_MEDIA_SEGMENT";
    default:
      return "IMPOSSIBLE";
  }
}

TrackBuffersManager::TrackBuffersManager(dom::SourceBuffer* aParent, MediaSourceDecoder* aParentDecoder, const nsACString& aType)
  : mInputBuffer(new MediaLargeByteBuffer)
  , mAppendState(AppendState::WAITING_FOR_SEGMENT)
  , mBufferFull(false)
  , mFirstInitializationSegmentReceived(false)
  , mActiveTrack(false)
  , mType(aType)
  , mParser(ContainerParser::CreateForMIMEType(aType))
  , mTaskQueue(new MediaTaskQueue(GetMediaThreadPool(MediaThreadType::PLAYBACK)))
  , mParent(new nsMainThreadPtrHolder<dom::SourceBuffer>(aParent, false ))
  , mParentDecoder(new nsMainThreadPtrHolder<MediaSourceDecoder>(aParentDecoder, false ))
  , mAbort(false)
  , mMonitor("TrackBuffersManager")
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be instanciated on the main thread");
}

bool
TrackBuffersManager::AppendData(MediaLargeByteBuffer* aData,
                                TimeUnit aTimestampOffset)
{
  MOZ_ASSERT(NS_IsMainThread());
  MonitorAutoLock mon(mMonitor);
  MSE_DEBUG("Appending %lld bytes", aData->Length());
  mIncomingBuffers.AppendElement(IncomingBuffer(aData, aTimestampOffset));
  mEnded = false;
  return true;
}

nsRefPtr<TrackBuffersManager::AppendPromise>
TrackBuffersManager::BufferAppend()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mAppendPromise.IsEmpty());
  nsRefPtr<AppendPromise> p = mAppendPromise.Ensure(__func__);

  nsCOMPtr<nsIRunnable> task =
    NS_NewRunnableMethod(this, &TrackBuffersManager::InitSegmentParserLoop);
  GetTaskQueue()->Dispatch(task.forget());

  return p;
}






void
TrackBuffersManager::AbortAppendData()
{
  MOZ_ASSERT(NS_IsMainThread());
  mAppendPromise.RejectIfExists(NS_ERROR_ABORT, __func__);
}

void
TrackBuffersManager::ResetParserState()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mAppendPromise.IsEmpty(), "AbortAppendData must have been called");

  
  if (mAppendState == AppendState::PARSING_MEDIA_SEGMENT) {
    nsCOMPtr<nsIRunnable> task =
      NS_NewRunnableMethod(this, &TrackBuffersManager::FinishCodedFrameProcessing);
    GetTaskQueue()->Dispatch(task.forget());
  } else {
    nsCOMPtr<nsIRunnable> task =
      NS_NewRunnableMethod(this, &TrackBuffersManager::CompleteResetParserState);
    GetTaskQueue()->Dispatch(task.forget());
  }
}

nsRefPtr<TrackBuffersManager::RangeRemovalPromise>
TrackBuffersManager::RangeRemoval(TimeUnit aStart, TimeUnit aEnd)
{
  MOZ_ASSERT(NS_IsMainThread());
  mEnded = false;

  nsRefPtr<RangeRemovalPromise> p = mRangeRemovalPromise.Ensure(__func__);

  nsCOMPtr<nsIRunnable> task =
  NS_NewRunnableMethodWithArg<TimeInterval>(
    this, &TrackBuffersManager::CodedFrameRemoval, TimeInterval(aStart, aEnd));
  GetTaskQueue()->Dispatch(task.forget());
  return p;
}

TrackBuffersManager::EvictDataResult
TrackBuffersManager::EvictData(TimeUnit aPlaybackTime,
                               uint32_t aThreshold,
                               TimeUnit* aBufferStartTime)
{
  
  return EvictDataResult::NO_DATA_EVICTED;
}

void
TrackBuffersManager::EvictBefore(TimeUnit aTime)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<nsIRunnable> task =
  NS_NewRunnableMethodWithArg<TimeInterval>(
    this, &TrackBuffersManager::CodedFrameRemoval,
    TimeInterval(TimeUnit::FromSeconds(0), aTime));
  GetTaskQueue()->Dispatch(task.forget());
}

media::TimeIntervals
TrackBuffersManager::Buffered()
{
  MonitorAutoLock mon(mMonitor);
  
  
  TimeUnit highestEndTime;

  nsTArray<TimeIntervals*> tracks;
  if (HasVideo()) {
    tracks.AppendElement(&mVideoBufferedRanges);
  }
  if (HasAudio()) {
    tracks.AppendElement(&mAudioBufferedRanges);
  }
  for (auto trackRanges : tracks) {
    highestEndTime = std::max(trackRanges->GetEnd(), highestEndTime);
  }

  
  TimeIntervals intersection{TimeInterval(TimeUnit::FromSeconds(0), highestEndTime)};

  
  
  for (auto trackRanges : tracks) {
    
    if (mEnded) {
      trackRanges->Add(TimeInterval(trackRanges->GetEnd(), highestEndTime));
    }
    
    intersection.Intersection(*trackRanges);
  }
  return intersection;
}

int64_t
TrackBuffersManager::GetSize()
{
  
  return 0;
}

void
TrackBuffersManager::Ended()
{
  mEnded = true;
}

void
TrackBuffersManager::Detach()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mAppendPromise.IsEmpty(), "Abort wasn't called");
  
  mRangeRemovalPromise.ResolveIfExists(false, __func__);
  
  nsCOMPtr<nsIRunnable> task =
  NS_NewRunnableMethodWithArg<TimeInterval>(
    this, &TrackBuffersManager::CodedFrameRemoval,
    TimeInterval(TimeUnit::FromSeconds(0), TimeUnit::FromInfinity()));
  GetTaskQueue()->Dispatch(task.forget());
}

#if defined(DEBUG)
void
TrackBuffersManager::Dump(const char* aPath)
{

}
#endif

void
TrackBuffersManager::FinishCodedFrameProcessing()
{
  MOZ_ASSERT(OnTaskQueue());

  if (mProcessingRequest.Exists()) {
    NS_WARNING("Processing request pending");
    mProcessingRequest.Disconnect();
  }
  
  
  
  
  

  CompleteResetParserState();
}

void
TrackBuffersManager::CompleteResetParserState()
{
  MOZ_ASSERT(OnTaskQueue());

  for (auto track : GetTracksList()) {
    
    track->mLastDecodeTimestamp.reset();
    
    track->mLastFrameDuration.reset();
    
    track->mHighestEndTimestamp.reset();
    
    track->mNeedRandomAccessPoint = true;

    
    
    track->mQueuedSamples.Clear();
  }
  
  mIncomingBuffers.Clear();
  mInputBuffer->Clear();
  if (mCurrentInputBuffer) {
    mCurrentInputBuffer->EvictAll();
    mCurrentInputBuffer = new SourceBufferResource(mType);
  }

  
  
  
  
  if (mFirstInitializationSegmentReceived) {
    nsRefPtr<MediaLargeByteBuffer> initData = mParser->InitData();
    MOZ_ASSERT(initData->Length(), "we must have an init segment");
    
    CreateDemuxerforMIMEType();
    mInputBuffer->AppendElements(*initData);
  }
  RecreateParser();

  
  SetAppendState(AppendState::WAITING_FOR_SEGMENT);

  
  mAbort = false;
}

void
TrackBuffersManager::CodedFrameRemoval(TimeInterval aInterval)
{
  MSE_DEBUG("From %.2fs to %.2f",
            aInterval.mStart.ToSeconds(), aInterval.mEnd.ToSeconds());
  TimeUnit duration{TimeUnit::FromSeconds(mParentDecoder->GetMediaSourceDuration())};

  MSE_DEBUG("duration:%.2f", duration.ToSeconds());
  MSE_DEBUG("before video ranges=%s", DumpTimeRanges(mVideoTracks.mBufferedRanges).get());
  MSE_DEBUG("before audio ranges=%s", DumpTimeRanges(mAudioTracks.mBufferedRanges).get());

  
  TimeUnit start = aInterval.mStart;
  
  TimeUnit end = aInterval.mEnd;

  
  for (auto track : GetTracksList()) {
    MSE_DEBUGV("Processing %s track", track->mInfo->mMimeType.get());
    
    
    TimeUnit removeEndTimestamp = std::max(duration, track->mBufferedRanges.GetEnd());

    
    
    if (end < track->mBufferedRanges.GetEnd()) {
      for (auto& frame : track->mBuffers.LastElement()) {
        if (frame->mKeyframe && frame->mTime >= end.ToMicroseconds()) {
          removeEndTimestamp = TimeUnit::FromMicroseconds(frame->mTime);
          break;
        }
      }
    }
    
    
    TimeInterval removedInterval;
    int32_t firstRemovedIndex = -1;
    TrackBuffer& data = track->mBuffers.LastElement();
    for (uint32_t i = 0; i < data.Length(); i++) {
      const auto& frame = data[i];
      if (frame->mTime >= start.ToMicroseconds() &&
          frame->mTime < removeEndTimestamp.ToMicroseconds()) {
        if (firstRemovedIndex < 0) {
          removedInterval =
            TimeInterval(TimeUnit::FromMicroseconds(frame->mTime),
                         TimeUnit::FromMicroseconds(frame->mTime + frame->mDuration));
          firstRemovedIndex = i;
        } else {
          removedInterval = removedInterval.Span(
            TimeInterval(TimeUnit::FromMicroseconds(frame->mTime),
                         TimeUnit::FromMicroseconds(frame->mTime + frame->mDuration)));
        }
        data.RemoveElementAt(i);
      }
    }
    
    
    if (firstRemovedIndex >= 0) {
      for (uint32_t i = firstRemovedIndex; i < data.Length(); i++) {
        const auto& frame = data[i];
        if (frame->mKeyframe) {
          break;
        }
        removedInterval = removedInterval.Span(
          TimeInterval(TimeUnit::FromMicroseconds(frame->mTime),
                       TimeUnit::FromMicroseconds(frame->mTime + frame->mDuration)));
        data.RemoveElementAt(i);
      }
    }
    track->mBufferedRanges -= removedInterval;

    
    
    
    
    
    
  }
  
  
  mBufferFull = false;
  {
    MonitorAutoLock mon(mMonitor);
    mVideoBufferedRanges = mVideoTracks.mBufferedRanges;
    mAudioBufferedRanges = mAudioTracks.mBufferedRanges;
  }
  MSE_DEBUG("after video ranges=%s", DumpTimeRanges(mVideoTracks.mBufferedRanges).get());
  MSE_DEBUG("after audio ranges=%s", DumpTimeRanges(mAudioTracks.mBufferedRanges).get());

  mRangeRemovalPromise.ResolveIfExists(true, __func__);
}

void
TrackBuffersManager::InitSegmentParserLoop()
{
  AppendIncomingBuffers();
  SegmentParserLoop();
}

void
TrackBuffersManager::AppendIncomingBuffers()
{
  MOZ_ASSERT(OnTaskQueue());
  MonitorAutoLock mon(mMonitor);
  for (auto& incomingBuffer : mIncomingBuffers) {
    if (!mInputBuffer->AppendElements(*incomingBuffer.first())) {
      RejectAppend(NS_ERROR_OUT_OF_MEMORY, __func__);
    }
  }
  if (!mIncomingBuffers.IsEmpty()) {
    mTimestampOffset = mIncomingBuffers.LastElement().second();
    mLastTimestampOffset = mTimestampOffset;
  }
  mIncomingBuffers.Clear();
}

void
TrackBuffersManager::SegmentParserLoop()
{
  MOZ_ASSERT(OnTaskQueue());
  while (true) {
    bool completeMediaHeader;
    
    if (!mInputBuffer || mInputBuffer->IsEmpty()) {
      NeedMoreData();
      return;
    }
    
    
    
    

    
    
    

    
    
    if (mAppendState == AppendState::WAITING_FOR_SEGMENT) {
      if (mParser->IsInitSegmentPresent(mInputBuffer)) {
        SetAppendState(AppendState::PARSING_INIT_SEGMENT);
        continue;
      }
      if (mParser->IsMediaSegmentPresent(mInputBuffer)) {
        SetAppendState(AppendState::PARSING_MEDIA_SEGMENT);
        continue;
      }
      
      
      
      MSE_DEBUG("Found invalid data, ignoring for now");
      NeedMoreData();
      return;
    }

    int64_t start, end;
    completeMediaHeader =
      mParser->ParseStartAndEndTimestamps(mInputBuffer, start, end);

    
    
    if (mAppendState == AppendState::PARSING_INIT_SEGMENT) {
      if (mParser->InitSegmentRange().IsNull()) {
        NeedMoreData();
        return;
      }
      InitializationSegmentReceived();
      return;
    }
    if (mAppendState == AppendState::PARSING_MEDIA_SEGMENT) {
      
      if (!mFirstInitializationSegmentReceived) {
        RejectAppend(NS_ERROR_FAILURE, __func__);
        return;
      }
      
      if (!completeMediaHeader) {
        NeedMoreData();
        return;
      }
      
      nsRefPtr<TrackBuffersManager> self = this;
      mProcessingRequest.Begin(CodedFrameProcessing()
          ->Then(GetTaskQueue(), __func__,
                 [self] (bool aNeedMoreData) {
                   self->mProcessingRequest.Complete();
                   if (aNeedMoreData || self->mAbort) {
                     self->NeedMoreData();
                   } else {
                     self->ScheduleSegmentParserLoop();
                   }
                 },
                 [self] (nsresult aRejectValue) {
                   self->mProcessingRequest.Complete();
                   self->RejectAppend(aRejectValue, __func__);
                 }));
      return;
    }
  }
}

void
TrackBuffersManager::NeedMoreData()
{
  MSE_DEBUG("");
  
  
  
  RecreateParser();
  RestoreCachedVariables();
  mAppendPromise.ResolveIfExists(mActiveTrack, __func__);
}

void
TrackBuffersManager::RejectAppend(nsresult aRejectValue, const char* aName)
{
  MSE_DEBUG("rv=%d", aRejectValue);
  mAppendPromise.RejectIfExists(aRejectValue, aName);
}

void
TrackBuffersManager::ScheduleSegmentParserLoop()
{
  nsCOMPtr<nsIRunnable> task =
    NS_NewRunnableMethod(this, &TrackBuffersManager::SegmentParserLoop);
  GetTaskQueue()->Dispatch(task.forget());
}

void
TrackBuffersManager::CreateDemuxerforMIMEType()
{
  if (mVideoTracks.mDemuxer) {
    mVideoTracks.mDemuxer->BreakCycles();
    mVideoTracks.mDemuxer = nullptr;
  }
  if (mAudioTracks.mDemuxer) {
    mAudioTracks.mDemuxer->BreakCycles();
    mAudioTracks.mDemuxer = nullptr;
  }
  mInputDemuxer = nullptr;
  if (mType.LowerCaseEqualsLiteral("video/webm") || mType.LowerCaseEqualsLiteral("audio/webm")) {
    MOZ_ASSERT(false, "Waiting on WebMDemuxer");
  
  }

#ifdef MOZ_FMP4
  if (mType.LowerCaseEqualsLiteral("video/mp4") || mType.LowerCaseEqualsLiteral("audio/mp4")) {
    mInputDemuxer = new MP4Demuxer(mCurrentInputBuffer);
    return;
  }
#endif
  MOZ_ASSERT(false, "Not supported (yet)");
}

void
TrackBuffersManager::InitializationSegmentReceived()
{
  MOZ_ASSERT(mParser->HasCompleteInitData());
  mCurrentInputBuffer = new SourceBufferResource(mType);
  mCurrentInputBuffer->AppendData(mParser->InitData());
  CreateDemuxerforMIMEType();
  if (!mInputDemuxer) {
    MOZ_ASSERT(false, "TODO type not supported");
    return;
  }
  mDemuxerInitRequest.Begin(mInputDemuxer->Init()
                      ->Then(GetTaskQueue(), __func__,
                             this,
                             &TrackBuffersManager::OnDemuxerInitDone,
                             &TrackBuffersManager::OnDemuxerInitFailed));
}

void
TrackBuffersManager::OnDemuxerInitDone(nsresult)
{
  mDemuxerInitRequest.Complete();

  if (mAbort) {
    RejectAppend(NS_ERROR_ABORT, __func__);
    return;
  }

  MediaInfo info;

  uint32_t numVideos = mInputDemuxer->GetNumberTracks(TrackInfo::kVideoTrack);
  if (numVideos) {
    
    mVideoTracks.mDemuxer = mInputDemuxer->GetTrackDemuxer(TrackInfo::kVideoTrack, 0);
    MOZ_ASSERT(mVideoTracks.mDemuxer);
    info.mVideo = *mVideoTracks.mDemuxer->GetInfo()->GetAsVideoInfo();
  }

  uint32_t numAudios = mInputDemuxer->GetNumberTracks(TrackInfo::kAudioTrack);
  if (numAudios) {
    
    mAudioTracks.mDemuxer = mInputDemuxer->GetTrackDemuxer(TrackInfo::kAudioTrack, 0);
    MOZ_ASSERT(mAudioTracks.mDemuxer);
    info.mAudio = *mAudioTracks.mDemuxer->GetInfo()->GetAsAudioInfo();
  }

  int64_t videoDuration = numVideos ? info.mVideo.mDuration : 0;
  int64_t audioDuration = numAudios ? info.mAudio.mDuration : 0;

  int64_t duration = std::max(videoDuration, audioDuration);
  
  
  nsCOMPtr<nsIRunnable> task =
    NS_NewRunnableMethodWithArg<int64_t>(mParentDecoder,
                                         &MediaSourceDecoder::SetInitialDuration,
                                         duration ? duration : -1);
  AbstractThread::MainThread()->Dispatch(task.forget());

  
  
  
  if (!numVideos && !numAudios) {
    RejectAppend(NS_ERROR_FAILURE, __func__);
    return;
  }

  
  if (mFirstInitializationSegmentReceived) {
    if (numVideos != mVideoTracks.mNumTracks ||
        numAudios != mAudioTracks.mNumTracks ||
        (numVideos && info.mVideo.mMimeType != mVideoTracks.mInfo->mMimeType) ||
        (numAudios && info.mAudio.mMimeType != mAudioTracks.mInfo->mMimeType)) {
      RejectAppend(NS_ERROR_FAILURE, __func__);
      return;
    }
    
    
    
    
    
    
    
    mVideoTracks.mNeedRandomAccessPoint = true;
    mAudioTracks.mNeedRandomAccessPoint = true;
  }

  
  mActiveTrack = false;

  
  if (!mFirstInitializationSegmentReceived) {
    mAudioTracks.mNumTracks = numAudios;
    
    
    
    

    
    
    if (numAudios) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      mActiveTrack = true;
      
      
      
      
      mAudioTracks.mBuffers.AppendElement(TrackBuffer());
      
      mAudioTracks.mInfo = info.mAudio.Clone();
    }

    mVideoTracks.mNumTracks = numVideos;
    
    
    if (numVideos) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      mActiveTrack = true;
      
      
      
      
      mVideoTracks.mBuffers.AppendElement(TrackBuffer());
      
      mVideoTracks.mInfo = info.mVideo.Clone();
    }
    
    
    

    
    mFirstInitializationSegmentReceived = true;
  }

  
  UniquePtr<EncryptionInfo> crypto = mInputDemuxer->GetCrypto();
  if (crypto && crypto->IsEncrypted()) {
#ifdef MOZ_EME
    
    for (uint32_t i = 0; i < crypto->mInitDatas.Length(); i++) {


    }
#endif 
    info.mCrypto = *crypto;
    mEncrypted = true;
  }

  {
    MonitorAutoLock mon(mMonitor);
    mInfo = info;
  }

  
  mInputBuffer->RemoveElementsAt(0, mParser->InitSegmentRange().mEnd);
  RecreateParser();

  
  SetAppendState(AppendState::WAITING_FOR_SEGMENT);
  
  ScheduleSegmentParserLoop();
}

void
TrackBuffersManager::OnDemuxerInitFailed(DemuxerFailureReason aFailure)
{
  MOZ_ASSERT(aFailure != DemuxerFailureReason::WAITING_FOR_DATA);
  mDemuxerInitRequest.Complete();

  RejectAppend(NS_ERROR_FAILURE, __func__);
}

nsRefPtr<TrackBuffersManager::CodedFrameProcessingPromise>
TrackBuffersManager::CodedFrameProcessing()
{
  MOZ_ASSERT(mProcessingPromise.IsEmpty());
  nsRefPtr<CodedFrameProcessingPromise> p = mProcessingPromise.Ensure(__func__);

  int64_t offset = mCurrentInputBuffer->GetLength();
  MediaByteRange mediaRange = mParser->MediaSegmentRange();
  
  int64_t rangeOffset = mParser->InitData()->Length();
  int64_t length;
  if (mediaRange.IsNull()) {
    length = mInputBuffer->Length();
    mCurrentInputBuffer->AppendData(mInputBuffer);
  } else {
    nsRefPtr<MediaLargeByteBuffer> segment = new MediaLargeByteBuffer;
    length = mediaRange.Length();
    MOZ_ASSERT(mInputBuffer->Length() >= uint64_t(mediaRange.mEnd - rangeOffset),
               "We're missing some data");
    MOZ_ASSERT(mediaRange.mStart >= rangeOffset, "Invalid media segment range");
    if (!segment->AppendElements(mInputBuffer->Elements() +
                                 mediaRange.mStart - rangeOffset, length)) {
      return CodedFrameProcessingPromise::CreateAndReject(NS_ERROR_OUT_OF_MEMORY, __func__);
    }
    mCurrentInputBuffer->AppendData(segment);
  }
  mInputDemuxer->NotifyDataArrived(length, offset);

  DoDemuxVideo();

  return p;
}

void
TrackBuffersManager::OnDemuxFailed(TrackType aTrack,
                                   DemuxerFailureReason aFailure)
{
  MSE_DEBUG("Failed to demux %s, failure = %d",
      aTrack == TrackType::kVideoTrack ? "video" : "audio", aFailure);
  if (mAbort) {
    mProcessingPromise.RejectIfExists(NS_ERROR_ABORT, __func__);
    return;
  }
  switch (aFailure) {
    case DemuxerFailureReason::END_OF_STREAM:
    case DemuxerFailureReason::WAITING_FOR_DATA:
      if (aTrack == TrackType::kVideoTrack) {
        DoDemuxAudio();
      } else {
        CompleteCodedFrameProcessing();
      }
      break;
    case DemuxerFailureReason::DEMUXER_ERROR:
      mProcessingPromise.RejectIfExists(NS_ERROR_FAILURE, __func__);
      break;
    case DemuxerFailureReason::CANCELED:
    case DemuxerFailureReason::SHUTDOWN:
      mProcessingPromise.RejectIfExists(NS_ERROR_ABORT, __func__);
      break;
    default:
      MOZ_ASSERT(false);
      break;
  }
}

void
TrackBuffersManager::DoDemuxVideo()
{
  if (!HasVideo()) {
    DoDemuxAudio();
    return;
  }
  mVideoTracks.mDemuxRequest.Begin(mVideoTracks.mDemuxer->GetSamples(-1)
                             ->Then(GetTaskQueue(), __func__, this,
                                    &TrackBuffersManager::OnVideoDemuxCompleted,
                                    &TrackBuffersManager::OnVideoDemuxFailed));
}

void
TrackBuffersManager::OnVideoDemuxCompleted(nsRefPtr<MediaTrackDemuxer::SamplesHolder> aSamples)
{
  MSE_DEBUG("%d video samples demuxed", aSamples->mSamples.Length());
  mVideoTracks.mDemuxRequest.Complete();
  if (mAbort) {
    mProcessingPromise.RejectIfExists(NS_ERROR_ABORT, __func__);
    return;
  }
  mVideoTracks.mQueuedSamples.AppendElements(aSamples->mSamples);
  DoDemuxAudio();
}

void
TrackBuffersManager::DoDemuxAudio()
{
  if (mAbort) {
    mProcessingPromise.RejectIfExists(NS_ERROR_ABORT, __func__);
    return;
  }
  if (!HasAudio()) {
    CompleteCodedFrameProcessing();
    return;
  }
  mAudioTracks.mDemuxRequest.Begin(mAudioTracks.mDemuxer->GetSamples(-1)
                             ->Then(GetTaskQueue(), __func__, this,
                                    &TrackBuffersManager::OnAudioDemuxCompleted,
                                    &TrackBuffersManager::OnAudioDemuxFailed));
}

void
TrackBuffersManager::OnAudioDemuxCompleted(nsRefPtr<MediaTrackDemuxer::SamplesHolder> aSamples)
{
  MSE_DEBUG("%d audio samples demuxed", aSamples->mSamples.Length());
  mAudioTracks.mDemuxRequest.Complete();
  if (mAbort) {
    mProcessingPromise.RejectIfExists(NS_ERROR_ABORT, __func__);
    return;
  }
  mAudioTracks.mQueuedSamples.AppendElements(aSamples->mSamples);
  CompleteCodedFrameProcessing();
}

void
TrackBuffersManager::CompleteCodedFrameProcessing()
{
  MOZ_ASSERT(OnTaskQueue());

  

  for (auto& sample : mVideoTracks.mQueuedSamples) {
    while (true) {
      if (!ProcessFrame(sample, mVideoTracks)) {
        break;
      }
    }
  }
  mVideoTracks.mQueuedSamples.Clear();

  for (auto& sample : mAudioTracks.mQueuedSamples) {
    while (true) {
      if (!ProcessFrame(sample, mAudioTracks)) {
        break;
      }
    }
  }
  mAudioTracks.mQueuedSamples.Clear();

  {
    MonitorAutoLock mon(mMonitor);

    
    mVideoBufferedRanges = mVideoTracks.mBufferedRanges;
    mAudioBufferedRanges = mAudioTracks.mBufferedRanges;
  }

  
  
  
  mBufferFull = false;

  
  if (mParser->MediaSegmentRange().IsNull()) {
    mProcessingPromise.ResolveIfExists(true, __func__);
    return;
  }

  
  int64_t rangeOffset = mParser->InitSegmentRange().mEnd;
  mInputBuffer->RemoveElementsAt(0, mParser->MediaSegmentRange().mEnd - rangeOffset);
  RecreateParser();

  
  
  
  mCurrentInputBuffer->EvictAll();
  mInputDemuxer->NotifyDataRemoved();

  
  SetAppendState(AppendState::WAITING_FOR_SEGMENT);

  
  mProcessingPromise.ResolveIfExists(false, __func__);
}

bool
TrackBuffersManager::ProcessFrame(MediaRawData* aSample,
                                  TrackData& aTrackData)
{
  TrackData* tracks[] = { &mVideoTracks, &mAudioTracks };
  TimeUnit presentationTimestamp;
  TimeUnit decodeTimestamp;

  if (!mParent->mGenerateTimestamp) {
    presentationTimestamp = TimeUnit::FromMicroseconds(aSample->mTime);
    decodeTimestamp = TimeUnit::FromMicroseconds(aSample->mTimecode);
  }

  
  TimeUnit frameDuration{TimeUnit::FromMicroseconds(aSample->mDuration)};

  
  if (mParent->mAppendMode == SourceBufferAppendMode::Sequence &&
      mGroupStartTimestamp.isSome()) {
    mTimestampOffset = mGroupStartTimestamp.ref();
    mGroupEndTimestamp = mGroupStartTimestamp.ref();
    mVideoTracks.mNeedRandomAccessPoint = true;
    mAudioTracks.mNeedRandomAccessPoint = true;
    mGroupStartTimestamp.reset();
  }

  
  if (mTimestampOffset != TimeUnit::FromSeconds(0)) {
    presentationTimestamp += mTimestampOffset;
    decodeTimestamp += mTimestampOffset;
  }

  MSE_DEBUGV("Processing %s frame(pts:%lld end:%lld, dts:%lld, duration:%lld, "
             "kf:%d)",
             aTrackData.mInfo->mMimeType.get(),
             presentationTimestamp.ToMicroseconds(),
             (presentationTimestamp + frameDuration).ToMicroseconds(),
             decodeTimestamp.ToMicroseconds(),
             frameDuration.ToMicroseconds(),
             aSample->mKeyframe);

  
  auto& trackBuffer = aTrackData;

  
  
  

  

  if ((trackBuffer.mLastDecodeTimestamp.isSome() &&
       decodeTimestamp < trackBuffer.mLastDecodeTimestamp.ref()) ||
      (trackBuffer.mLastDecodeTimestamp.isSome() &&
       decodeTimestamp - trackBuffer.mLastDecodeTimestamp.ref() > 2*trackBuffer.mLastFrameDuration.ref())) {
    if (mParent->mAppendMode == SourceBufferAppendMode::Segments) {
      mGroupEndTimestamp = presentationTimestamp;
    }
    if (mParent->mAppendMode == SourceBufferAppendMode::Sequence) {
      mGroupStartTimestamp = Some(mGroupEndTimestamp);
    }
    for (auto& track : tracks) {
      track->mLastDecodeTimestamp.reset();
      track->mLastFrameDuration.reset();
      track->mHighestEndTimestamp.reset();
      track->mNeedRandomAccessPoint = true;
    }
    return true;
  }

  
  TimeUnit frameEndTimestamp = presentationTimestamp + frameDuration;

  
  if (presentationTimestamp.ToSeconds() < mParent->mAppendWindowStart) {
    trackBuffer.mNeedRandomAccessPoint = true;
    return false;
  }

  
  if (frameEndTimestamp.ToSeconds() > mParent->mAppendWindowEnd) {
    trackBuffer.mNeedRandomAccessPoint = true;
    return false;
  }

  
  if (trackBuffer.mNeedRandomAccessPoint) {
    
    if (!aSample->mKeyframe) {
      return false;
    }
    
    trackBuffer.mNeedRandomAccessPoint = false;
  }

  
  
  

  
  
  

  

  
  
  
  int firstRemovedIndex = -1;
  TimeInterval removedInterval;
  TrackBuffer& data = trackBuffer.mBuffers.LastElement();
  if (trackBuffer.mBufferedRanges.Contains(presentationTimestamp)) {
    if (trackBuffer.mHighestEndTimestamp.isNothing()) {
      for (uint32_t i = 0; i < data.Length(); i++) {
        MediaRawData* sample = data[i].get();
        if (sample->mTime >= presentationTimestamp.ToMicroseconds() &&
            sample->mTime < frameEndTimestamp.ToMicroseconds()) {
          if (firstRemovedIndex < 0) {
            removedInterval =
              TimeInterval(TimeUnit::FromMicroseconds(sample->mTime),
                           TimeUnit::FromMicroseconds(sample->mTime + sample->mDuration));
            firstRemovedIndex = i;
          } else {
            removedInterval = removedInterval.Span(
              TimeInterval(TimeUnit::FromMicroseconds(sample->mTime),
                           TimeUnit::FromMicroseconds(sample->mTime + sample->mDuration)));
          }
          data.RemoveElementAt(i);
        }
      }
    } else if (trackBuffer.mHighestEndTimestamp.ref() <= presentationTimestamp) {
      for (uint32_t i = 0; i < data.Length(); i++) {
        MediaRawData* sample = data[i].get();
        if (sample->mTime >= trackBuffer.mHighestEndTimestamp.ref().ToMicroseconds() &&
            sample->mTime < frameEndTimestamp.ToMicroseconds()) {
          if (firstRemovedIndex < 0) {
            removedInterval =
              TimeInterval(TimeUnit::FromMicroseconds(sample->mTime),
                           TimeUnit::FromMicroseconds(sample->mTime + sample->mDuration));
            firstRemovedIndex = i;
          } else {
            removedInterval = removedInterval.Span(
              TimeInterval(TimeUnit::FromMicroseconds(sample->mTime),
                           TimeUnit::FromMicroseconds(sample->mTime + sample->mDuration)));
          }
          data.RemoveElementAt(i);
        }
      }
    }
  }
  
  
  if (firstRemovedIndex >= 0) {
    for (uint32_t i = firstRemovedIndex; i < data.Length(); i++) {
      MediaRawData* sample = data[i].get();
      if (sample->mKeyframe) {
        break;
      }
      removedInterval = removedInterval.Span(
        TimeInterval(TimeUnit::FromMicroseconds(sample->mTime),
                     TimeUnit::FromMicroseconds(sample->mTime + sample->mDuration)));
      data.RemoveElementAt(i);
    }
    
    trackBuffer.mBufferedRanges -= removedInterval;
  }

  
  aSample->mTime = presentationTimestamp.ToMicroseconds();
  aSample->mTimecode = decodeTimestamp.ToMicroseconds();
  if (firstRemovedIndex >= 0) {
    data.InsertElementAt(firstRemovedIndex, aSample);
  } else {
    data.AppendElement(aSample);
  }
  
  trackBuffer.mLastDecodeTimestamp = Some(decodeTimestamp);
  
  trackBuffer.mLastFrameDuration = Some(TimeUnit::FromMicroseconds(aSample->mDuration));
  
  if (trackBuffer.mHighestEndTimestamp.isNothing() ||
      frameEndTimestamp > trackBuffer.mHighestEndTimestamp.ref()) {
    trackBuffer.mHighestEndTimestamp = Some(frameEndTimestamp);
  }
  
  if (frameEndTimestamp > mGroupEndTimestamp) {
    mGroupEndTimestamp = frameEndTimestamp;
  }
  
  if (mParent->mGenerateTimestamp) {
    mTimestampOffset = frameEndTimestamp;
  }

  
  
  
  
  trackBuffer.mBufferedRanges +=
    TimeInterval(presentationTimestamp, frameEndTimestamp,
                 TimeUnit::FromMicroseconds(aSample->mDuration / 2));
  return false;
}

void
TrackBuffersManager::RecreateParser()
{
  
  
  
  
  nsRefPtr<MediaLargeByteBuffer> initData = mParser->InitData();
  mParser = ContainerParser::CreateForMIMEType(mType);
  if (initData) {
    int64_t start, end;
    mParser->ParseStartAndEndTimestamps(initData, start, end);
  }
}

nsTArray<TrackBuffersManager::TrackData*>
TrackBuffersManager::GetTracksList()
{
  nsTArray<TrackData*> tracks;
  if (HasVideo()) {
    tracks.AppendElement(&mVideoTracks);
  }
  if (HasAudio()) {
    tracks.AppendElement(&mAudioTracks);
  }
  return tracks;
}

void
TrackBuffersManager::RestoreCachedVariables()
{
  if (mTimestampOffset != mLastTimestampOffset) {
    nsCOMPtr<nsIRunnable> task =
      NS_NewRunnableMethodWithArg<TimeUnit>(
        mParent.get(),
        static_cast<void (dom::SourceBuffer::*)(const TimeUnit&)>(&dom::SourceBuffer::SetTimestampOffset), 
        mTimestampOffset);
    AbstractThread::MainThread()->Dispatch(task.forget());
  }
}

void
TrackBuffersManager::SetAppendState(TrackBuffersManager::AppendState aAppendState)
{
  MSE_DEBUG("AppendState changed from %s to %s",
            AppendStateToStr(mAppendState), AppendStateToStr(aAppendState));
  mAppendState = aAppendState;
}

void
TrackBuffersManager::SetGroupStartTimestamp(const TimeUnit& aGroupStartTimestamp)
{
  if (NS_IsMainThread()) {
    nsCOMPtr<nsIRunnable> task =
      NS_NewRunnableMethodWithArg<TimeUnit>(
        this,
        &TrackBuffersManager::SetGroupStartTimestamp,
        aGroupStartTimestamp);
    GetTaskQueue()->Dispatch(task.forget());
    return;
  }
  MOZ_ASSERT(OnTaskQueue());
  mGroupStartTimestamp = Some(aGroupStartTimestamp);
}

void
TrackBuffersManager::RestartGroupStartTimestamp()
{
  if (NS_IsMainThread()) {
    nsCOMPtr<nsIRunnable> task =
      NS_NewRunnableMethod(this, &TrackBuffersManager::RestartGroupStartTimestamp);
    GetTaskQueue()->Dispatch(task.forget());
    return;
  }
  MOZ_ASSERT(OnTaskQueue());
  mGroupStartTimestamp = Some(mGroupEndTimestamp);
}

TrackBuffersManager::~TrackBuffersManager()
{
  mTaskQueue->BeginShutdown();
  mTaskQueue = nullptr;
}

MediaInfo
TrackBuffersManager::GetMetadata()
{
  MonitorAutoLock mon(mMonitor);
  return mInfo;
}

const TimeIntervals&
TrackBuffersManager::Buffered(TrackInfo::TrackType aTrack)
{
  MOZ_ASSERT(OnTaskQueue());
  return GetTracksData(aTrack).mBufferedRanges;
}

const TrackBuffersManager::TrackBuffer&
TrackBuffersManager::GetTrackBuffer(TrackInfo::TrackType aTrack)
{
  MOZ_ASSERT(OnTaskQueue());
  return GetTracksData(aTrack).mBuffers.LastElement();
}

}
#undef MSE_DEBUG
