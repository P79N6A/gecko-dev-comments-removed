





#ifndef MOZILLA_TRACKBUFFERSMANAGER_H_
#define MOZILLA_TRACKBUFFERSMANAGER_H_

#include "SourceBufferContentManager.h"
#include "MediaSourceDecoder.h"
#include "mozilla/Atomics.h"
#include "mozilla/Maybe.h"
#include "mozilla/Monitor.h"
#include "mozilla/Pair.h"
#include "nsProxyRelease.h"
#include "nsTArray.h"

namespace mozilla {

class ContainerParser;
class MediaLargeByteBuffer;
class MediaRawData;
class SourceBuffer;
class SourceBufferResource;

using media::TimeUnit;
using media::TimeInterval;
using media::TimeIntervals;
using dom::SourceBufferAppendMode;

class TrackBuffersManager : public SourceBufferContentManager {
public:
  typedef MediaPromise<bool, nsresult,  true> CodedFrameProcessingPromise;
  typedef TrackInfo::TrackType TrackType;
  typedef MediaData::Type MediaType;
  typedef nsTArray<nsRefPtr<MediaRawData>> TrackBuffer;

  TrackBuffersManager(dom::SourceBuffer* aParent, MediaSourceDecoder* aParentDecoder, const nsACString& aType);

  bool AppendData(MediaLargeByteBuffer* aData, TimeUnit aTimestampOffset) override;

  nsRefPtr<AppendPromise> BufferAppend() override;

  void AbortAppendData() override;

  void ResetParserState() override;

  nsRefPtr<RangeRemovalPromise> RangeRemoval(TimeUnit aStart, TimeUnit aEnd) override;

  EvictDataResult
  EvictData(TimeUnit aPlaybackTime, uint32_t aThreshold, TimeUnit* aBufferStartTime) override;

  void EvictBefore(TimeUnit aTime) override;

  TimeIntervals Buffered() override;

  int64_t GetSize() override;

  void Ended() override;

  void Detach() override;

  AppendState GetAppendState() override
  {
    return mAppendState;
  }

  void SetGroupStartTimestamp(const TimeUnit& aGroupStartTimestamp) override;
  void RestartGroupStartTimestamp() override;

  
  MediaInfo GetMetadata();
  const TrackBuffer& GetTrackBuffer(TrackInfo::TrackType aTrack);
  const TimeIntervals& Buffered(TrackInfo::TrackType);
  bool IsEnded() const
  {
    return mEnded;
  }

#if defined(DEBUG)
  void Dump(const char* aPath) override;
#endif

private:
  virtual ~TrackBuffersManager();
  
  nsRefPtr<AppendPromise> InitSegmentParserLoop();
  void ScheduleSegmentParserLoop();
  void SegmentParserLoop();
  void AppendIncomingBuffers();
  void InitializationSegmentReceived();
  void CreateDemuxerforMIMEType();
  void NeedMoreData();
  void RejectAppend(nsresult aRejectValue, const char* aName);
  
  
  nsRefPtr<CodedFrameProcessingPromise> CodedFrameProcessing();
  void CompleteCodedFrameProcessing();
  
  
  void FinishCodedFrameProcessing();
  void CompleteResetParserState();
  nsRefPtr<RangeRemovalPromise> CodedFrameRemovalWithPromise(TimeInterval aInterval);
  bool CodedFrameRemoval(TimeInterval aInterval);
  void SetAppendState(AppendState aAppendState);

  bool HasVideo() const
  {
    return mVideoTracks.mNumTracks > 0;
  }
  bool HasAudio() const
  {
    return mAudioTracks.mNumTracks > 0;
  }

  typedef Pair<nsRefPtr<MediaLargeByteBuffer>, TimeUnit> IncomingBuffer;
  void AppendIncomingBuffer(IncomingBuffer aData);
  nsTArray<IncomingBuffer> mIncomingBuffers;

  
  nsRefPtr<MediaLargeByteBuffer> mInputBuffer;
  
  
  Atomic<AppendState> mAppendState;
  
  
  
  Atomic<bool> mBufferFull;
  bool mFirstInitializationSegmentReceived;
  bool mActiveTrack;
  Maybe<TimeUnit> mGroupStartTimestamp;
  TimeUnit mGroupEndTimestamp;
  nsCString mType;

  
  

  
  
  void RecreateParser();
  nsAutoPtr<ContainerParser> mParser;

  
  nsRefPtr<SourceBufferResource> mCurrentInputBuffer;
  nsRefPtr<MediaDataDemuxer> mInputDemuxer;
  
  uint32_t mProcessedInput;

  void OnDemuxerInitDone(nsresult);
  void OnDemuxerInitFailed(DemuxerFailureReason aFailure);
  MediaPromiseRequestHolder<MediaDataDemuxer::InitPromise> mDemuxerInitRequest;
  bool mEncrypted;

  void OnDemuxFailed(TrackType aTrack, DemuxerFailureReason aFailure);
  void DoDemuxVideo();
  void OnVideoDemuxCompleted(nsRefPtr<MediaTrackDemuxer::SamplesHolder> aSamples);
  void OnVideoDemuxFailed(DemuxerFailureReason aFailure)
  {
    mVideoTracks.mDemuxRequest.Complete();
    OnDemuxFailed(TrackType::kVideoTrack, aFailure);
  }
  void DoDemuxAudio();
  void OnAudioDemuxCompleted(nsRefPtr<MediaTrackDemuxer::SamplesHolder> aSamples);
  void OnAudioDemuxFailed(DemuxerFailureReason aFailure)
  {
    mAudioTracks.mDemuxRequest.Complete();
    OnDemuxFailed(TrackType::kAudioTrack, aFailure);
  }

  void DoEvictData(const TimeUnit& aPlaybackTime, uint32_t aThreshold);

  struct TrackData {
    TrackData()
      : mNumTracks(0)
      , mNeedRandomAccessPoint(true)
      , mSizeBuffer(0)
    {}
    uint32_t mNumTracks;
    
    
    
    
    
    
    Maybe<TimeUnit> mLastDecodeTimestamp;
    
    
    
    
    Maybe<TimeUnit> mLastFrameDuration;
    
    
    
    
    
    Maybe<TimeUnit> mHighestEndTimestamp;
    
    Maybe<TimeUnit> mLongestFrameDuration;
    
    
    
    
    
    bool mNeedRandomAccessPoint;
    nsRefPtr<MediaTrackDemuxer> mDemuxer;
    MediaPromiseRequestHolder<MediaTrackDemuxer::SamplesPromise> mDemuxRequest;
    
    TrackBuffer mQueuedSamples;
    
    nsTArray<TrackBuffer> mBuffers;
    
    
    TimeIntervals mBufferedRanges;
    
    uint32_t mSizeBuffer;
    
    UniquePtr<TrackInfo> mInfo;
  };

  bool ProcessFrame(MediaRawData* aSample, TrackData& aTrackData);
  void RejectProcessing(nsresult aRejectValue, const char* aName);
  void ResolveProcessing(bool aResolveValue, const char* aName);
  MediaPromiseRequestHolder<CodedFrameProcessingPromise> mProcessingRequest;
  MediaPromiseHolder<CodedFrameProcessingPromise> mProcessingPromise;

  MediaPromiseHolder<AppendPromise> mAppendPromise;
  
  
  
  bool mAppendRunning;

  
  nsTArray<TrackData*> GetTracksList();
  TrackData& GetTracksData(TrackType aTrack)
  {
    switch(aTrack) {
      case TrackType::kVideoTrack:
        return mVideoTracks;
      case TrackType::kAudioTrack:
      default:
        return mAudioTracks;
    }
  }
  TrackData mVideoTracks;
  TrackData mAudioTracks;

  
  AbstractThread* GetTaskQueue() {
    return mTaskQueue;
  }
  bool OnTaskQueue()
  {
    return !GetTaskQueue() || GetTaskQueue()->IsCurrentThreadIn();
  }
  RefPtr<MediaTaskQueue> mTaskQueue;

  TimeUnit mTimestampOffset;
  TimeUnit mLastTimestampOffset;
  void RestoreCachedVariables();

  
  nsMainThreadPtrHandle<dom::SourceBuffer> mParent;
  nsMainThreadPtrHandle<MediaSourceDecoder> mParentDecoder;

  
  Atomic<bool> mAbort;
  
  Atomic<bool> mEnded;

  
  Atomic<int64_t> mSizeSourceBuffer;

  
  mutable Monitor mMonitor;
  
  TimeIntervals mVideoBufferedRanges;
  TimeIntervals mAudioBufferedRanges;
  
  MediaInfo mInfo;
};

} 
#endif 
