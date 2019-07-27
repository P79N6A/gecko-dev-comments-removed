




#if !defined(MediaDecoderReader_h_)
#define MediaDecoderReader_h_

#include "AbstractMediaDecoder.h"
#include "MediaInfo.h"
#include "MediaData.h"
#include "MediaPromise.h"
#include "MediaQueue.h"
#include "MediaTimer.h"
#include "AudioCompactor.h"
#include "Intervals.h"
#include "TimeUnits.h"

namespace mozilla {

class MediaDecoderReader;
class SharedDecoderManager;

struct WaitForDataRejectValue
{
  enum Reason {
    SHUTDOWN,
    CANCELED
  };

  WaitForDataRejectValue(MediaData::Type aType, Reason aReason)
    :mType(aType), mReason(aReason) {}
  MediaData::Type mType;
  Reason mReason;
};

class MetadataHolder
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MetadataHolder)
  MediaInfo mInfo;
  nsAutoPtr<MetadataTags> mTags;

private:
  virtual ~MetadataHolder() {}
};

enum class ReadMetadataFailureReason : int8_t
{
  WAITING_FOR_RESOURCES,
  METADATA_ERROR
};







class MediaDecoderReader {
public:
  enum NotDecodedReason {
    END_OF_STREAM,
    DECODE_ERROR,
    WAITING_FOR_DATA,
    CANCELED
  };

  typedef MediaPromise<nsRefPtr<MetadataHolder>, ReadMetadataFailureReason,  true> MetadataPromise;
  typedef MediaPromise<nsRefPtr<AudioData>, NotDecodedReason,  true> AudioDataPromise;
  typedef MediaPromise<nsRefPtr<VideoData>, NotDecodedReason,  true> VideoDataPromise;
  typedef MediaPromise<int64_t, nsresult,  true> SeekPromise;

  
  
  
  
  typedef MediaPromise<MediaData::Type, WaitForDataRejectValue,  true> WaitForDataPromise;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaDecoderReader)

  
  
  explicit MediaDecoderReader(AbstractMediaDecoder* aDecoder, MediaTaskQueue* aBorrowedTaskQueue = nullptr);

  
  
  
  
  void InitializationTask();

  
  
  virtual nsresult Init(MediaDecoderReader* aCloneDonor) = 0;

  
  virtual bool IsWaitingMediaResources() { return false; }
  
  
  virtual bool IsWaitingOnCDMResource() { return false; }
  
  
  virtual void ReleaseMediaResources() {};
  virtual void SetSharedDecoderManager(SharedDecoderManager* aManager) {}
  
  
  
  virtual void BreakCycles();

  
  
  
  
  virtual nsRefPtr<ShutdownPromise> Shutdown();

  virtual bool OnTaskQueue()
  {
    return TaskQueue()->IsCurrentThreadIn();
  }

  
  
  
  
  
  
  
  
  
  
  
  
  virtual nsresult ResetDecode();

  
  
  
  
  
  
  virtual nsRefPtr<AudioDataPromise> RequestAudioData();

  
  
  
  
  
  
  virtual nsRefPtr<VideoDataPromise>
  RequestVideoData(bool aSkipToNextKeyframe, int64_t aTimeThreshold, bool aForceDecodeAhead);

  friend class ReRequestVideoWithSkipTask;
  friend class ReRequestAudioTask;

  
  
  
  virtual bool IsWaitForDataSupported() { return false; }
  virtual nsRefPtr<WaitForDataPromise> WaitForData(MediaData::Type aType) { MOZ_CRASH(); }

  virtual bool HasAudio() = 0;
  virtual bool HasVideo() = 0;

  
  
  
  virtual nsRefPtr<MetadataPromise> AsyncReadMetadata();

  
  
  
  
  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags) { MOZ_CRASH(); }

  
  
  virtual void ReadUpdatedMetadata(MediaInfo* aInfo) { };

  
  
  
  virtual nsRefPtr<SeekPromise>
  Seek(int64_t aTime, int64_t aEndTime) = 0;

  
  
  
  
  
  
  
  
  
  
  virtual void SetIdle() { }

  
  
  
  void SetIgnoreAudioOutputFormat()
  {
    mIgnoreAudioOutputFormat = true;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual media::TimeIntervals GetBuffered();

  
  virtual void UpdateBuffered();

  
  virtual bool ForceZeroStartTime() const { return false; }

  
  
  
  
  virtual bool UseBufferingHeuristics() { return true; }

  
  
  size_t SizeOfVideoQueueInBytes() const;

  
  
  size_t SizeOfAudioQueueInBytes() const;

  virtual size_t SizeOfVideoQueueInFrames();
  virtual size_t SizeOfAudioQueueInFrames();

protected:
  friend class TrackBuffer;
  virtual void NotifyDataArrivedInternal(uint32_t aLength, int64_t aOffset) { }

  void NotifyDataArrived(const media::Interval<int64_t>& aInfo)
  {
    MOZ_ASSERT(OnTaskQueue());
    NS_ENSURE_TRUE_VOID(!mShutdown);
    NotifyDataArrivedInternal(aInfo.Length(), aInfo.mStart);
    UpdateBuffered();
  }

  
  void ThrottledNotifyDataArrived(const media::Interval<int64_t>& aInterval);
  void DoThrottledNotify();

public:
  
  
  
  
  
  
  void DispatchNotifyDataArrived(uint32_t aLength, int64_t aOffset, bool aThrottleUpdates)
  {
    RefPtr<nsRunnable> r =
      NS_NewRunnableMethodWithArg<media::Interval<int64_t>>(this, aThrottleUpdates ? &MediaDecoderReader::ThrottledNotifyDataArrived
                                                                                   : &MediaDecoderReader::NotifyDataArrived,
                                                            media::Interval<int64_t>(aOffset, aOffset + aLength));
    TaskQueue()->Dispatch(r.forget(), AbstractThread::DontAssertDispatchSuccess);
  }

  
  virtual void NotifyDataRemoved() {}
  virtual int64_t GetEvictionOffset(double aTime) { return -1; }

  virtual MediaQueue<AudioData>& AudioQueue() { return mAudioQueue; }
  virtual MediaQueue<VideoData>& VideoQueue() { return mVideoQueue; }

  
  AbstractMediaDecoder* GetDecoder() {
    return mDecoder;
  }

  nsRefPtr<VideoDataPromise> DecodeToFirstVideoData();

  MediaInfo GetMediaInfo() { return mInfo; }

  
  
  virtual bool IsMediaSeekable() = 0;

  void DispatchSetStartTime(int64_t aStartTime)
  {
    nsRefPtr<MediaDecoderReader> self = this;
    nsCOMPtr<nsIRunnable> r =
      NS_NewRunnableFunction([self, aStartTime] () -> void
    {
      MOZ_ASSERT(self->OnTaskQueue());
      MOZ_ASSERT(!self->HaveStartTime());
      self->mStartTime.emplace(aStartTime);
      self->UpdateBuffered();
    });
    TaskQueue()->Dispatch(r.forget());
  }

  MediaTaskQueue* TaskQueue() {
    return mTaskQueue;
  }

  
  
  
  
  virtual bool IsAsync() const { return false; }

  
  
  virtual bool VideoIsHardwareAccelerated() const { return false; }

  virtual void DisableHardwareAcceleration() {}

protected:
  virtual ~MediaDecoderReader();

  
  
  
  
  
  virtual bool DecodeAudioData() {
    return false;
  }

  
  
  
  
  
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip, int64_t aTimeThreshold) {
    return false;
  }

  
  
  MediaQueue<AudioData> mAudioQueue;

  
  
  MediaQueue<VideoData> mVideoQueue;

  
  
  
  
  AudioCompactor mAudioCompactor;

  
  AbstractMediaDecoder* mDecoder;

  
  nsRefPtr<MediaTaskQueue> mTaskQueue;

  
  WatchManager<MediaDecoderReader> mWatchManager;

  
  nsRefPtr<MediaTimer> mTimer;

  
  Canonical<media::TimeIntervals> mBuffered;
public:
  AbstractCanonical<media::TimeIntervals>* CanonicalBuffered() { return &mBuffered; }
protected:

  
  MediaInfo mInfo;

  
  Mirror<media::NullableTimeUnit> mDuration;

  
  MediaPromiseRequestHolder<MediaTimerPromise> mThrottledNotify;
  const TimeDuration mThrottleDuration;
  TimeStamp mLastThrottledNotify;
  Maybe<media::Interval<int64_t>> mThrottledInterval;

  
  
  
  bool mIgnoreAudioOutputFormat;

  
  
  
  
  
  
  
  
  
  
  Maybe<int64_t> mStartTime;
  bool HaveStartTime() { MOZ_ASSERT(OnTaskQueue()); return mStartTime.isSome(); }
  int64_t StartTime() { MOZ_ASSERT(HaveStartTime()); return mStartTime.ref(); }

  
  
  
  
  bool mHitAudioDecodeError;
  bool mShutdown;

private:
  
  
  MediaPromiseHolder<AudioDataPromise> mBaseAudioPromise;
  MediaPromiseHolder<VideoDataPromise> mBaseVideoPromise;

  bool mTaskQueueIsBorrowed;

  
  
  bool mAudioDiscontinuity;
  bool mVideoDiscontinuity;
};

} 

#endif
