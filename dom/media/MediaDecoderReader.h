




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







class MediaDecoderReader {
public:
  enum NotDecodedReason {
    END_OF_STREAM,
    DECODE_ERROR,
    WAITING_FOR_DATA,
    CANCELED
  };

  typedef MediaPromise<nsRefPtr<AudioData>, NotDecodedReason> AudioDataPromise;
  typedef MediaPromise<nsRefPtr<VideoData>, NotDecodedReason> VideoDataPromise;
  typedef MediaPromise<bool, nsresult> SeekPromise;

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

  virtual bool OnDecodeThread()
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

  virtual bool HasAudio() = 0;
  virtual bool HasVideo() = 0;

  
  virtual void PreReadMetadata() {};

  
  
  
  
  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags) = 0;

  
  
  virtual void ReadUpdatedMetadata(MediaInfo* aInfo) { };

  
  
  
  virtual nsRefPtr<SeekPromise>
  Seek(int64_t aTime, int64_t aStartTime,
       int64_t aEndTime, int64_t aCurrentTime) = 0;

  
  
  
  
  
  
  
  
  
  
  virtual void SetIdle() { }

  
  
  
  void SetIgnoreAudioOutputFormat()
  {
    mIgnoreAudioOutputFormat = true;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual nsresult GetBuffered(dom::TimeRanges* aBuffered);

  virtual int64_t ComputeStartTime(const VideoData* aVideo, const AudioData* aAudio);

  
  
  
  virtual uint32_t GetBufferingWait() { return 30; }

  
  
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

  void ClearDecoder() {
    mDecoder = nullptr;
  }

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

private:
  
  
  MediaPromiseHolder<AudioDataPromise> mBaseAudioPromise;
  MediaPromiseHolder<VideoDataPromise> mBaseVideoPromise;

  nsRefPtr<MediaTaskQueue> mTaskQueue;
  bool mTaskQueueIsBorrowed;

  
  
  bool mAudioDiscontinuity;
  bool mVideoDiscontinuity;
  bool mShutdown;
};

} 

#endif
