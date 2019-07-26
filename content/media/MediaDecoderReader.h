




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





class MediaDecoderReader {
public:
  MediaDecoderReader(AbstractMediaDecoder* aDecoder);
  virtual ~MediaDecoderReader();

  
  
  virtual nsresult Init(MediaDecoderReader* aCloneDonor) = 0;

  
  virtual bool IsWaitingMediaResources() { return false; }
  
  virtual bool IsDormantNeeded() { return false; }
  
  virtual void ReleaseMediaResources() {};
  
  virtual void ReleaseDecoder() {};

  
  virtual nsresult ResetDecode();

  
  
  
  
  virtual bool DecodeAudioData() = 0;

  
  
  
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                int64_t aTimeThreshold) = 0;

  virtual bool HasAudio() = 0;
  virtual bool HasVideo() = 0;

  
  
  
  
  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags) = 0;

  
  
  
  virtual VideoData* FindStartTime(int64_t& aOutStartTime);

  
  
  
  virtual nsresult Seek(int64_t aTime,
                        int64_t aStartTime,
                        int64_t aEndTime,
                        int64_t aCurrentTime) = 0;

  
  
  
  virtual void OnDecodeThreadStart() {}

  
  
  
  
  virtual void OnDecodeThreadFinish() {}

  
  
  
  void SetIgnoreAudioOutputFormat()
  {
    mIgnoreAudioOutputFormat = true;
  }

protected:
  
  
  MediaQueue<AudioData> mAudioQueue;

  
  
  MediaQueue<VideoData> mVideoQueue;

  
  
  
  
  AudioCompactor mAudioCompactor;

public:
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual nsresult GetBuffered(dom::TimeRanges* aBuffered,
                               int64_t aStartTime);

  class VideoQueueMemoryFunctor : public nsDequeFunctor {
  public:
    VideoQueueMemoryFunctor() : mResult(0) {}

    virtual void* operator()(void* anObject);

    int64_t mResult;
  };

  virtual int64_t VideoQueueMemoryInUse() {
    VideoQueueMemoryFunctor functor;
    mVideoQueue.LockedForEach(functor);
    return functor.mResult;
  }

  class AudioQueueMemoryFunctor : public nsDequeFunctor {
  public:
    AudioQueueMemoryFunctor() : mSize(0) {}

    MOZ_DEFINE_MALLOC_SIZE_OF(MallocSizeOf);

    virtual void* operator()(void* anObject) {
      const AudioData* audioData = static_cast<const AudioData*>(anObject);
      mSize += audioData->SizeOfIncludingThis(MallocSizeOf);
      return nullptr;
    }

    size_t mSize;
  };

  size_t SizeOfAudioQueue() {
    AudioQueueMemoryFunctor functor;
    mAudioQueue.LockedForEach(functor);
    return functor.mSize;
  }

  
  
  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset) {}

  virtual MediaQueue<AudioData>& AudioQueue() { return mAudioQueue; }
  virtual MediaQueue<VideoData>& VideoQueue() { return mVideoQueue; }

  
  AbstractMediaDecoder* GetDecoder() {
    return mDecoder;
  }

  AudioData* DecodeToFirstAudioData();
  VideoData* DecodeToFirstVideoData();

protected:
  
  
  nsresult DecodeToTarget(int64_t aTarget);

  
  AbstractMediaDecoder* mDecoder;

  
  MediaInfo mInfo;

  
  
  
  bool mIgnoreAudioOutputFormat;
};

} 

#endif
