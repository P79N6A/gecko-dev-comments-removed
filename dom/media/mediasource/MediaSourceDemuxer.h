





#if !defined(MediaSourceDemuxer_h_)
#define MediaSourceDemuxer_h_

#include "mozilla/Maybe.h"
#include "MediaDataDemuxer.h"
#include "MediaDecoderReader.h"
#include "MediaResource.h"
#include "MediaSource.h"
#include "MediaTaskQueue.h"
#include "TrackBuffersManager.h"
#include "mozilla/Atomics.h"
#include "mozilla/Monitor.h"

namespace mozilla {

class MediaSourceTrackDemuxer;

class MediaSourceDemuxer : public MediaDataDemuxer
{
public:
  explicit MediaSourceDemuxer();

  nsRefPtr<InitPromise> Init() override;

  bool IsThreadSafe() override { return true; }

  already_AddRefed<MediaDataDemuxer> Clone() const override
  {
    MOZ_CRASH("Shouldn't be called");
    return nullptr;
  }

  bool HasTrackType(TrackInfo::TrackType aType) const override;

  uint32_t GetNumberTracks(TrackInfo::TrackType aType) const override;

  already_AddRefed<MediaTrackDemuxer> GetTrackDemuxer(TrackInfo::TrackType aType,
                                                              uint32_t aTrackNumber) override;

  bool IsSeekable() const override;

  UniquePtr<EncryptionInfo> GetCrypto() override;

  bool ShouldComputeStartTime() const override { return false; }

  
  void AttachSourceBuffer(TrackBuffersManager* aSourceBuffer);
  void DetachSourceBuffer(TrackBuffersManager* aSourceBuffer);
  MediaTaskQueue* GetTaskQueue() { return mTaskQueue; }
  void NotifyTimeRangesChanged();

private:
  ~MediaSourceDemuxer();
  friend class MediaSourceTrackDemuxer;
  
  void ScanSourceBuffersForContent();
  void AttemptInit();
  TrackBuffersManager* GetManager(TrackInfo::TrackType aType);
  TrackInfo* GetTrackInfo(TrackInfo::TrackType);
  bool OnTaskQueue()
  {
    return !GetTaskQueue() || GetTaskQueue()->IsCurrentThreadIn();
  }

  RefPtr<MediaTaskQueue> mTaskQueue;
  nsTArray<nsRefPtr<MediaSourceTrackDemuxer>> mDemuxers;
  MediaPromiseHolder<InitPromise> mInitPromise;

  
  mutable Monitor mMonitor;
  MediaInfo mInfo;
  nsTArray<nsRefPtr<TrackBuffersManager>> mSourceBuffers;
  nsRefPtr<TrackBuffersManager> mAudioTrack;
  nsRefPtr<TrackBuffersManager> mVideoTrack;
};

class MediaSourceTrackDemuxer : public MediaTrackDemuxer
{
public:
  MediaSourceTrackDemuxer(MediaSourceDemuxer* aParent,
                          TrackInfo::TrackType aType,
                          TrackBuffersManager* aManager);

  UniquePtr<TrackInfo> GetInfo() const override;

  nsRefPtr<SeekPromise> Seek(media::TimeUnit aTime) override;

  nsRefPtr<SamplesPromise> GetSamples(int32_t aNumSamples = 1) override;

  void Reset() override;

  nsresult GetNextRandomAccessPoint(media::TimeUnit* aTime) override;

  nsRefPtr<SkipAccessPointPromise> SkipToNextRandomAccessPoint(media::TimeUnit aTimeThreshold) override;

  media::TimeIntervals GetBuffered() override;

  int64_t GetEvictionOffset(media::TimeUnit aTime) override;

  void BreakCycles() override;

  
  
  void NotifyTimeRangesChanged();

private:
  void DoReset();
  void DoSeek(const media::TimeUnit& aTime);
  void DoGetSamples(int32_t aNumSamples);
  void DoSkipToNextRandomAccessPoint(const TimeUnit& aTimeThreadshold);
  already_AddRefed<MediaRawData> GetSample(DemuxerFailureReason& aFailure);
  
  TimeUnit GetNextRandomAccessPoint();

  nsRefPtr<MediaSourceDemuxer> mParent;
  nsRefPtr<TrackBuffersManager> mManager;
  TrackInfo::TrackType mType;
  uint32_t mNextSampleIndex;
  media::TimeUnit mNextSampleTime;
  media::TimeUnit mNextRandomAccessPoint;
  MediaPromiseHolder<SeekPromise> mSeekPromise;
  MediaPromiseHolder<SamplesPromise> mSamplePromise;
  MediaPromiseHolder<SkipAccessPointPromise> mSkipPromise;
  Atomic<bool> mDetached;
};

} 

#endif
