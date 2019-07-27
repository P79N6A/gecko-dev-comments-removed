





#include "TrackBuffersManager.h"
#include "SourceBufferResource.h"
#include "SourceBuffer.h"
#include "MediaSourceDemuxer.h"

#ifdef MOZ_FMP4
#include "MP4Demuxer.h"
#endif

#include <limits>

extern PRLogModuleInfo* GetMediaSourceLog();

#define MSE_DEBUG(arg, ...) MOZ_LOG(GetMediaSourceLog(), mozilla::LogLevel::Debug, ("TrackBuffersManager(%p:%s)::%s: " arg, this, mType.get(), __func__, ##__VA_ARGS__))
#define MSE_DEBUGV(arg, ...) MOZ_LOG(GetMediaSourceLog(), mozilla::LogLevel::Verbose, ("TrackBuffersManager(%p:%s)::%s: " arg, this, mType.get(), __func__, ##__VA_ARGS__))

PRLogModuleInfo* GetMediaSourceSamplesLog()
{
  static PRLogModuleInfo* sLogModule = nullptr;
  if (!sLogModule) {
    sLogModule = PR_NewLogModule("MediaSourceSamples");
  }
  return sLogModule;
}
#define SAMPLE_DEBUG(arg, ...) MOZ_LOG(GetMediaSourceSamplesLog(), mozilla::LogLevel::Debug, ("TrackBuffersManager(%p:%s)::%s: " arg, this, mType.get(), __func__, ##__VA_ARGS__))

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

static Atomic<uint32_t> sStreamSourceID(0u);

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

TrackBuffersManager::TrackBuffersManager(dom::SourceBuffer* aParent, MediaSourceDecoder* aParentDecoder, const nsACString& aType)
  : mInputBuffer(new MediaByteBuffer)
  , mAppendState(AppendState::WAITING_FOR_SEGMENT)
  , mBufferFull(false)
  , mFirstInitializationSegmentReceived(false)
  , mActiveTrack(false)
  , mType(aType)
  , mParser(ContainerParser::CreateForMIMEType(aType))
  , mProcessedInput(0)
  , mAppendRunning(false)
  , mTaskQueue(aParentDecoder->GetDemuxer()->GetTaskQueue())
  , mParent(new nsMainThreadPtrHolder<dom::SourceBuffer>(aParent, false ))
  , mParentDecoder(new nsMainThreadPtrHolder<MediaSourceDecoder>(aParentDecoder, false ))
  , mMediaSourceDemuxer(mParentDecoder->GetDemuxer())
  , mMediaSourceDuration(mTaskQueue, Maybe<double>(), "TrackBuffersManager::mMediaSourceDuration (Mirror)")
  , mAbort(false)
  , mEvictionThreshold(Preferences::GetUint("media.mediasource.eviction_threshold",
                                            100 * (1 << 20)))
  , mEvictionOccurred(false)
  , mMonitor("TrackBuffersManager")
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be instanciated on the main thread");
  nsRefPtr<TrackBuffersManager> self = this;
  nsCOMPtr<nsIRunnable> task =
    NS_NewRunnableFunction([self] () {
      self->mMediaSourceDuration.Connect(self->mParentDecoder->CanonicalExplicitDuration());
    });
  GetTaskQueue()->Dispatch(task.forget());
}

TrackBuffersManager::~TrackBuffersManager()
{
}

bool
TrackBuffersManager::AppendData(MediaByteBuffer* aData,
                                TimeUnit aTimestampOffset)
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_DEBUG("Appending %lld bytes", aData->Length());

  mEnded = false;
  nsCOMPtr<nsIRunnable> task =
    NS_NewRunnableMethodWithArg<IncomingBuffer>(
      this, &TrackBuffersManager::AppendIncomingBuffer,
      IncomingBuffer(aData, aTimestampOffset));
  GetTaskQueue()->Dispatch(task.forget());
  return true;
}

void
TrackBuffersManager::AppendIncomingBuffer(IncomingBuffer aData)
{
  MOZ_ASSERT(OnTaskQueue());
  mIncomingBuffers.AppendElement(aData);
  mAbort = false;
}

nsRefPtr<TrackBuffersManager::AppendPromise>
TrackBuffersManager::BufferAppend()
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_DEBUG("");

  return ProxyMediaCall(GetTaskQueue(), this,
                        __func__, &TrackBuffersManager::InitSegmentParserLoop);
}









void
TrackBuffersManager::AbortAppendData()
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_DEBUG("");

  mAbort = true;
}

void
TrackBuffersManager::ResetParserState()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mAppendRunning, "AbortAppendData must have been called");
  MSE_DEBUG("");

  
  if (mAppendState == AppendState::PARSING_MEDIA_SEGMENT) {
    nsCOMPtr<nsIRunnable> task =
      NS_NewRunnableMethod(this, &TrackBuffersManager::FinishCodedFrameProcessing);
    GetTaskQueue()->Dispatch(task.forget());
  } else {
    nsCOMPtr<nsIRunnable> task =
      NS_NewRunnableMethod(this, &TrackBuffersManager::CompleteResetParserState);
    GetTaskQueue()->Dispatch(task.forget());
  }

  
  
  
  
  
  SetAppendState(AppendState::WAITING_FOR_SEGMENT);
}

nsRefPtr<TrackBuffersManager::RangeRemovalPromise>
TrackBuffersManager::RangeRemoval(TimeUnit aStart, TimeUnit aEnd)
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_DEBUG("From %.2f to %.2f", aStart.ToSeconds(), aEnd.ToSeconds());

  mEnded = false;

  return ProxyMediaCall(GetTaskQueue(), this, __func__,
                        &TrackBuffersManager::CodedFrameRemovalWithPromise,
                        TimeInterval(aStart, aEnd));
}

TrackBuffersManager::EvictDataResult
TrackBuffersManager::EvictData(TimeUnit aPlaybackTime,
                               uint32_t aThreshold,
                               TimeUnit* aBufferStartTime)
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_DEBUG("");

  int64_t toEvict = GetSize() - aThreshold;
  if (toEvict <= 0) {
    return EvictDataResult::NO_DATA_EVICTED;
  }
  if (toEvict <= 512*1024) {
    
    return EvictDataResult::CANT_EVICT;
  }

  if (mBufferFull && mEvictionOccurred) {
    return EvictDataResult::BUFFER_FULL;
  }

  MSE_DEBUG("Reaching our size limit, schedule eviction of %lld bytes", toEvict);

  nsCOMPtr<nsIRunnable> task =
    NS_NewRunnableMethodWithArgs<TimeUnit, uint32_t>(
      this, &TrackBuffersManager::DoEvictData,
      aPlaybackTime, toEvict);
  GetTaskQueue()->Dispatch(task.forget());

  return EvictDataResult::NO_DATA_EVICTED;
}

void
TrackBuffersManager::EvictBefore(TimeUnit aTime)
{
  MOZ_ASSERT(NS_IsMainThread());
  MSE_DEBUG("");

  nsCOMPtr<nsIRunnable> task =
    NS_NewRunnableMethodWithArg<TimeInterval>(
      this, &TrackBuffersManager::CodedFrameRemoval,
      TimeInterval(TimeUnit::FromSeconds(0), aTime));
  GetTaskQueue()->Dispatch(task.forget());
}

media::TimeIntervals
TrackBuffersManager::Buffered()
{
  MSE_DEBUG("");
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
  return mSizeSourceBuffer;
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
  MSE_DEBUG("");

  nsRefPtr<TrackBuffersManager> self = this;
  nsCOMPtr<nsIRunnable> task =
    NS_NewRunnableFunction([self] () {
      
      self->CodedFrameRemoval(TimeInterval(TimeUnit::FromSeconds(0),
                                           TimeUnit::FromInfinity()));
      self->mMediaSourceDuration.DisconnectIfConnected();
    });
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
  MOZ_ASSERT(!mAppendRunning);
  MSE_DEBUG("");

  for (auto& track : GetTracksList()) {
    
    
    
    
    track->ResetAppendState();

    
    
    track->mQueuedSamples.Clear();
  }
  
  mIncomingBuffers.Clear();
  mInputBuffer = nullptr;
  if (mCurrentInputBuffer) {
    mCurrentInputBuffer->EvictAll();
    mCurrentInputBuffer = new SourceBufferResource(mType);
  }

  
  
  
  
  if (mFirstInitializationSegmentReceived) {
    MOZ_ASSERT(mInitData && mInitData->Length(), "we must have an init segment");
    
    CreateDemuxerforMIMEType();
    
    
    mInputBuffer = new MediaByteBuffer;
    mInputBuffer->AppendElements(*mInitData);
  }
  RecreateParser();

  
  SetAppendState(AppendState::WAITING_FOR_SEGMENT);

  
  mAppendPromise.RejectIfExists(NS_ERROR_ABORT, __func__);
}

void
TrackBuffersManager::DoEvictData(const TimeUnit& aPlaybackTime,
                                 uint32_t aSizeToEvict)
{
  MOZ_ASSERT(OnTaskQueue());

  
  const auto& track = HasVideo() ? mVideoTracks : mAudioTracks;
  const auto& buffer = track.mBuffers.LastElement();
  
  
  TimeUnit lowerLimit = std::min(track.mNextSampleTime, aPlaybackTime);
  uint32_t lastKeyFrameIndex = 0;
  int64_t toEvict = aSizeToEvict;
  uint32_t partialEvict = 0;
  for (uint32_t i = 0; i < buffer.Length(); i++) {
    const auto& frame = buffer[i];
    if (frame->mKeyframe) {
      lastKeyFrameIndex = i;
      toEvict -= partialEvict;
      if (toEvict < 0) {
        break;
      }
      partialEvict = 0;
    }
    if (frame->mTime >= lowerLimit.ToMicroseconds()) {
      break;
    }
    partialEvict += sizeof(*frame) + frame->mSize;
  }

  int64_t finalSize = mSizeSourceBuffer - aSizeToEvict;

  if (lastKeyFrameIndex > 0) {
    MSE_DEBUG("Step1. Evicting %u bytes prior currentTime",
              aSizeToEvict - toEvict);
    CodedFrameRemoval(
      TimeInterval(TimeUnit::FromMicroseconds(0),
                   TimeUnit::FromMicroseconds(buffer[lastKeyFrameIndex]->mTime - 1)));
  }

  if (mSizeSourceBuffer <= finalSize) {
    return;
  }

  toEvict = mSizeSourceBuffer - finalSize;

  
  
  
  TimeUnit upperLimit =
    std::max(aPlaybackTime, track.mNextSampleTime) + TimeUnit::FromSeconds(30);
  lastKeyFrameIndex = buffer.Length();
  for (int32_t i = buffer.Length() - 1; i >= 0; i--) {
    const auto& frame = buffer[i];
    if (frame->mKeyframe) {
      lastKeyFrameIndex = i;
      toEvict -= partialEvict;
      if (toEvict < 0) {
        break;
      }
      partialEvict = 0;
    }
    if (frame->mTime <= upperLimit.ToMicroseconds()) {
      break;
    }
    partialEvict += sizeof(*frame) + frame->mSize;
  }
  if (lastKeyFrameIndex < buffer.Length()) {
    MSE_DEBUG("Step2. Evicting %u bytes from trailing data",
              mSizeSourceBuffer - finalSize);
    CodedFrameRemoval(
      TimeInterval(TimeUnit::FromMicroseconds(buffer[lastKeyFrameIndex]->GetEndTime() + 1),
                   TimeUnit::FromInfinity()));
  }
}

nsRefPtr<TrackBuffersManager::RangeRemovalPromise>
TrackBuffersManager::CodedFrameRemovalWithPromise(TimeInterval aInterval)
{
  MOZ_ASSERT(OnTaskQueue());
  bool rv = CodedFrameRemoval(aInterval);
  return RangeRemovalPromise::CreateAndResolve(rv, __func__);
}

bool
TrackBuffersManager::CodedFrameRemoval(TimeInterval aInterval)
{
  MOZ_ASSERT(OnTaskQueue());
  MOZ_ASSERT(!mAppendRunning, "Logic error: Append in progress");
  MSE_DEBUG("From %.2fs to %.2f",
            aInterval.mStart.ToSeconds(), aInterval.mEnd.ToSeconds());

  if (mMediaSourceDuration.Ref().isNothing() ||
      IsNaN(mMediaSourceDuration.Ref().ref())) {
    MSE_DEBUG("Nothing to remove, aborting");
    return false;
  }
  TimeUnit duration{TimeUnit::FromSeconds(mMediaSourceDuration.Ref().ref())};

#if DEBUG
  MSE_DEBUG("duration:%.2f", duration.ToSeconds());
  if (HasVideo()) {
    MSE_DEBUG("before video ranges=%s",
              DumpTimeRanges(mVideoTracks.mBufferedRanges).get());
  }
  if (HasAudio()) {
    MSE_DEBUG("before audio ranges=%s",
              DumpTimeRanges(mAudioTracks.mBufferedRanges).get());
  }
#endif

  
  TimeUnit start = aInterval.mStart;
  
  TimeUnit end = aInterval.mEnd;

  bool dataRemoved = false;

  
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

    
    
    
    
    TimeIntervals removedInterval{TimeInterval(start, removeEndTimestamp)};
    RemoveFrames(removedInterval, *track, 0);

    
    
    
    
    
    
  }

  UpdateBufferedRanges();

  
  mSizeSourceBuffer = mVideoTracks.mSizeBuffer + mAudioTracks.mSizeBuffer;

  
  if (mBufferFull && mSizeSourceBuffer < mEvictionThreshold) {
    mBufferFull = false;
  }
  mEvictionOccurred = true;

  
  mMediaSourceDemuxer->NotifyTimeRangesChanged();

  return dataRemoved;
}

void
TrackBuffersManager::UpdateBufferedRanges()
{
  MonitorAutoLock mon(mMonitor);

  mVideoBufferedRanges = mVideoTracks.mBufferedRanges;
  mAudioBufferedRanges = mAudioTracks.mBufferedRanges;

#if DEBUG
  if (HasVideo()) {
    MSE_DEBUG("after video ranges=%s",
              DumpTimeRanges(mVideoTracks.mBufferedRanges).get());
  }
  if (HasAudio()) {
    MSE_DEBUG("after audio ranges=%s",
              DumpTimeRanges(mAudioTracks.mBufferedRanges).get());
  }
#endif

  mOfficialGroupEndTimestamp = mGroupEndTimestamp;
}

nsRefPtr<TrackBuffersManager::AppendPromise>
TrackBuffersManager::InitSegmentParserLoop()
{
  MOZ_ASSERT(OnTaskQueue());

  MOZ_ASSERT(mAppendPromise.IsEmpty() && !mAppendRunning);
  nsRefPtr<AppendPromise> p = mAppendPromise.Ensure(__func__);

  AppendIncomingBuffers();
  SegmentParserLoop();

  return p;
}

void
TrackBuffersManager::AppendIncomingBuffers()
{
  MOZ_ASSERT(OnTaskQueue());
  MonitorAutoLock mon(mMonitor);
  for (auto& incomingBuffer : mIncomingBuffers) {
    if (!mInputBuffer) {
      mInputBuffer = incomingBuffer.first();
    } else if (!mInputBuffer->AppendElements(*incomingBuffer.first(), fallible)) {
      RejectAppend(NS_ERROR_OUT_OF_MEMORY, __func__);
    }
    mTimestampOffset = incomingBuffer.second();
    mLastTimestampOffset = mTimestampOffset;
  }
  mIncomingBuffers.Clear();

  mAppendWindow =
    TimeInterval(TimeUnit::FromSeconds(mParent->AppendWindowStart()),
                 TimeUnit::FromSeconds(mParent->AppendWindowEnd()));
}

void
TrackBuffersManager::SegmentParserLoop()
{
  MOZ_ASSERT(OnTaskQueue());
  while (true) {
    
    if (!mInputBuffer || mInputBuffer->IsEmpty()) {
      NeedMoreData();
      return;
    }
    
    
    
    

    
    
    
    
    

    
    
    if (mAppendState == AppendState::WAITING_FOR_SEGMENT) {
      if (mParser->IsInitSegmentPresent(mInputBuffer)) {
        SetAppendState(AppendState::PARSING_INIT_SEGMENT);
        if (mFirstInitializationSegmentReceived) {
          
          mInitData = nullptr;
          RecreateParser();
        }
        continue;
      }
      if (mParser->IsMediaSegmentPresent(mInputBuffer)) {
        SetAppendState(AppendState::PARSING_MEDIA_SEGMENT);
        continue;
      }
      
      
      MSE_DEBUG("Found invalid data, ignoring.");
      mInputBuffer = nullptr;
      NeedMoreData();
      return;
    }

    int64_t start, end;
    mParser->ParseStartAndEndTimestamps(mInputBuffer, start, end);
    mProcessedInput += mInputBuffer->Length();

    
    
    if (mAppendState == AppendState::PARSING_INIT_SEGMENT) {
      if (mParser->InitSegmentRange().IsNull()) {
        mInputBuffer = nullptr;
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
      
      if (mParser->MediaHeaderRange().IsNull()) {
        mCurrentInputBuffer->AppendData(mInputBuffer);
        mInputBuffer = nullptr;
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
  if (!mAbort) {
    RestoreCachedVariables();
  }
  mAppendRunning = false;
  mAppendPromise.ResolveIfExists(mActiveTrack, __func__);
}

void
TrackBuffersManager::RejectAppend(nsresult aRejectValue, const char* aName)
{
  MSE_DEBUG("rv=%d", aRejectValue);
  mAppendRunning = false;
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
    NS_WARNING("Waiting on WebMDemuxer");
  
    return;
  }

#ifdef MOZ_FMP4
  if (mType.LowerCaseEqualsLiteral("video/mp4") || mType.LowerCaseEqualsLiteral("audio/mp4")) {
    mInputDemuxer = new MP4Demuxer(mCurrentInputBuffer);
    return;
  }
#endif
  NS_WARNING("Not supported (yet)");
  return;
}

void
TrackBuffersManager::InitializationSegmentReceived()
{
  MOZ_ASSERT(mParser->HasCompleteInitData());
  mInitData = mParser->InitData();
  mCurrentInputBuffer = new SourceBufferResource(mType);
  mCurrentInputBuffer->AppendData(mInitData);
  uint32_t length =
    mParser->InitSegmentRange().mEnd - (mProcessedInput - mInputBuffer->Length());
  if (mInputBuffer->Length() == length) {
    mInputBuffer = nullptr;
  } else {
    mInputBuffer->RemoveElementsAt(0, length);
  }
  CreateDemuxerforMIMEType();
  if (!mInputDemuxer) {
    NS_WARNING("TODO type not supported");
    RejectAppend(NS_ERROR_DOM_NOT_SUPPORTED_ERR, __func__);
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
  MOZ_ASSERT(OnTaskQueue());
  MSE_DEBUG("mAbort:%d", static_cast<bool>(mAbort));
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

    mVideoTracks.mLongestFrameDuration = mVideoTracks.mLastFrameDuration;
    mAudioTracks.mLongestFrameDuration = mAudioTracks.mLastFrameDuration;
  }

  
  mActiveTrack = false;

  
  uint32_t streamID = sStreamSourceID++;

  
  if (!mFirstInitializationSegmentReceived) {
    mAudioTracks.mNumTracks = numAudios;
    
    
    
    

    
    
    if (numAudios) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      mActiveTrack = true;
      
      
      
      
      mAudioTracks.mBuffers.AppendElement(TrackBuffer());
      
      mAudioTracks.mInfo = new SharedTrackInfo(info.mAudio, streamID);
      mAudioTracks.mLastInfo = mAudioTracks.mInfo;
    }

    mVideoTracks.mNumTracks = numVideos;
    
    
    if (numVideos) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      mActiveTrack = true;
      
      
      
      
      mVideoTracks.mBuffers.AppendElement(TrackBuffer());
      
      mVideoTracks.mInfo = new SharedTrackInfo(info.mVideo, streamID);
      mVideoTracks.mLastInfo = mVideoTracks.mInfo;
    }
    
    
    

    
    mFirstInitializationSegmentReceived = true;
  } else {
    mAudioTracks.mLastInfo = new SharedTrackInfo(info.mAudio, streamID);
    mVideoTracks.mLastInfo = new SharedTrackInfo(info.mVideo, streamID);
  }

  UniquePtr<EncryptionInfo> crypto = mInputDemuxer->GetCrypto();
  if (crypto && crypto->IsEncrypted()) {
#ifdef MOZ_EME
    
    for (uint32_t i = 0; i < crypto->mInitDatas.Length(); i++) {
      NS_DispatchToMainThread(
        new DispatchKeyNeededEvent(mParentDecoder, crypto->mInitDatas[i].mInitData, NS_LITERAL_STRING("cenc")));
    }
#endif 
    info.mCrypto = *crypto;
    
    
    info.mCrypto.mInitDatas.Clear();
    mEncrypted = true;
  }

  {
    MonitorAutoLock mon(mMonitor);
    mInfo = info;
  }

  
  
  
  mCurrentInputBuffer->EvictAll();
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
  MOZ_ASSERT(OnTaskQueue());
  MOZ_ASSERT(mProcessingPromise.IsEmpty());
  nsRefPtr<CodedFrameProcessingPromise> p = mProcessingPromise.Ensure(__func__);

  int64_t offset = mCurrentInputBuffer->GetLength();
  MediaByteRange mediaRange = mParser->MediaSegmentRange();
  uint32_t length;
  if (mediaRange.IsNull()) {
    length = mInputBuffer->Length();
    mCurrentInputBuffer->AppendData(mInputBuffer);
    mInputBuffer = nullptr;
  } else {
    
    length = mediaRange.mEnd - (mProcessedInput - mInputBuffer->Length());
    nsRefPtr<MediaByteBuffer> segment = new MediaByteBuffer;
    MOZ_ASSERT(mInputBuffer->Length() >= length);
    if (!segment->AppendElements(mInputBuffer->Elements(), length, fallible)) {
      return CodedFrameProcessingPromise::CreateAndReject(NS_ERROR_OUT_OF_MEMORY, __func__);
    }
    mCurrentInputBuffer->AppendData(segment);
    mInputBuffer->RemoveElementsAt(0, length);
  }
  mInputDemuxer->NotifyDataArrived(length, offset);

  DoDemuxVideo();

  return p;
}

void
TrackBuffersManager::OnDemuxFailed(TrackType aTrack,
                                   DemuxerFailureReason aFailure)
{
  MOZ_ASSERT(OnTaskQueue());
  MSE_DEBUG("Failed to demux %s, failure:%d mAbort:%d",
            aTrack == TrackType::kVideoTrack ? "video" : "audio",
            aFailure, static_cast<bool>(mAbort));
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
      RejectProcessing(NS_ERROR_FAILURE, __func__);
      break;
    case DemuxerFailureReason::CANCELED:
    case DemuxerFailureReason::SHUTDOWN:
      RejectProcessing(NS_ERROR_ABORT, __func__);
      break;
    default:
      MOZ_ASSERT(false);
      break;
  }
}

void
TrackBuffersManager::DoDemuxVideo()
{
  MOZ_ASSERT(OnTaskQueue());
  MSE_DEBUG("mAbort:%d", static_cast<bool>(mAbort));
  if (!HasVideo()) {
    DoDemuxAudio();
    return;
  }
  if (mAbort) {
    RejectProcessing(NS_ERROR_ABORT, __func__);
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
  MOZ_ASSERT(OnTaskQueue());
  MSE_DEBUG("%d video samples demuxed", aSamples->mSamples.Length());
  mVideoTracks.mDemuxRequest.Complete();
  mVideoTracks.mQueuedSamples.AppendElements(aSamples->mSamples);
  DoDemuxAudio();
}

void
TrackBuffersManager::DoDemuxAudio()
{
  MOZ_ASSERT(OnTaskQueue());
  MSE_DEBUG("mAbort:%d", static_cast<bool>(mAbort));
  if (!HasAudio()) {
    CompleteCodedFrameProcessing();
    return;
  }
  if (mAbort) {
    RejectProcessing(NS_ERROR_ABORT, __func__);
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
  MOZ_ASSERT(OnTaskQueue());
  MSE_DEBUG("%d audio samples demuxed", aSamples->mSamples.Length());
  mAudioTracks.mDemuxRequest.Complete();
  mAudioTracks.mQueuedSamples.AppendElements(aSamples->mSamples);
  CompleteCodedFrameProcessing();
}

void
TrackBuffersManager::CompleteCodedFrameProcessing()
{
  MOZ_ASSERT(OnTaskQueue());
  MSE_DEBUG("mAbort:%d", static_cast<bool>(mAbort));

  
  
  ProcessFrames(mVideoTracks.mQueuedSamples, mVideoTracks);
  mVideoTracks.mQueuedSamples.Clear();

#if defined(DEBUG)
  if (HasVideo()) {
    const auto& track = mVideoTracks.mBuffers.LastElement();
    MOZ_ASSERT(track.IsEmpty() || track[0]->mKeyframe);
    for (uint32_t i = 1; i < track.Length(); i++) {
      MOZ_ASSERT((track[i-1]->mTrackInfo->GetID() == track[i]->mTrackInfo->GetID() && track[i-1]->mTimecode < track[i]->mTimecode) ||
                 track[i]->mKeyframe);
    }
  }
#endif

  ProcessFrames(mAudioTracks.mQueuedSamples, mAudioTracks);
  mAudioTracks.mQueuedSamples.Clear();

#if defined(DEBUG)
  if (HasAudio()) {
    const auto& track = mAudioTracks.mBuffers.LastElement();
    MOZ_ASSERT(track.IsEmpty() || track[0]->mKeyframe);
    for (uint32_t i = 1; i < track.Length(); i++) {
      MOZ_ASSERT((track[i-1]->mTrackInfo->GetID() == track[i]->mTrackInfo->GetID() && track[i-1]->mTimecode < track[i]->mTimecode) ||
                 track[i]->mKeyframe);
    }
  }
#endif

  UpdateBufferedRanges();

  
  mSizeSourceBuffer = mVideoTracks.mSizeBuffer + mAudioTracks.mSizeBuffer;

  
  
  if (mSizeSourceBuffer >= mEvictionThreshold) {
    mBufferFull = true;
    mEvictionOccurred = false;
  }

  
  if (mParser->MediaSegmentRange().IsNull()) {
    ResolveProcessing(true, __func__);
    return;
  }

  
  
  
  
  mCurrentInputBuffer->EvictAll();
  mInputDemuxer->NotifyDataRemoved();
  RecreateParser();

  
  SetAppendState(AppendState::WAITING_FOR_SEGMENT);

  
  mMediaSourceDemuxer->NotifyTimeRangesChanged();

  
  ResolveProcessing(false, __func__);
}

void
TrackBuffersManager::RejectProcessing(nsresult aRejectValue, const char* aName)
{
  if (mAbort) {
    
    
    mAppendRunning = false;
  }
  mProcessingPromise.RejectIfExists(aRejectValue, __func__);
}

void
TrackBuffersManager::ResolveProcessing(bool aResolveValue, const char* aName)
{
  if (mAbort) {
    
    
    mAppendRunning = false;
  }
  mProcessingPromise.ResolveIfExists(aResolveValue, __func__);
}

void
TrackBuffersManager::CheckSequenceDiscontinuity()
{
  if (mParent->mAppendMode == SourceBufferAppendMode::Sequence &&
      mGroupStartTimestamp.isSome()) {
    mTimestampOffset = mGroupStartTimestamp.ref();
    mGroupEndTimestamp = mGroupStartTimestamp.ref();
    mVideoTracks.mNeedRandomAccessPoint = true;
    mAudioTracks.mNeedRandomAccessPoint = true;
    mGroupStartTimestamp.reset();
  }
}

void
TrackBuffersManager::ProcessFrames(TrackBuffer& aSamples, TrackData& aTrackData)
{
  if (!aSamples.Length()) {
    return;
  }

  
  CheckSequenceDiscontinuity();

  
  auto& trackBuffer = aTrackData;

  
  
  TimeInterval targetWindow{
    TimeInterval(mAppendWindow.mStart, mAppendWindow.mEnd,
                 trackBuffer.mLongestFrameDuration.refOr(TimeUnit::FromMicroseconds(aSamples[0]->mDuration)))};

  TimeIntervals samplesRange;
  uint32_t sizeNewSamples = 0;
  TrackBuffer samples; 
                       

  
  
  
  bool needDiscontinuityCheck = true;

  for (auto& sample : aSamples) {
    SAMPLE_DEBUG("Processing %s frame(pts:%lld end:%lld, dts:%lld, duration:%lld, "
               "kf:%d)",
               aTrackData.mInfo->mMimeType.get(),
               sample->mTime,
               sample->GetEndTime(),
               sample->mTimecode,
               sample->mDuration,
               sample->mKeyframe);

    
    

    
    if (trackBuffer.mNeedRandomAccessPoint) {
      
      if (!sample->mKeyframe) {
        continue;
      }
      
      trackBuffer.mNeedRandomAccessPoint = false;
    }

    
    
    
    
    
    
    

    
    
    

    TimeInterval sampleInterval =
      mParent->mGenerateTimestamps
        ? TimeInterval(mTimestampOffset,
                       mTimestampOffset + TimeUnit::FromMicroseconds(sample->mDuration))
        : TimeInterval(TimeUnit::FromMicroseconds(sample->mTime) + mTimestampOffset,
                       TimeUnit::FromMicroseconds(sample->GetEndTime()) + mTimestampOffset);
    TimeUnit decodeTimestamp =
      mParent->mGenerateTimestamps
        ? mTimestampOffset
        : TimeUnit::FromMicroseconds(sample->mTimecode) + mTimestampOffset;

    
    
    

    if (needDiscontinuityCheck && trackBuffer.mLastDecodeTimestamp.isSome() &&
        (decodeTimestamp < trackBuffer.mLastDecodeTimestamp.ref() ||
         decodeTimestamp - trackBuffer.mLastDecodeTimestamp.ref() > 2*trackBuffer.mLongestFrameDuration.ref())) {
      MSE_DEBUG("Discontinuity detected.");
      
      if (mParent->mAppendMode == SourceBufferAppendMode::Segments) {
        
        mGroupEndTimestamp = sampleInterval.mStart;
      }
      
      if (mParent->mAppendMode == SourceBufferAppendMode::Sequence) {
        
        mGroupStartTimestamp = Some(mGroupEndTimestamp);
      }
      for (auto& track : GetTracksList()) {
        
        
        
        
        track->ResetAppendState();
      }
      
      
      
      
      CheckSequenceDiscontinuity();

      if (!sample->mKeyframe) {
        continue;
      }
      if (mParent->mAppendMode == SourceBufferAppendMode::Sequence) {
        
        
        sampleInterval =
          mParent->mGenerateTimestamps
            ? TimeInterval(mTimestampOffset,
                           mTimestampOffset + TimeUnit::FromMicroseconds(sample->mDuration))
            : TimeInterval(TimeUnit::FromMicroseconds(sample->mTime) + mTimestampOffset,
                           TimeUnit::FromMicroseconds(sample->GetEndTime()) + mTimestampOffset);
        decodeTimestamp =
          mParent->mGenerateTimestamps
            ? mTimestampOffset
            : TimeUnit::FromMicroseconds(sample->mTimecode) + mTimestampOffset;
      }
      trackBuffer.mNeedRandomAccessPoint = false;
      needDiscontinuityCheck = false;
    }

    
    

    
    
    if (!targetWindow.Contains(sampleInterval)) {
      if (samples.Length()) {
        
        
        InsertFrames(samples, samplesRange, trackBuffer);
        samples.Clear();
        samplesRange = TimeIntervals();
        trackBuffer.mSizeBuffer += sizeNewSamples;
        sizeNewSamples = 0;
      }
      trackBuffer.mNeedRandomAccessPoint = true;
      needDiscontinuityCheck = true;
      continue;
    }

    samplesRange += sampleInterval;
    sizeNewSamples += sizeof(*sample) + sample->mSize;
    sample->mTime = sampleInterval.mStart.ToMicroseconds();
    sample->mTimecode = decodeTimestamp.ToMicroseconds();
    sample->mTrackInfo = trackBuffer.mLastInfo;
    samples.AppendElement(sample);

    

    
    trackBuffer.mLastDecodeTimestamp =
      Some(TimeUnit::FromMicroseconds(sample->mTimecode));
    
    trackBuffer.mLastFrameDuration =
      Some(TimeUnit::FromMicroseconds(sample->mDuration));

    trackBuffer.mLongestFrameDuration =
      Some(trackBuffer.mLongestFrameDuration.isNothing()
           ? trackBuffer.mLastFrameDuration.ref()
           : std::max(trackBuffer.mLastFrameDuration.ref(),
                      trackBuffer.mLongestFrameDuration.ref()));

    
    if (trackBuffer.mHighestEndTimestamp.isNothing() ||
        sampleInterval.mEnd > trackBuffer.mHighestEndTimestamp.ref()) {
      trackBuffer.mHighestEndTimestamp = Some(sampleInterval.mEnd);
    }
    
    if (sampleInterval.mEnd > mGroupEndTimestamp) {
      mGroupEndTimestamp = sampleInterval.mEnd;
    }
    
    if (mParent->mGenerateTimestamps) {
      mTimestampOffset = sampleInterval.mEnd;
    }
  }

  if (samples.Length()) {
    InsertFrames(samples, samplesRange, trackBuffer);
    trackBuffer.mSizeBuffer += sizeNewSamples;
  }
}

void
TrackBuffersManager::CheckNextInsertionIndex(TrackData& aTrackData,
                                             const TimeUnit& aSampleTime)
{
  if (aTrackData.mNextInsertionIndex.isSome()) {
    return;
  }

  TrackBuffer& data = aTrackData.mBuffers.LastElement();

  if (data.IsEmpty() || aSampleTime < aTrackData.mBufferedRanges.GetStart()) {
    aTrackData.mNextInsertionIndex = Some(size_t(0));
    return;
  }

  
  TimeInterval target;
  for (const auto& interval : aTrackData.mBufferedRanges) {
    if (aSampleTime < interval.mStart) {
      target = interval;
      break;
    }
  }
  if (target.IsEmpty()) {
    
    aTrackData.mNextInsertionIndex = Some(data.Length());
    return;
  }
  for (uint32_t i = 0; i < data.Length(); i++) {
    const nsRefPtr<MediaRawData>& sample = data[i];
    TimeInterval sampleInterval{
      TimeUnit::FromMicroseconds(sample->mTime),
      TimeUnit::FromMicroseconds(sample->GetEndTime())};
    if (target.Intersects(sampleInterval)) {
      aTrackData.mNextInsertionIndex = Some(size_t(i));
      return;
    }
  }
  MOZ_CRASH("Insertion Index Not Found");
}

void
TrackBuffersManager::InsertFrames(TrackBuffer& aSamples,
                                  const TimeIntervals& aIntervals,
                                  TrackData& aTrackData)
{
  
  auto& trackBuffer = aTrackData;

  MSE_DEBUGV("Processing %d %s frames(start:%lld end:%lld)",
             aSamples.Length(),
             aTrackData.mInfo->mMimeType.get(),
             aIntervals.GetStart().ToMicroseconds(),
             aIntervals.GetEnd().ToMicroseconds());

  
  
  

  
  
  

  
  
  
  
  

  
  
  

  
  

  TimeIntervals intersection = trackBuffer.mBufferedRanges;
  intersection.Intersection(aIntervals);

  if (intersection.Length()) {
    RemoveFrames(aIntervals, trackBuffer, trackBuffer.mNextInsertionIndex.refOr(0));
  }

  
  CheckNextInsertionIndex(aTrackData,
                          TimeUnit::FromMicroseconds(aSamples[0]->mTime));

  TrackBuffer& data = trackBuffer.mBuffers.LastElement();
  data.InsertElementsAt(trackBuffer.mNextInsertionIndex.ref(), aSamples);
  trackBuffer.mNextInsertionIndex.ref() += aSamples.Length();

  
  
  
  
  TimeIntervals range(aIntervals);
  range.SetFuzz(trackBuffer.mLongestFrameDuration.ref() / 2);
  trackBuffer.mBufferedRanges += range;
}

void
TrackBuffersManager::RemoveFrames(const TimeIntervals& aIntervals,
                                  TrackData& aTrackData,
                                  uint32_t aStartIndex)
{
  TrackBuffer& data = aTrackData.mBuffers.LastElement();
  Maybe<uint32_t> firstRemovedIndex;
  uint32_t lastRemovedIndex;

  
  
  
  
  
  
  
  
  
  for (uint32_t i = aStartIndex; i < data.Length(); i++) {
    MediaRawData* sample = data[i].get();
    TimeInterval sampleInterval =
      TimeInterval(TimeUnit::FromMicroseconds(sample->mTime),
                   TimeUnit::FromMicroseconds(sample->GetEndTime()));
    if (aIntervals.Contains(sampleInterval)) {
      if (firstRemovedIndex.isNothing()) {
        firstRemovedIndex = Some(i);
      }
      lastRemovedIndex = i;
    }
  }

  if (firstRemovedIndex.isNothing()) {
    return;
  }

  
  
  for (uint32_t i = lastRemovedIndex + 1; i < data.Length(); i++) {
    MediaRawData* sample = data[i].get();
    if (sample->mKeyframe) {
      break;
    }
    lastRemovedIndex = i;
  }

  TimeIntervals removedIntervals;
  for (uint32_t i = firstRemovedIndex.ref(); i <= lastRemovedIndex; i++) {
    MediaRawData* sample = data[i].get();
    TimeInterval sampleInterval =
      TimeInterval(TimeUnit::FromMicroseconds(sample->mTime),
                   TimeUnit::FromMicroseconds(sample->GetEndTime()));
    removedIntervals += sampleInterval;
    aTrackData.mSizeBuffer -= sizeof(*sample) + sample->mSize;
  }

  MSE_DEBUG("Removing frames from:%u (frames:%u) ([%f, %f))",
            firstRemovedIndex.ref(),
            lastRemovedIndex - firstRemovedIndex.ref() + 1,
            removedIntervals.GetStart().ToSeconds(),
            removedIntervals.GetEnd().ToSeconds());

  if (aTrackData.mNextGetSampleIndex.isSome()) {
    if (aTrackData.mNextGetSampleIndex.ref() >= firstRemovedIndex.ref() &&
        aTrackData.mNextGetSampleIndex.ref() <= lastRemovedIndex) {
      MSE_DEBUG("Next sample to be played got evicted");
      aTrackData.mNextGetSampleIndex.reset();
    } else if (aTrackData.mNextGetSampleIndex.ref() > lastRemovedIndex) {
      aTrackData.mNextGetSampleIndex.ref() -=
        lastRemovedIndex - firstRemovedIndex.ref() + 1;
    }
  }

  if (aTrackData.mNextInsertionIndex.isSome()) {
    if (aTrackData.mNextInsertionIndex.ref() > firstRemovedIndex.ref() &&
        aTrackData.mNextInsertionIndex.ref() <= lastRemovedIndex + 1) {
      aTrackData.ResetAppendState();
    } else if (aTrackData.mNextInsertionIndex.ref() > lastRemovedIndex + 1) {
      aTrackData.mNextInsertionIndex.ref() -=
        lastRemovedIndex - firstRemovedIndex.ref() + 1;
    }
  }

  
  aTrackData.mBufferedRanges -= removedIntervals;

  data.RemoveElementsAt(firstRemovedIndex.ref(),
                        lastRemovedIndex - firstRemovedIndex.ref() + 1);
}

void
TrackBuffersManager::RecreateParser()
{
  MOZ_ASSERT(OnTaskQueue());
  
  
  
  
  mParser = ContainerParser::CreateForMIMEType(mType);
  if (mInitData) {
    int64_t start, end;
    mParser->ParseStartAndEndTimestamps(mInitData, start, end);
    mProcessedInput = mInitData->Length();
  } else {
    mProcessedInput = 0;
  }
}

nsTArray<TrackBuffersManager::TrackData*>
TrackBuffersManager::GetTracksList()
{
  MOZ_ASSERT(OnTaskQueue());
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
  MOZ_ASSERT(OnTaskQueue());
  if (mTimestampOffset != mLastTimestampOffset) {
    nsRefPtr<TrackBuffersManager> self = this;
    nsCOMPtr<nsIRunnable> task =
      NS_NewRunnableFunction([self] {
        self->mParent->SetTimestampOffset(self->mTimestampOffset);
      });
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

TimeUnit
TrackBuffersManager::GroupEndTimestamp()
{
  MonitorAutoLock mon(mMonitor);
  return mOfficialGroupEndTimestamp;
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

TimeUnit
TrackBuffersManager::Seek(TrackInfo::TrackType aTrack,
                          const TimeUnit& aTime)
{
  MOZ_ASSERT(OnTaskQueue());
  auto& trackBuffer = GetTracksData(aTrack);
  const TrackBuffersManager::TrackBuffer& track = GetTrackBuffer(aTrack);
  TimeUnit lastKeyFrameTime;
  TimeUnit lastKeyFrameTimecode;
  uint32_t lastKeyFrameIndex = 0;
  for (uint32_t i = 0; i < track.Length(); i++) {
    const nsRefPtr<MediaRawData>& sample = track[i];
    TimeUnit sampleTime = TimeUnit::FromMicroseconds(sample->mTime);
    if (sampleTime > aTime) {
      break;
    }
    if (sample->mKeyframe) {
      lastKeyFrameTimecode = TimeUnit::FromMicroseconds(sample->mTimecode);
      lastKeyFrameTime = sampleTime;
      lastKeyFrameIndex = i;
    }
    if (sampleTime == aTime) {
      break;
    }
  }
  trackBuffer.mNextGetSampleIndex = Some(lastKeyFrameIndex);
  trackBuffer.mNextSampleTimecode = lastKeyFrameTimecode;
  trackBuffer.mNextSampleTime = lastKeyFrameTime;

  return lastKeyFrameTime;
}

uint32_t
TrackBuffersManager::SkipToNextRandomAccessPoint(TrackInfo::TrackType aTrack,
                                                 const TimeUnit& aTimeThreadshold,
                                                 bool& aFound)
{
  MOZ_ASSERT(OnTaskQueue());
  uint32_t parsed = 0;
  auto& trackData = GetTracksData(aTrack);
  const TrackBuffer& track = GetTrackBuffer(aTrack);

  uint32_t nextSampleIndex = trackData.mNextGetSampleIndex.valueOr(0);
  for (uint32_t i = nextSampleIndex; i < track.Length(); i++) {
    const nsRefPtr<MediaRawData>& sample = track[i];
    if (sample->mKeyframe &&
        sample->mTime >= aTimeThreadshold.ToMicroseconds()) {
      trackData.mNextSampleTimecode =
        TimeUnit::FromMicroseconds(sample->mTimecode);
      trackData.mNextSampleTime =
        TimeUnit::FromMicroseconds(sample->mTime);
      trackData.mNextGetSampleIndex = Some(i);
      aFound = true;
      break;
    }
    parsed++;
  }

  return parsed;
}

already_AddRefed<MediaRawData>
TrackBuffersManager::GetSample(TrackInfo::TrackType aTrack,
                               const TimeUnit& aFuzz,
                               bool& aError)
{
  MOZ_ASSERT(OnTaskQueue());
  auto& trackData = GetTracksData(aTrack);
  const TrackBuffer& track = GetTrackBuffer(aTrack);

  aError = false;

  if (!track.Length() ||
      (trackData.mNextGetSampleIndex.isSome() &&
       trackData.mNextGetSampleIndex.ref() >= track.Length())) {
    return nullptr;
  }
  if (trackData.mNextGetSampleIndex.isNothing() &&
      trackData.mNextSampleTimecode == TimeUnit()) {
    
    trackData.mNextGetSampleIndex = Some(0u);
  }

  if (trackData.mNextGetSampleIndex.isSome()) {
    const nsRefPtr<MediaRawData>& sample =
      track[trackData.mNextGetSampleIndex.ref()];
    if (trackData.mNextGetSampleIndex.ref() &&
        sample->mTimecode > (trackData.mNextSampleTimecode + aFuzz).ToMicroseconds()) {
      
      return nullptr;
    }

    nsRefPtr<MediaRawData> p = sample->Clone();
    if (!p) {
      aError = true;
      return nullptr;
    }
    trackData.mNextGetSampleIndex.ref()++;
    
    trackData.mNextSampleTimecode =
      TimeUnit::FromMicroseconds(sample->mTimecode + sample->mDuration);
    trackData.mNextSampleTime =
      TimeUnit::FromMicroseconds(sample->GetEndTime());
    return p.forget();
  }

  
  for (uint32_t i = 0; i < track.Length(); i++) {
    const nsRefPtr<MediaRawData>& sample = track[i];
    TimeInterval sampleInterval{
      TimeUnit::FromMicroseconds(sample->mTimecode),
      TimeUnit::FromMicroseconds(sample->mTimecode + sample->mDuration),
      aFuzz};

    if (sampleInterval.ContainsWithStrictEnd(trackData.mNextSampleTimecode)) {
      nsRefPtr<MediaRawData> p = sample->Clone();
      if (!p) {
        
        aError = true;
        return nullptr;
      }
      trackData.mNextGetSampleIndex = Some(i+1);
      trackData.mNextSampleTimecode = sampleInterval.mEnd;
      trackData.mNextSampleTime =
        TimeUnit::FromMicroseconds(sample->GetEndTime());
      return p.forget();
    }
  }

  
  
    for (uint32_t i = 0; i < track.Length(); i++) {
    const nsRefPtr<MediaRawData>& sample = track[i];
    TimeInterval sampleInterval{
      TimeUnit::FromMicroseconds(sample->mTime),
      TimeUnit::FromMicroseconds(sample->GetEndTime()),
      aFuzz};

    if (sampleInterval.ContainsWithStrictEnd(trackData.mNextSampleTimecode)) {
      nsRefPtr<MediaRawData> p = sample->Clone();
      if (!p) {
        
        aError = true;
        return nullptr;
      }
      trackData.mNextGetSampleIndex = Some(i+1);
      
      trackData.mNextSampleTimecode = sampleInterval.mEnd;
      trackData.mNextSampleTime =
        TimeUnit::FromMicroseconds(sample->GetEndTime());
      return p.forget();
    }
  }

  MSE_DEBUG("Couldn't find sample (pts:%lld dts:%lld)",
             trackData.mNextSampleTime.ToMicroseconds(),
            trackData.mNextSampleTimecode.ToMicroseconds());
  return nullptr;
}

TimeUnit
TrackBuffersManager::GetNextRandomAccessPoint(TrackInfo::TrackType aTrack)
{
  auto& trackData = GetTracksData(aTrack);
  MOZ_ASSERT(trackData.mNextGetSampleIndex.isSome());
  const TrackBuffersManager::TrackBuffer& track = GetTrackBuffer(aTrack);

  uint32_t i = trackData.mNextGetSampleIndex.ref();
  for (; i < track.Length(); i++) {
    const nsRefPtr<MediaRawData>& sample = track[i];
    if (sample->mKeyframe) {
      return TimeUnit::FromMicroseconds(sample->mTime);
    }
  }
  return media::TimeUnit::FromInfinity();
}

}
#undef MSE_DEBUG
#undef MSE_DEBUGV
#undef SAMPLE_DEBUG
