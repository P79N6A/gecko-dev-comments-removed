




#if !defined(MediaDecoderReader_h_)
#define MediaDecoderReader_h_

#include "AbstractMediaDecoder.h"
#include "MediaInfo.h"
#include "MediaData.h"
#include "MediaPromise.h"
#include "MediaQueue.h"
#include "AudioCompactor.h"

namespace mozilla {

namespace dom {
class TimeRanges;
}

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

  
  
  explicit MediaDecoderReader(AbstractMediaDecoder* aDecoder);

  
  
  virtual nsresult Init(MediaDecoderReader* aCloneDonor) = 0;

  
  virtual bool IsWaitingMediaResources() { return false; }
  
  
  virtual bool IsWaitingOnCDMResource() { return false; }
  
  virtual bool IsDormantNeeded() { return false; }
  
  
  virtual void ReleaseMediaResources() {};
  virtual void SetSharedDecoderManager(SharedDecoderManager* aManager) {}
  
  
  
  virtual void BreakCycles();

  
  
  
  
  virtual nsRefPtr<ShutdownPromise> Shutdown();

  MediaTaskQueue* EnsureTaskQueue();

  virtual bool OnTaskQueue()
  {
    return !GetTaskQueue() || GetTaskQueue()->IsCurrentThreadIn();
  }

  void SetBorrowedTaskQueue(MediaTaskQueue* aTaskQueue)
  {
    MOZ_ASSERT(!mTaskQueue && aTaskQueue);
    mTaskQueue = aTaskQueue;
    mTaskQueueIsBorrowed = true;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  virtual nsresult ResetDecode();

  
  
  
  
  
  
  virtual nsRefPtr<AudioDataPromise> RequestAudioData();

  
  
  
  
  
  
  virtual nsRefPtr<VideoDataPromise>
  RequestVideoData(bool aSkipToNextKeyframe, int64_t aTimeThreshold);

  friend class ReRequestVideoWithSkipTask;
  friend class ReRequestAudioTask;

  
  
  
  virtual bool IsWaitForDataSupported() { return false; }
  virtual nsRefPtr<WaitForDataPromise> WaitForData(MediaData::Type aType) { MOZ_CRASH(); }

  virtual bool HasAudio() = 0;
  virtual bool HasVideo() = 0;

  
  
  
  virtual nsRefPtr<MetadataPromise> AsyncReadMetadata();

  
  virtual void PreReadMetadata() {};

  
  
  
  
  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags) = 0;

  
  
  virtual void ReadUpdatedMetadata(MediaInfo* aInfo) { };

  
  
  
  virtual nsRefPtr<SeekPromise>
  Seek(int64_t aTime, int64_t aEndTime) = 0;

  
  
  
  
  
  
  
  
  
  
  virtual void SetIdle() { }

  
  
  
  void SetIgnoreAudioOutputFormat()
  {
    mIgnoreAudioOutputFormat = true;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual nsresult GetBuffered(dom::TimeRanges* aBuffered);

  virtual int64_t ComputeStartTime(const VideoData* aVideo, const AudioData* aAudio);

  
  
  
  
  virtual bool UseBufferingHeuristics() { return true; }

  
  
  size_t SizeOfVideoQueueInBytes() const;

  
  
  size_t SizeOfAudioQueueInBytes() const;

  virtual size_t SizeOfVideoQueueInFrames();
  virtual size_t SizeOfAudioQueueInFrames();

  
  
  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset) {}
  virtual int64_t GetEvictionOffset(double aTime) { return -1; }

  virtual MediaQueue<AudioData>& AudioQueue() { return mAudioQueue; }
  virtual MediaQueue<VideoData>& VideoQueue() { return mVideoQueue; }

  
  AbstractMediaDecoder* GetDecoder() {
    return mDecoder;
  }

  
  VideoData* DecodeToFirstVideoData();

  MediaInfo GetMediaInfo() { return mInfo; }

  
  
  virtual bool IsMediaSeekable() = 0;
  void SetStartTime(int64_t aStartTime);

  MediaTaskQueue* GetTaskQueue() {
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

  
  MediaInfo mInfo;

  
  
  
  bool mIgnoreAudioOutputFormat;

  
  
  
  
  int64_t mStartTime;

  
  
  
  
  bool mHitAudioDecodeError;
  bool mShutdown;

private:
  
  
  MediaPromiseHolder<AudioDataPromise> mBaseAudioPromise;
  MediaPromiseHolder<VideoDataPromise> mBaseVideoPromise;

  nsRefPtr<MediaTaskQueue> mTaskQueue;
  bool mTaskQueueIsBorrowed;

  
  
  bool mAudioDiscontinuity;
  bool mVideoDiscontinuity;
};

} 

#endif
