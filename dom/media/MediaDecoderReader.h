




#if !defined(MediaDecoderReader_h_)
#define MediaDecoderReader_h_

#include "AbstractMediaDecoder.h"
#include "MediaInfo.h"
#include "MediaData.h"
#include "MediaQueue.h"
#include "AudioCompactor.h"

namespace mozilla {

namespace dom {
class TimeRanges;
}

class RequestSampleCallback;
class MediaDecoderReader;







class MediaDecoderReader {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaDecoderReader)

  explicit MediaDecoderReader(AbstractMediaDecoder* aDecoder);

  
  
  virtual nsresult Init(MediaDecoderReader* aCloneDonor) = 0;

  
  virtual bool IsWaitingMediaResources() { return false; }
  
  
  virtual bool IsWaitingOnCDMResource() { return false; }
  
  virtual bool IsDormantNeeded() { return false; }
  
  
  virtual void ReleaseMediaResources() {};
  
  
  
  virtual void BreakCycles();

  
  
  
  virtual void Shutdown();

  virtual void SetCallback(RequestSampleCallback* aDecodedSampleCallback);
  virtual void SetTaskQueue(MediaTaskQueue* aTaskQueue);

  
  
  
  
  
  
  
  
  
  virtual nsresult ResetDecode();

  
  
  
  
  
  virtual void RequestAudioData();

  
  
  
  
  
  
  
  virtual void RequestVideoData(bool aSkipToNextKeyframe,
                                int64_t aTimeThreshold);

  virtual bool HasAudio() = 0;
  virtual bool HasVideo() = 0;

  
  virtual void PreReadMetadata() {};

  
  
  
  
  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags) = 0;

  
  
  virtual void ReadUpdatedMetadata(MediaInfo* aInfo) { };

  
  
  
  
  
  virtual void Seek(int64_t aTime,
                    int64_t aStartTime,
                    int64_t aEndTime,
                    int64_t aCurrentTime) = 0;

  
  
  
  
  
  
  
  
  
  
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

  RequestSampleCallback* GetCallback() {
    MOZ_ASSERT(mSampleDecodedCallback);
    return mSampleDecodedCallback;
  }

  
  
  MediaQueue<AudioData> mAudioQueue;

  
  
  MediaQueue<VideoData> mVideoQueue;

  
  
  
  
  AudioCompactor mAudioCompactor;

  
  AbstractMediaDecoder* mDecoder;

  
  MediaInfo mInfo;

  
  
  
  bool mIgnoreAudioOutputFormat;

  
  
  
  
  int64_t mStartTime;
private:

  nsRefPtr<RequestSampleCallback> mSampleDecodedCallback;

  nsRefPtr<MediaTaskQueue> mTaskQueue;

  
  
  bool mAudioDiscontinuity;
  bool mVideoDiscontinuity;
};





class RequestSampleCallback {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(RequestSampleCallback)

  enum NotDecodedReason {
    END_OF_STREAM,
    DECODE_ERROR,
    WAITING_FOR_DATA,
    CANCELED
  };

  
  virtual void OnAudioDecoded(AudioData* aSample) = 0;

  
  virtual void OnVideoDecoded(VideoData* aSample) = 0;

  
  
  virtual void OnNotDecoded(MediaData::Type aType, NotDecodedReason aReason) = 0;

  virtual void OnSeekCompleted(nsresult aResult) = 0;

  
  virtual void BreakCycles() = 0;

protected:
  virtual ~RequestSampleCallback() {}
};





class AudioDecodeRendezvous : public RequestSampleCallback {
public:
  using RequestSampleCallback::NotDecodedReason;

  AudioDecodeRendezvous();
  ~AudioDecodeRendezvous();

  
  
  virtual void OnAudioDecoded(AudioData* aSample) MOZ_OVERRIDE;
  virtual void OnVideoDecoded(VideoData* aSample) MOZ_OVERRIDE {}
  virtual void OnNotDecoded(MediaData::Type aType, NotDecodedReason aReason) MOZ_OVERRIDE;
  virtual void OnSeekCompleted(nsresult aResult) MOZ_OVERRIDE {};
  virtual void BreakCycles() MOZ_OVERRIDE {};
  void Reset();

  
  
  nsresult Await(nsAutoPtr<AudioData>& aSample);

  
  void Cancel();

private:
  Monitor mMonitor;
  nsresult mStatus;
  nsAutoPtr<AudioData> mSample;
  bool mHaveResult;
};

} 

#endif
